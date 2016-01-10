
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


static void ngx_http_read_client_request_body_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_do_read_client_request_body(ngx_http_request_t *r);
static ngx_int_t ngx_http_write_request_body(ngx_http_request_t *r);
static ngx_int_t ngx_http_read_discarded_request_body(ngx_http_request_t *r);
static ngx_int_t ngx_http_discard_request_body_filter(ngx_http_request_t *r, ngx_buf_t *b);
static ngx_int_t ngx_http_test_expect(ngx_http_request_t *r);

static ngx_int_t ngx_http_request_body_filter(ngx_http_request_t *r, ngx_chain_t *in);
static ngx_int_t ngx_http_request_body_length_filter(ngx_http_request_t *r, ngx_chain_t *in);
static ngx_int_t ngx_http_request_body_chunked_filter(ngx_http_request_t *r, ngx_chain_t *in);

//ngx_http_read_client_request_body是一个异步方法，调用它只是说明要求Nginx开始接收请求的包体，并不表示是否已经接收完，
//当接收完所有的包体内容后，post_handler指向的回调方法会被调用。因此，即使在调用了ngx_http_read_client_request_body方法
//后它已经返回，也无法确定这时是否已经调用过post_handler指向的方法。
//注意: post_handler的返回类型是void，Nginx不会根据返回值做一些收尾工作，因此，我们在该方法里处理完请求时必须要主动调用
//ngx_http_finalize_request方法来结束请求。
ngx_int_t
ngx_http_read_client_request_body(ngx_http_request_t *r, ngx_http_client_body_handler_pt post_handler)
{
    size_t                     preread;
    ssize_t                    size;
    ngx_int_t                  rc;
    ngx_buf_t                 *b;
    ngx_chain_t                out, *cl;
    ngx_http_request_body_t   *rb;
    ngx_http_core_loc_conf_t  *clcf;

	//将该请求对应的主请求的引用计数加1。这同时是在要求每一个HTTP模块在传入的post_handler方法被回调时，
	//务必调用类似ngx_http_finlize_request的方法去结束请求，否则引用计数会始终无法清零，从而导致请求无法释放。
    r->main->count++;

#if (NGX_HTTP_V2)
    if (r->stream && r == r->main) 
	{
        r->request_body_no_buffering = 0;
        rc = ngx_http_v2_read_request_body(r, post_handler);
        goto done;
    }
#endif
	//检查当前请求是主请求还是子请求，对于子请求而言，它不是来自客户端的请求，所以不存在处理HTTP请求包体的概念
	//检查请求ngx_http_request_t结构体中的request_body成员，如果它已经被分配过了，证明已经读取过HTTP包体了，不需要再次读取一遍;
	//检查请求ngx_http_request_t结构体中的discard_body成员，如果discard_body为1，则证明曾经执行过丢弃包体的方法，现在包体正在被丢弃中
    if (r != r->main || r->request_body || r->discard_body)
	{
        r->request_body_no_buffering = 0;
        post_handler(r);
        return NGX_OK;
    }

	//现在真正需要接收HTTP包体....
	
    if (ngx_http_test_expect(r) != NGX_OK)
	{
        rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
        goto done;
    }

    if (r->request_body_no_buffering) 
	{
        r->request_body_in_file_only = 0;
    }

	//分配请求ngx_http_request_t的结构体中的request_body成员(之前是NULL空指针)，准备接收包体
    rb = ngx_pcalloc(r->pool, sizeof(ngx_http_request_body_t));
    if (rb == NULL)
	{
        rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
        goto done;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     rb->bufs = NULL;
     *     rb->buf = NULL;
     *     rb->free = NULL;
     *     rb->busy = NULL;
     *     rb->chunked = NULL;
     */

    rb->rest = -1;
    rb->post_handler = post_handler;

    r->request_body = rb;

	//检查请求中的content-length头部和chunked头部
    if (r->headers_in.content_length_n < 0 && !r->headers_in.chunked) 
	{
        r->request_body_no_buffering = 0;
        post_handler(r);
        return NGX_OK;
    }

	//接收HTTP头部的流程中，是有可能接收到HTTP包体的
    preread = r->header_in->last - r->header_in->pos;

    if (preread) 
	{

        /* there is the pre-read part of the request body */

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http client request body preread %uz", preread);

        out.buf = r->header_in;
        out.next = NULL;

        rc = ngx_http_request_body_filter(r, &out);

        if (rc != NGX_OK) 
		{
            goto done;
        }

        r->request_length += preread - (r->header_in->last - r->header_in->pos);

		//检查请求体长度是否以contend-length方式记录
		//检查在header_in缓冲区里已经接收到的包体长度是否小于content-length头部指定的长度
		//检查header_in缓冲区里的剩余空间是否可以存放下全部的包体，
		//如果可以，就不用分配新的包体缓冲区浪费内存了，直接将header_in剩余空间作为接收包体的缓冲区
        if (!r->headers_in.chunked && rb->rest > 0 && rb->rest <= (off_t) (r->header_in->end - r->header_in->last))
        {
            /* the whole request body may be placed in r->header_in */
            b = ngx_calloc_buf(r->pool);
            if (b == NULL) 
			{
                rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
                goto done;
            }

            b->temporary = 1;
            b->start = r->header_in->pos;
            b->pos = r->header_in->pos;
            b->last = r->header_in->last;
            b->end = r->header_in->end;

            rb->buf = b;

            r->read_event_handler = ngx_http_read_client_request_body_handler;
            r->write_event_handler = ngx_http_request_empty_handler;

            rc = ngx_http_do_read_client_request_body(r);
            goto done;
        }

    }
	else 
	{
        /* set rb->rest */

        if (ngx_http_request_body_filter(r, NULL) != NGX_OK)
		{
            rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
            goto done;
        }
    }

	//检查在header_in缓冲区中已经接收到的包体数据是否就是完整的包体数据
    if (rb->rest == 0)
	{
        /* the whole request body was pre-read */

        if (r->request_body_in_file_only) 
		{
            if (ngx_http_write_request_body(r) != NGX_OK)
			{
                rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
                goto done;
            }

            if (rb->temp_file->file.offset != 0) 
			{

                cl = ngx_chain_get_free_buf(r->pool, &rb->free);
                if (cl == NULL) {
                    rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
                    goto done;
                }

                b = cl->buf;

                ngx_memzero(b, sizeof(ngx_buf_t));

                b->in_file = 1;
                b->file_last = rb->temp_file->file.offset;
                b->file = &rb->temp_file->file;

                rb->bufs = cl;

            } else {
                rb->bufs = NULL;
            }
        }

        r->request_body_no_buffering = 0;

        post_handler(r);

        return NGX_OK;
    }

    if (rb->rest < 0) 
	{
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "negative request body rest");
        rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
        goto done;
    }

	//分配用于接收包体的缓冲区
    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    size = clcf->client_body_buffer_size;
    size += size >> 2;

    /* TODO: honor r->request_body_in_single_buf */

    if (!r->headers_in.chunked && rb->rest < size) 
	{
        size = (ssize_t) rb->rest;

        if (r->request_body_in_single_buf) 
		{
            size += preread;
        }

    }
	else 
	{
        size = clcf->client_body_buffer_size;
    }

    rb->buf = ngx_create_temp_buf(r->pool, size);
    if (rb->buf == NULL)
	{
        rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
        goto done;
    }

    r->read_event_handler = ngx_http_read_client_request_body_handler;
    r->write_event_handler = ngx_http_request_empty_handler;

	//调用ngx_http_do_read_client_request_body方法接收包体。该方法的意义在于把客户端与Nginx之间TCP连接上
	//套接字缓冲区中的当前字符流全部读出来，并判断是否需要写入文件，以及是否接收到全部包体，同时在接收到
	//完整的包体后激活post_handler回调方法
    rc = ngx_http_do_read_client_request_body(r);

done:

    if (r->request_body_no_buffering && (rc == NGX_OK || rc == NGX_AGAIN))
    {
        if (rc == NGX_OK)
		{
            r->request_body_no_buffering = 0;

        } 
		else 
		{
            /* rc == NGX_AGAIN */
            r->reading_body = 1;
        }

        r->read_event_handler = ngx_http_block_reading;
        post_handler(r);
    }

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) 
	{
        r->main->count--;
    }

    return rc;
}


