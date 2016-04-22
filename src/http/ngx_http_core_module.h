
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
	/*ip地址和端口的字符串表示 -- "192.18.1.2:80"*/
    u_char                     addr[NGX_SOCKADDR_STRLEN + 1];
} ngx_http_listen_opt_t;

//ngx_http_phases定义的11个阶段是有顺序的，必须按照其定义的顺序执行。
//NGX_HTTP_FIND_CONFIG_PHASE，NGX_HTTP_POST_REWRITE_PHASE，NGX_HTTP_POST_ACCESS_PHASE和NGX_HTTP_TRY_FILES_PHASE
//这4个阶段则不允许HTTP模块加入自己的ngx_http_handler_pt方法处理用户请求，它们由HTTP框架实现。
typedef enum 
{
//接收完请求头之后的第一个阶段，它位于uri重写之前，实际上很少有模块会注册在该阶段，默认的情况下，该阶段被跳过
//请求头读取完成之后的阶段
    NGX_HTTP_POST_READ_PHASE = 0,	
//server级别的uri重写阶段，也就是该阶段执行处于server块内，location块外的重写指令，(在读取请求头的过程中nginx会根据host及端口找到对应的虚拟主机配置)
//这个阶段主要是处理全局的(server block)的rewrite。
	NGX_HTTP_SERVER_REWRITE_PHASE,	
// 寻找location配置阶段，该阶段使用重写之后的uri来查找对应的location，值得注意的是该阶段可能会被执行多次，因为也可能有location级别的重写指令
    NGX_HTTP_FIND_CONFIG_PHASE,		//这个阶段主要是通过uri来查找对应的location。然后将uri和location的数据关联起来  
//location级别的uri重写阶段，该阶段执行location基本的重写指令，也可能会被执行多次；
    NGX_HTTP_REWRITE_PHASE,			
//location级别重写的后一阶段，用来检查上阶段是否有uri重写，并根据结果跳转到合适的阶段；
    NGX_HTTP_POST_REWRITE_PHASE,	//post rewrite，这个主要是进行一些校验以及收尾工作，以便于交给后面的模块。
// 访问权限控制的前一阶段，该阶段在权限控制阶段之前，一般也用于访问控制，比如限制访问频率，链接数等；
    NGX_HTTP_PREACCESS_PHASE,		//比如流控这种类型的access就放在这个phase，也就是说它主要是进行一些比较粗粒度的access。 
//访问权限控制阶段，比如基于ip黑白名单的权限控制，基于用户名密码的权限控制等；
    NGX_HTTP_ACCESS_PHASE,			//这个比如存取控制，权限验证就放在这个phase，一般来说处理动作是交给下面的模块做的.这个主要是做一些细粒度的access。  
//访问权限控制的后一阶段，该阶段根据权限控制阶段的执行结果进行相应处理；
	NGX_HTTP_POST_ACCESS_PHASE,		//一般来说当上面的access模块得到access_code之后就会由这个模块根据access_code来进行操作  
//try_files指令的处理阶段，如果没有配置try_files指令，则该阶段被跳过
    NGX_HTTP_TRY_FILES_PHASE,	
//内容生成阶段，该阶段产生响应，并发送到客户端；
	NGX_HTTP_CONTENT_PHASE,			//内容处理模块，我们一般的handle都是处于这个模块  
//日志记录阶段，该阶段记录访问日志；
    NGX_HTTP_LOG_PHASE				
} ngx_http_phases;

typedef struct ngx_http_phase_handler_s  ngx_http_phase_handler_t;


//一个HTTP处理阶段中的checker检查方法，仅可以由HTTP框架实现，以此控制HTTP请求的处理流程
//返回值 
//	NGX_OK -- 意味着把控制权交还给Nginx的事件模块，由它根据事件(网络事件、定时器事件、异步I/O事件等)再次调度请求
//	非NGX_OK -- 意味着向下执行phase_engine中的各处理方法  
typedef ngx_int_t (*ngx_http_phase_handler_pt)(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);

