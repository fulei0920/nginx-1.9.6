
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

//ngx_http_read_client_request_body��һ���첽������������ֻ��˵��Ҫ��Nginx��ʼ��������İ��壬������ʾ�Ƿ��Ѿ������꣬
//�����������еİ������ݺ�post_handlerָ��Ļص������ᱻ���á���ˣ���ʹ�ڵ�����ngx_http_read_client_request_body����
//�����Ѿ����أ�Ҳ�޷�ȷ����ʱ�Ƿ��Ѿ����ù�post_handlerָ��ķ�����
//ע��: post_handler�ķ���������void��Nginx������ݷ���ֵ��һЩ��β��������ˣ������ڸ÷����ﴦ��������ʱ����Ҫ��������
//ngx_http_finalize_request��������������
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

	//���������Ӧ������������ü�����1����ͬʱ����Ҫ��ÿһ��HTTPģ���ڴ����post_handler�������ص�ʱ��
	//��ص�������ngx_http_finlize_request�ķ���ȥ�������󣬷������ü�����ʼ���޷����㣬�Ӷ����������޷��ͷš�
    r->main->count++;

#if (NGX_HTTP_V2)
    if (r->stream && r == r->main) 
	{
        r->request_body_no_buffering = 0;
        rc = ngx_http_v2_read_request_body(r, post_handler);
        goto done;
    }
#endif
	//��鵱ǰ�������������������󣬶�����������ԣ����������Կͻ��˵��������Բ����ڴ���HTTP�������ĸ���
	//�������ngx_http_request_t�ṹ���е�request_body��Ա��������Ѿ���������ˣ�֤���Ѿ���ȡ��HTTP�����ˣ�����Ҫ�ٴζ�ȡһ��;
	//�������ngx_http_request_t�ṹ���е�discard_body��Ա�����discard_bodyΪ1����֤������ִ�й���������ķ��������ڰ������ڱ�������
    if (r != r->main || r->request_body || r->discard_body)
	{
        r->request_body_no_buffering = 0;
        post_handler(r);
        return NGX_OK;
    }

	//����������Ҫ����HTTP����....
	
    if (ngx_http_test_expect(r) != NGX_OK)
	{
        rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
        goto done;
    }

    if (r->request_body_no_buffering) 
	{
        r->request_body_in_file_only = 0;
    }

	//��������ngx_http_request_t�Ľṹ���е�request_body��Ա(֮ǰ��NULL��ָ��)��׼�����հ���
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

	//��������е�content-lengthͷ����chunkedͷ��
    if (r->headers_in.content_length_n < 0 && !r->headers_in.chunked) 
	{
        r->request_body_no_buffering = 0;
        post_handler(r);
        return NGX_OK;
    }

	//����HTTPͷ���������У����п��ܽ��յ�HTTP�����
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

		//��������峤���Ƿ���contend-length��ʽ��¼
		//�����header_in���������Ѿ����յ��İ��峤���Ƿ�С��content-lengthͷ��ָ���ĳ���
		//���header_in���������ʣ��ռ��Ƿ���Դ����ȫ���İ��壬
		//������ԣ��Ͳ��÷����µİ��建�����˷��ڴ��ˣ�ֱ�ӽ�header_inʣ��ռ���Ϊ���հ���Ļ�����
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

	//�����header_in���������Ѿ����յ��İ��������Ƿ���������İ�������
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

	//�������ڽ��հ���Ļ�����
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

	//����ngx_http_do_read_client_request_body�������հ��塣�÷������������ڰѿͻ�����Nginx֮��TCP������
	//�׽��ֻ������еĵ�ǰ�ַ���ȫ�������������ж��Ƿ���Ҫд���ļ����Լ��Ƿ���յ�ȫ�����壬ͬʱ�ڽ��յ�
	//�����İ���󼤻�post_handler�ص�����
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

	//����Ƿ����HTTP���峬ʱ
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
			//��������request_body��Ա�е�buf�������Ƿ��п��еĿռ�
            if (rb->buf->last == rb->buf->end) 
			{
				//request_body��Ա�е�buf������û�п��еĿռ�
				//���request_body ��Ա�е�buf���������Ƿ�������
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


			//�����request_body��Ա�е�buf�������п��еĿռ䣬ֱ�Ӷ�ȡ�ں����׽��ֻ��������TCP�ַ���
			//buffer��ʣ��ռ��С
            size = rb->buf->end - rb->buf->last;
			//request body ʣ���С
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

			//recv�������ش�����߿ͻ��������ر�������
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

			//����rest��Ա����Ƿ���յ������İ���
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

		//�����ǰû�пɶ����ַ�����ͬʱ��û�н��յ������İ��壬��˵����Ҫ�Ѷ��¼���ӵ��¼�ģ�飬
		//�ȴ��ɶ��¼�����ʱ���¼���ܿ����ٴε��ȵ�����������հ���
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

			//�����¼���ӵ���ʱ����
            clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
            ngx_add_timer(c->read, clcf->client_body_timeout);

			//�����¼���ӵ�epoll�¼�������
            if (ngx_handle_read_event(c->read, 0) != NGX_OK) 
			{
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }

            return NGX_AGAIN;
        }
    }

	//���˴��������Ѿ����յ������İ��壬��Ҫ��һЩ��β�Ĺ����ˡ�

	//����Ҫ����Ƿ����HTTP���峬ʱ�ˣ�Ҫ�Ѷ��¼��Ӷ�ʱ����ȡ������ֹ����Ҫ�Ķ�ʱ������
    if (c->read->timer_set) 
	{
        ngx_del_timer(c->read);
    }

	//�������������δд���ļ������ݣ�����ngx_http_write_request_body���������İ���д���ļ�
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
		//�Ѿ����յ������İ����ˣ��ͻ��read_event_handler��Ϊngx_http_block_reading��������ʾ���������ж��¼��������κδ���
        r->read_event_handler = ngx_http_block_reading;
		//ִ��HTTPģ���ṩ��post_handler�ص�����
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

