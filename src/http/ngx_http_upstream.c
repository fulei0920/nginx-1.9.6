
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#if (NGX_HTTP_CACHE)
static ngx_int_t ngx_http_upstream_cache(ngx_http_request_t *r, ngx_http_upstream_t *u);
static ngx_int_t ngx_http_upstream_cache_get(ngx_http_request_t *r, ngx_http_upstream_t *u, ngx_http_file_cache_t **cache);
static ngx_int_t ngx_http_upstream_cache_send(ngx_http_request_t *r, ngx_http_upstream_t *u);
static ngx_int_t ngx_http_upstream_cache_status(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_upstream_cache_last_modified(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_upstream_cache_etag(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);
#endif

static void ngx_http_upstream_init_request(ngx_http_request_t *r);
static void ngx_http_upstream_resolve_handler(ngx_resolver_ctx_t *ctx);
static void ngx_http_upstream_rd_check_broken_connection(ngx_http_request_t *r);
static void ngx_http_upstream_wr_check_broken_connection(ngx_http_request_t *r);
static void ngx_http_upstream_check_broken_connection(ngx_http_request_t *r, ngx_event_t *ev);
static void ngx_http_upstream_connect(ngx_http_request_t *r, ngx_http_upstream_t *u);
static ngx_int_t ngx_http_upstream_reinit(ngx_http_request_t *r, ngx_http_upstream_t *u);
static void ngx_http_upstream_send_request(ngx_http_request_t *r, ngx_http_upstream_t *u, ngx_uint_t do_write);
static ngx_int_t ngx_http_upstream_send_request_body(ngx_http_request_t *r,
    ngx_http_upstream_t *u, ngx_uint_t do_write);
static void ngx_http_upstream_send_request_handler(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_read_request_handler(ngx_http_request_t *r);
static void ngx_http_upstream_process_header(ngx_http_request_t *r, ngx_http_upstream_t *u);
static ngx_int_t ngx_http_upstream_test_next(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static ngx_int_t ngx_http_upstream_intercept_errors(ngx_http_request_t *r, ngx_http_upstream_t *u);
static ngx_int_t ngx_http_upstream_test_connect(ngx_connection_t *c);
static ngx_int_t ngx_http_upstream_process_headers(ngx_http_request_t *r, ngx_http_upstream_t *u);
static void ngx_http_upstream_process_body_in_memory(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_send_response(ngx_http_request_t *r, ngx_http_upstream_t *u);
static void ngx_http_upstream_upgrade(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_upgraded_read_downstream(ngx_http_request_t *r);
static void ngx_http_upstream_upgraded_write_downstream(ngx_http_request_t *r);
static void ngx_http_upstream_upgraded_read_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_upgraded_write_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_process_upgraded(ngx_http_request_t *r,
    ngx_uint_t from_upstream, ngx_uint_t do_write);
static void
    ngx_http_upstream_process_non_buffered_downstream(ngx_http_request_t *r);
static void
    ngx_http_upstream_process_non_buffered_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void
    ngx_http_upstream_process_non_buffered_request(ngx_http_request_t *r,
    ngx_uint_t do_write);
static ngx_int_t ngx_http_upstream_non_buffered_filter_init(void *data);
static ngx_int_t ngx_http_upstream_non_buffered_filter(void *data,
    ssize_t bytes);
static void ngx_http_upstream_process_downstream(ngx_http_request_t *r);
static void ngx_http_upstream_process_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_process_request(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_store(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_dummy_handler(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_next(ngx_http_request_t *r,
    ngx_http_upstream_t *u, ngx_uint_t ft_type);
static void ngx_http_upstream_cleanup(void *data);
static void ngx_http_upstream_finalize_request(ngx_http_request_t *r, ngx_http_upstream_t *u, ngx_int_t rc);

static ngx_int_t ngx_http_upstream_process_header_line(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_content_length(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_last_modified(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_set_cookie(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t
    ngx_http_upstream_process_cache_control(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_ignore_header_line(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_expires(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_accel_expires(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_limit_rate(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_buffering(ngx_http_request_t *r, ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_charset(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_connection(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t
    ngx_http_upstream_process_transfer_encoding(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_vary(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_copy_header_line(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t
    ngx_http_upstream_copy_multi_header_lines(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_copy_content_type(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_copy_last_modified(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_rewrite_location(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_rewrite_refresh(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_rewrite_set_cookie(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_copy_allow_ranges(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);

#if (NGX_HTTP_GZIP)
static ngx_int_t ngx_http_upstream_copy_content_encoding(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
#endif

static ngx_int_t ngx_http_upstream_add_variables(ngx_conf_t *cf);
static ngx_int_t ngx_http_upstream_addr_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_upstream_status_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_upstream_response_time_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_upstream_response_length_variable(
    ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);

static char *ngx_http_upstream(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy);
static char *ngx_http_upstream_server(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_addr_t *ngx_http_upstream_get_local(ngx_http_request_t *r, ngx_http_upstream_local_t *local);

static void *ngx_http_upstream_create_main_conf(ngx_conf_t *cf);
static char *ngx_http_upstream_init_main_conf(ngx_conf_t *cf, void *conf);

#if (NGX_HTTP_SSL)
static void ngx_http_upstream_ssl_init_connection(ngx_http_request_t *, ngx_http_upstream_t *u, ngx_connection_t *c);
static void ngx_http_upstream_ssl_handshake(ngx_connection_t *c);
static ngx_int_t ngx_http_upstream_ssl_name(ngx_http_request_t *r, ngx_http_upstream_t *u, ngx_connection_t *c);
#endif


ngx_http_upstream_header_t  ngx_http_upstream_headers_in[] = 
{

    { ngx_string("Status"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, status),
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("Content-Type"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, content_type),
                 ngx_http_upstream_copy_content_type, 0, 1 },

    { ngx_string("Content-Length"),
                 ngx_http_upstream_process_content_length, 0,
                 ngx_http_upstream_ignore_header_line, 0, 0 },

    { ngx_string("Date"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, date),
                 ngx_http_upstream_copy_header_line,
                 offsetof(ngx_http_headers_out_t, date), 0 },

    { ngx_string("Last-Modified"),
                 ngx_http_upstream_process_last_modified, 0,
                 ngx_http_upstream_copy_last_modified, 0, 0 },

    { ngx_string("ETag"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, etag),
                 ngx_http_upstream_copy_header_line,
                 offsetof(ngx_http_headers_out_t, etag), 0 },

    { ngx_string("Server"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, server),
                 ngx_http_upstream_copy_header_line,
                 offsetof(ngx_http_headers_out_t, server), 0 },

    { ngx_string("WWW-Authenticate"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, www_authenticate),
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("Location"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, location),
                 ngx_http_upstream_rewrite_location, 0, 0 },

    { ngx_string("Refresh"),
                 ngx_http_upstream_ignore_header_line, 0,
                 ngx_http_upstream_rewrite_refresh, 0, 0 },

    { ngx_string("Set-Cookie"),
                 ngx_http_upstream_process_set_cookie,
                 offsetof(ngx_http_upstream_headers_in_t, cookies),
                 ngx_http_upstream_rewrite_set_cookie, 0, 1 },

    { ngx_string("Content-Disposition"),
                 ngx_http_upstream_ignore_header_line, 0,
                 ngx_http_upstream_copy_header_line, 0, 1 },

    { ngx_string("Cache-Control"),
                 ngx_http_upstream_process_cache_control, 0,
                 ngx_http_upstream_copy_multi_header_lines,
                 offsetof(ngx_http_headers_out_t, cache_control), 1 },

    { ngx_string("Expires"),
                 ngx_http_upstream_process_expires, 0,
                 ngx_http_upstream_copy_header_line,
                 offsetof(ngx_http_headers_out_t, expires), 1 },

    { ngx_string("Accept-Ranges"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, accept_ranges),
                 ngx_http_upstream_copy_allow_ranges,
                 offsetof(ngx_http_headers_out_t, accept_ranges), 1 },

    { ngx_string("Connection"),
                 ngx_http_upstream_process_connection, 0,
                 ngx_http_upstream_ignore_header_line, 0, 0 },

    { ngx_string("Keep-Alive"),
                 ngx_http_upstream_ignore_header_line, 0,
                 ngx_http_upstream_ignore_header_line, 0, 0 },

    { ngx_string("Vary"),
                 ngx_http_upstream_process_vary, 0,
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("X-Powered-By"),
                 ngx_http_upstream_ignore_header_line, 0,
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("X-Accel-Expires"),
                 ngx_http_upstream_process_accel_expires, 0,
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("X-Accel-Redirect"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, x_accel_redirect),
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("X-Accel-Limit-Rate"),
                 ngx_http_upstream_process_limit_rate, 0,
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("X-Accel-Buffering"),
                 ngx_http_upstream_process_buffering, 0,
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("X-Accel-Charset"),
                 ngx_http_upstream_process_charset, 0,
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("Transfer-Encoding"),
                 ngx_http_upstream_process_transfer_encoding, 0,
                 ngx_http_upstream_ignore_header_line, 0, 0 },

#if (NGX_HTTP_GZIP)
    { ngx_string("Content-Encoding"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, content_encoding),
                 ngx_http_upstream_copy_content_encoding, 0, 0 },
#endif

    { ngx_null_string, NULL, 0, NULL, 0, 0 }
};


static ngx_command_t  ngx_http_upstream_commands[] = 
{
	//语法: upstream name {...}
	//upstream块定义了一个上游服务器的集群，便于方向代理中的proxy_pass使用
    { 
		ngx_string("upstream"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_TAKE1,
		ngx_http_upstream,
		0,
		0,
		NULL 
    },
	//语法: server name [weight=number | max_fails=number | fail_timeout=time | down | backpup]
	//server配置项制订了一台上游服务器的名字，这个名字可以是域名、IP地址端口、UNIX句柄等，其后还可跟下列参数
	//weight=number: 设置这台上游服务器转发的权重，默认为1
	//max_fails=number: 默认为1，如果设置为0， 则表示不检查失败次数 
	//fail_timeout=time: 默认为10秒
	//	表示在fail_timeout时间段内转发失败max_fails次后就认为上游服务器暂时不可用，用于优化反向代理功能。
	//	fail_timeout与向上游服务器建立连接的超时时间、读取上游服务器的响应超时时间等完全无关。
	//down: 表示所在的上游服务器永久下线，只在使用ip_hash配置项时有效
	//backup: 表示该上游服务器是备份服务器，只有在所有的非备份服务器都失效后，才会向该上游服务器转发请求，
	//	在使用ip_hash配置项时无效
	//例如: upstream backend {
	//			server backend1.example.com weight=5
	//			server 127.0.0.1:8080		max_fails=3 fail_timeout=30s
	//			server unix:/tmp/backend3
	
    { 
		ngx_string("server"),
		NGX_HTTP_UPS_CONF|NGX_CONF_1MORE,
		ngx_http_upstream_server,
		NGX_HTTP_SRV_CONF_OFFSET,
		0,
		NULL 
    },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_upstream_module_ctx = 
{
    ngx_http_upstream_add_variables,       /* preconfiguration */
    NULL,                                  /* postconfiguration */

    ngx_http_upstream_create_main_conf,    /* create main configuration */
    ngx_http_upstream_init_main_conf,      /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};

//upstream 模块不产生自己的内容，而是通过请求后端服务器得到内容。Nginx 内部封装了请求并取得响应内容的整个过程，
//所以upstream 模块只需要开发若干回调函数，完成构造请求和解析响应等具体的工作。

//当客户端发来HTTP请求时，Nginx并不会立刻转发到上游服务器，而是先把用户的请求(包括HTTP包体)完整地接收到Nginx
//所在服务器的硬盘或内存中，然后再向上游服务器发起连接，把缓存的客户端请求转发到上游服务器
//Nginx反向代理服务器可以根据多种方案从上游服务器的集群中选择一台。它的负载均衡方案包括按IP地址做散列等
//如果上游服务器返回内容，则不会先完整的缓存到Nginx代理服务器再向客户端转发，而是边接受边转发到客户端
ngx_module_t  ngx_http_upstream_module =
{
    NGX_MODULE_V1,
    &ngx_http_upstream_module_ctx,         /* module context */
    ngx_http_upstream_commands,            /* module directives */
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


static ngx_http_variable_t  ngx_http_upstream_vars[] = 
{
	//处理请求的上游服务器地址
    { ngx_string("upstream_addr"), NULL, ngx_http_upstream_addr_variable, 0, NGX_HTTP_VAR_NOCACHEABLE, 0 },
	//上游服务器返回的响应中的HTTP响应码
    { ngx_string("upstream_status"), NULL,
      ngx_http_upstream_status_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_connect_time"), NULL,
      ngx_http_upstream_response_time_variable, 2,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_header_time"), NULL,
      ngx_http_upstream_response_time_variable, 1,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },
	//上游服务器的响应时间，精确到毫秒
    { ngx_string("upstream_response_time"), NULL,
      ngx_http_upstream_response_time_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_response_length"), NULL,
      ngx_http_upstream_response_length_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

#if (NGX_HTTP_CACHE)

    { ngx_string("upstream_cache_status"), NULL,
      ngx_http_upstream_cache_status, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_cache_last_modified"), NULL,
      ngx_http_upstream_cache_last_modified, 0,
      NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },

    { ngx_string("upstream_cache_etag"), NULL,
      ngx_http_upstream_cache_etag, 0,
      NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },

#endif

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};

//响应码对应的标志位
static ngx_http_upstream_next_t  ngx_http_upstream_next_errors[] = {
    { 500, NGX_HTTP_UPSTREAM_FT_HTTP_500 },
    { 502, NGX_HTTP_UPSTREAM_FT_HTTP_502 },
    { 503, NGX_HTTP_UPSTREAM_FT_HTTP_503 },
    { 504, NGX_HTTP_UPSTREAM_FT_HTTP_504 },
    { 403, NGX_HTTP_UPSTREAM_FT_HTTP_403 },
    { 404, NGX_HTTP_UPSTREAM_FT_HTTP_404 },
    { 0, 0 }
};


ngx_conf_bitmask_t  ngx_http_upstream_cache_method_mask[] = {
   { ngx_string("GET"),  NGX_HTTP_GET},
   { ngx_string("HEAD"), NGX_HTTP_HEAD },
   { ngx_string("POST"), NGX_HTTP_POST },
   { ngx_null_string, 0 }
};


ngx_conf_bitmask_t  ngx_http_upstream_ignore_headers_masks[] = {
    { ngx_string("X-Accel-Redirect"), NGX_HTTP_UPSTREAM_IGN_XA_REDIRECT },
    { ngx_string("X-Accel-Expires"), NGX_HTTP_UPSTREAM_IGN_XA_EXPIRES },
    { ngx_string("X-Accel-Limit-Rate"), NGX_HTTP_UPSTREAM_IGN_XA_LIMIT_RATE },
    { ngx_string("X-Accel-Buffering"), NGX_HTTP_UPSTREAM_IGN_XA_BUFFERING },
    { ngx_string("X-Accel-Charset"), NGX_HTTP_UPSTREAM_IGN_XA_CHARSET },
    { ngx_string("Expires"), NGX_HTTP_UPSTREAM_IGN_EXPIRES },
    { ngx_string("Cache-Control"), NGX_HTTP_UPSTREAM_IGN_CACHE_CONTROL },
    { ngx_string("Set-Cookie"), NGX_HTTP_UPSTREAM_IGN_SET_COOKIE },
    { ngx_string("Vary"), NGX_HTTP_UPSTREAM_IGN_VARY },
    { ngx_null_string, 0 }
};

//从内存池中创建ngx_http_upstream_t结构体，其中的成员还需要各个HTTP模块自行设置
ngx_int_t
ngx_http_upstream_create(ngx_http_request_t *r)
{
    ngx_http_upstream_t  *u;

    u = r->upstream;

	/*
     * 若已经创建过ngx_http_upstream_t 且定义了cleanup成员，
     * 则调用cleanup清理方法将原始结构体清除；
     */
    if (u && u->cleanup) {
        r->main->count++;
        ngx_http_upstream_cleanup(r);
    }

    u = ngx_pcalloc(r->pool, sizeof(ngx_http_upstream_t));
    if (u == NULL) {
        return NGX_ERROR;
    }

    r->upstream = u;

    u->peer.log = r->connection->log;
    u->peer.log_error = NGX_ERROR_ERR;

#if (NGX_HTTP_CACHE)
    r->cache = NULL;
#endif

    u->headers_in.content_length_n = -1;
    u->headers_in.last_modified_time = -1;

    return NGX_OK;
}

/*
启动upstream。
在 Nginx 中调用 ngx_http_upstream_init 方法启动 upstream 机制，但是在使用 upstream 机制之前必须调用
ngx_http_upstream_create 方法创建 ngx_http_upstream_t 结构体，因为默认情况下 ngx_http_request_t 
结构体中的 upstream 成员是指向 NULL，该结构体的具体初始化工作还需由 HTTP 模块完成。
*/
void
ngx_http_upstream_init(ngx_http_request_t *r)
{
    ngx_connection_t     *c;

    c = r->connection;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "http init upstream, client timer: %d", c->read->timer_set);

#if (NGX_HTTP_V2)
    if (r->stream) 
	{
        ngx_http_upstream_init_request(r);
        return;
    }
#endif
	//如果请求对应的客户端的连接上的读事件在定时器中，那么把这个读事件从定时器中移除
	//因为一旦启动upstream机制，就不应该对客户端的读操作带有超时时间的处理，请求的主要
	//触发事件将以与上游服务器的连接为主
    if (c->read->timer_set) {
        ngx_del_timer(c->read);
    }

    if (ngx_event_flags & NGX_USE_CLEAR_EVENT)
	{
        if (!c->write->active) 
		{
            if (ngx_add_event(c->write, NGX_WRITE_EVENT, NGX_CLEAR_EVENT) == NGX_ERROR)
            {
                ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;
            }
        }
    }

    ngx_http_upstream_init_request(r);
}


static void
ngx_http_upstream_init_request(ngx_http_request_t *r)
{
    ngx_str_t                      *host;
    ngx_uint_t                      i;
    ngx_resolver_ctx_t             *ctx, temp;
    ngx_http_cleanup_t             *cln;
    ngx_http_upstream_t            *u;
    ngx_http_core_loc_conf_t       *clcf;
    ngx_http_upstream_srv_conf_t   *uscf, **uscfp;
    ngx_http_upstream_main_conf_t  *umcf;

    if (r->aio) {
        return;
    }

    u = r->upstream;

#if (NGX_HTTP_CACHE)
	///nignx向上游服务器发出upstream请求时，会先检查是否有可用缓存。
	
    if (u->conf->cache) {   //检查该location是否配置了proxy_cache/fastcgi_cache，如果是则使用缓存
        ngx_int_t  rc;

		///寻找缓存  
        ///如果返回NGX_DECLINED就是说缓存中没有，请向后端发送请求 
        rc = ngx_http_upstream_cache(r, u);

        if (rc == NGX_BUSY) {
            r->write_event_handler = ngx_http_upstream_init_request;
            return;
        }

        r->write_event_handler = ngx_http_request_empty_handler;

        if (rc == NGX_ERROR) {
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        if (rc == NGX_OK) {
            rc = ngx_http_upstream_cache_send(r, u);

            if (rc == NGX_DONE) {
                return;
            }

            if (rc == NGX_HTTP_UPSTREAM_INVALID_HEADER) {
                rc = NGX_DECLINED;
                r->cached = 0;
            }
        }

        if (rc != NGX_DECLINED) {
            ngx_http_finalize_request(r, rc);
            return;
        }
    }

#endif
	/* 文件缓存标志位 */ 
    u->store = u->conf->store;

	//检查ngx_http_upstream_conf_t配置结构体中的ignore_client_abort标志位，
	//如果ignore_client_abort为0(实际上还需要让store标志位为0， 请求结构体中post_action标志位为0)，
	//就会设置Nginx与下游客户端之间TCP连接的检查方法
	 /*
     * 检查ngx_http_upstream_t 结构中标志位 store；
     * 检查ngx_http_request_t 结构中标志位 post_action；
     * 检查ngx_http_upstream_conf_t 结构中标志位 ignore_client_abort；
     * 若上面这些标志位为1，则表示需要检查Nginx与下游(即客户端)之间的TCP连接是否断开；
     */
    if (!u->store && !r->post_action && !u->conf->ignore_client_abort) {
        r->read_event_handler = ngx_http_upstream_rd_check_broken_connection;
        r->write_event_handler = ngx_http_upstream_wr_check_broken_connection;
    }

	 /* 把当前请求包体结构保存在ngx_http_upstream_t 结构的request_bufs链表缓冲区中 */
    if (r->request_body) {
        u->request_bufs = r->request_body->bufs;
    }

	//调用请求中ngx_http_upstream_t结构体里由某个HTTP模块实现的create_request方法，构造发往上游服务器的请求
    if (u->create_request(r) != NGX_OK) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
	
	/* 获取ngx_http_upstream_t结构中主动连接结构peer的local本地地址信息 */  
    u->peer.local = ngx_http_upstream_get_local(r, u->conf->local);
	
	/* 获取ngx_http_core_module模块的loc级别的配置项结构 */  
    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
	
	/* 初始化ngx_http_upstream_t结构中成员output向下游发送响应的方式 */
    u->output.alignment = clcf->directio_alignment;
    u->output.pool = r->pool;
    u->output.bufs.num = 1;
    u->output.bufs.size = clcf->client_body_buffer_size;

    if (u->output.output_filter == NULL) 
	{
        u->output.output_filter = ngx_chain_writer;
        u->output.filter_ctx = &u->writer;
    }

    u->writer.pool = r->pool;

	/* 添加用于表示上游响应的状态，例如：错误编码、包体长度等 */  
    if (r->upstream_states == NULL) {
        r->upstream_states = ngx_array_create(r->pool, 1, sizeof(ngx_http_upstream_state_t));
        if (r->upstream_states == NULL) {
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

    } else {

        u->state = ngx_array_push(r->upstream_states);
        if (u->state == NULL) {
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        ngx_memzero(u->state, sizeof(ngx_http_upstream_state_t));
    }

	//调用 ngx_http_cleanup_add 方法向原始请求的 cleanup 链表尾端添加一个回调 handler 方法，
	//该回调方法设置为ngx_http_upstream_cleanup，若当前请求结束时会调用该方法做一些清理工作
    cln = ngx_http_cleanup_add(r, 0);
    if (cln == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    cln->handler = ngx_http_upstream_cleanup;
    cln->data = r;
    u->cleanup = &cln->handler;

	//不需要解析地址的情况，包括配置upstream模式下的静态server，静态proxy_pass指令，
	//即初始化过程中已经能将地址解析好的情况（静态IP后端或者可通过gethostbyname获取到地址的域名后端）
    if (u->resolved == NULL) {
		/* 若没有实现u->resolved标志位，则定义上游服务器的配置 */ 
        uscf = u->conf->upstream;
    } 
	else 
	{
		/* 若实现了u->resolved标志位，则解析主机域名，指定上游服务器的地址； */
#if (NGX_HTTP_SSL)
        u->ssl_name = u->resolved->host;
#endif
		/*
         * 若已经指定了上游服务器地址，则不需要解析，
         * 直接调用ngx_http_upstream_connection方法向上游服务器发起连接；
         * 并return从当前函数返回；
         */
        //需要解析地址，并且已经解析完毕的情况（已经有socket地址），直接连接后端
        if (u->resolved->sockaddr) 
		{
            if (ngx_http_upstream_create_round_robin_peer(r, u->resolved) != NGX_OK)
            {
                ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;
            }

            ngx_http_upstream_connect(r, u);

            return;
        }
		/*需要解析地址，但socket地址还未解析*/
        host = &u->resolved->host;

        umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);

		//先找当前访问的host是否在配置的upstream数组中
        uscfp = umcf->upstreams.elts;

        for (i = 0; i < umcf->upstreams.nelts; i++) 
		{
            uscf = uscfp[i];

            if (uscf->host.len == host->len  
				&& ((uscf->port == 0 && u->resolved->no_port)|| uscf->port == u->resolved->port)
                && ngx_strncasecmp(uscf->host.data, host->data, host->len) == 0)
            {
                goto found;
            }
        }


		//当访问的地址需要进行域名解析的情况下（比如proxy_pass中存在变量，变量的值为一个域名，任何静态域名均会在初始化时候被解析），
		//是无法使用keepalive连接的，请看nginx在解析域名成功后是怎么处理的（即ngx_http_upstream_init_request函数中设置ctx->handler = ngx_http_upstream_resolve_handler）

        if (u->resolved->port == 0) 
		{
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "no port in upstream \"%V\"", host);
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        temp.name = *host;



		// 开始进行域名解析
        ctx = ngx_resolve_start(clcf->resolver, &temp);
        if (ctx == NULL)
		{
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        if (ctx == NGX_NO_RESOLVER)
		{
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "no resolver defined to resolve %V", host);
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_BAD_GATEWAY);
            return;
        }

        ctx->name = *host;
		// 设置DNS解析钩子，解析DNS成功后会执行
        ctx->handler = ngx_http_upstream_resolve_handler;
        ctx->data = r;
        ctx->timeout = clcf->resolver_timeout;

        u->resolved->ctx = ctx;

		 // 开始处理DNS解析域名
        if (ngx_resolve_name(ctx) != NGX_OK) 
		{
            u->resolved->ctx = NULL;
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        return;
    }

	//只有不需要解析地址或者能找到upstream的情况才会走到这里
	
found:

    if (uscf == NULL) 
	{
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "no upstream configuration");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

#if (NGX_HTTP_SSL)
    u->ssl_name = uscf->host;
#endif

    if (uscf->peer.init(r, uscf) != NGX_OK)
	{
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    u->peer.start_time = ngx_current_msec;

    if (u->conf->next_upstream_tries && u->peer.tries > u->conf->next_upstream_tries)
    {
        u->peer.tries = u->conf->next_upstream_tries;
    }

	//调用方法ngx_http_upstream_connect向上游服务器发起连接
    ngx_http_upstream_connect(r, u);
}


#if (NGX_HTTP_CACHE)

static ngx_int_t
ngx_http_upstream_cache(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_int_t               rc;
    ngx_http_cache_t       *c;
    ngx_http_file_cache_t  *cache;

    c = r->cache;

    if (c == NULL) {   //该请求第一个对缓存进行访问?

		//检查请求方式是否满足缓存方式
        if (!(r->method & u->conf->cache_methods)) {  
            return NGX_DECLINED;
        }

		//获取对应的。。。
        rc = ngx_http_upstream_cache_get(r, u, &cache);
        if (rc != NGX_OK) {
            return rc;
        }

        if (r->method & NGX_HTTP_HEAD) {
            u->method = ngx_http_core_get_method;
        }

        if (ngx_http_file_cache_new(r) != NGX_OK) {
            return NGX_ERROR;
        }

		//获取对应的key值
        if (u->create_key(r) != NGX_OK) {	//ngx_http_proxy_create_key 将 `proxy_cache_key` 配置指令定义的缓存 key 根据请求信息进行取值
            return NGX_ERROR;
        }

        /* TODO: add keys */

		//计算key值的crc32和MD5值
        ngx_http_file_cache_create_key(r);  

        if (r->cache->header_start + 256 >= u->conf->buffer_size) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%V_buffer_size %uz is not enough for cache key, it should be increased to at least %uz",
                          &u->conf->module, u->conf->buffer_size, ngx_align(r->cache->header_start + 256, 1024));

            r->cache = NULL;
            return NGX_DECLINED;
        }

        u->cacheable = 1;

        c = r->cache;

        c->body_start = u->conf->buffer_size;
        c->min_uses = u->conf->cache_min_uses;
        c->file_cache = cache;


		//根据配置文件中 (proxy_cache_bypass) 缓存绕过条件和请求信息，判断是否应该继续尝试使用缓存数据响应该请求
        switch (ngx_http_test_predicates(r, u->conf->cache_bypass)) {

	        case NGX_ERROR:
	            return NGX_ERROR;

	        case NGX_DECLINED:
	            u->cache_status = NGX_HTTP_CACHE_BYPASS;
	            return NGX_DECLINED;

	        default: /* NGX_OK */
	            break;
        }

        c->lock = u->conf->cache_lock;
        c->lock_timeout = u->conf->cache_lock_timeout;
        c->lock_age = u->conf->cache_lock_age;

        u->cache_status = NGX_HTTP_CACHE_MISS;
    }

	//查找是否有对应的有效缓存数据
    rc = ngx_http_file_cache_open(r);

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream cache: %i", rc);

    switch (rc) {

    case NGX_HTTP_CACHE_UPDATING:

        if (u->conf->cache_use_stale & NGX_HTTP_UPSTREAM_FT_UPDATING) {
            u->cache_status = rc;
            rc = NGX_OK;

        } else {
            rc = NGX_HTTP_CACHE_STALE;
        }

        break;

    case NGX_OK:
        u->cache_status = NGX_HTTP_CACHE_HIT;
    }

    switch (rc) {

    case NGX_OK:

        return NGX_OK;

    case NGX_HTTP_CACHE_STALE:

        c->valid_sec = 0;
        u->buffer.start = NULL;
        u->cache_status = NGX_HTTP_CACHE_EXPIRED;

        break;

    case NGX_DECLINED:

        if ((size_t) (u->buffer.end - u->buffer.start) < u->conf->buffer_size) {
            u->buffer.start = NULL;

        } else {
            u->buffer.pos = u->buffer.start + c->header_start;
            u->buffer.last = u->buffer.pos;
        }

        break;

    case NGX_HTTP_CACHE_SCARCE:

        u->cacheable = 0;

        break;

    case NGX_AGAIN:

        return NGX_BUSY;

    case NGX_ERROR:

        return NGX_ERROR;

    default:

        /* cached NGX_HTTP_BAD_GATEWAY, NGX_HTTP_GATEWAY_TIME_OUT, etc. */

        u->cache_status = NGX_HTTP_CACHE_HIT;

        return rc;
    }

    r->cached = 0;

    return NGX_DECLINED;
}

///获取存储cache key于对应文件路径的散列表所在的。。。
static ngx_int_t
ngx_http_upstream_cache_get(ngx_http_request_t *r, ngx_http_upstream_t *u, ngx_http_file_cache_t **cache)
{
    ngx_str_t               *name, val;
    ngx_uint_t               i;
    ngx_http_file_cache_t  **caches;

    if (u->conf->cache_zone) {
        *cache = u->conf->cache_zone->data;
        return NGX_OK;
    }

    if (ngx_http_complex_value(r, u->conf->cache_value, &val) != NGX_OK) {
        return NGX_ERROR;
    }

    if (val.len == 0 || (val.len == 3 && ngx_strncmp(val.data, "off", 3) == 0)) {
        return NGX_DECLINED;
    }

    caches = u->caches->elts;

    for (i = 0; i < u->caches->nelts; i++) {
        name = &caches[i]->shm_zone->shm.name;

        if (name->len == val.len && ngx_strncmp(name->data, val.data, val.len) == 0) {
            *cache = caches[i];
            return NGX_OK;
        }
    }

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "cache \"%V\" not found", &val);

    return NGX_ERROR;
}


static ngx_int_t
ngx_http_upstream_cache_send(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_int_t          rc;
    ngx_http_cache_t  *c;

    r->cached = 1;
    c = r->cache;

    if (c->header_start == c->body_start) {
        r->http_version = NGX_HTTP_VERSION_9;
        return ngx_http_cache_send(r);
    }

    /* TODO: cache stack */

    u->buffer = *c->buf;
    u->buffer.pos += c->header_start;

    ngx_memzero(&u->headers_in, sizeof(ngx_http_upstream_headers_in_t));
    u->headers_in.content_length_n = -1;
    u->headers_in.last_modified_time = -1;

    if (ngx_list_init(&u->headers_in.headers, r->pool, 8,
                      sizeof(ngx_table_elt_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    rc = u->process_header(r);

    if (rc == NGX_OK) {

        if (ngx_http_upstream_process_headers(r, u) != NGX_OK) {
            return NGX_DONE;
        }

        return ngx_http_cache_send(r);
    }

    if (rc == NGX_ERROR) {
        return NGX_ERROR;
    }

    /* rc == NGX_HTTP_UPSTREAM_INVALID_HEADER */

    /* TODO: delete file */

    return rc;
}

#endif


static void
ngx_http_upstream_resolve_handler(ngx_resolver_ctx_t *ctx)
{
    ngx_connection_t              *c;
    ngx_http_request_t            *r;
    ngx_http_upstream_t           *u;
    ngx_http_upstream_resolved_t  *ur;

    r = ctx->data;
    c = r->connection;

    u = r->upstream;
    ur = u->resolved;

    ngx_http_set_log_request(c->log, r);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream resolve: \"%V?%V\"", &r->uri, &r->args);

    if (ctx->state)
	{
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "%V could not be resolved (%i: %s)",
                      &ctx->name, ctx->state,
                      ngx_resolver_strerror(ctx->state));

        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_BAD_GATEWAY);
        goto failed;
    }

    ur->naddrs = ctx->naddrs;
    ur->addrs = ctx->addrs;

#if (NGX_DEBUG)
    {
    u_char      text[NGX_SOCKADDR_STRLEN];
    ngx_str_t   addr;
    ngx_uint_t  i;

    addr.data = text;

    for (i = 0; i < ctx->naddrs; i++) {
        addr.len = ngx_sock_ntop(ur->addrs[i].sockaddr, ur->addrs[i].socklen,
                                 text, NGX_SOCKADDR_STRLEN, 0);

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "name was resolved to %V", &addr);
    }
    }
#endif
	// 创建轮询后端，设置获取peer和释放peer的钩子等（与ngx_http_upstream_init_round_robin_peer的功能有点类似）
    if (ngx_http_upstream_create_round_robin_peer(r, ur) != NGX_OK)
	{
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
        goto failed;
    }

    ngx_resolve_name_done(ctx);
    ur->ctx = NULL;

    u->peer.start_time = ngx_current_msec;

    if (u->conf->next_upstream_tries && u->peer.tries > u->conf->next_upstream_tries)
    {
        u->peer.tries = u->conf->next_upstream_tries;
    }

    ngx_http_upstream_connect(r, u);

failed:

    ngx_http_run_posted_requests(c);
}


static void
ngx_http_upstream_handler(ngx_event_t *ev)
{
    ngx_connection_t     *c;
    ngx_http_request_t   *r;
    ngx_http_upstream_t  *u;

	//由事件的data成员取得ngx_connection_t连接。注意，这个连接并不是Nginx与客户端的连接，而是Nginx与上游服务器间的连接
    c = ev->data;
    r = c->data;

    u = r->upstream;
	//注意，ngx_http_request_t结构体中的这个connection连接是客户端与Nginx间的连接
    c = r->connection;

    ngx_http_set_log_request(c->log, r);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream request: \"%V?%V\"", &r->uri, &r->args);

    if (ev->write)
	{
        u->write_event_handler(r, u);
    }
	else 
	{
        u->read_event_handler(r, u);
    }

    ngx_http_run_posted_requests(c);
}


static void
ngx_http_upstream_rd_check_broken_connection(ngx_http_request_t *r)
{
    ngx_http_upstream_check_broken_connection(r, r->connection->read);
}


static void
ngx_http_upstream_wr_check_broken_connection(ngx_http_request_t *r)
{
    ngx_http_upstream_check_broken_connection(r, r->connection->write);
}

//检查nginx与下游客户端之间TCP连接是否正常，如果出现错误，就会立即终止连接
static void
ngx_http_upstream_check_broken_connection(ngx_http_request_t *r, ngx_event_t *ev)
{
    int                  n;
    char                 buf[1];
    ngx_err_t            err;
    ngx_int_t            event;
    ngx_connection_t     *c;
    ngx_http_upstream_t  *u;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, ev->log, 0, "http upstream check client, write event:%d, \"%V\"", ev->write, &r->uri);

    c = r->connection;
    u = r->upstream;
	
	//如果连接已经出现错误。
    if (c->error) 
	{
        if ((ngx_event_flags & NGX_USE_LEVEL_EVENT) && ev->active)
		{
            event = ev->write ? NGX_WRITE_EVENT : NGX_READ_EVENT;

            if (ngx_del_event(ev, event, 0) != NGX_OK) 
			{
                ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;
            }
        }

        if (!u->cacheable) 
		{
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_CLIENT_CLOSED_REQUEST);
        }

        return;
    }

#if (NGX_HTTP_V2)
    if (r->stream) 
	{
        return;
    }
#endif

#if (NGX_HAVE_KQUEUE)

    if (ngx_event_flags & NGX_USE_KQUEUE_EVENT) 
	{

        if (!ev->pending_eof) 
		{
            return;
        }

        ev->eof = 1;
        c->error = 1;

        if (ev->kq_errno)
		{
            ev->error = 1;
        }

        if (!u->cacheable && u->peer.connection) 
		{
            ngx_log_error(NGX_LOG_INFO, ev->log, ev->kq_errno, "kevent() reported that client prematurely closed "
                          "connection, so upstream connection is closed too");
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_CLIENT_CLOSED_REQUEST);
            return;
        }

        ngx_log_error(NGX_LOG_INFO, ev->log, ev->kq_errno, "kevent() reported that client prematurely closed connection");

        if (u->peer.connection == NULL) 
		{
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_CLIENT_CLOSED_REQUEST);
        }

        return;
    }

#endif

#if (NGX_HAVE_EPOLLRDHUP)

    if ((ngx_event_flags & NGX_USE_EPOLL_EVENT) && ev->pending_eof)
	{
        socklen_t  len;

        ev->eof = 1;
        c->error = 1;

        err = 0;
        len = sizeof(ngx_err_t);

        /*
         * BSDs and Linux return 0 and set a pending error in err, Solaris returns -1 and sets errno
         */

        if (getsockopt(c->fd, SOL_SOCKET, SO_ERROR, (void *) &err, &len) == -1)
        {
            err = ngx_socket_errno;
        }

        if (err) 
		{
            ev->error = 1;
        }

        if (!u->cacheable && u->peer.connection)
		{
            ngx_log_error(NGX_LOG_INFO, ev->log, err, "epoll_wait() reported that client prematurely closed "
                        "connection, so upstream connection is closed too");
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_CLIENT_CLOSED_REQUEST);
            return;
        }

        ngx_log_error(NGX_LOG_INFO, ev->log, err, "epoll_wait() reported that client prematurely closed connection");

        if (u->peer.connection == NULL) 
		{
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_CLIENT_CLOSED_REQUEST);
        }

        return;
    }

#endif

    n = recv(c->fd, buf, 1, MSG_PEEK);

    err = ngx_socket_errno;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, ev->log, err, "http upstream recv(): %d", n);

	//n = 0时连接断开，为什么还直接返回
    if (ev->write && (n >= 0 || err == NGX_EAGAIN)) 
	{
        return;
    }

	/*如果是水平触发，且事件在事件驱动机制中，则将其移除*/
	/*防止其重复不停的触发事件*/
    if ((ngx_event_flags & NGX_USE_LEVEL_EVENT) && ev->active)
	{
        event = ev->write ? NGX_WRITE_EVENT : NGX_READ_EVENT;

        if (ngx_del_event(ev, event, 0) != NGX_OK)
		{
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
    }

    if (n > 0)
	{
        return;
    }

    if (n == -1)
	{
        if (err == NGX_EAGAIN) 
		{
            return;
        }

        ev->error = 1;

    }
	else 
	{ /* n == 0 */
        err = 0;
    }

    ev->eof = 1;
    c->error = 1;

    if (!u->cacheable && u->peer.connection) 
	{
        ngx_log_error(NGX_LOG_INFO, ev->log, err, "client prematurely closed connection, so upstream connection is closed too");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_CLIENT_CLOSED_REQUEST);
        return;
    }

    ngx_log_error(NGX_LOG_INFO, ev->log, err, "client prematurely closed connection");

	//如果有cache，并且后端的upstream还在处理，则此时继续处理upstream，忽略对端的错误.
    if (u->peer.connection == NULL) 
	{
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_CLIENT_CLOSED_REQUEST);
    }
}

//建立连接。
//upstream 机制与上游服务器建立 TCP 连接时，采用的是非阻塞模式的套接字，即发起连接请求之后立即返回，不管连接是否建立成功，
//若没有立即建立成功，则需在 epoll 事件机制中监听该套接字，当它出现可写事件时，就说明连接已经建立成功了
static void
ngx_http_upstream_connect(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_int_t          rc;
    ngx_connection_t  *c;

    r->connection->log->action = "connecting to upstream";

    if (u->state && u->state->response_time) 
	{
        u->state->response_time = ngx_current_msec - u->state->response_time;
    }

    u->state = ngx_array_push(r->upstream_states);
    if (u->state == NULL) 
	{
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    ngx_memzero(u->state, sizeof(ngx_http_upstream_state_t));

    u->state->response_time = ngx_current_msec;
    u->state->connect_time = (ngx_msec_t) -1;
    u->state->header_time = (ngx_msec_t) -1;

	 /* 向上游服务器发起连接请求，需要注意的是该方法已经将相应的套接字注册到epoll事件机制来监听读、写事件 */
    rc = ngx_event_connect_peer(&u->peer);

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream connect: %i", rc);

	/* 下面根据rc不同返回值进行分析......*/  

	/* 若建立连接失败，则调用ngx_http_upstream_finalize_request关闭当前请求，并return从当前函数返回 */  
    if (rc == NGX_ERROR) {
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    u->state->peer = u->peer.name;

	 /*
     * 若返回rc = NGX_BUSY，表示当前上游服务器不活跃，则调用ngx_http_upstream_next向上游服务器重新发起连接，
     * 实际上，该方法最终还是调用ngx_http_upstream_connect方法, 并return从当前函数返回；
     */
    if (rc == NGX_BUSY) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "no live upstreams");
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_NOLIVE);
        return;
    }

	/*
     * 若返回rc = NGX_DECLINED，表示当前上游服务器负载过重，则调用ngx_http_upstream_next向上游其它服务器重新发起连接，
     * 实际上，该方法最终还是调用ngx_http_upstream_connect方法,并return从当前函数返回；
     */
    if (rc == NGX_DECLINED) {
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

    /* rc == NGX_OK || rc == NGX_AGAIN || rc == NGX_DONE */

    c = u->peer.connection;

    c->data = r;

	/* 设置当前连接ngx_connection_t 上读、写事件的回调方法 */
    c->write->handler = ngx_http_upstream_handler;
    c->read->handler = ngx_http_upstream_handler;
	 /* 设置upstream机制的读、写事件的回调方法 */
    u->write_event_handler = ngx_http_upstream_send_request_handler;
    u->read_event_handler = ngx_http_upstream_process_header;

    c->sendfile &= r->connection->sendfile;
    u->output.sendfile = c->sendfile;

    if (c->pool == NULL)
	{
        /* we need separate pool here to be able to cache SSL connections */
        c->pool = ngx_create_pool(128, r->connection->log);
        if (c->pool == NULL)
		{
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
    }

    c->log = r->connection->log;
    c->pool->log = c->log;
    c->read->log = c->log;
    c->write->log = c->log;

    /* init or reinit the ngx_output_chain() and ngx_chain_writer() contexts */

    u->writer.out = NULL;
    u->writer.last = &u->writer.out;
    u->writer.connection = c;
    u->writer.limit = 0;


	/*
	 * 检查当前ngx_http_upstream_t 结构的request_sent标志位，
	 * 若该标志位为1，则表示已经向上游服务器发送请求，即本次发起连接失败；
	 * 则调用ngx_http_upstream_reinit方法重新向上游服务器发起连接；
	 */

    if (u->request_sent) {
		//重新初始化upstream
        if (ngx_http_upstream_reinit(r, u) != NGX_OK) {
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
    }
	//如果request_body存在的话，保存request_body
    if (r->request_body && r->request_body->buf && r->request_body->temp_file && r == r->main) {
        /*
         * the r->request_body->buf can be reused for one request only,
         * the subrequests should allocate their own temporary bufs
         */

        u->output.free = ngx_alloc_chain_link(r->pool);
        if (u->output.free == NULL) {
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
		//保存到output
        u->output.free->buf = r->request_body->buf;
        u->output.free->next = NULL;
        u->output.allocated = 1;
		//重置request_body
        r->request_body->buf->pos = r->request_body->buf->start;
        r->request_body->buf->last = r->request_body->buf->start;
        r->request_body->buf->tag = u->output.tag;
    }

    u->request_sent = 0;

	/*
     * 若返回rc = NGX_AGAIN，表示当前已经发起连接，但是没有收到上游服务器的确认应答报文，即上游连接的写事件不可写，
     * 由于写事件已经添加到epoll事件机制中等待可写事件发生，
     * 所有在这里只需将当前连接的写事件添加到定时器机制中，管理超时确认应答；
     * 并return从当前函数返回；
     */
    if (rc == NGX_AGAIN) {
        ngx_add_timer(c->write, u->conf->connect_timeout);
        return;
    }

	/*
     * 若返回值rc = NGX_OK，表示连接成功建立，
     * 调用此方法向上游服务器发送请求 */
#if (NGX_HTTP_SSL)

    if (u->ssl && c->ssl == NULL) {
        ngx_http_upstream_ssl_init_connection(r, u, c);
        return;
    }

#endif

    ngx_http_upstream_send_request(r, u, 1);
}


#if (NGX_HTTP_SSL)

static void
ngx_http_upstream_ssl_init_connection(ngx_http_request_t *r, ngx_http_upstream_t *u, ngx_connection_t *c)
{
    int                        tcp_nodelay;
    ngx_int_t                  rc;
    ngx_http_core_loc_conf_t  *clcf;

    if (ngx_http_upstream_test_connect(c) != NGX_OK)
	{
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

    if (ngx_ssl_create_connection(u->conf->ssl, c, NGX_SSL_BUFFER|NGX_SSL_CLIENT) != NGX_OK)
    {
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    c->sendfile = 0;
    u->output.sendfile = 0;

    if (u->conf->ssl_server_name || u->conf->ssl_verify) 
	{
        if (ngx_http_upstream_ssl_name(r, u, c) != NGX_OK) 
		{
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
    }

    if (u->conf->ssl_session_reuse) 
	{
        if (u->peer.set_session(&u->peer, u->peer.data) != NGX_OK)
		{
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        /* abbreviated SSL handshake may interact badly with Nagle */

        clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

        if (clcf->tcp_nodelay && c->tcp_nodelay == NGX_TCP_NODELAY_UNSET)
		{
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "tcp_nodelay");

            tcp_nodelay = 1;

            if (setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY, (const void *) &tcp_nodelay, sizeof(int)) == -1)
            {
                ngx_connection_error(c, ngx_socket_errno, "setsockopt(TCP_NODELAY) failed");
                ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;
            }

            c->tcp_nodelay = NGX_TCP_NODELAY_SET;
        }
    }

    r->connection->log->action = "SSL handshaking to upstream";

    rc = ngx_ssl_handshake(c);

    if (rc == NGX_AGAIN)
	{
        if (!c->write->timer_set)
		{
            ngx_add_timer(c->write, u->conf->connect_timeout);
        }

        c->ssl->handler = ngx_http_upstream_ssl_handshake;
        return;
    }

    ngx_http_upstream_ssl_handshake(c);
}


static void
ngx_http_upstream_ssl_handshake(ngx_connection_t *c)
{
    long                  rc;
    ngx_http_request_t   *r;
    ngx_http_upstream_t  *u;

    r = c->data;
    u = r->upstream;

    ngx_http_set_log_request(c->log, r);

    if (c->ssl->handshaked) 
	{

        if (u->conf->ssl_verify) {
            rc = SSL_get_verify_result(c->ssl->connection);

            if (rc != X509_V_OK) {
                ngx_log_error(NGX_LOG_ERR, c->log, 0,
                              "upstream SSL certificate verify error: (%l:%s)",
                              rc, X509_verify_cert_error_string(rc));
                goto failed;
            }

            if (ngx_ssl_check_host(c, &u->ssl_name) != NGX_OK) 
			{
                ngx_log_error(NGX_LOG_ERR, c->log, 0, "upstream SSL certificate does not match \"%V\"", &u->ssl_name);
                goto failed;
            }
        }

        if (u->conf->ssl_session_reuse)
		{
            u->peer.save_session(&u->peer, u->peer.data);
        }

        c->write->handler = ngx_http_upstream_handler;
        c->read->handler = ngx_http_upstream_handler;

        c = r->connection;

        ngx_http_upstream_send_request(r, u, 1);

        ngx_http_run_posted_requests(c);
        return;
    }

failed:

    c = r->connection;

    ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);

    ngx_http_run_posted_requests(c);
}


static ngx_int_t
ngx_http_upstream_ssl_name(ngx_http_request_t *r, ngx_http_upstream_t *u, ngx_connection_t *c)
{
    u_char     *p, *last;
    ngx_str_t   name;

    if (u->conf->ssl_name)
	{
        if (ngx_http_complex_value(r, u->conf->ssl_name, &name) != NGX_OK) 
		{
            return NGX_ERROR;
        }

    } 
	else 
	{
        name = u->ssl_name;
    }

    if (name.len == 0) 
	{
        goto done;
    }

    /*
     * ssl name here may contain port, notably if derived from $proxy_host
     * or $http_host; we have to strip it
     */

    p = name.data;
    last = name.data + name.len;

    if (*p == '[') 
	{
        p = ngx_strlchr(p, last, ']');

        if (p == NULL) 
		{
            p = name.data;
        }
    }

    p = ngx_strlchr(p, last, ':');

    if (p != NULL) {
        name.len = p - name.data;
    }

    if (!u->conf->ssl_server_name) 
	{
        goto done;
    }

#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME

    /* as per RFC 6066, literal IPv4 and IPv6 addresses are not permitted */

    if (name.len == 0 || *name.data == '[') {
        goto done;
    }

    if (ngx_inet_addr(name.data, name.len) != INADDR_NONE) {
        goto done;
    }

    /*
     * SSL_set_tlsext_host_name() needs a null-terminated string,
     * hence we explicitly null-terminate name here
     */

    p = ngx_pnalloc(r->pool, name.len + 1);
    if (p == NULL) 
	{
        return NGX_ERROR;
    }

    (void) ngx_cpystrn(p, name.data, name.len + 1);

    name.data = p;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "upstream SSL server name: \"%s\"", name.data);

    if (SSL_set_tlsext_host_name(c->ssl->connection, name.data) == 0)
	{
        ngx_ssl_error(NGX_LOG_ERR, r->connection->log, 0, "SSL_set_tlsext_host_name(\"%s\") failed", name.data);
        return NGX_ERROR;
    }

#endif

done:

    u->ssl_name = name;

    return NGX_OK;
}

#endif


static ngx_int_t
ngx_http_upstream_reinit(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    off_t         file_pos;
    ngx_chain_t  *cl;

    if (u->reinit_request(r) != NGX_OK) 
	{
        return NGX_ERROR;
    }

    u->keepalive = 0;
    u->upgrade = 0;

    ngx_memzero(&u->headers_in, sizeof(ngx_http_upstream_headers_in_t));
    u->headers_in.content_length_n = -1;
    u->headers_in.last_modified_time = -1;

    if (ngx_list_init(&u->headers_in.headers, r->pool, 8, sizeof(ngx_table_elt_t)) != NGX_OK)
    {
        return NGX_ERROR;
    }

    /* reinit the request chain */

    file_pos = 0;

    for (cl = u->request_bufs; cl; cl = cl->next) 
	{
        cl->buf->pos = cl->buf->start;

        /* there is at most one file */

        if (cl->buf->in_file) {
            cl->buf->file_pos = file_pos;
            file_pos = cl->buf->file_last;
        }
    }

    /* reinit the subrequest's ngx_output_chain() context */

    if (r->request_body && r->request_body->temp_file && r != r->main && u->output.buf)
    {
        u->output.free = ngx_alloc_chain_link(r->pool);
        if (u->output.free == NULL)
		{
            return NGX_ERROR;
        }

        u->output.free->buf = u->output.buf;
        u->output.free->next = NULL;

        u->output.buf->pos = u->output.buf->start;
        u->output.buf->last = u->output.buf->start;
    }

    u->output.buf = NULL;
    u->output.in = NULL;
    u->output.busy = NULL;

    /* reinit u->buffer */

    u->buffer.pos = u->buffer.start;

#if (NGX_HTTP_CACHE)

    if (r->cache) {
        u->buffer.pos += r->cache->header_start;
    }

#endif

    u->buffer.last = u->buffer.pos;

    return NGX_OK;
}

//发送请求。
//当 Nginx 与上游服务器成功建立连接之后，会调用 ngx_http_upstream_send_request 方法发送请求，
//若是该方法不能一次性把请求内容发送完成时，则需等待 epoll 事件机制的写事件发生，若写事件发生，
//则会调用写事件 write_event_handler 的回调方法 ngx_http_upstream_send_request_handler 继续发送请求，
//并且有可能会多次调用该写事件的回调方法， 直到把请求发送完成。
static void
ngx_http_upstream_send_request(ngx_http_request_t *r, ngx_http_upstream_t *u, ngx_uint_t do_write)
{
    ngx_int_t          rc;
    ngx_connection_t  *c;

    c = u->peer.connection;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream send request");

    if (u->state->connect_time == (ngx_msec_t) -1) {
        u->state->connect_time = ngx_current_msec - u->state->response_time;
    }

	/*
     * 若标志位request_sent为0，表示还未向上游发送请求；
     * 且此时调用ngx_http_upstream_test_connect方法测试是否与上游建立连接，
     * 若返回非NGX_OK，标志当前还未与上游服务器成功建立连接，则需要调用ngx_http_upstream_next方法尝试与下一个上游服务器建立连接；
     * 并return从当前函数返回；
     */
    if (!u->request_sent && ngx_http_upstream_test_connect(c) != NGX_OK) {
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

    c->log->action = "sending request to upstream";

    rc = ngx_http_upstream_send_request_body(r, u, do_write);

	/*
     * 若返回值rc=NGX_ERROR，表示当前连接上出错，
     * 将错误信息传递给ngx_http_upstream_next方法，
     * 该方法根据错误信息决定是否重新向上游服务器发起连接；
     * 并return从当前函数返回；
     */
    if (rc == NGX_ERROR) {
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        ngx_http_upstream_finalize_request(r, u, rc);
        return;
    }

	/*
     * 若返回值rc = NGX_AGAIN，表示请求数据并未完全发送，
     * 即有剩余的请求数据保存在output中，但此时，写事件已经不可写，
     * 则调用ngx_add_timer方法把当前连接上的写事件添加到定时器机制，
     * 并调用ngx_handle_write_event方法将写事件注册到epoll事件机制中；
     * 并return从当前函数返回；
     */
    if (rc == NGX_AGAIN)
	{
        if (!c->write->ready) 
		{
            ngx_add_timer(c->write, u->conf->send_timeout);

        } 
		else if (c->write->timer_set) 
		{
            ngx_del_timer(c->write);
        }

        if (ngx_handle_write_event(c->write, u->conf->send_lowat) != NGX_OK) 
		{
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        return;
    }

	/*
	 * 若返回值 rc = NGX_OK，表示已经发送完全部请求数据，准备接收来自上游服务器的响应报文，则执行以下程序；
	 */

  	//检查当前连接上写事件的标志位timer_set是否为1，若该标志位为1，则需把写事件从定时器机制中移除
    if (c->write->timer_set)
	{
        ngx_del_timer(c->write);
    }

    if (c->tcp_nopush == NGX_TCP_NOPUSH_SET)
	{
        if (ngx_tcp_push(c->fd) == NGX_ERROR) 
		{
            ngx_log_error(NGX_LOG_CRIT, c->log, ngx_socket_errno, ngx_tcp_push_n " failed");
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        c->tcp_nopush = NGX_TCP_NOPUSH_UNSET;
    }

	//由于此时请求以全部发送到上游服务器了，重置write_event_handler为ngx_http_upstream_dummy_handler
	//该方法不做任何事情，这样即使与上游服务器TCP连接上再次有可写事件时也不会有任何动作发生
	//并把写事件注册到epoll事件机制中；
    u->write_event_handler = ngx_http_upstream_dummy_handler;

    if (ngx_handle_write_event(c->write, 0) != NGX_OK) 
	{
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

	//将当前连接上读事件添加到定时器机制中，检查接收响应是否超时
    ngx_add_timer(c->read, u->conf->read_timeout);

	 /*
     * 若此时，读事件已经准备就绪，
     * 则调用ngx_http_upstream_process_header方法开始接收并处理响应头部；
     * 并return从当前函数返回；
     */
    if (c->read->ready) {
        ngx_http_upstream_process_header(r, u);
        return;
    }
}


static ngx_int_t
ngx_http_upstream_send_request_body(ngx_http_request_t *r, ngx_http_upstream_t *u, ngx_uint_t do_write)
{
    int                        tcp_nodelay;
    ngx_int_t                  rc;
    ngx_chain_t               *out, *cl, *ln;
    ngx_connection_t          *c;
    ngx_http_core_loc_conf_t  *clcf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream send request body");

    if (!r->request_body_no_buffering) {
       /* buffered request body */
       if (!u->request_sent) {
           u->request_sent = 1;
           out = u->request_bufs;
       }
	   else 
	   {
           out = NULL;
       }
	/*
     * 调用ngx_output_chain方法向上游发送保存在request_bufs链表中的请求数据；
     * 值得注意的是该方法的第二个参数可以是NULL也可以是request_bufs，那怎么来区分呢？
     * 若是第一次调用该方法发送request_bufs链表中的请求数据时，request_sent标志位为0，
     * 此时，第二个参数自然就是request_bufs了，那么为什么会有NULL作为参数的情况呢？
     * 当在第一次调用该方法时，并不能一次性把所有request_bufs中的数据发送完毕时，
     * 此时，会把剩余的数据保存在output结构里面，并把标志位request_sent设置为1，
     * 因此，再次发送请求数据时，不用指定request_bufs参数，因为此时剩余数据已经保存在output中；
     */
       return ngx_output_chain(&u->output, out);
    }

    if (!u->request_sent)
	{
        u->request_sent = 1;
        out = u->request_bufs;

        if (r->request_body->bufs) 
		{
            for (cl = out; cl->next; cl = out->next) { /* void */ }
            cl->next = r->request_body->bufs;
            r->request_body->bufs = NULL;
        }

        c = u->peer.connection;
        clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

		//设置上游连接TCP_NODELAY选项
        if (clcf->tcp_nodelay && c->tcp_nodelay == NGX_TCP_NODELAY_UNSET) 
		{
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "tcp_nodelay");

            tcp_nodelay = 1;

            if (setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY, (const void *) &tcp_nodelay, sizeof(int)) == -1)
            {
                ngx_connection_error(c, ngx_socket_errno, "setsockopt(TCP_NODELAY) failed");
                return NGX_ERROR;
            }

            c->tcp_nodelay = NGX_TCP_NODELAY_SET;
        }
		//为什么???
        r->read_event_handler = ngx_http_upstream_read_request_handler;

    }
	else 
	{
        out = NULL;
    }

    for ( ;; ) 
	{

        if (do_write)
		{
            rc = ngx_output_chain(&u->output, out);

            if (rc == NGX_ERROR) {
                return NGX_ERROR;
            }

            while (out) {
                ln = out;
                out = out->next;
                ngx_free_chain(r->pool, ln);
            }

            if (rc == NGX_OK && !r->reading_body) {
                break;
            }
        }

        if (r->reading_body) 
		{
            /* read client request body */

            rc = ngx_http_read_unbuffered_request_body(r);

            if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
                return rc;
            }

            out = r->request_body->bufs;
            r->request_body->bufs = NULL;
        }

        /* stop if there is nothing to send */

        if (out == NULL) 
		{
            rc = NGX_AGAIN;
            break;
        }

        do_write = 1;
    }

    if (!r->reading_body)
	{
        if (!u->store && !r->post_action && !u->conf->ignore_client_abort) 
		{
            r->read_event_handler = ngx_http_upstream_rd_check_broken_connection;
        }
    }

    return rc;
}


static void
ngx_http_upstream_send_request_handler(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_connection_t  *c;

	//获取与上游服务器表示连接的ngx_connection_t结构体
    c = u->peer.connection;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream send request handler");

	//检查是否向上游服务器发送的请求已经超时
	//若写事件超时，则将超时错误传递给方法，该方法将会根据允许的错误重连策略决定:重新发起连接执行upstream请求，或者结束upstream请求。
    if (c->write->timedout) {
		//执行超时重连机制 
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_TIMEOUT);
        return;
    }

#if (NGX_HTTP_SSL)

    if (u->ssl && c->ssl == NULL) 
	{
        ngx_http_upstream_ssl_init_connection(r, u, c);
        return;
    }

#endif

	//检查 header_sent 标志位是否为 1，表示已经接收到来自上游服务器的响应头部，则不需要(不应该)继续向上游发送请求，
	//所以把写事件的回调方法设置为任何工作都没有做的 ngx_http_upstream_dummy_handler，同时将写事件注册到 epoll 事件机制中，
	//并return 从当前函数返回；
    if (u->header_sent) 
	{
        u->write_event_handler = ngx_http_upstream_dummy_handler;
        (void) ngx_handle_write_event(c->write, 0);

		//因为不存在继续发送请求到上游的可能，所以直接返回
        return;
    }
	
	//若没有接收来自上游服务器的响应头部，则需向上游服务器发送请求数据  
    ngx_http_upstream_send_request(r, u, 1);
}


static void
ngx_http_upstream_read_request_handler(ngx_http_request_t *r)
{
    ngx_connection_t     *c;
    ngx_http_upstream_t  *u;

    c = r->connection;
    u = r->upstream;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream read request handler");

    if (c->read->timedout) 
	{
        c->timedout = 1;
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_REQUEST_TIME_OUT);
        return;
    }

    ngx_http_upstream_send_request(r, u, 0);
}

//接收响应 --接收响应头部
//当 Nginx 已经向上游发送请求，准备开始接收来自上游的响应头部，由方法 ngx_http_upstream_process_header 实现，该方法接收并解析响应头部
static void
ngx_http_upstream_process_header(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ssize_t            n;
    ngx_int_t          rc;
    ngx_connection_t  *c;

    c = u->peer.connection;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream process header");

    c->log->action = "reading response header from upstream";

	//检查读事件是否超时
	//检查上游连接上的读事件是否超时，若标志位timedout为 1，则表示超时，把超时错误传递给ngx_http_upstream_next方法，
	//该方法根据允许的错误进行重连接策略，并 return 从当前函数返回
    if (c->read->timedout) {
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_TIMEOUT);
        return;
    }

	//检查是否发送了请求给上游服务器

	/*
     * 若标志位request_sent为0，表示还未发送请求；
     * 且ngx_http_upstream_test_connect方法返回非NGX_OK，标志当前还未与上游服务器成功建立连接；
     * 则需要调用ngx_http_upstream_next方法尝试与下一个上游服务器建立连接；
     * 并return从当前函数返回；
     */
    if (!u->request_sent && ngx_http_upstream_test_connect(c) != NGX_OK) {
		//还没有发送请求到上游服务器就收到了来自上游的响应，不符合upstream的设计场景
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

	//检查 ngx_http_upstream_t 结构体中接收响应头部的 buffer 缓冲区是否有内存空间以便接收响应头部，
	//若 buffer.start 为 NULL，表示该缓冲区为空，则需调用 ngx_palloc 方法分配内存，
	//该内存大小 buffer_size 由 ngx_http_upstream_conf_t 配置结构体的 buffer_size 成员指定
    if (u->buffer.start == NULL) {
        u->buffer.start = ngx_palloc(r->pool, u->conf->buffer_size);
        if (u->buffer.start == NULL) {
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        u->buffer.pos = u->buffer.start;
        u->buffer.last = u->buffer.start;
        u->buffer.end = u->buffer.start + u->conf->buffer_size;
		/* 表示该缓冲区内存可被复用、数据可被改变 */
        u->buffer.temporary = 1;

        u->buffer.tag = u->output.tag;

        if (ngx_list_init(&u->headers_in.headers, r->pool, 8, sizeof(ngx_table_elt_t)) != NGX_OK) {
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

#if (NGX_HTTP_CACHE)

        if (r->cache) {
            u->buffer.pos += r->cache->header_start;
            u->buffer.last = u->buffer.pos;
        }
#endif
    }

    for ( ;; ) 
	{
		/* 调用recv方法从当前连接上读取响应头部数据 */
        n = c->recv(c, u->buffer.last, u->buffer.end - u->buffer.last);
		/* 下面根据 recv 方法不同返回值 n 进行判断 */
		
		/*
         * 若返回值 n = NGX_AGAIN，表示读事件未准备就绪，
         * 需等待下次读事件被触发时继续接收响应头部，
         * 即将读事件注册到epoll事件机制中，等待可读事件发生；
         * 并return从当前函数返回；
         */
        if (n == NGX_AGAIN) {
#if 0
            ngx_add_timer(rev, u->read_timeout);
#endif

            if (ngx_handle_read_event(c->read, 0) != NGX_OK) {
                ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;
            }

            return;
        }

        if (n == 0) {
            ngx_log_error(NGX_LOG_ERR, c->log, 0, "upstream prematurely closed connection");
        }

	       /*
         * 若返回值 n = NGX_ERROR 或 n = 0，则表示上游服务器已经主动关闭连接；
         * 此时，调用ngx_http_upstream_next方法决定是否重新发起连接；
         * 并return从当前函数返回；
         */
        if (n == NGX_ERROR || n == 0) {
            ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
            return;
        }

		 /* 若返回值 n 大于 0，表示已经接收到响应头部 */
		
        u->buffer.last += n;

#if 0
        u->valid_header_in = 0;

        u->peer.cached = 0;
#endif
		//HTTP模块会通过process_header方法解析包头(将解析出的值设置到ngx_http_upstream结构体的headers_in成员中)
		//并根据该方法返回值进行不同的判断；
        rc = u->process_header(r);

		  /*
         * 若返回值 rc = NGX_AGAIN，表示接收到的响应头部不完整，需等待下次读事件被触发时继续接收响应头部；
         * continue继续接收响应；
         */
        if (rc == NGX_AGAIN) {
			//检查接收缓冲区 buffer 是否还有剩余的内存空间，若缓冲区没有剩余的内存空间，表示接收到的响应头部过大，
			//此时调用 ngx_http_upstream_next 方法重新建立连接，并 return 从当前函数返回；若缓冲区还有剩余的内存空间，
			//则continue 继续接收响应头部
            if (u->buffer.last == u->buffer.end) {
                ngx_log_error(NGX_LOG_ERR, c->log, 0, "upstream sent too big header");

                ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_INVALID_HEADER);
                return;
            }

            continue;
        }

        break;
    }

	    /*
     * 若返回值 rc = NGX_HTTP_UPSTREAM_INVALID_HEADER，
     * 则表示接收到的响应头部是非法的，
     * 调用ngx_http_upstream_next方法决定是否重新发起连接；
     * 并return从当前函数返回；
     */
    if (rc == NGX_HTTP_UPSTREAM_INVALID_HEADER) {
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_INVALID_HEADER);
        return;
    }

	//若 rc = NGX_ERROR，表示连接出错，此时调用 ngx_http_upstream_finalize_request 方法结束请求，并 return 从当前函数返回
    if (rc == NGX_ERROR) {
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    /* rc == NGX_OK */

	/*若返回值 rc = NGX_OK，表示成功解析到完整的响应头部；*/
    u->state->header_time = ngx_current_msec - u->state->response_time;

    if (u->headers_in.status_n >= NGX_HTTP_SPECIAL_RESPONSE) {
        if (ngx_http_upstream_test_next(r, u) == NGX_OK) {
            return;
        }

        if (ngx_http_upstream_intercept_errors(r, u) == NGX_OK) {
            return;
        }
    }
	
	//调用ngx_http_upstream_process_headers方法处理已解析处理的响应头部 
	//该方法将会把已经解析出的头部设置到ngx_http_request_t请求结构体的headers_out成员中，
	//这样在调用ngx_http_send_header方法发送响应包头给客户端时将会发送这些设置好了的头部
    if (ngx_http_upstream_process_headers(r, u) != NGX_OK) {
        return;
    }
	
//upstream 处理上游响应包体的三种方式:	
//当请求结构体 ngx_http_request_t 中的成员subrequest_in_memory 标志位为 1 时，upstream 不转发响应包体到下游，并由HTTP 模块实现的input_filter() 方法处理包体；
//当请求结构体 ngx_http_request_t 中的成员subrequest_in_memory 标志位为 0 时，且ngx_http_upstream_conf_t 配置结构体中的成员buffering 标志位为 1 时，upstream 将开启更多的内存和磁盘文件用于缓存上游的响应包体（此时，上游网速更快），并转发响应包体；
//当请求结构体 ngx_http_request_t 中的成员subrequest_in_memory 标志位为 0 时，且ngx_http_upstream_conf_t 配置结构体中的成员buffering 标志位为 0 时，upstream 将使用固定大小的缓冲区来转发响应包体；

	/*
     * 检查ngx_http_request_t 结构体的subrequest_in_memory成员决定是否转发响应给下游服务器；
     * 若该标志位为0，则需调用ngx_http_upstream_send_response方法转发响应给下游服务器；
     * 并return从当前函数返回；
     */
    if (!r->subrequest_in_memory) {
		//转发响应给客户端
        ngx_http_upstream_send_response(r, u);
        return;
    }
	/* 若不需要转发响应，则调用ngx_http_upstream_t中的input_filter方法处理响应包体 */
    /* subrequest content in memory */

	//检查HTTP模块是否是实现了用于处理包体的input_filter方法，
	//如果没有实现，则使用upstream定义的默认方法代替之
    if (u->input_filter == NULL) 
	{
        u->input_filter_init = ngx_http_upstream_non_buffered_filter_init;
        u->input_filter = ngx_http_upstream_non_buffered_filter;
        u->input_filter_ctx = r;
    }

	    /*
     * 调用input_filter_init方法为处理包体做初始化工作；
     */
    if (u->input_filter_init(u->input_filter_ctx) == NGX_ERROR)
	{
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }


	/*
     * 检查接收缓冲区是否有剩余的响应数据；
     * 因为响应头部已经解析完毕，若接收缓冲区还有未被解析的剩余数据，
     * 则该数据就是响应包体；
     */
     
    n = u->buffer.last - u->buffer.pos;
	/*
     * 若接收缓冲区有剩余的响应包体，调用input_filter方法开始处理已接收到响应包体；
     */
    if (n)
	{
        u->buffer.last = u->buffer.pos;

        u->state->response_length += n;
		 /* 调用input_filter方法处理响应包体 */
        if (u->input_filter(u->input_filter_ctx, n) == NGX_ERROR)
		{
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }
    }

    if (u->length == 0) 
	{
        ngx_http_upstream_finalize_request(r, u, 0);
        return;
    }

	/* 设置upstream机制的读事件回调方法read_event_handler为ngx_http_upstream_process_body_in_memory */
    u->read_event_handler = ngx_http_upstream_process_body_in_memory;

	/* 调用ngx_http_upstream_process_body_in_memory方法开始处理响应包体 */
    ngx_http_upstream_process_body_in_memory(r, u);
}


static ngx_int_t
ngx_http_upstream_test_next(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_uint_t                 status;
    ngx_http_upstream_next_t  *un;

    status = u->headers_in.status_n;  

    for (un = ngx_http_upstream_next_errors; un->status; un++) {

        if (status != un->status) {
            continue;
        }

        if (u->peer.tries > 1 && (u->conf->next_upstream & un->mask)) {
            ngx_http_upstream_next(r, u, un->mask);
            return NGX_OK;
        }

#if (NGX_HTTP_CACHE)

        if (u->cache_status == NGX_HTTP_CACHE_EXPIRED && (u->conf->cache_use_stale & un->mask)) {
            ngx_int_t  rc;

            rc = u->reinit_request(r);

            if (rc == NGX_OK) {
                u->cache_status = NGX_HTTP_CACHE_STALE;
                rc = ngx_http_upstream_cache_send(r, u);
            }

            ngx_http_upstream_finalize_request(r, u, rc);
            return NGX_OK;
        }

#endif
    }

#if (NGX_HTTP_CACHE)

    if (status == NGX_HTTP_NOT_MODIFIED && u->cache_status == NGX_HTTP_CACHE_EXPIRED && u->conf->cache_revalidate) {
        time_t     now, valid;
        ngx_int_t  rc;

        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream not modified");

        now = ngx_time();
        valid = r->cache->valid_sec;

        rc = u->reinit_request(r);

        if (rc != NGX_OK) {
            ngx_http_upstream_finalize_request(r, u, rc);
            return NGX_OK;
        }

        u->cache_status = NGX_HTTP_CACHE_REVALIDATED;
        rc = ngx_http_upstream_cache_send(r, u);

        if (valid == 0) {
            valid = r->cache->valid_sec;
        }

        if (valid == 0) {
            valid = ngx_http_file_cache_valid(u->conf->cache_valid, u->headers_in.status_n);
            if (valid) {
                valid = now + valid;
            }
        }

        if (valid) {
            r->cache->valid_sec = valid;
            r->cache->date = now;

            ngx_http_file_cache_update_header(r);
        }

        ngx_http_upstream_finalize_request(r, u, rc);
        return NGX_OK;
    }

#endif

    return NGX_DECLINED;
}


static ngx_int_t
ngx_http_upstream_intercept_errors(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_int_t                  status;
    ngx_uint_t                 i;
    ngx_table_elt_t           *h;
    ngx_http_err_page_t       *err_page;
    ngx_http_core_loc_conf_t  *clcf;

    status = u->headers_in.status_n;

    if (status == NGX_HTTP_NOT_FOUND && u->conf->intercept_404) {
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_NOT_FOUND);
        return NGX_OK;
    }

    if (!u->conf->intercept_errors) {
        return NGX_DECLINED;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (clcf->error_pages == NULL) {
        return NGX_DECLINED;
    }

    err_page = clcf->error_pages->elts;
    for (i = 0; i < clcf->error_pages->nelts; i++) {

        if (err_page[i].status == status) {

            if (status == NGX_HTTP_UNAUTHORIZED && u->headers_in.www_authenticate) {
                h = ngx_list_push(&r->headers_out.headers);

                if (h == NULL) {
                    ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
                    return NGX_OK;
                }

                *h = *u->headers_in.www_authenticate;

                r->headers_out.www_authenticate = h;
            }

#if (NGX_HTTP_CACHE)

            if (r->cache) {
                time_t  valid;

                valid = ngx_http_file_cache_valid(u->conf->cache_valid, status);

                if (valid) {
                    r->cache->valid_sec = ngx_time() + valid;
                    r->cache->error = status;
                }

                ngx_http_file_cache_free(r->cache, u->pipe->temp_file);
            }
#endif
            ngx_http_upstream_finalize_request(r, u, status);

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}


static ngx_int_t
ngx_http_upstream_test_connect(ngx_connection_t *c)
{
    int        err;
    socklen_t  len;

#if (NGX_HAVE_KQUEUE)

    if (ngx_event_flags & NGX_USE_KQUEUE_EVENT) 
	{
        if (c->write->pending_eof || c->read->pending_eof) 
		{
            if (c->write->pending_eof) {
                err = c->write->kq_errno;

            } else {
                err = c->read->kq_errno;
            }

            c->log->action = "connecting to upstream";
            (void) ngx_connection_error(c, err,
                                    "kevent() reported that connect() failed");
            return NGX_ERROR;
        }

    }
	else
#endif
    {
        err = 0;
        len = sizeof(int);

        /*
         * BSDs and Linux return 0 and set a pending error in err
         * Solaris returns -1 and sets errno
         */

        if (getsockopt(c->fd, SOL_SOCKET, SO_ERROR, (void *) &err, &len) == -1)
        {
            err = ngx_socket_errno;
        }

        if (err) 
		{
            c->log->action = "connecting to upstream";
            (void) ngx_connection_error(c, err, "connect() failed");
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_headers(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_str_t                       uri, args;
    ngx_uint_t                      i, flags;
    ngx_list_part_t                *part;
    ngx_table_elt_t                *h;
    ngx_http_upstream_header_t     *hh;
    ngx_http_upstream_main_conf_t  *umcf;

    umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);

    if (u->headers_in.x_accel_redirect
        && !(u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_XA_REDIRECT))
    {
        ngx_http_upstream_finalize_request(r, u, NGX_DECLINED);

        part = &u->headers_in.headers.part;
        h = part->elts;

        for (i = 0; /* void */; i++) {

            if (i >= part->nelts) {
                if (part->next == NULL) {
                    break;
                }

                part = part->next;
                h = part->elts;
                i = 0;
            }

            hh = ngx_hash_find(&umcf->headers_in_hash, h[i].hash, h[i].lowcase_key, h[i].key.len);

            if (hh && hh->redirect) {
                if (hh->copy_handler(r, &h[i], hh->conf) != NGX_OK) {
                    ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
                    return NGX_DONE;
                }
            }
        }

        uri = u->headers_in.x_accel_redirect->value;

        if (uri.data[0] == '@') {
            ngx_http_named_location(r, &uri);

        } else {
            ngx_str_null(&args);
            flags = NGX_HTTP_LOG_UNSAFE;

            if (ngx_http_parse_unsafe_uri(r, &uri, &args, &flags) != NGX_OK) {
                ngx_http_finalize_request(r, NGX_HTTP_NOT_FOUND);
                return NGX_DONE;
            }

            if (r->method != NGX_HTTP_HEAD) {
                r->method = NGX_HTTP_GET;
            }

            ngx_http_internal_redirect(r, &uri, &args);
        }

        ngx_http_finalize_request(r, NGX_DONE);
        return NGX_DONE;
    }

    part = &u->headers_in.headers.part;
    h = part->elts;

    for (i = 0; /* void */; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            h = part->elts;
            i = 0;
        }

        if (ngx_hash_find(&u->conf->hide_headers_hash, h[i].hash, h[i].lowcase_key, h[i].key.len))
        {
            continue;
        }

        hh = ngx_hash_find(&umcf->headers_in_hash, h[i].hash,
                           h[i].lowcase_key, h[i].key.len);

        if (hh) {
            if (hh->copy_handler(r, &h[i], hh->conf) != NGX_OK) {
                ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
                return NGX_DONE;
            }

            continue;
        }

        if (ngx_http_upstream_copy_header_line(r, &h[i], 0) != NGX_OK) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return NGX_DONE;
        }
    }

    if (r->headers_out.server && r->headers_out.server->value.data == NULL) {
        r->headers_out.server->hash = 0;
    }

    if (r->headers_out.date && r->headers_out.date->value.data == NULL) {
        r->headers_out.date->hash = 0;
    }

    r->headers_out.status = u->headers_in.status_n;
    r->headers_out.status_line = u->headers_in.status_line;

    r->headers_out.content_length_n = u->headers_in.content_length_n;

    r->disable_not_modified = !u->cacheable;

    if (u->conf->force_ranges) {
        r->allow_ranges = 1;
        r->single_range = 1;

#if (NGX_HTTP_CACHE)
        if (r->cached) {
            r->single_range = 0;
        }
#endif
    }

    u->length = -1;

    return NGX_OK;
}

//接收响应 --接收响应包体
//接收并解析响应包体由 ngx_http_upstream_process_body_in_memory 方法实现；
static void
ngx_http_upstream_process_body_in_memory(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    size_t             size;
    ssize_t            n;
    ngx_buf_t         *b;
    ngx_event_t       *rev;
    ngx_connection_t  *c;

    c = u->peer.connection;
    rev = c->read;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream process body on memory");

	/*
     * 检查读事件标志位timedout是否超时，若该标志位为1，表示响应已经超时；
     * 则调用ngx_http_upstream_finalize_request方法结束请求；
     * 并return从当前函数返回；
     */
    if (rev->timedout) 
	{
        ngx_connection_error(c, NGX_ETIMEDOUT, "upstream timed out");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_GATEWAY_TIME_OUT);
        return;
    }

    b = &u->buffer;

    for ( ;; ) 
	{
		/* 检查当前接收缓冲区是否剩余的内存空间 */
        size = b->end - b->last;

		/*
         * 若接收缓冲区不存在空闲的内存空间，
         * 则调用ngx_http_upstream_finalize_request方法结束请求；
         * 并return从当前函数返回；
         */
        if (size == 0) 
		{
            ngx_log_error(NGX_LOG_ALERT, c->log, 0, "upstream buffer is too small to read response");
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }

		       /*
         * 若接收缓冲区有可用的内存空间，
         * 则调用recv方法开始接收响应包体；
         */
        n = c->recv(c, b->last, size);

		/*
         * 若返回值 n = NGX_AGAIN，表示等待下一次触发读事件再接收响应包体；
         */
        if (n == NGX_AGAIN)
		{
            break;
        }

		/*
         * 若返回值n = 0(表示上游服务器主动关闭连接)，或n = NGX_ERROR(表示出错)；
         * 则调用ngx_http_upstream_finalize_request方法结束请求；
         * 并return从当前函数返回；
         */
        if (n == 0 || n == NGX_ERROR) 
		{
            ngx_http_upstream_finalize_request(r, u, n);
            return;
        }
		/* 若返回值 n 大于0，表示成功读取到响应包体 */
		
        u->state->response_length += n;

		/* 调用input_filter方法处理本次接收到的响应包体 */
        if (u->input_filter(u->input_filter_ctx, n) == NGX_ERROR)
		{
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }

		/* 检查读事件的ready标志位，若为1，继续读取响应包体 */
        if (!rev->ready) 
		{
            break;
        }
    }

    if (u->length == 0) 
	{
        ngx_http_upstream_finalize_request(r, u, 0);
        return;
    }

	/*
     * 若读事件的ready标志位为0，表示读事件未准备就绪，
     * 则将读事件注册到epoll事件机制中，添加到定时器机制中；
     * 读事件的回调方法不改变，即依旧为ngx_http_upstream_process_body_in_memory；
     */
    if (ngx_handle_read_event(rev, 0) != NGX_OK) 
	{
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (rev->active)
	{
        ngx_add_timer(rev, u->conf->read_timeout);
    } 
	else if (rev->timer_set) 
	{
        ngx_del_timer(rev);
    }
}

//转发响应
static void
ngx_http_upstream_send_response(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    int                        tcp_nodelay;
    ssize_t                    n;
    ngx_int_t                  rc;
    ngx_event_pipe_t          *p;
    ngx_connection_t          *c;
    ngx_http_core_loc_conf_t  *clcf;

	 /* 调用ngx_http_send_hander方法向下游发送响应头部 */
    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->post_action) {
        ngx_http_upstream_finalize_request(r, u, rc);
        return;
    }

	/* 将标志位header_sent设置为1，表示已经转发响应头部 */
    u->header_sent = 1;

    if (u->upgrade) {
        ngx_http_upstream_upgrade(r, u);
        return;
    }
	
	/* 获取Nginx与下游之间的TCP连接 */
    c = r->connection;

    if (r->header_only) {

        if (!u->buffering) {
            ngx_http_upstream_finalize_request(r, u, rc);
            return;
        }

        if (!u->cacheable && !u->store) {
            ngx_http_upstream_finalize_request(r, u, rc);
            return;
        }

        u->pipe->downstream_error = 1;
    }

	//如果客户端的请求中有HTTP包体，而且曾经调用过ngx_http_read_client_request_body方法接收HTTP包体
	//并把包体存放在里临时文件中，这时就会调用ngx_pool_run_cleanup_file方法清理临时文件。因为上游服务器
	//发送响应时可能会使用到临时文件，之后收到响应解析响应包头时也不可以清理临时文件，而一旦开始向下游
	//客户端转发HTTP响应时，则意味着肯定不会在需要客户端请求的包体了，这时可以关闭、转移、或者删除临时
	//文件，具体动作由HTTP模块实现的handler回调方法决定
	 /* 若临时文件保存着请求包体，则调用ngx_pool_run_cleanup_file方法清理临时文件的请求包体 */
    if (r->request_body && r->request_body->temp_file) {
        ngx_pool_run_cleanup_file(r->pool, r->request_body->temp_file->file.fd);
        r->request_body->temp_file->file.fd = NGX_INVALID_FILE;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
	
	 /*
     * 若标志位buffering为0，转发响应时以下游服务器网速优先；
     * 即只需分配固定的内存块大小来接收来自上游服务器的响应并转发，
     * 当该内存块已满，则暂停接收来自上游服务器的响应数据，
     * 等待把内存块的响应数据转发给下游服务器后有剩余内存空间再继续接收响应；
     */
    if (!u->buffering) 
	{
		//检查HTTP模块是否是实现了用于处理包体的input_filter方法，
		//如果没有实现，则使用upstream定义的默认方法代替之
        if (u->input_filter == NULL) 
		{
            u->input_filter_init = ngx_http_upstream_non_buffered_filter_init;
            u->input_filter = ngx_http_upstream_non_buffered_filter;
            u->input_filter_ctx = r;
        }

		/*
         * 设置ngx_http_upstream_t结构体中读事件的回调方法为ngx_http_upstream_non_buffered_upstream，(即读取上游响应的方法)；
         * 设置当前请求ngx_http_request_t结构体中写事件的回调方法为ngx_http_upstream_process_non_buffered_downstream，(即转发响应到下游的方法)；
         */
        u->read_event_handler = ngx_http_upstream_process_non_buffered_upstream;
        r->write_event_handler = ngx_http_upstream_process_non_buffered_downstream;

        r->limit_rate = 0;

		/* 调用input_filter_init为input_filter方法处理响应包体做初始化工作 */
        if (u->input_filter_init(u->input_filter_ctx) == NGX_ERROR) 
		{
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }

        if (clcf->tcp_nodelay && c->tcp_nodelay == NGX_TCP_NODELAY_UNSET) 
		{
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "tcp_nodelay");

            tcp_nodelay = 1;

            if (setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY, (const void *) &tcp_nodelay, sizeof(int)) == -1)
            {
                ngx_connection_error(c, ngx_socket_errno, "setsockopt(TCP_NODELAY) failed");
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

            c->tcp_nodelay = NGX_TCP_NODELAY_SET;
        }

		//如果解析完包头后缓冲区中还有多余的字符，则表示还接收到了包体，这时将调用input_filter方法第一次处理接收到的包体
		/* 检查解析完响应头部后接收缓冲区buffer是否已接收了响应包体 */
        n = u->buffer.last - u->buffer.pos;
		
        if (n) {
            u->buffer.last = u->buffer.pos;

            u->state->response_length += n;
			/* 调用input_filter方法开始处理响应包体 */
            if (u->input_filter(u->input_filter_ctx, n) == NGX_ERROR) 
			{
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }
			//调用该方法把本次接收到的响应包体转发给下游客户端
            ngx_http_upstream_process_non_buffered_downstream(r);

        } else {	
			/* 若接收缓冲区中没有响应包体，则将其清空，即复用这个缓冲区 */
            u->buffer.pos = u->buffer.start;
            u->buffer.last = u->buffer.start;

			//NGX_HTTP_FLUSH标志位意味着如果请求r的out缓冲区中依然有等待发送的响应，则"催促"着发送出它们
            if (ngx_http_send_special(r, NGX_HTTP_FLUSH) == NGX_ERROR) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

			//如果与上游服务器的连接上读事件已准备就绪，则调用ngx_http_upstream_process_non_buffered_upstream方法接收响应包体并处理
            if (u->peer.connection->read->ready || u->length == 0)
			{
                ngx_http_upstream_process_non_buffered_upstream(r, u);
            }
        }

        return;
    }

	
    /*
     * 若ngx_http_upstream_t结构体的buffering标志位为1，则转发响应包体时以上游网速优先；
     * 即分配更多的内存和缓存，即一直接收来自上游服务器的响应，把来自上游服务器的响应保存的内存或缓存中；
     */
    /* TODO: preallocate event_pipe bufs, look "Content-Length" */

#if (NGX_HTTP_CACHE)

    if (r->cache && r->cache->file.fd != NGX_INVALID_FILE) {
        ngx_pool_run_cleanup_file(r->pool, r->cache->file.fd);
        r->cache->file.fd = NGX_INVALID_FILE;
    }

    switch (ngx_http_test_predicates(r, u->conf->no_cache)) {

	    case NGX_ERROR:
	        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
	        return;

	    case NGX_DECLINED:
	        u->cacheable = 0;
	        break;

	    default: /* NGX_OK */

	        if (u->cache_status == NGX_HTTP_CACHE_BYPASS) {

	            /* create cache if previously bypassed */

	            if (ngx_http_file_cache_create(r) != NGX_OK) {
	                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
	                return;
	            }
	        }

	        break;
    }

    if (u->cacheable) {
        time_t  now, valid;

        now = ngx_time();

        valid = r->cache->valid_sec;

        if (valid == 0) {
            valid = ngx_http_file_cache_valid(u->conf->cache_valid, u->headers_in.status_n);
            if (valid) {
                r->cache->valid_sec = now + valid;
            }
        }

        if (valid) {
            r->cache->date = now;
            r->cache->body_start = (u_short) (u->buffer.pos - u->buffer.start);

            if (u->headers_in.status_n == NGX_HTTP_OK || u->headers_in.status_n == NGX_HTTP_PARTIAL_CONTENT) {
                r->cache->last_modified = u->headers_in.last_modified_time;

                if (u->headers_in.etag) {
                    r->cache->etag = u->headers_in.etag->value;

                } else {
                    ngx_str_null(&r->cache->etag);
                }

            } else {
                r->cache->last_modified = -1;
                ngx_str_null(&r->cache->etag);
            }

            if (ngx_http_file_cache_set_header(r, u->buffer.start) != NGX_OK) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

        } else {
            u->cacheable = 0;
        }
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "http cacheable: %d", u->cacheable);

    if (u->cacheable == 0 && r->cache) {
        ngx_http_file_cache_free(r->cache, u->pipe->temp_file);
    }

#endif
	/* 初始化 ngx_http_upstream_t 结构体中的 ngx_event_pipe_t pipe 成员*/
    p = u->pipe;

    p->output_filter = (ngx_event_pipe_output_filter_pt) ngx_http_output_filter;
    p->output_ctx = r;
    p->tag = u->output.tag;
    p->bufs = u->conf->bufs;
    p->busy_size = u->conf->busy_buffers_size;
    p->upstream = u->peer.connection;
    p->downstream = c;
    p->pool = r->pool;
    p->log = c->log;
    p->limit_rate = u->conf->limit_rate;
    p->start_sec = ngx_time();

    p->cacheable = u->cacheable || u->store;

    p->temp_file = ngx_pcalloc(r->pool, sizeof(ngx_temp_file_t));
    if (p->temp_file == NULL) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    p->temp_file->file.fd = NGX_INVALID_FILE;
    p->temp_file->file.log = c->log;
    p->temp_file->path = u->conf->temp_path;
    p->temp_file->pool = r->pool;

    if (p->cacheable) {
        p->temp_file->persistent = 1;

#if (NGX_HTTP_CACHE)
        if (r->cache && r->cache->file_cache->temp_path) {
            p->temp_file->path = r->cache->file_cache->temp_path;
        }
#endif

    } else {
        p->temp_file->log_level = NGX_LOG_WARN;
        p->temp_file->warn = "an upstream response is buffered to a temporary file";
    }

    p->max_temp_file_size = u->conf->max_temp_file_size;
    p->temp_file_write_size = u->conf->temp_file_write_size;
	
	/* 初始化预读链表缓冲区preread_bufs */
    p->preread_bufs = ngx_alloc_chain_link(r->pool);
    if (p->preread_bufs == NULL) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    p->preread_bufs->buf = &u->buffer;
    p->preread_bufs->next = NULL;
    u->buffer.recycled = 1;

    p->preread_size = u->buffer.last - u->buffer.pos;

    if (u->cacheable) {

        p->buf_to_file = ngx_calloc_buf(r->pool);
        if (p->buf_to_file == NULL) {
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }

        p->buf_to_file->start = u->buffer.start;
        p->buf_to_file->pos = u->buffer.start;
        p->buf_to_file->last = u->buffer.pos;
        p->buf_to_file->temporary = 1;
    }

    if (ngx_event_flags & NGX_USE_IOCP_EVENT) {
        /* the posted aio operation may corrupt a shadow buffer */
        p->single_buf = 1;
    }

    /* TODO: p->free_bufs = 0 if use ngx_create_chain_of_bufs() */
    p->free_bufs = 1;

    /*
     * event_pipe would do u->buffer.last += p->preread_size as though these bytes were read
     */
    u->buffer.last = u->buffer.pos;

    if (u->conf->cyclic_temp_file) {

        /*
         * we need to disable the use of sendfile() if we use cyclic temp file
         * because the writing a new data may interfere with sendfile()
         * that uses the same kernel file pages (at least on FreeBSD)
         */

        p->cyclic_temp_file = 1;
        c->sendfile = 0;

    } else {
        p->cyclic_temp_file = 0;
    }

    p->read_timeout = u->conf->read_timeout;
    p->send_timeout = clcf->send_timeout;
    p->send_lowat = clcf->send_lowat;

    p->length = -1;

	 /* 调用input_filter_init为input_filter方法处理响应包体做初始化工作 */
    if (u->input_filter_init && u->input_filter_init(p->input_ctx) != NGX_OK) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

	/* 设置上游读事件的方法 */
    u->read_event_handler = ngx_http_upstream_process_upstream;
	/* 设置下游写事件的方法 */
    r->write_event_handler = ngx_http_upstream_process_downstream;

	/* 处理上游响应包体 */
    ngx_http_upstream_process_upstream(r, u);
}


static void
ngx_http_upstream_upgrade(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    int                        tcp_nodelay;
    ngx_connection_t          *c;
    ngx_http_core_loc_conf_t  *clcf;

    c = r->connection;
    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    /* TODO: prevent upgrade if not requested or not possible */

    r->keepalive = 0;
    c->log->action = "proxying upgraded connection";

    u->read_event_handler = ngx_http_upstream_upgraded_read_upstream;
    u->write_event_handler = ngx_http_upstream_upgraded_write_upstream;
    r->read_event_handler = ngx_http_upstream_upgraded_read_downstream;
    r->write_event_handler = ngx_http_upstream_upgraded_write_downstream;

    if (clcf->tcp_nodelay) {
        tcp_nodelay = 1;

        if (c->tcp_nodelay == NGX_TCP_NODELAY_UNSET) {
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "tcp_nodelay");

            if (setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY,
                           (const void *) &tcp_nodelay, sizeof(int)) == -1)
            {
                ngx_connection_error(c, ngx_socket_errno,
                                     "setsockopt(TCP_NODELAY) failed");
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

            c->tcp_nodelay = NGX_TCP_NODELAY_SET;
        }

        if (u->peer.connection->tcp_nodelay == NGX_TCP_NODELAY_UNSET) {
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, u->peer.connection->log, 0,
                           "tcp_nodelay");

            if (setsockopt(u->peer.connection->fd, IPPROTO_TCP, TCP_NODELAY,
                           (const void *) &tcp_nodelay, sizeof(int)) == -1)
            {
                ngx_connection_error(u->peer.connection, ngx_socket_errno,
                                     "setsockopt(TCP_NODELAY) failed");
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

            u->peer.connection->tcp_nodelay = NGX_TCP_NODELAY_SET;
        }
    }

    if (ngx_http_send_special(r, NGX_HTTP_FLUSH) == NGX_ERROR) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (u->peer.connection->read->ready
        || u->buffer.pos != u->buffer.last)
    {
        ngx_post_event(c->read, &ngx_posted_events);
        ngx_http_upstream_process_upgraded(r, 1, 1);
        return;
    }

    ngx_http_upstream_process_upgraded(r, 0, 1);
}


static void
ngx_http_upstream_upgraded_read_downstream(ngx_http_request_t *r)
{
    ngx_http_upstream_process_upgraded(r, 0, 0);
}


static void
ngx_http_upstream_upgraded_write_downstream(ngx_http_request_t *r)
{
    ngx_http_upstream_process_upgraded(r, 1, 1);
}


static void
ngx_http_upstream_upgraded_read_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u)
{
    ngx_http_upstream_process_upgraded(r, 1, 0);
}


static void
ngx_http_upstream_upgraded_write_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u)
{
    ngx_http_upstream_process_upgraded(r, 0, 1);
}


static void
ngx_http_upstream_process_upgraded(ngx_http_request_t *r,
    ngx_uint_t from_upstream, ngx_uint_t do_write)
{
    size_t                     size;
    ssize_t                    n;
    ngx_buf_t                 *b;
    ngx_connection_t          *c, *downstream, *upstream, *dst, *src;
    ngx_http_upstream_t       *u;
    ngx_http_core_loc_conf_t  *clcf;

    c = r->connection;
    u = r->upstream;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http upstream process upgraded, fu:%ui", from_upstream);

    downstream = c;
    upstream = u->peer.connection;

    if (downstream->write->timedout) {
        c->timedout = 1;
        ngx_connection_error(c, NGX_ETIMEDOUT, "client timed out");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_REQUEST_TIME_OUT);
        return;
    }

    if (upstream->read->timedout || upstream->write->timedout) {
        ngx_connection_error(c, NGX_ETIMEDOUT, "upstream timed out");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_GATEWAY_TIME_OUT);
        return;
    }

    if (from_upstream) {
        src = upstream;
        dst = downstream;
        b = &u->buffer;

    } else {
        src = downstream;
        dst = upstream;
        b = &u->from_client;

        if (r->header_in->last > r->header_in->pos) {
            b = r->header_in;
            b->end = b->last;
            do_write = 1;
        }

        if (b->start == NULL) {
            b->start = ngx_palloc(r->pool, u->conf->buffer_size);
            if (b->start == NULL) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

            b->pos = b->start;
            b->last = b->start;
            b->end = b->start + u->conf->buffer_size;
            b->temporary = 1;
            b->tag = u->output.tag;
        }
    }

    for ( ;; ) {

        if (do_write) {

            size = b->last - b->pos;

            if (size && dst->write->ready) {

                n = dst->send(dst, b->pos, size);

                if (n == NGX_ERROR) {
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                    return;
                }

                if (n > 0) {
                    b->pos += n;

                    if (b->pos == b->last) {
                        b->pos = b->start;
                        b->last = b->start;
                    }
                }
            }
        }

        size = b->end - b->last;

        if (size && src->read->ready) {

            n = src->recv(src, b->last, size);

            if (n == NGX_AGAIN || n == 0) {
                break;
            }

            if (n > 0) {
                do_write = 1;
                b->last += n;

                continue;
            }

            if (n == NGX_ERROR) {
                src->read->eof = 1;
            }
        }

        break;
    }

    if ((upstream->read->eof && u->buffer.pos == u->buffer.last)
        || (downstream->read->eof && u->from_client.pos == u->from_client.last)
        || (downstream->read->eof && upstream->read->eof))
    {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                       "http upstream upgraded done");
        ngx_http_upstream_finalize_request(r, u, 0);
        return;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (ngx_handle_write_event(upstream->write, u->conf->send_lowat)
        != NGX_OK)
    {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (upstream->write->active && !upstream->write->ready) {
        ngx_add_timer(upstream->write, u->conf->send_timeout);

    } else if (upstream->write->timer_set) {
        ngx_del_timer(upstream->write);
    }

    if (ngx_handle_read_event(upstream->read, 0) != NGX_OK) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (upstream->read->active && !upstream->read->ready) {
        ngx_add_timer(upstream->read, u->conf->read_timeout);

    } else if (upstream->read->timer_set) {
        ngx_del_timer(upstream->read);
    }

    if (ngx_handle_write_event(downstream->write, clcf->send_lowat)
        != NGX_OK)
    {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (ngx_handle_read_event(downstream->read, 0) != NGX_OK) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (downstream->write->active && !downstream->write->ready) {
        ngx_add_timer(downstream->write, clcf->send_timeout);

    } else if (downstream->write->timer_set) {
        ngx_del_timer(downstream->write);
    }
}

//当以下游网速优先转发响应包体给下游时，由函数 ngx_http_upstream_process_non_buffered_downstrean 实现
///* buffering 标志位为0时，转发响应包体给下游客户端 */
static void
ngx_http_upstream_process_non_buffered_downstream(ngx_http_request_t *r)
{
    ngx_event_t          *wev;
    ngx_connection_t     *c;
    ngx_http_upstream_t  *u;

	//注意，这个c是Nginx与客户端之间的TCP连接
    c = r->connection;
    u = r->upstream;
    wev = c->write;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream process non buffered downstream");

    c->log->action = "sending to client";

	//检查下游连接上写事件是否超时，若超时则结束当前请求, 并从当前函数返回
    if (wev->timedout) 
	{
        c->timedout = 1;
        ngx_connection_error(c, NGX_ETIMEDOUT, "client timed out");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_REQUEST_TIME_OUT);
        return;
    }

	//以固定内存块方式转发响应包体给下游客户端，此时第二个参数为 1
    ngx_http_upstream_process_non_buffered_request(r, 1);
}

//由于 buffering 标志位为0时，没有开启文件缓存，只有固定大小的内存块作为接收响应缓冲区，当上游的响应包体比较大时，
//此时，接收缓冲区内存并不能够满足一次性接收完所有响应包体， 因此，在接收缓冲区已满时，会阻塞接收响应包体，
//并先把已经收到的响应包体转发给下游客户端。所有在转发响应包体时，有可能会接收上游响应包体。
//此过程由 ngx_http_upstream_process_non_buffered_upstream 方法实现；
static void
ngx_http_upstream_process_non_buffered_upstream(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_connection_t  *c;

	//获取Nginx与上游服务器间的TCP连接
    c = u->peer.connection;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream process non buffered upstream");

    c->log->action = "reading upstream";

	//判断上游连接上读事件是否超时，若超时则结束当前请求, 并从当前函数返回
    if (c->read->timedout) {
        ngx_connection_error(c, NGX_ETIMEDOUT, "upstream timed out");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_GATEWAY_TIME_OUT);
        return;
    }
	  /*
     * 以固定内存块方式转发响应包体给下游服务器，
     * 注意：转发的过程中，会接收来自上游服务器的响应包体，此时第二个参数为 0
     */
    ngx_http_upstream_process_non_buffered_request(r, 0);
}

/* 以固定内存块方式转发响应包体给下游客户端 */
/*
 * 若do_write为0时，首先收来自上游服务器的响应，再转发响应给下游；
 * 若do_write为1，首先转发响应给下游客户端，再接收来自上游服务器的响应
 */

static void
ngx_http_upstream_process_non_buffered_request(ngx_http_request_t *r, ngx_uint_t do_write)
{
    size_t                     size;
    ssize_t                    n;
    ngx_buf_t                 *b;
    ngx_int_t                  rc;
    ngx_connection_t          *downstream, *upstream;
    ngx_http_upstream_t       *u;
    ngx_http_core_loc_conf_t  *clcf;

    u = r->upstream;
	/* 获取Nginx与下游客户端之间的TCP连接 */ 
    downstream = r->connection;
	/* 获取Nginx与上游服务器之间的TCP连接 */
    upstream = u->peer.connection;

    b = &u->buffer;

	//当u->length为0时，说明不再需要接收上游的响应，那只能继续向下游发送响应
    do_write = do_write || u->length == 0;

    for ( ;; ) 
	{
		/* 若do_write为1，则开始向下游客户端转发响应包体 */  
        if (do_write) 
		{
			 /*
             * 检查是否有响应包体需要转发给下游服务器；
             * 其中out_bufs表示本次需要转发给下游服务器的响应包体；
             * busy_bufs表示上一次向下游服务器转发响应包体时没有转发完的响应包体内存；
             * 即若一次性转发不完所有的响应包体，则会保存在busy_bufs链表缓冲区中，
             * 这里的保存只是将busy_bufs指向未发送完毕的响应数据；
             */
		
            if (u->out_bufs || u->busy_bufs)
			{
				/* 调用ngx_http_output_filter方法将响应包体发送给下游服务器 */
                rc = ngx_http_output_filter(r, u->out_bufs);

				/* 若返回值 rc = NGX_ERROR，则结束请求 */ 
                if (rc == NGX_ERROR) 
				{
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                    return;
                }
				/* 
                 * 调用ngx_chain_update_chains方法更新free_bufs、busy_bufs、out_bufs链表； 
                 * 即清空out_bufs链表，把out_bufs链表中已发送完的ngx_buf_t缓冲区清空，并将其添加到free_bufs链表中； 
                 * 把out_bufs链表中未发送完的ngx_buf_t缓冲区添加到busy_bufs链表中； 
                 */
                ngx_chain_update_chains(r->pool, &u->free_bufs, &u->busy_bufs, &u->out_bufs, u->output.tag);
            }

			 /* 
             * busy_bufs为空，表示所有响应包体已经转发到下游服务器， 
             * 此时清空接收缓冲区buffer以便再次接收来自上游服务器的响应包体； 
             */  
            if (u->busy_bufs == NULL) 
			{

                if (u->length == 0 || (upstream->read->eof && u->length == -1))
                {
                    ngx_http_upstream_finalize_request(r, u, 0);
                    return;
                }

                if (upstream->read->eof) 
				{
                    ngx_log_error(NGX_LOG_ERR, upstream->log, 0, "upstream prematurely closed connection");
                    ngx_http_upstream_finalize_request(r, u, NGX_HTTP_BAD_GATEWAY);
                    return;
                }

                if (upstream->read->error)
				{
                    ngx_http_upstream_finalize_request(r, u, NGX_HTTP_BAD_GATEWAY);
                    return;
                }

                b->pos = b->start;
                b->last = b->start;
            }
        }

		/* 计算接收缓冲区buffer剩余可用的内存空间 */
        size = b->end - b->last;
		/* 
         * 若接收缓冲区buffer有剩余的可用空间， 
         * 且此时读事件可读，即可读取来自上游服务器的响应包体； 
         * 则调用recv方法开始接收来自上游服务器的响应包体，并保存在接收缓冲区buffer中； 
         */ 
        if (size && upstream->read->ready) 
		{

            n = upstream->recv(upstream, b->last, size);
			/* 若返回值 n = NGX_AGAIN，则等待下一次可读事件发生继续接收响应 */  
            if (n == NGX_AGAIN) 
			{
                break;
            }

			/* 
             * 若返回值 n 大于0，表示接收到响应包体， 
             * 则调用input_filter方法处理响应包体； 
             * 并把do_write设置为1，表示已经接收到响应包体，此时可转发给下游服务器
             */  
            if (n > 0) 
			{
                u->state->response_length += n;

                if (u->input_filter(u->input_filter_ctx, n) == NGX_ERROR) 
				{
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                    return;
                }
            }

            do_write = 1;

            continue;
        }

        break;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

	/* 调用ngx_handle_write_event方法将Nginx与下游之间的连接上的写事件注册的epoll事件机制中 */
    if (downstream->data == r) 
	{
        if (ngx_handle_write_event(downstream->write, clcf->send_lowat) != NGX_OK)
        {
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }
    }
	
	/* 调用ngx_add_timer方法将Nginx与下游之间的连接上的写事件添加到定时器事件机制中 */
    if (downstream->write->active && !downstream->write->ready)
	{
        ngx_add_timer(downstream->write, clcf->send_timeout);
    }
	else if (downstream->write->timer_set) 
   	{
        ngx_del_timer(downstream->write);
    }

	/* 调用ngx_handle_read_event方法将Nginx与上游之间的连接上的读事件注册的epoll事件机制中 */
    if (ngx_handle_read_event(upstream->read, 0) != NGX_OK) 
	{
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

	 /* 调用ngx_add_timer方法将Nginx与上游之间的连接上的读事件添加到定时器事件机制中 */
    if (upstream->read->active && !upstream->read->ready)
	{
        ngx_add_timer(upstream->read, u->conf->read_timeout);
    } 
	else if (upstream->read->timer_set)
	{
        ngx_del_timer(upstream->read);
    }
}


static ngx_int_t
ngx_http_upstream_non_buffered_filter_init(void *data)
{
    return NGX_OK;
}

/* upstream机制默认的input_filter方法 */
static ngx_int_t
ngx_http_upstream_non_buffered_filter(void *data, ssize_t bytes)
{
	 /* 
     * data参数是ngx_http_upstream_t结构体中的input_filter_ctx， 
     * 当HTTP模块没有实现input_filter方法时， 
     * input_filter_ctx指向ngx_http_request_t结构体； 
     */  
    ngx_http_request_t  *r = data;

    ngx_buf_t            *b;
    ngx_chain_t          *cl, **ll;
    ngx_http_upstream_t  *u;

    u = r->upstream;

	/* 找到out_bufs链表的最后一个缓冲区，并由ll指向该缓冲区 */ 
    for (cl = u->out_bufs, ll = &u->out_bufs; cl; cl = cl->next)
	{
        ll = &cl->next;
    }

	/* 从free_bufs空闲链表缓冲区中获取一个ngx_buf_t结构体给cl */  
    cl = ngx_chain_get_free_buf(r->pool, &u->free_bufs);
    if (cl == NULL) 
	{
        return NGX_ERROR;
    }

	/* 将新分配的ngx_buf_t缓冲区添加到out_bufs链表的尾端 */ 
    *ll = cl;

    cl->buf->flush = 1;
    cl->buf->memory = 1;

	 /* buffer是保存来自上游服务器的响应包体 */
    b = &u->buffer;

	/* 将响应包体数据保存在cl缓冲区中 */ 
    cl->buf->pos = b->last;
    b->last += bytes;
    cl->buf->last = b->last;
    cl->buf->tag = u->output.tag;

    if (u->length == -1) 
	{
        return NGX_OK;
    }

	/* 更新length长度，表示需要接收的包体长度减少bytes字节 */ 
    u->length -= bytes;

    return NGX_OK;
}


static void
ngx_http_upstream_process_downstream(ngx_http_request_t *r)
{
    ngx_event_t          *wev;
    ngx_connection_t     *c;
    ngx_event_pipe_t     *p;
    ngx_http_upstream_t  *u;

	/* 获取 Nginx 与下游客户端之间的连接 */ 
    c = r->connection;
    u = r->upstream;
    p = u->pipe;
    wev = c->write;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream process downstream");

    c->log->action = "sending to client";

	/* 检查下游连接上写事件是否超时*/
    if (wev->timedout) 
	{
		//检查是否需要延迟处理这个事件
        if (wev->delayed)
		{
            wev->timedout = 0;
            wev->delayed = 0;

			//若写事件未准备就绪，则将写事件加入到定时器机制中和epoll事件机制中，并从当前函数返回
            if (!wev->ready) 
			{
                ngx_add_timer(wev, p->send_timeout);

                if (ngx_handle_write_event(wev, p->send_lowat) != NGX_OK)
				{
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                }

                return;
            }

			//写事件已经准备就绪，将响应包体转发给下游客户端
            if (ngx_event_pipe(p, wev->write) == NGX_ABORT) 
			{
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

        } 
		else 
		{
			//未设置延迟处理而写事件超时，则表示连接超时错误
            p->downstream_error = 1;
            c->timedout = 1;
            ngx_connection_error(c, NGX_ETIMEDOUT, "client timed out");
        }

    } 
	else 
	{
		//若需要延迟处理这个写事件，则将写事件加入到epoll机制中，并从当前函数返回
        if (wev->delayed) 
		{
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http downstream delayed");

            if (ngx_handle_write_event(wev, p->send_lowat) != NGX_OK)
			{
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            }

            return;
        }

		//将响应包体转发给下游客户端
        if (ngx_event_pipe(p, 1) == NGX_ABORT)
		{
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }
    }

    ngx_http_upstream_process_request(r, u);
}


static void
ngx_http_upstream_process_upstream(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_event_t       *rev;
    ngx_event_pipe_t  *p;
    ngx_connection_t  *c;

    c = u->peer.connection;
    p = u->pipe;
    rev = c->read;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream process upstream");

    c->log->action = "reading upstream";

	/* 检查上游连接上读事件是否超时*/
    if (rev->timedout) {
        if (rev->delayed) {

            rev->timedout = 0;
            rev->delayed = 0;

			//若读事件未准备就绪，则将读事件加入到定时器机制中和epoll事件机制中，并从当前函数返回
            if (!rev->ready) 
			{
                ngx_add_timer(rev, p->read_timeout);

                if (ngx_handle_read_event(rev, 0) != NGX_OK) 
				{
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                }

                return;
            }

			//读事件已经准备就绪，将读取响应包体
            if (ngx_event_pipe(p, 0) == NGX_ABORT)
			{
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

        } 
		else
		{
            p->upstream_error = 1;
            ngx_connection_error(c, NGX_ETIMEDOUT, "upstream timed out");
        }

    } 
	else 
	{

        if (rev->delayed)
		{

            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream delayed");

            if (ngx_handle_read_event(rev, 0) != NGX_OK)
			{
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            }

            return;
        }

        if (ngx_event_pipe(p, 0) == NGX_ABORT)
		{
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }
    }

    ngx_http_upstream_process_request(r, u);
}


static void
ngx_http_upstream_process_request(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_temp_file_t   *tf;
    ngx_event_pipe_t  *p;

    p = u->pipe;

    if (u->peer.connection) {

        if (u->store) {

            if (p->upstream_eof || p->upstream_done) {

                tf = p->temp_file;

                if (u->headers_in.status_n == NGX_HTTP_OK
                    && (p->upstream_done || p->length == -1)
                    && (u->headers_in.content_length_n == -1
                        || u->headers_in.content_length_n == tf->offset))
                {
                    ngx_http_upstream_store(r, u);
                }
            }
        }

#if (NGX_HTTP_CACHE)

        if (u->cacheable) {

            if (p->upstream_done) {
                ngx_http_file_cache_update(r, p->temp_file);

            } else if (p->upstream_eof) {

                tf = p->temp_file;

                if (p->length == -1 && (u->headers_in.content_length_n == -1 || u->headers_in.content_length_n == tf->offset - (off_t) r->cache->body_start)) {
                    ngx_http_file_cache_update(r, tf);
                } else {
                    ngx_http_file_cache_free(r->cache, tf);
                }

            } else if (p->upstream_error) {
                ngx_http_file_cache_free(r->cache, p->temp_file);
            }
        }

#endif

        if (p->upstream_done || p->upstream_eof || p->upstream_error) {
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream exit: %p", p->out);

            if (p->upstream_done || (p->upstream_eof && p->length == -1)) {
                ngx_http_upstream_finalize_request(r, u, 0);
                return;
            }

            if (p->upstream_eof) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "upstream prematurely closed connection");
            }

            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_BAD_GATEWAY);
            return;
        }
    }

    if (p->downstream_error) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream downstream error");

        if (!u->cacheable && !u->store && u->peer.connection) 
		{
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        }
    }
}


static void
ngx_http_upstream_store(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    size_t                  root;
    time_t                  lm;
    ngx_str_t               path;
    ngx_temp_file_t        *tf;
    ngx_ext_rename_file_t   ext;

    tf = u->pipe->temp_file;

    if (tf->file.fd == NGX_INVALID_FILE) {

        /* create file for empty 200 response */

        tf = ngx_pcalloc(r->pool, sizeof(ngx_temp_file_t));
        if (tf == NULL) {
            return;
        }

        tf->file.fd = NGX_INVALID_FILE;
        tf->file.log = r->connection->log;
        tf->path = u->conf->temp_path;
        tf->pool = r->pool;
        tf->persistent = 1;

        if (ngx_create_temp_file(&tf->file, tf->path, tf->pool,
                                 tf->persistent, tf->clean, tf->access)
            != NGX_OK)
        {
            return;
        }

        u->pipe->temp_file = tf;
    }

    ext.access = u->conf->store_access;
    ext.path_access = u->conf->store_access;
    ext.time = -1;
    ext.create_path = 1;
    ext.delete_file = 1;
    ext.log = r->connection->log;

    if (u->headers_in.last_modified) {

        lm = ngx_parse_http_time(u->headers_in.last_modified->value.data,
                                 u->headers_in.last_modified->value.len);

        if (lm != NGX_ERROR) {
            ext.time = lm;
            ext.fd = tf->file.fd;
        }
    }

    if (u->conf->store_lengths == NULL) {

        if (ngx_http_map_uri_to_path(r, &path, &root, 0) == NULL) {
            return;
        }

    } else {
        if (ngx_http_script_run(r, &path, u->conf->store_lengths->elts, 0,
                                u->conf->store_values->elts)
            == NULL)
        {
            return;
        }
    }

    path.len--;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "upstream stores \"%s\" to \"%s\"",
                   tf->file.name.data, path.data);

    (void) ngx_ext_rename_file(&tf->file.name, &path, &ext);

    u->store = 0;
}


static void
ngx_http_upstream_dummy_handler(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream dummy handler");
}


static void
ngx_http_upstream_next(ngx_http_request_t *r, ngx_http_upstream_t *u, ngx_uint_t ft_type)
{
    ngx_msec_t  timeout;
    ngx_uint_t  status, state;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http next upstream, %xi", ft_type);

    if (u->peer.sockaddr)
	{
		//只要错误类型不是 NGX_HTTP_UPSTREAM_FT_HTTP_403或者NGX_HTTP_UPSTREAM_FT_HTTP_404，都认为上游服务器有问题
        if (ft_type == NGX_HTTP_UPSTREAM_FT_HTTP_403 || ft_type == NGX_HTTP_UPSTREAM_FT_HTTP_404)
        {
            state = NGX_PEER_NEXT;

        }
		else 
		{
            state = NGX_PEER_FAILED;
        }

        u->peer.free(&u->peer, u->peer.data, state);
        u->peer.sockaddr = NULL;
    }

    if (ft_type == NGX_HTTP_UPSTREAM_FT_TIMEOUT) 
	{
        ngx_log_error(NGX_LOG_ERR, r->connection->log, NGX_ETIMEDOUT, "upstream timed out");
    }

    if (u->peer.cached && ft_type == NGX_HTTP_UPSTREAM_FT_ERROR && (!u->request_sent || !r->request_body_no_buffering))
    {
        status = 0;

        /* TODO: inform balancer instead */

        u->peer.tries++;

    } 
	else 
	{
        switch (ft_type)
		{

        case NGX_HTTP_UPSTREAM_FT_TIMEOUT:
            status = NGX_HTTP_GATEWAY_TIME_OUT;
            break;

        case NGX_HTTP_UPSTREAM_FT_HTTP_500:
            status = NGX_HTTP_INTERNAL_SERVER_ERROR;
            break;

        case NGX_HTTP_UPSTREAM_FT_HTTP_403:
            status = NGX_HTTP_FORBIDDEN;
            break;

        case NGX_HTTP_UPSTREAM_FT_HTTP_404:
            status = NGX_HTTP_NOT_FOUND;
            break;

        /*
         * NGX_HTTP_UPSTREAM_FT_BUSY_LOCK and NGX_HTTP_UPSTREAM_FT_MAX_WAITING
         * never reach here
         */

        default:
            status = NGX_HTTP_BAD_GATEWAY;
        }
    }

    if (r->connection->error) 
	{
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_CLIENT_CLOSED_REQUEST);
        return;
    }

    if (status)
	{
        u->state->status = status;
        timeout = u->conf->next_upstream_timeout;

        if (u->peer.tries == 0 || !(u->conf->next_upstream & ft_type)
            || (u->request_sent && r->request_body_no_buffering)
            || (timeout && ngx_current_msec - u->peer.start_time >= timeout))
        {
#if (NGX_HTTP_CACHE)

            if (u->cache_status == NGX_HTTP_CACHE_EXPIRED && (u->conf->cache_use_stale & ft_type)) {
                ngx_int_t  rc;

                rc = u->reinit_request(r);

                if (rc == NGX_OK) 
				{
                    u->cache_status = NGX_HTTP_CACHE_STALE;
                    rc = ngx_http_upstream_cache_send(r, u);
                }

                ngx_http_upstream_finalize_request(r, u, rc);
                return;
            }
#endif

            ngx_http_upstream_finalize_request(r, u, status);
            return;
        }
    }

    if (u->peer.connection) 
	{
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "close http upstream connection: %d", u->peer.connection->fd);
#if (NGX_HTTP_SSL)

        if (u->peer.connection->ssl) 
		{
            u->peer.connection->ssl->no_wait_shutdown = 1;
            u->peer.connection->ssl->no_send_shutdown = 1;

            (void) ngx_ssl_shutdown(u->peer.connection);
        }
#endif

        if (u->peer.connection->pool) 
		{
            ngx_destroy_pool(u->peer.connection->pool);
        }

        ngx_close_connection(u->peer.connection);
        u->peer.connection = NULL;
    }

    ngx_http_upstream_connect(r, u);
}


static void
ngx_http_upstream_cleanup(void *data)
{
    ngx_http_request_t *r = data;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "cleanup http upstream request: \"%V\"", &r->uri);

    ngx_http_upstream_finalize_request(r, r->upstream, NGX_DONE);
}

//结束 upstream 请求由函数 ngx_http_upstream_finalize_request 实现，
//该函数最终会调用 HTTP 框架的 ngx_http_finalize_request 方法来结束请求
//NGX_DECLINED -- 关闭上游连接，不关闭下游连接
static void
ngx_http_upstream_finalize_request(ngx_http_request_t *r, ngx_http_upstream_t *u, ngx_int_t rc)
{
    ngx_uint_t  flush;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "finalize http upstream request: %i", rc);

	
    if (u->cleanup == NULL)
	{
        /* the request was already finalized */
        ngx_http_finalize_request(r, NGX_DONE);
        return;
    }

	/* 将 cleanup 指向的清理资源回调方法设置为 NULL */ 
    *u->cleanup = NULL;
    u->cleanup = NULL;

	/* 释放解析主机域名时分配的资源 */ 
    if (u->resolved && u->resolved->ctx) 
	{
        ngx_resolve_name_done(u->resolved->ctx);
        u->resolved->ctx = NULL;
    }

	/* 设置当前时间为 HTTP 响应结束的时间 */ 
    if (u->state && u->state->response_time) 
	{
        u->state->response_time = ngx_current_msec - u->state->response_time;

        if (u->pipe && u->pipe->read_length) 
		{
            u->state->response_length = u->pipe->read_length;
        }
    }

	/* 调用该方法执行一些操作 */
    u->finalize_request(r, rc);

	//如果有设置peer.free钩子，则调用释放peer
	//非keepalive的情况下该钩子是ngx_http_upstream_free_round_robin_peer
	//keepalive的情况下该钩子是ngx_http_upstream_free_keepalive_peer，该钩子会将连接
	//缓存到长连接cache池，并将u->peer.connection设置成空，防止下面代码关闭连接。
    if (u->peer.free && u->peer.sockaddr) 
	{
        u->peer.free(&u->peer, u->peer.data, 0);
        u->peer.sockaddr = NULL;
    }

	/* 若上游连接还未关闭(或未被缓存起来)，则调用 ngx_close_connection 方法关闭该连接 */ 
    if (u->peer.connection) 
	{

#if (NGX_HTTP_SSL)

        /* TODO: do not shutdown persistent connection */

        if (u->peer.connection->ssl) 
		{

            /*
             * We send the "close notify" shutdown alert to the upstream only
             * and do not wait its "close notify" shutdown alert.
             * It is acceptable according to the TLS standard.
             */

            u->peer.connection->ssl->no_wait_shutdown = 1;

            (void) ngx_ssl_shutdown(u->peer.connection);
        }
#endif

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "close http upstream connection: %d", u->peer.connection->fd);

        if (u->peer.connection->pool)
		{
            ngx_destroy_pool(u->peer.connection->pool);
        }

        ngx_close_connection(u->peer.connection);
    }

	//关闭连接后置空
    u->peer.connection = NULL;

    if (u->pipe && u->pipe->temp_file)
	{
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream temp fd: %d", u->pipe->temp_file->file.fd);
    }

	/* 若使用了文件缓存，则调用 ngx_delete_file 方法删除用于缓存响应的临时文件 */
    if (u->store && u->pipe && u->pipe->temp_file && u->pipe->temp_file->file.fd != NGX_INVALID_FILE)
    {
        if (ngx_delete_file(u->pipe->temp_file->file.name.data) == NGX_FILE_ERROR)
        {
            ngx_log_error(NGX_LOG_CRIT, r->connection->log, ngx_errno, 
				ngx_delete_file_n " \"%s\" failed", u->pipe->temp_file->file.name.data);
        }
    }

#if (NGX_HTTP_CACHE)

    if (r->cache) {

        if (u->cacheable) {

            if (rc == NGX_HTTP_BAD_GATEWAY || rc == NGX_HTTP_GATEWAY_TIME_OUT) {
                time_t  valid;

                valid = ngx_http_file_cache_valid(u->conf->cache_valid, rc);

                if (valid) {
                    r->cache->valid_sec = ngx_time() + valid;
                    r->cache->error = rc;
                }
            }
        }

        ngx_http_file_cache_free(r->cache, u->pipe->temp_file);
    }

#endif

    if (r->subrequest_in_memory && u->headers_in.status_n >= NGX_HTTP_SPECIAL_RESPONSE)
    {
        u->buffer.last = u->buffer.pos;
    }

    if (rc == NGX_DECLINED) 
	{
        return;
    }

    r->connection->log->action = "sending to client";

    if (!u->header_sent || rc == NGX_HTTP_REQUEST_TIME_OUT || rc == NGX_HTTP_CLIENT_CLOSED_REQUEST)
    {
        ngx_http_finalize_request(r, rc);
        return;
    }

    flush = 0;

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) 
	{
        rc = NGX_ERROR;
        flush = 1;
    }

    if (r->header_only) 
	{
        ngx_http_finalize_request(r, rc);
        return;
    }

    if (rc == 0) 
	{
        rc = ngx_http_send_special(r, NGX_HTTP_LAST);

    } 
	else if (flush)
	{
        r->keepalive = 0;
        rc = ngx_http_send_special(r, NGX_HTTP_FLUSH);
    }

	/* 调用 HTTP 框架实现的 ngx_http_finalize_request 方法关闭请求 */ 
    ngx_http_finalize_request(r, rc);
}


static ngx_int_t
ngx_http_upstream_process_header_line(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_table_elt_t  **ph;

    ph = (ngx_table_elt_t **) ((char *) &r->upstream->headers_in + offset);

    if (*ph == NULL) {
        *ph = h;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_ignore_header_line(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_content_length(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_http_upstream_t  *u;

    u = r->upstream;

    u->headers_in.content_length = h;
    u->headers_in.content_length_n = ngx_atoof(h->value.data, h->value.len);

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_last_modified(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_http_upstream_t  *u;

    u = r->upstream;

    u->headers_in.last_modified = h;

#if (NGX_HTTP_CACHE)

    if (u->cacheable) 
	{
        u->headers_in.last_modified_time = ngx_parse_http_time(h->value.data, h->value.len);
    }

#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_set_cookie(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_array_t           *pa;
    ngx_table_elt_t      **ph;
    ngx_http_upstream_t   *u;

    u = r->upstream;
    pa = &u->headers_in.cookies;

    if (pa->elts == NULL) {
        if (ngx_array_init(pa, r->pool, 1, sizeof(ngx_table_elt_t *)) != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

    ph = ngx_array_push(pa);
    if (ph == NULL) {
        return NGX_ERROR;
    }

    *ph = h;

#if (NGX_HTTP_CACHE)
    if (!(u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_SET_COOKIE)) {
        u->cacheable = 0;
    }
#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_cache_control(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_array_t          *pa;
    ngx_table_elt_t     **ph;
    ngx_http_upstream_t  *u;

    u = r->upstream;
    pa = &u->headers_in.cache_control;

    if (pa->elts == NULL) {
       if (ngx_array_init(pa, r->pool, 2, sizeof(ngx_table_elt_t *)) != NGX_OK)
       {
           return NGX_ERROR;
       }
    }

    ph = ngx_array_push(pa);
    if (ph == NULL) {
        return NGX_ERROR;
    }

    *ph = h;

#if (NGX_HTTP_CACHE)
    {
    u_char     *p, *start, *last;
    ngx_int_t   n;

    if (u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_CACHE_CONTROL) {
        return NGX_OK;
    }

    if (r->cache == NULL) {
        return NGX_OK;
    }

    if (r->cache->valid_sec != 0 && u->headers_in.x_accel_expires != NULL) {
        return NGX_OK;
    }

    start = h->value.data;
    last = start + h->value.len;

    if (ngx_strlcasestrn(start, last, (u_char *) "no-cache", 8 - 1) != NULL
        || ngx_strlcasestrn(start, last, (u_char *) "no-store", 8 - 1) != NULL
        || ngx_strlcasestrn(start, last, (u_char *) "private", 7 - 1) != NULL)
    {
        u->cacheable = 0;
        return NGX_OK;
    }

    p = ngx_strlcasestrn(start, last, (u_char *) "s-maxage=", 9 - 1);
    offset = 9;

    if (p == NULL) {
        p = ngx_strlcasestrn(start, last, (u_char *) "max-age=", 8 - 1);
        offset = 8;
    }

    if (p == NULL) {
        return NGX_OK;
    }

    n = 0;

    for (p += offset; p < last; p++) {
        if (*p == ',' || *p == ';' || *p == ' ') {
            break;
        }

        if (*p >= '0' && *p <= '9') {
            n = n * 10 + *p - '0';
            continue;
        }

        u->cacheable = 0;
        return NGX_OK;
    }

    if (n == 0) {
        u->cacheable = 0;
        return NGX_OK;
    }

    r->cache->valid_sec = ngx_time() + n;
    }
#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_expires(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_http_upstream_t  *u;

    u = r->upstream;
    u->headers_in.expires = h;

#if (NGX_HTTP_CACHE)
    {
    time_t  expires;

    if (u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_EXPIRES) {
        return NGX_OK;
    }

    if (r->cache == NULL) {
        return NGX_OK;
    }

    if (r->cache->valid_sec != 0) {
        return NGX_OK;
    }

    expires = ngx_parse_http_time(h->value.data, h->value.len);

    if (expires == NGX_ERROR || expires < ngx_time()) {
        u->cacheable = 0;
        return NGX_OK;
    }

    r->cache->valid_sec = expires;
    }
#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_accel_expires(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_http_upstream_t  *u;

    u = r->upstream;
    u->headers_in.x_accel_expires = h;

#if (NGX_HTTP_CACHE)
    {
    u_char     *p;
    size_t      len;
    ngx_int_t   n;

    if (u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_XA_EXPIRES) {
        return NGX_OK;
    }

    if (r->cache == NULL) {
        return NGX_OK;
    }

    len = h->value.len;
    p = h->value.data;

    if (p[0] != '@') {
        n = ngx_atoi(p, len);

        switch (n) {
        case 0:
            u->cacheable = 0;
            /* fall through */

        case NGX_ERROR:
            return NGX_OK;

        default:
            r->cache->valid_sec = ngx_time() + n;
            return NGX_OK;
        }
    }

    p++;
    len--;

    n = ngx_atoi(p, len);

    if (n != NGX_ERROR) {
        r->cache->valid_sec = n;
    }
    }
#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_limit_rate(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_int_t             n;
    ngx_http_upstream_t  *u;

    u = r->upstream;
    u->headers_in.x_accel_limit_rate = h;

    if (u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_XA_LIMIT_RATE) {
        return NGX_OK;
    }

    n = ngx_atoi(h->value.data, h->value.len);

    if (n != NGX_ERROR) {
        r->limit_rate = (size_t) n;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_buffering(ngx_http_request_t *r, ngx_table_elt_t *h, ngx_uint_t offset)
{
    u_char                c0, c1, c2;
    ngx_http_upstream_t  *u;

    u = r->upstream;

    if (u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_XA_BUFFERING) {
        return NGX_OK;
    }

    if (u->conf->change_buffering) {

        if (h->value.len == 2) {
            c0 = ngx_tolower(h->value.data[0]);
            c1 = ngx_tolower(h->value.data[1]);

            if (c0 == 'n' && c1 == 'o') {
                u->buffering = 0;
            }

        } else if (h->value.len == 3) {
            c0 = ngx_tolower(h->value.data[0]);
            c1 = ngx_tolower(h->value.data[1]);
            c2 = ngx_tolower(h->value.data[2]);

            if (c0 == 'y' && c1 == 'e' && c2 == 's') {
                u->buffering = 1;
            }
        }
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_charset(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    if (r->upstream->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_XA_CHARSET) {
        return NGX_OK;
    }

    r->headers_out.override_charset = &h->value;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_connection(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    r->upstream->headers_in.connection = h;

    if (ngx_strlcasestrn(h->value.data, h->value.data + h->value.len,
                         (u_char *) "close", 5 - 1)
        != NULL)
    {
        r->upstream->headers_in.connection_close = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_transfer_encoding(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    r->upstream->headers_in.transfer_encoding = h;

    if (ngx_strlcasestrn(h->value.data, h->value.data + h->value.len,
                         (u_char *) "chunked", 7 - 1)
        != NULL)
    {
        r->upstream->headers_in.chunked = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_vary(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_http_upstream_t  *u;

    u = r->upstream;
    u->headers_in.vary = h;

#if (NGX_HTTP_CACHE)

    if (u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_VARY) {
        return NGX_OK;
    }

    if (r->cache == NULL) {
        return NGX_OK;
    }

    if (h->value.len > NGX_HTTP_CACHE_VARY_LEN
        || (h->value.len == 1 && h->value.data[0] == '*'))
    {
        u->cacheable = 0;
    }

    r->cache->vary = h->value;

#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_copy_header_line(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_table_elt_t  *ho, **ph;

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    if (offset) {
        ph = (ngx_table_elt_t **) ((char *) &r->headers_out + offset);
        *ph = ho;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_copy_multi_header_lines(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_array_t      *pa;
    ngx_table_elt_t  *ho, **ph;

    pa = (ngx_array_t *) ((char *) &r->headers_out + offset);

    if (pa->elts == NULL) {
        if (ngx_array_init(pa, r->pool, 2, sizeof(ngx_table_elt_t *)) != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

    ph = ngx_array_push(pa);
    if (ph == NULL) {
        return NGX_ERROR;
    }

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;
    *ph = ho;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_copy_content_type(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    u_char  *p, *last;

    r->headers_out.content_type_len = h->value.len;
    r->headers_out.content_type = h->value;
    r->headers_out.content_type_lowcase = NULL;

    for (p = h->value.data; *p; p++) {

        if (*p != ';') {
            continue;
        }

        last = p;

        while (*++p == ' ') { /* void */ }

        if (*p == '\0') {
            return NGX_OK;
        }

        if (ngx_strncasecmp(p, (u_char *) "charset=", 8) != 0) {
            continue;
        }

        p += 8;

        r->headers_out.content_type_len = last - h->value.data;

        if (*p == '"') {
            p++;
        }

        last = h->value.data + h->value.len;

        if (*(last - 1) == '"') {
            last--;
        }

        r->headers_out.charset.len = last - p;
        r->headers_out.charset.data = p;

        return NGX_OK;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_copy_last_modified(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_table_elt_t  *ho;

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    r->headers_out.last_modified = ho;

#if (NGX_HTTP_CACHE)

    if (r->upstream->cacheable) {
        r->headers_out.last_modified_time =
                                    r->upstream->headers_in.last_modified_time;
    }

#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_rewrite_location(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_int_t         rc;
    ngx_table_elt_t  *ho;

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    if (r->upstream->rewrite_redirect) {
        rc = r->upstream->rewrite_redirect(r, ho, 0);

        if (rc == NGX_DECLINED) {
            return NGX_OK;
        }

        if (rc == NGX_OK) {
            r->headers_out.location = ho;

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "rewritten location: \"%V\"", &ho->value);
        }

        return rc;
    }

    if (ho->value.data[0] != '/') {
        r->headers_out.location = ho;
    }

    /*
     * we do not set r->headers_out.location here to avoid the handling
     * the local redirects without a host name by ngx_http_header_filter()
     */

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_rewrite_refresh(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    u_char           *p;
    ngx_int_t         rc;
    ngx_table_elt_t  *ho;

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    if (r->upstream->rewrite_redirect) {

        p = ngx_strcasestrn(ho->value.data, "url=", 4 - 1);

        if (p) {
            rc = r->upstream->rewrite_redirect(r, ho, p + 4 - ho->value.data);

        } else {
            return NGX_OK;
        }

        if (rc == NGX_DECLINED) {
            return NGX_OK;
        }

        if (rc == NGX_OK) {
            r->headers_out.refresh = ho;

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "rewritten refresh: \"%V\"", &ho->value);
        }

        return rc;
    }

    r->headers_out.refresh = ho;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_rewrite_set_cookie(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_int_t         rc;
    ngx_table_elt_t  *ho;

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    if (r->upstream->rewrite_cookie) {
        rc = r->upstream->rewrite_cookie(r, ho);

        if (rc == NGX_DECLINED) {
            return NGX_OK;
        }

#if (NGX_DEBUG)
        if (rc == NGX_OK) {
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "rewritten cookie: \"%V\"", &ho->value);
        }
#endif

        return rc;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_copy_allow_ranges(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_table_elt_t  *ho;

    if (r->upstream->conf->force_ranges) {
        return NGX_OK;
    }

#if (NGX_HTTP_CACHE)

    if (r->cached) {
        r->allow_ranges = 1;
        return NGX_OK;
    }

    if (r->upstream->cacheable) {
        r->allow_ranges = 1;
        r->single_range = 1;
        return NGX_OK;
    }

#endif

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    r->headers_out.accept_ranges = ho;

    return NGX_OK;
}


#if (NGX_HTTP_GZIP)

static ngx_int_t
ngx_http_upstream_copy_content_encoding(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_table_elt_t  *ho;

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    r->headers_out.content_encoding = ho;

    return NGX_OK;
}

#endif


static ngx_int_t
ngx_http_upstream_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_upstream_vars; v->name.len; v++) 
	{
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL)
		{
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_addr_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                     *p;
    size_t                      len;
    ngx_uint_t                  i;
    ngx_http_upstream_state_t  *state;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (r->upstream_states == NULL || r->upstream_states->nelts == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len = 0;
    state = r->upstream_states->elts;

    for (i = 0; i < r->upstream_states->nelts; i++) {
        if (state[i].peer) {
            len += state[i].peer->len + 2;

        } else {
            len += 3;
        }
    }

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;

    i = 0;

    for ( ;; ) {
        if (state[i].peer) {
            p = ngx_cpymem(p, state[i].peer->data, state[i].peer->len);
        }

        if (++i == r->upstream_states->nelts) {
            break;
        }

        if (state[i].peer) {
            *p++ = ',';
            *p++ = ' ';

        } else {
            *p++ = ' ';
            *p++ = ':';
            *p++ = ' ';

            if (++i == r->upstream_states->nelts) {
                break;
            }

            continue;
        }
    }

    v->len = p - v->data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_status_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                     *p;
    size_t                      len;
    ngx_uint_t                  i;
    ngx_http_upstream_state_t  *state;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (r->upstream_states == NULL || r->upstream_states->nelts == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len = r->upstream_states->nelts * (3 + 2);

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;

    i = 0;
    state = r->upstream_states->elts;

    for ( ;; ) {
        if (state[i].status) {
            p = ngx_sprintf(p, "%ui", state[i].status);

        } else {
            *p++ = '-';
        }

        if (++i == r->upstream_states->nelts) {
            break;
        }

        if (state[i].peer) {
            *p++ = ',';
            *p++ = ' ';

        } else {
            *p++ = ' ';
            *p++ = ':';
            *p++ = ' ';

            if (++i == r->upstream_states->nelts) {
                break;
            }

            continue;
        }
    }

    v->len = p - v->data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_response_time_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                     *p;
    size_t                      len;
    ngx_uint_t                  i;
    ngx_msec_int_t              ms;
    ngx_http_upstream_state_t  *state;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (r->upstream_states == NULL || r->upstream_states->nelts == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len = r->upstream_states->nelts * (NGX_TIME_T_LEN + 4 + 2);

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;

    i = 0;
    state = r->upstream_states->elts;

    for ( ;; ) {
        if (state[i].status) {

            if (data == 1 && state[i].header_time != (ngx_msec_t) -1) {
                ms = state[i].header_time;

            } else if (data == 2 && state[i].connect_time != (ngx_msec_t) -1) {
                ms = state[i].connect_time;

            } else {
                ms = state[i].response_time;
            }

            ms = ngx_max(ms, 0);
            p = ngx_sprintf(p, "%T.%03M", (time_t) ms / 1000, ms % 1000);

        } else {
            *p++ = '-';
        }

        if (++i == r->upstream_states->nelts) {
            break;
        }

        if (state[i].peer) {
            *p++ = ',';
            *p++ = ' ';

        } else {
            *p++ = ' ';
            *p++ = ':';
            *p++ = ' ';

            if (++i == r->upstream_states->nelts) {
                break;
            }

            continue;
        }
    }

    v->len = p - v->data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_response_length_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                     *p;
    size_t                      len;
    ngx_uint_t                  i;
    ngx_http_upstream_state_t  *state;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (r->upstream_states == NULL || r->upstream_states->nelts == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len = r->upstream_states->nelts * (NGX_OFF_T_LEN + 2);

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;

    i = 0;
    state = r->upstream_states->elts;

    for ( ;; ) {
        p = ngx_sprintf(p, "%O", state[i].response_length);

        if (++i == r->upstream_states->nelts) {
            break;
        }

        if (state[i].peer) {
            *p++ = ',';
            *p++ = ' ';

        } else {
            *p++ = ' ';
            *p++ = ':';
            *p++ = ' ';

            if (++i == r->upstream_states->nelts) {
                break;
            }

            continue;
        }
    }

    v->len = p - v->data;

    return NGX_OK;
}


ngx_int_t
ngx_http_upstream_header_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->upstream == NULL) {
        v->not_found = 1;
        return NGX_OK;
    }

    return ngx_http_variable_unknown_header(v, (ngx_str_t *) data,
                                         &r->upstream->headers_in.headers.part,
                                         sizeof("upstream_http_") - 1);
}


ngx_int_t
ngx_http_upstream_cookie_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_str_t  *name = (ngx_str_t *) data;

    ngx_str_t   cookie, s;

    if (r->upstream == NULL) {
        v->not_found = 1;
        return NGX_OK;
    }

    s.len = name->len - (sizeof("upstream_cookie_") - 1);
    s.data = name->data + sizeof("upstream_cookie_") - 1;

    if (ngx_http_parse_set_cookie_lines(&r->upstream->headers_in.cookies,
                                        &s, &cookie)
        == NGX_DECLINED)
    {
        v->not_found = 1;
        return NGX_OK;
    }

    v->len = cookie.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = cookie.data;

    return NGX_OK;
}


#if (NGX_HTTP_CACHE)

ngx_int_t
ngx_http_upstream_cache_status(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t  n;

    if (r->upstream == NULL || r->upstream->cache_status == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    n = r->upstream->cache_status - 1;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->len = ngx_http_cache_status[n].len;
    v->data = ngx_http_cache_status[n].data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_cache_last_modified(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    if (r->upstream == NULL
        || !r->upstream->conf->cache_revalidate
        || r->upstream->cache_status != NGX_HTTP_CACHE_EXPIRED
        || r->cache->last_modified == -1)
    {
        v->not_found = 1;
        return NGX_OK;
    }

    p = ngx_pnalloc(r->pool, sizeof("Mon, 28 Sep 1970 06:00:00 GMT") - 1);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_http_time(p, r->cache->last_modified) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_cache_etag(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->upstream == NULL
        || !r->upstream->conf->cache_revalidate
        || r->upstream->cache_status != NGX_HTTP_CACHE_EXPIRED
        || r->cache->etag.len == 0)
    {
        v->not_found = 1;
        return NGX_OK;
    }

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->len = r->cache->etag.len;
    v->data = r->cache->etag.data;

    return NGX_OK;
}

#endif


static char *
ngx_http_upstream(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy)
{
    char                          *rv;
    void                          *mconf;
    ngx_str_t                     *value;
    ngx_url_t                      u;
    ngx_uint_t                     m;
    ngx_conf_t                     pcf;
    ngx_http_module_t             *module;
    ngx_http_conf_ctx_t           *ctx, *http_ctx;
    ngx_http_upstream_srv_conf_t  *uscf;

    ngx_memzero(&u, sizeof(ngx_url_t));

    value = cf->args->elts;
    u.host = value[1];
    u.no_resolve = 1;
    u.no_port = 1;

    uscf = ngx_http_upstream_add(cf, &u, NGX_HTTP_UPSTREAM_CREATE |NGX_HTTP_UPSTREAM_WEIGHT |NGX_HTTP_UPSTREAM_MAX_FAILS
				|NGX_HTTP_UPSTREAM_FAIL_TIMEOUT |NGX_HTTP_UPSTREAM_DOWN |NGX_HTTP_UPSTREAM_BACKUP);
    if (uscf == NULL) 
	{
        return NGX_CONF_ERROR;
    }


    ctx = ngx_pcalloc(cf->pool, sizeof(ngx_http_conf_ctx_t));
    if (ctx == NULL)
	{
        return NGX_CONF_ERROR;
    }

    http_ctx = cf->ctx;
    ctx->main_conf = http_ctx->main_conf;

    /* the upstream{}'s srv_conf */
    ctx->srv_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_http_max_module);
    if (ctx->srv_conf == NULL)
	{
        return NGX_CONF_ERROR;
    }

    ctx->srv_conf[ngx_http_upstream_module.ctx_index] = uscf;
    uscf->srv_conf = ctx->srv_conf;


    /* the upstream{}'s loc_conf */
    ctx->loc_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_http_max_module);
    if (ctx->loc_conf == NULL) 
	{
        return NGX_CONF_ERROR;
    }

    for (m = 0; ngx_modules[m]; m++)
	{
        if (ngx_modules[m]->type != NGX_HTTP_MODULE) 
		{
            continue;
        }

        module = ngx_modules[m]->ctx;

        if (module->create_srv_conf)
		{
            mconf = module->create_srv_conf(cf);
            if (mconf == NULL)
			{
                return NGX_CONF_ERROR;
            }

            ctx->srv_conf[ngx_modules[m]->ctx_index] = mconf;
        }

        if (module->create_loc_conf) 
		{
            mconf = module->create_loc_conf(cf);
            if (mconf == NULL) 
			{
                return NGX_CONF_ERROR;
            }

            ctx->loc_conf[ngx_modules[m]->ctx_index] = mconf;
        }
    }

    uscf->servers = ngx_array_create(cf->pool, 4, sizeof(ngx_http_upstream_server_t));
    if (uscf->servers == NULL)
	{
        return NGX_CONF_ERROR;
    }


    /* parse inside upstream{} */
    pcf = *cf;
    cf->ctx = ctx;
    cf->cmd_type = NGX_HTTP_UPS_CONF;

    rv = ngx_conf_parse(cf, NULL);

    *cf = pcf;

    if (rv != NGX_CONF_OK) 
	{
        return rv;
    }

    if (uscf->servers->nelts == 0) 
	{
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "no servers are inside upstream");
        return NGX_CONF_ERROR;
    }

    return rv;
}


static char *
ngx_http_upstream_server(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_upstream_srv_conf_t  *uscf = conf;

    time_t                       fail_timeout;
    ngx_str_t                   *value, s;
    ngx_url_t                    u;
    ngx_int_t                    weight, max_fails;
    ngx_uint_t                   i;
    ngx_http_upstream_server_t  *us;

    us = ngx_array_push(uscf->servers);
    if (us == NULL) 
	{
        return NGX_CONF_ERROR;
    }

    ngx_memzero(us, sizeof(ngx_http_upstream_server_t));

    value = cf->args->elts;

    weight = 1;
    max_fails = 1;
    fail_timeout = 10;

    for (i = 2; i < cf->args->nelts; i++)
	{

        if (ngx_strncmp(value[i].data, "weight=", 7) == 0) 
		{

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_WEIGHT))
			{
                goto not_supported;
            }

            weight = ngx_atoi(&value[i].data[7], value[i].len - 7);

            if (weight == NGX_ERROR || weight == 0) 
			{
                goto invalid;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "max_fails=", 10) == 0) 
		{

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_MAX_FAILS))
			{
                goto not_supported;
            }

            max_fails = ngx_atoi(&value[i].data[10], value[i].len - 10);

            if (max_fails == NGX_ERROR) 
			{
                goto invalid;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "fail_timeout=", 13) == 0) 
		{

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_FAIL_TIMEOUT)) 
			{
                goto not_supported;
            }

            s.len = value[i].len - 13;
            s.data = &value[i].data[13];

            fail_timeout = ngx_parse_time(&s, 1);

            if (fail_timeout == (time_t) NGX_ERROR) 
			{
                goto invalid;
            }

            continue;
        }

        if (ngx_strcmp(value[i].data, "backup") == 0) 
		{
            if (!(uscf->flags & NGX_HTTP_UPSTREAM_BACKUP)) 
			{
                goto not_supported;
            }

            us->backup = 1;

            continue;
        }

        if (ngx_strcmp(value[i].data, "down") == 0)
		{
            if (!(uscf->flags & NGX_HTTP_UPSTREAM_DOWN))
			{
                goto not_supported;
            }

            us->down = 1;

            continue;
        }

        goto invalid;
    }

    ngx_memzero(&u, sizeof(ngx_url_t));

    u.url = value[1];
    u.default_port = 80;

    if (ngx_parse_url(cf->pool, &u) != NGX_OK) 
	{
        if (u.err) 
		{
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s in upstream \"%V\"", u.err, &u.url);
        }

        return NGX_CONF_ERROR;
    }

    us->name = u.url;
    us->addrs = u.addrs;
    us->naddrs = u.naddrs;
    us->weight = weight;
    us->max_fails = max_fails;
    us->fail_timeout = fail_timeout;

    return NGX_CONF_OK;

invalid:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid parameter \"%V\"", &value[i]);

    return NGX_CONF_ERROR;

not_supported:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "balancing method does not support parameter \"%V\"", &value[i]);

    return NGX_CONF_ERROR;
}

/*
根据url查找umcf->upstreams中是否存在相同的，存在相同的则返回，不存在则添加
*/
ngx_http_upstream_srv_conf_t *
ngx_http_upstream_add(ngx_conf_t *cf, ngx_url_t *u, ngx_uint_t flags)
{
    ngx_uint_t                      i;
    ngx_http_upstream_server_t     *us;
    ngx_http_upstream_srv_conf_t   *uscf, **uscfp;
    ngx_http_upstream_main_conf_t  *umcf;

    if (!(flags & NGX_HTTP_UPSTREAM_CREATE)) 
	{

        if (ngx_parse_url(cf->pool, u) != NGX_OK)
		{
            if (u->err) 
			{
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s in upstream \"%V\"", u->err, &u->url);
            }

            return NULL;
        }
    }

    umcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_upstream_module);

    uscfp = umcf->upstreams.elts;
	/*查找全局上有服务器是否有相同ip(host)和port的*/
    for (i = 0; i < umcf->upstreams.nelts; i++)
	{
        if (uscfp[i]->host.len != u->host.len || ngx_strncasecmp(uscfp[i]->host.data, u->host.data, u->host.len) != 0)
        {
            continue;
        }

        if ((flags & NGX_HTTP_UPSTREAM_CREATE) && (uscfp[i]->flags & NGX_HTTP_UPSTREAM_CREATE))
        {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "duplicate upstream \"%V\"", &u->host);
            return NULL;
        }

        if ((uscfp[i]->flags & NGX_HTTP_UPSTREAM_CREATE) && !u->no_port)
		{
            ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "upstream \"%V\" may not have port %d", &u->host, u->port);
            return NULL;
        }

        if ((flags & NGX_HTTP_UPSTREAM_CREATE) && !uscfp[i]->no_port)
		{
            ngx_log_error(NGX_LOG_WARN, cf->log, 0, "upstream \"%V\" may not have port %d in %s:%ui",
                          &u->host, uscfp[i]->port, uscfp[i]->file_name, uscfp[i]->line);
            return NULL;
        }

        if (uscfp[i]->port && u->port && uscfp[i]->port != u->port)
        {
            continue;
        }

        if (uscfp[i]->default_port && u->default_port && uscfp[i]->default_port != u->default_port)
        {
            continue;
        }

        if (flags & NGX_HTTP_UPSTREAM_CREATE)
		{
            uscfp[i]->flags = flags;
        }

        return uscfp[i];
    }

	/*未找到，则新建一个*/
    uscf = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_srv_conf_t));
    if (uscf == NULL) 
	{
        return NULL;
    }

    uscf->flags = flags;
    uscf->host = u->host;
    uscf->file_name = cf->conf_file->file.name.data;
    uscf->line = cf->conf_file->line;
    uscf->port = u->port;
    uscf->default_port = u->default_port;
    uscf->no_port = u->no_port;

    if (u->naddrs == 1 && (u->port || u->family == AF_UNIX)) 
	{
        uscf->servers = ngx_array_create(cf->pool, 1, sizeof(ngx_http_upstream_server_t));
        if (uscf->servers == NULL)
		{
            return NULL;
        }

        us = ngx_array_push(uscf->servers);
        if (us == NULL) 
		{
            return NULL;
        }

        ngx_memzero(us, sizeof(ngx_http_upstream_server_t));

        us->addrs = u->addrs;
        us->naddrs = 1;
    }

    uscfp = ngx_array_push(&umcf->upstreams);
    if (uscfp == NULL) 
	{
        return NULL;
    }

    *uscfp = uscf;

    return uscf;
}


char *
ngx_http_upstream_bind_set_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_int_t                           rc;
    ngx_str_t                          *value;
    ngx_http_complex_value_t            cv;
    ngx_http_upstream_local_t         **plocal, *local;
    ngx_http_compile_complex_value_t    ccv;

    plocal = (ngx_http_upstream_local_t **) (p + cmd->offset);

    if (*plocal != NGX_CONF_UNSET_PTR) {
        return "is duplicate";
    }

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "off") == 0) {
        *plocal = NULL;
        return NGX_CONF_OK;
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &cv;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    local = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_local_t));
    if (local == NULL) {
        return NGX_CONF_ERROR;
    }

    *plocal = local;

    if (cv.lengths) {
        local->value = ngx_palloc(cf->pool, sizeof(ngx_http_complex_value_t));
        if (local->value == NULL) {
            return NGX_CONF_ERROR;
        }

        *local->value = cv;

        return NGX_CONF_OK;
    }

    local->addr = ngx_palloc(cf->pool, sizeof(ngx_addr_t));
    if (local->addr == NULL) {
        return NGX_CONF_ERROR;
    }

    rc = ngx_parse_addr(cf->pool, local->addr, value[1].data, value[1].len);

    switch (rc) {
    case NGX_OK:
        local->addr->name = value[1];
        return NGX_CONF_OK;

    case NGX_DECLINED:
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid address \"%V\"", &value[1]);
        /* fall through */

    default:
        return NGX_CONF_ERROR;
    }
}


static ngx_addr_t *
ngx_http_upstream_get_local(ngx_http_request_t *r, ngx_http_upstream_local_t *local)
{
    ngx_int_t    rc;
    ngx_str_t    val;
    ngx_addr_t  *addr;

    if (local == NULL) 
	{
        return NULL;
    }

    if (local->value == NULL) 
	{
        return local->addr;
    }

    if (ngx_http_complex_value(r, local->value, &val) != NGX_OK) 
	{
        return NULL;
    }

    if (val.len == 0) {
        return NULL;
    }

    addr = ngx_palloc(r->pool, sizeof(ngx_addr_t));
    if (addr == NULL) {
        return NULL;
    }

    rc = ngx_parse_addr(r->pool, addr, val.data, val.len);

    switch (rc) {
    case NGX_OK:
        addr->name = val;
        return addr;

    case NGX_DECLINED:
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "invalid local address \"%V\"", &val);
        /* fall through */

    default:
        return NULL;
    }
}


char *
ngx_http_upstream_param_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf)
{
    char  *p = conf;

    ngx_str_t                   *value;
    ngx_array_t                **a;
    ngx_http_upstream_param_t   *param;

    a = (ngx_array_t **) (p + cmd->offset);

    if (*a == NULL) {
        *a = ngx_array_create(cf->pool, 4, sizeof(ngx_http_upstream_param_t));
        if (*a == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    param = ngx_array_push(*a);
    if (param == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;

    param->key = value[1];
    param->value = value[2];
    param->skip_empty = 0;

    if (cf->args->nelts == 4) {
        if (ngx_strcmp(value[3].data, "if_not_empty") != 0) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid parameter \"%V\"", &value[3]);
            return NGX_CONF_ERROR;
        }

        param->skip_empty = 1;
    }

    return NGX_CONF_OK;
}


ngx_int_t
ngx_http_upstream_hide_headers_hash(ngx_conf_t *cf,
    ngx_http_upstream_conf_t *conf, ngx_http_upstream_conf_t *prev,
    ngx_str_t *default_hide_headers, ngx_hash_init_t *hash)
{
    ngx_str_t       *h;
    ngx_uint_t       i, j;
    ngx_array_t      hide_headers;
    ngx_hash_key_t  *hk;

    if (conf->hide_headers == NGX_CONF_UNSET_PTR
        && conf->pass_headers == NGX_CONF_UNSET_PTR)
    {
        conf->hide_headers = prev->hide_headers;
        conf->pass_headers = prev->pass_headers;

        conf->hide_headers_hash = prev->hide_headers_hash;

        if (conf->hide_headers_hash.buckets
#if (NGX_HTTP_CACHE)
            && ((conf->cache == 0) == (prev->cache == 0))
#endif
           )
        {
            return NGX_OK;
        }

    } else {
        if (conf->hide_headers == NGX_CONF_UNSET_PTR) {
            conf->hide_headers = prev->hide_headers;
        }

        if (conf->pass_headers == NGX_CONF_UNSET_PTR) {
            conf->pass_headers = prev->pass_headers;
        }
    }

    if (ngx_array_init(&hide_headers, cf->temp_pool, 4, sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    for (h = default_hide_headers; h->len; h++) {
        hk = ngx_array_push(&hide_headers);
        if (hk == NULL) {
            return NGX_ERROR;
        }

        hk->key = *h;
        hk->key_hash = ngx_hash_key_lc(h->data, h->len);
        hk->value = (void *) 1;
    }

    if (conf->hide_headers != NGX_CONF_UNSET_PTR) {

        h = conf->hide_headers->elts;

        for (i = 0; i < conf->hide_headers->nelts; i++) {

            hk = hide_headers.elts;

            for (j = 0; j < hide_headers.nelts; j++) {
                if (ngx_strcasecmp(h[i].data, hk[j].key.data) == 0) {
                    goto exist;
                }
            }

            hk = ngx_array_push(&hide_headers);
            if (hk == NULL) {
                return NGX_ERROR;
            }

            hk->key = h[i];
            hk->key_hash = ngx_hash_key_lc(h[i].data, h[i].len);
            hk->value = (void *) 1;

        exist:

            continue;
        }
    }

    if (conf->pass_headers != NGX_CONF_UNSET_PTR) {

        h = conf->pass_headers->elts;
        hk = hide_headers.elts;

        for (i = 0; i < conf->pass_headers->nelts; i++) {
            for (j = 0; j < hide_headers.nelts; j++) {

                if (hk[j].key.data == NULL) {
                    continue;
                }

                if (ngx_strcasecmp(h[i].data, hk[j].key.data) == 0) {
                    hk[j].key.data = NULL;
                    break;
                }
            }
        }
    }

    hash->hash = &conf->hide_headers_hash;
    hash->key = ngx_hash_key_lc;
    hash->pool = cf->pool;
    hash->temp_pool = NULL;

    return ngx_hash_init(hash, hide_headers.elts, hide_headers.nelts);
}


static void *
ngx_http_upstream_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_upstream_main_conf_t  *umcf;

    umcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_main_conf_t));
    if (umcf == NULL) 
	{
        return NULL;
    }

    if (ngx_array_init(&umcf->upstreams, cf->pool, 4, sizeof(ngx_http_upstream_srv_conf_t *)) != NGX_OK)
    {
        return NULL;
    }

    return umcf;
}


static char *
ngx_http_upstream_init_main_conf(ngx_conf_t *cf, void *conf)
{
    ngx_http_upstream_main_conf_t  *umcf = conf;

    ngx_uint_t                      i;
    ngx_array_t                     headers_in;
    ngx_hash_key_t                 *hk;
    ngx_hash_init_t                 hash;
    ngx_http_upstream_init_pt       init;
    ngx_http_upstream_header_t     *header;
    ngx_http_upstream_srv_conf_t  **uscfp;

    uscfp = umcf->upstreams.elts;
	
	//遍历upstream数组。upstream的来源包括显性的配置upstream模式（ngx_http_upstream）
	//和隐性的proxy_pass指令（ngx_http_proxy_pass），upstream的添加是通过
	//ngx_http_upstream_add函数，需要注意的是proxy_pass指令携带变量参数时不会添加upstream
    for (i = 0; i < umcf->upstreams.nelts; i++) {
        init = uscfp[i]->peer.init_upstream ? uscfp[i]->peer.init_upstream: ngx_http_upstream_init_round_robin;

        if (init(cf, uscfp[i]) != NGX_OK) {
            return NGX_CONF_ERROR;
        }
    }


    /* upstream_headers_in_hash */

    if (ngx_array_init(&headers_in, cf->temp_pool, 32, sizeof(ngx_hash_key_t)) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    for (header = ngx_http_upstream_headers_in; header->name.len; header++) {
        hk = ngx_array_push(&headers_in);
        if (hk == NULL) {
            return NGX_CONF_ERROR;
        }

        hk->key = header->name;
        hk->key_hash = ngx_hash_key_lc(header->name.data, header->name.len);
        hk->value = header;
    }

    hash.hash = &umcf->headers_in_hash;
    hash.key = ngx_hash_key_lc;
    hash.max_size = 512;
    hash.bucket_size = ngx_align(64, ngx_cacheline_size);
    hash.name = "upstream_headers_in_hash";
    hash.pool = cf->pool;
    hash.temp_pool = NULL;

    if (ngx_hash_init(&hash, headers_in.elts, headers_in.nelts) != NGX_OK)
	{
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}
