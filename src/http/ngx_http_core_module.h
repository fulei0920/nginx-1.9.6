
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_CORE_H_INCLUDED_
#define _NGX_HTTP_CORE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#if (NGX_THREADS)
#include <ngx_thread_pool.h>
#endif


#define NGX_HTTP_GZIP_PROXIED_OFF       0x0002
#define NGX_HTTP_GZIP_PROXIED_EXPIRED   0x0004
#define NGX_HTTP_GZIP_PROXIED_NO_CACHE  0x0008
#define NGX_HTTP_GZIP_PROXIED_NO_STORE  0x0010
#define NGX_HTTP_GZIP_PROXIED_PRIVATE   0x0020
#define NGX_HTTP_GZIP_PROXIED_NO_LM     0x0040
#define NGX_HTTP_GZIP_PROXIED_NO_ETAG   0x0080
#define NGX_HTTP_GZIP_PROXIED_AUTH      0x0100
#define NGX_HTTP_GZIP_PROXIED_ANY       0x0200


#define NGX_HTTP_AIO_OFF                0
#define NGX_HTTP_AIO_ON                 1
#define NGX_HTTP_AIO_THREADS            2


#define NGX_HTTP_SATISFY_ALL            0
#define NGX_HTTP_SATISFY_ANY            1


#define NGX_HTTP_LINGERING_OFF          0
#define NGX_HTTP_LINGERING_ON           1
#define NGX_HTTP_LINGERING_ALWAYS       2


#define NGX_HTTP_IMS_OFF                0
#define NGX_HTTP_IMS_EXACT              1
#define NGX_HTTP_IMS_BEFORE             2


#define NGX_HTTP_KEEPALIVE_DISABLE_NONE    0x0002
#define NGX_HTTP_KEEPALIVE_DISABLE_MSIE6   0x0004
#define NGX_HTTP_KEEPALIVE_DISABLE_SAFARI  0x0008


typedef struct ngx_http_location_tree_node_s  ngx_http_location_tree_node_t;
typedef struct ngx_http_core_loc_conf_s  ngx_http_core_loc_conf_t;


typedef struct 
{
    union 
	{
        struct sockaddr        sockaddr;
        struct sockaddr_in     sockaddr_in;
#if (NGX_HAVE_INET6)
        struct sockaddr_in6    sockaddr_in6;
#endif
#if (NGX_HAVE_UNIX_DOMAIN)
        struct sockaddr_un     sockaddr_un;
#endif
        u_char                 sockaddr_data[NGX_SOCKADDRLEN];
    } u;

    socklen_t                  socklen;

    unsigned                   set:1;
    unsigned                   default_server:1;
    unsigned                   bind:1;
    unsigned                   wildcard:1;
#if (NGX_HTTP_SSL)
    unsigned                   ssl:1;
#endif
#if (NGX_HTTP_V2)
    unsigned                   http2:1;
#endif
#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    unsigned                   ipv6only:1;
#endif
#if (NGX_HAVE_REUSEPORT)
    unsigned                   reuseport:1;
#endif
    unsigned                   so_keepalive:2;
    unsigned                   proxy_protocol:1;

    int                        backlog;
    int                        rcvbuf;
    int                        sndbuf;
#if (NGX_HAVE_SETFIB)
    int                        setfib;
#endif
#if (NGX_HAVE_TCP_FASTOPEN)
    int                        fastopen;
#endif
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
    int                        tcp_keepidle;
    int                        tcp_keepintvl;
    int                        tcp_keepcnt;
#endif

#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
    char                      *accept_filter;
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
    ngx_uint_t                 deferred_accept;
#endif
	/*ip��ַ�Ͷ˿ڵ��ַ�����ʾ -- "192.18.1.2:80"*/
    u_char                     addr[NGX_SOCKADDR_STRLEN + 1];
} ngx_http_listen_opt_t;