//����HTTPģ����ԣ��������հ�����Ǽ򵥵ز���������ˣ����Ƕ���HTTP��ܶ��ԣ������ǲ����հ���Ϳ��Եġ�
//��Ϊ���ڿͻ��˶��ԣ�ͨ�������һЩ�����ķ��ͷ��������Ͱ��壬���HTTP���һֱ�����հ��壬�ᵼ��ʵ����
//������׳�Ŀͻ�����Ϊ��������ʱ����Ӧ������򵥵Ĺر����ӣ�����ʱNginx���ܻ��ڴ���������ӡ���ˣ�HTTP
//ģ���еķ������հ��壬��HTTP��ܶ��Ծ��ǽ��հ��壬���ǽ��պ������棬ֱ�Ӷ�����

//��Ϊ��Щ�ͻ��˿��ܻ�һֱ��ͼ���Ͱ��壬�����HTTPģ�鲻���շ�����TCP�����п�����ɿͻ��˷��ͳ�ʱ��
//����������봦�������еİ��壬��ô���Ե���ngx_http_discard_request_body�����������Կͻ��˵�HTTP���嶪����
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
	//��鵱ǰ�������������������󣬶�����������ԣ����������Կͻ��˵��������Բ����ڴ���HTTP�������ĸ���
	//�������ngx_http_request_t�ṹ���е�request_body��Ա��������Ѿ���������ˣ�֤���Ѿ���ȡ��HTTP�����ˣ�����Ҫ�ٴζ�ȡһ��;
	//�������ngx_http_request_t�ṹ���е�discard_body��Ա�����discard_bodyΪ1����֤������ִ�й���������ķ��������ڰ������ڱ�������
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

	//������������ϵĶ��¼��Ƿ��ڶ�ʱ���У�������Ϊ�������岻�ÿ��ǳ�ʱ����(linger_timer����)
    if (rev->timer_set) 
	{
        ngx_del_timer(rev);
    }

    if (r->headers_in.content_length_n <= 0 && !r->headers_in.chunked)
	{
        return NGX_OK;
    }

	
    size = r->header_in->last - r->header_in->pos;
	//������HTTPͷ��ʱ���Ƿ��Ѿ����յ�������HTTP����
    if (size || r->headers_in.chunked)
	{
        rc = ngx_http_discard_request_body_filter(r, r->header_in);

        if (rc != NGX_OK) 
		{
            return rc;
        }

        if (r->headers_in.content_length_n == 0)
		{
			//����HTTPͷ��ʱ���Ѿ����յ�������HTTP����
            return NGX_OK;
        }
    }

    rc = ngx_http_read_discarded_request_body(r);

	//rc == NGX_OK��ʾ�Ѿ����յ������İ���
    if (rc == NGX_OK)
	{
		//�������lingering_close��ʱ�رձ�־λ��Ϊ0����ʾ����ҪΪ�˰���Ľ��ն���ʱ�ر���
        r->lingering_close = 0;
        return NGX_OK;
    }

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) 
	{
        return rc;
    }

    //rc == NGX_AGAIN����ʾ��ҪNginx�¼���ܶ�ε��Ȳ�����ɶ���������һ����
	
    r->read_event_handler = ngx_http_discarded_request_body_handler;

    if (ngx_handle_read_event(rev, 0) != NGX_OK) 
	{
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

	//�����ü�����1����ֹ��߻��ڶ������壬�������¼�ȴ���������������٣��������ش���
    r->count++;
	//��ngx_http_request_t�ṹ���discard_body��־λ��1����ʾ���ڶ�������
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

	//rc == NGX_OK����ʾ�Ѿ��ɹ����������а���
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



//����ֵ:
//	NGX_OK -- �Ѿ����������а���
//	NGX_AGAIN -- ��Ҫ�ȴ����¼��Ĵ������ȴ�Nginx��ܵ��ٴε���
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

	//��������ʱ�����request_body��Աʵ������NULL��ָ�룬��ʱʹ��������ngx_http_request_t�Ľṹheaders_in��Ա�����content_length_n
	//����ʾ�Ѿ������İ����ж�����������content_lengthͷ������ÿ����һ���ְ��壬�ͻ���content_length_n�����м�ȥ
	//��Ӧ�Ĵ�С�����content_length_n��ʾ����Ҫ�����İ��峤��
    for ( ;; ) 
	{
		//content_length_nΪ0����ʾ�Ѿ����յ������İ���
        if (r->headers_in.content_length_n == 0) 
		{
			//��read_event_handler����Ϊngx_http_block_reading����ʾ������пɶ��¼������������κδ���
            r->read_event_handler = ngx_http_block_reading;
			//
            return NGX_OK;
        }

		//�׽��ֻ�������û�����ݿɶ�����ֱ�ӷ���NGX_ANGIN�������ϲ㷽����Ҫ�ȴ����¼��ĳ������ȴ�Nginx��ܵ��ٴε���
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
		
		//�׽��ֻ�������û�����ݿɶ�����ֱ�ӷ���NGX_ANGIN�������ϲ㷽����Ҫ�ȴ����¼��ĳ������ȴ�Nginx��ܵ��ٴε���
        if (n == NGX_AGAIN) 
		{
            return NGX_AGAIN;
        }

		//�ͻ��������ر������ӣ�ֱ�ӷ���NGX_OK�����ϲ㷽�������������嶯������
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

//���ͻ����Ƿ�����Expect: 100-continueͷ���ǵĻ�����ͻ��˻ظ���HTTP/1.1 100 Continue��������http 1.1Э�飬
//�ͻ��˿��Է���һ��Expectͷ��������������������������壬�������������ͻ��˷��������壬���ظ���HTTP/1.1 100 Continue����
//�ͻ����յ�ʱ���ŻῪʼ����������
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