ngx_int_t
ngx_http_read_unbuffered_request_body(ngx_http_request_t *r)
{
    ngx_int_t  rc;

    if (r->connection->read->timedout) {
        r->connection->timedout = 1;
        return NGX_HTTP_REQUEST_TIME_OUT;
    }

    rc = ngx_http_do_read_client_request_body(r);

    if (rc == NGX_OK)
	{
        r->reading_body = 0;
    }

    return rc;
}


static void
ngx_http_read_client_request_body_handler(ngx_http_request_t *r)
{
    ngx_int_t  rc;

	//检查是否接收HTTP包体超时
    if (r->connection->read->timedout) 
	{
        r->connection->timedout = 1;
        ngx_http_finalize_request(r, NGX_HTTP_REQUEST_TIME_OUT);
        return;
    }

    rc = ngx_http_do_read_client_request_body(r);

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) 
	{
        ngx_http_finalize_request(r, rc);
    }
}


static ngx_int_t
ngx_http_do_read_client_request_body(ngx_http_request_t *r)
{
    off_t                      rest;
    size_t                     size;
    ssize_t                    n;
    ngx_int_t                  rc;
    ngx_buf_t                 *b;
    ngx_chain_t               *cl, out;
    ngx_connection_t          *c;
    ngx_http_request_body_t   *rb;
    ngx_http_core_loc_conf_t  *clcf;

    c = r->connection;
    rb = r->request_body;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http read client request body");

    for ( ;; ) 
	{
        for ( ;; )
		{
			//检查请求的request_body成员中的buf缓冲区是否有空闲的空间
            if (rb->buf->last == rb->buf->end) 
			{
				//request_body成员中的buf缓冲区没有空闲的空间
				//检查request_body 成员中的buf缓冲区中是否有数据
                if (rb->buf->pos != rb->buf->last) 
				{
					
                    /* pass buffer to request body filter chain */
                    out.buf = rb->buf;
                    out.next = NULL;

                    rc = ngx_http_request_body_filter(r, &out);

                    if (rc != NGX_OK) 
					{
                        return rc;
                    }

                }
				else 
				{

                    /* update chains */
                    rc = ngx_http_request_body_filter(r, NULL);

                    if (rc != NGX_OK) 
					{
                        return rc;
                    }
                }

                if (rb->busy != NULL) 
				{
                    if (r->request_body_no_buffering) 
					{
                        if (c->read->timer_set) 
						{
                            ngx_del_timer(c->read);
                        }

                        if (ngx_handle_read_event(c->read, 0) != NGX_OK)
						{
                            return NGX_HTTP_INTERNAL_SERVER_ERROR;
                        }

                        return NGX_AGAIN;
                    }

                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                }

                rb->buf->pos = rb->buf->start;
                rb->buf->last = rb->buf->start;
            }


			//请求的request_body成员中的buf缓冲区有空闲的空间，直接读取内核中套接字缓冲区里的TCP字符流
			//buffer中剩余空间大小
            size = rb->buf->end - rb->buf->last;
			//request body 剩余大小
            rest = rb->rest - (rb->buf->last - rb->buf->pos);

            if ((off_t) size > rest) 
			{
                size = (size_t) rest;
            }

            n = c->recv(c, rb->buf->last, size);

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "http client request body recv %z", n);

            if (n == NGX_AGAIN) 
			{
                break;
            }

            if (n == 0) 
			{
                ngx_log_error(NGX_LOG_INFO, c->log, 0, "client prematurely closed connection");
            }

			//recv方法返回错误或者客户端主动关闭了连接
            if (n == 0 || n == NGX_ERROR) 
			{
                c->error = 1;
                return NGX_HTTP_BAD_REQUEST;
            }

            rb->buf->last += n;
            r->request_length += n;

            if (n == rest) 
			{
                /* pass buffer to request body filter chain */

                out.buf = rb->buf;
                out.next = NULL;

                rc = ngx_http_request_body_filter(r, &out);

                if (rc != NGX_OK) 
				{
                    return rc;
                }
            }

			//根据rest成员检查是否接收到完整的包体
            if (rb->rest == 0) 
			{
                break;
            }

            if (rb->buf->last < rb->buf->end)
			{
                break;
            }
        }

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "http client request body rest %O", rb->rest);

        if (rb->rest == 0) 
		{
            break;
        }

		//如果当前没有可读的字符流，同时还没有接收到完整的包体，则说明需要把读事件添加到事件模块，
		//等待可读事件发生时，事件框架可以再次调度到这个方法接收包体
        if (!c->read->ready)
		{

            if (r->request_body_no_buffering && rb->buf->pos != rb->buf->last)
            {
                /* pass buffer to request body filter chain */

                out.buf = rb->buf;
                out.next = NULL;

                rc = ngx_http_request_body_filter(r, &out);

                if (rc != NGX_OK) 
				{
                    return rc;
                }
            }

			//将读事件添加到定时器中
            clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
            ngx_add_timer(c->read, clcf->client_body_timeout);

			//将读事件添加到epoll事件机制中
            if (ngx_handle_read_event(c->read, 0) != NGX_OK) 
			{
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }

            return NGX_AGAIN;
        }
    }

	//到此处，表明已经接收到完整的包体，需要做一些收尾的工作了。

	//不需要检查是否接收HTTP包体超时了，要把读事件从定时器中取出，防止不必要的定时器触发
    if (c->read->timer_set) 
	{
        ngx_del_timer(c->read);
    }

	//如果缓冲区还有未写入文件的内容，调用ngx_http_write_request_body方法把最后的包体写入文件
    if (rb->temp_file || r->request_body_in_file_only) 
	{

        /* save the last part */

        if (ngx_http_write_request_body(r) != NGX_OK)
		{
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        if (rb->temp_file->file.offset != 0)
		{

            cl = ngx_chain_get_free_buf(r->pool, &rb->free);
            if (cl == NULL) {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }

            b = cl->buf;

            ngx_memzero(b, sizeof(ngx_buf_t));

            b->in_file = 1;
            b->file_last = rb->temp_file->file.offset;
            b->file = &rb->temp_file->file;

            rb->bufs = cl;

        } else {
            rb->bufs = NULL;
        }
    }

    if (!r->request_body_no_buffering) 
	{
		//已经接收到完整的包体了，就会把read_event_handler设为ngx_http_block_reading方法，表示连接上再有读事件将不做任何处理
        r->read_event_handler = ngx_http_block_reading;
		//执行HTTP模块提供的post_handler回调方法
        rb->post_handler(r);
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_write_request_body(ngx_http_request_t *r)
{
    ssize_t                    n;
    ngx_chain_t               *cl, *ln;
    ngx_temp_file_t           *tf;
    ngx_http_request_body_t   *rb;
    ngx_http_core_loc_conf_t  *clcf;

    rb = r->request_body;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http write client request body, bufs %p", rb->bufs);

    if (rb->temp_file == NULL) 
	{
        tf = ngx_pcalloc(r->pool, sizeof(ngx_temp_file_t));
        if (tf == NULL)
		{
            return NGX_ERROR;
        }

        clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

        tf->file.fd = NGX_INVALID_FILE;
        tf->file.log = r->connection->log;
        tf->path = clcf->client_body_temp_path;
        tf->pool = r->pool;
        tf->warn = "a client request body is buffered to a temporary file";
        tf->log_level = r->request_body_file_log_level;
        tf->persistent = r->request_body_in_persistent_file;
        tf->clean = r->request_body_in_clean_file;

        if (r->request_body_file_group_access) 
		{
            tf->access = 0660;
        }

        rb->temp_file = tf;

        if (rb->bufs == NULL) 
		{
            /* empty body with r->request_body_in_file_only */

            if (ngx_create_temp_file(&tf->file, tf->path, tf->pool, tf->persistent, tf->clean, tf->access) != NGX_OK)
            {
                return NGX_ERROR;
            }

            return NGX_OK;
        }
    }

    if (rb->bufs == NULL)
	{
        return NGX_OK;
    }

    n = ngx_write_chain_to_temp_file(rb->temp_file, rb->bufs);

    /* TODO: n == 0 or not complete and level event */

    if (n == NGX_ERROR) 
	{
        return NGX_ERROR;
    }

    rb->temp_file->offset += n;

    /* mark all buffers as written */

    for (cl = rb->bufs; cl; /* void */) 
	{

        cl->buf->pos = cl->buf->last;

        ln = cl;
        cl = cl->next;
        ngx_free_chain(r->pool, ln);
    }

    rb->bufs = NULL;

    return NGX_OK;
}

//对于HTTP模块而言，放弃接收包体就是简单地不处理包体了，可是对于HTTP框架而言，并不是不接收包体就可以的。
//因为对于客户端而言，通常会调用一些阻塞的发送方法来发送包体，如果HTTP框架一直不接收包体，会导致实现上
//不够健壮的客户端认为服务器超时无响应，因而简单的关闭连接，可这时Nginx可能还在处理这个连接。因此，HTTP
//模块中的放弃接收包体，对HTTP框架而言就是接收包体，但是接收后不做保存，直接丢弃。

//因为有些客户端可能会一直试图发送包体，而如果HTTP模块不接收发来的TCP流，有可能造成客户端发送超时。
//所以如果不想处理请求中的包体，那么可以调用ngx_http_discard_request_body方法将接收自客户端的HTTP包体丢弃掉
ngx_int_t
ngx_http_discard_request_body(ngx_http_request_t *r)
{
    ssize_t       size;
    ngx_int_t     rc;
    ngx_event_t  *rev;

#if (NGX_HTTP_V2)
    if (r->stream && r == r->main) 
	{
        r->stream->skip_data = NGX_HTTP_V2_DATA_DISCARD;
        return NGX_OK;
    }
#endif
	//检查当前请求是主请求还是子请求，对于子请求而言，它不是来自客户端的请求，所以不存在处理HTTP请求包体的概念
	//检查请求ngx_http_request_t结构体中的request_body成员，如果它已经被分配过了，证明已经读取过HTTP包体了，不需要再次读取一遍;
	//检查请求ngx_http_request_t结构体中的discard_body成员，如果discard_body为1，则证明曾经执行过丢弃包体的方法，现在包体正在被丢弃中
    if (r != r->main || r->discard_body || r->request_body) 
	{
        return NGX_OK;
    }

    if (ngx_http_test_expect(r) != NGX_OK) 
	{
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    rev = r->connection->read;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, rev->log, 0, "http set discard body");

	//检查请求连接上的读事件是否在定时器中，这是因为丢弃包体不用考虑超时问题(linger_timer例外)
    if (rev->timer_set) 
	{
        ngx_del_timer(rev);
    }

    if (r->headers_in.content_length_n <= 0 && !r->headers_in.chunked)
	{
        return NGX_OK;
    }

	
    size = r->header_in->last - r->header_in->pos;
	//检查接收HTTP头部时，是否已经接收到完整的HTTP包体
    if (size || r->headers_in.chunked)
	{
        rc = ngx_http_discard_request_body_filter(r, r->header_in);

        if (rc != NGX_OK) 
		{
            return rc;
        }

        if (r->headers_in.content_length_n == 0)
		{
			//接收HTTP头部时，已经接收到完整的HTTP包体
            return NGX_OK;
        }
    }

    rc = ngx_http_read_discarded_request_body(r);

	//rc == NGX_OK表示已经接收到完整的包体
    if (rc == NGX_OK)
	{
		//将请求的lingering_close延时关闭标志位设为0，表示不需要为了包体的接收而延时关闭了
        r->lingering_close = 0;
        return NGX_OK;
    }

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) 
	{
        return rc;
    }

    //rc == NGX_AGAIN，表示需要Nginx事件框架多次调度才能完成丢弃包体这一动作
	
    r->read_event_handler = ngx_http_discarded_request_body_handler;

    if (ngx_handle_read_event(rev, 0) != NGX_OK) 
	{
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

	//把引用计数加1，防止这边还在丢弃包体，而其他事件却已让请求意外销毁，引发严重错误
    r->count++;
	//将ngx_http_request_t结构体的discard_body标志位置1，表示正在丢弃包体
    r->discard_body = 1;

    return NGX_OK;
}


void
ngx_http_discarded_request_body_handler(ngx_http_request_t *r)
{
    ngx_int_t                  rc;
    ngx_msec_t                 timer;
    ngx_event_t               *rev;
    ngx_connection_t          *c;
    ngx_http_core_loc_conf_t  *clcf;

    c = r->connection;
    rev = c->read;

    if (rev->timedout)
	{
        c->timedout = 1;
        c->error = 1;
        ngx_http_finalize_request(r, NGX_ERROR);
        return;
    }

    if (r->lingering_time)
	{
        timer = (ngx_msec_t) r->lingering_time - (ngx_msec_t) ngx_time();

        if ((ngx_msec_int_t) timer <= 0)
		{
            r->discard_body = 0;
            r->lingering_close = 0;
            ngx_http_finalize_request(r, NGX_ERROR);
            return;
        }

    } 
	else
	{
        timer = 0;
    }

    rc = ngx_http_read_discarded_request_body(r);

	//rc == NGX_OK，表示已经成功丢弃完所有包体
    if (rc == NGX_OK) 
	{
        r->discard_body = 0;
        r->lingering_close = 0;
        ngx_http_finalize_request(r, NGX_DONE);
        return;
    }

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE)
	{
        c->error = 1;
        ngx_http_finalize_request(r, NGX_ERROR);
        return;
    }

    /* rc == NGX_AGAIN */

    if (ngx_handle_read_event(rev, 0) != NGX_OK) 
	{
        c->error = 1;
        ngx_http_finalize_request(r, NGX_ERROR);
        return;
    }

    if (timer) 
	{

        clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

        timer *= 1000;

        if (timer > clcf->lingering_timeout) 
		{
            timer = clcf->lingering_timeout;
        }

        ngx_add_timer(rev, timer);
    }
}



//返回值:
//	NGX_OK -- 已经丢弃了所有包体
//	NGX_AGAIN -- 需要等待读事件的触发，等待Nginx框架的再次调度
static ngx_int_t
ngx_http_read_discarded_request_body(ngx_http_request_t *r)
{
    size_t     size;
    ssize_t    n;
    ngx_int_t  rc;
    ngx_buf_t  b;
    u_char     buffer[NGX_HTTP_DISCARD_BUFFER_SIZE];

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http read discarded body");

    ngx_memzero(&b, sizeof(ngx_buf_t));

    b.temporary = 1;

	//丢弃包体时请求的request_body成员实际上是NULL空指针，这时使用了请求ngx_http_request_t的结构headers_in成员里面的content_length_n
	//来表示已经丢弃的包体有多大，最初它等于content_length头部，而每丢弃一部分包体，就会在content_length_n变量中减去
	//相应的大小。因此content_length_n表示还需要丢弃的包体长度
    for ( ;; ) 
	{
		//content_length_n为0，表示已经接收到完整的包体
        if (r->headers_in.content_length_n == 0) 
		{
			//将read_event_handler重置为ngx_http_block_reading，表示如果再有可读事件触发，不做任何处理
            r->read_event_handler = ngx_http_block_reading;
			//
            return NGX_OK;
        }

		//套接字缓冲区上没有内容可读，则直接返回NGX_ANGIN，告诉上层方法需要等待读事件的出发，等待Nginx框架的再次调度
        if (!r->connection->read->ready) 
		{
            return NGX_AGAIN;
        }

        size = (size_t) ngx_min(r->headers_in.content_length_n, NGX_HTTP_DISCARD_BUFFER_SIZE);

        n = r->connection->recv(r->connection, buffer, size);

        if (n == NGX_ERROR) 
		{
            r->connection->error = 1;
            return NGX_OK;
        }
		
		//套接字缓冲区上没有内容可读，则直接返回NGX_ANGIN，告诉上层方法需要等待读事件的出发，等待Nginx框架的再次调度
        if (n == NGX_AGAIN) 
		{
            return NGX_AGAIN;
        }

		//客户端主动关闭了连接，直接返回NGX_OK告诉上层方法结束丢弃包体动作即可
        if (n == 0) 
		{
            return NGX_OK;
        }

        b.pos = buffer;
        b.last = buffer + n;

        rc = ngx_http_discard_request_body_filter(r, &b);

        if (rc != NGX_OK)
		{
            return rc;
        }
    }
}


static ngx_int_t
ngx_http_discard_request_body_filter(ngx_http_request_t *r, ngx_buf_t *b)
{
    size_t                    size;
    ngx_int_t                 rc;
    ngx_http_request_body_t  *rb;

    if (r->headers_in.chunked) 
	{

        rb = r->request_body;

        if (rb == NULL)
		{

            rb = ngx_pcalloc(r->pool, sizeof(ngx_http_request_body_t));
            if (rb == NULL)
			{
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }

            rb->chunked = ngx_pcalloc(r->pool, sizeof(ngx_http_chunked_t));
            if (rb->chunked == NULL) 
			{
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }

            r->request_body = rb;
        }

        for ( ;; ) 
		{

            rc = ngx_http_parse_chunked(r, b, rb->chunked);

            if (rc == NGX_OK) {

                /* a chunk has been parsed successfully */

                size = b->last - b->pos;

                if ((off_t) size > rb->chunked->size) {
                    b->pos += (size_t) rb->chunked->size;
                    rb->chunked->size = 0;

                } else {
                    rb->chunked->size -= size;
                    b->pos = b->last;
                }

                continue;
            }

            if (rc == NGX_DONE) {

                /* a whole response has been parsed successfully */

                r->headers_in.content_length_n = 0;
                break;
            }

            if (rc == NGX_AGAIN) {

                /* set amount of data we want to see next time */

                r->headers_in.content_length_n = rb->chunked->length;
                break;
            }

            /* invalid */

            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "client sent invalid chunked body");

            return NGX_HTTP_BAD_REQUEST;
        }

    } 
	else 
	{
        size = b->last - b->pos;

        if ((off_t) size > r->headers_in.content_length_n) 
		{
            b->pos += (size_t) r->headers_in.content_length_n;
            r->headers_in.content_length_n = 0;

        } 
		else 
		{
            b->pos = b->last;
            r->headers_in.content_length_n -= size;
        }
    }

    return NGX_OK;
}