//ngx_http_phases�����11���׶�����˳��ģ����밴���䶨���˳��ִ�С�
//NGX_HTTP_FIND_CONFIG_PHASE��NGX_HTTP_POST_REWRITE_PHASE��NGX_HTTP_POST_ACCESS_PHASE��NGX_HTTP_TRY_FILES_PHASE
//��4���׶�������HTTPģ������Լ���ngx_http_handler_pt���������û�����������HTTP���ʵ�֡�
typedef enum 
{
//����������ͷ֮��ĵ�һ���׶Σ���λ��uri��д֮ǰ��ʵ���Ϻ�����ģ���ע���ڸý׶Σ�Ĭ�ϵ�����£��ý׶α�����
//����ͷ��ȡ���֮��Ľ׶�
    NGX_HTTP_POST_READ_PHASE = 0,	
//server�����uri��д�׶Σ�Ҳ���Ǹý׶�ִ�д���server���ڣ�location�������дָ�(�ڶ�ȡ����ͷ�Ĺ�����nginx�����host���˿��ҵ���Ӧ��������������)
//����׶���Ҫ�Ǵ���ȫ�ֵ�(server block)��rewrite��
	NGX_HTTP_SERVER_REWRITE_PHASE,	
// Ѱ��location���ý׶Σ��ý׶�ʹ����д֮���uri�����Ҷ�Ӧ��location��ֵ��ע����Ǹý׶ο��ܻᱻִ�ж�Σ���ΪҲ������location�������дָ��
    NGX_HTTP_FIND_CONFIG_PHASE,		//����׶���Ҫ��ͨ��uri�����Ҷ�Ӧ��location��Ȼ��uri��location�����ݹ�������  
//location�����uri��д�׶Σ��ý׶�ִ��location��������дָ�Ҳ���ܻᱻִ�ж�Σ�
    NGX_HTTP_REWRITE_PHASE,			
//location������д�ĺ�һ�׶Σ���������Ͻ׶��Ƿ���uri��д�������ݽ����ת�����ʵĽ׶Σ�
    NGX_HTTP_POST_REWRITE_PHASE,	//post rewrite�������Ҫ�ǽ���һЩУ���Լ���β�������Ա��ڽ��������ģ�顣
// ����Ȩ�޿��Ƶ�ǰһ�׶Σ��ý׶���Ȩ�޿��ƽ׶�֮ǰ��һ��Ҳ���ڷ��ʿ��ƣ��������Ʒ���Ƶ�ʣ��������ȣ�
    NGX_HTTP_PREACCESS_PHASE,		//���������������͵�access�ͷ������phase��Ҳ����˵����Ҫ�ǽ���һЩ�Ƚϴ����ȵ�access�� 
//����Ȩ�޿��ƽ׶Σ��������ip�ڰ�������Ȩ�޿��ƣ������û��������Ȩ�޿��Ƶȣ�
    NGX_HTTP_ACCESS_PHASE,			//��������ȡ���ƣ�Ȩ����֤�ͷ������phase��һ����˵�������ǽ��������ģ������.�����Ҫ����һЩϸ���ȵ�access��  
//����Ȩ�޿��Ƶĺ�һ�׶Σ��ý׶θ���Ȩ�޿��ƽ׶ε�ִ�н��������Ӧ����
	NGX_HTTP_POST_ACCESS_PHASE,		//һ����˵�������accessģ��õ�access_code֮��ͻ������ģ�����access_code�����в���  
//try_filesָ��Ĵ���׶Σ����û������try_filesָ���ý׶α�����
    NGX_HTTP_TRY_FILES_PHASE,	
//�������ɽ׶Σ��ý׶β�����Ӧ�������͵��ͻ��ˣ�
	NGX_HTTP_CONTENT_PHASE,			//���ݴ���ģ�飬����һ���handle���Ǵ������ģ��  
//��־��¼�׶Σ��ý׶μ�¼������־��
    NGX_HTTP_LOG_PHASE				
} ngx_http_phases;

typedef struct ngx_http_phase_handler_s  ngx_http_phase_handler_t;


//һ��HTTP����׶��е�checker��鷽������������HTTP���ʵ�֣��Դ˿���HTTP����Ĵ�������
//����ֵ 
//	NGX_OK -- ��ζ�Űѿ���Ȩ������Nginx���¼�ģ�飬���������¼�(�����¼�����ʱ���¼����첽I/O�¼���)�ٴε�������
//	��NGX_OK -- ��ζ������ִ��phase_engine�еĸ�������  
typedef ngx_int_t (*ngx_http_phase_handler_pt)(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);

struct ngx_http_phase_handler_s 
{
	//�ڴ���ĳһ��HTTP�׶�ʱ��HTTP��ܽ�����checker������ʵ�ֵ�ǰ�������ȵ���checker��������������
	//������ֱ�ӵ����κν׶��е�handler������ֻ����checker�����вŻ�ȥ����handler��������ˣ���ʵ�����е�checker
	//���������ɿ���е�ngx_http_core_moduleģ��ʵ�ֵģ�����ͨ��HTTPģ���޷��ض���checkerģ��
    ngx_http_phase_handler_pt  checker;		//����ص������������жϺ�������ͬ�׶εĽڵ������ͬ��check����*/	
	/**/
    ngx_http_handler_pt        handler;		/*ģ��ע��Ļص�����*/
	//��Ҫִ�е���һ��HTTP����׶ε����
    ngx_uint_t                 next;	
};


typedef struct 
{
	//��ngx_http_phase_handler_t���ɵ������׵�ַ������ʾһ��������ܾ��������д�����
    ngx_http_phase_handler_t  *handlers;	
	//��ʾNGX_HTTP_SERVER_REWRITE_PHASE�׶ε�һ��ngx_http_phase_handler_t��������handlers�����е���ţ�
	//������ִ��HTTP������κν׶��п�����ת��NGX_HTTP_SERVER_REWRITE_PHASE�׶δ�������
    ngx_uint_t                 server_rewrite_index;
	//��ʾNGX_HTTP_REWRITE_PHASE�׶ε�һ��ngx_http_phase_handler_t��������handlers�����е���ţ�
	//������ִ��HTTP������κν׶��п�����ת��NGX_HTTP_REWRITE_PHASE�׶δ�������
    ngx_uint_t                 location_rewrite_index;	
} ngx_http_phase_engine_t;


