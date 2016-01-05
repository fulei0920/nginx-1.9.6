
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

//��ʾ��ͷ���Ϸ�
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
    ngx_array_t                      upstreams;		 	/* array of ngx_http_upstream_srv_conf_t* �洢���е����η���������*/
                                            
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
    ngx_uint_t                       weight;			/*Ȩ��*/
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
	//����ngx_http_upstream_t�ṹ����û��ʵ��resolved��Աʱ��upstream����ṹ��Ż���Ч��
	//���ᶨ�����η�����������
    ngx_http_upstream_srv_conf_t    *upstream;
	//����TCP���ӵĳ�ʱʱ�䣬ʵ���Ͼ���д�¼���ӵ���ʱ����ʱ���õĳ�ʱʱ��
    ngx_msec_t                       connect_timeout;
	//��������ĳ�ʱʱ�䡣ͨ������д�¼���ӵ���ʱ����ʱ���õĳ�ʱʱ��
    ngx_msec_t                       send_timeout;
	//������Ӧ�ĳ�ʱʱ�䡣ͨ�����Ƕ��¼���ӵ���ʱ����ʱ���õĳ�ʱʱ��
    ngx_msec_t                       read_timeout;
	//Ŀǰ������
    ngx_msec_t                       timeout;
    ngx_msec_t                       next_upstream_timeout;
	//TCP��SO_SNOLOWATѡ���ʾ���ͻ�����������
    size_t                           send_lowat;
	//�����˽���ͷ���Ļ�����������ڴ��С(ngx_http_upstream_t�е�buffer������)������ת����Ӧ
	//�����λ�����buffering��־λΪ0�������ת����Ӧʱ����ͬ����ʾ���հ���Ļ�������С
    size_t                           buffer_size;
    size_t                           limit_rate;
	//����buffering��־λΪ1������������ת����Ӧʱ��Ч���������õ�ngx_event_pipe_t�ṹ���
	//busy_size��Ա��
    size_t                           busy_buffers_size;
	//��ʱ�����ļ�����󳤶�
    size_t                           max_temp_file_size;
    size_t                           temp_file_write_size;

    size_t                           busy_buffers_size_conf;
    size_t                           max_temp_file_size_conf;
	//һ��д����ʱ�ļ����ַ�����󳤶�
    size_t                           temp_file_write_size_conf;
	//�Ի�����Ӧ�ķ�ʽת�����η������İ���ʱ���õ��ڴ��С
    ngx_bufs_t                       bufs;
	//���ngx_http_upstream_t�е�header_in��Ա��ignore_headers�ɸ���λ��������һЩͷ��
    ngx_uint_t                       ignore_headers;
    ngx_uint_t                       next_upstream;
	//��ʾ������Ŀ¼���ļ���Ȩ��
    ngx_uint_t                       store_access;
    ngx_uint_t                       next_upstream_tries;
	//����ת����Ӧ��ʽ�ı�־λ
    //1����Ϊ���ο������Σ��ᾡ�������ڴ���ߴ����л����������ε���Ӧ��
    //0����Ϊ���ο������Σ�������һ��̶���С���ڴ����Ϊ������ת����Ӧ 
    ngx_flag_t                       buffering;
    ngx_flag_t                       request_buffering;
    ngx_flag_t                       pass_request_headers;
    ngx_flag_t                       pass_request_body;

	// 1�����η���������ʱ������Ƿ������οͻ��˶Ͽ����ӣ�����ִ�н�������
    ngx_flag_t                       ignore_client_abort;
	//��ȡ�����룬�鿴�Ƿ��ж�Ӧ���Է��ص�����
    ngx_flag_t                       intercept_errors;
	// 1����ͼ������ʱ�ļ����Ѿ�ʹ�ù��Ŀռ�
    ngx_flag_t                       cyclic_temp_file;
    ngx_flag_t                       force_ranges;
	//�����ʱ�ļ���·��
    ngx_path_t                      *temp_path;
	//����ngx_http_upstream_hide_headers_hash�������������Ҫ���ص�HTTPͷ��ɢ�б�
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
	//ʹ��upstream��ģ�����ƣ������ڼ�¼��־
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
	//������¼��Ļص�������ÿһ���׶ζ��в�ͬ��read_event_handler
    ngx_http_upstream_handler_pt     read_event_handler;
	//����д�¼��Ļص�������ÿһ���׶ζ��в�ͬ��write_event_handler
    ngx_http_upstream_handler_pt     write_event_handler;

    ngx_peer_connection_t            peer;

    ngx_event_pipe_t                *pipe;
	//�洢�������η�����������Ļ�����
    ngx_chain_t                     *request_bufs;

    ngx_output_chain_ctx_t           output;
    ngx_chain_writer_ctx_t           writer;

	//ʹ��upstream����ʱ�ĸ�������
	//proxy module ָ���Ӧlocation�е�plcf->upstream
    ngx_http_upstream_conf_t        *conf;