//检查客户端是否发送了Expect: 100-continue头，是的话则给客户端回复”HTTP/1.1 100 Continue”，根据http 1.1协议，
//客户端可以发送一个Expect头来向服务器表明期望发送请求体，服务器如果允许客户端发送请求体，则会回复”HTTP/1.1 100 Continue”，
//客户端收到时，才会开始发送请求体
static ngx_int_t
ngx_http_test_expect(ngx_http_request_t *r)
{
    ngx_int_t   n;
    ngx_str_t  *expect;

    if (r->expect_tested || r->headers_in.expect == NULL || r->http_version < NGX_HTTP_VERSION_11)
    {
        return NGX_OK;
    }

    r->expect_tested = 1;

    expect = &r->headers_in.expect->value;

    if (expect->len != sizeof("100-continue") - 1 || ngx_strncasecmp(expect->data, (u_char *) "100-continue", sizeof("100-continue") - 1) != 0)
    {
        return NGX_OK;
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "send 100 Continue");

    n = r->connection->send(r->connection, (u_char *) "HTTP/1.1 100 Continue" CRLF CRLF, sizeof("HTTP/1.1 100 Continue" CRLF CRLF) - 1);

    if (n == sizeof("HTTP/1.1 100 Continue" CRLF CRLF) - 1) 
	{
        return NGX_OK;
    }

    /* we assume that such small packet should be send successfully */

    return NGX_ERROR;
}