typedef struct 
{
	//ngx_http_handler_pt���͵Ķ�̬���飬������ÿһ��HTTPģ���ʼ����ӵ���ǰ�׶εĴ�����
    ngx_array_t                handlers;		
} ngx_http_phase_t;


typedef struct 
{
	//(ngx_http_core_srv_conf_t*)���͵Ķ�̬���飬ÿһ��Ԫ��ָ���ʾserver���ngx_http_core_srv_conf_t�ṹ��ĵ�ַ���Ӷ�����������server�� 
    ngx_array_t                servers;
	//��������׶δ��������ɵ�phases�����Ա�����Ľ׶����������ˮʽ����HTTP�����ʵ�����ݽṹ
	//�������й������Ը�HTTP������Ҫ������HTTP����׶Σ��������ngx_http_request_t�ṹ���е�phase_handlerʹ��
    ngx_http_phase_engine_t    phase_engine;

    ngx_hash_t                 headers_in_hash;

    ngx_hash_t                 variables_hash;

    ngx_array_t                variables;       /* array of ngx_http_variable_t */
    ngx_uint_t                 ncaptures;
	//��ʾ�洢����server_name��ɢ�б��ɢ��Ͱ�ĸ���
    ngx_uint_t                 server_names_hash_max_size;
	//��ʾ�洢����server_name��ɢ�б��ÿ��ɢ��Ͱռ�õ��ڴ��С
    ngx_uint_t                 server_names_hash_bucket_size;

    ngx_uint_t                 variables_hash_max_size;
    ngx_uint_t                 variables_hash_bucket_size;

    ngx_hash_keys_arrays_t    *variables_keys;  /* ��nameΪkey����ngx_http_variable_tΪvalue�ı�*/

    ngx_array_t               *ports;			//����Ÿ�http{}���ÿ��¼���������ngx_http_conf_port_t�˿�

    ngx_uint_t                 try_files;       /* unsigned  try_files:1 */

	//������HTTP��ܳ�ʼ��ʱ��������HTTPģ��������׶������HTTP��������
	//����һ����11����Ա��ngx_http_phase_t���飬����ÿһ��ngx_http_phase_t�ṹ���Ӧ
	//һ��HTTP�׶Ρ���HTTP��ܳ�ʼ����Ϻ����й����е�phases���������õ�
    ngx_http_phase_t           phases[NGX_HTTP_LOG_PHASE + 1];  	
} ngx_http_core_main_conf_t;


typedef struct 
{	
	//�ڿ�ʼ����һ��HTTP����ʱ��Nginx��ȡ��header�е�Host����ÿ��server�е�server_name����ƥ�䣬
	//�Դ˾�����������һ��server���������������
	//��һ��Host����server���е�sever_name��ƥ�䣬������server_name��Hostƥ�����ȼ���ѡ��ʵ�ʴ����server��
	//1>ѡ���ַ�����ȫƥ���server_name����www.testweb.com
	//2>ѡ��ͨ�����ǰ���server_name����*.testweb.com
	//3>ѡ��ͨ����ں����server_name����www.testweb.*
	//4>ѡ��ʹ��������ʽ��ƥ���server_name����~^\.testweb\.com$
	//���Host�����е�server_name����ƥ�䣬����һ��˳��ѡ�����server��
	//1>ѡ����listen����������[default | default_server]��server��
	//2>ѡ��ƥ��listen�˿ڵĵ�һ��server��
    ngx_array_t                 server_names;  		/* ngx_http_server_name_t, �����ڸ�server������ص�����sever name*/
	//ָ��ǰserver��������ngx_http_conf_ctx_t�ṹ��
    ngx_http_conf_ctx_t        *ctx;	
	//��ǰserver���������������������ڵĻ��������http�����е�Hostͷ����ƥ�䣬
	//ƥ���Ϻ����ɵ�ǰngx_http_core_srv_conf_t��������
    ngx_str_t                   server_name;		
	//ָ��ÿ��TCP�����ϳ�ʼ�ڴ�ش�С
    size_t                      connection_pool_size;	
	//ָ��ÿ��HTTP����ĳ�ʼ�ڴ�ش�С�����ڼ����ں˶���С���ڴ�ķ������
	//TCP���ӹر�ʱ������connection_pool_sizeָ���������ڴ�أ�HTTP�������ʱ��
	//����request_pool_sizeָ����HTTP�����ڴ�أ������ǵĴ���������ʱ�䲢��һ�£�
	//��Ϊһ��TCP���ӿ��ܱ���������HTTP����
    size_t                      request_pool_size;
	//�洢HTTPͷ��(�����к�����ͷ)ʱ������ڴ�buffer��С
    size_t                      client_header_buffer_size;

    ngx_bufs_t                  large_client_header_buffers;
	//��ȡHTTPͷ��(�����к�����ͷ)�ĳ�ʱʱ��
    ngx_msec_t                  client_header_timeout;
	//�����������Ϊoff����ô�����ֲ��ͷ���HTTPͷ��ʱ��
	//Nginx��ܾ����񣬲�ֱ�����û�����400(Bad Request)����
	//�����������Ϊon�������Դ�HTTPͷ��
    ngx_flag_t                  ignore_invalid_headers;
	//�Ƿ�ϲ����ڵ�'/'
	//����: //test///a.txt��������Ϊonʱ���Ὣ��ƥ��Ϊ location /test/a.txt;
	//	�������Ϊoff���򲻻�ƥ�䣬URI�Խ���//test///a.txt
    ngx_flag_t                  merge_slashes;
	
    ngx_flag_t                  underscores_in_headers; //�Ƿ�����HTTPͷ���������д�'_'(�»���)

    unsigned                    listen:1;			//��ʾ�Ƿ�����listen ָ��
#if (NGX_PCRE)
    unsigned                    captures:1;
#endif

    ngx_http_core_loc_conf_t  **named_locations;
} ngx_http_core_srv_conf_t;


