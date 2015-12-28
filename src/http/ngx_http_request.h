
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_REQUEST_H_INCLUDED_
#define _NGX_HTTP_REQUEST_H_INCLUDED_


#define NGX_HTTP_MAX_URI_CHANGES           10
#define NGX_HTTP_MAX_SUBREQUESTS           50

/* must be 2^n */
#define NGX_HTTP_LC_HEADER_LEN             32


#define NGX_HTTP_DISCARD_BUFFER_SIZE       4096
#define NGX_HTTP_LINGERING_BUFFER_SIZE     4096


#define NGX_HTTP_VERSION_9                 9
#define NGX_HTTP_VERSION_10                1000
#define NGX_HTTP_VERSION_11                1001
#define NGX_HTTP_VERSION_20                2000

#define NGX_HTTP_UNKNOWN                   0x0001
#define NGX_HTTP_GET                       0x0002
#define NGX_HTTP_HEAD                      0x0004
#define NGX_HTTP_POST                      0x0008
#define NGX_HTTP_PUT                       0x0010
#define NGX_HTTP_DELETE                    0x0020
#define NGX_HTTP_MKCOL                     0x0040
#define NGX_HTTP_COPY                      0x0080
#define NGX_HTTP_MOVE                      0x0100
#define NGX_HTTP_OPTIONS                   0x0200
#define NGX_HTTP_PROPFIND                  0x0400
#define NGX_HTTP_PROPPATCH                 0x0800
#define NGX_HTTP_LOCK                      0x1000
#define NGX_HTTP_UNLOCK                    0x2000
#define NGX_HTTP_PATCH                     0x4000
#define NGX_HTTP_TRACE                     0x8000

#define NGX_HTTP_CONNECTION_CLOSE          1
#define NGX_HTTP_CONNECTION_KEEP_ALIVE     2


#define NGX_NONE                           1


#define NGX_HTTP_PARSE_HEADER_DONE         1

#define NGX_HTTP_CLIENT_ERROR              10
#define NGX_HTTP_PARSE_INVALID_METHOD      10
#define NGX_HTTP_PARSE_INVALID_REQUEST     11
#define NGX_HTTP_PARSE_INVALID_09_METHOD   12

#define NGX_HTTP_PARSE_INVALID_HEADER      13


/* unused                                  1 */
#define NGX_HTTP_SUBREQUEST_IN_MEMORY      2      	//结果数据是否可以可存放于内存中
#define NGX_HTTP_SUBREQUEST_WAITED         4		//先结束的子请求是否需要等待前面子请求的结束才设置done标记
#define NGX_HTTP_LOG_UNSAFE                8


#define NGX_HTTP_CONTINUE                  100
#define NGX_HTTP_SWITCHING_PROTOCOLS       101
#define NGX_HTTP_PROCESSING                102

#define NGX_HTTP_OK                        200
#define NGX_HTTP_CREATED                   201
#define NGX_HTTP_ACCEPTED                  202
#define NGX_HTTP_NO_CONTENT                204
#define NGX_HTTP_PARTIAL_CONTENT           206

#define NGX_HTTP_SPECIAL_RESPONSE          300
#define NGX_HTTP_MOVED_PERMANENTLY         301
#define NGX_HTTP_MOVED_TEMPORARILY         302
#define NGX_HTTP_SEE_OTHER                 303
#define NGX_HTTP_NOT_MODIFIED              304
#define NGX_HTTP_TEMPORARY_REDIRECT        307

#define NGX_HTTP_BAD_REQUEST               400		//(错误请求) 服务器不理解请求的语法
#define NGX_HTTP_UNAUTHORIZED              401
#define NGX_HTTP_FORBIDDEN                 403
#define NGX_HTTP_NOT_FOUND                 404
#define NGX_HTTP_NOT_ALLOWED               405
#define NGX_HTTP_REQUEST_TIME_OUT          408
#define NGX_HTTP_CONFLICT                  409
#define NGX_HTTP_LENGTH_REQUIRED           411		//(需要有效长度) 服务器不接受不含有效内容长度标头字段的请求
#define NGX_HTTP_PRECONDITION_FAILED       412
#define NGX_HTTP_REQUEST_ENTITY_TOO_LARGE  413
#define NGX_HTTP_REQUEST_URI_TOO_LARGE     414
#define NGX_HTTP_UNSUPPORTED_MEDIA_TYPE    415
#define NGX_HTTP_RANGE_NOT_SATISFIABLE     416


