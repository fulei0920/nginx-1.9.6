
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_SSL_H_INCLUDED_
#define _NGX_HTTP_SSL_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct 
{
    ngx_flag_t                      enable;

    ngx_ssl_t                       ssl;

    ngx_flag_t                      prefer_server_ciphers;

    ngx_uint_t                      protocols;

    ngx_uint_t                      verify;
	/*证书验证深度*/
    ngx_uint_t                      verify_depth;		

    size_t                          buffer_size;

    ssize_t                         builtin_session_cache;

    time_t                          session_timeout;
	/*证书文件路径*/
    ngx_str_t                       certificate;			
    ngx_str_t                       certificate_key;
    ngx_str_t                       dhparam;
    ngx_str_t                       ecdh_curve;
    ngx_str_t                       client_certificate;
    ngx_str_t                       trusted_certificate;	/*包含PEM格式编码的一个或多个受信任的CA证书的文件路径*/
    ngx_str_t                       crl;
	/*表示允许使用的密码组列表， 列表的各部分以冒号分隔，
	如"RC4-SHA; DES-CBC3-SHA",表示可使用密码组TLS_RSA_WITH_RC4_128_SHA和TLS_RSA_WITH_3DES_EDE_CBC_SHA*/
    ngx_str_t                       ciphers;		
													

    ngx_array_t                    *passwords;

    ngx_shm_zone_t                 *shm_zone;

    ngx_flag_t                      session_tickets;
    ngx_array_t                    *session_ticket_keys;

    ngx_flag_t                      stapling;
    ngx_flag_t                      stapling_verify;
    ngx_str_t                       stapling_file;
    ngx_str_t                       stapling_responder;

    u_char                         *file;
    ngx_uint_t                      line;
} ngx_http_ssl_srv_conf_t;


extern ngx_module_t  ngx_http_ssl_module;


#endif /* _NGX_HTTP_SSL_H_INCLUDED_ */