/* list of structures to find core_srv_conf quickly at run time */


typedef struct
{
#if (NGX_PCRE)
    ngx_http_regex_t          *regex;
#endif
    ngx_http_core_srv_conf_t  *server;   /* virtual name server conf */
    ngx_str_t                  name;
} ngx_http_server_name_t;


typedef struct {
     ngx_hash_combined_t       names;

     ngx_uint_t                nregex;
     ngx_http_server_name_t   *regex;
} ngx_http_virtual_names_t;


struct ngx_http_addr_conf_s
{
    /* the default server configuration for this address:port */
	//���������ַ��Ӧ��server��������Ϣ
    ngx_http_core_srv_conf_t  *default_server;

    ngx_http_virtual_names_t  *virtual_names;

#if (NGX_HTTP_SSL)
    unsigned                   ssl:1;
#endif
#if (NGX_HTTP_V2)
    unsigned                   http2:1;
#endif
    unsigned                   proxy_protocol:1;
};


typedef struct 
{
	/*IP��ַ�������ֽ���*/
    in_addr_t                  addr;  
    ngx_http_addr_conf_t       conf;
} ngx_http_in_addr_t;


#if (NGX_HAVE_INET6)

typedef struct 
{
    struct in6_addr            addr6;
    ngx_http_addr_conf_t       conf;
} ngx_http_in6_addr_t;

#endif


typedef struct 
{
    //ngx_http_in_addr_t or ngx_http_in6_addr_t 
    void                      *addrs;
	//addrsָ��Ķ����Ԫ�صĸ���
    ngx_uint_t                 naddrs;		
} ngx_http_port_t;


typedef struct
{
    ngx_int_t                  family;		//socket��ַ����
    in_port_t                  port;		//�����˿�(�����ֽ���)
    ngx_array_t                addrs;     	//�����˿��¶�Ӧ�ŵ�����ngx_http_conf_addr_t��ַ
} ngx_http_conf_port_t;



//ÿһ��ngx_http_conf_addr_t������һ��ngx_listening_t�������Ӧ
typedef struct 
{
    ngx_http_listen_opt_t      opt;			//�����׽��ֵĸ�������

	//����3��ɢ�б����ڼ���Ѱ�ҵ���Ӧ�����˿��ϵ������ӣ�ȷ������ʹ���ĸ�server{}���������µ���������������
	//����ɢ�б��ֵ����ngx_http_core_srv_conf_t�ṹ��ĵ�ַ
    ngx_hash_t                 hash;		//��ȫƥ��server name��ɢ�б�
    ngx_hash_wildcard_t       *wc_head;		//ͨ���ǰ�õ�ɢ�б�
    ngx_hash_wildcard_t       *wc_tail;		//ͨ������õ�ɢ�б�

#if (NGX_PCRE)
    ngx_uint_t                 nregex;		//��Աregex�����е�Ԫ�ظ���
    ngx_http_server_name_t    *regex;		//ָ��̬���飬�������Ա����ngx_http_server_name�ṹ�壬��ʾ������ʽ����ƥ���server{}�����
#endif

    /* the default server configuration for this address:port */
    ngx_http_core_srv_conf_t  *default_server;	//�ü����˿��¶�Ӧ��Ĭ��server{}��������
    ngx_array_t                servers;  	/* array of (ngx_http_core_srv_conf_t*), ��¼��ͬIP�µĲ�ͬserver{}����*/
} ngx_http_conf_addr_t;


typedef struct {
    ngx_int_t                  status;
    ngx_int_t                  overwrite;
    ngx_http_complex_value_t   value;
    ngx_str_t                  args;
} ngx_http_err_page_t;


typedef struct 
{
    ngx_array_t               *lengths;
    ngx_array_t               *values;
    ngx_str_t                  name;

    unsigned                   code:10;
    unsigned                   test_dir:1;
} ngx_http_try_file_t;