/* Our own HTTP codes */

/* The special code to close connection without any response */
#define NGX_HTTP_CLOSE                     444

#define NGX_HTTP_NGINX_CODES               494

#define NGX_HTTP_REQUEST_HEADER_TOO_LARGE  494

#define NGX_HTTPS_CERT_ERROR               495
#define NGX_HTTPS_NO_CERT                  496

/*
 * We use the special code for the plain HTTP requests that are sent to
 * HTTPS port to distinguish it from 4XX in an error page redirection
 */
#define NGX_HTTP_TO_HTTPS                  497

/* 498 is the canceled code for the requests with invalid host name */

/*
 * HTTP does not define the code for the case when a client closed
 * the connection while we are processing its request so we introduce
 * own code to log such situation when a client has closed the connection
 * before we even try to send the HTTP header to it
 */
#define NGX_HTTP_CLIENT_CLOSED_REQUEST     499


#define NGX_HTTP_INTERNAL_SERVER_ERROR     500
#define NGX_HTTP_NOT_IMPLEMENTED           501
#define NGX_HTTP_BAD_GATEWAY               502
#define NGX_HTTP_SERVICE_UNAVAILABLE       503
#define NGX_HTTP_GATEWAY_TIME_OUT          504
#define NGX_HTTP_INSUFFICIENT_STORAGE      507


#define NGX_HTTP_LOWLEVEL_BUFFERED         0xf0
#define NGX_HTTP_WRITE_BUFFERED            0x10
#define NGX_HTTP_GZIP_BUFFERED             0x20
#define NGX_HTTP_SSI_BUFFERED              0x01
#define NGX_HTTP_SUB_BUFFERED              0x02
#define NGX_HTTP_COPY_BUFFERED             0x04


typedef enum {
    NGX_HTTP_INITING_REQUEST_STATE = 0,
    NGX_HTTP_READING_REQUEST_STATE,
    NGX_HTTP_PROCESS_REQUEST_STATE,

    NGX_HTTP_CONNECT_UPSTREAM_STATE,
    NGX_HTTP_WRITING_UPSTREAM_STATE,
    NGX_HTTP_READING_UPSTREAM_STATE,

    NGX_HTTP_WRITING_REQUEST_STATE,
    NGX_HTTP_LINGERING_CLOSE_STATE,
    NGX_HTTP_KEEPALIVE_STATE
} ngx_http_state_e;


typedef struct 
{
    ngx_str_t                         name;
    ngx_uint_t                        offset;
    ngx_http_header_handler_pt        handler;
} ngx_http_header_t;


typedef struct {
    ngx_str_t                         name;
    ngx_uint_t                        offset;
} ngx_http_header_out_t;


typedef struct 
{
	//所有解析过的HTTP头部都在headers链表中
    ngx_list_t                        headers;   /*list of ngx_table_elt_t; 存储请求头，例如Host: 127.0.0.1*/

	//以下每个ngx_table_elt_t成员都是RFC1616规范中定义的HTTP头部，它们实际都指向headers链表中的响应项。
	//注意，当它们为NULL空指针时，表示没有解析到相应的HTTP头部
    ngx_table_elt_t                  *host;
    ngx_table_elt_t                  *connection;
    ngx_table_elt_t                  *if_modified_since;
    ngx_table_elt_t                  *if_unmodified_since;
    ngx_table_elt_t                  *if_match;
    ngx_table_elt_t                  *if_none_match;
    ngx_table_elt_t                  *user_agent;
    ngx_table_elt_t                  *referer;
    ngx_table_elt_t                  *content_length;
    ngx_table_elt_t                  *content_type;

    ngx_table_elt_t                  *range;
    ngx_table_elt_t                  *if_range;

    ngx_table_elt_t                  *transfer_encoding;
    ngx_table_elt_t                  *expect;
    ngx_table_elt_t                  *upgrade;

#if (NGX_HTTP_GZIP)
    ngx_table_elt_t                  *accept_encoding;
    ngx_table_elt_t                  *via;
#endif

    ngx_table_elt_t                  *authorization;

    ngx_table_elt_t                  *keep_alive;

#if (NGX_HTTP_X_FORWARDED_FOR)
    ngx_array_t                       x_forwarded_for;
#endif

#if (NGX_HTTP_REALIP)
    ngx_table_elt_t                  *x_real_ip;
#endif

#if (NGX_HTTP_HEADERS)
    ngx_table_elt_t                  *accept;
    ngx_table_elt_t                  *accept_language;
#endif

#if (NGX_HTTP_DAV)
    ngx_table_elt_t                  *depth;
    ngx_table_elt_t                  *destination;
    ngx_table_elt_t                  *overwrite;
    ngx_table_elt_t                  *date;
#endif

	//user和passwd是只有ngx_http_auth_basic_module才会用到的成员
    ngx_str_t                         user;
    ngx_str_t                         passwd;

    ngx_array_t                       cookies;

	//server名称
    ngx_str_t                         server;
	//根据ngx_table_elt_t * content_length计算出HTTP包体大小
    off_t                             content_length_n;
    time_t                            keep_alive_n;

	//HTTP连接类型，它的取值范围是0， NGX_HTTP_CONNECTION_CLOSE 或者 NGX_HTTP_CONNECTION_KEEP_ALIVE
    unsigned                          connection_type:2;
    unsigned                          chunked:1;

	//以下7个标志位是HTTP框架根据浏览器传来的"useragent"头部，它们可用来判断浏览器的类型，
	//值为1时表示是相应的浏览器发来的请求，值为0时则相反
    unsigned                          msie:1;
    unsigned                          msie6:1;
    unsigned                          opera:1;
    unsigned                          gecko:1;
    unsigned                          chrome:1;
    unsigned                          safari:1;
    unsigned                          konqueror:1;
} ngx_http_headers_in_t;


