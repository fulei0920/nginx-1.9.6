
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_UPSTREAM_H_INCLUDED_
#define _NGX_HTTP_UPSTREAM_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_event_connect.h>
#include <ngx_event_pipe.h>
#include <ngx_http.h>


#define NGX_HTTP_UPSTREAM_FT_ERROR           0x00000002
#define NGX_HTTP_UPSTREAM_FT_TIMEOUT         0x00000004
#define NGX_HTTP_UPSTREAM_FT_INVALID_HEADER  0x00000008
#define NGX_HTTP_UPSTREAM_FT_HTTP_500        0x00000010
#define NGX_HTTP_UPSTREAM_FT_HTTP_502        0x00000020
#define NGX_HTTP_UPSTREAM_FT_HTTP_503        0x00000040
#define NGX_HTTP_UPSTREAM_FT_HTTP_504        0x00000080
#define NGX_HTTP_UPSTREAM_FT_HTTP_403        0x00000100
#define NGX_HTTP_UPSTREAM_FT_HTTP_404        0x00000200
#define NGX_HTTP_UPSTREAM_FT_UPDATING        0x00000400
#define NGX_HTTP_UPSTREAM_FT_BUSY_LOCK       0x00000800
#define NGX_HTTP_UPSTREAM_FT_MAX_WAITING     0x00001000
#define NGX_HTTP_UPSTREAM_FT_NOLIVE          0x40000000
#define NGX_HTTP_UPSTREAM_FT_OFF             0x80000000

#define NGX_HTTP_UPSTREAM_FT_STATUS          (NGX_HTTP_UPSTREAM_FT_HTTP_500  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_502  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_503  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_504  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_403  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_404)

//表示包头不合法
#define NGX_HTTP_UPSTREAM_INVALID_HEADER     40


#define NGX_HTTP_UPSTREAM_IGN_XA_REDIRECT    0x00000002
#define NGX_HTTP_UPSTREAM_IGN_XA_EXPIRES     0x00000004
#define NGX_HTTP_UPSTREAM_IGN_EXPIRES        0x00000008
#define NGX_HTTP_UPSTREAM_IGN_CACHE_CONTROL  0x00000010
#define NGX_HTTP_UPSTREAM_IGN_SET_COOKIE     0x00000020
#define NGX_HTTP_UPSTREAM_IGN_XA_LIMIT_RATE  0x00000040
#define NGX_HTTP_UPSTREAM_IGN_XA_BUFFERING   0x00000080
#define NGX_HTTP_UPSTREAM_IGN_XA_CHARSET     0x00000100
#define NGX_HTTP_UPSTREAM_IGN_VARY           0x00000200


typedef struct 
{
    ngx_msec_t                       bl_time;
    ngx_uint_t                       bl_state;

    ngx_uint_t                       status;
    ngx_msec_t                       response_time;
    ngx_msec_t                       connect_time;
    ngx_msec_t                       header_time;
    off_t                            response_length;

    ngx_str_t                       *peer;
} ngx_http_upstream_state_t;


typedef struct 
{
    ngx_hash_t                       headers_in_hash;
    ngx_array_t                      upstreams;		 	/* array of ngx_http_upstream_srv_conf_t* 存储所有的上游服务器配置*/
                                            
} ngx_http_upstream_main_conf_t;

typedef struct ngx_http_upstream_srv_conf_s  ngx_http_upstream_srv_conf_t;

typedef ngx_int_t (*ngx_http_upstream_init_pt)(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *us);
typedef ngx_int_t (*ngx_http_upstream_init_peer_pt)(ngx_http_request_t *r, ngx_http_upstream_srv_conf_t *us);


typedef struct 
{
    ngx_http_upstream_init_pt        init_upstream;
    ngx_http_upstream_init_peer_pt   init;
    void                            *data;
} ngx_http_upstream_peer_t;


typedef struct
{
    ngx_str_t                        name;
    ngx_addr_t                      *addrs;
    ngx_uint_t                       naddrs;
    ngx_uint_t                       weight;			/*权重*/
    ngx_uint_t                       max_fails;
    time_t                           fail_timeout;

    unsigned                         down:1;
    unsigned                         backup:1;
} ngx_http_upstream_server_t;


#define NGX_HTTP_UPSTREAM_CREATE        0x0001
#define NGX_HTTP_UPSTREAM_WEIGHT        0x0002
#define NGX_HTTP_UPSTREAM_MAX_FAILS     0x0004
#define NGX_HTTP_UPSTREAM_FAIL_TIMEOUT  0x0008
#define NGX_HTTP_UPSTREAM_DOWN          0x0010
#define NGX_HTTP_UPSTREAM_BACKUP        0x0020