//������������һ��location�����Ϣ��
//�Լ�ƥ�����location�������http����һЩ��Դ��Ϣ
struct ngx_http_core_loc_conf_s 
{
	//location���ƣ���locationָ���ı��ʽ
    ngx_str_t     name;        
#if (NGX_PCRE)
    ngx_http_regex_t  *regex;
#endif
    unsigned      noname:1;   			/* "if () {}" block or limit_except */
    unsigned      lmt_excpt:1;
	//����location��������server�ڲ���ת
    unsigned      named:1;				
	//����ƥ��
    unsigned      exact_match:1;		
    unsigned      noregex:1;

    unsigned      auto_redirect:1;
#if (NGX_HTTP_GZIP)
    unsigned      gzip_disable_msie6:2;
#if (NGX_HTTP_DEGRADATION)
    unsigned      gzip_disable_degradation:2;
#endif
#endif

    ngx_http_location_tree_node_t   *static_locations;
#if (NGX_PCRE)
    ngx_http_core_loc_conf_t       **regex_locations;
#endif

    //ָ������location����ngx_http_conf_ctx_t�ṹ���е�loc_confָ�����飬�������ŵ�ǰlocation��������HTTPģ��create_loc_conf���������Ľṹ��ָ��
    void        **loc_conf;		

    uint32_t      limit_except;
    void        **limit_except_loc_conf;

    ngx_http_handler_pt  handler;

    /* location name length for inclusive location with inherited alias */
    size_t        alias;
    ngx_str_t     root;                    /* root, alias */
    ngx_str_t     post_action;

    ngx_array_t  *root_lengths;
    ngx_array_t  *root_values;
	/* array of ngx_hash_key_t 
	[config] text/html   html htm shtml;
	[result] key -- html value -- text/html 
	[result] key -- htm value -- text/html
	[result] key -- shtml value -- text/html
	*/
    ngx_array_t  *types;    		/*array of ngx_hash_key_t*/   			
    ngx_hash_t    types_hash;
	//���Ҳ�����Ӧ��MIME type���ļ���չ��֮���ӳ��ʱ��
	//ʹ��Ĭ�ϵ�MIME type��ΪHTTP header�е�Content-type
    ngx_str_t     default_type;   /*[config]defatult_type  application/octet-stream*/
	//HTTP�����������ֵ
    off_t         client_max_body_size;   
	//ָ����FreeBSD��Linuxϵͳ��ʹ��O_DIRECTѡ��ȥ��ȡ�ļ�ʱ��
	//�������Ĵ�С��ͨ���Դ��ļ��Ķ�ȡ�ٶ����Ż����á�
	//ע�⣬����sendfile�����ǻ����
    off_t         directio;                
	//��directio���ʹ�ã�ָ����directio��ʽ��ȡ�ļ�ʱ�Ķ��뷽ʽ��
	//һ�������512B�Ѿ��㹻�ˣ������һЩ�������ļ�ϵͳ����Linux�µ�XFS�ļ�ϵͳ��
	//������Ҫ���õ�4KB��Ϊ���뷽ʽ
    off_t         directio_alignment;      /* directio_alignment */
	//�洢HTTP������ڴ滺������С
	//HTTP������Ƚ��յ�ָ������黺���У�֮��ž����Ƿ�д�����
	//����û������а�����HTTPͷ��Content-Length���������ʶ�ĳ���С�ڶ���Ļ�������
	//��ôNginx���Զ����ͱ���������ʹ�õ��ڴ�buffer���Խ����ڴ�����
    size_t        client_body_buffer_size; /* client_body_buffer_size */
    size_t        send_lowat;              /* send_lowat */
    size_t        postpone_output;         /* postpone_output */
	//�Կͻ�����������ÿ�봫����ֽ�����0��ʾ������
    size_t        limit_rate;   
	//��ͻ��˷��͵���Ӧ���ȳ���limit_rate_after��ſ�ʼ����
    size_t        limit_rate_after;        
    size_t        sendfile_max_chunk;      /* sendfile_max_chunk */
    size_t        read_ahead;              /* read_ahead */
	//��ȡHTTP����ĳ�ʱʱ��
    ngx_msec_t    client_body_timeout;     /* client_body_timeout */
	//������Ӧ�ĳ�ʱʱ��
    ngx_msec_t    send_timeout;            /* send_timeout */
	//keepalive��ʱʱ��
    ngx_msec_t    keepalive_timeout;       
	//�����ӳٹر�״̬�������ʱ��
    ngx_msec_t    lingering_time;          
	//�ӳٹرյĳ�ʱʱ��
	//�ӳٹر�״̬�У�������lingering_timeoutʱ���û�����ݿɶ�����ֱ�ӹر�����
    ngx_msec_t    lingering_timeout;       /* lingering_timeout */
	//DNS�����ĳ�ʱʱ��
    ngx_msec_t    resolver_timeout;        