struct ngx_http_phase_handler_s 
{
	//在处理到某一个HTTP阶段时，HTTP框架将会在checker方法以实现的前提下首先调用checker方法来处理请求，
	//而不会直接调用任何阶段中的handler方法，只有在checker方法中才会去调用handler方法。因此，事实上所有的checker
	//方法都是由框架中的ngx_http_core_module模块实现的，且普通的HTTP模块无法重定义checker模块
    ngx_http_phase_handler_pt  checker;		//进入回调函数的条件判断函数，相同阶段的节点具有相同的check函数*/	
	/**/
    ngx_http_handler_pt        handler;		/*模块注册的回调函数*/
	//将要执行的下一个HTTP处理阶段的序号
    ngx_uint_t                 next;	
};


typedef struct 
{
	//由ngx_http_phase_handler_t构成的数组首地址，它表示一个请求可能经历的所有处理方法
    ngx_http_phase_handler_t  *handlers;	
	//表示NGX_HTTP_SERVER_REWRITE_PHASE阶段第一个ngx_http_phase_handler_t处理方法在handlers数组中的序号，
	//用于在执行HTTP请求的任何阶段中快速跳转到NGX_HTTP_SERVER_REWRITE_PHASE阶段处理请求
    ngx_uint_t                 server_rewrite_index;
	//表示NGX_HTTP_REWRITE_PHASE阶段第一个ngx_http_phase_handler_t处理方法在handlers数组中的序号，
	//用于在执行HTTP请求的任何阶段中快速跳转到NGX_HTTP_REWRITE_PHASE阶段处理请求
    ngx_uint_t                 location_rewrite_index;	
} ngx_http_phase_engine_t;


typedef struct 
{
	//ngx_http_handler_pt类型的动态数组，保存着每一个HTTP模块初始化添加到当前阶段的处理方法
    ngx_array_t                handlers;		
} ngx_http_phase_t;


typedef struct 
{
	//(ngx_http_core_srv_conf_t*)类型的动态数组，每一个元素指向表示server块的ngx_http_core_srv_conf_t结构体的地址，从而关联了所有server块 
    ngx_array_t                servers;
	//由下面各阶段处理方法构成的phases数组成员构建的阶段引擎才是流水式处理HTTP请求的实际数据结构
	//控制运行过程中以个HTTP请求所要经过的HTTP处理阶段，它将配合ngx_http_request_t结构体中的phase_handler使用
    ngx_http_phase_engine_t    phase_engine;

    ngx_hash_t                 headers_in_hash;

    ngx_hash_t                 variables_hash;

    ngx_array_t                variables;       /* array of ngx_http_variable_t */
    ngx_uint_t                 ncaptures;
	//表示存储所有server_name的散列表的散列桶的个数
    ngx_uint_t                 server_names_hash_max_size;
	//表示存储所有server_name的散列表的每个散列桶占用的内存大小
    ngx_uint_t                 server_names_hash_bucket_size;

    ngx_uint_t                 variables_hash_max_size;
    ngx_uint_t                 variables_hash_bucket_size;

    ngx_hash_keys_arrays_t    *variables_keys;  /* 以name为key，以ngx_http_variable_t为value的表*/

    ngx_array_t               *ports;			//存放着该http{}配置块下监听的所有ngx_http_conf_port_t端口

    ngx_uint_t                 try_files;       /* unsigned  try_files:1 */

	//用于在HTTP框架初始化时帮助各个HTTP模块在任意阶段中添加HTTP处理方法，
	//它是一个有11个成员的ngx_http_phase_t数组，其中每一个ngx_http_phase_t结构体对应
	//一个HTTP阶段。在HTTP框架初始化完毕后，运行过程中的phases数组是无用的
    ngx_http_phase_t           phases[NGX_HTTP_LOG_PHASE + 1];  	
} ngx_http_core_main_conf_t;