struct ngx_http_upstream_srv_conf_s 
{
    ngx_http_upstream_peer_t         peer;
    void                           **srv_conf;

    ngx_array_t                     *servers;  /* array of ngx_http_upstream_server_t */

    ngx_uint_t                       flags;
    ngx_str_t                        host;
    u_char                          *file_name;
    ngx_uint_t                       line;
    in_port_t                        port;
    in_port_t                        default_port;
    ngx_uint_t                       no_port;  /* unsigned no_port:1 */

#if (NGX_HTTP_UPSTREAM_ZONE)
    ngx_shm_zone_t                  *shm_zone;
#endif
};


typedef struct 
{
    ngx_addr_t                      *addr;
    ngx_http_complex_value_t        *value;
} ngx_http_upstream_local_t;


typedef struct 
{
	//当在ngx_http_upstream_t结构体中没有实现resolved成员时，upstream这个结构体才会生效，
	//它会定义上游服务器的配置
    ngx_http_upstream_srv_conf_t    *upstream;
	//建立TCP连接的超时时间，实际上就是写事件添加到定时器中时设置的超时时间
    ngx_msec_t                       connect_timeout;
	//发送请求的超时时间。通常就是写事件添加到定时器中时设置的超时时间
    ngx_msec_t                       send_timeout;
	//接收响应的超时时间。通常就是读事件添加到定时器中时设置的超时时间
    ngx_msec_t                       read_timeout;
	//目前无意义
    ngx_msec_t                       timeout;
    ngx_msec_t                       next_upstream_timeout;
	//TCP的SO_SNOLOWAT选项，表示发送缓冲区的下限
    size_t                           send_lowat;
	//定义了接收头部的缓冲区分配的内存大小(ngx_http_upstream_t中的buffer缓冲区)，当不转发响应
	//给下游或者在buffering标志位为0的情况下转发响应时，它同样表示接收包体的缓冲区大小
    size_t                           buffer_size;
    size_t                           limit_rate;
	//仅当buffering标志位为1，并且向下游转发响应时生效。它会设置到ngx_event_pipe_t结构体的
	//busy_size成员中
    size_t                           busy_buffers_size;
	//临时缓存文件的最大长度
    size_t                           max_temp_file_size;
    size_t                           temp_file_write_size;

    size_t                           busy_buffers_size_conf;
    size_t                           max_temp_file_size_conf;
	//一次写入临时文件的字符流最大长度
    size_t                           temp_file_write_size_conf;
	//以缓存响应的方式转发上游服务器的包体时所用的内存大小
    ngx_bufs_t                       bufs;
	//针对ngx_http_upstream_t中的header_in成员，ignore_headers可根据位操作跳过一些头部
    ngx_uint_t                       ignore_headers;
    ngx_uint_t                       next_upstream;
	//表示创建的目录、文件的权限
    ngx_uint_t                       store_access;
    ngx_uint_t                       next_upstream_tries;
	//决定转发响应方式的标志位
    //1：认为上游快于下游，会尽量地在内存或者磁盘中缓存来自上游的响应；
    //0：认为下游快于上游，仅开辟一块固定大小的内存块作为缓存来转发响应 
    ngx_flag_t                       buffering;
    ngx_flag_t                       request_buffering;
    ngx_flag_t                       pass_request_headers;
    ngx_flag_t                       pass_request_body;

	// 1：上游服务器交互时不检查是否与下游客户端断开连接，继续执行交互内容
    ngx_flag_t                       ignore_client_abort;
	//截取错误码，查看是否有对应可以返回的语义
    ngx_flag_t                       intercept_errors;
	// 1：试图复用临时文件中已经使用过的空间
    ngx_flag_t                       cyclic_temp_file;
    ngx_flag_t                       force_ranges;
	//存放临时文件的路径
    ngx_path_t                      *temp_path;
	//根据ngx_http_upstream_hide_headers_hash函数构造出的需要隐藏的HTTP头部散列表
    ngx_hash_t                       hide_headers_hash;
    ngx_array_t                     *hide_headers;
    ngx_array_t                     *pass_headers;

    ngx_http_upstream_local_t       *local;

#if (NGX_HTTP_CACHE)
    ngx_shm_zone_t                  *cache_zone;
    ngx_http_complex_value_t        *cache_value;

    ngx_uint_t                       cache_min_uses;
    ngx_uint_t                       cache_use_stale;
    ngx_uint_t                       cache_methods;

    ngx_flag_t                       cache_lock;
    ngx_msec_t                       cache_lock_timeout;
    ngx_msec_t                       cache_lock_age;

    ngx_flag_t                       cache_revalidate;

    ngx_array_t                     *cache_valid;
    ngx_array_t                     *cache_bypass;
    ngx_array_t                     *no_cache;
#endif

    ngx_array_t                     *store_lengths;
    ngx_array_t                     *store_values;

#if (NGX_HTTP_CACHE)
    signed                           cache:2;
#endif
    signed                           store:2;
    unsigned                         intercept_404:1;
    unsigned                         change_buffering:1;

#if (NGX_HTTP_SSL)
    ngx_ssl_t                       *ssl;
    ngx_flag_t                       ssl_session_reuse;

    ngx_http_complex_value_t        *ssl_name;
    ngx_flag_t                       ssl_server_name;
    ngx_flag_t                       ssl_verify;
#endif
	//使用upstream的模块名称，仅用于记录日志
    ngx_str_t                        module;				/* "proxy" */
} ngx_http_upstream_conf_t;