    ngx_resolver_t  *resolver;             /* resolver */
	//Nginx���͸��ͻ�����Ӧͷ�е�Keep-Alive���ʱ��
    time_t        keepalive_header;        
	//һ��keepalive��������������ص����������Ŀ
    ngx_uint_t    keepalive_requests;      /* keepalive_requests */
	//��ĳЩ���������keepalive����
	//HTTP�����е�keepalive������Ϊ���ö��������һ��HTTP������
	//������Щ������Գ����Ӵ���������
    ngx_uint_t    keepalive_disable;       
	//����ȡֵΪNGX_HTTP_SATISFY_ALL����NGX_HTTP_SATISFY_ANY
    ngx_uint_t    satisfy;                 /* satisfy */
	//����Nginx�ر��û����ӵķ�ʽ
	//always��ʾ�ر��û�����ǰ�����������ش��������������û����͵�����
	//off��ʾ�ر�����ʱ��ȫ�����������Ƿ��Ѿ���׼�������������û�������
	//on���м�ֵ��һ��������ڹر�����ǰ���ᴦ�������ϵ��û����͵����ݣ�
	//������Щ�������ҵ�����϶���֮��������ǲ���Ҫ��
    ngx_uint_t    lingering_close;         /* lingering_close */
	//��If_Modified_Sinceͷ���Ĵ������
	//�������ܵĿ��ǣ�Web�����һ����ڿͻ��˱��ػ���һЩ�ļ������洢��ʱ��ȡ��ʱ�䡣
	//�������´���Web��������ȡ���������Դʱ���Ϳ�����If-Modified-Sinceͷ�����ϴλ�ȡ
	//��ʱ���Ӵ��ϣ���Nginx������if_modified_since��ֵ��������δ���If-Modified-Sinceͷ��
	//off--��ʾ�����û������е�If_Modified_Sinceͷ������ʱ�������ȡһ���ļ���
	//	��ô�������ط����ļ����ݡ�HTTP��Ӧ��ͨ����200
	//exact--��If_Modified_Sinceͷ��������ʱ���뽫Ҫ���ص��ļ��ϴ��޸ĵ�ʱ������ȷ�Ƚϣ�
	//	���û��ƥ���ϣ��򷵻�200���ļ���ʵ�����ݣ����ƥ���ϣ����ʾ�����������ļ������Ѿ������µ��ˣ�
	//	û�б�Ҫ�ٷ����ļ��Ӷ��˷�ʱ��������ˣ���ʱ�᷵��304 Not Modified��������յ����ֱ�Ӷ�ȡ�Լ��ı��ػ���
	//before--�Ǳ�exact�����ɵıȽϡ�ֻҪ�ļ����ϴ��޸�ʱ����ڻ������û������е�If_Modified_Sinceͷ��ʱ�䣬
	//	�ͻ���ͻ��˷���304 Not Modified��
    ngx_uint_t    if_modified_since;       
    ngx_uint_t    max_ranges;              /* max_ranges */
    ngx_uint_t    client_body_in_file_only; /* client_body_in_file_only */

	//
    ngx_flag_t    client_body_in_single_buffer;
                                           /* client_body_in_singe_buffer */
	//�ڲ�location������locationֻ��������������ڲ���ת����
    ngx_flag_t    internal;                /* internal */
	//ȷ���Ƿ�����sendfileϵͳ�����������ļ�
	//sendfileϵͳ���ü������ں�̬���û�̬֮��������ڴ渴�ƣ������ͻ�Ӵ����ж�ȡ�ļ���ֱ��
	//���ں�̬���͵������豸������˷����ļ���Ч��
    ngx_flag_t    sendfile;        

	//�Ƿ���FreeBSD��Linuxϵͳ�������ں˼�����첽�ļ�I/O���ܡ�
	//ע��: ����sendfile�����ǻ����
    ngx_flag_t    aio;                   
	
	//ȷ���Ƿ���FreeBSDϵͳ�ϵ�TCP_NOPUSH��Linuxϵͳ�ϵ�TCP_CORK���ܡ�
	//����ʹ��sendfile��ʱ�����Ч
	//��tcp_nopush�󣬽����ڷ�����Ӧʱ��������Ӧ��ͷ�ŵ�һ��TCP���з���
    ngx_flag_t    tcp_nopush;     
	
	//������ر�Nginxʹ��TCP_NODELAYѡ��Ĺ��� 
	//���ѡ���ڽ�����ת��Ϊ�����ӵ�ʱ��ű����ã���upstream������Ӧ���ͻ���ʱҲ������
    ngx_flag_t    tcp_nodelay;      
	
