
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
static ngx_int_t ngx_http_upstream_intercept_errors(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
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
static ngx_int_t ngx_http_upstream_process_buffering(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
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
	//Óï·¨: upstream name {...}
	//upstream¿é¶¨ÒåÁËÒ»¸öÉÏÓÎ·şÎñÆ÷µÄ¼¯Èº£¬±ãÓÚ·½Ïò´úÀíÖĞµÄproxy_passÊ¹ÓÃ
    { 
		ngx_string("upstream"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_TAKE1,
		ngx_http_upstream,
		0,
		0,
		NULL 
    },
	//Óï·¨: server name [weight=number | max_fails=number | fail_timeout=time | down | backpup]
	//serverÅäÖÃÏîÖÆ¶©ÁËÒ»Ì¨ÉÏÓÎ·şÎñÆ÷µÄÃû×Ö£¬Õâ¸öÃû×Ö¿ÉÒÔÊÇÓòÃû¡¢IPµØÖ·¶Ë¿Ú¡¢UNIX¾ä±úµÈ£¬Æäºó»¹¿É¸úÏÂÁĞ²ÎÊı
	//weight=number: ÉèÖÃÕâÌ¨ÉÏÓÎ·şÎñÆ÷×ª·¢µÄÈ¨ÖØ£¬Ä¬ÈÏÎª1
	//max_fails=number: Ä¬ÈÏÎª1£¬Èç¹ûÉèÖÃÎª0£¬ Ôò±íÊ¾²»¼ì²éÊ§°Ü´ÎÊı 
	//fail_timeout=time: Ä¬ÈÏÎª10Ãë
	//	±íÊ¾ÔÚfail_timeoutÊ±¼ä¶ÎÄÚ×ª·¢Ê§°Ümax_fails´Îºó¾ÍÈÏÎªÉÏÓÎ·şÎñÆ÷ÔİÊ±²»¿ÉÓÃ£¬ÓÃÓÚÓÅ»¯·´Ïò´úÀí¹¦ÄÜ¡£
	//	fail_timeoutÓëÏòÉÏÓÎ·şÎñÆ÷½¨Á¢Á¬½ÓµÄ³¬Ê±Ê±¼ä¡¢¶ÁÈ¡ÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦³¬Ê±Ê±¼äµÈÍêÈ«ÎŞ¹Ø¡£
	//down: ±íÊ¾ËùÔÚµÄÉÏÓÎ·şÎñÆ÷ÓÀ¾ÃÏÂÏß£¬Ö»ÔÚÊ¹ÓÃip_hashÅäÖÃÏîÊ±ÓĞĞ§
	//backup: ±íÊ¾¸ÃÉÏÓÎ·şÎñÆ÷ÊÇ±¸·İ·şÎñÆ÷£¬Ö»ÓĞÔÚËùÓĞµÄ·Ç±¸·İ·şÎñÆ÷¶¼Ê§Ğ§ºó£¬²Å»áÏò¸ÃÉÏÓÎ·şÎñÆ÷×ª·¢ÇëÇó£¬
	//	ÔÚÊ¹ÓÃip_hashÅäÖÃÏîÊ±ÎŞĞ§
	//ÀıÈç: upstream backend {
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

//upstream Ä£¿é²»²úÉú×Ô¼ºµÄÄÚÈİ£¬¶øÊÇÍ¨¹ıÇëÇóºó¶Ë·şÎñÆ÷µÃµ½ÄÚÈİ¡£Nginx ÄÚ²¿·â×°ÁËÇëÇó²¢È¡µÃÏìÓ¦ÄÚÈİµÄÕû¸ö¹ı³Ì£¬
//ËùÒÔupstream Ä£¿éÖ»ĞèÒª¿ª·¢Èô¸É»Øµ÷º¯Êı£¬Íê³É¹¹ÔìÇëÇóºÍ½âÎöÏìÓ¦µÈ¾ßÌåµÄ¹¤×÷¡£

//µ±¿Í»§¶Ë·¢À´HTTPÇëÇóÊ±£¬Nginx²¢²»»áÁ¢¿Ì×ª·¢µ½ÉÏÓÎ·şÎñÆ÷£¬¶øÊÇÏÈ°ÑÓÃ»§µÄÇëÇó(°üÀ¨HTTP°üÌå)ÍêÕûµØ½ÓÊÕµ½Nginx
//ËùÔÚ·şÎñÆ÷µÄÓ²ÅÌ»òÄÚ´æÖĞ£¬È»ºóÔÙÏòÉÏÓÎ·şÎñÆ÷·¢ÆğÁ¬½Ó£¬°Ñ»º´æµÄ¿Í»§¶ËÇëÇó×ª·¢µ½ÉÏÓÎ·şÎñÆ÷
//Nginx·´Ïò´úÀí·şÎñÆ÷¿ÉÒÔ¸ù¾İ¶àÖÖ·½°¸´ÓÉÏÓÎ·şÎñÆ÷µÄ¼¯ÈºÖĞÑ¡ÔñÒ»Ì¨¡£ËüµÄ¸ºÔØ¾ùºâ·½°¸°üÀ¨°´IPµØÖ·×öÉ¢ÁĞµÈ
//Èç¹ûÉÏÓÎ·şÎñÆ÷·µ»ØÄÚÈİ£¬Ôò²»»áÏÈÍêÕûµÄ»º´æµ½Nginx´úÀí·şÎñÆ÷ÔÙÏò¿Í»§¶Ë×ª·¢£¬¶øÊÇ±ß½ÓÊÜ±ß×ª·¢µ½¿Í»§¶Ë
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
	//´¦ÀíÇëÇóµÄÉÏÓÎ·şÎñÆ÷µØÖ·
    { ngx_string("upstream_addr"), NULL, ngx_http_upstream_addr_variable, 0, NGX_HTTP_VAR_NOCACHEABLE, 0 },
	//ÉÏÓÎ·şÎñÆ÷·µ»ØµÄÏìÓ¦ÖĞµÄHTTPÏìÓ¦Âë
    { ngx_string("upstream_status"), NULL,
      ngx_http_upstream_status_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_connect_time"), NULL,
      ngx_http_upstream_response_time_variable, 2,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_header_time"), NULL,
      ngx_http_upstream_response_time_variable, 1,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },
	//ÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦Ê±¼ä£¬¾«È·µ½ºÁÃë
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

//´ÓÄÚ´æ³ØÖĞ´´½¨ngx_http_upstream_t½á¹¹Ìå£¬ÆäÖĞµÄ³ÉÔ±»¹ĞèÒª¸÷¸öHTTPÄ£¿é×ÔĞĞÉèÖÃ
ngx_int_t
ngx_http_upstream_create(ngx_http_request_t *r)
{
    ngx_http_upstream_t  *u;

    u = r->upstream;

	/*
     * ÈôÒÑ¾­´´½¨¹ıngx_http_upstream_t ÇÒ¶¨ÒåÁËcleanup³ÉÔ±£¬
     * Ôòµ÷ÓÃcleanupÇåÀí·½·¨½«Ô­Ê¼½á¹¹ÌåÇå³ı£»
     */
    if (u && u->cleanup) 
	{
        r->main->count++;
        ngx_http_upstream_cleanup(r);
    }

    u = ngx_pcalloc(r->pool, sizeof(ngx_http_upstream_t));
    if (u == NULL) 
	{
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
Æô¶¯upstream¡£
ÔÚ Nginx ÖĞµ÷ÓÃ ngx_http_upstream_init ·½·¨Æô¶¯ upstream »úÖÆ£¬µ«ÊÇÔÚÊ¹ÓÃ upstream »úÖÆÖ®Ç°±ØĞëµ÷ÓÃ
ngx_http_upstream_create ·½·¨´´½¨ ngx_http_upstream_t ½á¹¹Ìå£¬ÒòÎªÄ¬ÈÏÇé¿öÏÂ ngx_http_request_t 
½á¹¹ÌåÖĞµÄ upstream ³ÉÔ±ÊÇÖ¸Ïò NULL£¬¸Ã½á¹¹ÌåµÄ¾ßÌå³õÊ¼»¯¹¤×÷»¹ĞèÓÉ HTTP Ä£¿éÍê³É¡£
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
	//Èç¹ûÇëÇó¶ÔÓ¦µÄ¿Í»§¶ËµÄÁ¬½ÓÉÏµÄ¶ÁÊÂ¼şÔÚ¶¨Ê±Æ÷ÖĞ£¬ÄÇÃ´°ÑÕâ¸ö¶ÁÊÂ¼ş´Ó¶¨Ê±Æ÷ÖĞÒÆ³ı
	//ÒòÎªÒ»µ©Æô¶¯upstream»úÖÆ£¬¾Í²»Ó¦¸Ã¶Ô¿Í»§¶ËµÄ¶Á²Ù×÷´øÓĞ³¬Ê±Ê±¼äµÄ´¦Àí£¬ÇëÇóµÄÖ÷Òª
	//´¥·¢ÊÂ¼ş½«ÒÔÓëÉÏÓÎ·şÎñÆ÷µÄÁ¬½ÓÎªÖ÷
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
	///nignxÏòÉÏÓÎ·şÎñÆ÷·¢³öupstreamÇëÇóÊ±£¬»áÏÈ¼ì²éÊÇ·ñÓĞ¿ÉÓÃ»º´æ¡£
	
    if (u->conf->cache) {   //¼ì²é¸ÃlocationÊÇ·ñÅäÖÃÁËproxy_cache/fastcgi_cache£¬Èç¹ûÊÇÔòÊ¹ÓÃ»º´æ
        ngx_int_t  rc;

		///Ñ°ÕÒ»º´æ  
        ///Èç¹û·µ»ØNGX_DECLINED¾ÍÊÇËµ»º´æÖĞÃ»ÓĞ£¬ÇëÏòºó¶Ë·¢ËÍÇëÇó 
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
	/* ÎÄ¼ş»º´æ±êÖ¾Î» */ 
    u->store = u->conf->store;

	//¼ì²éngx_http_upstream_conf_tÅäÖÃ½á¹¹ÌåÖĞµÄignore_client_abort±êÖ¾Î»£¬
	//Èç¹ûignore_client_abortÎª0(Êµ¼ÊÉÏ»¹ĞèÒªÈÃstore±êÖ¾Î»Îª0£¬ ÇëÇó½á¹¹ÌåÖĞpost_action±êÖ¾Î»Îª0)£¬
	//¾Í»áÉèÖÃNginxÓëÏÂÓÎ¿Í»§¶ËÖ®¼äTCPÁ¬½ÓµÄ¼ì²é·½·¨
	 /*
     * ¼ì²éngx_http_upstream_t ½á¹¹ÖĞ±êÖ¾Î» store£»
     * ¼ì²éngx_http_request_t ½á¹¹ÖĞ±êÖ¾Î» post_action£»
     * ¼ì²éngx_http_upstream_conf_t ½á¹¹ÖĞ±êÖ¾Î» ignore_client_abort£»
     * ÈôÉÏÃæÕâĞ©±êÖ¾Î»Îª1£¬Ôò±íÊ¾ĞèÒª¼ì²éNginxÓëÏÂÓÎ(¼´¿Í»§¶Ë)Ö®¼äµÄTCPÁ¬½ÓÊÇ·ñ¶Ï¿ª£»
     */
    if (!u->store && !r->post_action && !u->conf->ignore_client_abort) {
        r->read_event_handler = ngx_http_upstream_rd_check_broken_connection;
        r->write_event_handler = ngx_http_upstream_wr_check_broken_connection;
    }

	 /* °Ñµ±Ç°ÇëÇó°üÌå½á¹¹±£´æÔÚngx_http_upstream_t ½á¹¹µÄrequest_bufsÁ´±í»º³åÇøÖĞ */
    if (r->request_body) {
        u->request_bufs = r->request_body->bufs;
    }

	//µ÷ÓÃÇëÇóÖĞngx_http_upstream_t½á¹¹ÌåÀïÓÉÄ³¸öHTTPÄ£¿éÊµÏÖµÄcreate_request·½·¨£¬¹¹Ôì·¢ÍùÉÏÓÎ·şÎñÆ÷µÄÇëÇó
    if (u->create_request(r) != NGX_OK) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
	
	/* »ñÈ¡ngx_http_upstream_t½á¹¹ÖĞÖ÷¶¯Á¬½Ó½á¹¹peerµÄlocal±¾µØµØÖ·ĞÅÏ¢ */  
    u->peer.local = ngx_http_upstream_get_local(r, u->conf->local);
	
	/* »ñÈ¡ngx_http_core_moduleÄ£¿éµÄloc¼¶±ğµÄÅäÖÃÏî½á¹¹ */  
    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
	
	/* ³õÊ¼»¯ngx_http_upstream_t½á¹¹ÖĞ³ÉÔ±outputÏòÏÂÓÎ·¢ËÍÏìÓ¦µÄ·½Ê½ */
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

	/* Ìí¼ÓÓÃÓÚ±íÊ¾ÉÏÓÎÏìÓ¦µÄ×´Ì¬£¬ÀıÈç£º´íÎó±àÂë¡¢°üÌå³¤¶ÈµÈ */  
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

	//µ÷ÓÃ ngx_http_cleanup_add ·½·¨ÏòÔ­Ê¼ÇëÇóµÄ cleanup Á´±íÎ²¶ËÌí¼ÓÒ»¸ö»Øµ÷ handler ·½·¨£¬
	//¸Ã»Øµ÷·½·¨ÉèÖÃÎªngx_http_upstream_cleanup£¬Èôµ±Ç°ÇëÇó½áÊøÊ±»áµ÷ÓÃ¸Ã·½·¨×öÒ»Ğ©ÇåÀí¹¤×÷
    cln = ngx_http_cleanup_add(r, 0);
    if (cln == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    cln->handler = ngx_http_upstream_cleanup;
    cln->data = r;
    u->cleanup = &cln->handler;

	//²»ĞèÒª½âÎöµØÖ·µÄÇé¿ö£¬°üÀ¨ÅäÖÃupstreamÄ£Ê½ÏÂµÄ¾²Ì¬server£¬¾²Ì¬proxy_passÖ¸Áî£¬
	//¼´³õÊ¼»¯¹ı³ÌÖĞÒÑ¾­ÄÜ½«µØÖ·½âÎöºÃµÄÇé¿ö£¨¾²Ì¬IPºó¶Ë»òÕß¿ÉÍ¨¹ıgethostbyname»ñÈ¡µ½µØÖ·µÄÓòÃûºó¶Ë£©
    if (u->resolved == NULL) {
		/* ÈôÃ»ÓĞÊµÏÖu->resolved±êÖ¾Î»£¬Ôò¶¨ÒåÉÏÓÎ·şÎñÆ÷µÄÅäÖÃ */ 
        uscf = u->conf->upstream;
    } 
	else 
	{
		/* ÈôÊµÏÖÁËu->resolved±êÖ¾Î»£¬Ôò½âÎöÖ÷»úÓòÃû£¬Ö¸¶¨ÉÏÓÎ·şÎñÆ÷µÄµØÖ·£» */
#if (NGX_HTTP_SSL)
        u->ssl_name = u->resolved->host;
#endif
		/*
         * ÈôÒÑ¾­Ö¸¶¨ÁËÉÏÓÎ·şÎñÆ÷µØÖ·£¬Ôò²»ĞèÒª½âÎö£¬
         * Ö±½Óµ÷ÓÃngx_http_upstream_connection·½·¨ÏòÉÏÓÎ·şÎñÆ÷·¢ÆğÁ¬½Ó£»
         * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
         */
        //ĞèÒª½âÎöµØÖ·£¬²¢ÇÒÒÑ¾­½âÎöÍê±ÏµÄÇé¿ö£¨ÒÑ¾­ÓĞsocketµØÖ·£©£¬Ö±½ÓÁ¬½Óºó¶Ë
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
		/*ĞèÒª½âÎöµØÖ·£¬µ«socketµØÖ·»¹Î´½âÎö*/
        host = &u->resolved->host;

        umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);

		//ÏÈÕÒµ±Ç°·ÃÎÊµÄhostÊÇ·ñÔÚÅäÖÃµÄupstreamÊı×éÖĞ
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


		//µ±·ÃÎÊµÄµØÖ·ĞèÒª½øĞĞÓòÃû½âÎöµÄÇé¿öÏÂ£¨±ÈÈçproxy_passÖĞ´æÔÚ±äÁ¿£¬±äÁ¿µÄÖµÎªÒ»¸öÓòÃû£¬ÈÎºÎ¾²Ì¬ÓòÃû¾ù»áÔÚ³õÊ¼»¯Ê±ºò±»½âÎö£©£¬
		//ÊÇÎŞ·¨Ê¹ÓÃkeepaliveÁ¬½ÓµÄ£¬Çë¿´nginxÔÚ½âÎöÓòÃû³É¹¦ºóÊÇÔõÃ´´¦ÀíµÄ£¨¼´ngx_http_upstream_init_requestº¯ÊıÖĞÉèÖÃctx->handler = ngx_http_upstream_resolve_handler£©

        if (u->resolved->port == 0) 
		{
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "no port in upstream \"%V\"", host);
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        temp.name = *host;



		// ¿ªÊ¼½øĞĞÓòÃû½âÎö
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
		// ÉèÖÃDNS½âÎö¹³×Ó£¬½âÎöDNS³É¹¦ºó»áÖ´ĞĞ
        ctx->handler = ngx_http_upstream_resolve_handler;
        ctx->data = r;
        ctx->timeout = clcf->resolver_timeout;

        u->resolved->ctx = ctx;

		 // ¿ªÊ¼´¦ÀíDNS½âÎöÓòÃû
        if (ngx_resolve_name(ctx) != NGX_OK) 
		{
            u->resolved->ctx = NULL;
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        return;
    }

	//Ö»ÓĞ²»ĞèÒª½âÎöµØÖ·»òÕßÄÜÕÒµ½upstreamµÄÇé¿ö²Å»á×ßµ½ÕâÀï
	
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

	//µ÷ÓÃ·½·¨ngx_http_upstream_connectÏòÉÏÓÎ·şÎñÆ÷·¢ÆğÁ¬½Ó
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

    if (c == NULL) {   //ÕâÊÇÒ»¸öĞÂµÄÇëÇóå

		//¼ì²éÇëÇó·½Ê½ÊÇ·ñÂú×ã»º´æ·½Ê½
        if (!(r->method & u->conf->cache_methods)) {  
            return NGX_DECLINED;
        }

		//»ñÈ¡¶ÔÓ¦µÄ¡£¡£¡£
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

        if (u->create_key(r) != NGX_OK) {	//ngx_http_proxy_create_key ½« `proxy_cache_key` ÅäÖÃÖ¸Áî¶¨ÒåµÄ»º´æ key ¸ù¾İÇëÇóĞÅÏ¢½øĞĞÈ¡Öµ
            return NGX_ERROR;
        }

        /* TODO: add keys */

        ngx_http_file_cache_create_key(r);  //¹¹ÔìÊı×Ökey

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


		//¸ù¾İÅäÖÃÎÄ¼şÖĞ (proxy_cache_bypass) »º´æÈÆ¹ıÌõ¼şºÍÇëÇóĞÅÏ¢£¬ÅĞ¶ÏÊÇ·ñÓ¦¸Ã¼ÌĞø³¢ÊÔÊ¹ÓÃ»º´æÊı¾İÏìÓ¦¸ÃÇëÇó
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

	//²éÕÒÊÇ·ñÓĞ¶ÔÓ¦µÄÓĞĞ§»º´æÊı¾İ
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

///»ñÈ¡´æ´¢cache keyÓÚ¶ÔÓ¦ÎÄ¼şÂ·¾¶µÄÉ¢ÁĞ±íËùÔÚµÄ¡£¡£¡£
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
	// ´´½¨ÂÖÑ¯ºó¶Ë£¬ÉèÖÃ»ñÈ¡peerºÍÊÍ·ÅpeerµÄ¹³×ÓµÈ£¨Óëngx_http_upstream_init_round_robin_peerµÄ¹¦ÄÜÓĞµãÀàËÆ£©
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

	//ÓÉÊÂ¼şµÄdata³ÉÔ±È¡µÃngx_connection_tÁ¬½Ó¡£×¢Òâ£¬Õâ¸öÁ¬½Ó²¢²»ÊÇNginxÓë¿Í»§¶ËµÄÁ¬½Ó£¬¶øÊÇNginxÓëÉÏÓÎ·şÎñÆ÷¼äµÄÁ¬½Ó
    c = ev->data;
    r = c->data;

    u = r->upstream;
	//×¢Òâ£¬ngx_http_request_t½á¹¹ÌåÖĞµÄÕâ¸öconnectionÁ¬½ÓÊÇ¿Í»§¶ËÓëNginx¼äµÄÁ¬½Ó
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

//¼ì²énginxÓëÏÂÓÎ¿Í»§¶ËÖ®¼äTCPÁ¬½ÓÊÇ·ñÕı³££¬Èç¹û³öÏÖ´íÎó£¬¾Í»áÁ¢¼´ÖÕÖ¹Á¬½Ó
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
	
	//Èç¹ûÁ¬½ÓÒÑ¾­³öÏÖ´íÎó¡£
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

	//n = 0Ê±Á¬½Ó¶Ï¿ª£¬ÎªÊ²Ã´»¹Ö±½Ó·µ»Ø
    if (ev->write && (n >= 0 || err == NGX_EAGAIN)) 
	{
        return;
    }

	/*Èç¹ûÊÇË®Æ½´¥·¢£¬ÇÒÊÂ¼şÔÚÊÂ¼şÇı¶¯»úÖÆÖĞ£¬Ôò½«ÆäÒÆ³ı*/
	/*·ÀÖ¹ÆäÖØ¸´²»Í£µÄ´¥·¢ÊÂ¼ş*/
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

	//Èç¹ûÓĞcache£¬²¢ÇÒºó¶ËµÄupstream»¹ÔÚ´¦Àí£¬Ôò´ËÊ±¼ÌĞø´¦Àíupstream£¬ºöÂÔ¶Ô¶ËµÄ´íÎó.
    if (u->peer.connection == NULL) 
	{
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_CLIENT_CLOSED_REQUEST);
    }
}

//½¨Á¢Á¬½Ó¡£
//upstream »úÖÆÓëÉÏÓÎ·şÎñÆ÷½¨Á¢ TCP Á¬½ÓÊ±£¬²ÉÓÃµÄÊÇ·Ç×èÈûÄ£Ê½µÄÌ×½Ó×Ö£¬¼´·¢ÆğÁ¬½ÓÇëÇóÖ®ºóÁ¢¼´·µ»Ø£¬²»¹ÜÁ¬½ÓÊÇ·ñ½¨Á¢³É¹¦£¬
//ÈôÃ»ÓĞÁ¢¼´½¨Á¢³É¹¦£¬ÔòĞèÔÚ epoll ÊÂ¼ş»úÖÆÖĞ¼àÌı¸ÃÌ×½Ó×Ö£¬µ±Ëü³öÏÖ¿ÉĞ´ÊÂ¼şÊ±£¬¾ÍËµÃ÷Á¬½ÓÒÑ¾­½¨Á¢³É¹¦ÁË
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

	 /* ÏòÉÏÓÎ·şÎñÆ÷·¢ÆğÁ¬½ÓÇëÇó£¬ĞèÒª×¢ÒâµÄÊÇ¸Ã·½·¨ÒÑ¾­½«ÏàÓ¦µÄÌ×½Ó×Ö×¢²áµ½epollÊÂ¼ş»úÖÆÀ´¼àÌı¶Á¡¢Ğ´ÊÂ¼ş */
    rc = ngx_event_connect_peer(&u->peer);

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream connect: %i", rc);

	/* ÏÂÃæ¸ù¾İrc²»Í¬·µ»ØÖµ½øĞĞ·ÖÎö......*/  

	/* Èô½¨Á¢Á¬½ÓÊ§°Ü£¬Ôòµ÷ÓÃngx_http_upstream_finalize_request¹Ø±Õµ±Ç°ÇëÇó£¬²¢return´Óµ±Ç°º¯Êı·µ»Ø */  
    if (rc == NGX_ERROR)
	{
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    u->state->peer = u->peer.name;

	 /*
     * Èô·µ»Ørc = NGX_BUSY£¬±íÊ¾µ±Ç°ÉÏÓÎ·şÎñÆ÷²»»îÔ¾£¬
     * Ôòµ÷ÓÃngx_http_upstream_nextÏòÉÏÓÎ·şÎñÆ÷ÖØĞÂ·¢ÆğÁ¬½Ó£¬
     * Êµ¼ÊÉÏ£¬¸Ã·½·¨×îÖÕ»¹ÊÇµ÷ÓÃngx_http_upstream_connect·½·¨£»
     * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
     */
    if (rc == NGX_BUSY) 
	{
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "no live upstreams");
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_NOLIVE);
        return;
    }

	/*
     * Èô·µ»Ørc = NGX_DECLINED£¬±íÊ¾µ±Ç°ÉÏÓÎ·şÎñÆ÷¸ºÔØ¹ıÖØ£¬
     * Ôòµ÷ÓÃngx_http_upstream_nextÏòÉÏÓÎÆäËü·şÎñÆ÷ÖØĞÂ·¢ÆğÁ¬½Ó£¬
     * Êµ¼ÊÉÏ£¬¸Ã·½·¨×îÖÕ»¹ÊÇµ÷ÓÃngx_http_upstream_connect·½·¨£»
     * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
     */
    if (rc == NGX_DECLINED) 
	{
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

    /* rc == NGX_OK || rc == NGX_AGAIN || rc == NGX_DONE */

    c = u->peer.connection;

    c->data = r;

	/* ÉèÖÃµ±Ç°Á¬½Óngx_connection_t ÉÏ¶Á¡¢Ğ´ÊÂ¼şµÄ»Øµ÷·½·¨ */
    c->write->handler = ngx_http_upstream_handler;
    c->read->handler = ngx_http_upstream_handler;
	 /* ÉèÖÃupstream»úÖÆµÄ¶Á¡¢Ğ´ÊÂ¼şµÄ»Øµ÷·½·¨ */
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
	 * ¼ì²éµ±Ç°ngx_http_upstream_t ½á¹¹µÄrequest_sent±êÖ¾Î»£¬
	 * Èô¸Ã±êÖ¾Î»Îª1£¬Ôò±íÊ¾ÒÑ¾­ÏòÉÏÓÎ·şÎñÆ÷·¢ËÍÇëÇó£¬¼´±¾´Î·¢ÆğÁ¬½ÓÊ§°Ü£»
	 * Ôòµ÷ÓÃngx_http_upstream_reinit·½·¨ÖØĞÂÏòÉÏÓÎ·şÎñÆ÷·¢ÆğÁ¬½Ó£»
	 */

    if (u->request_sent)
	{
		//ÖØĞÂ³õÊ¼»¯upstream
        if (ngx_http_upstream_reinit(r, u) != NGX_OK) {
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
    }
	//Èç¹ûrequest_body´æÔÚµÄ»°£¬±£´ærequest_body
    if (r->request_body && r->request_body->buf && r->request_body->temp_file && r == r->main)
    {
        /*
         * the r->request_body->buf can be reused for one request only,
         * the subrequests should allocate their own temporary bufs
         */

        u->output.free = ngx_alloc_chain_link(r->pool);
        if (u->output.free == NULL) {
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
		//±£´æµ½output
        u->output.free->buf = r->request_body->buf;
        u->output.free->next = NULL;
        u->output.allocated = 1;
		//ÖØÖÃrequest_body
        r->request_body->buf->pos = r->request_body->buf->start;
        r->request_body->buf->last = r->request_body->buf->start;
        r->request_body->buf->tag = u->output.tag;
    }

    u->request_sent = 0;

	/*
     * Èô·µ»Ørc = NGX_AGAIN£¬±íÊ¾µ±Ç°ÒÑ¾­·¢ÆğÁ¬½Ó£¬µ«ÊÇÃ»ÓĞÊÕµ½ÉÏÓÎ·şÎñÆ÷µÄÈ·ÈÏÓ¦´ğ±¨ÎÄ£¬¼´ÉÏÓÎÁ¬½ÓµÄĞ´ÊÂ¼ş²»¿ÉĞ´£¬
     * ÓÉÓÚĞ´ÊÂ¼şÒÑ¾­Ìí¼Óµ½epollÊÂ¼ş»úÖÆÖĞµÈ´ı¿ÉĞ´ÊÂ¼ş·¢Éú£¬
     * ËùÓĞÔÚÕâÀïÖ»Ğè½«µ±Ç°Á¬½ÓµÄĞ´ÊÂ¼şÌí¼Óµ½¶¨Ê±Æ÷»úÖÆÖĞ£¬¹ÜÀí³¬Ê±È·ÈÏÓ¦´ğ£»
     * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
     */
    if (rc == NGX_AGAIN)
	{
        ngx_add_timer(c->write, u->conf->connect_timeout);
        return;
    }

	/*
     * Èô·µ»ØÖµrc = NGX_OK£¬±íÊ¾Á¬½Ó³É¹¦½¨Á¢£¬
     * µ÷ÓÃ´Ë·½·¨ÏòÉÏÓÎ·şÎñÆ÷·¢ËÍÇëÇó */
#if (NGX_HTTP_SSL)

    if (u->ssl && c->ssl == NULL) 
	{
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

//·¢ËÍÇëÇó¡£
//µ± Nginx ÓëÉÏÓÎ·şÎñÆ÷³É¹¦½¨Á¢Á¬½ÓÖ®ºó£¬»áµ÷ÓÃ ngx_http_upstream_send_request ·½·¨·¢ËÍÇëÇó£¬
//ÈôÊÇ¸Ã·½·¨²»ÄÜÒ»´ÎĞÔ°ÑÇëÇóÄÚÈİ·¢ËÍÍê³ÉÊ±£¬ÔòĞèµÈ´ı epoll ÊÂ¼ş»úÖÆµÄĞ´ÊÂ¼ş·¢Éú£¬ÈôĞ´ÊÂ¼ş·¢Éú£¬
//Ôò»áµ÷ÓÃĞ´ÊÂ¼ş write_event_handler µÄ»Øµ÷·½·¨ ngx_http_upstream_send_request_handler ¼ÌĞø·¢ËÍÇëÇó£¬
//²¢ÇÒÓĞ¿ÉÄÜ»á¶à´Îµ÷ÓÃ¸ÃĞ´ÊÂ¼şµÄ»Øµ÷·½·¨£¬ Ö±µ½°ÑÇëÇó·¢ËÍÍê³É¡£
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
     * Èô±êÖ¾Î»request_sentÎª0£¬±íÊ¾»¹Î´ÏòÉÏÓÎ·¢ËÍÇëÇó£»
     * ÇÒ´ËÊ±µ÷ÓÃngx_http_upstream_test_connect·½·¨²âÊÔÊÇ·ñÓëÉÏÓÎ½¨Á¢Á¬½Ó£¬
     * Èô·µ»Ø·ÇNGX_OK£¬±êÖ¾µ±Ç°»¹Î´ÓëÉÏÓÎ·şÎñÆ÷³É¹¦½¨Á¢Á¬½Ó£¬ÔòĞèÒªµ÷ÓÃngx_http_upstream_next·½·¨³¢ÊÔÓëÏÂÒ»¸öÉÏÓÎ·şÎñÆ÷½¨Á¢Á¬½Ó£»
     * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
     */
    if (!u->request_sent && ngx_http_upstream_test_connect(c) != NGX_OK) {
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

    c->log->action = "sending request to upstream";

    rc = ngx_http_upstream_send_request_body(r, u, do_write);

	/*
     * Èô·µ»ØÖµrc=NGX_ERROR£¬±íÊ¾µ±Ç°Á¬½ÓÉÏ³ö´í£¬
     * ½«´íÎóĞÅÏ¢´«µİ¸øngx_http_upstream_next·½·¨£¬
     * ¸Ã·½·¨¸ù¾İ´íÎóĞÅÏ¢¾ö¶¨ÊÇ·ñÖØĞÂÏòÉÏÓÎ·şÎñÆ÷·¢ÆğÁ¬½Ó£»
     * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
     */
    if (rc == NGX_ERROR) 
	{
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) 
	{
        ngx_http_upstream_finalize_request(r, u, rc);
        return;
    }

	/*
     * Èô·µ»ØÖµrc = NGX_AGAIN£¬±íÊ¾ÇëÇóÊı¾İ²¢Î´ÍêÈ«·¢ËÍ£¬
     * ¼´ÓĞÊ£ÓàµÄÇëÇóÊı¾İ±£´æÔÚoutputÖĞ£¬µ«´ËÊ±£¬Ğ´ÊÂ¼şÒÑ¾­²»¿ÉĞ´£¬
     * Ôòµ÷ÓÃngx_add_timer·½·¨°Ñµ±Ç°Á¬½ÓÉÏµÄĞ´ÊÂ¼şÌí¼Óµ½¶¨Ê±Æ÷»úÖÆ£¬
     * ²¢µ÷ÓÃngx_handle_write_event·½·¨½«Ğ´ÊÂ¼ş×¢²áµ½epollÊÂ¼ş»úÖÆÖĞ£»
     * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
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
	 * Èô·µ»ØÖµ rc = NGX_OK£¬±íÊ¾ÒÑ¾­·¢ËÍÍêÈ«²¿ÇëÇóÊı¾İ£¬×¼±¸½ÓÊÕÀ´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦±¨ÎÄ£¬ÔòÖ´ĞĞÒÔÏÂ³ÌĞò£»
	 */

  	//¼ì²éµ±Ç°Á¬½ÓÉÏĞ´ÊÂ¼şµÄ±êÖ¾Î»timer_setÊÇ·ñÎª1£¬Èô¸Ã±êÖ¾Î»Îª1£¬ÔòĞè°ÑĞ´ÊÂ¼ş´Ó¶¨Ê±Æ÷»úÖÆÖĞÒÆ³ı
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

	//ÓÉÓÚ´ËÊ±ÇëÇóÒÔÈ«²¿·¢ËÍµ½ÉÏÓÎ·şÎñÆ÷ÁË£¬ÖØÖÃwrite_event_handlerÎªngx_http_upstream_dummy_handler
	//¸Ã·½·¨²»×öÈÎºÎÊÂÇé£¬ÕâÑù¼´Ê¹ÓëÉÏÓÎ·şÎñÆ÷TCPÁ¬½ÓÉÏÔÙ´ÎÓĞ¿ÉĞ´ÊÂ¼şÊ±Ò²²»»áÓĞÈÎºÎ¶¯×÷·¢Éú
	//²¢°ÑĞ´ÊÂ¼ş×¢²áµ½epollÊÂ¼ş»úÖÆÖĞ£»
    u->write_event_handler = ngx_http_upstream_dummy_handler;

    if (ngx_handle_write_event(c->write, 0) != NGX_OK) 
	{
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

	//½«µ±Ç°Á¬½ÓÉÏ¶ÁÊÂ¼şÌí¼Óµ½¶¨Ê±Æ÷»úÖÆÖĞ£¬¼ì²é½ÓÊÕÏìÓ¦ÊÇ·ñ³¬Ê±
    ngx_add_timer(c->read, u->conf->read_timeout);

	 /*
     * Èô´ËÊ±£¬¶ÁÊÂ¼şÒÑ¾­×¼±¸¾ÍĞ÷£¬
     * Ôòµ÷ÓÃngx_http_upstream_process_header·½·¨¿ªÊ¼½ÓÊÕ²¢´¦ÀíÏìÓ¦Í·²¿£»
     * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
     */
    if (c->read->ready)
	{
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
     * µ÷ÓÃngx_output_chain·½·¨ÏòÉÏÓÎ·¢ËÍ±£´æÔÚrequest_bufsÁ´±íÖĞµÄÇëÇóÊı¾İ£»
     * ÖµµÃ×¢ÒâµÄÊÇ¸Ã·½·¨µÄµÚ¶ş¸ö²ÎÊı¿ÉÒÔÊÇNULLÒ²¿ÉÒÔÊÇrequest_bufs£¬ÄÇÔõÃ´À´Çø·ÖÄØ£¿
     * ÈôÊÇµÚÒ»´Îµ÷ÓÃ¸Ã·½·¨·¢ËÍrequest_bufsÁ´±íÖĞµÄÇëÇóÊı¾İÊ±£¬request_sent±êÖ¾Î»Îª0£¬
     * ´ËÊ±£¬µÚ¶ş¸ö²ÎÊı×ÔÈ»¾ÍÊÇrequest_bufsÁË£¬ÄÇÃ´ÎªÊ²Ã´»áÓĞNULL×÷Îª²ÎÊıµÄÇé¿öÄØ£¿
     * µ±ÔÚµÚÒ»´Îµ÷ÓÃ¸Ã·½·¨Ê±£¬²¢²»ÄÜÒ»´ÎĞÔ°ÑËùÓĞrequest_bufsÖĞµÄÊı¾İ·¢ËÍÍê±ÏÊ±£¬
     * ´ËÊ±£¬»á°ÑÊ£ÓàµÄÊı¾İ±£´æÔÚoutput½á¹¹ÀïÃæ£¬²¢°Ñ±êÖ¾Î»request_sentÉèÖÃÎª1£¬
     * Òò´Ë£¬ÔÙ´Î·¢ËÍÇëÇóÊı¾İÊ±£¬²»ÓÃÖ¸¶¨request_bufs²ÎÊı£¬ÒòÎª´ËÊ±Ê£ÓàÊı¾İÒÑ¾­±£´æÔÚoutputÖĞ£»
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

		//ÉèÖÃÉÏÓÎÁ¬½ÓTCP_NODELAYÑ¡Ïî
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
		//ÎªÊ²Ã´???
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

	//»ñÈ¡ÓëÉÏÓÎ·şÎñÆ÷±íÊ¾Á¬½ÓµÄngx_connection_t½á¹¹Ìå
    c = u->peer.connection;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream send request handler");

	//¼ì²éÊÇ·ñÏòÉÏÓÎ·şÎñÆ÷·¢ËÍµÄÇëÇóÒÑ¾­³¬Ê±
	//ÈôĞ´ÊÂ¼ş³¬Ê±£¬Ôò½«³¬Ê±´íÎó´«µİ¸ø·½·¨£¬¸Ã·½·¨½«»á¸ù¾İÔÊĞíµÄ´íÎóÖØÁ¬²ßÂÔ¾ö¶¨:ÖØĞÂ·¢ÆğÁ¬½ÓÖ´ĞĞupstreamÇëÇó£¬»òÕß½áÊøupstreamÇëÇó¡£
    if (c->write->timedout)
	{
		//Ö´ĞĞ³¬Ê±ÖØÁ¬»úÖÆ 
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

	//¼ì²é header_sent ±êÖ¾Î»ÊÇ·ñÎª 1£¬±íÊ¾ÒÑ¾­½ÓÊÕµ½À´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦Í·²¿£¬Ôò²»ĞèÒª(²»Ó¦¸Ã)¼ÌĞøÏòÉÏÓÎ·¢ËÍÇëÇó£¬
	//ËùÒÔ°ÑĞ´ÊÂ¼şµÄ»Øµ÷·½·¨ÉèÖÃÎªÈÎºÎ¹¤×÷¶¼Ã»ÓĞ×öµÄ ngx_http_upstream_dummy_handler£¬Í¬Ê±½«Ğ´ÊÂ¼ş×¢²áµ½ epoll ÊÂ¼ş»úÖÆÖĞ£¬
	//²¢return ´Óµ±Ç°º¯Êı·µ»Ø£»
    if (u->header_sent) 
	{
        u->write_event_handler = ngx_http_upstream_dummy_handler;
        (void) ngx_handle_write_event(c->write, 0);

		//ÒòÎª²»´æÔÚ¼ÌĞø·¢ËÍÇëÇóµ½ÉÏÓÎµÄ¿ÉÄÜ£¬ËùÒÔÖ±½Ó·µ»Ø
        return;
    }
	
	//ÈôÃ»ÓĞ½ÓÊÕÀ´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦Í·²¿£¬ÔòĞèÏòÉÏÓÎ·şÎñÆ÷·¢ËÍÇëÇóÊı¾İ  
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

//½ÓÊÕÏìÓ¦ --½ÓÊÕÏìÓ¦Í·²¿
//µ± Nginx ÒÑ¾­ÏòÉÏÓÎ·¢ËÍÇëÇó£¬×¼±¸¿ªÊ¼½ÓÊÕÀ´×ÔÉÏÓÎµÄÏìÓ¦Í·²¿£¬ÓÉ·½·¨ ngx_http_upstream_process_header ÊµÏÖ£¬¸Ã·½·¨½ÓÊÕ²¢½âÎöÏìÓ¦Í·²¿
static void
ngx_http_upstream_process_header(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ssize_t            n;
    ngx_int_t          rc;
    ngx_connection_t  *c;

    c = u->peer.connection;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream process header");

    c->log->action = "reading response header from upstream";

	//¼ì²é¶ÁÊÂ¼şÊÇ·ñ³¬Ê±
	//¼ì²éÉÏÓÎÁ¬½ÓÉÏµÄ¶ÁÊÂ¼şÊÇ·ñ³¬Ê±£¬Èô±êÖ¾Î»timedoutÎª 1£¬Ôò±íÊ¾³¬Ê±£¬°Ñ³¬Ê±´íÎó´«µİ¸øngx_http_upstream_next·½·¨£¬
	//¸Ã·½·¨¸ù¾İÔÊĞíµÄ´íÎó½øĞĞÖØÁ¬½Ó²ßÂÔ£¬²¢ return ´Óµ±Ç°º¯Êı·µ»Ø
    if (c->read->timedout) {
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_TIMEOUT);
        return;
    }

	//¼ì²éÊÇ·ñ·¢ËÍÁËÇëÇó¸øÉÏÓÎ·şÎñÆ÷

	/*
     * Èô±êÖ¾Î»request_sentÎª0£¬±íÊ¾»¹Î´·¢ËÍÇëÇó£»
     * ÇÒngx_http_upstream_test_connect·½·¨·µ»Ø·ÇNGX_OK£¬±êÖ¾µ±Ç°»¹Î´ÓëÉÏÓÎ·şÎñÆ÷³É¹¦½¨Á¢Á¬½Ó£»
     * ÔòĞèÒªµ÷ÓÃngx_http_upstream_next·½·¨³¢ÊÔÓëÏÂÒ»¸öÉÏÓÎ·şÎñÆ÷½¨Á¢Á¬½Ó£»
     * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
     */
    if (!u->request_sent && ngx_http_upstream_test_connect(c) != NGX_OK) {
		//»¹Ã»ÓĞ·¢ËÍÇëÇóµ½ÉÏÓÎ·şÎñÆ÷¾ÍÊÕµ½ÁËÀ´×ÔÉÏÓÎµÄÏìÓ¦£¬²»·ûºÏupstreamµÄÉè¼Æ³¡¾°
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

	//¼ì²é ngx_http_upstream_t ½á¹¹ÌåÖĞ½ÓÊÕÏìÓ¦Í·²¿µÄ buffer »º³åÇøÊÇ·ñÓĞÄÚ´æ¿Õ¼äÒÔ±ã½ÓÊÕÏìÓ¦Í·²¿£¬
	//Èô buffer.start Îª NULL£¬±íÊ¾¸Ã»º³åÇøÎª¿Õ£¬ÔòĞèµ÷ÓÃ ngx_palloc ·½·¨·ÖÅäÄÚ´æ£¬
	//¸ÃÄÚ´æ´óĞ¡ buffer_size ÓÉ ngx_http_upstream_conf_t ÅäÖÃ½á¹¹ÌåµÄ buffer_size ³ÉÔ±Ö¸¶¨
    if (u->buffer.start == NULL) {
        u->buffer.start = ngx_palloc(r->pool, u->conf->buffer_size);
        if (u->buffer.start == NULL) {
            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        u->buffer.pos = u->buffer.start;
        u->buffer.last = u->buffer.start;
        u->buffer.end = u->buffer.start + u->conf->buffer_size;
		/* ±íÊ¾¸Ã»º³åÇøÄÚ´æ¿É±»¸´ÓÃ¡¢Êı¾İ¿É±»¸Ä±ä */
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
		/* µ÷ÓÃrecv·½·¨´Óµ±Ç°Á¬½ÓÉÏ¶ÁÈ¡ÏìÓ¦Í·²¿Êı¾İ */
        n = c->recv(c, u->buffer.last, u->buffer.end - u->buffer.last);
		/* ÏÂÃæ¸ù¾İ recv ·½·¨²»Í¬·µ»ØÖµ n ½øĞĞÅĞ¶Ï */
		
		/*
         * Èô·µ»ØÖµ n = NGX_AGAIN£¬±íÊ¾¶ÁÊÂ¼şÎ´×¼±¸¾ÍĞ÷£¬
         * ĞèµÈ´ıÏÂ´Î¶ÁÊÂ¼ş±»´¥·¢Ê±¼ÌĞø½ÓÊÕÏìÓ¦Í·²¿£¬
         * ¼´½«¶ÁÊÂ¼ş×¢²áµ½epollÊÂ¼ş»úÖÆÖĞ£¬µÈ´ı¿É¶ÁÊÂ¼ş·¢Éú£»
         * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
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
         * Èô·µ»ØÖµ n = NGX_ERROR »ò n = 0£¬Ôò±íÊ¾ÉÏÓÎ·şÎñÆ÷ÒÑ¾­Ö÷¶¯¹Ø±ÕÁ¬½Ó£»
         * ´ËÊ±£¬µ÷ÓÃngx_http_upstream_next·½·¨¾ö¶¨ÊÇ·ñÖØĞÂ·¢ÆğÁ¬½Ó£»
         * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
         */
        if (n == NGX_ERROR || n == 0) {
            ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
            return;
        }

		 /* Èô·µ»ØÖµ n ´óÓÚ 0£¬±íÊ¾ÒÑ¾­½ÓÊÕµ½ÏìÓ¦Í·²¿ */
		
        u->buffer.last += n;

#if 0
        u->valid_header_in = 0;

        u->peer.cached = 0;
#endif
		//HTTPÄ£¿é»áÍ¨¹ıprocess_header·½·¨½âÎö°üÍ·(½«½âÎö³öµÄÖµÉèÖÃµ½ngx_http_upstream½á¹¹ÌåµÄheaders_in³ÉÔ±ÖĞ)
		//²¢¸ù¾İ¸Ã·½·¨·µ»ØÖµ½øĞĞ²»Í¬µÄÅĞ¶Ï£»
        rc = u->process_header(r);

		  /*
         * Èô·µ»ØÖµ rc = NGX_AGAIN£¬±íÊ¾½ÓÊÕµ½µÄÏìÓ¦Í·²¿²»ÍêÕû£¬ĞèµÈ´ıÏÂ´Î¶ÁÊÂ¼ş±»´¥·¢Ê±¼ÌĞø½ÓÊÕÏìÓ¦Í·²¿£»
         * continue¼ÌĞø½ÓÊÕÏìÓ¦£»
         */
        if (rc == NGX_AGAIN) {
			//¼ì²é½ÓÊÕ»º³åÇø buffer ÊÇ·ñ»¹ÓĞÊ£ÓàµÄÄÚ´æ¿Õ¼ä£¬Èô»º³åÇøÃ»ÓĞÊ£ÓàµÄÄÚ´æ¿Õ¼ä£¬±íÊ¾½ÓÊÕµ½µÄÏìÓ¦Í·²¿¹ı´ó£¬
			//´ËÊ±µ÷ÓÃ ngx_http_upstream_next ·½·¨ÖØĞÂ½¨Á¢Á¬½Ó£¬²¢ return ´Óµ±Ç°º¯Êı·µ»Ø£»Èô»º³åÇø»¹ÓĞÊ£ÓàµÄÄÚ´æ¿Õ¼ä£¬
			//Ôòcontinue ¼ÌĞø½ÓÊÕÏìÓ¦Í·²¿
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
     * Èô·µ»ØÖµ rc = NGX_HTTP_UPSTREAM_INVALID_HEADER£¬
     * Ôò±íÊ¾½ÓÊÕµ½µÄÏìÓ¦Í·²¿ÊÇ·Ç·¨µÄ£¬
     * µ÷ÓÃngx_http_upstream_next·½·¨¾ö¶¨ÊÇ·ñÖØĞÂ·¢ÆğÁ¬½Ó£»
     * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
     */
    if (rc == NGX_HTTP_UPSTREAM_INVALID_HEADER) {
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_INVALID_HEADER);
        return;
    }

	//Èô rc = NGX_ERROR£¬±íÊ¾Á¬½Ó³ö´í£¬´ËÊ±µ÷ÓÃ ngx_http_upstream_finalize_request ·½·¨½áÊøÇëÇó£¬²¢ return ´Óµ±Ç°º¯Êı·µ»Ø
    if (rc == NGX_ERROR) {
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    /* rc == NGX_OK */

	/*Èô·µ»ØÖµ rc = NGX_OK£¬±íÊ¾³É¹¦½âÎöµ½ÍêÕûµÄÏìÓ¦Í·²¿£»*/
    u->state->header_time = ngx_current_msec - u->state->response_time;

    if (u->headers_in.status_n >= NGX_HTTP_SPECIAL_RESPONSE) {
        if (ngx_http_upstream_test_next(r, u) == NGX_OK)
		{
            return;
        }

        if (ngx_http_upstream_intercept_errors(r, u) == NGX_OK) 
		{
            return;
        }
    }
	
	//µ÷ÓÃngx_http_upstream_process_headers·½·¨´¦ÀíÒÑ½âÎö´¦ÀíµÄÏìÓ¦Í·²¿ 
	//¸Ã·½·¨½«»á°ÑÒÑ¾­½âÎö³öµÄÍ·²¿ÉèÖÃµ½ngx_http_request_tÇëÇó½á¹¹ÌåµÄheaders_out³ÉÔ±ÖĞ£¬
	//ÕâÑùÔÚµ÷ÓÃngx_http_send_header·½·¨·¢ËÍÏìÓ¦°üÍ·¸ø¿Í»§¶ËÊ±½«»á·¢ËÍÕâĞ©ÉèÖÃºÃÁËµÄÍ·²¿
    if (ngx_http_upstream_process_headers(r, u) != NGX_OK) {
        return;
    }
	
//upstream ´¦ÀíÉÏÓÎÏìÓ¦°üÌåµÄÈıÖÖ·½Ê½:	
//µ±ÇëÇó½á¹¹Ìå ngx_http_request_t ÖĞµÄ³ÉÔ±subrequest_in_memory ±êÖ¾Î»Îª 1 Ê±£¬upstream ²»×ª·¢ÏìÓ¦°üÌåµ½ÏÂÓÎ£¬²¢ÓÉHTTP Ä£¿éÊµÏÖµÄinput_filter() ·½·¨´¦Àí°üÌå£»
//µ±ÇëÇó½á¹¹Ìå ngx_http_request_t ÖĞµÄ³ÉÔ±subrequest_in_memory ±êÖ¾Î»Îª 0 Ê±£¬ÇÒngx_http_upstream_conf_t ÅäÖÃ½á¹¹ÌåÖĞµÄ³ÉÔ±buffering ±êÖ¾Î»Îª 1 Ê±£¬upstream ½«¿ªÆô¸ü¶àµÄÄÚ´æºÍ´ÅÅÌÎÄ¼şÓÃÓÚ»º´æÉÏÓÎµÄÏìÓ¦°üÌå£¨´ËÊ±£¬ÉÏÓÎÍøËÙ¸ü¿ì£©£¬²¢×ª·¢ÏìÓ¦°üÌå£»
//µ±ÇëÇó½á¹¹Ìå ngx_http_request_t ÖĞµÄ³ÉÔ±subrequest_in_memory ±êÖ¾Î»Îª 0 Ê±£¬ÇÒngx_http_upstream_conf_t ÅäÖÃ½á¹¹ÌåÖĞµÄ³ÉÔ±buffering ±êÖ¾Î»Îª 0 Ê±£¬upstream ½«Ê¹ÓÃ¹Ì¶¨´óĞ¡µÄ»º³åÇøÀ´×ª·¢ÏìÓ¦°üÌå£»

	/*
     * ¼ì²éngx_http_request_t ½á¹¹ÌåµÄsubrequest_in_memory³ÉÔ±¾ö¶¨ÊÇ·ñ×ª·¢ÏìÓ¦¸øÏÂÓÎ·şÎñÆ÷£»
     * Èô¸Ã±êÖ¾Î»Îª0£¬ÔòĞèµ÷ÓÃngx_http_upstream_send_response·½·¨×ª·¢ÏìÓ¦¸øÏÂÓÎ·şÎñÆ÷£»
     * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
     */
    if (!r->subrequest_in_memory) {
		//×ª·¢ÏìÓ¦¸ø¿Í»§¶Ë
        ngx_http_upstream_send_response(r, u);
        return;
    }
	/* Èô²»ĞèÒª×ª·¢ÏìÓ¦£¬Ôòµ÷ÓÃngx_http_upstream_tÖĞµÄinput_filter·½·¨´¦ÀíÏìÓ¦°üÌå */
    /* subrequest content in memory */

	//¼ì²éHTTPÄ£¿éÊÇ·ñÊÇÊµÏÖÁËÓÃÓÚ´¦Àí°üÌåµÄinput_filter·½·¨£¬
	//Èç¹ûÃ»ÓĞÊµÏÖ£¬ÔòÊ¹ÓÃupstream¶¨ÒåµÄÄ¬ÈÏ·½·¨´úÌæÖ®
    if (u->input_filter == NULL) 
	{
        u->input_filter_init = ngx_http_upstream_non_buffered_filter_init;
        u->input_filter = ngx_http_upstream_non_buffered_filter;
        u->input_filter_ctx = r;
    }

	    /*
     * µ÷ÓÃinput_filter_init·½·¨Îª´¦Àí°üÌå×ö³õÊ¼»¯¹¤×÷£»
     */
    if (u->input_filter_init(u->input_filter_ctx) == NGX_ERROR)
	{
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }


	/*
     * ¼ì²é½ÓÊÕ»º³åÇøÊÇ·ñÓĞÊ£ÓàµÄÏìÓ¦Êı¾İ£»
     * ÒòÎªÏìÓ¦Í·²¿ÒÑ¾­½âÎöÍê±Ï£¬Èô½ÓÊÕ»º³åÇø»¹ÓĞÎ´±»½âÎöµÄÊ£ÓàÊı¾İ£¬
     * Ôò¸ÃÊı¾İ¾ÍÊÇÏìÓ¦°üÌå£»
     */
     
    n = u->buffer.last - u->buffer.pos;
	/*
     * Èô½ÓÊÕ»º³åÇøÓĞÊ£ÓàµÄÏìÓ¦°üÌå£¬µ÷ÓÃinput_filter·½·¨¿ªÊ¼´¦ÀíÒÑ½ÓÊÕµ½ÏìÓ¦°üÌå£»
     */
    if (n)
	{
        u->buffer.last = u->buffer.pos;

        u->state->response_length += n;
		 /* µ÷ÓÃinput_filter·½·¨´¦ÀíÏìÓ¦°üÌå */
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

	/* ÉèÖÃupstream»úÖÆµÄ¶ÁÊÂ¼ş»Øµ÷·½·¨read_event_handlerÎªngx_http_upstream_process_body_in_memory */
    u->read_event_handler = ngx_http_upstream_process_body_in_memory;

	/* µ÷ÓÃngx_http_upstream_process_body_in_memory·½·¨¿ªÊ¼´¦ÀíÏìÓ¦°üÌå */
    ngx_http_upstream_process_body_in_memory(r, u);
}


static ngx_int_t
ngx_http_upstream_test_next(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_uint_t                 status;
    ngx_http_upstream_next_t  *un;

    status = u->headers_in.status_n;

    for (un = ngx_http_upstream_next_errors; un->status; un++) 
	{

        if (status != un->status) 
		{
            continue;
        }

        if (u->peer.tries > 1 && (u->conf->next_upstream & un->mask)) {
            ngx_http_upstream_next(r, u, un->mask);
            return NGX_OK;
        }

#if (NGX_HTTP_CACHE)

        if (u->cache_status == NGX_HTTP_CACHE_EXPIRED
            && (u->conf->cache_use_stale & un->mask))
        {
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

    if (status == NGX_HTTP_NOT_MODIFIED
        && u->cache_status == NGX_HTTP_CACHE_EXPIRED
        && u->conf->cache_revalidate)
    {
        time_t     now, valid;
        ngx_int_t  rc;

        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http upstream not modified");

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

            if (status == NGX_HTTP_UNAUTHORIZED
                && u->headers_in.www_authenticate)
            {
                h = ngx_list_push(&r->headers_out.headers);

                if (h == NULL) {
                    ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
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

            hh = ngx_hash_find(&umcf->headers_in_hash, h[i].hash,
                               h[i].lowcase_key, h[i].key.len);

            if (hh && hh->redirect) {
                if (hh->copy_handler(r, &h[i], hh->conf) != NGX_OK) {
                    ngx_http_finalize_request(r,
                                              NGX_HTTP_INTERNAL_SERVER_ERROR);
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

        if (ngx_hash_find(&u->conf->hide_headers_hash, h[i].hash,
                          h[i].lowcase_key, h[i].key.len))
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

//½ÓÊÕÏìÓ¦ --½ÓÊÕÏìÓ¦°üÌå
//½ÓÊÕ²¢½âÎöÏìÓ¦°üÌåÓÉ ngx_http_upstream_process_body_in_memory ·½·¨ÊµÏÖ£»
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
     * ¼ì²é¶ÁÊÂ¼ş±êÖ¾Î»timedoutÊÇ·ñ³¬Ê±£¬Èô¸Ã±êÖ¾Î»Îª1£¬±íÊ¾ÏìÓ¦ÒÑ¾­³¬Ê±£»
     * Ôòµ÷ÓÃngx_http_upstream_finalize_request·½·¨½áÊøÇëÇó£»
     * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
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
		/* ¼ì²éµ±Ç°½ÓÊÕ»º³åÇøÊÇ·ñÊ£ÓàµÄÄÚ´æ¿Õ¼ä */
        size = b->end - b->last;

		/*
         * Èô½ÓÊÕ»º³åÇø²»´æÔÚ¿ÕÏĞµÄÄÚ´æ¿Õ¼ä£¬
         * Ôòµ÷ÓÃngx_http_upstream_finalize_request·½·¨½áÊøÇëÇó£»
         * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
         */
        if (size == 0) 
		{
            ngx_log_error(NGX_LOG_ALERT, c->log, 0, "upstream buffer is too small to read response");
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }

		       /*
         * Èô½ÓÊÕ»º³åÇøÓĞ¿ÉÓÃµÄÄÚ´æ¿Õ¼ä£¬
         * Ôòµ÷ÓÃrecv·½·¨¿ªÊ¼½ÓÊÕÏìÓ¦°üÌå£»
         */
        n = c->recv(c, b->last, size);

		/*
         * Èô·µ»ØÖµ n = NGX_AGAIN£¬±íÊ¾µÈ´ıÏÂÒ»´Î´¥·¢¶ÁÊÂ¼şÔÙ½ÓÊÕÏìÓ¦°üÌå£»
         */
        if (n == NGX_AGAIN)
		{
            break;
        }

		/*
         * Èô·µ»ØÖµn = 0(±íÊ¾ÉÏÓÎ·şÎñÆ÷Ö÷¶¯¹Ø±ÕÁ¬½Ó)£¬»òn = NGX_ERROR(±íÊ¾³ö´í)£»
         * Ôòµ÷ÓÃngx_http_upstream_finalize_request·½·¨½áÊøÇëÇó£»
         * ²¢return´Óµ±Ç°º¯Êı·µ»Ø£»
         */
        if (n == 0 || n == NGX_ERROR) 
		{
            ngx_http_upstream_finalize_request(r, u, n);
            return;
        }
		/* Èô·µ»ØÖµ n ´óÓÚ0£¬±íÊ¾³É¹¦¶ÁÈ¡µ½ÏìÓ¦°üÌå */
		
        u->state->response_length += n;

		/* µ÷ÓÃinput_filter·½·¨´¦Àí±¾´Î½ÓÊÕµ½µÄÏìÓ¦°üÌå */
        if (u->input_filter(u->input_filter_ctx, n) == NGX_ERROR)
		{
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }

		/* ¼ì²é¶ÁÊÂ¼şµÄready±êÖ¾Î»£¬ÈôÎª1£¬¼ÌĞø¶ÁÈ¡ÏìÓ¦°üÌå */
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
     * Èô¶ÁÊÂ¼şµÄready±êÖ¾Î»Îª0£¬±íÊ¾¶ÁÊÂ¼şÎ´×¼±¸¾ÍĞ÷£¬
     * Ôò½«¶ÁÊÂ¼ş×¢²áµ½epollÊÂ¼ş»úÖÆÖĞ£¬Ìí¼Óµ½¶¨Ê±Æ÷»úÖÆÖĞ£»
     * ¶ÁÊÂ¼şµÄ»Øµ÷·½·¨²»¸Ä±ä£¬¼´ÒÀ¾ÉÎªngx_http_upstream_process_body_in_memory£»
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

//×ª·¢ÏìÓ¦
static void
ngx_http_upstream_send_response(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    int                        tcp_nodelay;
    ssize_t                    n;
    ngx_int_t                  rc;
    ngx_event_pipe_t          *p;
    ngx_connection_t          *c;
    ngx_http_core_loc_conf_t  *clcf;

	 /* µ÷ÓÃngx_http_send_hander·½·¨ÏòÏÂÓÎ·¢ËÍÏìÓ¦Í·²¿ */
    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->post_action) {
        ngx_http_upstream_finalize_request(r, u, rc);
        return;
    }

	/* ½«±êÖ¾Î»header_sentÉèÖÃÎª1£¬±íÊ¾ÒÑ¾­×ª·¢ÏìÓ¦Í·²¿ */
    u->header_sent = 1;

    if (u->upgrade) {
        ngx_http_upstream_upgrade(r, u);
        return;
    }
	
	/* »ñÈ¡NginxÓëÏÂÓÎÖ®¼äµÄTCPÁ¬½Ó */
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

	//Èç¹û¿Í»§¶ËµÄÇëÇóÖĞÓĞHTTP°üÌå£¬¶øÇÒÔø¾­µ÷ÓÃ¹ıngx_http_read_client_request_body·½·¨½ÓÊÕHTTP°üÌå
	//²¢°Ñ°üÌå´æ·ÅÔÚÀïÁÙÊ±ÎÄ¼şÖĞ£¬ÕâÊ±¾Í»áµ÷ÓÃngx_pool_run_cleanup_file·½·¨ÇåÀíÁÙÊ±ÎÄ¼ş¡£ÒòÎªÉÏÓÎ·şÎñÆ÷
	//·¢ËÍÏìÓ¦Ê±¿ÉÄÜ»áÊ¹ÓÃµ½ÁÙÊ±ÎÄ¼ş£¬Ö®ºóÊÕµ½ÏìÓ¦½âÎöÏìÓ¦°üÍ·Ê±Ò²²»¿ÉÒÔÇåÀíÁÙÊ±ÎÄ¼ş£¬¶øÒ»µ©¿ªÊ¼ÏòÏÂÓÎ
	//¿Í»§¶Ë×ª·¢HTTPÏìÓ¦Ê±£¬ÔòÒâÎ¶×Å¿Ï¶¨²»»áÔÚĞèÒª¿Í»§¶ËÇëÇóµÄ°üÌåÁË£¬ÕâÊ±¿ÉÒÔ¹Ø±Õ¡¢×ªÒÆ¡¢»òÕßÉ¾³ıÁÙÊ±
	//ÎÄ¼ş£¬¾ßÌå¶¯×÷ÓÉHTTPÄ£¿éÊµÏÖµÄhandler»Øµ÷·½·¨¾ö¶¨
	 /* ÈôÁÙÊ±ÎÄ¼ş±£´æ×ÅÇëÇó°üÌå£¬Ôòµ÷ÓÃngx_pool_run_cleanup_file·½·¨ÇåÀíÁÙÊ±ÎÄ¼şµÄÇëÇó°üÌå */
    if (r->request_body && r->request_body->temp_file) {
        ngx_pool_run_cleanup_file(r->pool, r->request_body->temp_file->file.fd);
        r->request_body->temp_file->file.fd = NGX_INVALID_FILE;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
	
	 /*
     * Èô±êÖ¾Î»bufferingÎª0£¬×ª·¢ÏìÓ¦Ê±ÒÔÏÂÓÎ·şÎñÆ÷ÍøËÙÓÅÏÈ£»
     * ¼´Ö»Ğè·ÖÅä¹Ì¶¨µÄÄÚ´æ¿é´óĞ¡À´½ÓÊÕÀ´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦²¢×ª·¢£¬
     * µ±¸ÃÄÚ´æ¿éÒÑÂú£¬ÔòÔİÍ£½ÓÊÕÀ´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦Êı¾İ£¬
     * µÈ´ı°ÑÄÚ´æ¿éµÄÏìÓ¦Êı¾İ×ª·¢¸øÏÂÓÎ·şÎñÆ÷ºóÓĞÊ£ÓàÄÚ´æ¿Õ¼äÔÙ¼ÌĞø½ÓÊÕÏìÓ¦£»
     */
    if (!u->buffering) 
	{
		//¼ì²éHTTPÄ£¿éÊÇ·ñÊÇÊµÏÖÁËÓÃÓÚ´¦Àí°üÌåµÄinput_filter·½·¨£¬
		//Èç¹ûÃ»ÓĞÊµÏÖ£¬ÔòÊ¹ÓÃupstream¶¨ÒåµÄÄ¬ÈÏ·½·¨´úÌæÖ®
        if (u->input_filter == NULL) 
		{
            u->input_filter_init = ngx_http_upstream_non_buffered_filter_init;
            u->input_filter = ngx_http_upstream_non_buffered_filter;
            u->input_filter_ctx = r;
        }

		/*
         * ÉèÖÃngx_http_upstream_t½á¹¹ÌåÖĞ¶ÁÊÂ¼şµÄ»Øµ÷·½·¨Îªngx_http_upstream_non_buffered_upstream£¬(¼´¶ÁÈ¡ÉÏÓÎÏìÓ¦µÄ·½·¨)£»
         * ÉèÖÃµ±Ç°ÇëÇóngx_http_request_t½á¹¹ÌåÖĞĞ´ÊÂ¼şµÄ»Øµ÷·½·¨Îªngx_http_upstream_process_non_buffered_downstream£¬(¼´×ª·¢ÏìÓ¦µ½ÏÂÓÎµÄ·½·¨)£»
         */
        u->read_event_handler = ngx_http_upstream_process_non_buffered_upstream;
        r->write_event_handler = ngx_http_upstream_process_non_buffered_downstream;

        r->limit_rate = 0;

		/* µ÷ÓÃinput_filter_initÎªinput_filter·½·¨´¦ÀíÏìÓ¦°üÌå×ö³õÊ¼»¯¹¤×÷ */
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

		//Èç¹û½âÎöÍê°üÍ·ºó»º³åÇøÖĞ»¹ÓĞ¶àÓàµÄ×Ö·û£¬Ôò±íÊ¾»¹½ÓÊÕµ½ÁË°üÌå£¬ÕâÊ±½«µ÷ÓÃinput_filter·½·¨µÚÒ»´Î´¦Àí½ÓÊÕµ½µÄ°üÌå
		/* ¼ì²é½âÎöÍêÏìÓ¦Í·²¿ºó½ÓÊÕ»º³åÇøbufferÊÇ·ñÒÑ½ÓÊÕÁËÏìÓ¦°üÌå */
        n = u->buffer.last - u->buffer.pos;
		
        if (n) {
            u->buffer.last = u->buffer.pos;

            u->state->response_length += n;
			/* µ÷ÓÃinput_filter·½·¨¿ªÊ¼´¦ÀíÏìÓ¦°üÌå */
            if (u->input_filter(u->input_filter_ctx, n) == NGX_ERROR) 
			{
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }
			//µ÷ÓÃ¸Ã·½·¨°Ñ±¾´Î½ÓÊÕµ½µÄÏìÓ¦°üÌå×ª·¢¸øÏÂÓÎ¿Í»§¶Ë
            ngx_http_upstream_process_non_buffered_downstream(r);

        } else {	
			/* Èô½ÓÊÕ»º³åÇøÖĞÃ»ÓĞÏìÓ¦°üÌå£¬Ôò½«ÆäÇå¿Õ£¬¼´¸´ÓÃÕâ¸ö»º³åÇø */
            u->buffer.pos = u->buffer.start;
            u->buffer.last = u->buffer.start;

			//NGX_HTTP_FLUSH±êÖ¾Î»ÒâÎ¶×ÅÈç¹ûÇëÇórµÄout»º³åÇøÖĞÒÀÈ»ÓĞµÈ´ı·¢ËÍµÄÏìÓ¦£¬Ôò"´ß´Ù"×Å·¢ËÍ³öËüÃÇ
            if (ngx_http_send_special(r, NGX_HTTP_FLUSH) == NGX_ERROR) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

			//Èç¹ûÓëÉÏÓÎ·şÎñÆ÷µÄÁ¬½ÓÉÏ¶ÁÊÂ¼şÒÑ×¼±¸¾ÍĞ÷£¬Ôòµ÷ÓÃngx_http_upstream_process_non_buffered_upstream·½·¨½ÓÊÕÏìÓ¦°üÌå²¢´¦Àí
            if (u->peer.connection->read->ready || u->length == 0)
			{
                ngx_http_upstream_process_non_buffered_upstream(r, u);
            }
        }

        return;
    }

	
    /*
     * Èôngx_http_upstream_t½á¹¹ÌåµÄbuffering±êÖ¾Î»Îª1£¬Ôò×ª·¢ÏìÓ¦°üÌåÊ±ÒÔÉÏÓÎÍøËÙÓÅÏÈ£»
     * ¼´·ÖÅä¸ü¶àµÄÄÚ´æºÍ»º´æ£¬¼´Ò»Ö±½ÓÊÕÀ´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦£¬°ÑÀ´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦±£´æµÄÄÚ´æ»ò»º´æÖĞ£»
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
	/* ³õÊ¼»¯ ngx_http_upstream_t ½á¹¹ÌåÖĞµÄ ngx_event_pipe_t pipe ³ÉÔ±*/
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
	
	/* ³õÊ¼»¯Ô¤¶ÁÁ´±í»º³åÇøpreread_bufs */
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

    if (u->conf->cyclic_temp_file) 
	{

        /*
         * we need to disable the use of sendfile() if we use cyclic temp file
         * because the writing a new data may interfere with sendfile()
         * that uses the same kernel file pages (at least on FreeBSD)
         */

        p->cyclic_temp_file = 1;
        c->sendfile = 0;

    } 
	else 
	{
        p->cyclic_temp_file = 0;
    }

    p->read_timeout = u->conf->read_timeout;
    p->send_timeout = clcf->send_timeout;
    p->send_lowat = clcf->send_lowat;

    p->length = -1;

	 /* µ÷ÓÃinput_filter_initÎªinput_filter·½·¨´¦ÀíÏìÓ¦°üÌå×ö³õÊ¼»¯¹¤×÷ */
    if (u->input_filter_init && u->input_filter_init(p->input_ctx) != NGX_OK)
    {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

	/* ÉèÖÃÉÏÓÎ¶ÁÊÂ¼şµÄ·½·¨ */
    u->read_event_handler = ngx_http_upstream_process_upstream;
	/* ÉèÖÃÏÂÓÎĞ´ÊÂ¼şµÄ·½·¨ */
    r->write_event_handler = ngx_http_upstream_process_downstream;

	/* ´¦ÀíÉÏÓÎÏìÓ¦°üÌå */
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

//µ±ÒÔÏÂÓÎÍøËÙÓÅÏÈ×ª·¢ÏìÓ¦°üÌå¸øÏÂÓÎÊ±£¬ÓÉº¯Êı ngx_http_upstream_process_non_buffered_downstrean ÊµÏÖ
///* buffering ±êÖ¾Î»Îª0Ê±£¬×ª·¢ÏìÓ¦°üÌå¸øÏÂÓÎ¿Í»§¶Ë */
static void
ngx_http_upstream_process_non_buffered_downstream(ngx_http_request_t *r)
{
    ngx_event_t          *wev;
    ngx_connection_t     *c;
    ngx_http_upstream_t  *u;

	//×¢Òâ£¬Õâ¸öcÊÇNginxÓë¿Í»§¶ËÖ®¼äµÄTCPÁ¬½Ó
    c = r->connection;
    u = r->upstream;
    wev = c->write;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream process non buffered downstream");

    c->log->action = "sending to client";

	//¼ì²éÏÂÓÎÁ¬½ÓÉÏĞ´ÊÂ¼şÊÇ·ñ³¬Ê±£¬Èô³¬Ê±Ôò½áÊøµ±Ç°ÇëÇó, ²¢´Óµ±Ç°º¯Êı·µ»Ø
    if (wev->timedout) 
	{
        c->timedout = 1;
        ngx_connection_error(c, NGX_ETIMEDOUT, "client timed out");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_REQUEST_TIME_OUT);
        return;
    }

	//ÒÔ¹Ì¶¨ÄÚ´æ¿é·½Ê½×ª·¢ÏìÓ¦°üÌå¸øÏÂÓÎ¿Í»§¶Ë£¬´ËÊ±µÚ¶ş¸ö²ÎÊıÎª 1
    ngx_http_upstream_process_non_buffered_request(r, 1);
}

//ÓÉÓÚ buffering ±êÖ¾Î»Îª0Ê±£¬Ã»ÓĞ¿ªÆôÎÄ¼ş»º´æ£¬Ö»ÓĞ¹Ì¶¨´óĞ¡µÄÄÚ´æ¿é×÷Îª½ÓÊÕÏìÓ¦»º³åÇø£¬µ±ÉÏÓÎµÄÏìÓ¦°üÌå±È½Ï´óÊ±£¬
//´ËÊ±£¬½ÓÊÕ»º³åÇøÄÚ´æ²¢²»ÄÜ¹»Âú×ãÒ»´ÎĞÔ½ÓÊÕÍêËùÓĞÏìÓ¦°üÌå£¬ Òò´Ë£¬ÔÚ½ÓÊÕ»º³åÇøÒÑÂúÊ±£¬»á×èÈû½ÓÊÕÏìÓ¦°üÌå£¬
//²¢ÏÈ°ÑÒÑ¾­ÊÕµ½µÄÏìÓ¦°üÌå×ª·¢¸øÏÂÓÎ¿Í»§¶Ë¡£ËùÓĞÔÚ×ª·¢ÏìÓ¦°üÌåÊ±£¬ÓĞ¿ÉÄÜ»á½ÓÊÕÉÏÓÎÏìÓ¦°üÌå¡£
//´Ë¹ı³ÌÓÉ ngx_http_upstream_process_non_buffered_upstream ·½·¨ÊµÏÖ£»
static void
ngx_http_upstream_process_non_buffered_upstream(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_connection_t  *c;

	//»ñÈ¡NginxÓëÉÏÓÎ·şÎñÆ÷¼äµÄTCPÁ¬½Ó
    c = u->peer.connection;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream process non buffered upstream");

    c->log->action = "reading upstream";

	//ÅĞ¶ÏÉÏÓÎÁ¬½ÓÉÏ¶ÁÊÂ¼şÊÇ·ñ³¬Ê±£¬Èô³¬Ê±Ôò½áÊøµ±Ç°ÇëÇó, ²¢´Óµ±Ç°º¯Êı·µ»Ø
    if (c->read->timedout) {
        ngx_connection_error(c, NGX_ETIMEDOUT, "upstream timed out");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_GATEWAY_TIME_OUT);
        return;
    }
	  /*
     * ÒÔ¹Ì¶¨ÄÚ´æ¿é·½Ê½×ª·¢ÏìÓ¦°üÌå¸øÏÂÓÎ·şÎñÆ÷£¬
     * ×¢Òâ£º×ª·¢µÄ¹ı³ÌÖĞ£¬»á½ÓÊÕÀ´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦°üÌå£¬´ËÊ±µÚ¶ş¸ö²ÎÊıÎª 0
     */
    ngx_http_upstream_process_non_buffered_request(r, 0);
}

/* ÒÔ¹Ì¶¨ÄÚ´æ¿é·½Ê½×ª·¢ÏìÓ¦°üÌå¸øÏÂÓÎ¿Í»§¶Ë */
/*
 * Èôdo_writeÎª0Ê±£¬Ê×ÏÈÊÕÀ´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦£¬ÔÙ×ª·¢ÏìÓ¦¸øÏÂÓÎ£»
 * Èôdo_writeÎª1£¬Ê×ÏÈ×ª·¢ÏìÓ¦¸øÏÂÓÎ¿Í»§¶Ë£¬ÔÙ½ÓÊÕÀ´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦
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
	/* »ñÈ¡NginxÓëÏÂÓÎ¿Í»§¶ËÖ®¼äµÄTCPÁ¬½Ó */ 
    downstream = r->connection;
	/* »ñÈ¡NginxÓëÉÏÓÎ·şÎñÆ÷Ö®¼äµÄTCPÁ¬½Ó */
    upstream = u->peer.connection;

    b = &u->buffer;

	//µ±u->lengthÎª0Ê±£¬ËµÃ÷²»ÔÙĞèÒª½ÓÊÕÉÏÓÎµÄÏìÓ¦£¬ÄÇÖ»ÄÜ¼ÌĞøÏòÏÂÓÎ·¢ËÍÏìÓ¦
    do_write = do_write || u->length == 0;

    for ( ;; ) 
	{
		/* Èôdo_writeÎª1£¬Ôò¿ªÊ¼ÏòÏÂÓÎ¿Í»§¶Ë×ª·¢ÏìÓ¦°üÌå */  
        if (do_write) 
		{
			 /*
             * ¼ì²éÊÇ·ñÓĞÏìÓ¦°üÌåĞèÒª×ª·¢¸øÏÂÓÎ·şÎñÆ÷£»
             * ÆäÖĞout_bufs±íÊ¾±¾´ÎĞèÒª×ª·¢¸øÏÂÓÎ·şÎñÆ÷µÄÏìÓ¦°üÌå£»
             * busy_bufs±íÊ¾ÉÏÒ»´ÎÏòÏÂÓÎ·şÎñÆ÷×ª·¢ÏìÓ¦°üÌåÊ±Ã»ÓĞ×ª·¢ÍêµÄÏìÓ¦°üÌåÄÚ´æ£»
             * ¼´ÈôÒ»´ÎĞÔ×ª·¢²»ÍêËùÓĞµÄÏìÓ¦°üÌå£¬Ôò»á±£´æÔÚbusy_bufsÁ´±í»º³åÇøÖĞ£¬
             * ÕâÀïµÄ±£´æÖ»ÊÇ½«busy_bufsÖ¸ÏòÎ´·¢ËÍÍê±ÏµÄÏìÓ¦Êı¾İ£»
             */
		
            if (u->out_bufs || u->busy_bufs)
			{
				/* µ÷ÓÃngx_http_output_filter·½·¨½«ÏìÓ¦°üÌå·¢ËÍ¸øÏÂÓÎ·şÎñÆ÷ */
                rc = ngx_http_output_filter(r, u->out_bufs);

				/* Èô·µ»ØÖµ rc = NGX_ERROR£¬Ôò½áÊøÇëÇó */ 
                if (rc == NGX_ERROR) 
				{
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                    return;
                }
				/* 
                 * µ÷ÓÃngx_chain_update_chains·½·¨¸üĞÂfree_bufs¡¢busy_bufs¡¢out_bufsÁ´±í£» 
                 * ¼´Çå¿Õout_bufsÁ´±í£¬°Ñout_bufsÁ´±íÖĞÒÑ·¢ËÍÍêµÄngx_buf_t»º³åÇøÇå¿Õ£¬²¢½«ÆäÌí¼Óµ½free_bufsÁ´±íÖĞ£» 
                 * °Ñout_bufsÁ´±íÖĞÎ´·¢ËÍÍêµÄngx_buf_t»º³åÇøÌí¼Óµ½busy_bufsÁ´±íÖĞ£» 
                 */
                ngx_chain_update_chains(r->pool, &u->free_bufs, &u->busy_bufs, &u->out_bufs, u->output.tag);
            }

			 /* 
             * busy_bufsÎª¿Õ£¬±íÊ¾ËùÓĞÏìÓ¦°üÌåÒÑ¾­×ª·¢µ½ÏÂÓÎ·şÎñÆ÷£¬ 
             * ´ËÊ±Çå¿Õ½ÓÊÕ»º³åÇøbufferÒÔ±ãÔÙ´Î½ÓÊÕÀ´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦°üÌå£» 
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

		/* ¼ÆËã½ÓÊÕ»º³åÇøbufferÊ£Óà¿ÉÓÃµÄÄÚ´æ¿Õ¼ä */
        size = b->end - b->last;
		/* 
         * Èô½ÓÊÕ»º³åÇøbufferÓĞÊ£ÓàµÄ¿ÉÓÃ¿Õ¼ä£¬ 
         * ÇÒ´ËÊ±¶ÁÊÂ¼ş¿É¶Á£¬¼´¿É¶ÁÈ¡À´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦°üÌå£» 
         * Ôòµ÷ÓÃrecv·½·¨¿ªÊ¼½ÓÊÕÀ´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦°üÌå£¬²¢±£´æÔÚ½ÓÊÕ»º³åÇøbufferÖĞ£» 
         */ 
        if (size && upstream->read->ready) 
		{

            n = upstream->recv(upstream, b->last, size);
			/* Èô·µ»ØÖµ n = NGX_AGAIN£¬ÔòµÈ´ıÏÂÒ»´Î¿É¶ÁÊÂ¼ş·¢Éú¼ÌĞø½ÓÊÕÏìÓ¦ */  
            if (n == NGX_AGAIN) 
			{
                break;
            }

			/* 
             * Èô·µ»ØÖµ n ´óÓÚ0£¬±íÊ¾½ÓÊÕµ½ÏìÓ¦°üÌå£¬ 
             * Ôòµ÷ÓÃinput_filter·½·¨´¦ÀíÏìÓ¦°üÌå£» 
             * ²¢°Ñdo_writeÉèÖÃÎª1£¬±íÊ¾ÒÑ¾­½ÓÊÕµ½ÏìÓ¦°üÌå£¬´ËÊ±¿É×ª·¢¸øÏÂÓÎ·şÎñÆ÷
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

	/* µ÷ÓÃngx_handle_write_event·½·¨½«NginxÓëÏÂÓÎÖ®¼äµÄÁ¬½ÓÉÏµÄĞ´ÊÂ¼ş×¢²áµÄepollÊÂ¼ş»úÖÆÖĞ */
    if (downstream->data == r) 
	{
        if (ngx_handle_write_event(downstream->write, clcf->send_lowat) != NGX_OK)
        {
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }
    }
	
	/* µ÷ÓÃngx_add_timer·½·¨½«NginxÓëÏÂÓÎÖ®¼äµÄÁ¬½ÓÉÏµÄĞ´ÊÂ¼şÌí¼Óµ½¶¨Ê±Æ÷ÊÂ¼ş»úÖÆÖĞ */
    if (downstream->write->active && !downstream->write->ready)
	{
        ngx_add_timer(downstream->write, clcf->send_timeout);
    }
	else if (downstream->write->timer_set) 
   	{
        ngx_del_timer(downstream->write);
    }

	/* µ÷ÓÃngx_handle_read_event·½·¨½«NginxÓëÉÏÓÎÖ®¼äµÄÁ¬½ÓÉÏµÄ¶ÁÊÂ¼ş×¢²áµÄepollÊÂ¼ş»úÖÆÖĞ */
    if (ngx_handle_read_event(upstream->read, 0) != NGX_OK) 
	{
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

	 /* µ÷ÓÃngx_add_timer·½·¨½«NginxÓëÉÏÓÎÖ®¼äµÄÁ¬½ÓÉÏµÄ¶ÁÊÂ¼şÌí¼Óµ½¶¨Ê±Æ÷ÊÂ¼ş»úÖÆÖĞ */
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

/* upstream»úÖÆÄ¬ÈÏµÄinput_filter·½·¨ */
static ngx_int_t
ngx_http_upstream_non_buffered_filter(void *data, ssize_t bytes)
{
	 /* 
     * data²ÎÊıÊÇngx_http_upstream_t½á¹¹ÌåÖĞµÄinput_filter_ctx£¬ 
     * µ±HTTPÄ£¿éÃ»ÓĞÊµÏÖinput_filter·½·¨Ê±£¬ 
     * input_filter_ctxÖ¸Ïòngx_http_request_t½á¹¹Ìå£» 
     */  
    ngx_http_request_t  *r = data;

    ngx_buf_t            *b;
    ngx_chain_t          *cl, **ll;
    ngx_http_upstream_t  *u;

    u = r->upstream;

	/* ÕÒµ½out_bufsÁ´±íµÄ×îºóÒ»¸ö»º³åÇø£¬²¢ÓÉllÖ¸Ïò¸Ã»º³åÇø */ 
    for (cl = u->out_bufs, ll = &u->out_bufs; cl; cl = cl->next)
	{
        ll = &cl->next;
    }

	/* ´Ófree_bufs¿ÕÏĞÁ´±í»º³åÇøÖĞ»ñÈ¡Ò»¸öngx_buf_t½á¹¹Ìå¸øcl */  
    cl = ngx_chain_get_free_buf(r->pool, &u->free_bufs);
    if (cl == NULL) 
	{
        return NGX_ERROR;
    }

	/* ½«ĞÂ·ÖÅäµÄngx_buf_t»º³åÇøÌí¼Óµ½out_bufsÁ´±íµÄÎ²¶Ë */ 
    *ll = cl;

    cl->buf->flush = 1;
    cl->buf->memory = 1;

	 /* bufferÊÇ±£´æÀ´×ÔÉÏÓÎ·şÎñÆ÷µÄÏìÓ¦°üÌå */
    b = &u->buffer;

	/* ½«ÏìÓ¦°üÌåÊı¾İ±£´æÔÚcl»º³åÇøÖĞ */ 
    cl->buf->pos = b->last;
    b->last += bytes;
    cl->buf->last = b->last;
    cl->buf->tag = u->output.tag;

    if (u->length == -1) 
	{
        return NGX_OK;
    }

	/* ¸üĞÂlength³¤¶È£¬±íÊ¾ĞèÒª½ÓÊÕµÄ°üÌå³¤¶È¼õÉÙbytes×Ö½Ú */ 
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

	/* »ñÈ¡ Nginx ÓëÏÂÓÎ¿Í»§¶ËÖ®¼äµÄÁ¬½Ó */ 
    c = r->connection;
    u = r->upstream;
    p = u->pipe;
    wev = c->write;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream process downstream");

    c->log->action = "sending to client";

	/* ¼ì²éÏÂÓÎÁ¬½ÓÉÏĞ´ÊÂ¼şÊÇ·ñ³¬Ê±*/
    if (wev->timedout) 
	{
		//¼ì²éÊÇ·ñĞèÒªÑÓ³Ù´¦ÀíÕâ¸öÊÂ¼ş
        if (wev->delayed)
		{
            wev->timedout = 0;
            wev->delayed = 0;

			//ÈôĞ´ÊÂ¼şÎ´×¼±¸¾ÍĞ÷£¬Ôò½«Ğ´ÊÂ¼ş¼ÓÈëµ½¶¨Ê±Æ÷»úÖÆÖĞºÍepollÊÂ¼ş»úÖÆÖĞ£¬²¢´Óµ±Ç°º¯Êı·µ»Ø
            if (!wev->ready) 
			{
                ngx_add_timer(wev, p->send_timeout);

                if (ngx_handle_write_event(wev, p->send_lowat) != NGX_OK)
				{
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                }

                return;
            }

			//Ğ´ÊÂ¼şÒÑ¾­×¼±¸¾ÍĞ÷£¬½«ÏìÓ¦°üÌå×ª·¢¸øÏÂÓÎ¿Í»§¶Ë
            if (ngx_event_pipe(p, wev->write) == NGX_ABORT) 
			{
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

        } 
		else 
		{
			//Î´ÉèÖÃÑÓ³Ù´¦Àí¶øĞ´ÊÂ¼ş³¬Ê±£¬Ôò±íÊ¾Á¬½Ó³¬Ê±´íÎó
            p->downstream_error = 1;
            c->timedout = 1;
            ngx_connection_error(c, NGX_ETIMEDOUT, "client timed out");
        }

    } 
	else 
	{
		//ÈôĞèÒªÑÓ³Ù´¦ÀíÕâ¸öĞ´ÊÂ¼ş£¬Ôò½«Ğ´ÊÂ¼ş¼ÓÈëµ½epoll»úÖÆÖĞ£¬²¢´Óµ±Ç°º¯Êı·µ»Ø
        if (wev->delayed) 
		{
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "http downstream delayed");

            if (ngx_handle_write_event(wev, p->send_lowat) != NGX_OK)
			{
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            }

            return;
        }

		//½«ÏìÓ¦°üÌå×ª·¢¸øÏÂÓÎ¿Í»§¶Ë
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

	/* ¼ì²éÉÏÓÎÁ¬½ÓÉÏ¶ÁÊÂ¼şÊÇ·ñ³¬Ê±*/
    if (rev->timedout) 
	{
        if (rev->delayed) 
		{

            rev->timedout = 0;
            rev->delayed = 0;

			//Èô¶ÁÊÂ¼şÎ´×¼±¸¾ÍĞ÷£¬Ôò½«¶ÁÊÂ¼ş¼ÓÈëµ½¶¨Ê±Æ÷»úÖÆÖĞºÍepollÊÂ¼ş»úÖÆÖĞ£¬²¢´Óµ±Ç°º¯Êı·µ»Ø
            if (!rev->ready) 
			{
                ngx_add_timer(rev, p->read_timeout);

                if (ngx_handle_read_event(rev, 0) != NGX_OK) 
				{
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                }

                return;
            }

			//¶ÁÊÂ¼şÒÑ¾­×¼±¸¾ÍĞ÷£¬½«¶ÁÈ¡ÏìÓ¦°üÌå
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

            if (p->upstream_done || (p->upstream_eof && p->length == -1))
            {
                ngx_http_upstream_finalize_request(r, u, 0);
                return;
            }

            if (p->upstream_eof) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "upstream prematurely closed connection");
            }

            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_BAD_GATEWAY);
            return;
        }
    }

    if (p->downstream_error)
	{
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
		//Ö»Òª´íÎóÀàĞÍ²»ÊÇ NGX_HTTP_UPSTREAM_FT_HTTP_403»òÕßNGX_HTTP_UPSTREAM_FT_HTTP_404£¬¶¼ÈÏÎªÉÏÓÎ·şÎñÆ÷ÓĞÎÊÌâ
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

            if (u->cache_status == NGX_HTTP_CACHE_EXPIRED
                && (u->conf->cache_use_stale & ft_type))
            {
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

//½áÊø upstream ÇëÇóÓÉº¯Êı ngx_http_upstream_finalize_request ÊµÏÖ£¬
//¸Ãº¯Êı×îÖÕ»áµ÷ÓÃ HTTP ¿ò¼ÜµÄ ngx_http_finalize_request ·½·¨À´½áÊøÇëÇó
//NGX_DECLINED -- ¹Ø±ÕÉÏÓÎÁ¬½Ó£¬²»¹Ø±ÕÏÂÓÎÁ¬½Ó
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

	/* ½« cleanup Ö¸ÏòµÄÇåÀí×ÊÔ´»Øµ÷·½·¨ÉèÖÃÎª NULL */ 
    *u->cleanup = NULL;
    u->cleanup = NULL;

	/* ÊÍ·Å½âÎöÖ÷»úÓòÃûÊ±·ÖÅäµÄ×ÊÔ´ */ 
    if (u->resolved && u->resolved->ctx) 
	{
        ngx_resolve_name_done(u->resolved->ctx);
        u->resolved->ctx = NULL;
    }

	/* ÉèÖÃµ±Ç°Ê±¼äÎª HTTP ÏìÓ¦½áÊøµÄÊ±¼ä */ 
    if (u->state && u->state->response_time) 
	{
        u->state->response_time = ngx_current_msec - u->state->response_time;

        if (u->pipe && u->pipe->read_length) 
		{
            u->state->response_length = u->pipe->read_length;
        }
    }

	/* µ÷ÓÃ¸Ã·½·¨Ö´ĞĞÒ»Ğ©²Ù×÷ */
    u->finalize_request(r, rc);

	//Èç¹ûÓĞÉèÖÃpeer.free¹³×Ó£¬Ôòµ÷ÓÃÊÍ·Åpeer
	//·ÇkeepaliveµÄÇé¿öÏÂ¸Ã¹³×ÓÊÇngx_http_upstream_free_round_robin_peer
	//keepaliveµÄÇé¿öÏÂ¸Ã¹³×ÓÊÇngx_http_upstream_free_keepalive_peer£¬¸Ã¹³×Ó»á½«Á¬½Ó
	//»º´æµ½³¤Á¬½Ócache³Ø£¬²¢½«u->peer.connectionÉèÖÃ³É¿Õ£¬·ÀÖ¹ÏÂÃæ´úÂë¹Ø±ÕÁ¬½Ó¡£
    if (u->peer.free && u->peer.sockaddr) 
	{
        u->peer.free(&u->peer, u->peer.data, 0);
        u->peer.sockaddr = NULL;
    }

	/* ÈôÉÏÓÎÁ¬½Ó»¹Î´¹Ø±Õ(»òÎ´±»»º´æÆğÀ´)£¬Ôòµ÷ÓÃ ngx_close_connection ·½·¨¹Ø±Õ¸ÃÁ¬½Ó */ 
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

	//¹Ø±ÕÁ¬½ÓºóÖÃ¿Õ
    u->peer.connection = NULL;

    if (u->pipe && u->pipe->temp_file)
	{
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http upstream temp fd: %d", u->pipe->temp_file->file.fd);
    }

	/* ÈôÊ¹ÓÃÁËÎÄ¼ş»º´æ£¬Ôòµ÷ÓÃ ngx_delete_file ·½·¨É¾³ıÓÃÓÚ»º´æÏìÓ¦µÄÁÙÊ±ÎÄ¼ş */
    if (u->store && u->pipe && u->pipe->temp_file && u->pipe->temp_file->file.fd != NGX_INVALID_FILE)
    {
        if (ngx_delete_file(u->pipe->temp_file->file.name.data) == NGX_FILE_ERROR)
        {
            ngx_log_error(NGX_LOG_CRIT, r->connection->log, ngx_errno, 
				ngx_delete_file_n " \"%s\" failed", u->pipe->temp_file->file.name.data);
        }
    }

#if (NGX_HTTP_CACHE)

    if (r->cache) 
	{

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

	/* µ÷ÓÃ HTTP ¿ò¼ÜÊµÏÖµÄ ngx_http_finalize_request ·½·¨¹Ø±ÕÇëÇó */ 
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
ngx_http_upstream_process_buffering(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
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
¸ù¾İurl²éÕÒumcf->upstreamsÖĞÊÇ·ñ´æÔÚÏàÍ¬µÄ£¬´æÔÚÏàÍ¬µÄÔò·µ»Ø£¬²»´æÔÚÔòÌí¼Ó
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
	/*²éÕÒÈ«¾ÖÉÏÓĞ·şÎñÆ÷ÊÇ·ñÓĞÏàÍ¬ip(host)ºÍportµÄ*/
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

	/*Î´ÕÒµ½£¬ÔòĞÂ½¨Ò»¸ö*/
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
	
	//±éÀúupstreamÊı×é¡£upstreamµÄÀ´Ô´°üÀ¨ÏÔĞÔµÄÅäÖÃupstreamÄ£Ê½£¨ngx_http_upstream£©
	//ºÍÒşĞÔµÄproxy_passÖ¸Áî£¨ngx_http_proxy_pass£©£¬upstreamµÄÌí¼ÓÊÇÍ¨¹ı
	//ngx_http_upstream_addº¯Êı£¬ĞèÒª×¢ÒâµÄÊÇproxy_passÖ¸ÁîĞ¯´ø±äÁ¿²ÎÊıÊ±²»»áÌí¼Óupstream
    for (i = 0; i < umcf->upstreams.nelts; i++) 
	{
        init = uscfp[i]->peer.init_upstream ? uscfp[i]->peer.init_upstream: ngx_http_upstream_init_round_robin;

        if (init(cf, uscfp[i]) != NGX_OK)
		{
            return NGX_CONF_ERROR;
        }
    }


    /* upstream_headers_in_hash */

    if (ngx_array_init(&headers_in, cf->temp_pool, 32, sizeof(ngx_hash_key_t)) != NGX_OK)
    {
        return NGX_CONF_ERROR;
    }

    for (header = ngx_http_upstream_headers_in; header->name.len; header++)
	{
        hk = ngx_array_push(&headers_in);
        if (hk == NULL)
		{
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