typedef struct {
    ngx_str_t                        name;
    ngx_http_header_handler_pt       handler;
    ngx_uint_t                       offset;
    ngx_http_header_handler_pt       copy_handler;
    ngx_uint_t                       conf;
    ngx_uint_t                       redirect;  /* unsigned   redirect:1; */
} ngx_http_upstream_header_t;


typedef struct 
{
    ngx_list_t                       headers;		/*array of ngx_table_elt_t*/

    ngx_uint_t                       status_n;
    ngx_str_t                        status_line;

    ngx_table_elt_t                 *status;
    ngx_table_elt_t                 *date;
    ngx_table_elt_t                 *server;
    ngx_table_elt_t                 *connection;

    ngx_table_elt_t                 *expires;
    ngx_table_elt_t                 *etag;
    ngx_table_elt_t                 *x_accel_expires;
    ngx_table_elt_t                 *x_accel_redirect;
    ngx_table_elt_t                 *x_accel_limit_rate;

    ngx_table_elt_t                 *content_type;
    ngx_table_elt_t                 *content_length;

    ngx_table_elt_t                 *last_modified;
    ngx_table_elt_t                 *location;
    ngx_table_elt_t                 *accept_ranges;
    ngx_table_elt_t                 *www_authenticate;
    ngx_table_elt_t                 *transfer_encoding;
    ngx_table_elt_t                 *vary;

#if (NGX_HTTP_GZIP)
    ngx_table_elt_t                 *content_encoding;
#endif

    ngx_array_t                      cache_control;
    ngx_array_t                      cookies;

    off_t                            content_length_n;
    time_t                           last_modified_time;

    unsigned                         connection_close:1;
    unsigned                         chunked:1;
} ngx_http_upstream_headers_in_t;


typedef struct 
{
    ngx_str_t                        host;
    in_port_t                        port;
    ngx_uint_t                       no_port; /* unsigned no_port:1 */

    ngx_uint_t                       naddrs;
    ngx_addr_t                      *addrs;

    struct sockaddr                 *sockaddr;
    socklen_t                        socklen;

    ngx_resolver_ctx_t              *ctx;
} ngx_http_upstream_resolved_t;


typedef void (*ngx_http_upstream_handler_pt)(ngx_http_request_t *r,
    ngx_http_upstream_t *u);


struct ngx_http_upstream_s 
{
	//处理读事件的回调方法，每一个阶段都有不同的read_event_handler
    ngx_http_upstream_handler_pt     read_event_handler;
	//处理写事件的回调方法，每一个阶段都有不同的write_event_handler
    ngx_http_upstream_handler_pt     write_event_handler;

    ngx_peer_connection_t            peer;

    ngx_event_pipe_t                *pipe;
	//存储发往上游服务器的请求的缓冲区
    ngx_chain_t                     *request_bufs;

    ngx_output_chain_ctx_t           output;
    ngx_chain_writer_ctx_t           writer;

	//使用upstream机制时的各种配置
	//proxy module 指向对应location中的plcf->upstream
    ngx_http_upstream_conf_t        *conf;
#if (NGX_HTTP_CACHE)
    ngx_array_t                     *caches;
#endif

    ngx_http_upstream_headers_in_t   headers_in;

    ngx_http_upstream_resolved_t    *resolved;

    ngx_buf_t                        from_client;

    ngx_buf_t                        buffer;		//接收上游服务器发来的响应的头部的缓冲区
    //还需要接收上游包体的长度
    off_t                            length;

    ngx_chain_t                     *out_bufs;
    ngx_chain_t                     *busy_bufs;
    ngx_chain_t                     *free_bufs;