	//���ӳ�ʱ��ͨ����ͻ��˷���RST����ֱ����������
	//��������Ĺرշ�ʽ����ʹ�÷��������������ദ��FIN_WAIT_1,FIN_WAIT_2/TIME_WAIT״̬��TCP����
	//ע�⣬ʹ��RST���ð��ر����ӻ����һЩ���⣬Ĭ������²��Ὺ��
    ngx_flag_t    reset_timedout_connection; 
	//�ض����������ƵĴ�����Ҫ���server_name����ʹ��
	//on--��ʾ���ض�������ʱ��ʹ��server_name�����õĵ�һ������������ԭ�������Hostͷ����
	//off--��ʾ���ض�������ʱʹ���������Hostͷ��
    ngx_flag_t    server_name_in_redirect; 
    ngx_flag_t    port_in_redirect;        /* port_in_redirect */
    ngx_flag_t    msie_padding;            /* msie_padding */
    ngx_flag_t    msie_refresh;            /* msie_refresh */
	//�������û���������Ҫ�����ļ�ʱ�����û���ҵ��ļ����Ƿ񽫴�����־
	//��¼��erro.log�ļ��С�������ڶ�λ����
	ngx_flag_t    log_not_found;           
    ngx_flag_t    log_subrequest;          /* log_subrequest */
    ngx_flag_t    recursive_error_pages;   /* recursive_error_pages */
	//�����������ʱ�Ƿ�����Ӧ��Serverͷ���б���Nginx�汾������Ϊ�˷�������
    ngx_flag_t    server_tokens;          
    ngx_flag_t    chunked_transfer_encoding; /* chunked_transfer_encoding */
    ngx_flag_t    etag;                    /* etag */

#if (NGX_HTTP_GZIP)
    ngx_flag_t    gzip_vary;               /* gzip_vary */

    ngx_uint_t    gzip_http_version;       /* gzip_http_version */
    ngx_uint_t    gzip_proxied;            /* gzip_proxied */

#if (NGX_PCRE)
    ngx_array_t  *gzip_disable;            /* gzip_disable */
#endif
#endif

#if (NGX_THREADS)
    ngx_thread_pool_t         *thread_pool;
    ngx_http_complex_value_t  *thread_pool_value;
#endif

#if (NGX_HAVE_OPENAT)
    ngx_uint_t    disable_symlinks;        /* disable_symlinks */
    ngx_http_complex_value_t  *disable_symlinks_from;
#endif

    ngx_array_t  *error_pages;             /* error_page */
    ngx_http_try_file_t    *try_files;     /* try_files */

	//HTTP�������ʱ���Ŀ¼
    ngx_path_t   *client_body_temp_path;   /* client_body_temp_path */

	//�ļ����棬ͨ����ȡ����ͼ����˶Դ��̵Ĳ���
    ngx_open_file_cache_t  *open_file_cache;
	//�����ļ�������Ԫ����Ч�Ե�Ƶ��
    time_t        open_file_cache_valid;
	//ָ���ļ������У���inactiveָ����ʱ����ڣ����ʴ���������
	//open_file_cache_min_usesָ������С��������ô�����ᱻ��̭������
    ngx_uint_t    open_file_cache_min_uses;
	//�Ƿ����ļ������л�����ļ�ʱ���ֵ��Ҳ���·����û��Ȩ�޵ȴ�����Ϣ
    ngx_flag_t    open_file_cache_errors;
    ngx_flag_t    open_file_cache_events;

    ngx_log_t    *error_log;
	//Ϊ�˿���Ѱ�ҵ���ӦMIME type��Nginxʹ��ɢ�б����洢�ļ���չ����MIME type��ӳ��
	//types_hash_max_sizeɢ�б��еĳ�ͻͰ�ĸ���
    ngx_uint_t    types_hash_max_size;
	//Ϊ�˿���Ѱ�ҵ���ӦMIME type��Nginxʹ��ɢ�б����洢�ļ���չ����MIME type��ӳ��
	//types_hash_bucket_size������ɢ�б���ÿ����ͻͰͰռ�õ��ڴ��С
    ngx_uint_t    types_hash_bucket_size;

	//���ڵ�ǰ�������location��ͨ��ngx_http_location_queue_t�ṹ�幹�ɵ�˫������
    ngx_queue_t  *locations;   		//ngx_http_location_queue_t���͵�˫������ͬһ��server���ڶ�����location���ngx_http_core_loc_conf_t�ṹ����˫������ʽ��֯����

#if 0
    ngx_http_core_loc_conf_t  *prev_location;
#endif
};


typedef struct
{
    ngx_queue_t                      queue;  	//queue����Ϊngx_queue_t˫�������������Ӷ���ngx_http_location_queue_t�ṹ����������
    ngx_http_core_loc_conf_t        *exact;		//���location�е��ַ������Ծ�ȷƥ��(����������ʽ)��exact��ָ���Ӧ��ngx_http_core_loc_conf_t�ṹ�壬����ֵΪNULL
    ngx_http_core_loc_conf_t        *inclusive;	//���location�е��ַ����޷���ȷƥ��(�������Զ����ͨ���),inclusizve��ָ���Ӧ��ngx_http_core_loc_conf_t�ṹ�壬����ֵΪNULL
    ngx_str_t                       *name;		//ָ��location������
    u_char                          *file_name;
    ngx_uint_t                       line;
    ngx_queue_t                      list;
} ngx_http_location_queue_t;


struct ngx_http_location_tree_node_s 
{
    ngx_http_location_tree_node_t   *left;		//������
    ngx_http_location_tree_node_t   *right;		//������
    ngx_http_location_tree_node_t   *tree;		//�޷���ȫƥ���location��ɵ���

    ngx_http_core_loc_conf_t        *exact;		//���location��Ӧ��URIƥ���ַ��������ܹ���ȫƥ������ͣ���exactָ�����Ӧ��ngx_http_core_loc_conf_t�ṹ�壬����ΪNULL
    //���location��Ӧ��URIƥ���ַ��������޷���ȫƥ������ͣ���inclusiveָ�����Ӧ��ngx_http_core_loc_conf_t�ṹ�壬����ΪNULL
    ngx_http_core_loc_conf_t        *inclusive;	
	//�Զ��ض����־
    u_char                           auto_redirect;	
	//name�ַ���ʵ�ʳ���
    u_char                           len;		
	//nameָ��location��Ӧ��URIƥ����ʽ
    u_char                           name[1];	
};


void ngx_http_core_run_phases(ngx_http_request_t *r);
ngx_int_t ngx_http_core_generic_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_rewrite_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_find_config_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_post_rewrite_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_access_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_post_access_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_try_files_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_content_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);


void *ngx_http_test_content_type(ngx_http_request_t *r, ngx_hash_t *types_hash);
ngx_int_t ngx_http_set_content_type(ngx_http_request_t *r);
void ngx_http_set_exten(ngx_http_request_t *r);
ngx_int_t ngx_http_set_etag(ngx_http_request_t *r);
void ngx_http_weak_etag(ngx_http_request_t *r);
ngx_int_t ngx_http_send_response(ngx_http_request_t *r, ngx_uint_t status, ngx_str_t *ct, ngx_http_complex_value_t *cv);
u_char *ngx_http_map_uri_to_path(ngx_http_request_t *r, ngx_str_t *name,
    size_t *root_length, size_t reserved);
ngx_int_t ngx_http_auth_basic_user(ngx_http_request_t *r);
#if (NGX_HTTP_GZIP)
ngx_int_t ngx_http_gzip_ok(ngx_http_request_t *r);
#endif


ngx_int_t ngx_http_subrequest(ngx_http_request_t *r,
    ngx_str_t *uri, ngx_str_t *args, ngx_http_request_t **sr,
    ngx_http_post_subrequest_t *psr, ngx_uint_t flags);
ngx_int_t ngx_http_internal_redirect(ngx_http_request_t *r,
    ngx_str_t *uri, ngx_str_t *args);
ngx_int_t ngx_http_named_location(ngx_http_request_t *r, ngx_str_t *name);


ngx_http_cleanup_t *ngx_http_cleanup_add(ngx_http_request_t *r, size_t size);


typedef ngx_int_t (*ngx_http_output_header_filter_pt)(ngx_http_request_t *r);
typedef ngx_int_t (*ngx_http_output_body_filter_pt)(ngx_http_request_t *r, ngx_chain_t *chain);
typedef ngx_int_t (*ngx_http_request_body_filter_pt) (ngx_http_request_t *r, ngx_chain_t *chain);


ngx_int_t ngx_http_output_filter(ngx_http_request_t *r, ngx_chain_t *chain);
ngx_int_t ngx_http_write_filter(ngx_http_request_t *r, ngx_chain_t *chain);
ngx_int_t ngx_http_request_body_save_filter(ngx_http_request_t *r,
   ngx_chain_t *chain);


ngx_int_t ngx_http_set_disable_symlinks(ngx_http_request_t *r,
    ngx_http_core_loc_conf_t *clcf, ngx_str_t *path, ngx_open_file_info_t *of);

ngx_int_t ngx_http_get_forwarded_addr(ngx_http_request_t *r, ngx_addr_t *addr,
    ngx_array_t *headers, ngx_str_t *value, ngx_array_t *proxies,
    int recursive);


extern ngx_module_t  ngx_http_core_module;

extern ngx_uint_t ngx_http_max_module;

extern ngx_str_t  ngx_http_core_get_method;


#define ngx_http_clear_content_length(r)                                      \
                                                                              \
    r->headers_out.content_length_n = -1;                                     \
    if (r->headers_out.content_length) {                                      \
        r->headers_out.content_length->hash = 0;                              \
        r->headers_out.content_length = NULL;                                 \
    }

#define ngx_http_clear_accept_ranges(r)                                       \
                                                                              \
    r->allow_ranges = 0;                                                      \
    if (r->headers_out.accept_ranges) {                                       \
        r->headers_out.accept_ranges->hash = 0;                               \
        r->headers_out.accept_ranges = NULL;                                  \
    }

#define ngx_http_clear_last_modified(r)                                       \
                                                                              \
    r->headers_out.last_modified_time = -1;                                   \
    if (r->headers_out.last_modified) {                                       \
        r->headers_out.last_modified->hash = 0;                               \
        r->headers_out.last_modified = NULL;                                  \
    }

#define ngx_http_clear_location(r)                                            \
                                                                              \
    if (r->headers_out.location) {                                            \
        r->headers_out.location->hash = 0;                                    \
        r->headers_out.location = NULL;                                       \
    }

#define ngx_http_clear_etag(r)                                                \
                                                                              \
    if (r->headers_out.etag) {                                                \
        r->headers_out.etag->hash = 0;                                        \
        r->headers_out.etag = NULL;                                           \
    }


#endif /* _NGX_HTTP_CORE_H_INCLUDED_ */