typedef struct 
{	
	//在开始处理一个HTTP请求时，Nginx会取出header中的Host，与每个server中的server_name进行匹配，
	//以此决定到底由哪一个server块来处理这个请求。
	//当一个Host与多个server块中的sever_name都匹配，将根据server_name与Host匹配优先级来选择实际处理的server块
	//1>选择字符串完全匹配的server_name，如www.testweb.com
	//2>选择通配符在前面的server_name，如*.testweb.com
	//3>选择通配符在后面的server_name，如www.testweb.*
	//4>选择使用正则表达式才匹配的server_name，如~^\.testweb\.com$
	//如果Host与所有的server_name都不匹配，将按一下顺序选择处理的server块
	//1>选择在listen配置项后加入[default | default_server]的server块
	//2>选择匹配listen端口的第一个server块
    ngx_array_t                 server_names;  		/* ngx_http_server_name_t, 保存于该server配置相关的所有sever name*/
	//指向当前server块所属的ngx_http_conf_ctx_t结构体
    ngx_http_conf_ctx_t        *ctx;	
	//当前server块的虚拟主机名，如果存在的话，则会与http请求中的Host头部做匹配，
	//匹配上后再由当前ngx_http_core_srv_conf_t处理请求
    ngx_str_t                   server_name;		
	//指定每个TCP连接上初始内存池大小
    size_t                      connection_pool_size;	
	//指定每个HTTP请求的初始内存池大小，用于减少内核对于小块内存的分配次数
	//TCP连接关闭时会销毁connection_pool_size指定的连接内存池，HTTP请求结束时会
	//销毁request_pool_size指定的HTTP请求内存池，但它们的创建、销毁时间并不一致，
	//因为一个TCP连接可能被复用与多个HTTP请求
    size_t                      request_pool_size;
	//存储HTTP头部(请求行和请求头)时分配的内存buffer大小
    size_t                      client_header_buffer_size;

    ngx_bufs_t                  large_client_header_buffers;
	//读取HTTP头部(请求行和请求头)的超时时间
    ngx_msec_t                  client_header_timeout;
	//如果将其设置为off，那么当出现不和法的HTTP头部时，
	//Nginx会拒绝服务，并直接向用户发送400(Bad Request)错误
	//如果将其设置为on，则会忽略此HTTP头部
    ngx_flag_t                  ignore_invalid_headers;
	//是否合并相邻的'/'
	//例如: //test///a.txt，在配置为on时，会将其匹配为 location /test/a.txt;
	//	如果配置为off，则不会匹配，URI仍将是//test///a.txt
    ngx_flag_t                  merge_slashes;
	
    ngx_flag_t                  underscores_in_headers; //是否允许HTTP头部的名称中带'_'(下划线)

    unsigned                    listen:1;			//表示是否配置listen 指令
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
	//这个监听地址对应的server块配置信息
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
	/*IP地址，网络字节序*/
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
	//addrs指向的对象的元素的个数
    ngx_uint_t                 naddrs;		
} ngx_http_port_t;


typedef struct
{
    ngx_int_t                  family;		//socket地址家族
    in_port_t                  port;		//监听端口(网络字节序)
    ngx_array_t                addrs;     	//监听端口下对应着的所有ngx_http_conf_addr_t地址
} ngx_http_conf_port_t;