typedef struct
{
    ngx_list_t                        headers;				// list of ngx_table_elt_t

    ngx_uint_t                        status;				//http相应的状态码 
    ngx_str_t                         status_line;

    ngx_table_elt_t                  *server;
    ngx_table_elt_t                  *date;
    ngx_table_elt_t                  *content_length;
    ngx_table_elt_t                  *content_encoding;
    ngx_table_elt_t                  *location;
    ngx_table_elt_t                  *refresh;
    ngx_table_elt_t                  *last_modified;
    ngx_table_elt_t                  *content_range;
    ngx_table_elt_t                  *accept_ranges;
    ngx_table_elt_t                  *www_authenticate;
    ngx_table_elt_t                  *expires;
    ngx_table_elt_t                  *etag;

    ngx_str_t                        *override_charset;

    size_t                            content_type_len;
    ngx_str_t                         content_type;
    ngx_str_t                         charset;
    u_char                           *content_type_lowcase;
    ngx_uint_t                        content_type_hash;

    ngx_array_t                       cache_control;

    off_t                             content_length_n;			//头部content_lenght  
    time_t                            date_time;
    time_t                            last_modified_time;		//资源的最后修改时间 
} ngx_http_headers_out_t;


typedef void (*ngx_http_client_body_handler_pt)(ngx_http_request_t *r);

typedef struct 
{	
	//存放HTTP包体的临时文件
    ngx_temp_file_t                  *temp_file;
	//接收HTTP包体的缓冲区链表。
	//当包体需要全部存放在内存中时，如果一块ngx_buf_t缓冲区无法存放完时，这时就需要使用ngx_chain_t链表来存放。
	/*指向保存请求体的链表头*/
	ngx_chain_t                      *bufs;
	//直接接收HTTP包体的缓存
	/*指向当前用于保存请求体的内存缓存*/
    ngx_buf_t                        *buf;
	//根据content-length头部和接收到的包体长度，计算出的还需要接收的包体长度
    off_t                             rest;
    ngx_chain_t                      *free;
    ngx_chain_t                      *busy;
    ngx_http_chunked_t               *chunked;
	//HTTP接收完毕后执行的回调方法，也就是ngx_http_read_client_request_body方法传递的第2个参数
    ngx_http_client_body_handler_pt   post_handler;
} ngx_http_request_body_t;


typedef struct ngx_http_addr_conf_s  ngx_http_addr_conf_t;

typedef struct 
{
    ngx_http_addr_conf_t             *addr_conf;
    ngx_http_conf_ctx_t              *conf_ctx;

#if (NGX_HTTP_SSL && defined SSL_CTRL_SET_TLSEXT_HOSTNAME)
    ngx_str_t                        *ssl_servername;
#if (NGX_PCRE)
    ngx_http_regex_t                 *ssl_servername_regex;
#endif
#endif

    ngx_buf_t                       **busy;
    ngx_int_t                         nbusy;

    ngx_buf_t                       **free;
    ngx_int_t                         nfree;

#if (NGX_HTTP_SSL)
    unsigned                          ssl:1;
#endif
    unsigned                          proxy_protocol:1;
} ngx_http_connection_t;