#if (NGX_HTTP_CACHE)
    ngx_array_t                     *caches;
#endif

    ngx_http_upstream_headers_in_t   headers_in;

    ngx_http_upstream_resolved_t    *resolved;

    ngx_buf_t                        from_client;

    ngx_buf_t                        buffer;		//�������η�������������Ӧ��ͷ���Ļ�����
    //����Ҫ�������ΰ���ĳ���
    off_t                            length;

    ngx_chain_t                     *out_bufs;
    ngx_chain_t                     *busy_bufs;
    ngx_chain_t                     *free_bufs;

	//��ʼ��input filter�������ġ�nginxĬ�ϵ�input_filter_init ֱ�ӷ��ء�
    ngx_int_t                      (*input_filter_init)(void *data);
	//�����˷��������ص���Ӧ���ġ�nginxĬ�ϵ�input_filter�Ὣ�յ������ݷ�װ��Ϊ��������ngx_chain��
	//������upstream��out_bufsָ����λ�����Կ�����Ա������ģ������ͨ����ָ��õ���˷��������ص��������ݡ�
	//memcachedģ��ʵ�����Լ��� input_filter���ں�������������ģ�顣
    ngx_int_t                      (*input_filter)(void *data, ssize_t bytes);
	//
    void                            *input_filter_ctx;

#if (NGX_HTTP_CACHE)
    ngx_int_t                      (*create_key)(ngx_http_request_t *r);
#endif
	//���ɷ��͵���˷����������󻺳壨�����������ڳ�ʼ��upstream ʱʹ�á�
    ngx_int_t                      (*create_request)(ngx_http_request_t *r);
	//��ĳ̨��˷���������������nginx�᳢����һ̨��˷������� nginxѡ���µķ������Ժ�
	//���ȵ��ô˺����������³�ʼ�� upstreamģ��Ĺ���״̬��Ȼ���ٴν���upstream���ӡ�
    ngx_int_t                      (*reinit_request)(ngx_http_request_t *r);
	//�����˷��������ص���Ϣͷ������νͷ������upstream server ͨ�ŵ�Э��涨�ģ�
	//����HTTPЭ���header���֣�����memcached Э�����Ӧ״̬���֡�
    ngx_int_t                      (*process_header)(ngx_http_request_t *r);
	//�ڿͻ��˷�������ʱ�����á�����Ҫ�ں�����ʵ�ֹرպ�˷��� �����ӵĹ��ܣ�
	//ϵͳ���Զ���ɹر����ӵĲ��裬����һ��˺� ����������κξ��幤����
    void                           (*abort_request)(ngx_http_request_t *r);
	//����������˷��������������øú�������abort_request ��ͬ��һ��Ҳ��������κξ��幤����
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
	//��ʾ�Ƿ��Ѿ�������request_bufs���������ڵ�һ����request_bufs��Ϊ��������ngx_output_chain������request_sent����Ϊ1
    unsigned                         request_sent:1;
	//��־λ��Ϊ1ʱ�������η���������Ӧ��Ҫֱ��ת�����ͻ��ˣ����Ҵ�ʱNginx�Ѿ�����Ӧ��ͷת�����ͻ�����
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
