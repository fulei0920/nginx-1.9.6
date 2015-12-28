
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CONNECTION_H_INCLUDED_
#define _NGX_CONNECTION_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

/*
����һ�������׽���
*/
typedef struct ngx_listening_s  ngx_listening_t;
struct ngx_listening_s 
{
    ngx_socket_t        fd;					/*�ļ�������,��socket*/  
    struct sockaddr    *sockaddr;
    socklen_t           socklen;    		/* size of sockaddr */
    size_t              addr_text_max_len;
    ngx_str_t           addr_text;
    int                 type;				/* type of socket*/	
    int                 backlog;
    int                 rcvbuf;				/*���ջ�������С*/  
    int                 sndbuf;				/*���ͻ�������С*/  
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
    int                 keepidle;
    int                 keepintvl;
    int                 keepcnt;
#endif

    /* handler of accepted connection */
    ngx_connection_handler_pt   handler;
	//HTTP -- ngx_http_port_t(ָ�������һ�˿ڵ����м�����ַ����ÿ��������ַҲ������������server���������)
    void               *servers;    
    ngx_log_t           log;
    ngx_log_t          *logp;

    size_t              pool_size;
    /* should be here because of the AcceptEx() preread */
    size_t              post_accept_buffer_size;
    /* should be here because of the deferred accept */
    ngx_msec_t          post_accept_timeout;

    ngx_listening_t    *previous;
    ngx_connection_t   *connection;

    ngx_uint_t          worker;  		/*�ü����׽���������worker���̵ı��*/

    unsigned            open:1;
    unsigned            remain:1;
    unsigned            ignore:1;				/* ���Ը��׽��� */

    unsigned            bound:1;       			/* already bound */
    unsigned            inherited:1;   			/* inherited from previous process */
    unsigned            nonblocking_accept:1;
    unsigned            listen:1;				/* listen״̬*/
    unsigned            nonblocking:1;
    unsigned            shared:1;    			/* shared between threads or processes */
    unsigned            addr_ntop:1;

#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    unsigned            ipv6only:1;
#endif
#if (NGX_HAVE_REUSEPORT)
    unsigned            reuseport:1;			/* reuseport״̬*/	
    unsigned            add_reuseport:1;       	/* �Ƿ�����reuseport*/
#endif
    unsigned            keepalive:2;

#if (NGX_HAVE_DEFERRED_ACCEPT)
    unsigned            deferred_accept:1;   	/*����TCP_DEFER_ACCEPT��־*/
    unsigned            delete_deferred:1;
    unsigned            add_deferred:1;
#ifdef SO_ACCEPTFILTER
    char               *accept_filter;
#endif
#endif
#if (NGX_HAVE_SETFIB)
    int                 setfib;
#endif

#if (NGX_HAVE_TCP_FASTOPEN)
    int                 fastopen;
#endif

};


typedef enum 
{
     NGX_ERROR_ALERT = 0,
     NGX_ERROR_ERR,
     NGX_ERROR_INFO,
     NGX_ERROR_IGNORE_ECONNRESET,
     NGX_ERROR_IGNORE_EINVAL
} ngx_connection_log_error_e;


typedef enum 
{
     NGX_TCP_NODELAY_UNSET = 0,
     NGX_TCP_NODELAY_SET,
     NGX_TCP_NODELAY_DISABLED
} ngx_connection_tcp_nodelay_e;


typedef enum 
{
     NGX_TCP_NOPUSH_UNSET = 0,
     NGX_TCP_NOPUSH_SET,
     NGX_TCP_NOPUSH_DISABLED
} ngx_connection_tcp_nopush_e;


#define NGX_LOWLEVEL_BUFFERED  0x0f
#define NGX_SSL_BUFFERED       0x01
#define NGX_HTTP_V2_BUFFERED   0x02

//(��������)��ʾ�ͻ�����������ģ�Nginx�������������ܵ�TCP����
struct ngx_connection_s 
{
	//����δʹ��ʱ��data��Ա���ڳ䵱���ӳ��п������������е�nextָ�롣
	//�����ӱ�ʹ��ʱ��data��������ʹ������nginxģ�����������HTTP����У�dataָ��ngx_http_request_t����
    void               *data;
    ngx_event_t        *read;			//���Ӷ�Ӧ�Ķ��¼�
    ngx_event_t        *write;			//���Ӷ�Ӧ��д�¼�
    ngx_socket_t        fd;    			//���Ӷ�Ӧ���׽��־��
    ngx_recv_pt         recv;			//ֱ�ӽ��������ַ����ķ���������ϵͳ�����Ĳ�ָͬ��ͬ�ĺ���
    ngx_send_pt         send;			//ֱ�ӷ��������ַ����ķ���������ϵͳ�����Ĳ�ָͬ��ͬ�ĺ���
    ngx_recv_chain_pt   recv_chain;		//��ngx_chain_t����Ϊ���������������ֽ����ķ���
    ngx_send_chain_pt   send_chain;		//��ngx_chain_t����Ϊ���������������ֽ����ķ���

    ngx_listening_t    *listening;		//���Ӷ�Ӧ��ngx_listening_t�������󣬴�������listening�����Կڵ��¼�����

	//�������Ѿ����ͳ�ȥ���ֽ���
    off_t               sent;			

    ngx_log_t          *log;			//���Լ�¼��־��ngx_log_t����

    ngx_pool_t         *pool;