static ngx_int_t
ngx_http_request_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    if (r->headers_in.chunked) 
	{
        return ngx_http_request_body_chunked_filter(r, in);

    } 
	else 
	{
        return ngx_http_request_body_length_filter(r, in);
    }
}


static ngx_int_t
ngx_http_request_body_length_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    size_t                     size;
    ngx_int_t                  rc;
    ngx_buf_t                 *b;
    ngx_chain_t               *cl, *tl, *out, **ll;
    ngx_http_request_body_t   *rb;

    rb = r->request_body;

    if (rb->rest == -1) 
	{
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http request body content length filter");

        rb->rest = r->headers_in.content_length_n;
    }

    out = NULL;
    ll = &out;

    for (cl = in; cl; cl = cl->next) 
	{

        if (rb->rest == 0) 
		{
            break;
        }

        tl = ngx_chain_get_free_buf(r->pool, &rb->free);
        if (tl == NULL) 
		{
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        b = tl->buf;

        ngx_memzero(b, sizeof(ngx_buf_t));

        b->temporary = 1;
        b->tag = (ngx_buf_tag_t) &ngx_http_read_client_request_body;
        b->start = cl->buf->pos;
        b->pos = cl->buf->pos;
        b->last = cl->buf->last;
        b->end = cl->buf->end;
        b->flush = r->request_body_no_buffering;

        size = cl->buf->last - cl->buf->pos;

        if ((off_t) size < rb->rest) 
		{
            cl->buf->pos = cl->buf->last;
            rb->rest -= size;

        } 
		else 
		{
            cl->buf->pos += (size_t) rb->rest;
            rb->rest = 0;
            b->last = cl->buf->pos;
            b->last_buf = 1;
        }

        *ll = tl;
        ll = &tl->next;
    }

    rc = ngx_http_top_request_body_filter(r, out);

    ngx_chain_update_chains(r->pool, &rb->free, &rb->busy, &out, (ngx_buf_tag_t) &ngx_http_read_client_request_body);

    return rc;
}