typedef void (*ngx_http_cleanup_pt)(void *data);

typedef struct ngx_http_cleanup_s  ngx_http_cleanup_t;

struct ngx_http_cleanup_s 
{
	//由HTTP模块提供的清理资源的回调方法
    ngx_http_cleanup_pt               handler;
	//希望给上面handler方法传递的参数
    void                             *data;
	//一个请求可能有多个ngx_http_cleanup_t清理方法，这些清理方法之间是通过next指针连接成单链表
    ngx_http_cleanup_t               *next;
};


//返回值
//rc -- 子请求在结束时的状态，它到的取值则是执行ngx_http_finalize_request销毁请求时传递的rc参数
typedef ngx_int_t (*ngx_http_post_subrequest_pt)(ngx_http_request_t *r, void *data, ngx_int_t rc);

typedef struct 
{
	//Nginx在子请求正常或者异常结束时，都会调用ngx_http_post_subrequest_pt回调方法
	//在回调方法内必须设置父请求激活后的处理方法，设置方法很简单，首先找出父请求，例如ngx_http_request_t *pr = r->parent;
	//然后将实现好的ngx_http_event_handler_pt回调方法赋给父请求的write_event_handler指针
    ngx_http_post_subrequest_pt       handler;
	//handler指向的ngx_http_post_subrequest_pt回调方法执行执行时的data参数
    void                             *data;
} ngx_http_post_subrequest_t;


typedef struct ngx_http_postponed_request_s  ngx_http_postponed_request_t;

struct ngx_http_postponed_request_s 
{
	//当前请求的子请求
    ngx_http_request_t               *request;
	//临时保存当前请求处理后的结果数据(该请求自己产生的数据)，也就是为了组织最终的响应数据而设计
    ngx_chain_t                      *out;		
    ngx_http_postponed_request_t     *next;
};


typedef struct ngx_http_posted_request_s  ngx_http_posted_request_t;

struct ngx_http_posted_request_s 
{
	//指向当前待处理的子请求的ngx_http_request_t结构体
    ngx_http_request_t               *request;
	//指向下一个子请求，如果没有，则为NULL空指针
    ngx_http_posted_request_t        *next;
};

//由HTTP模块实现的handler处理方法
typedef ngx_int_t (*ngx_http_handler_pt)(ngx_http_request_t *r);
typedef void (*ngx_http_event_handler_pt)(ngx_http_request_t *r);


struct ngx_http_request_s 
{
    uint32_t                          signature;         /* "HTTP" */
	//这个请求对应的客户端连接
    ngx_connection_t                 *connection;
	//指向存放所有HTTP模块的上下文结构体的指针数组
    void                            **ctx;
	//指向请求对应的存放main级别配置结构体的指针数组
    void                            **main_conf;
	//指向请求对应的存放srv级别配置结构体的指针数组
    void                            **srv_conf;
	//指向请求对应的存放loc级别配置结构体的指针数组
    void                            **loc_conf;
	//在接收完HTTP头部，第一次在业务上处理HTTP请求时，HTTP框架提供的处理方法是ngx_http_process_request。
	//但如果该方法无法一次处理完该请求的全部业务，在归还控制权到epoll事件模块后，该请求再次被回调时，
	//将通过ngx_http_request_handler方法来处理，而这个方法中对于可读事件的处理就是调用read_event_handler
	//处理请求。也就是说，HTTP模块希望在底层处理请求的读事件时，重新实现read_event_handler方法
    ngx_http_event_handler_pt         read_event_handler;
	//与read_event_handler回调方法类似，如果ngx_http_request_handler方法判断当前事件是可写事件，
	//则调用write_event_handler处理请求
    ngx_http_event_handler_pt         write_event_handler;

#if (NGX_HTTP_CACHE)
    ngx_http_cache_t                 *cache;
#endif

	//upstream机制用到的结构体
    ngx_http_upstream_t              *upstream;
    ngx_array_t                      *upstream_states;
                                         /* of ngx_http_upstream_state_t */