//每一个ngx_http_conf_addr_t都会有一个ngx_listening_t与其相对应
typedef struct 
{
    ngx_http_listen_opt_t      opt;			//监听套接字的各种属性

	//以下3个散列表用于加速寻找到对应监听端口上的新连接，确定到底使用哪个server{}虚拟主机下的配置来处理它。
	//所以散列表的值就是ngx_http_core_srv_conf_t结构体的地址
    ngx_hash_t                 hash;		//完全匹配server name的散列表
    ngx_hash_wildcard_t       *wc_head;		//通配符前置的散列表
    ngx_hash_wildcard_t       *wc_tail;		//通配符后置的散列表

#if (NGX_PCRE)
    ngx_uint_t                 nregex;		//成员regex数组中的元素个数
    ngx_http_server_name_t    *regex;		//指向静态数组，其数组成员就是ngx_http_server_name结构体，表示正则表达式及其匹配的server{}虚拟机
#endif

    /* the default server configuration for this address:port */
    ngx_http_core_srv_conf_t  *default_server;	//该监听端口下对应的默认server{}虚拟主机
    ngx_array_t                servers;  	/* array of (ngx_http_core_srv_conf_t*), 记录相同IP下的不同server{}配置*/
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

//完整的描叙了一个location块的信息，
//以及匹配这个location的请求的http包的一些资源信息
struct ngx_http_core_loc_conf_s 
{
	//location名称，即location指令后的表达式
    ngx_str_t     name;        
#if (NGX_PCRE)
    ngx_http_regex_t  *regex;
#endif
    unsigned      noname:1;   			/* "if () {}" block or limit_except */
    unsigned      lmt_excpt:1;
	//命名location，仅用于server内部跳转
    unsigned      named:1;				
	//绝对匹配
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

    //指向所属location块内ngx_http_conf_ctx_t结构体中的loc_conf指针数组，它保存着当前location块内所有HTTP模块create_loc_conf方法产生的结构体指针
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
	//当找不到相应的MIME type与文件扩展名之间的映射时，
	//使用默认的MIME type作为HTTP header中的Content-type
    ngx_str_t     default_type;   /*[config]defatult_type  application/octet-stream*/
	//HTTP请求包体的最大值
    off_t         client_max_body_size;   
	//指定在FreeBSD和Linux系统上使用O_DIRECT选项去读取文件时，
	//缓冲区的大小，通常对大文件的读取速度有优化作用。
	//注意，它与sendfile功能是互斥的
    off_t         directio;                
	//与directio配合使用，指定以directio方式读取文件时的对齐方式。
	//一般情况下512B已经足够了，但针对一些高性能文件系统，如Linux下的XFS文件系统，
	//可能需要设置到4KB作为对齐方式
    off_t         directio_alignment;      /* directio_alignment */
	//存储HTTP包体的内存缓冲区大小
	//HTTP包体会先接收到指定的这块缓存中，之后才决定是否写入磁盘
	//如果用户请求中包含有HTTP头部Content-Length，并且其标识的长度小于定义的缓冲区，
	//那么Nginx会自动降低本次请求所使用的内存buffer，以降低内存消耗
    size_t        client_body_buffer_size; /* client_body_buffer_size */
    size_t        send_lowat;              /* send_lowat */
    size_t        postpone_output;         /* postpone_output */
	//对客户端请求限制每秒传输的字节数，0表示不限速
    size_t        limit_rate;   
	//向客户端发送的响应长度超过limit_rate_after后才开始限速
    size_t        limit_rate_after;        
    size_t        sendfile_max_chunk;      /* sendfile_max_chunk */
    size_t        read_ahead;              /* read_ahead */
	//读取HTTP包体的超时时间
    ngx_msec_t    client_body_timeout;     /* client_body_timeout */
	//发送响应的超时时间
    ngx_msec_t    send_timeout;            /* send_timeout */
	//keepalive超时时间
    ngx_msec_t    keepalive_timeout;       
	//保持延迟关闭状态的最长持续时间
    ngx_msec_t    lingering_time;          
	//延迟关闭的超时时间
	//延迟关闭状态中，若超过lingering_timeout时间后还没有数据可读，就直接关闭连接
    ngx_msec_t    lingering_timeout;       /* lingering_timeout */
	//DNS解析的超时时间
    ngx_msec_t    resolver_timeout;        

    ngx_resolver_t  *resolver;             /* resolver */
	//Nginx发送给客户端响应头中的Keep-Alive域的时间
    time_t        keepalive_header;        
	//一个keepalive长连接上允许承载的请求最大数目
    ngx_uint_t    keepalive_requests;      /* keepalive_requests */
	//对某些浏览器禁用keepalive功能
	//HTTP请求中的keepalive功能是为了让多个请求复用一个HTTP长连接
	//但是有些浏览器对长连接处理有问题
    ngx_uint_t    keepalive_disable;       
	//仅可取值为NGX_HTTP_SATISFY_ALL或者NGX_HTTP_SATISFY_ANY
    ngx_uint_t    satisfy;                 /* satisfy */
	//控制Nginx关闭用户连接的方式
	//always表示关闭用户连接前必须无条件地处理连接上所有用户发送的数据
	//off表示关闭连接时完全不管连接上是否已经有准备就绪的来自用户的数据
	//on是中间值，一般情况下在关闭连接前都会处理连接上的用户发送的数据，
	//除了有些情况下在业务上认定这之后的数据是不必要的
    ngx_uint_t    lingering_close;         /* lingering_close */
	//对If_Modified_Since头部的处理策略
	//处于性能的考虑，Web浏览器一般会在客户端本地缓存一些文件，并存储当时获取的时间。
	//这样，下次向Web服务器获取缓存过的资源时，就可以用If-Modified-Since头部把上次获取
	//的时间捎带上，而Nginx将根据if_modified_since的值来决定如何处理If-Modified-Since头部
	//off--表示忽略用户请求中的If_Modified_Since头部。这时，如果获取一个文件，
	//	那么会正常地返回文件内容。HTTP响应码通常是200
	//exact--将If_Modified_Since头部包含的时间与将要返回的文件上次修改的时间做精确比较，
	//	如果没有匹配上，则返回200和文件的实际内容，如果匹配上，则表示浏览器缓存的文件内容已经是最新的了，
	//	没有必要再返回文件从而浪费时间与带宽了，这时会返回304 Not Modified，浏览器收到后会直接读取自己的本地缓存
	//before--是比exact更宽松的比较。只要文件的上次修改时间等于或早于用户请求中的If_Modified_Since头部时间，
	//	就会向客户端返回304 Not Modified。
    ngx_uint_t    if_modified_since;       
    ngx_uint_t    max_ranges;              /* max_ranges */
    ngx_uint_t    client_body_in_file_only; /* client_body_in_file_only */

	//
    ngx_flag_t    client_body_in_single_buffer;
                                           /* client_body_in_singe_buffer */
	//内部location，这种location只能由子请求或者内部跳转访问
    ngx_flag_t    internal;                /* internal */
	//确定是否启用sendfile系统调用来发送文件
	//sendfile系统调用减少了内核态与用户态之间的两次内存复制，这样就会从磁盘中读取文件后直接
	//在内核态发送到网卡设备，提高了发送文件的效率
    ngx_flag_t    sendfile;        

	//是否在FreeBSD或Linux系统上启用内核级别的异步文件I/O功能。
	//注意: 它与sendfile功能是互斥的
    ngx_flag_t    aio;                   
	
	//确定是否开启FreeBSD系统上的TCP_NOPUSH或Linux系统上的TCP_CORK功能。
	//仅在使用sendfile的时候才有效
	//打开tcp_nopush后，将会在发送响应时把整个响应包头放到一个TCP包中发送
    ngx_flag_t    tcp_nopush;     
	
	//开启或关闭Nginx使用TCP_NODELAY选项的功能 
	//这个选项在将连接转变为长连接的时候才被启用，在upstream发送响应到客户端时也会启用
    ngx_flag_t    tcp_nodelay;      
	
	//连接超时后将通过向客户端发送RST包来直接重置连接
	//相比正常的关闭方式，它使得服务器避免产生许多处于FIN_WAIT_1,FIN_WAIT_2/TIME_WAIT状态的TCP连接
	//注意，使用RST重置包关闭连接会带来一些问题，默认情况下不会开启
    ngx_flag_t    reset_timedout_connection; 
	//重定向主机名称的处理，需要配合server_name配置使用
	//on--表示在重定向请求时会使用server_name里配置的第一个主机名代替原先请求的Host头部，
	//off--表示在重定向请求时使用请求本身的Host头部
    ngx_flag_t    server_name_in_redirect; 
    ngx_flag_t    port_in_redirect;        /* port_in_redirect */
    ngx_flag_t    msie_padding;            /* msie_padding */
    ngx_flag_t    msie_refresh;            /* msie_refresh */
	//当处理用户请求且需要访问文件时，如果没有找到文件，是否将错误日志
	//记录到erro.log文件中。这仅用于定位问题
	ngx_flag_t    log_not_found;           
    ngx_flag_t    log_subrequest;          /* log_subrequest */
    ngx_flag_t    recursive_error_pages;   /* recursive_error_pages */
	//处理请求出错时是否在响应的Server头部中表明Nginx版本，这是为了方便问题
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

	//HTTP包体的临时存放目录
    ngx_path_t   *client_body_temp_path;   /* client_body_temp_path */

	//文件缓存，通过读取缓存就减少了对磁盘的操作
    ngx_open_file_cache_t  *open_file_cache;
	//检验文件缓存中元素有效性的频率
    time_t        open_file_cache_valid;
	//指定文件缓存中，在inactive指定的时间段内，访问次数超过了
	//open_file_cache_min_uses指定的最小次数，那么将不会被淘汰出缓存
    ngx_uint_t    open_file_cache_min_uses;
	//是否在文件缓存中缓存打开文件时出现的找不到路径、没有权限等错误信息
    ngx_flag_t    open_file_cache_errors;
    ngx_flag_t    open_file_cache_events;

    ngx_log_t    *error_log;
	//为了快速寻找到相应MIME type，Nginx使用散列表来存储文件扩展名到MIME type的映射
	//types_hash_max_size散列表中的冲突桶的个数
    ngx_uint_t    types_hash_max_size;
	//为了快速寻找到相应MIME type，Nginx使用散列表来存储文件扩展名到MIME type的映射
	//types_hash_bucket_size设置了散列表中每个冲突桶桶占用的内存大小
    ngx_uint_t    types_hash_bucket_size;

	//属于当前块的所有location块通过ngx_http_location_queue_t结构体构成的双向链表
    ngx_queue_t  *locations;   		//ngx_http_location_queue_t类型的双链表，将同一个server块内多个表达location块的ngx_http_core_loc_conf_t结构体以双向链表方式组织起来

#if 0
    ngx_http_core_loc_conf_t  *prev_location;
#endif
};


typedef struct
{
    ngx_queue_t                      queue;  	//queue将作为ngx_queue_t双向链表容器，从而将ngx_http_location_queue_t结构体连接起来
    ngx_http_core_loc_conf_t        *exact;		//如果location中的字符串可以精确匹配(包括正则表达式)，exact将指向对应的ngx_http_core_loc_conf_t结构体，否则值为NULL
    ngx_http_core_loc_conf_t        *inclusive;	//如果location中的字符串无法精确匹配(包括了自定义的通配符),inclusizve将指向对应的ngx_http_core_loc_conf_t结构体，否则值为NULL
    ngx_str_t                       *name;		//指向location的名称
    u_char                          *file_name;
    ngx_uint_t                       line;
    ngx_queue_t                      list;
} ngx_http_location_queue_t;


struct ngx_http_location_tree_node_s 
{
    ngx_http_location_tree_node_t   *left;		//左子树
    ngx_http_location_tree_node_t   *right;		//右子树
    ngx_http_location_tree_node_t   *tree;		//无法完全匹配的location组成的树

    ngx_http_core_loc_conf_t        *exact;		//如果location对应的URI匹配字符串属于能够完全匹配的类型，则exact指向其对应的ngx_http_core_loc_conf_t结构体，否则为NULL
    //如果location对应的URI匹配字符串属于无法完全匹配的类型，则inclusive指向其对应的ngx_http_core_loc_conf_t结构体，否则为NULL
    ngx_http_core_loc_conf_t        *inclusive;	
	//自动重定向标志
    u_char                           auto_redirect;	
	//name字符串实际长度
    u_char                           len;		
	//name指向location对应的URI匹配表达式
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