static ngx_int_t
ngx_http_request_body_chunked_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    size_t                     size;
    ngx_int_t                  rc;
    ngx_buf_t                 *b;
    ngx_chain_t               *cl, *out, *tl, **ll;
    ngx_http_request_body_t   *rb;
    ngx_http_core_loc_conf_t  *clcf;

    rb = r->request_body;

    if (rb->rest == -1) 
	{

        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http request body chunked filter");

        rb->chunked = ngx_pcalloc(r->pool, sizeof(ngx_http_chunked_t));
        if (rb->chunked == NULL) 
		{
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        r->headers_in.content_length_n = 0;
        rb->rest = 3;
    }

    out = NULL;
    ll = &out;

    for (cl = in; cl; cl = cl->next)
	{

        for ( ;; ) {

            ngx_log_debug7(NGX_LOG_DEBUG_EVENT, r->connection->log, 0,
                           "http body chunked buf "
                           "t:%d f:%d %p, pos %p, size: %z file: %O, size: %O",
                           cl->buf->temporary, cl->buf->in_file,
                           cl->buf->start, cl->buf->pos,
                           cl->buf->last - cl->buf->pos,
                           cl->buf->file_pos,
                           cl->buf->file_last - cl->buf->file_pos);

            rc = ngx_http_parse_chunked(r, cl->buf, rb->chunked);

            if (rc == NGX_OK) 
			{

                /* a chunk has been parsed successfully */

                clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

                if (clcf->client_max_body_size && 
					clcf->client_max_body_size  - r->headers_in.content_length_n < rb->chunked->size)
                {
                    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "client intended to send too large chunked "
                                  "body: %O+%O bytes", r->headers_in.content_length_n, rb->chunked->size);

                    r->lingering_close = 1;

                    return NGX_HTTP_REQUEST_ENTITY_TOO_LARGE;
                }

                tl = ngx_chain_get_free_buf(r->pool, &rb->free);
                if (tl == NULL) {
                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                }

                b = tl->buf;

                ngx_memzero(b, sizeof(ngx_buf_t));

                b->temporary = 1;
                b->tag = (ngx_buf_tag_t) &ngx_http_read_client_request_body;
                b->start = cl->buf->pos;
                b->pos = cl->buf->pos;
                b->last = cl->buf->last;
                b->end = cl->buf->end;
                b->flush = r->request_body_no_buffering;

                *ll = tl;
                ll = &tl->next;

                size = cl->buf->last - cl->buf->pos;

                if ((off_t) size > rb->chunked->size) {
                    cl->buf->pos += (size_t) rb->chunked->size;
                    r->headers_in.content_length_n += rb->chunked->size;
                    rb->chunked->size = 0;

                } else {
                    rb->chunked->size -= size;
                    r->headers_in.content_length_n += size;
                    cl->buf->pos = cl->buf->last;
                }

                b->last = cl->buf->pos;

                continue;
            }

            if (rc == NGX_DONE) {

                /* a whole response has been parsed successfully */

                rb->rest = 0;

                tl = ngx_chain_get_free_buf(r->pool, &rb->free);
                if (tl == NULL) {
                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                }

                b = tl->buf;

                ngx_memzero(b, sizeof(ngx_buf_t));

                b->last_buf = 1;

                *ll = tl;
                ll = &tl->next;

                break;
            }

            if (rc == NGX_AGAIN) {

                /* set rb->rest, amount of data we want to see next time */

                rb->rest = rb->chunked->length;

                break;
            }

            /* invalid */

            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "client sent invalid chunked body");

            return NGX_HTTP_BAD_REQUEST;
        }
    }

    rc = ngx_http_top_request_body_filter(r, out);

    ngx_chain_update_chains(r->pool, &rb->free, &rb->busy, &out,
                            (ngx_buf_tag_t) &ngx_http_read_client_request_body);

    return rc;
}