    struct sockaddr    *sockaddr;		//���ӿͻ��˵�sockaddr�ṹ��		
    socklen_t           socklen;		//sockaddr�ṹ��ĳ���		
    ngx_str_t           addr_text;		//���ӿͻ����ַ�����ʽ��ip��ַ		
    ngx_str_t           proxy_protocol_addr;	//

#if (NGX_SSL)
    ngx_ssl_connection_t  *ssl;
#endif

    struct sockaddr    *local_sockaddr;		//�����ļ����˿ڶ�Ӧ��sockaddr�ṹ�壬Ҳ����listening���������е�sockaddr��Ա
    socklen_t           local_socklen;
	//���ڽ��ա�����ͻ��˷������ֽ�����ÿ���¼�����ģ������ɾ��������ӳ��з�����Ŀռ�����ֶΡ�
	//���磬��HTTPģ���У����Ĵ�С������client_header_buffer_size������	
    ngx_buf_t          *buffer;		
	//������������˫������Ԫ�ص���ʽ��ӵ�ngx_cycle_t���Ľṹ���reuseable_connections_queue˫�������У���ʾ�������õ�����
    ngx_queue_t         queue;		

    ngx_atomic_uint_t   number;		//����ʹ�ô�����ngx_connection_t�ṹ��ÿ�ν���һ�����Կͻ��˵����ӣ����������������˷�������������ʱ(ngx_peer_connection_sҲʹ����)��number����� 1
	//������������
    ngx_uint_t          requests;	

    unsigned            buffered:8;

    unsigned            log_error:3;     		/* ngx_connection_log_error_e */
	//��־λ��Ϊ 1ʱ��ʾ���ڴ��ַ���������Ŀǰ������
    unsigned            unexpected_eof:1;	
	//��־λ��Ϊ 1ʱ��ʾ�����ѳ�ʱ
    unsigned            timedout:1;	
	//��־λ��Ϊ 1ʱ��ʾ���Ӵ�������г��ִ���
    unsigned            error:1;	
	//��־λ��Ϊ 1ʱ��ʾ�����Ѿ����١����������ָ����TCP���ӣ�������ngx_connection_t�ṹ�塣
	//��destroyedΪ 1ʱ���ṹ����Ȼ���ڣ������Ӧ���׽��֡��ڴ�ص��Ѿ�������
    unsigned            destroyed:1;		
	//��־λ��Ϊ 1ʱ��ʾ���Ӵ��ڿ���״̬����keepalive��������������֮���״̬
    unsigned            idle:1;				
    unsigned            reusable:1;			//��־λ��Ϊ 1ʱ��ʾ���ӿ����ã����������queue�ֶ�ʱ��Ӧʹ�õ�
    unsigned            close:1;			//��־λ��Ϊ 1ʱ��ʾ���ӹر�

    unsigned            sendfile:1;			//��־λ��Ϊ 1ʱ��ʾ�����ļ��е����ݷ������ӵ���һ��
    //��־λ��Ϊ 1ʱ��ʾֻ���������׽��ֶ�Ӧ�ķ��ͻ�������������������õĴ�С��ֵʱ���¼�����ģ��Ż�ַ����¼���
    //��ngx_handle_write_event�����е�lowat�����Ƕ�Ӧ��
    unsigned            sndlowat:1;
	//��־λ����ʾ���ʹ��TCP��nodelay���ԡ�ȡֵ��Χ��ö������ ngx_connection_tcp_nodelay_e
    unsigned            tcp_nodelay:2;   
	//��־λ����ʾ���ʹ��TCP��nodelay���ԡ�ȡֵ��Χ��ö������ ngx_connection_tcp_nopush_e
    unsigned            tcp_nopush:2;    	

    unsigned            need_last_buf:1;

#if (NGX_HAVE_IOCP)
    unsigned            accept_context_updated:1;	
#endif

#if (NGX_HAVE_AIO_SENDFILE)
    unsigned            busy_count:2;
#endif

#if (NGX_THREADS)
    ngx_thread_task_t  *sendfile_task;
#endif
};


#define ngx_set_connection_log(c, l)                                         \
                                                                             \
    c->log->file = l->file;                                                  \
    c->log->next = l->next;                                                  \
    c->log->writer = l->writer;                                              \
    c->log->wdata = l->wdata;                                                \
    if (!(c->log->log_level & NGX_LOG_DEBUG_CONNECTION)) {                   \
        c->log->log_level = l->log_level;                                    \
    }


ngx_listening_t *ngx_create_listening(ngx_conf_t *cf, void *sockaddr, socklen_t socklen);
ngx_int_t ngx_clone_listening(ngx_conf_t *cf, ngx_listening_t *ls);
ngx_int_t ngx_set_inherited_sockets(ngx_cycle_t *cycle);
ngx_int_t ngx_open_listening_sockets(ngx_cycle_t *cycle);
void ngx_configure_listening_sockets(ngx_cycle_t *cycle);
void ngx_close_listening_sockets(ngx_cycle_t *cycle);
void ngx_close_connection(ngx_connection_t *c);
void ngx_close_idle_connections(ngx_cycle_t *cycle);
ngx_int_t ngx_connection_local_sockaddr(ngx_connection_t *c, ngx_str_t *s, ngx_uint_t port);
ngx_int_t ngx_connection_error(ngx_connection_t *c, ngx_err_t err, char *text);
ngx_connection_t *ngx_get_connection(ngx_socket_t s, ngx_log_t *log);
void ngx_free_connection(ngx_connection_t *c);

void ngx_reusable_connection(ngx_connection_t *c, ngx_uint_t reusable);

#endif /* _NGX_CONNECTION_H_INCLUDED_ */