	//表示这个请求的内存池，在ngx_http_free_request方法中销毁。它与ngx_connection_t中的内存池意义不同，
	//当请求释放时，TCP连接可能并没有关闭，这时请求的内存池会销毁，但ngx_connection_t的内存池并不会销毁
    ngx_pool_t                       *pool;
	//用于接收HTTP请求内容的缓冲区，主要用于接收HTTP头部
    ngx_buf_t                        *header_in;
	//ngx_http_process_request_headers方法在接收、解析完HTTP请求的头部后，会把解析完的每一个HTTP头部
	//加入到headers_in的headers链表中，同时会构造headers_in中的其他成员
    ngx_http_headers_in_t             headers_in;
	//HTTP模块会把想要发送的HTTP响应信息放到headers_out中，期望HTTP框架将headers_out中的成员序列化为HTTP响应包发送给用户
    ngx_http_headers_out_t            headers_out;
	//接收HTTP请求中包体的数据结构
    ngx_http_request_body_t          *request_body;
	//延迟关闭连接的时间
    time_t                            lingering_time;
	
	//请求开始时间,以收到数据为准(为限速功能服务)
	//当前请求初始化时的时间。start_sec是格林威治时间1970年1月1日凌晨0点0分0秒到当前时间的秒数。
	//如果这个请求是子请求，则该时间是子请求的生成时间; 如果这个请求是用户发来的请求，
	//则是在建立起TCP连接后，第一次接收到可读事件时的时间
    time_t                            start_sec;
	//与start_sec配合使用，表示相对于start_sec秒的毫秒偏移量
    ngx_msec_t                        start_msec;

	//以下9个成员都是ngx_http_processs_request_line方法在接收、解析HTTP请求行时解析出的信息
    ngx_uint_t                        method;			/*NGX_HTTP_GET | NGX_HTTP_POST| ...*/
    ngx_uint_t                        http_version;
    ngx_str_t                         request_line;		// "GET / HTTP/1.1"
    ngx_str_t                         uri;
    ngx_str_t                         args;
    ngx_str_t                         exten;
    ngx_str_t                         unparsed_uri;

    ngx_str_t                         method_name;		// "GET"
    ngx_str_t                         http_protocol;	// "HTTP/1.1"

	//表示需要发送给客户端的HTTP响应。 out中保存着由headers_out中序列化后的表示HTTP头部的TCP流。
	//在调用ngx_http_output_filter方法后，out中还会保存待发送的HTTP包体，它是实现异步发送HTTP响应的关键
    ngx_chain_t                      *out;

	
	//主请求，通过这个域我们能够判断当前的请求是否是子请求
	//当前请求既可能是用户发来的请求，也可能是派生出的子请求，而main则标识一系列相关的派生子请求的原始请求，
	//我们一般可通过main和当前请求的地址是否相等来判断当前请求是否为用户发来的原始请求
    ngx_http_request_t               *main;		
	//当前请求的父请求。注意，父请求未必是原始请求
    ngx_http_request_t               *parent;
	//当前请求的子请求链表	
    ngx_http_postponed_request_t     *postponed;
	//回调函数以及可传递给该回调函数的数据，回调函数在当前子请求结束时被调用，因此可以做进一步自定义处理
    ngx_http_post_subrequest_t       *post_subrequest;	
    //记录当前所有需要被执行的子请求的链表，该字段仅主请求有效
    ngx_http_posted_request_t        *posted_requests;	

	//全局的ngx_http_phase_engine_t结构体中定义了一个ngx_http_phase_handler_t回调方法组成的数组，
	//而phase_handler成员则与该数组配合使用，表示请求下次应当执行以phase_handler作为序号指定的
	//数组中的回调方法。HTTP框架正是以这种方式把HTTP模块集成起来处理请求的
    ngx_int_t                         phase_handler;
	//表示NGX_HTTP_CONTENT_PHASE阶段提供给HTTP模块处理请求的一种方式，content_handler指向HTTTP模块实现的请求处理方法
	//在NGX_HTTP_FIND_CONFIG_PHASE阶段就会把它设为匹配了URI的location块中对应的ngx_http_core_loc_conf_t结构体的handler成员
	ngx_http_handler_pt               content_handler;
	//在NGX_HTTP_ACCESS_PHASE阶段需要判断请求是否具有访问权限时，通过access_code来传递HTTP模块的handler回调方法的返回值，
	//如果access_code为0，则表示请求具备访问权限，反之则说明请求不具备访问权限
    ngx_uint_t                        access_code;