ngx_int_t
ngx_http_request_body_save_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
#if (NGX_DEBUG)
    ngx_chain_t               *cl;
#endif
    ngx_http_request_body_t   *rb;

    rb = r->request_body;

#if (NGX_DEBUG)

    for (cl = rb->bufs; cl; cl = cl->next) {
        ngx_log_debug7(NGX_LOG_DEBUG_EVENT, r->connection->log, 0,
                       "http body old buf t:%d f:%d %p, pos %p, size: %z "
                       "file: %O, size: %O",
                       cl->buf->temporary, cl->buf->in_file,
                       cl->buf->start, cl->buf->pos,
                       cl->buf->last - cl->buf->pos,
                       cl->buf->file_pos,
                       cl->buf->file_last - cl->buf->file_pos);
    }

    for (cl = in; cl; cl = cl->next) {
        ngx_log_debug7(NGX_LOG_DEBUG_EVENT, r->connection->log, 0,
                       "http body new buf t:%d f:%d %p, pos %p, size: %z "
                       "file: %O, size: %O",
                       cl->buf->temporary, cl->buf->in_file,
                       cl->buf->start, cl->buf->pos,
                       cl->buf->last - cl->buf->pos,
                       cl->buf->file_pos,
                       cl->buf->file_last - cl->buf->file_pos);
    }

#endif

    /* TODO: coalesce neighbouring buffers */

    if (ngx_chain_add_copy(r->pool, &rb->bufs, in) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (rb->rest > 0
        && rb->buf && rb->buf->last == rb->buf->end
        && !r->request_body_no_buffering)
    {
        if (ngx_http_write_request_body(r) != NGX_OK) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    return NGX_OK;
}
