
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CONNECTION_H_INCLUDED_
#define _NGX_CONNECTION_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

/*
描述一个监听套接字
*/
typedef struct ngx_listening_s  ngx_listening_t;
struct ngx_listening_s 
{
    ngx_socket_t        fd;					/*文件描述符,即socket*/  
    struct sockaddr    *sockaddr;
    socklen_t           socklen;    		/* size of sockaddr */
    size_t              addr_text_max_len;
    ngx_str_t           addr_text;
    int                 type;				/* type of socket*/	
    int                 backlog;
    int                 rcvbuf;				/*接收缓冲区大小*/  
    int                 sndbuf;				/*发送缓冲区大小*/  
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
    int                 keepidle;
    int                 keepintvl;
    int                 keepcnt;
#endif

    /* handler of accepted connection */
    ngx_connection_handler_pt   handler;
	//HTTP -- ngx_http_port_t(指向监听这一端口的所有监听地址，而每个监听地址也包含了其所属server块的配置项)
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

    ngx_uint_t          worker;  		/*该监听套接字所属的worker进程的标号*/

    unsigned            open:1;
    unsigned            remain:1;
    unsigned            ignore:1;				/* 忽略该套接字 */

    unsigned            bound:1;       			/* already bound */
    unsigned            inherited:1;   			/* inherited from previous process */
    unsigned            nonblocking_accept:1;
    unsigned            listen:1;				/* listen状态*/
    unsigned            nonblocking:1;
    unsigned            shared:1;    			/* shared between threads or processes */
    unsigned            addr_ntop:1;

#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    unsigned            ipv6only:1;
#endif
#if (NGX_HAVE_REUSEPORT)
    unsigned            reuseport:1;			/* reuseport状态*/	
    unsigned            add_reuseport:1;       	/* 是否允许reuseport*/
#endif
    unsigned            keepalive:2;

#if (NGX_HAVE_DEFERRED_ACCEPT)
    unsigned            deferred_accept:1;   	/*设置TCP_DEFER_ACCEPT标志*/
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

//(被动连接)表示客户端主动发起的，Nginx服务器被动接受的TCP连接
struct ngx_connection_s 
{
	//连接未使用时，data成员用于充当连接池中空闲连接链表中的next指针。
	//当连接被使用时，data的意义由使用它的nginx模块而定，如在HTTP框架中，data指向ngx_http_request_t请求
    void               *data;
    ngx_event_t        *read;			//连接对应的读事件
    ngx_event_t        *write;			//连接对应的写事件
    ngx_socket_t        fd;    			//连接对应的套接字句柄
    ngx_recv_pt         recv;			//直接接受网络字符流的方法，根据系统环境的不同指向不同的函数
    ngx_send_pt         send;			//直接发送网络字符流的方法，根据系统环境的不同指向不同的函数
    ngx_recv_chain_pt   recv_chain;		//以ngx_chain_t链表为参数来接收网络字节流的方法
    ngx_send_chain_pt   send_chain;		//以ngx_chain_t链表为参数来发送网络字节流的方法

    ngx_listening_t    *listening;		//连接对应的ngx_listening_t监听对象，此连接由listening监听对口的事件建立

	//连接上已经发送出去的字节数
    off_t               sent;			

    ngx_log_t          *log;			//可以记录日志的ngx_log_t对象

    ngx_pool_t         *pool;

    struct sockaddr    *sockaddr;		//连接客户端的sockaddr结构体		
    socklen_t           socklen;		//sockaddr结构体的长度		
    ngx_str_t           addr_text;		//连接客户端字符串形式的ip地址		
    ngx_str_t           proxy_protocol_addr;	//

#if (NGX_SSL)
    ngx_ssl_connection_t  *ssl;
#endif

    struct sockaddr    *local_sockaddr;		//本机的监听端口对应的sockaddr结构体，也就是listening监听对象中的sockaddr成员
    socklen_t           local_socklen;
	//用于接收、缓存客户端发来的字节流，每个事件消费模块可自由决定从连接池中分配多大的空间给该字段。
	//例如，在HTTP模块中，它的大小决定于client_header_buffer_size配置项	
    ngx_buf_t          *buffer;		
	//用来将连接以双向链表元素的形式添加到ngx_cycle_t核心结构体的reuseable_connections_queue双向链表中，表示可以重用的连接
    ngx_queue_t         queue;		

    ngx_atomic_uint_t   number;		//连接使用次数。ngx_connection_t结构体每次建立一条来自客户端的连接，或者用于主动向后端服务器发起连接时(ngx_peer_connection_s也使用它)，number都会加 1
	//处理的请求次数
    ngx_uint_t          requests;	

    unsigned            buffered:8;

    unsigned            log_error:3;     		/* ngx_connection_log_error_e */
	//标志位，为 1时表示不期待字符流结束，目前无意义
    unsigned            unexpected_eof:1;	
	//标志位，为 1时表示连接已超时
    unsigned            timedout:1;	
	//标志位，为 1时表示连接处理过程中出现错误
    unsigned            error:1;	
	//标志位，为 1时表示连接已经销毁。这里的连接指的是TCP连接，而不是ngx_connection_t结构体。
	//当destroyed为 1时，结构体仍然存在，但其对应的套接字、内存池等已经不可用
    unsigned            destroyed:1;		
	//标志位，为 1时表示连接处于空闲状态，如keepalive请求中两次请求之间的状态
    unsigned            idle:1;				
    unsigned            reusable:1;			//标志位，为 1时表示连接可重用，它与上面的queue字段时对应使用的
    unsigned            close:1;			//标志位，为 1时表示连接关闭

    unsigned            sendfile:1;			//标志位，为 1时表示正将文件中的数据发往连接的另一端
    //标志位，为 1时表示只有在连接套接字对应的发送缓冲区必须满足最低设置的大小阈值时，事件驱动模块才会分发该事件。
    //与ngx_handle_write_event函数中的lowat参数是对应的
    unsigned            sndlowat:1;
	//标志位，表示如何使用TCP的nodelay特性。取值范围是枚举类型 ngx_connection_tcp_nodelay_e
    unsigned            tcp_nodelay:2;   
	//标志位，表示如何使用TCP的nodelay特性。取值范围是枚举类型 ngx_connection_tcp_nopush_e
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