    ngx_http_variable_value_t        *variables;		/*array of ngx_http_variable_value_t*/

#if (NGX_PCRE)
    ngx_uint_t                        ncaptures;
    int                              *captures;
    u_char                           *captures_data;
#endif
	//发送响应的最大限速速率(byte/sec)， 0表示不限制
    size_t                            limit_rate;
	//限速这个动作是在发送了limit_rate_after字节的响应后才能生效(对于小响应包的优化)
    size_t                            limit_rate_after;

    /* used to learn the Apache compatible response length without a header */
    size_t                            header_size;
	//HTTP请求的全部长度，包括HTTP包体
    off_t                             request_length;		//"GET / HTTP/1.1\r\n"

    ngx_uint_t                        err_status;

    ngx_http_connection_t            *http_connection;
#if (NGX_HTTP_V2)
    ngx_http_v2_stream_t             *stream;
#endif

    ngx_http_log_handler_pt           log_handler;

	//在这个请求中如果打开了某些资源，并需要在请求结束时释放，那么都需要在把定义的释放资源方法添加到cleanup成员中
    ngx_http_cleanup_t               *cleanup;

	//表示当前请求的引用次数。例如，在使用subrequest功能时，依附在这个请求上的子请求数目会返回到count上，
	//每增加一个子请求，count数就要加1.其中任何一个子请求派生出新的子请求时，对应的原始请求(main指针指向的请求)
	//的count值都要加1。又如，当我们接收HTTP包体时，由于这也是一个异步调用，所以count上也需要加1，这样在结束请求时，
	//就不会在count引用计数未清零时销毁请求。

	//每当派生出子请求时，原始请求的count成员都会加1，在正真销毁请求前，可以通过检查count成员是否为0以确认是否销毁
	//原始请求，这样可以做到唯有所有的子请求都结束时，原始请求才会销毁，内存池、TCP连接等资源才会释放。

	//在HTTP模块中每进行一类新的操作，包括为一个请求添加新的事件，或者把一些已经由定时器、epoll中移除的事件重新加入其中，
	//都需要把这个请求的引用计数加1。这是因为需要让HTTP框架知道HTTP模块对于该请求有独立的异步处理机制，将由该HTTP模块决
	//定这个操作什么时候结束，防止在这个操作还未结束时HTTP框架却把这个请求销毁了(如其他HTTP模块通过调用ngx_http_finalize_request
	//方法要求HTTP框架结束请求)，导致请求出现不可知的严重错误。这就要求每个操作在"认为"自身的动作结束时，都得最终调用到
	//ngx_http_close_request方法，该方法会自动检查引用计数，当引用计数为0时才真正的销毁请求。实际上，很多结束请求的方法
	//最后一定会调用到ngx_http_close_request方法。

	//引用计数一般都作用于这个请求的主请求上，因此，在结束请求时同一检查主请求的引用计数就可以了。当然，目前的HTTP框架也
	//要求我们必须这么做，因为ngx_http_close_request方法只是把主请求上单 引用计数减1.
    unsigned                          count:16;
    unsigned                          subrequests:8;
	//阻塞标志位，目前仅由aio使用
    unsigned                          blocked:8;
	//标志位，为1时表示当前请求正在使用异步文件IO
    unsigned                          aio:1;

    unsigned                          http_state:4;

    /* URI with "/." and on Win32 with "//" */
    unsigned                          complex_uri:1;

    /* URI with "%" */
    unsigned                          quoted_uri:1;

    /* URI with "+" */
    unsigned                          plus_in_uri:1;

    /* URI with " " */
    unsigned                          space_in_uri:1;

    unsigned                          invalid_header:1;

    unsigned                          add_uri_to_alias:1;
    unsigned                          valid_location:1;
    unsigned                          valid_unparsed_uri:1;
	//标志位，为1时表示URL发生过rewrite重写
    unsigned                          uri_changed:1;
	//表示使用rewrite重写URL的次数。因为目前最多可以更改10次，所以uri_changes初始化为11，
	//而每次重写URL一次就把uri_changes减1，一旦uri_changes等于0，则向用户返回失败
    unsigned                          uri_changes:4;

    unsigned                          request_body_in_single_buf:1;
    unsigned                          request_body_in_file_only:1;
    unsigned                          request_body_in_persistent_file:1;
    unsigned                          request_body_in_clean_file:1;
    unsigned                          request_body_file_group_access:1;
    unsigned                          request_body_file_log_level:3;
    unsigned                          request_body_no_buffering:1;

