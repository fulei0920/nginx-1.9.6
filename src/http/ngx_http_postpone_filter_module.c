
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


static ngx_int_t ngx_http_postpone_filter_add(ngx_http_request_t *r, ngx_chain_t *in);
static ngx_int_t ngx_http_postpone_filter_init(ngx_conf_t *cf);


static ngx_http_module_t  ngx_http_postpone_filter_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_postpone_filter_init,         /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};

//如果当前请求对象不是排序在最前面，那么就需要做缓存
//创建一个ngx_http_postponed_request_t对象并把数据缓存在其内，
//然后挂接到当前请求的postponed链表内
ngx_module_t  ngx_http_postpone_filter_module = 
{
    NGX_MODULE_V1,
    &ngx_http_postpone_filter_module_ctx,  /* module context */
    NULL,                                  /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;


static ngx_int_t
ngx_http_postpone_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_connection_t              *c;
    ngx_http_postponed_request_t  *pr;

    c = r->connection;

    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, c->log, 0, "http postpone filter \"%V?%V\" %p", &r->uri, &r->args, in);

	//当前请求对象并不是最前面的，因此不能向out chain链写入(如果有)数据，
	//取而代之的是通过函数ngx_http_postpone_filter_add将其加入到当前请求的postponed链内。
    if (r != c->data)
	{
        if (in)
		{
            ngx_http_postpone_filter_add(r, in);
            return NGX_OK;
        }

#if 0
        /* TODO: SSI may pass NULL */
        ngx_log_error(NGX_LOG_ALERT, c->log, 0, "http postpone filter NULL inactive request");
#endif

        return NGX_OK;
    }

	//如果r->postponed为空，则说明是最后一个sub request，也就是最新的那个，因此需要将它先发送出去。  
    if (r->postponed == NULL) 
	{

        if (in || c->buffered) 
		{
            return ngx_http_next_body_filter(r->main, in);
        }

        return NGX_OK;
    }

	//当前请求是否有新数据产生，如果有的话需加到最后
    if (in) 
	{
        ngx_http_postpone_filter_add(r, in);
    }

    do 
	{
        pr = r->postponed;

        if (pr->request)
		{

            ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http postpone filter wake \"%V?%V\"", &pr->request->uri, &pr->request->args);

            r->postponed = pr->next;

            c->data = pr->request;

            return ngx_http_post_request(pr->request, NULL);
        }

        if (pr->out == NULL)
		{
            ngx_log_error(NGX_LOG_ALERT, c->log, 0, "http postpone filter NULL output");
        } 
		else 
		{
            ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http postpone filter output \"%V?%V\"", &r->uri, &r->args);

            if (ngx_http_next_body_filter(r->main, pr->out) == NGX_ERROR) 
			{
                return NGX_ERROR;
            }
        }

        r->postponed = pr->next;

    } 
	while (r->postponed);

    return NGX_OK;
}


static ngx_int_t
ngx_http_postpone_filter_add(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_http_postponed_request_t  *pr, **ppr;

	//判断postponed链最末尾(必定是在最末尾)的ngx_http_postponed_request_t结构存放的是否就是数据(即pr->request == NULL),
	//如果是则把数据直接添加进去(具体也就是ngx_http_postponed_request_t结构体对象的out链)即可，
	//否则的话，就需在postponed链表末尾新建一个对应的ngx_http_postponed_request_t结构来存放该数据。
    if (r->postponed)
	{
        for (pr = r->postponed; pr->next; pr = pr->next) { /* void */ }

        if (pr->request == NULL)
		{
            goto found;
        }

        ppr = &pr->next;

    }
	else 
	{
        ppr = &r->postponed;
    }

    pr = ngx_palloc(r->pool, sizeof(ngx_http_postponed_request_t));
    if (pr == NULL)
	{
        return NGX_ERROR;
    }

    *ppr = pr;

    pr->request = NULL;
    pr->out = NULL;
    pr->next = NULL;

found:
	//最终复制in到pr->out,也就是保存request 需要发送的数据。  
    if (ngx_chain_add_copy(r->pool, &pr->out, in) == NGX_OK)
	{
        return NGX_OK;
    }

    return NGX_ERROR;
}


static ngx_int_t
ngx_http_postpone_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_postpone_filter;

    return NGX_OK;
}