	//初始化input filter的上下文。nginx默认的input_filter_init 直接返回。
    ngx_int_t                      (*input_filter_init)(void *data);
	//处理后端服务器返回的响应正文。nginx默认的input_filter会将收到的内容封装成为缓冲区链ngx_chain。
	//该链由upstream的out_bufs指针域定位，所以开发人员可以在模块以外通过该指针得到后端服务器返回的正文数据。
	//memcached模块实现了自己的 input_filter，在后面会具体分析这个模块。
    ngx_int_t                      (*input_filter)(void *data, ssize_t bytes);
	//
    void                            *input_filter_ctx;

#if (NGX_HTTP_CACHE)
    ngx_int_t                      (*create_key)(ngx_http_request_t *r);
#endif
	//生成发送到后端服务器的请求缓冲（缓冲链），在初始化upstream 时使用。
    ngx_int_t                      (*create_request)(ngx_http_request_t *r);
	//在某台后端服务器出错的情况，nginx会尝试另一台后端服务器。 nginx选定新的服务器以后，
	//会先调用此函数，以重新初始化 upstream模块的工作状态，然后再次进行upstream连接。
    ngx_int_t                      (*reinit_request)(ngx_http_request_t *r);
	//处理后端服务器返回的信息头部。所谓头部是与upstream server 通信的协议规定的，
	//比如HTTP协议的header部分，或者memcached 协议的响应状态部分。
    ngx_int_t                      (*process_header)(ngx_http_request_t *r);
	//在客户端放弃请求时被调用。不需要在函数中实现关闭后端服务 器连接的功能，
	//系统会自动完成关闭连接的步骤，所以一般此函 数不会进行任何具体工作。
    void                           (*abort_request)(ngx_http_request_t *r);
	//正常完成与后端服务器的请求后调用该函数，与abort_request 相同，一般也不会进行任何具体工作。
    void                           (*finalize_request)(ngx_http_request_t *r, ngx_int_t rc);

    ngx_int_t                      (*rewrite_redirect)(ngx_http_request_t *r, ngx_table_elt_t *h, size_t prefix);
    ngx_int_t                      (*rewrite_cookie)(ngx_http_request_t *r, ngx_table_elt_t *h);

    ngx_msec_t                       timeout;

    ngx_http_upstream_state_t       *state;

    ngx_str_t                        method;
    ngx_str_t                        schema;
    ngx_str_t                        uri;

#if (NGX_HTTP_SSL)
    ngx_str_t                        ssl_name;
#endif

    ngx_http_cleanup_pt             *cleanup;

    unsigned                         store:1;
    unsigned                         cacheable:1;
    unsigned                         accel:1;
    unsigned                         ssl:1;
#if (NGX_HTTP_CACHE)
    unsigned                         cache_status:3;
#endif

    unsigned                         buffering:1;
    unsigned                         keepalive:1;
    unsigned                         upgrade:1;
	//表示是否已经传递了request_bufs缓冲区。在第一次以request_bufs作为参数调用ngx_output_chain方法后，request_sent会置为1
    unsigned                         request_sent:1;
	//标志位，为1时表明上游服务器的响应需要直接转发给客户端，而且此时Nginx已经把响应包头转发给客户端了
    unsigned                         header_sent:1;
};


typedef struct {
    ngx_uint_t                      status;
    ngx_uint_t                      mask;
} ngx_http_upstream_next_t;


typedef struct {
    ngx_str_t   key;
    ngx_str_t   value;
    ngx_uint_t  skip_empty;
} ngx_http_upstream_param_t;


ngx_int_t ngx_http_upstream_cookie_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
ngx_int_t ngx_http_upstream_header_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

ngx_int_t ngx_http_upstream_create(ngx_http_request_t *r);
void ngx_http_upstream_init(ngx_http_request_t *r);
ngx_http_upstream_srv_conf_t *ngx_http_upstream_add(ngx_conf_t *cf,
    ngx_url_t *u, ngx_uint_t flags);
char *ngx_http_upstream_bind_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
char *ngx_http_upstream_param_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
ngx_int_t ngx_http_upstream_hide_headers_hash(ngx_conf_t *cf,
    ngx_http_upstream_conf_t *conf, ngx_http_upstream_conf_t *prev,
    ngx_str_t *default_hide_headers, ngx_hash_init_t *hash);


#define ngx_http_conf_upstream_srv_conf(uscf, module)                         \
    uscf->srv_conf[module.ctx_index]


extern ngx_module_t        ngx_http_upstream_module;
extern ngx_conf_bitmask_t  ngx_http_upstream_cache_method_mask[];
extern ngx_conf_bitmask_t  ngx_http_upstream_ignore_headers_masks[];


#endif /* _NGX_HTTP_UPSTREAM_H_INCLUDED_ */