    unsigned                          subrequest_in_memory:1;
    unsigned                          waited:1;

#if (NGX_HTTP_CACHE)
    unsigned                          cached:1;
#endif

#if (NGX_HTTP_GZIP)
    unsigned                          gzip_tested:1;
    unsigned                          gzip_ok:1;
    unsigned                          gzip_vary:1;
#endif

    unsigned                          proxy:1;
    unsigned                          bypass_cache:1;
    unsigned                          no_cache:1;

    /*
     * instead of using the request context data in
     * ngx_http_limit_conn_module and ngx_http_limit_req_module
     * we use the single bits in the request structure
     */
    unsigned                          limit_conn_set:1;
    unsigned                          limit_req_set:1;

#if 0
    unsigned                          cacheable:1;
#endif

    unsigned                          pipeline:1;
    unsigned                          chunked:1;
    unsigned                          header_only:1;
	//标志位，为1时表示当前请求是keepalive请求
    unsigned                          keepalive:1;
	//延迟关闭标志位，为1时表示需要延迟关闭。例如，在接收完HTTP头部时如果发现包体存在，
	//该标志位会设为1，而放弃接收包体时则会设为0
    unsigned                          lingering_close:1;
	//标志位，为1时表示正在丢弃HTTP请求中的包体
    unsigned                          discard_body:1;
    unsigned                          reading_body:1;
	//该请求属于内部请求对象(例如内部跳转、命名location跳转、子请求等)
	//标志位，为1时表示请求的当前状态是在做内部跳转
    unsigned                          internal:1;			
    unsigned                          error_page:1;
    unsigned                          filter_finalize:1;
    unsigned                          post_action:1;
    unsigned                          request_complete:1;
    unsigned                          request_output:1;
	//标志位，为1时表示发送给客户端的HTTP响应头部已经发送。在调用ngx_http_send_header方法后，
	//若已经成功地启动响应头部发送流程，该标志位就会置1，用来防止反复地发送头部
    unsigned                          header_sent:1;
    unsigned                          expect_tested:1;
    unsigned                          root_tested:1;
    unsigned                          done:1;
    unsigned                          logged:1;

	//表示缓存区是否有待发送内容的标志位
    unsigned                          buffered:4;

    unsigned                          main_filter_need_in_memory:1;
    unsigned                          filter_need_in_memory:1;
    unsigned                          filter_need_temporary:1;
    unsigned                          allow_ranges:1;
    unsigned                          single_range:1;
    unsigned                          disable_not_modified:1;

#if (NGX_STAT_STUB)
    unsigned                          stat_reading:1;
    unsigned                          stat_writing:1;
#endif

    /* used to parse HTTP headers */
	//状态机解析HTTP时使用state来表示当前的解析状态
    ngx_uint_t                        state;

    ngx_uint_t                        header_hash;
    ngx_uint_t                        lowcase_index;
    u_char                            lowcase_header[NGX_HTTP_LC_HEADER_LEN];

    u_char                           *header_name_start;    //请求头key字段起始地址
    u_char                           *header_name_end;		//请求头key字段结束地址
    u_char                           *header_start;			//请求头value字段起始地址
    u_char                           *header_end;			//请求头value字段结束地址

    /*
     * a memory that can be reused after parsing a request line
     * via ngx_http_ephemeral_t
     */

    u_char                           *uri_start;
    u_char                           *uri_end;
    u_char                           *uri_ext;
    u_char                           *args_start;
    u_char                           *request_start;
    u_char                           *request_end;
    u_char                           *method_end;
    u_char                           *schema_start;
    u_char                           *schema_end;
    u_char                           *host_start;
    u_char                           *host_end;
    u_char                           *port_start;
    u_char                           *port_end;

    unsigned                          http_minor:16;
    unsigned                          http_major:16;
};


typedef struct {
    ngx_http_posted_request_t         terminal_posted_request;
} ngx_http_ephemeral_t;


#define ngx_http_ephemeral(r)  (void *) (&r->uri_start)


extern ngx_http_header_t       ngx_http_headers_in[];
extern ngx_http_header_out_t   ngx_http_headers_out[];


#define ngx_http_set_log_request(log, r)                                      \
    ((ngx_http_log_ctx_t *) log->data)->current_request = r


#endif /* _NGX_HTTP_REQUEST_H_INCLUDED_ */
