
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    u_char    *name;
    uint32_t   method;
} ngx_http_method_name_t;


#define NGX_HTTP_REQUEST_BODY_FILE_OFF    0
#define NGX_HTTP_REQUEST_BODY_FILE_ON     1
#define NGX_HTTP_REQUEST_BODY_FILE_CLEAN  2


static ngx_int_t ngx_http_core_find_location(ngx_http_request_t *r);
static ngx_int_t ngx_http_core_find_static_location(ngx_http_request_t *r,
    ngx_http_location_tree_node_t *node);

static ngx_int_t ngx_http_core_preconfiguration(ngx_conf_t *cf);
static ngx_int_t ngx_http_core_postconfiguration(ngx_conf_t *cf);
static void *ngx_http_core_create_main_conf(ngx_conf_t *cf);
static char *ngx_http_core_init_main_conf(ngx_conf_t *cf, void *conf);
static void *ngx_http_core_create_srv_conf(ngx_conf_t *cf);
static char *ngx_http_core_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);
static void *ngx_http_core_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_core_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child);

static char *ngx_http_core_server(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy);
static char *ngx_http_core_location(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy);
static ngx_int_t ngx_http_core_regex_location(ngx_conf_t *cf, ngx_http_core_loc_conf_t *clcf, ngx_str_t *regex, ngx_uint_t caseless);

static char *ngx_http_core_types(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_core_type(ngx_conf_t *cf, ngx_command_t *dummy,
    void *conf);

static char *ngx_http_core_listen(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_core_server_name(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_core_root(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_core_limit_except(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_core_set_aio(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_core_directio(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_core_error_page(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_core_try_files(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_core_open_file_cache(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_core_error_log(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_core_keepalive(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_core_internal(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_core_resolver(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
#if (NGX_HTTP_GZIP)
static ngx_int_t ngx_http_gzip_accept_encoding(ngx_str_t *ae);
static ngx_uint_t ngx_http_gzip_quantity(u_char *p, u_char *last);
static char *ngx_http_gzip_disable(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
#endif
static ngx_int_t ngx_http_get_forwarded_addr_internal(ngx_http_request_t *r,
    ngx_addr_t *addr, u_char *xff, size_t xfflen, ngx_array_t *proxies,
    int recursive);
#if (NGX_HAVE_OPENAT)
static char *ngx_http_disable_symlinks(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
#endif

static char *ngx_http_core_lowat_check(ngx_conf_t *cf, void *post, void *data);
static char *ngx_http_core_pool_size(ngx_conf_t *cf, void *post, void *data);

static ngx_conf_post_t  ngx_http_core_lowat_post =
    { ngx_http_core_lowat_check };

static ngx_conf_post_handler_pt  ngx_http_core_pool_size_p = ngx_http_core_pool_size;


static ngx_conf_enum_t  ngx_http_core_request_body_in_file[] = 
{
    { ngx_string("off"), NGX_HTTP_REQUEST_BODY_FILE_OFF },
    { ngx_string("on"), NGX_HTTP_REQUEST_BODY_FILE_ON },
    { ngx_string("clean"), NGX_HTTP_REQUEST_BODY_FILE_CLEAN },
    { ngx_null_string, 0 }
};


static ngx_conf_enum_t  ngx_http_core_satisfy[] = {
    { ngx_string("all"), NGX_HTTP_SATISFY_ALL },
    { ngx_string("any"), NGX_HTTP_SATISFY_ANY },
    { ngx_null_string, 0 }
};


static ngx_conf_enum_t  ngx_http_core_lingering_close[] = 
{
    { ngx_string("off"), NGX_HTTP_LINGERING_OFF },
    { ngx_string("on"), NGX_HTTP_LINGERING_ON },
    { ngx_string("always"), NGX_HTTP_LINGERING_ALWAYS },
    { ngx_null_string, 0 }
};


static ngx_conf_enum_t  ngx_http_core_if_modified_since[] = {
    { ngx_string("off"), NGX_HTTP_IMS_OFF },
    { ngx_string("exact"), NGX_HTTP_IMS_EXACT },
    { ngx_string("before"), NGX_HTTP_IMS_BEFORE },
    { ngx_null_string, 0 }
};


static ngx_conf_bitmask_t  ngx_http_core_keepalive_disable[] = {
    { ngx_string("none"), NGX_HTTP_KEEPALIVE_DISABLE_NONE },
    { ngx_string("msie6"), NGX_HTTP_KEEPALIVE_DISABLE_MSIE6 },
    { ngx_string("safari"), NGX_HTTP_KEEPALIVE_DISABLE_SAFARI },
    { ngx_null_string, 0 }
};


static ngx_path_init_t  ngx_http_client_temp_path = {
    ngx_string(NGX_HTTP_CLIENT_TEMP_PATH), { 0, 0, 0 }
};


#if (NGX_HTTP_GZIP)

static ngx_conf_enum_t  ngx_http_gzip_http_version[] = {
    { ngx_string("1.0"), NGX_HTTP_VERSION_10 },
    { ngx_string("1.1"), NGX_HTTP_VERSION_11 },
    { ngx_null_string, 0 }
};


static ngx_conf_bitmask_t  ngx_http_gzip_proxied_mask[] = {
    { ngx_string("off"), NGX_HTTP_GZIP_PROXIED_OFF },
    { ngx_string("expired"), NGX_HTTP_GZIP_PROXIED_EXPIRED },
    { ngx_string("no-cache"), NGX_HTTP_GZIP_PROXIED_NO_CACHE },
    { ngx_string("no-store"), NGX_HTTP_GZIP_PROXIED_NO_STORE },
    { ngx_string("private"), NGX_HTTP_GZIP_PROXIED_PRIVATE },
    { ngx_string("no_last_modified"), NGX_HTTP_GZIP_PROXIED_NO_LM },
    { ngx_string("no_etag"), NGX_HTTP_GZIP_PROXIED_NO_ETAG },
    { ngx_string("auth"), NGX_HTTP_GZIP_PROXIED_AUTH },
    { ngx_string("any"), NGX_HTTP_GZIP_PROXIED_ANY },
    { ngx_null_string, 0 }
};


static ngx_str_t  ngx_http_gzip_no_cache = ngx_string("no-cache");
static ngx_str_t  ngx_http_gzip_no_store = ngx_string("no-store");
static ngx_str_t  ngx_http_gzip_private = ngx_string("private");

#endif

static ngx_command_t  ngx_http_core_commands[] = 
{

    { ngx_string("variables_hash_max_size"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_core_main_conf_t, variables_hash_max_size),
      NULL },

    { ngx_string("variables_hash_bucket_size"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_core_main_conf_t, variables_hash_bucket_size),
      NULL 
     },
	//语法: server_names_hash_max_size size;
	//默认: server_names_hash_max_size 512;
	//设置存储所有server_name的散列表的散列桶的个数
    { 
    	ngx_string("server_names_hash_max_size"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_num_slot,
		NGX_HTTP_MAIN_CONF_OFFSET,
		offsetof(ngx_http_core_main_conf_t, server_names_hash_max_size),
		NULL 
    },
	//语法: server_names_hash_bucket_size size;
	//默认: server_names_hash_bucket_size 32 | 64 | 128;
	//设置存储所有server_name的散列表的每个散列桶占用的内存大小
    { 
    	ngx_string("server_names_hash_bucket_size"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_num_slot,
		NGX_HTTP_MAIN_CONF_OFFSET,
		offsetof(ngx_http_core_main_conf_t, server_names_hash_bucket_size),
		NULL 
    },

	//每一个server块就是一个虚拟主机，它只处理与之相对应的主机域名请求
	//这样，一台服务器上的Nginx就能以不同的方式处理访问不同主机域名的HTTP请求了
    { 
		ngx_string("server"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_NOARGS,
		ngx_http_core_server,
		0,
		0,
		NULL 
    },
    //语法: connection_pool_size size;
	//默认: connection_pool_size 256;
	//指定每个TCP连接上初始内存池大小，用于减少内核对于小块内存的分配次数
    { ngx_string("connection_pool_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_core_srv_conf_t, connection_pool_size),
      &ngx_http_core_pool_size_p },

	//语法: request_pool_size size;
	//默认: request_pool_size 4k;
	//指定每个HTTP请求的初始内存池大小，用于减少内核对于小块内存的分配次数
    { 
		ngx_string("request_pool_size"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_size_slot,
		NGX_HTTP_SRV_CONF_OFFSET,
		offsetof(ngx_http_core_srv_conf_t, request_pool_size),
		&ngx_http_core_pool_size_p 
    },
	//语法: client_header_timeout time (默认单位:秒);
	//默认: client_header_timeout 60;
	//读取HTTP头部(请求行和请求头)的超时时间
    { 
		ngx_string("client_header_timeout"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_msec_slot,
		NGX_HTTP_SRV_CONF_OFFSET,
		offsetof(ngx_http_core_srv_conf_t, client_header_timeout),
		NULL 
    },

    { 
    	ngx_string("client_header_buffer_size"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_size_slot,
		NGX_HTTP_SRV_CONF_OFFSET,
		offsetof(ngx_http_core_srv_conf_t, client_header_buffer_size),
		NULL 
    },
	//语法: large_client_header_buffers number size;
	//默认: large_client_header_buffers 4 8k;
	//存储超大HTTP头部的内存buffer大小和个数
    { 
		ngx_string("large_client_header_buffers"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE2,
		ngx_conf_set_bufs_slot,
		NGX_HTTP_SRV_CONF_OFFSET,
		offsetof(ngx_http_core_srv_conf_t, large_client_header_buffers),
		NULL 
    },
	//语法: ignore_invalid_headers on | off;
	//默认: ignore_invalid_headers on;
	//忽略不合法的HTTP头部
	//如果将其设置为off，那么当出现不和法的HTTP头部时，Nginx会拒绝服务，并直接向用户发送400(Bad Request)错误
	//如果将其设置为on，则会忽略此HTTP头部
    { 
		ngx_string("ignore_invalid_headers"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_SRV_CONF_OFFSET,
		offsetof(ngx_http_core_srv_conf_t, ignore_invalid_headers),
		NULL 
    },
	//语法: merge_slashes on | off;
	//默认: merge_slashes off;
	//是否合并相邻的'/'
	//例如: //test///a.txt，在配置为on时，会将其匹配为 location /test/a.txt;
	//	如果配置为off，则不会匹配，URI仍将是//test///a.txt
    { 
		ngx_string("merge_slashes"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_SRV_CONF_OFFSET,
		offsetof(ngx_http_core_srv_conf_t, merge_slashes),
		NULL 
    },
	//语法: underscores_in_headers on | off;
	//默认: underscores_in_headers off;
	//是否允许HTTP头部的名称中带'_'(下划线)
    { 
		ngx_string("underscores_in_headers"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_SRV_CONF_OFFSET,
		offsetof(ngx_http_core_srv_conf_t, underscores_in_headers),
		NULL 
    },

    { 
    	ngx_string("location"),
		NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_BLOCK|NGX_CONF_TAKE12,
		ngx_http_core_location,
		NGX_HTTP_SRV_CONF_OFFSET,
		0,
		NULL 
    },

	//语法: listen address:port [default(deprecated in 0.8.21) | default_server |[ backlog=num |rcvbuf=size | sndbuf=size | accept_filter=filter | deferred | bind | ipv6only=[on | off] | ssl ]];
	//默认: listen 80;
	//决定Nginx服务如何监听端口
	//default: 将所在的server块作为整个Web服务的默认server块。如果没有设置这个参数，那么将会以在nginx.conf中找到的第一个server块作为默认server块。当一个请求无法匹配配置文件中的所有主机域名时，就会选用
	//	默认的虚拟主机
	//default_server: 同上
	//backlog=num: 表示TCP中backlog队列的大小。默认为-1，表示不予设置。
	//rcvbuf=size: 设置监听句柄的SO_RCVBUF参数
	//sndbuf=size: 设置监听句柄的SO_SNDBUF参数
	//accept_filter=filter: 设置accept过滤器，只对FreeBSD操作系统有用
	//deferred: 在设置该参数后，若用户发起建立连接请求，并且完成了TCP的三次握手，内核也不会为了这次的连接调度worker进程来处理，
	//	只有用户真的发送请求数据时(内核已经在网卡中收到请求数据包)，内核才会唤醒worker进程处理这个连接。这个参数适用于大并发的情况下，
	//	它减轻了worker进程的负担。当请求数据来临时，worker进程才会开始处理这个连接。只有确认上面所说的应用场景符合自己的业务需求时，才可以使用deferred配置。
	//bind: 绑定当前端口/地址对，如127.0.0.1:8000。只有同时对一个端口监听多个地址才会生效
	//ssl: 在当前监听的端口上建立的连接必须基于SSL协议
    { 
    	ngx_string("listen"),
		NGX_HTTP_SRV_CONF|NGX_CONF_1MORE,
		ngx_http_core_listen,
		NGX_HTTP_SRV_CONF_OFFSET,
		0,
		NULL 
    },
	//语法: server_name name [...];
	//默认: server_name "";
	//server_name配置所在的server块配置所对应主机名称
	//server_name后可以跟多个主机名称
	//Nginx正是使用server_name配置项针对特定Host域名的请求提供不同的服务，以此实现虚拟主机的功能
	//例如: server_name www.testweb.com download.testweb.com;
    { 
		ngx_string("server_name"),
		NGX_HTTP_SRV_CONF|NGX_CONF_1MORE,
		ngx_http_core_server_name,
		NGX_HTTP_SRV_CONF_OFFSET,
		0,
		NULL 
    },
	//语法: types_hash_max_size size;
	//默认: types_hash_max_size 1024;
	//存储MIME type到文件扩展名的映射的散列表的冲突桶的个数
    { 
		ngx_string("types_hash_max_size"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_num_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, types_hash_max_size),
		NULL 
    },
	//语法: types_hash_bucket_size size;
	//默认: types_hash_bucket_size 32|64|128;
	//存储MIME type到文件扩展名的映射的散列表的冲突桶占用的内存大小
    { 
    	ngx_string("types_hash_bucket_size"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_num_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, types_hash_bucket_size),
		NULL 
    },
    
	//语法: types {...};
	//定义文件扩展名到MIME type的映射。
	//多个扩展名可以映射到同一个MIME type
	{ 
		ngx_string("types"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF |NGX_CONF_BLOCK|NGX_CONF_NOARGS,
		ngx_http_core_types,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL 
	},

	//语法: default_type MIME-type;
	//默认: defautl_type text/plain;
	//当找不到相应的MIME type与文件扩展名之间的映射时，
	//使用默认的MIME type作为HTTP header中的Content-type
    { 
		ngx_string("default_type"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_str_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, default_type),
		NULL 
    },

	//语法: root path;
	//默认: root html;
	//以root方式设置资源路径
    { 
		ngx_string("root"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
		ngx_http_core_root,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL 
    },
    
	//语法: alias path;
	////以alias方式设置资源路径
    { 
		ngx_string("alias"),
		NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_http_core_root,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL 
    },

	//语法: limit_except method ...{};
	//例子:
	//		limit_except GET {
	//			allow 192.168.1.0/32
	//			deny all;
	//		}
	//按HTTP方法名限制用户请求
	//方法名可取值包括:GET/HEAD/POST/PUT/DELETE/MKCOL/COPY/
	//MOVE/OPTIONS/PROPFIND/PROPPATCH/LOCK/UNLOCK/PATCH
	//注意:允许GET方法就意味着也允许HEAD方法
    { 
		ngx_string("limit_except"),
		NGX_HTTP_LOC_CONF|NGX_CONF_BLOCK|NGX_CONF_1MORE,
		ngx_http_core_limit_except,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL 
    },
	//语法: client_max_body_size size;
	//默认: client_max_body_size 1m;
	//HTTP请求包体的最大值
    { 
		ngx_string("client_max_body_size"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_off_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, client_max_body_size),
		NULL 
    },
	//语法: client_body_buffer_size time size;
	//默认: client_body_buffer_size 8k/16k;
	//存储HTTP包体的内存buffer大小
    { 
   		ngx_string("client_body_buffer_size"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_size_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, client_body_buffer_size),
		NULL 
    },

	//语法: client_body_timeout time (默认单位:秒);
	//默认: client_body_timeout 60;
	//读取HTTP包体的超时时间
    { 
		ngx_string("client_body_timeout"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_msec_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, client_body_timeout),
		NULL 
    },
	//语法: client_body_temp_path dir-path [level1 [level2 [levle3]]]
	//默认: client_body_temp_path client_body_temp;
	//定义HTTP包体存放的临时目录。
	//在接收HTTP包体时，如果包体的大小大于client_body_buffer_size，则会以一个递增的
	//整数命名并存放到client_body_temp_path指定的目录中。后面跟着的level1、level2、level3，
	//是为了防止一个目录下的文件数量太多，从而导致性能下降，因此使用了level参数，这样可以
	//按照临时文件名最多再加三层目录。
	//例如: client_body_temp_path /opt/nginx/client_temp 1 2
	//	如果新上传的HTTP包体使用00000123456作为临时文件名，就会被存放在这个目录中
	//	/ope/nginx/client_temp/6/45/00000123456
    { 
		ngx_string("client_body_temp_path"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1234,
		ngx_conf_set_path_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, client_body_temp_path),
		NULL 
    },
	//语法: client_body_in_file_only on | clean| off;
	//默认: client_body_in_file_only off;
	//http包体只存储到磁盘文件中
	//当值为off时，用户请求中的HTTP包体一律存储到磁盘文件中，即使只有0个字节也会存储为文件。
	//当请求结束时，如果配置为on，则这个文件不会被删除(该配置一般用于调试，定位问题)，但
	//如果配置为clean，则会删除这个文件
    { 
    	ngx_string("client_body_in_file_only"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_enum_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, client_body_in_file_only),
      &ngx_http_core_request_body_in_file 
    },
   	//语法: client_body_in_single_buffer on | off;
	//默认: client_body_in_single_buffer off;
	//HTTP包体尽量写到一个内存buffer中
	//用户请求中的HTTP包体一律存储到内存buffer中，当然，如果HTTP包体的大小
	//超过了client_body_buffer_size设置的值，包体还是会写入到磁盘文件中
    { ngx_string("client_body_in_single_buffer"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, client_body_in_single_buffer),
      NULL },
	//语法: sendfile on | off;
	//默认: sendfile off;
	//确定是否启用sendfile系统调用来发送文件
    { 
		ngx_string("sendfile"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF |NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, sendfile),
		NULL 
    },

    { ngx_string("sendfile_max_chunk"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, sendfile_max_chunk),
      NULL },

	//语法: aio on | off;
	//默认: aio off;
	//是否在FreeBSD或Linux系统上启用内核级别的异步文件I/O功能。
	//注意: 它与sendfile功能是互斥的
    { 
		ngx_string("aio"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_http_core_set_aio,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL 
    },

    { ngx_string("read_ahead"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, read_ahead),
      NULL },
	//语法: directio size | off;
	//默认: directio off;
	//指定在FreeBSD和Linux系统上使用O_DIRECT选项去读取文件时，
	//缓冲区的大小为size，通常对大文件的读取速度有优化作用。
	//注意，它与sendfile功能是互斥的
    { 
		ngx_string("directio"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_http_core_directio,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL 
    },
	//语法: directio_alignment size;
	//默认: directio_alignment 512;
	//与directio配合使用，指定以directio方式读取文件时的对齐方式。
    { ngx_string("directio_alignment"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_off_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, directio_alignment),
      NULL },
	//语法: tcp_nopush on | off;
	//默认: tcp_nopush off;
	//确定是否开启FreeBSD系统上的TCP_NOPUSH或Linux系统上的TCP_CORK功能。
	//仅在使用sendfile的时候才有效
	//打开tcp_nopush后，将会在发送响应时把整个响应包头放到一个TCP包中发送
    { 
		ngx_string("tcp_nopush"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, tcp_nopush),
		NULL 
    },
	//语法: tcp_nodelay on | off;
	//默认: tcp_nodelay on;
	//开启或关闭Nginx使用TCP_NODELAY选项的功能(禁用Nagle算法) 
	//这个选项在将连接转变为长连接的时候启用，在upstream发送响应到客户端时也会启用
    { ngx_string("tcp_nodelay"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, tcp_nodelay),
      NULL },

	//语法: send_timeout time (默认单位:秒);
	//默认: send_timeout 60;
	//发送响应的超时时间
    { ngx_string("send_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, send_timeout),
      NULL },

    { ngx_string("send_lowat"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, send_lowat),
      &ngx_http_core_lowat_post },

    { 
		ngx_string("postpone_output"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_size_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, postpone_output),
		NULL 
    },
			
	//语法: limit_rate speed;
	//默认: limit_rate 0;
	//对客户端请求限制每秒传输的字节数，0表示不限速
	//针对不同的客户端，可以用$limit_rate参数执行不同的限速策略。
	//例如:
	//server {
	//	if($slow) {
	//		set $limit_rate 4k;
	//	}
	//}
    { 
		ngx_string("limit_rate"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF |NGX_CONF_TAKE1,
		ngx_conf_set_size_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, limit_rate),
		NULL 
    },
	//语法: limit_rate_after speed;
	//默认: limit_rate_after 1m;
	//向客户端发送的响应长度超过limit_rate_after后才开始限速
    { 
		ngx_string("limit_rate_after"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF |NGX_CONF_TAKE1,
		ngx_conf_set_size_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, limit_rate_after),
		NULL 
    },

	//语法: keepalive_timeout time1 [time2] (默认单位: 秒);
	//默认: keepalive_timeout 75 0;
	//time1是keepalive超时时间，time2是Nginx发送给客户端响应头中的Keep-Alive域的时间
    { 
		ngx_string("keepalive_timeout"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE12,
		ngx_http_core_keepalive,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL 
    },

	//语法: keepalive_requests n;
	//默认: keepalive_requests 100;
	//一个keepalive长连接上允许承载的请求最大数目
    { 
		ngx_string("keepalive_requests"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_num_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, keepalive_requests),
		NULL 
    },

	//语法: keepalive_disable [msie6|safari|none]...;
	//默认: keepalive_disable msie6 safari;
	//对某些浏览器禁用keepalive功能
    { 
		ngx_string("keepalive_disable"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE12,
		ngx_conf_set_bitmask_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, keepalive_disable),
		&ngx_http_core_keepalive_disable 
    },

    { ngx_string("satisfy"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_enum_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, satisfy),
      &ngx_http_core_satisfy },

    { ngx_string("internal"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_core_internal,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },
	
	//语法: lingering_close off | on | always;
	//默认: lingering_close on;
	//控制Nginx关闭用户连接的方式(延迟关闭)
    { ngx_string("lingering_close"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_enum_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, lingering_close),
      &ngx_http_core_lingering_close },

	//语法: lingering_time time;
	//默认: lingering_time 30s;
	//保持延迟关闭状态的最长持续时间
    { ngx_string("lingering_time"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, lingering_time),
      NULL },
      
	//语法: lingering_timeout time;
	//默认: lingering_timeout 5;
	//保持延迟关闭状态的最长持续时间
    { ngx_string("lingering_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, lingering_timeout),
      NULL },

	//语法: reset_timedout_connection on | off;
	//默认: reset_timedout_connection off;
	//连接超时后将通过向客户端发送RST包来直接重置连接
    { 
		ngx_string("reset_timedout_connection"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, reset_timedout_connection),
		NULL 
    },
	//语法: server_name_in_redirect on | off;
	//默认: server_name_in_redirect on;
	//重定向主机名称的处理，需要配合server_name配置使用
	//on--表示在重定向请求时会使用server_name里配置的第一个主机名代替原先请求的Host头部，
	//off--表示在重定向请求时使用请求本身的Host头部
    { 
		ngx_string("server_name_in_redirect"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, server_name_in_redirect),
		NULL 
    },

	//语法：port_in_redirect [ on|off ] 
	//默认值：port_in_redirect on 
	//指定是否在让nginx在重定向操作中对端口进行操作。
	//如果设置为off，在重定向的请求中nginx不会在url中添加端口。
    { 
		ngx_string("port_in_redirect"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, port_in_redirect),
		NULL 
    },

    { ngx_string("msie_padding"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, msie_padding),
      NULL },

    { ngx_string("msie_refresh"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, msie_refresh),
      NULL },
	//语法: log_not_found on | off;
	//默认: log_not_found exact;
	//当处理用户请求且需要访问文件时，如果没有找到文件，是否将错误日志
	//记录到erro.log文件中。这仅用于定位问题
    { 
		ngx_string("log_not_found"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, log_not_found),
		NULL 
    },

    { ngx_string("log_subrequest"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, log_subrequest),
      NULL },

    { ngx_string("recursive_error_pages"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, recursive_error_pages),
      NULL },
	//语法: server_tokens on | off;
	//默认: server_tokens on;
	//处理请求出错时是否在响应的Server头部中表明Nginx版本，这是为了方便问题
    { 
		ngx_string("server_tokens"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, server_tokens),
		NULL 
    },
	//语法: if_modified_since [off | exact | before];
	//默认: if_modified_since exact;
	//对If_Modified_Since头部的处理策略
	//off--表示忽略用户请求中的If_Modified_Since头部。这时，如果获取一个文件，
	//	那么会正常地返回文件内容。HTTP响应码通常是200
	//exact--将If_Modified_Since头部包含的时间与将要返回的文件上次修改的时间做精确比较，
	//	如果没有匹配上，则返回200和文件的实际内容，如果匹配上，则表示浏览器缓存的文件内容已经是最新的了，
	//	没有必要再返回文件从而浪费时间与带宽了，这时会返回304 Not Modified，浏览器收到后会直接读取自己的本地缓存
	//before--是比exact更宽松的比较。只要文件的上次修改时间等于或早于用户请求中的If_Modified_Since头部时间，
	//	就会向客户端返回304 Not Modified。
    { 
		ngx_string("if_modified_since"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_enum_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, if_modified_since),
		&ngx_http_core_if_modified_since 
    },

    { ngx_string("max_ranges"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, max_ranges),
      NULL },

    { ngx_string("chunked_transfer_encoding"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, chunked_transfer_encoding),
      NULL },

    { ngx_string("etag"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, etag),
      NULL },

    { ngx_string("error_page"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_2MORE,
      ngx_http_core_error_page,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { 
		ngx_string("try_files"),
		NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_2MORE,
		ngx_http_core_try_files,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL 
    },

	//语法: post_action [ uri|off ] 
	//默认: post_action off 
	//为当前完成请求的子请求定义一个URI。
	//例如: location /protected_files { 
	//			internal;
	//			proxy_pass http://127.0.0.2;
	//			post_action /protected_done;
	//		}
	//		#发送post_action 请求到FastCGI后端进行登录验证。
	//		location /protected_done {
	// 			internal;
	// 			fastcgi_pass 127.0.0.1:9000;
	//		}
    { 
    	ngx_string("post_action"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF |NGX_CONF_TAKE1,
		ngx_conf_set_str_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, post_action),
		NULL 
    },

    { 
    	ngx_string("error_log"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_core_error_log,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL 
   	},

	//语法: open_file_cache max=N [inactive=time] | off;
	//默认: open_file_cache off;
	//打开文件缓存，这样通过读取缓存减少了对磁盘的操作
	//max--表示在内存中存储元素的最大个数。当达到最大限制数量后，将采用LRU(Least Recently Used)
	//算法从缓存中淘汰最近最少使用的元素
	//inactive--表示在inactive指定的时间段内没有被访问过的元素将会被淘汰。默认时间为60秒
	//关闭缓存功能
    { 
		ngx_string("open_file_cache"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE12,
		ngx_http_core_open_file_cache,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, open_file_cache),
		NULL 
    },
	//语法: open_file_cache_valid time;
	//默认: open_file_cache_valid 60s;
	//检验文件缓存中元素有效性的频率
    { 
		ngx_string("open_file_cache_valid"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_sec_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, open_file_cache_valid),
		NULL 
    },

	//语法: open_file_cache_min_uses number;
	//默认: open_file_cache_min_uses 1;
	//指定文件缓存中，在inactive指定的时间段内，访问次数超过了open_file_cache_min_uses
	//指定的最小次数，那么将不会被淘汰出缓存
    { 
		ngx_string("open_file_cache_min_uses"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_num_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, open_file_cache_min_uses),
		NULL 
    },

	//语法: open_file_cache_errors on | off;
	//默认: open_file_cache_errors off;
	//是否在文件缓存中缓存打开文件时出现的找不到路径、没有权限等错误信息
    { 
		ngx_string("open_file_cache_errors"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, open_file_cache_errors),
		NULL 
    },

    { ngx_string("open_file_cache_events"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, open_file_cache_events),
      NULL },
	//语法: resolver address ...;
	//设置DNS域名解析服务器的地址
	//例如 resolver 127.0.0.1 192.0.2.1
    { 
		ngx_string("resolver"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
		ngx_http_core_resolver,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL 
    },
	//语法: resolver_timeout tiem;
	//默认: resolver_timeout 30s;
	//DNS解析的超时时间
    { 
		ngx_string("resolver_timeout"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_msec_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_core_loc_conf_t, resolver_timeout),
		NULL 
    },

#if (NGX_HTTP_GZIP)

    { ngx_string("gzip_vary"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, gzip_vary),
      NULL },

    { ngx_string("gzip_http_version"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_enum_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, gzip_http_version),
      &ngx_http_gzip_http_version },

    { ngx_string("gzip_proxied"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_core_loc_conf_t, gzip_proxied),
      &ngx_http_gzip_proxied_mask },

    { ngx_string("gzip_disable"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_gzip_disable,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

#endif

#if (NGX_HAVE_OPENAT)

    { ngx_string("disable_symlinks"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE12,
      ngx_http_disable_symlinks,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

#endif

      ngx_null_command
};


static ngx_http_module_t  ngx_http_core_module_ctx = 
{
    ngx_http_core_preconfiguration,        /* preconfiguration */
    ngx_http_core_postconfiguration,       /* postconfiguration */

    ngx_http_core_create_main_conf,        /* create main configuration */
    ngx_http_core_init_main_conf,          /* init main configuration */

    ngx_http_core_create_srv_conf,         /* create server configuration */
    ngx_http_core_merge_srv_conf,          /* merge server configuration */

    ngx_http_core_create_loc_conf,         /* create location configuration */
    ngx_http_core_merge_loc_conf           /* merge location configuration */
};


ngx_module_t  ngx_http_core_module = 
{
    NGX_MODULE_V1,
    &ngx_http_core_module_ctx,             /* module context */
    ngx_http_core_commands,                /* module directives */
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


ngx_str_t  ngx_http_core_get_method = { 3, (u_char *) "GET " };


void
ngx_http_handler(ngx_http_request_t *r)
{
    ngx_http_core_main_conf_t  *cmcf;

    r->connection->log->action = NULL;

    r->connection->unexpected_eof = 0;

    if (!r->internal) 
	{
        switch (r->headers_in.connection_type) 
		{
        case 0:
            r->keepalive = (r->http_version > NGX_HTTP_VERSION_10);
            break;

        case NGX_HTTP_CONNECTION_CLOSE:
            r->keepalive = 0;
            break;

        case NGX_HTTP_CONNECTION_KEEP_ALIVE:
            r->keepalive = 1;
            break;
        }

        r->lingering_close = (r->headers_in.content_length_n > 0 || r->headers_in.chunked);
        r->phase_handler = 0;

    } 
	else
	{
        cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
        r->phase_handler = cmcf->phase_engine.server_rewrite_index;
    }

    r->valid_location = 1;
#if (NGX_HTTP_GZIP)
    r->gzip_tested = 0;
    r->gzip_ok = 0;
    r->gzip_vary = 0;
#endif

    r->write_event_handler = ngx_http_core_run_phases;
    ngx_http_core_run_phases(r);
}

//开始调用各个HTTP模块共同处理请求
//由于回调函数的返回值会影响到同一阶段的后续回调函数的处理与否，
//而nginx又采用先进后出的方案，即先注册的模块，其回调函数反而后执行，
//所以回调函数或者说模块的前后顺序非常重要
void
ngx_http_core_run_phases(ngx_http_request_t *r)
{
    ngx_int_t                   rc;
    ngx_http_phase_handler_t   *ph;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    ph = cmcf->phase_engine.handlers;

    while (ph[r->phase_handler].checker) 
	{

        rc = ph[r->phase_handler].checker(r, &ph[r->phase_handler]);

        if (rc == NGX_OK)
		{
            return;
        }
    }
}

//NGX_HTTP_POST_READ_PHASE | NGX_HTTP_PREACCESS_PHASE | NGX_HTTP_LOG_PHASE阶段下HTTP模块的ngx_http_handler_pt方法返回值意义
//NGX_OK --	当前阶段已经被成功处理，必须进入到下一阶段
//NGX_DECLINED	--	当前回调不处理当前情况，进入到下一个回调处理
//NGX_AGAIN	-- 当前处理所需资源不足，需要等待所以来事件发生
//NGX_DONE	--	当前处理结束，仍需等待进一步事件发生后做处理
//NGX_ERROR, NGX_HTTP_...	当前回调发生错误，需要进入到异常处理流程
ngx_int_t
ngx_http_core_generic_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph)
{
    ngx_int_t  rc;

    /* generic phase checker, used by the post read and pre-access phases */
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "generic phase: %ui", r->phase_handler);

	//调用这一阶段中各HTTP模块添加的handler处理方法
    rc = ph->handler(r);

	//如果handler方法返回NGX_OK，之后将进入下一个阶段处理，而不会理会当前阶段中是否还有其他的处理方法
    if (rc == NGX_OK) 
	{
        r->phase_handler = ph->next;
        return NGX_AGAIN;
    }

	//如果handler方法返回NGX_DECLINED，那么将进入下一个处理方法，这个处理方法既可能属于当前阶段，也可能属于下一个阶段。
    if (rc == NGX_DECLINED) 
	{
        r->phase_handler++;
        return NGX_AGAIN;
    }

	//如果handler方法返回NGX_AGAIN或者NGX_DONE，那么当前请求将仍然停留在这一个处理阶段中
    if (rc == NGX_AGAIN || rc == NGX_DONE) 
	{
        return NGX_OK;
    }

    //rc == NGX_ERROR || rc == NGX_HTTP_... 
	//如果handler方法返回NGX_ERROR或者或者类似NGX_HTTP_开头的返回码，那么调用ngx_http_finalize_request结束请求
    ngx_http_finalize_request(r, rc);

    return NGX_OK;
}


//NGX_HTTP_SERVER_REWRITE_PHASE | NGX_HTTP_REWRITE_PHASE 阶段下HTTP模块的ngx_http_handler_pt方法返回值意义
//NGX_DONE	--	当前的ngx_http_handler_pt处理方法尚未结束，这意味着该处理方法在当前阶段中有机会再次被调用
//NGX_DECLINED	--	当前的ngx_http_handler_pt处理方法执行完毕，按照顺序执行下一个ngx_http_handler_pt处理方法
//NGX_OK, NGX_AGAIN, NGX_ERROR, NGX_HTTP_... --需要的调用ngx_http_finalize_request结束请求
//ngx_http_core_rewrite_phase永远不会导致跨过同一HTTP阶段的其他处理方法，就直接跳到下一个阶段来处理请求。
//因为，HTTP框架认为这两个阶段(NGX_HTTP_SERVER_REWRITE_PHASE | NGX_HTTP_REWRITE_PHASE)的HTTP模块是完全平等的，
//序号靠前的HTTP模块优先级并不会更高，它不能决定序号靠后的HTTP模块是否可以再次重写URL。
ngx_int_t
ngx_http_core_rewrite_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph)
{
    ngx_int_t  rc;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "rewrite phase: %ui", r->phase_handler);

    rc = ph->handler(r);

    if (rc == NGX_DECLINED) 
	{
        r->phase_handler++;
        return NGX_AGAIN;
    }

	//当前的handler方法无法在这一次调度中处理完这一个阶段，它需要多次的调度才能完成。
    if (rc == NGX_DONE) 
	{
        return NGX_OK;
    }

    /* NGX_OK, NGX_AGAIN, NGX_ERROR, NGX_HTTP_...  */
    ngx_http_finalize_request(r, rc);

    return NGX_OK;
}

//根据NGX_HTTP_SERVER_REWRITE_PHASE步骤重写后的URI检索出匹配的location块的，
//其原理为从location组成的静态二叉查找树中快速检索
ngx_int_t
ngx_http_core_find_config_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph)
{
    u_char                    *p;
    size_t                     len;
    ngx_int_t                  rc;
    ngx_http_core_loc_conf_t  *clcf;

    r->content_handler = NULL;
    r->uri_changed = 0;

    rc = ngx_http_core_find_location(r);

    if (rc == NGX_ERROR) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return NGX_OK;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (!r->internal && clcf->internal) {
        ngx_http_finalize_request(r, NGX_HTTP_NOT_FOUND);
        return NGX_OK;
    }

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "using configuration \"%s%V\"", 
			(clcf->noname ? "*" : (clcf->exact_match ? "=" : "")), &clcf->name);

    ngx_http_update_location_config(r);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http cl:%O max:%O",
                   r->headers_in.content_length_n, clcf->client_max_body_size);

    if (r->headers_in.content_length_n != -1 && !r->discard_body
        && clcf->client_max_body_size
        && clcf->client_max_body_size < r->headers_in.content_length_n)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "client intended to send too large body: %O bytes", r->headers_in.content_length_n);

        r->expect_tested = 1;
        (void) ngx_http_discard_request_body(r);
        ngx_http_finalize_request(r, NGX_HTTP_REQUEST_ENTITY_TOO_LARGE);
        return NGX_OK;
    }

    if (rc == NGX_DONE) {
        ngx_http_clear_location(r);

        r->headers_out.location = ngx_list_push(&r->headers_out.headers);
        if (r->headers_out.location == NULL) {
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return NGX_OK;
        }

        /*
         * we do not need to set the r->headers_out.location->hash and
         * r->headers_out.location->key fields
         */

        if (r->args.len == 0) {
            r->headers_out.location->value = clcf->name;

        } else {
            len = clcf->name.len + 1 + r->args.len;
            p = ngx_pnalloc(r->pool, len);

            if (p == NULL) {
                ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
                return NGX_OK;
            }

            r->headers_out.location->value.len = len;
            r->headers_out.location->value.data = p;

            p = ngx_cpymem(p, clcf->name.data, clcf->name.len);
            *p++ = '?';
            ngx_memcpy(p, r->args.data, r->args.len);
        }

        ngx_http_finalize_request(r, NGX_HTTP_MOVED_PERMANENTLY);
        return NGX_OK;
    }

    r->phase_handler++;
    return NGX_AGAIN;
}

//检查rewrite重写URL的次数不可以超过10次，以此防止由于rewrite死循环而造成整个Nginx服务都不可用
ngx_int_t
ngx_http_core_post_rewrite_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph)
{
    ngx_http_core_srv_conf_t  *cscf;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "post rewrite phase: %ui", r->phase_handler);

    if (!r->uri_changed)
	{
        r->phase_handler++;
        return NGX_AGAIN;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "uri changes: %d", r->uri_changes);

    /*
     * gcc before 3.3 compiles the broken code for
     *     if (r->uri_changes-- == 0)
     * if the r->uri_changes is defined as
     *     unsigned  uri_changes:4
     */

    r->uri_changes--;

    if (r->uri_changes == 0) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "rewrite or internal redirection cycle "
                      "while processing \"%V\"", &r->uri);

        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return NGX_OK;
    }

    r->phase_handler = ph->next;

    cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);
    r->loc_conf = cscf->ctx->loc_conf;

    return NGX_AGAIN;
}

//NGX_HTTP_ACCESS_PHASE 阶段下HTTP模块的ngx_http_handler_pt方法返回值意义
//NGX_OK -- 如果在nginx.conf中配置了satisfy all，那么将按照顺序执行下一个ngx_http_handler_pt处理方法;
//			如果在nginx.conf中配置了satisfy any，那么将执行下一个ngx_http_phases阶段中的第一个ngx_http_handler_pt处理方法;
//NGX_DONE，NGX_AGAIN	--	当前的ngx_http_handler_pt处理方法尚未结束，这意味着该处理方法在当前阶段中有机会再次被调用。
//			这时会把控制权交还给事件模块，下次可写事件发生时会再次执行到该ngx_http_handler_pt处理方法
//NGX_DECLINED	--	当前的ngx_http_handler_pt处理方法执行完毕，按照顺序执行下一个ngx_http_handler_pt处理方法
//NGX_HTTP_FORBIDDEN, NGX_HTTP_UNAUTHORIZED -- 
//NGX_ERROR, NGX_HTTP_... --需要的调用ngx_http_finalize_request结束请求
ngx_int_t
ngx_http_core_access_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph)
{
    ngx_int_t                  rc;
    ngx_http_core_loc_conf_t  *clcf;

	/*当前请求不是主请求，无需进行访问权限检测，直接进入下一个处理阶段*/
    if (r != r->main)		
	{
        r->phase_handler = ph->next;
        return NGX_AGAIN;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "access phase: %ui", r->phase_handler);

    rc = ph->handler(r);

    if (rc == NGX_DECLINED) 
	{
        r->phase_handler++;
        return NGX_AGAIN;
    }

    if (rc == NGX_AGAIN || rc == NGX_DONE) 
	{
        return NGX_OK;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

	//
	//NGX_HTTP_ACCESS_PHASE阶段可能有很多HTTP模块都对控制请求的访问权限感兴趣，那么以哪一个为准呢?
	//当satisfy的参数为all时，这些HTTP模块必须同时发生作用，即以该阶段中全部的handler方法共同决定请求的访问权限，
	//换句话说，这一阶段的所有handler方法必须全部返回NGX_OK才能认为请求具有访问权限
	//与all相反，参数为any时意味着在NGX_HTTP_ACCESS_PHASE阶段只要有任意一个HTTP模块认为请求合法，
	//就不用再调用其它HTTP模块继续检查了，可以认为请求是具有访问权限的。实际上，这时的情况有些复杂:如果其中任何一个handler
	//方法返回NGX_OK，则认为请求具有访问权限；如果某一个handler方法返回403或者401，则认为请求没有访问权限，还需要检查
	//NGX_HTTP_ACCESS_PHASE阶段的其他handler方法。也就是说，any配置项下任何一个handler方法一旦认为请求具有访问权限，就
	//认为这一阶段执行成功，继续向下执行；如果其中一个handler方法认为没有访问权限，则未必一次为准，还需要检测其他的handler方法
    if (clcf->satisfy == NGX_HTTP_SATISFY_ALL) 
	{

        if (rc == NGX_OK)
		{
            r->phase_handler++;
            return NGX_AGAIN;
        }

    } 
	else
	{
        if (rc == NGX_OK)
		{
            r->access_code = 0;

            if (r->headers_out.www_authenticate) {
                r->headers_out.www_authenticate->hash = 0;
            }

            r->phase_handler = ph->next;
            return NGX_AGAIN;
        }

		//如果返回值是NGX_HTTP_FORBIDDEN或者NGX_HTTP_UNAUTHORIZED，则表示这个HTTP模块的handler方法认为请求没有权限访问服务
        if (rc == NGX_HTTP_FORBIDDEN || rc == NGX_HTTP_UNAUTHORIZED)
		{
            if (r->access_code != NGX_HTTP_UNAUTHORIZED)
			{
				//将请求的access_code成员设置为handler方法的返回值，用于传递当前HTTP模块处理的结果
				//后续的handler方法可能会更改这一结果
                r->access_code = rc;
            }

            r->phase_handler++;
            return NGX_AGAIN;
        }
		//如果返回值为其它值，可以认为请求绝对无权访问服务
    }

    /* rc == NGX_ERROR || rc == NGX_HTTP_...  */

    ngx_http_finalize_request(r, rc);
    return NGX_OK;
}

//检查ngx_http_request_t请求中的access_code成员，当前不为0时就结束请求(表示没有访问权限)，
//否则继续执行下一个ngx_http_handler_pt处理方法
ngx_int_t
ngx_http_core_post_access_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph)
{
    ngx_int_t  access_code;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "post access phase: %ui", r->phase_handler);

    access_code = r->access_code;

    if (access_code) 
	{
        if (access_code == NGX_HTTP_FORBIDDEN) 
		{
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "access forbidden by rule");
        }

        r->access_code = 0;
        ngx_http_finalize_request(r, access_code);
        return NGX_OK;
    }

    r->phase_handler++;
    return NGX_AGAIN;
}

//与nginx.conf中try_files配置项密切相关，如果后指定的静态文件资源中有一个可以访问，
//这时就会直接读取文件并发送响应给用户，不会再向下执行后续阶段；如果所有静态文件资源
//都无法执行，将会继续执行ngx_http_phase_engine_t中的下一个ngx_http_phase_handler_t处理方法
ngx_int_t
ngx_http_core_try_files_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph)
{
    size_t                        len, root, alias, reserve, allocated;
    u_char                       *p, *name;
    ngx_str_t                     path, args;
    ngx_uint_t                    test_dir;
    ngx_http_try_file_t          *tf;
    ngx_open_file_info_t          of;
    ngx_http_script_code_pt       code;
    ngx_http_script_engine_t      e;
    ngx_http_core_loc_conf_t     *clcf;
    ngx_http_script_len_code_pt   lcode;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "try files phase: %ui", r->phase_handler);

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (clcf->try_files == NULL) {
        r->phase_handler++;
        return NGX_AGAIN;
    }

    allocated = 0;
    root = 0;
    name = NULL;
    /* suppress MSVC warning */
    path.data = NULL;

    tf = clcf->try_files;

    alias = clcf->alias;

    for ( ;; ) {

        if (tf->lengths) {
            ngx_memzero(&e, sizeof(ngx_http_script_engine_t));

            e.ip = tf->lengths->elts;
            e.request = r;

            /* 1 is for terminating '\0' as in static names */
            len = 1;

            while (*(uintptr_t *) e.ip) {
                lcode = *(ngx_http_script_len_code_pt *) e.ip;
                len += lcode(&e);
            }

        } else {
            len = tf->name.len;
        }

        if (!alias) {
            reserve = len > r->uri.len ? len - r->uri.len : 0;

        } else if (alias == NGX_MAX_SIZE_T_VALUE) {
            reserve = len;

        } else {
            reserve = len > r->uri.len - alias ? len - (r->uri.len - alias) : 0;
        }

        if (reserve > allocated || !allocated) {

            /* 16 bytes are preallocation */
            allocated = reserve + 16;

            if (ngx_http_map_uri_to_path(r, &path, &root, allocated) == NULL) {
                ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
                return NGX_OK;
            }

            name = path.data + root;
         }

        if (tf->values == NULL) {

            /* tf->name.len includes the terminating '\0' */

            ngx_memcpy(name, tf->name.data, tf->name.len);

            path.len = (name + tf->name.len - 1) - path.data;

        } else {
            e.ip = tf->values->elts;
            e.pos = name;
            e.flushed = 1;

            while (*(uintptr_t *) e.ip) {
                code = *(ngx_http_script_code_pt *) e.ip;
                code((ngx_http_script_engine_t *) &e);
            }

            path.len = e.pos - path.data;

            *e.pos = '\0';

            if (alias && alias != NGX_MAX_SIZE_T_VALUE
                && ngx_strncmp(name, r->uri.data, alias) == 0)
            {
                ngx_memmove(name, name + alias, len - alias);
                path.len -= alias;
            }
        }

        test_dir = tf->test_dir;

        tf++;

        ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "trying to use %s: \"%s\" \"%s\"",
                       test_dir ? "dir" : "file", name, path.data);

        if (tf->lengths == NULL && tf->name.len == 0) {

            if (tf->code) {
                ngx_http_finalize_request(r, tf->code);
                return NGX_OK;
            }

            path.len -= root;
            path.data += root;

            if (path.data[0] == '@') {
                (void) ngx_http_named_location(r, &path);

            } else {
                ngx_http_split_args(r, &path, &args);

                (void) ngx_http_internal_redirect(r, &path, &args);
            }

            ngx_http_finalize_request(r, NGX_DONE);
            return NGX_OK;
        }

        ngx_memzero(&of, sizeof(ngx_open_file_info_t));

        of.read_ahead = clcf->read_ahead;
        of.directio = clcf->directio;
        of.valid = clcf->open_file_cache_valid;
        of.min_uses = clcf->open_file_cache_min_uses;
        of.test_only = 1;
        of.errors = clcf->open_file_cache_errors;
        of.events = clcf->open_file_cache_events;

        if (ngx_http_set_disable_symlinks(r, clcf, &path, &of) != NGX_OK) {
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return NGX_OK;
        }

        if (ngx_open_cached_file(clcf->open_file_cache, &path, &of, r->pool)
            != NGX_OK)
        {
            if (of.err != NGX_ENOENT
                && of.err != NGX_ENOTDIR
                && of.err != NGX_ENAMETOOLONG)
            {
                ngx_log_error(NGX_LOG_CRIT, r->connection->log, of.err,
                              "%s \"%s\" failed", of.failed, path.data);
            }

            continue;
        }

        if (of.is_dir != test_dir) {
            continue;
        }

        path.len -= root;
        path.data += root;

        if (!alias) {
            r->uri = path;

        } else if (alias == NGX_MAX_SIZE_T_VALUE) {
            if (!test_dir) {
                r->uri = path;
                r->add_uri_to_alias = 1;
            }

        } else {
            name = r->uri.data;

            r->uri.len = alias + path.len;
            r->uri.data = ngx_pnalloc(r->pool, r->uri.len);
            if (r->uri.data == NULL) {
                ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
                return NGX_OK;
            }

            p = ngx_copy(r->uri.data, name, alias);
            ngx_memcpy(p, path.data, path.len);
        }

        ngx_http_set_exten(r);

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "try file uri: \"%V\"", &r->uri);

        r->phase_handler++;
        return NGX_AGAIN;
    }

    /* not reached */
}


ngx_int_t
ngx_http_core_content_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph)
{
    size_t     root;
    ngx_int_t  rc;
    ngx_str_t  path;

	//其余10个阶段中各HTTP模块的处理方法都是放在全局的ngx_http_core_main_conf_t结构体中的，也就是说，
	//它们对任何一个HTTP请求都是有效的。但在NGX_HTTP_CONTENT_PHASE阶段却很自然的有另一种需求，有的HTTP
	//模块可能仅希望在这个处理请求内容的阶段，仅仅针对某种请求唯一生效，而不是对所有请求生效。例如，
	//仅当请求的URI匹配了配置文件中的location块时，再根据location块下的配置选择一个HTTP模块执行它的
	//handler处理方法，并以此代替NGX_HTTP_CONTENT_PHASE阶段的其它handler方法(这些handler方法对于该请求将得不到执行)。

	//检测ngx_http_request_t结构体的content_handler成员是否为空，其实就是看在NGX_HTTP_FIND_CONFIG_PHASE阶段匹配了URI请求的location内，
	//是否有HTTP模块把处理方法设置到了ngx_http_core_loc_conf_t结构体的handler成员中。
    if (r->content_handler) 
	{
		//设置ngx_http_request_t结构体的write_event_handler成员为不做任何事情的ngx_http_request_empty_handler方法，也就是告诉HTTP框架
		//再有可写事件时就调用ngx_http_request_empty_handler直接把控制权交还给事件模块。为何要这样做呢?因为HTTP框架在这一阶段调用HTTP
		//模块处理请求就意味着接下来只希望该模块处理请求，先把write_event_handler强制转化为ngx_http_request_empty_handler，可以防止该
		//HTTP模块异步地处理请求时却有其它HTTP模块还在同时处理可写事件、向客户端发送响应、
        r->write_event_handler = ngx_http_request_empty_handler;
        ngx_http_finalize_request(r, r->content_handler(r));
        return NGX_OK;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "content phase: %ui", r->phase_handler);

    rc = ph->handler(r);

    if (rc != NGX_DECLINED) 
	{
        ngx_http_finalize_request(r, rc);
        return NGX_OK;
    }

    /* rc == NGX_DECLINED */

	//请求在第10个阶段NGX_HTTP_CONTENT_PHASE后，并没有去掉用第11个阶段NGX_HTTP_LOG_PHASE的处理方法，事实上，记录访问日志
	//是必须在请求将要结束时才能进行的，因此，NGX_HTTP_LOG_PHASE阶段的回调方法在ngx_http_free_request方法中才会调用到
	
	//通过转到数组中的下一个handler方法，检测其checker方法是否存在，来判断当前的handler方法是否已经是最后一个handler方法
    ph++;
    if (ph->checker)
	{
        r->phase_handler++;
        return NGX_AGAIN;
    }

    /* no content handler was found */

    if (r->uri.data[r->uri.len - 1] == '/')
	{

        if (ngx_http_map_uri_to_path(r, &path, &root, 0) != NULL)
		{
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "directory index of \"%s\" is forbidden", path.data);
        }

        ngx_http_finalize_request(r, NGX_HTTP_FORBIDDEN);
        return NGX_OK;
    }

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "no handler found");

    ngx_http_finalize_request(r, NGX_HTTP_NOT_FOUND);
    return NGX_OK;
}


void
ngx_http_update_location_config(ngx_http_request_t *r)
{
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (r->method & clcf->limit_except) 
	{
        r->loc_conf = clcf->limit_except_loc_conf;
        clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
    }

    if (r == r->main) {
        ngx_set_connection_log(r->connection, clcf->error_log);
    }

    if ((ngx_io.flags & NGX_IO_SENDFILE) && clcf->sendfile) {
        r->connection->sendfile = 1;

    } else {
        r->connection->sendfile = 0;
    }

    if (clcf->client_body_in_file_only) {
        r->request_body_in_file_only = 1;
        r->request_body_in_persistent_file = 1;
        r->request_body_in_clean_file =
            clcf->client_body_in_file_only == NGX_HTTP_REQUEST_BODY_FILE_CLEAN;
        r->request_body_file_log_level = NGX_LOG_NOTICE;

    } else {
        r->request_body_file_log_level = NGX_LOG_WARN;
    }

    r->request_body_in_single_buf = clcf->client_body_in_single_buffer;

    if (r->keepalive) {
        if (clcf->keepalive_timeout == 0) {
            r->keepalive = 0;

        } else if (r->connection->requests >= clcf->keepalive_requests) {
            r->keepalive = 0;

        } else if (r->headers_in.msie6
                   && r->method == NGX_HTTP_POST
                   && (clcf->keepalive_disable
                       & NGX_HTTP_KEEPALIVE_DISABLE_MSIE6))
        {
            /*
             * MSIE may wait for some time if an response for
             * a POST request was sent over a keepalive connection
             */
            r->keepalive = 0;

        } else if (r->headers_in.safari
                   && (clcf->keepalive_disable
                       & NGX_HTTP_KEEPALIVE_DISABLE_SAFARI))
        {
            /*
             * Safari may send a POST request to a closed keepalive
             * connection and may stall for some time, see
             *     https://bugs.webkit.org/show_bug.cgi?id=5760
             */
            r->keepalive = 0;
        }
    }

    if (!clcf->tcp_nopush) {
        /* disable TCP_NOPUSH/TCP_CORK use */
        r->connection->tcp_nopush = NGX_TCP_NOPUSH_DISABLED;
    }

    if (r->limit_rate == 0) {
        r->limit_rate = clcf->limit_rate;
    }

    if (clcf->handler)
	{
        r->content_handler = clcf->handler;
    }
}


/*
 * NGX_OK       - exact or regex match
 * NGX_DONE     - auto redirect
 * NGX_AGAIN    - inclusive match
 * NGX_ERROR    - regex error
 * NGX_DECLINED - no match
 */

static ngx_int_t
ngx_http_core_find_location(ngx_http_request_t *r)
{
    ngx_int_t                  rc;
    ngx_http_core_loc_conf_t  *pclcf;
#if (NGX_PCRE)
    ngx_int_t                  n;
    ngx_uint_t                 noregex;
    ngx_http_core_loc_conf_t  *clcf, **clcfp;

    noregex = 0;
#endif

    pclcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    rc = ngx_http_core_find_static_location(r, pclcf->static_locations);

    if (rc == NGX_AGAIN) {

#if (NGX_PCRE)
        clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

        noregex = clcf->noregex;
#endif

        /* look up nested locations */

        rc = ngx_http_core_find_location(r);
    }

    if (rc == NGX_OK || rc == NGX_DONE) {
        return rc;
    }

    /* rc == NGX_DECLINED or rc == NGX_AGAIN in nested location */

#if (NGX_PCRE)

    if (noregex == 0 && pclcf->regex_locations) {

        for (clcfp = pclcf->regex_locations; *clcfp; clcfp++) {

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "test location: ~ \"%V\"", &(*clcfp)->name);

            n = ngx_http_regex_exec(r, (*clcfp)->regex, &r->uri);

            if (n == NGX_OK) {
                r->loc_conf = (*clcfp)->loc_conf;

                /* look up nested locations */

                rc = ngx_http_core_find_location(r);

                return (rc == NGX_ERROR) ? rc : NGX_OK;
            }

            if (n == NGX_DECLINED) {
                continue;
            }

            return NGX_ERROR;
        }
    }
#endif

    return rc;
}


/*
 * NGX_OK       - exact match
 * NGX_DONE     - auto redirect
 * NGX_AGAIN    - inclusive match
 * NGX_DECLINED - no match
 */

static ngx_int_t
ngx_http_core_find_static_location(ngx_http_request_t *r,
    ngx_http_location_tree_node_t *node)
{
    u_char     *uri;
    size_t      len, n;
    ngx_int_t   rc, rv;

    len = r->uri.len;
    uri = r->uri.data;

    rv = NGX_DECLINED;

    for ( ;; ) {

        if (node == NULL) {
            return rv;
        }

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "test location: \"%*s\"", node->len, node->name);

        n = (len <= (size_t) node->len) ? len : node->len;

        rc = ngx_filename_cmp(uri, node->name, n);

        if (rc != 0) {
            node = (rc < 0) ? node->left : node->right;

            continue;
        }

        if (len > (size_t) node->len) {

            if (node->inclusive) {

                r->loc_conf = node->inclusive->loc_conf;
                rv = NGX_AGAIN;

                node = node->tree;
                uri += n;
                len -= n;

                continue;
            }

            /* exact only */

            node = node->right;

            continue;
        }

        if (len == (size_t) node->len) {

            if (node->exact) {
                r->loc_conf = node->exact->loc_conf;
                return NGX_OK;

            } else {
                r->loc_conf = node->inclusive->loc_conf;
                return NGX_AGAIN;
            }
        }

        /* len < node->len */

        if (len + 1 == (size_t) node->len && node->auto_redirect) {

            r->loc_conf = (node->exact) ? node->exact->loc_conf:
                                          node->inclusive->loc_conf;
            rv = NGX_DONE;
        }

        node = node->left;
    }
}


void *
ngx_http_test_content_type(ngx_http_request_t *r, ngx_hash_t *types_hash)
{
    u_char      c, *lowcase;
    size_t      len;
    ngx_uint_t  i, hash;

    if (types_hash->size == 0) {
        return (void *) 4;
    }

    if (r->headers_out.content_type.len == 0) {
        return NULL;
    }

    len = r->headers_out.content_type_len;

    if (r->headers_out.content_type_lowcase == NULL) {

        lowcase = ngx_pnalloc(r->pool, len);
        if (lowcase == NULL) {
            return NULL;
        }

        r->headers_out.content_type_lowcase = lowcase;

        hash = 0;

        for (i = 0; i < len; i++) {
            c = ngx_tolower(r->headers_out.content_type.data[i]);
            hash = ngx_hash(hash, c);
            lowcase[i] = c;
        }

        r->headers_out.content_type_hash = hash;
    }

    return ngx_hash_find(types_hash, r->headers_out.content_type_hash,
                         r->headers_out.content_type_lowcase, len);
}


ngx_int_t
ngx_http_set_content_type(ngx_http_request_t *r)
{
    u_char                     c, *exten;
    ngx_str_t                 *type;
    ngx_uint_t                 i, hash;
    ngx_http_core_loc_conf_t  *clcf;

    if (r->headers_out.content_type.len) 
	{
        return NGX_OK;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (r->exten.len) 
	{

        hash = 0;

        for (i = 0; i < r->exten.len; i++) {
            c = r->exten.data[i];

            if (c >= 'A' && c <= 'Z') {

                exten = ngx_pnalloc(r->pool, r->exten.len);
                if (exten == NULL) {
                    return NGX_ERROR;
                }

                hash = ngx_hash_strlow(exten, r->exten.data, r->exten.len);

                r->exten.data = exten;

                break;
            }

            hash = ngx_hash(hash, c);
        }

        type = ngx_hash_find(&clcf->types_hash, hash,
                             r->exten.data, r->exten.len);

        if (type) {
            r->headers_out.content_type_len = type->len;
            r->headers_out.content_type = *type;

            return NGX_OK;
        }
    }

    r->headers_out.content_type_len = clcf->default_type.len;
    r->headers_out.content_type = clcf->default_type;

    return NGX_OK;
}


void
ngx_http_set_exten(ngx_http_request_t *r)
{
    ngx_int_t  i;

    ngx_str_null(&r->exten);

    for (i = r->uri.len - 1; i > 1; i--) {
        if (r->uri.data[i] == '.' && r->uri.data[i - 1] != '/') {

            r->exten.len = r->uri.len - i - 1;
            r->exten.data = &r->uri.data[i + 1];

            return;

        } else if (r->uri.data[i] == '/') {
            return;
        }
    }

    return;
}


ngx_int_t
ngx_http_set_etag(ngx_http_request_t *r)
{
    ngx_table_elt_t           *etag;
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (!clcf->etag) {
        return NGX_OK;
    }

    etag = ngx_list_push(&r->headers_out.headers);
    if (etag == NULL) {
        return NGX_ERROR;
    }

    etag->hash = 1;
    ngx_str_set(&etag->key, "ETag");

    etag->value.data = ngx_pnalloc(r->pool, NGX_OFF_T_LEN + NGX_TIME_T_LEN + 3);
    if (etag->value.data == NULL) {
        etag->hash = 0;
        return NGX_ERROR;
    }

    etag->value.len = ngx_sprintf(etag->value.data, "\"%xT-%xO\"",
                                  r->headers_out.last_modified_time,
                                  r->headers_out.content_length_n)
                      - etag->value.data;

    r->headers_out.etag = etag;

    return NGX_OK;
}


void
ngx_http_weak_etag(ngx_http_request_t *r)
{
    size_t            len;
    u_char           *p;
    ngx_table_elt_t  *etag;

    etag = r->headers_out.etag;

    if (etag == NULL) {
        return;
    }

    if (etag->value.len > 2
        && etag->value.data[0] == 'W'
        && etag->value.data[1] == '/')
    {
        return;
    }

    if (etag->value.len < 1 || etag->value.data[0] != '"') {
        r->headers_out.etag->hash = 0;
        r->headers_out.etag = NULL;
        return;
    }

    p = ngx_pnalloc(r->pool, etag->value.len + 2);
    if (p == NULL) {
        r->headers_out.etag->hash = 0;
        r->headers_out.etag = NULL;
        return;
    }

    len = ngx_sprintf(p, "W/%V", &etag->value) - p;

    etag->value.data = p;
    etag->value.len = len;
}


ngx_int_t
ngx_http_send_response(ngx_http_request_t *r, ngx_uint_t status, ngx_str_t *ct, ngx_http_complex_value_t *cv)
{
    ngx_int_t     rc;
    ngx_str_t     val;
    ngx_buf_t    *b;
    ngx_chain_t   out;

    if (ngx_http_discard_request_body(r) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    r->headers_out.status = status;

    if (ngx_http_complex_value(r, cv, &val) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (status == NGX_HTTP_MOVED_PERMANENTLY
        || status == NGX_HTTP_MOVED_TEMPORARILY
        || status == NGX_HTTP_SEE_OTHER
        || status == NGX_HTTP_TEMPORARY_REDIRECT)
    {
        ngx_http_clear_location(r);

        r->headers_out.location = ngx_list_push(&r->headers_out.headers);
        if (r->headers_out.location == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        r->headers_out.location->hash = 1;
        ngx_str_set(&r->headers_out.location->key, "Location");
        r->headers_out.location->value = val;

        return status;
    }

    r->headers_out.content_length_n = val.len;

    if (ct) {
        r->headers_out.content_type_len = ct->len;
        r->headers_out.content_type = *ct;

    } else {
        if (ngx_http_set_content_type(r) != NGX_OK) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    if (r->method == NGX_HTTP_HEAD || (r != r->main && val.len == 0))
	{
        return ngx_http_send_header(r);
    }

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->pos = val.data;
    b->last = val.data + val.len;
    b->memory = val.len ? 1 : 0;
    b->last_buf = (r == r->main) ? 1 : 0;
    b->last_in_chain = 1;

    out.buf = b;
    out.next = NULL;

    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    return ngx_http_output_filter(r, &out);
}

//负责构造HTTP响应行，头部，同时会把它们发送给客户端
//发送响应头部使用了流水线式的过滤模块思想，即通过提供统一的接口，让各个感兴趣的HTTP模块加入到ngx_http_send_header
//方法中，然后通过每个过滤模块C源文件中独有的ngx_http_next_header_filter指针将各个过滤头部的方法连接起来，这样在调用
//ngx_http_send_header方法时，实际就是依次调用了所有头部过滤模块的方法，其中，链表里的最后一个头部过滤方法ngx_http_header_filter
//将负责发送头部。因此，这些过滤模块组成的链表顺序是非常重要的。

//如果不需要发送包体，那么这时需要调用ngx_http_finalize_request方法来结束请求，其中第二个参数务必要传递NGX_AGAIN,这样HTTP框架
//才会继续将可写事件注册到epoll，并持续地把请求的out成员中缓冲区里的HTTP响应发送完毕才会结束请求。
ngx_int_t
ngx_http_send_header(ngx_http_request_t *r)
{
    if (r->post_action)
	{
        return NGX_OK;
    }

    if (r->header_sent) 
	{
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "header already sent");
        return NGX_ERROR;
    }

    if (r->err_status) 
	{
        r->headers_out.status = r->err_status;
        r->headers_out.status_line.len = 0;
    }

    return ngx_http_top_header_filter(r);
}

//发送HTTP响应包体
//使用ngx_http_output_filter发送响应时，必须与结束请求的ngx_http_finalize_request方法配合使用
//(ngx_http_finalize_request方法会把请求的write_event_handler设置为ngx_http_writer方法，并将写事件
//添加到epoll和定时器中)，这样就使得正真负责在后台异步的发送响应的ngx_http_writer方法对HTTP模块而言也是透明的

//用于过滤包体的HTTP模块将以ngx_http_output_filter方法在发送包体时会一次调用各个过滤包体方法，其中最后一个过滤
//包体的方法就是ngx_http_write_filter方法，它属于ngx_http_write_filter_module模块

//参数:
//	in -- 用于存放响应包体的缓冲区
ngx_int_t
ngx_http_output_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_int_t          rc;
    ngx_connection_t  *c;

    c = r->connection;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http output filter \"%V?%V\"", &r->uri, &r->args);

    rc = ngx_http_top_body_filter(r, in);

    if (rc == NGX_ERROR) 
	{
        /* NGX_ERROR may be returned by any filter */
        c->error = 1;
    }

    return rc;
}

//把请求的http协议的路径转化成一个文件系统的路径
u_char *
ngx_http_map_uri_to_path(ngx_http_request_t *r, ngx_str_t *path, size_t *root_length, size_t reserved)
{
    u_char                    *last;
    size_t                     alias;
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    alias = clcf->alias;

    if (alias && !r->valid_location) 
	{
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "\"alias\" cannot be used in location \"%V\" "
                      "where URI was rewritten", &clcf->name);
        return NULL;
    }

    if (clcf->root_lengths == NULL) 
	{

        *root_length = clcf->root.len;

        path->len = clcf->root.len + reserved + r->uri.len - alias + 1;

        path->data = ngx_pnalloc(r->pool, path->len);
        if (path->data == NULL) 
		{
            return NULL;
        }

        last = ngx_copy(path->data, clcf->root.data, clcf->root.len);

    } else {

        if (alias == NGX_MAX_SIZE_T_VALUE) {
            reserved += r->add_uri_to_alias ? r->uri.len + 1 : 1;

        } else {
            reserved += r->uri.len - alias + 1;
        }

        if (ngx_http_script_run(r, path, clcf->root_lengths->elts, reserved,
                                clcf->root_values->elts)
            == NULL)
        {
            return NULL;
        }

        if (ngx_get_full_name(r->pool, (ngx_str_t *) &ngx_cycle->prefix, path)
            != NGX_OK)
        {
            return NULL;
        }

        *root_length = path->len - reserved;
        last = path->data + *root_length;

        if (alias == NGX_MAX_SIZE_T_VALUE) {
            if (!r->add_uri_to_alias) {
                *last = '\0';
                return last;
            }

            alias = 0;
        }
    }

    last = ngx_cpystrn(last, r->uri.data + alias, r->uri.len - alias + 1);

    return last;
}


ngx_int_t
ngx_http_auth_basic_user(ngx_http_request_t *r)
{
    ngx_str_t   auth, encoded;
    ngx_uint_t  len;

    if (r->headers_in.user.len == 0 && r->headers_in.user.data != NULL) {
        return NGX_DECLINED;
    }

    if (r->headers_in.authorization == NULL) {
        r->headers_in.user.data = (u_char *) "";
        return NGX_DECLINED;
    }

    encoded = r->headers_in.authorization->value;

    if (encoded.len < sizeof("Basic ") - 1
        || ngx_strncasecmp(encoded.data, (u_char *) "Basic ",
                           sizeof("Basic ") - 1)
           != 0)
    {
        r->headers_in.user.data = (u_char *) "";
        return NGX_DECLINED;
    }

    encoded.len -= sizeof("Basic ") - 1;
    encoded.data += sizeof("Basic ") - 1;

    while (encoded.len && encoded.data[0] == ' ') {
        encoded.len--;
        encoded.data++;
    }

    if (encoded.len == 0) {
        r->headers_in.user.data = (u_char *) "";
        return NGX_DECLINED;
    }

    auth.len = ngx_base64_decoded_length(encoded.len);
    auth.data = ngx_pnalloc(r->pool, auth.len + 1);
    if (auth.data == NULL) {
        return NGX_ERROR;
    }

    if (ngx_decode_base64(&auth, &encoded) != NGX_OK) {
        r->headers_in.user.data = (u_char *) "";
        return NGX_DECLINED;
    }

    auth.data[auth.len] = '\0';

    for (len = 0; len < auth.len; len++) {
        if (auth.data[len] == ':') {
            break;
        }
    }

    if (len == 0 || len == auth.len) {
        r->headers_in.user.data = (u_char *) "";
        return NGX_DECLINED;
    }

    r->headers_in.user.len = len;
    r->headers_in.user.data = auth.data;
    r->headers_in.passwd.len = auth.len - len - 1;
    r->headers_in.passwd.data = &auth.data[len + 1];

    return NGX_OK;
}


#if (NGX_HTTP_GZIP)

ngx_int_t
ngx_http_gzip_ok(ngx_http_request_t *r)
{
    time_t                     date, expires;
    ngx_uint_t                 p;
    ngx_array_t               *cc;
    ngx_table_elt_t           *e, *d, *ae;
    ngx_http_core_loc_conf_t  *clcf;

    r->gzip_tested = 1;

    if (r != r->main) {
        return NGX_DECLINED;
    }

    ae = r->headers_in.accept_encoding;
    if (ae == NULL) {
        return NGX_DECLINED;
    }

    if (ae->value.len < sizeof("gzip") - 1) {
        return NGX_DECLINED;
    }

    /*
     * test first for the most common case "gzip,...":
     *   MSIE:    "gzip, deflate"
     *   Firefox: "gzip,deflate"
     *   Chrome:  "gzip,deflate,sdch"
     *   Safari:  "gzip, deflate"
     *   Opera:   "gzip, deflate"
     */

    if (ngx_memcmp(ae->value.data, "gzip,", 5) != 0
        && ngx_http_gzip_accept_encoding(&ae->value) != NGX_OK)
    {
        return NGX_DECLINED;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (r->headers_in.msie6 && clcf->gzip_disable_msie6) {
        return NGX_DECLINED;
    }

    if (r->http_version < clcf->gzip_http_version) {
        return NGX_DECLINED;
    }

    if (r->headers_in.via == NULL) {
        goto ok;
    }

    p = clcf->gzip_proxied;

    if (p & NGX_HTTP_GZIP_PROXIED_OFF) {
        return NGX_DECLINED;
    }

    if (p & NGX_HTTP_GZIP_PROXIED_ANY) {
        goto ok;
    }

    if (r->headers_in.authorization && (p & NGX_HTTP_GZIP_PROXIED_AUTH)) {
        goto ok;
    }

    e = r->headers_out.expires;

    if (e) {

        if (!(p & NGX_HTTP_GZIP_PROXIED_EXPIRED)) {
            return NGX_DECLINED;
        }

        expires = ngx_parse_http_time(e->value.data, e->value.len);
        if (expires == NGX_ERROR) {
            return NGX_DECLINED;
        }

        d = r->headers_out.date;

        if (d) {
            date = ngx_parse_http_time(d->value.data, d->value.len);
            if (date == NGX_ERROR) {
                return NGX_DECLINED;
            }

        } else {
            date = ngx_time();
        }

        if (expires < date) {
            goto ok;
        }

        return NGX_DECLINED;
    }

    cc = &r->headers_out.cache_control;

    if (cc->elts) {

        if ((p & NGX_HTTP_GZIP_PROXIED_NO_CACHE)
            && ngx_http_parse_multi_header_lines(cc, &ngx_http_gzip_no_cache,
                                                 NULL)
               >= 0)
        {
            goto ok;
        }

        if ((p & NGX_HTTP_GZIP_PROXIED_NO_STORE)
            && ngx_http_parse_multi_header_lines(cc, &ngx_http_gzip_no_store,
                                                 NULL)
               >= 0)
        {
            goto ok;
        }

        if ((p & NGX_HTTP_GZIP_PROXIED_PRIVATE)
            && ngx_http_parse_multi_header_lines(cc, &ngx_http_gzip_private,
                                                 NULL)
               >= 0)
        {
            goto ok;
        }

        return NGX_DECLINED;
    }

    if ((p & NGX_HTTP_GZIP_PROXIED_NO_LM) && r->headers_out.last_modified) {
        return NGX_DECLINED;
    }

    if ((p & NGX_HTTP_GZIP_PROXIED_NO_ETAG) && r->headers_out.etag) {
        return NGX_DECLINED;
    }

ok:

#if (NGX_PCRE)

    if (clcf->gzip_disable && r->headers_in.user_agent) {

        if (ngx_regex_exec_array(clcf->gzip_disable,
                                 &r->headers_in.user_agent->value,
                                 r->connection->log)
            != NGX_DECLINED)
        {
            return NGX_DECLINED;
        }
    }

#endif

    r->gzip_ok = 1;

    return NGX_OK;
}


/*
 * gzip is enabled for the following quantities:
 *     "gzip; q=0.001" ... "gzip; q=1.000"
 * gzip is disabled for the following quantities:
 *     "gzip; q=0" ... "gzip; q=0.000", and for any invalid cases
 */

static ngx_int_t
ngx_http_gzip_accept_encoding(ngx_str_t *ae)
{
    u_char  *p, *start, *last;

    start = ae->data;
    last = start + ae->len;

    for ( ;; ) {
        p = ngx_strcasestrn(start, "gzip", 4 - 1);
        if (p == NULL) {
            return NGX_DECLINED;
        }

        if (p == start || (*(p - 1) == ',' || *(p - 1) == ' ')) {
            break;
        }

        start = p + 4;
    }

    p += 4;

    while (p < last) {
        switch (*p++) {
        case ',':
            return NGX_OK;
        case ';':
            goto quantity;
        case ' ':
            continue;
        default:
            return NGX_DECLINED;
        }
    }

    return NGX_OK;

quantity:

    while (p < last) {
        switch (*p++) {
        case 'q':
        case 'Q':
            goto equal;
        case ' ':
            continue;
        default:
            return NGX_DECLINED;
        }
    }

    return NGX_OK;

equal:

    if (p + 2 > last || *p++ != '=') {
        return NGX_DECLINED;
    }

    if (ngx_http_gzip_quantity(p, last) == 0) {
        return NGX_DECLINED;
    }

    return NGX_OK;
}


static ngx_uint_t
ngx_http_gzip_quantity(u_char *p, u_char *last)
{
    u_char      c;
    ngx_uint_t  n, q;

    c = *p++;

    if (c != '0' && c != '1') {
        return 0;
    }

    q = (c - '0') * 100;

    if (p == last) {
        return q;
    }

    c = *p++;

    if (c == ',' || c == ' ') {
        return q;
    }

    if (c != '.') {
        return 0;
    }

    n = 0;

    while (p < last) {
        c = *p++;

        if (c == ',' || c == ' ') {
            break;
        }

        if (c >= '0' && c <= '9') {
            q += c - '0';
            n++;
            continue;
        }

        return 0;
    }

    if (q > 100 || n > 3) {
        return 0;
    }

    return q;
}

#endif

//创建子请求
//参数:
//	r -- 当前创建子请求的请求对象，也就是父请求
//	uri -- 子请求的uri地址，它对究竟选用nginx.conf配置文件中的那个模块来处理子请求起决定性作用
//	args -- 子请求的URI参数，如果没有参数，可以传送NULL空指针
//	psr -- 一个传出参数，用于获取新创建的子请求。一般，我们先建立一个子请求的空指针ngx_http_request_t *psr，
//			再把它的地址&psr传入到ngx_http_subrequest方法中，如果返回成功，psr就指向建立好的子请求
//	ps -- 指定子请求结束时必须回调的处理方法
//	flags -- 标志值, NGX_HTTP_SUBREQUEST_WAITED | NGX_HTTP_SUBREQUEST_IN_MEMORY
//返回值:
//	NGX_OK -- 表示成功建立子请求 	NGX_ERROR -- 表示建立子请求失败
ngx_int_t
ngx_http_subrequest(ngx_http_request_t *r, ngx_str_t *uri, ngx_str_t *args, ngx_http_request_t **psr,
    ngx_http_post_subrequest_t *ps, ngx_uint_t flags)
{
    ngx_time_t                    *tp;
    ngx_connection_t              *c;
    ngx_http_request_t            *sr;
    ngx_http_core_srv_conf_t      *cscf;
    ngx_http_postponed_request_t  *pr, *p;

    if (r->subrequests == 0) 
	{
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "subrequests cycle while processing \"%V\"", uri);
        return NGX_ERROR;
    }

    /*
     * 1000 is reserved for other purposes.
     */
    if (r->main->count >= 65535 - 1000) 
	{
        ngx_log_error(NGX_LOG_CRIT, r->connection->log, 0, "request reference counter overflow while processing \"%V\"", uri);
        return NGX_ERROR;
    }

    sr = ngx_pcalloc(r->pool, sizeof(ngx_http_request_t));
    if (sr == NULL) 
	{
        return NGX_ERROR;
    }

    sr->signature = NGX_HTTP_MODULE;

    c = r->connection;
    sr->connection = c;

    sr->ctx = ngx_pcalloc(r->pool, sizeof(void *) * ngx_http_max_module);
    if (sr->ctx == NULL)
	{
        return NGX_ERROR;
    }

    if (ngx_list_init(&sr->headers_out.headers, r->pool, 20, sizeof(ngx_table_elt_t)) != NGX_OK)
    {
        return NGX_ERROR;
    }

    cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);
    sr->main_conf = cscf->ctx->main_conf;
    sr->srv_conf = cscf->ctx->srv_conf;
    sr->loc_conf = cscf->ctx->loc_conf;

    sr->pool = r->pool;

    sr->headers_in = r->headers_in;

    ngx_http_clear_content_length(sr);
    ngx_http_clear_accept_ranges(sr);
    ngx_http_clear_last_modified(sr);

    sr->request_body = r->request_body;

#if (NGX_HTTP_V2)
    sr->stream = r->stream;
#endif

    sr->method = NGX_HTTP_GET;
    sr->http_version = r->http_version;

    sr->request_line = r->request_line;
    sr->uri = *uri;

    if (args)
	{
        sr->args = *args;
    }

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http subrequest \"%V?%V\"", uri, &sr->args);

    sr->subrequest_in_memory = (flags & NGX_HTTP_SUBREQUEST_IN_MEMORY) != 0;
    sr->waited = (flags & NGX_HTTP_SUBREQUEST_WAITED) != 0;

    sr->unparsed_uri = r->unparsed_uri;
    sr->method_name = ngx_http_core_get_method;
    sr->http_protocol = r->http_protocol;

    ngx_http_set_exten(sr);
	//主请求保存在main字段中
    sr->main = r->main;
	//父请求为当前请求
    sr->parent = r;
	//保存回调handler及数据，在子请求执行完，将会调用
    sr->post_subrequest = ps;
	//读事件handler赋值为不做任何事的函数，因为子请求不用再读数据或者检查连接状态；
    //写事件handler为ngx_http_handler，它会重走phase
    sr->read_event_handler = ngx_http_request_empty_handler;
    sr->write_event_handler = ngx_http_handler;
	//ngx_connection_s的data字段比较关键，它保存了当前可以向out chain输出数据的请求
	//如果当前排在最前面的请求是父请求并且子请求是该父请求的第一个，那么就做切换
    if (c->data == r && r->postponed == NULL) 
	{
        c->data = sr;
    }

    sr->variables = r->variables;

    sr->log_handler = r->log_handler;

    pr = ngx_palloc(r->pool, sizeof(ngx_http_postponed_request_t));
    if (pr == NULL)
	{
        return NGX_ERROR;
    }

    pr->request = sr;
    pr->out = NULL;
    pr->next = NULL;

	//把该子请求挂载在其父请求的postponed链表的队尾
    if (r->postponed) 
	{
        for (p = r->postponed; p->next; p = p->next) { /* void */ }
        p->next = pr;
    }
	else 
	{
        r->postponed = pr;
    }

	//子请求为内部请求，它可以访问internal类型的location
    sr->internal = 1;

	//继承父请求的一些状态
    sr->discard_body = r->discard_body;
    sr->expect_tested = 1;
    sr->main_filter_need_in_memory = r->main_filter_need_in_memory;

    sr->uri_changes = NGX_HTTP_MAX_URI_CHANGES + 1;
    sr->subrequests = r->subrequests - 1;

    tp = ngx_timeofday();
    sr->start_sec = tp->sec;
    sr->start_msec = tp->msec;

	//增加主请求的引用数，这个字段主要是在ngx_http_finalize_request调用的一些结束请求和连接的函数中使用
    r->main->count++;

    *psr = sr;

	//将该子请求挂载在主请求的posted_requests链表队尾
    return ngx_http_post_request(sr, NULL);
}


ngx_int_t
ngx_http_internal_redirect(ngx_http_request_t *r,
    ngx_str_t *uri, ngx_str_t *args)
{
    ngx_http_core_srv_conf_t  *cscf;

    r->uri_changes--;

    if (r->uri_changes == 0) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "rewrite or internal redirection cycle "
                      "while internally redirecting to \"%V\"", uri);

        r->main->count++;
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return NGX_DONE;
    }

    r->uri = *uri;

    if (args) {
        r->args = *args;

    } else {
        ngx_str_null(&r->args);
    }

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "internal redirect: \"%V?%V\"", uri, &r->args);

    ngx_http_set_exten(r);

    /* clear the modules contexts */
    ngx_memzero(r->ctx, sizeof(void *) * ngx_http_max_module);

    cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);
    r->loc_conf = cscf->ctx->loc_conf;

    ngx_http_update_location_config(r);

#if (NGX_HTTP_CACHE)
    r->cache = NULL;
#endif

    r->internal = 1;
    r->valid_unparsed_uri = 0;
    r->add_uri_to_alias = 0;
    r->main->count++;

    ngx_http_handler(r);

    return NGX_DONE;
}


ngx_int_t
ngx_http_named_location(ngx_http_request_t *r, ngx_str_t *name)
{
    ngx_http_core_srv_conf_t    *cscf;
    ngx_http_core_loc_conf_t   **clcfp;
    ngx_http_core_main_conf_t   *cmcf;

    r->main->count++;
    r->uri_changes--;

    if (r->uri_changes == 0) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "rewrite or internal redirection cycle "
                      "while redirect to named location \"%V\"", name);

        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return NGX_DONE;
    }

    if (r->uri.len == 0) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "empty URI in redirect to named location \"%V\"", name);

        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return NGX_DONE;
    }

    cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);

    if (cscf->named_locations) {

        for (clcfp = cscf->named_locations; *clcfp; clcfp++) {

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "test location: \"%V\"", &(*clcfp)->name);

            if (name->len != (*clcfp)->name.len
                || ngx_strncmp(name->data, (*clcfp)->name.data, name->len) != 0)
            {
                continue;
            }

            ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "using location: %V \"%V?%V\"",
                           name, &r->uri, &r->args);

            r->internal = 1;
            r->content_handler = NULL;
            r->uri_changed = 0;
            r->loc_conf = (*clcfp)->loc_conf;

            /* clear the modules contexts */
            ngx_memzero(r->ctx, sizeof(void *) * ngx_http_max_module);

            ngx_http_update_location_config(r);

            cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

            r->phase_handler = cmcf->phase_engine.location_rewrite_index;

            r->write_event_handler = ngx_http_core_run_phases;
            ngx_http_core_run_phases(r);

            return NGX_DONE;
        }
    }

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "could not find named location \"%V\"", name);

    ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);

    return NGX_DONE;
}


//用于向请求ngx_http_request_t中添加ngx_http_cleanup_t结构体
//这个方法返回的就是已经插入请求的ngx_http_cleanup_t结构体指针，其中data成员指向的内存都已经分配好了，
//内存的大小由size参数指定
ngx_http_cleanup_t *
ngx_http_cleanup_add(ngx_http_request_t *r, size_t size)
{
    ngx_http_cleanup_t  *cln;

    r = r->main;

    cln = ngx_palloc(r->pool, sizeof(ngx_http_cleanup_t));
    if (cln == NULL)
	{
        return NULL;
    }

    if (size)
	{
        cln->data = ngx_palloc(r->pool, size);
        if (cln->data == NULL)
		{
            return NULL;
        }

    } 
	else
	{
        cln->data = NULL;
    }

    cln->handler = NULL;
    cln->next = r->cleanup;

    r->cleanup = cln;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http cleanup add: %p", cln);

    return cln;
}


ngx_int_t
ngx_http_set_disable_symlinks(ngx_http_request_t *r,
    ngx_http_core_loc_conf_t *clcf, ngx_str_t *path, ngx_open_file_info_t *of)
{
#if (NGX_HAVE_OPENAT)
    u_char     *p;
    ngx_str_t   from;

    of->disable_symlinks = clcf->disable_symlinks;

    if (clcf->disable_symlinks_from == NULL) {
        return NGX_OK;
    }

    if (ngx_http_complex_value(r, clcf->disable_symlinks_from, &from)
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    if (from.len == 0
        || from.len > path->len
        || ngx_memcmp(path->data, from.data, from.len) != 0)
    {
        return NGX_OK;
    }

    if (from.len == path->len) {
        of->disable_symlinks = NGX_DISABLE_SYMLINKS_OFF;
        return NGX_OK;
    }

    p = path->data + from.len;

    if (*p == '/') {
        of->disable_symlinks_from = from.len;
        return NGX_OK;
    }

    p--;

    if (*p == '/') {
        of->disable_symlinks_from = from.len - 1;
    }
#endif

    return NGX_OK;
}


ngx_int_t
ngx_http_get_forwarded_addr(ngx_http_request_t *r, ngx_addr_t *addr,
    ngx_array_t *headers, ngx_str_t *value, ngx_array_t *proxies,
    int recursive)
{
    ngx_int_t          rc;
    ngx_uint_t         i, found;
    ngx_table_elt_t  **h;

    if (headers == NULL) {
        return ngx_http_get_forwarded_addr_internal(r, addr, value->data,
                                                    value->len, proxies,
                                                    recursive);
    }

    i = headers->nelts;
    h = headers->elts;

    rc = NGX_DECLINED;

    found = 0;

    while (i-- > 0) {
        rc = ngx_http_get_forwarded_addr_internal(r, addr, h[i]->value.data,
                                                  h[i]->value.len, proxies,
                                                  recursive);

        if (!recursive) {
            break;
        }

        if (rc == NGX_DECLINED && found) {
            rc = NGX_DONE;
            break;
        }

        if (rc != NGX_OK) {
            break;
        }

        found = 1;
    }

    return rc;
}


static ngx_int_t
ngx_http_get_forwarded_addr_internal(ngx_http_request_t *r, ngx_addr_t *addr,
    u_char *xff, size_t xfflen, ngx_array_t *proxies, int recursive)
{
    u_char           *p;
    in_addr_t         inaddr;
    ngx_int_t         rc;
    ngx_addr_t        paddr;
    ngx_cidr_t       *cidr;
    ngx_uint_t        family, i;
#if (NGX_HAVE_INET6)
    ngx_uint_t        n;
    struct in6_addr  *inaddr6;
#endif

#if (NGX_SUPPRESS_WARN)
    inaddr = 0;
#if (NGX_HAVE_INET6)
    inaddr6 = NULL;
#endif
#endif

    family = addr->sockaddr->sa_family;

    if (family == AF_INET) {
        inaddr = ((struct sockaddr_in *) addr->sockaddr)->sin_addr.s_addr;
    }

#if (NGX_HAVE_INET6)
    else if (family == AF_INET6) {
        inaddr6 = &((struct sockaddr_in6 *) addr->sockaddr)->sin6_addr;

        if (IN6_IS_ADDR_V4MAPPED(inaddr6)) {
            family = AF_INET;

            p = inaddr6->s6_addr;

            inaddr = p[12] << 24;
            inaddr += p[13] << 16;
            inaddr += p[14] << 8;
            inaddr += p[15];

            inaddr = htonl(inaddr);
        }
    }
#endif

    for (cidr = proxies->elts, i = 0; i < proxies->nelts; i++) {
        if (cidr[i].family != family) {
            goto next;
        }

        switch (family) {

#if (NGX_HAVE_INET6)
        case AF_INET6:
            for (n = 0; n < 16; n++) {
                if ((inaddr6->s6_addr[n] & cidr[i].u.in6.mask.s6_addr[n])
                    != cidr[i].u.in6.addr.s6_addr[n])
                {
                    goto next;
                }
            }
            break;
#endif

#if (NGX_HAVE_UNIX_DOMAIN)
        case AF_UNIX:
            break;
#endif

        default: /* AF_INET */
            if ((inaddr & cidr[i].u.in.mask) != cidr[i].u.in.addr) {
                goto next;
            }
            break;
        }

        for (p = xff + xfflen - 1; p > xff; p--, xfflen--) {
            if (*p != ' ' && *p != ',') {
                break;
            }
        }

        for ( /* void */ ; p > xff; p--) {
            if (*p == ' ' || *p == ',') {
                p++;
                break;
            }
        }

        if (ngx_parse_addr(r->pool, &paddr, p, xfflen - (p - xff)) != NGX_OK) {
            return NGX_DECLINED;
        }

        *addr = paddr;

        if (recursive && p > xff) {
            rc = ngx_http_get_forwarded_addr_internal(r, addr, xff, p - 1 - xff,
                                                      proxies, 1);

            if (rc == NGX_DECLINED) {
                return NGX_DONE;
            }

            /* rc == NGX_OK || rc == NGX_DONE  */
            return rc;
        }

        return NGX_OK;

    next:
        continue;
    }

    return NGX_DECLINED;
}


static char *
ngx_http_core_server(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy)
{
    char                        *rv;
    void                        *mconf;
    ngx_uint_t                   i;
    ngx_conf_t                   pcf;
    ngx_http_module_t           *module;
    struct sockaddr_in          *sin;
    ngx_http_conf_ctx_t         *ctx, *http_ctx;
    ngx_http_listen_opt_t        lsopt;
    ngx_http_core_srv_conf_t    *cscf, **cscfp;
    ngx_http_core_main_conf_t   *cmcf;

	//建立属于这个server块ngx_http_conf_ctx_t结构体
	//main_conf将指向所属的http块下ngx_http_conf_ctx_t结构体main_conf指针数组
	//而srv_conf和loc_conf都将指向重新分配大小为ngx_http_max_module的指针数组
    ctx = ngx_pcalloc(cf->pool, sizeof(ngx_http_conf_ctx_t));
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    http_ctx = cf->ctx;
    ctx->main_conf = http_ctx->main_conf;

    /* the server{}'s srv_conf */
    ctx->srv_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_http_max_module);
    if (ctx->srv_conf == NULL) {
        return NGX_CONF_ERROR;
    }
	
	//仅用于合并location相关的配置项
    ctx->loc_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_http_max_module);
    if (ctx->loc_conf == NULL) {
        return NGX_CONF_ERROR;
    }

	//循环调用所有HTTP模块create_srv_conf方法和create_loc_conf方法，
	//将返回的结构体指针按照模块序号ctx_index保存到srv_conf和loc_conf指针数组中
    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->type != NGX_HTTP_MODULE) {
            continue;
        }

        module = ngx_modules[i]->ctx;

        if (module->create_srv_conf) {
            mconf = module->create_srv_conf(cf);
            if (mconf == NULL) {
                return NGX_CONF_ERROR;
            }

            ctx->srv_conf[ngx_modules[i]->ctx_index] = mconf;
        }

        if (module->create_loc_conf) {
            mconf = module->create_loc_conf(cf);
            if (mconf == NULL) {
                return NGX_CONF_ERROR;
            }

            ctx->loc_conf[ngx_modules[i]->ctx_index] = mconf;
        }
    }


    /* the server configuration context */
	//ngx_http_core_module模块的create_srv_conf方法生成的ngx_http_core_srv_conf_t配置结构体对应着当前正在解析的server块
    cscf = ctx->srv_conf[ngx_http_core_module.ctx_index];	//获取ngx_http_core_module模块的server{}下的srv配置
    cscf->ctx = ctx;

    cmcf = ctx->main_conf[ngx_http_core_module.ctx_index];	//获取ngx_http_core_module模块的http{}下的main配置
    cscfp = ngx_array_push(&cmcf->servers);	//将ngx_http_core_module模块在某sever{}下的srv配置添加到ngx_http_core_module模块在http{}下的main配置的servers动态数组中
    if (cscfp == NULL) {
        return NGX_CONF_ERROR;
    }

    *cscfp = cscf;


    //解析当前server{}块内的所有配置项
    pcf = *cf;
    cf->ctx = ctx;
    cf->cmd_type = NGX_HTTP_SRV_CONF;

    rv = ngx_conf_parse(cf, NULL);

    *cf = pcf;

	//若server{}内没有配置监听端口(listen指令)，
	//那么创建一个默认的监听端口,确保每个server配置至少有一个监听套接口
    if (rv == NGX_CONF_OK && !cscf->listen) {
        ngx_memzero(&lsopt, sizeof(ngx_http_listen_opt_t));

        sin = &lsopt.u.sockaddr_in;

        sin->sin_family = AF_INET;
#if (NGX_WIN32)
        sin->sin_port = htons(80);
#else
        sin->sin_port = htons((getuid() == 0) ? 80 : 8000);
#endif
        sin->sin_addr.s_addr = INADDR_ANY;

        lsopt.socklen = sizeof(struct sockaddr_in);

        lsopt.backlog = NGX_LISTEN_BACKLOG;
        lsopt.rcvbuf = -1;
        lsopt.sndbuf = -1;
#if (NGX_HAVE_SETFIB)
        lsopt.setfib = -1;
#endif
#if (NGX_HAVE_TCP_FASTOPEN)
        lsopt.fastopen = -1;
#endif
        lsopt.wildcard = 1;

        (void) ngx_sock_ntop(&lsopt.u.sockaddr, lsopt.socklen, lsopt.addr, NGX_SOCKADDR_STRLEN, 1);

        if (ngx_http_add_listen(cf, cscf, &lsopt) != NGX_OK) {
            return NGX_CONF_ERROR;
        }
    }

    return rv;
}


static char *
ngx_http_core_location(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy)
{
    char                      *rv;
    u_char                    *mod;
    size_t                     len;
    ngx_str_t                 *value, *name;
    ngx_uint_t                 i;
    ngx_conf_t                 save;
    ngx_http_module_t         *module;
    ngx_http_conf_ctx_t       *ctx, *pctx;
    ngx_http_core_loc_conf_t  *clcf, *pclcf;

	//建立属于这个location块ngx_http_conf_ctx_t结构体
	//main_conf将指向所属的server块下ngx_http_conf_ctx_t结构体main_conf指针数组，
	//srv_conf将指向所属的server块下ngx_http_conf_ctx_t结构体srv_conf指针数组，
	//而loc_conf将指向重新分配大小为ngx_http_max_module的指针数组

    ctx = ngx_pcalloc(cf->pool, sizeof(ngx_http_conf_ctx_t));
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    pctx = cf->ctx;
    ctx->main_conf = pctx->main_conf;
    ctx->srv_conf = pctx->srv_conf;

    ctx->loc_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_http_max_module);
    if (ctx->loc_conf == NULL) {
        return NGX_CONF_ERROR;
    }

	//循环调用所有HTTP模块的create_loc_conf方法，
	//将返回的结构体指针按照模块序号ctx_index保存到loc_conf指针数组中
    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->type != NGX_HTTP_MODULE) {
            continue;
        }

        module = ngx_modules[i]->ctx;

        if (module->create_loc_conf) {
            ctx->loc_conf[ngx_modules[i]->ctx_index] = module->create_loc_conf(cf);
            if (ctx->loc_conf[ngx_modules[i]->ctx_index] == NULL) {
                 return NGX_CONF_ERROR;
            }
        }
    }

	//ngx_http_core_module模块的create_loc_conf方法生成的ngx_http_core_loc_conf_t配置结构体对应着当前正在解析的location块
    clcf = ctx->loc_conf[ngx_http_core_module.ctx_index];  //获取ngx_http_core_module模块在location{}下的loc配置
    clcf->loc_conf = ctx->loc_conf;

    value = cf->args->elts;
    if (cf->args->nelts == 3) {
        len = value[1].len;
        mod = value[1].data;
        name = &value[2];

        if (len == 1 && mod[0] == '=') {
            clcf->name = *name;
            clcf->exact_match = 1;
        }  else if (len == 2 && mod[0] == '^' && mod[1] == '~') {
            clcf->name = *name;
            clcf->noregex = 1;
        } else if (len == 1 && mod[0] == '~') {
            if (ngx_http_core_regex_location(cf, clcf, name, 0) != NGX_OK) {
                return NGX_CONF_ERROR;
            }
        } else if (len == 2 && mod[0] == '~' && mod[1] == '*') {
            if (ngx_http_core_regex_location(cf, clcf, name, 1) != NGX_OK) {
                return NGX_CONF_ERROR;
            }
        } else {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid location modifier \"%V\"", &value[1]);
            return NGX_CONF_ERROR;
        }

    }
	else 
	{
        name = &value[1];

        if (name->data[0] == '=') {
            clcf->name.len = name->len - 1;
            clcf->name.data = name->data + 1;
            clcf->exact_match = 1;
        } else if (name->data[0] == '^' && name->data[1] == '~') {
            clcf->name.len = name->len - 2;
            clcf->name.data = name->data + 2;
            clcf->noregex = 1;
        } else if (name->data[0] == '~') {
            name->len--;
            name->data++;

            if (name->data[0] == '*') {
                name->len--;
                name->data++;

                if (ngx_http_core_regex_location(cf, clcf, name, 1) != NGX_OK) {
                    return NGX_CONF_ERROR;
                }

            } else {
                if (ngx_http_core_regex_location(cf, clcf, name, 0) != NGX_OK) {
                    return NGX_CONF_ERROR;
                }
            }
        } else {
            clcf->name = *name;
            if (name->data[0] == '@') {
                clcf->named = 1;
            }
        }
    }

    pclcf = pctx->loc_conf[ngx_http_core_module.ctx_index];  //获取ngx_http_core_module模块在location{}的上一级下的loc配置(server{}下或嵌套location{}下)

    if (cf->cmd_type == NGX_HTTP_LOC_CONF) { //嵌套的location块，检查是否与上级location块冲突
        /* nested location */
#if 0
        clcf->prev_location = pclcf;
#endif

        if (pclcf->exact_match) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "location \"%V\" cannot be inside the exact location \"%V\"", &clcf->name, &pclcf->name);
            return NGX_CONF_ERROR;
        }

        if (pclcf->named) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "location \"%V\" cannot be inside the named location \"%V\"", &clcf->name, &pclcf->name);
            return NGX_CONF_ERROR;
        }

        if (clcf->named) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "named location \"%V\" can be on the server level only", &clcf->name);
            return NGX_CONF_ERROR;
        }

        len = pclcf->name.len;

#if (NGX_PCRE)
        if (clcf->regex == NULL && ngx_filename_cmp(clcf->name.data, pclcf->name.data, len) != 0)
#else
        if (ngx_filename_cmp(clcf->name.data, pclcf->name.data, len) != 0)
#endif
        {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "location \"%V\" is outside location \"%V\"", &clcf->name, &pclcf->name);
            return NGX_CONF_ERROR;
        }
    }

	//ngx_http_core_module模块的create_loc_conf方法生成的ngx_http_core_loc_conf_t配置结构体对应着当前正在解析的location块
	//这时会生成ngx_http_location_queue_t结构体，因为每一个ngx_http_core_loc_conf_t结构体都对应着1个ngx_http_location_queue_t,
	//因此，此处将把ngx_http_location_queue_t串联成双向链表
    if (ngx_http_add_location(cf, &pclcf->locations, clcf) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

	//解析当前location{}配置块内的loc级别配置项
    save = *cf;
    cf->ctx = ctx;
    cf->cmd_type = NGX_HTTP_LOC_CONF;

    rv = ngx_conf_parse(cf, NULL);

    *cf = save;

    return rv;
}

//caseless -- 1 不区分大小写， 0 区分大小写
static ngx_int_t
ngx_http_core_regex_location(ngx_conf_t *cf, ngx_http_core_loc_conf_t *clcf, ngx_str_t *regex, ngx_uint_t caseless)
{
#if (NGX_PCRE)
    ngx_regex_compile_t  rc;
    u_char               errstr[NGX_MAX_CONF_ERRSTR];

    ngx_memzero(&rc, sizeof(ngx_regex_compile_t));

    rc.pattern = *regex;
    rc.err.len = NGX_MAX_CONF_ERRSTR;
    rc.err.data = errstr;

#if (NGX_HAVE_CASELESS_FILESYSTEM)
    rc.options = NGX_REGEX_CASELESS;
#else
    rc.options = caseless ? NGX_REGEX_CASELESS : 0;
#endif

    clcf->regex = ngx_http_regex_compile(cf, &rc);
    if (clcf->regex == NULL) 
	{
        return NGX_ERROR;
    }

    clcf->name = *regex;

    return NGX_OK;

#else

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "using regex \"%V\" requires PCRE library", regex);
    return NGX_ERROR;

#endif
}


/*
type {}
*/

static char *
ngx_http_core_types(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf = conf;

    char        *rv;
    ngx_conf_t   save;

    if (clcf->types == NULL) 
	{
        clcf->types = ngx_array_create(cf->pool, 64, sizeof(ngx_hash_key_t));
        if (clcf->types == NULL) 
		{
            return NGX_CONF_ERROR;
        }
    }

    save = *cf;
    cf->handler = ngx_http_core_type;
    cf->handler_conf = conf;

    rv = ngx_conf_parse(cf, NULL);

    *cf = save;

    return rv;
}


static char *
ngx_http_core_type(ngx_conf_t *cf, ngx_command_t *dummy, void *conf)
{
    ngx_http_core_loc_conf_t *clcf = conf;

    ngx_str_t       *value, *content_type, *old;
    ngx_uint_t       i, n, hash;
    ngx_hash_key_t  *type;

    value = cf->args->elts;

    if (ngx_strcmp(value[0].data, "include") == 0)
	{
        if (cf->args->nelts != 2)
		{
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid number of arguments in \"include\" directive");
            return NGX_CONF_ERROR;
        }

        return ngx_conf_include(cf, dummy, conf);
    }

    content_type = ngx_palloc(cf->pool, sizeof(ngx_str_t));
    if (content_type == NULL)
	{
        return NGX_CONF_ERROR;
    }

    *content_type = value[0];

    for (i = 1; i < cf->args->nelts; i++) 
	{
        hash = ngx_hash_strlow(value[i].data, value[i].data, value[i].len);

        type = clcf->types->elts;
        for (n = 0; n < clcf->types->nelts; n++) 
		{
            if (ngx_strcmp(value[i].data, type[n].key.data) == 0) 
			{
                old = type[n].value;
                type[n].value = content_type;

                ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "duplicate extension \"%V\", content type: \"%V\", "
						"previous content type: \"%V\"", &value[i], content_type, old);
                goto next;
            }
        }


        type = ngx_array_push(clcf->types);
        if (type == NULL) 
		{
            return NGX_CONF_ERROR;
        }

        type->key = value[i];
        type->key_hash = hash;
        type->value = content_type;

    next:
        continue;
    }

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_core_preconfiguration(ngx_conf_t *cf)
{
    return ngx_http_variables_add_core_vars(cf);
}


static ngx_int_t
ngx_http_core_postconfiguration(ngx_conf_t *cf)
{
    ngx_http_top_request_body_filter = ngx_http_request_body_save_filter;

    return NGX_OK;
}


static void *
ngx_http_core_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_core_main_conf_t));
    if (cmcf == NULL) 
	{
        return NULL;
    }

    if (ngx_array_init(&cmcf->servers, cf->pool, 4, sizeof(ngx_http_core_srv_conf_t *)) != NGX_OK)
    {
        return NULL;
    }

    cmcf->server_names_hash_max_size = NGX_CONF_UNSET_UINT;
    cmcf->server_names_hash_bucket_size = NGX_CONF_UNSET_UINT;

    cmcf->variables_hash_max_size = NGX_CONF_UNSET_UINT;
    cmcf->variables_hash_bucket_size = NGX_CONF_UNSET_UINT;

    return cmcf;
}


static char *
ngx_http_core_init_main_conf(ngx_conf_t *cf, void *conf)
{
    ngx_http_core_main_conf_t *cmcf = conf;

    ngx_conf_init_uint_value(cmcf->server_names_hash_max_size, 512);
    ngx_conf_init_uint_value(cmcf->server_names_hash_bucket_size, ngx_cacheline_size);

    cmcf->server_names_hash_bucket_size =
            ngx_align(cmcf->server_names_hash_bucket_size, ngx_cacheline_size);


    ngx_conf_init_uint_value(cmcf->variables_hash_max_size, 1024);
    ngx_conf_init_uint_value(cmcf->variables_hash_bucket_size, 64);

    cmcf->variables_hash_bucket_size =
               ngx_align(cmcf->variables_hash_bucket_size, ngx_cacheline_size);

    if (cmcf->ncaptures) {
        cmcf->ncaptures = (cmcf->ncaptures + 1) * 3;
    }

    return NGX_CONF_OK;
}


static void *
ngx_http_core_create_srv_conf(ngx_conf_t *cf)
{
    ngx_http_core_srv_conf_t  *cscf;

    cscf = ngx_pcalloc(cf->pool, sizeof(ngx_http_core_srv_conf_t));
    if (cscf == NULL) 
	{
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->client_large_buffers.num = 0;
     */

    if (ngx_array_init(&cscf->server_names, cf->temp_pool, 4, sizeof(ngx_http_server_name_t)) != NGX_OK)
    {
        return NULL;
    }

    cscf->connection_pool_size = NGX_CONF_UNSET_SIZE;
    cscf->request_pool_size = NGX_CONF_UNSET_SIZE;
    cscf->client_header_timeout = NGX_CONF_UNSET_MSEC;
    cscf->client_header_buffer_size = NGX_CONF_UNSET_SIZE;
    cscf->ignore_invalid_headers = NGX_CONF_UNSET;
    cscf->merge_slashes = NGX_CONF_UNSET;
    cscf->underscores_in_headers = NGX_CONF_UNSET;

    return cscf;
}


static char *
ngx_http_core_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_core_srv_conf_t *prev = parent;
    ngx_http_core_srv_conf_t *conf = child;

    ngx_str_t                name;
    ngx_http_server_name_t  *sn;

    /* TODO: it does not merge, it inits only */

    ngx_conf_merge_size_value(conf->connection_pool_size, prev->connection_pool_size, 256);
    ngx_conf_merge_size_value(conf->request_pool_size, prev->request_pool_size, 4096);
    ngx_conf_merge_msec_value(conf->client_header_timeout, prev->client_header_timeout, 60000);
    ngx_conf_merge_size_value(conf->client_header_buffer_size, prev->client_header_buffer_size, 1024);
    ngx_conf_merge_bufs_value(conf->large_client_header_buffers, prev->large_client_header_buffers, 4, 8192);

    if (conf->large_client_header_buffers.size < conf->connection_pool_size) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "the \"large_client_header_buffers\" size must be "
                           "equal to or greater than \"connection_pool_size\"");
        return NGX_CONF_ERROR;
    }

    ngx_conf_merge_value(conf->ignore_invalid_headers, prev->ignore_invalid_headers, 1);

    ngx_conf_merge_value(conf->merge_slashes, prev->merge_slashes, 1);

    ngx_conf_merge_value(conf->underscores_in_headers, prev->underscores_in_headers, 0);

    if (conf->server_names.nelts == 0) {
        /* the array has 4 empty preallocated elements, so push cannot fail */
        sn = ngx_array_push(&conf->server_names);
#if (NGX_PCRE)
        sn->regex = NULL;
#endif
        sn->server = conf;
        ngx_str_set(&sn->name, "");
    }

    sn = conf->server_names.elts;
    name = sn[0].name;

#if (NGX_PCRE)
    if (sn->regex) {
        name.len++;
        name.data--;
    } else
#endif

    if (name.data[0] == '.') {
        name.len--;
        name.data++;
    }

    conf->server_name.len = name.len;
    conf->server_name.data = ngx_pstrdup(cf->pool, &name);
    if (conf->server_name.data == NULL) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static void *
ngx_http_core_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_core_loc_conf_t));
    if (clcf == NULL) 
	{
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     clcf->root = { 0, NULL };
     *     clcf->limit_except = 0;
     *     clcf->post_action = { 0, NULL };
     *     clcf->types = NULL;
     *     clcf->default_type = { 0, NULL };
     *     clcf->error_log = NULL;
     *     clcf->error_pages = NULL;
     *     clcf->try_files = NULL;
     *     clcf->client_body_path = NULL;
     *     clcf->regex = NULL;
     *     clcf->exact_match = 0;
     *     clcf->auto_redirect = 0;
     *     clcf->alias = 0;
     *     clcf->gzip_proxied = 0;
     *     clcf->keepalive_disable = 0;
     */

    clcf->client_max_body_size = NGX_CONF_UNSET;
    clcf->client_body_buffer_size = NGX_CONF_UNSET_SIZE;
    clcf->client_body_timeout = NGX_CONF_UNSET_MSEC;
    clcf->satisfy = NGX_CONF_UNSET_UINT;
    clcf->if_modified_since = NGX_CONF_UNSET_UINT;
    clcf->max_ranges = NGX_CONF_UNSET_UINT;
    clcf->client_body_in_file_only = NGX_CONF_UNSET_UINT;
    clcf->client_body_in_single_buffer = NGX_CONF_UNSET;
    clcf->internal = NGX_CONF_UNSET;
    clcf->sendfile = NGX_CONF_UNSET;
    clcf->sendfile_max_chunk = NGX_CONF_UNSET_SIZE;
    clcf->aio = NGX_CONF_UNSET;
#if (NGX_THREADS)
    clcf->thread_pool = NGX_CONF_UNSET_PTR;
    clcf->thread_pool_value = NGX_CONF_UNSET_PTR;
#endif
    clcf->read_ahead = NGX_CONF_UNSET_SIZE;
    clcf->directio = NGX_CONF_UNSET;
    clcf->directio_alignment = NGX_CONF_UNSET;
    clcf->tcp_nopush = NGX_CONF_UNSET;
    clcf->tcp_nodelay = NGX_CONF_UNSET;
    clcf->send_timeout = NGX_CONF_UNSET_MSEC;
    clcf->send_lowat = NGX_CONF_UNSET_SIZE;
    clcf->postpone_output = NGX_CONF_UNSET_SIZE;
    clcf->limit_rate = NGX_CONF_UNSET_SIZE;
    clcf->limit_rate_after = NGX_CONF_UNSET_SIZE;
    clcf->keepalive_timeout = NGX_CONF_UNSET_MSEC;
    clcf->keepalive_header = NGX_CONF_UNSET;
    clcf->keepalive_requests = NGX_CONF_UNSET_UINT;
    clcf->lingering_close = NGX_CONF_UNSET_UINT;
    clcf->lingering_time = NGX_CONF_UNSET_MSEC;
    clcf->lingering_timeout = NGX_CONF_UNSET_MSEC;
    clcf->resolver_timeout = NGX_CONF_UNSET_MSEC;
    clcf->reset_timedout_connection = NGX_CONF_UNSET;
    clcf->server_name_in_redirect = NGX_CONF_UNSET;
    clcf->port_in_redirect = NGX_CONF_UNSET;
    clcf->msie_padding = NGX_CONF_UNSET;
    clcf->msie_refresh = NGX_CONF_UNSET;
    clcf->log_not_found = NGX_CONF_UNSET;
    clcf->log_subrequest = NGX_CONF_UNSET;
    clcf->recursive_error_pages = NGX_CONF_UNSET;
    clcf->server_tokens = NGX_CONF_UNSET;
    clcf->chunked_transfer_encoding = NGX_CONF_UNSET;
    clcf->etag = NGX_CONF_UNSET;
    clcf->types_hash_max_size = NGX_CONF_UNSET_UINT;
    clcf->types_hash_bucket_size = NGX_CONF_UNSET_UINT;

    clcf->open_file_cache = NGX_CONF_UNSET_PTR;
    clcf->open_file_cache_valid = NGX_CONF_UNSET;
    clcf->open_file_cache_min_uses = NGX_CONF_UNSET_UINT;
    clcf->open_file_cache_errors = NGX_CONF_UNSET;
    clcf->open_file_cache_events = NGX_CONF_UNSET;

#if (NGX_HTTP_GZIP)
    clcf->gzip_vary = NGX_CONF_UNSET;
    clcf->gzip_http_version = NGX_CONF_UNSET_UINT;
#if (NGX_PCRE)
    clcf->gzip_disable = NGX_CONF_UNSET_PTR;
#endif
    clcf->gzip_disable_msie6 = 3;
#if (NGX_HTTP_DEGRADATION)
    clcf->gzip_disable_degradation = 3;
#endif
#endif

#if (NGX_HAVE_OPENAT)
    clcf->disable_symlinks = NGX_CONF_UNSET_UINT;
    clcf->disable_symlinks_from = NGX_CONF_UNSET_PTR;
#endif

    return clcf;
}


static ngx_str_t  ngx_http_core_text_html_type = ngx_string("text/html");
static ngx_str_t  ngx_http_core_image_gif_type = ngx_string("image/gif");
static ngx_str_t  ngx_http_core_image_jpeg_type = ngx_string("image/jpeg");

static ngx_hash_key_t  ngx_http_core_default_types[] = {
    { ngx_string("html"), 0, &ngx_http_core_text_html_type },
    { ngx_string("gif"), 0, &ngx_http_core_image_gif_type },
    { ngx_string("jpg"), 0, &ngx_http_core_image_jpeg_type },
    { ngx_null_string, 0, NULL }
};


static char *
ngx_http_core_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_core_loc_conf_t *prev = parent;
    ngx_http_core_loc_conf_t *conf = child;

    ngx_uint_t        i;
    ngx_hash_key_t   *type;
    ngx_hash_init_t   types_hash;

    if (conf->root.data == NULL) {

        conf->alias = prev->alias;
        conf->root = prev->root;
        conf->root_lengths = prev->root_lengths;
        conf->root_values = prev->root_values;

        if (prev->root.data == NULL) {
            ngx_str_set(&conf->root, "html");

            if (ngx_conf_full_name(cf->cycle, &conf->root, 0) != NGX_OK) {
                return NGX_CONF_ERROR;
            }
        }
    }

    if (conf->post_action.data == NULL) {
        conf->post_action = prev->post_action;
    }

    ngx_conf_merge_uint_value(conf->types_hash_max_size,
                              prev->types_hash_max_size, 1024);

    ngx_conf_merge_uint_value(conf->types_hash_bucket_size,
                              prev->types_hash_bucket_size, 64);

    conf->types_hash_bucket_size = ngx_align(conf->types_hash_bucket_size,
                                             ngx_cacheline_size);

    /*
     * the special handling of the "types" directive in the "http" section
     * to inherit the http's conf->types_hash to all servers
     */

    if (prev->types && prev->types_hash.buckets == NULL) {

        types_hash.hash = &prev->types_hash;
        types_hash.key = ngx_hash_key_lc;
        types_hash.max_size = conf->types_hash_max_size;
        types_hash.bucket_size = conf->types_hash_bucket_size;
        types_hash.name = "types_hash";
        types_hash.pool = cf->pool;
        types_hash.temp_pool = NULL;

        if (ngx_hash_init(&types_hash, prev->types->elts, prev->types->nelts)
            != NGX_OK)
        {
            return NGX_CONF_ERROR;
        }
    }

    if (conf->types == NULL) {
        conf->types = prev->types;
        conf->types_hash = prev->types_hash;
    }

    if (conf->types == NULL) {
        conf->types = ngx_array_create(cf->pool, 3, sizeof(ngx_hash_key_t));
        if (conf->types == NULL) {
            return NGX_CONF_ERROR;
        }

        for (i = 0; ngx_http_core_default_types[i].key.len; i++) {
            type = ngx_array_push(conf->types);
            if (type == NULL) {
                return NGX_CONF_ERROR;
            }

            type->key = ngx_http_core_default_types[i].key;
            type->key_hash =
                       ngx_hash_key_lc(ngx_http_core_default_types[i].key.data,
                                       ngx_http_core_default_types[i].key.len);
            type->value = ngx_http_core_default_types[i].value;
        }
    }

    if (conf->types_hash.buckets == NULL) {

        types_hash.hash = &conf->types_hash;
        types_hash.key = ngx_hash_key_lc;
        types_hash.max_size = conf->types_hash_max_size;
        types_hash.bucket_size = conf->types_hash_bucket_size;
        types_hash.name = "types_hash";
        types_hash.pool = cf->pool;
        types_hash.temp_pool = NULL;

        if (ngx_hash_init(&types_hash, conf->types->elts, conf->types->nelts)
            != NGX_OK)
        {
            return NGX_CONF_ERROR;
        }
    }

    if (conf->error_log == NULL) {
        if (prev->error_log) {
            conf->error_log = prev->error_log;
        } else {
            conf->error_log = &cf->cycle->new_log;
        }
    }

    if (conf->error_pages == NULL && prev->error_pages) {
        conf->error_pages = prev->error_pages;
    }

    ngx_conf_merge_str_value(conf->default_type,
                              prev->default_type, "text/plain");

    ngx_conf_merge_off_value(conf->client_max_body_size, prev->client_max_body_size, 1 * 1024 * 1024);
    ngx_conf_merge_size_value(conf->client_body_buffer_size, prev->client_body_buffer_size, (size_t) 2 * ngx_pagesize);
    ngx_conf_merge_msec_value(conf->client_body_timeout, prev->client_body_timeout, 60000);

    ngx_conf_merge_bitmask_value(conf->keepalive_disable,
                              prev->keepalive_disable,
                              (NGX_CONF_BITMASK_SET
                               |NGX_HTTP_KEEPALIVE_DISABLE_MSIE6));
    ngx_conf_merge_uint_value(conf->satisfy, prev->satisfy,
                              NGX_HTTP_SATISFY_ALL);
    ngx_conf_merge_uint_value(conf->if_modified_since, prev->if_modified_since,
                              NGX_HTTP_IMS_EXACT);
    ngx_conf_merge_uint_value(conf->max_ranges, prev->max_ranges,
                              NGX_MAX_INT32_VALUE);
    ngx_conf_merge_uint_value(conf->client_body_in_file_only,
                              prev->client_body_in_file_only,
                              NGX_HTTP_REQUEST_BODY_FILE_OFF);
    ngx_conf_merge_value(conf->client_body_in_single_buffer,
                              prev->client_body_in_single_buffer, 0);
    ngx_conf_merge_value(conf->internal, prev->internal, 0);
    ngx_conf_merge_value(conf->sendfile, prev->sendfile, 0);
    ngx_conf_merge_size_value(conf->sendfile_max_chunk,
                              prev->sendfile_max_chunk, 0);
#if (NGX_HAVE_FILE_AIO || NGX_THREADS)
    ngx_conf_merge_value(conf->aio, prev->aio, NGX_HTTP_AIO_OFF);
#endif
#if (NGX_THREADS)
    ngx_conf_merge_ptr_value(conf->thread_pool, prev->thread_pool, NULL);
    ngx_conf_merge_ptr_value(conf->thread_pool_value, prev->thread_pool_value,
                             NULL);
#endif
    ngx_conf_merge_size_value(conf->read_ahead, prev->read_ahead, 0);
    ngx_conf_merge_off_value(conf->directio, prev->directio,
                              NGX_OPEN_FILE_DIRECTIO_OFF);
    ngx_conf_merge_off_value(conf->directio_alignment, prev->directio_alignment,
                              512);
    ngx_conf_merge_value(conf->tcp_nopush, prev->tcp_nopush, 0);
    ngx_conf_merge_value(conf->tcp_nodelay, prev->tcp_nodelay, 1);

    ngx_conf_merge_msec_value(conf->send_timeout, prev->send_timeout, 60000);
    ngx_conf_merge_size_value(conf->send_lowat, prev->send_lowat, 0);
    ngx_conf_merge_size_value(conf->postpone_output, prev->postpone_output,
                              1460);
    ngx_conf_merge_size_value(conf->limit_rate, prev->limit_rate, 0);
    ngx_conf_merge_size_value(conf->limit_rate_after, prev->limit_rate_after,
                              0);
    ngx_conf_merge_msec_value(conf->keepalive_timeout, prev->keepalive_timeout, 75000);
    ngx_conf_merge_sec_value(conf->keepalive_header, prev->keepalive_header, 0);
    ngx_conf_merge_uint_value(conf->keepalive_requests, prev->keepalive_requests, 100);
    ngx_conf_merge_uint_value(conf->lingering_close, prev->lingering_close, NGX_HTTP_LINGERING_ON);
    ngx_conf_merge_msec_value(conf->lingering_time,
                              prev->lingering_time, 30000);
    ngx_conf_merge_msec_value(conf->lingering_timeout,
                              prev->lingering_timeout, 5000);
    ngx_conf_merge_msec_value(conf->resolver_timeout,
                              prev->resolver_timeout, 30000);

    if (conf->resolver == NULL) {

        if (prev->resolver == NULL) {

            /*
             * create dummy resolver in http {} context
             * to inherit it in all servers
             */

            prev->resolver = ngx_resolver_create(cf, NULL, 0);
            if (prev->resolver == NULL) {
                return NGX_CONF_ERROR;
            }
        }

        conf->resolver = prev->resolver;
    }

    if (ngx_conf_merge_path_value(cf, &conf->client_body_temp_path,
                              prev->client_body_temp_path,
                              &ngx_http_client_temp_path)
        != NGX_OK)
    {
        return NGX_CONF_ERROR;
    }

    ngx_conf_merge_value(conf->reset_timedout_connection,
                              prev->reset_timedout_connection, 0);
    ngx_conf_merge_value(conf->server_name_in_redirect,
                              prev->server_name_in_redirect, 0);
    ngx_conf_merge_value(conf->port_in_redirect, prev->port_in_redirect, 1);
    ngx_conf_merge_value(conf->msie_padding, prev->msie_padding, 1);
    ngx_conf_merge_value(conf->msie_refresh, prev->msie_refresh, 0);
    ngx_conf_merge_value(conf->log_not_found, prev->log_not_found, 1);
    ngx_conf_merge_value(conf->log_subrequest, prev->log_subrequest, 0);
    ngx_conf_merge_value(conf->recursive_error_pages,
                              prev->recursive_error_pages, 0);
    ngx_conf_merge_value(conf->server_tokens, prev->server_tokens, 1);
    ngx_conf_merge_value(conf->chunked_transfer_encoding,
                              prev->chunked_transfer_encoding, 1);
    ngx_conf_merge_value(conf->etag, prev->etag, 1);

    ngx_conf_merge_ptr_value(conf->open_file_cache,
                              prev->open_file_cache, NULL);

    ngx_conf_merge_sec_value(conf->open_file_cache_valid,
                              prev->open_file_cache_valid, 60);

    ngx_conf_merge_uint_value(conf->open_file_cache_min_uses, prev->open_file_cache_min_uses, 1);

    ngx_conf_merge_sec_value(conf->open_file_cache_errors, prev->open_file_cache_errors, 0);

    ngx_conf_merge_sec_value(conf->open_file_cache_events, prev->open_file_cache_events, 0);
#if (NGX_HTTP_GZIP)

    ngx_conf_merge_value(conf->gzip_vary, prev->gzip_vary, 0);
    ngx_conf_merge_uint_value(conf->gzip_http_version, prev->gzip_http_version,
                              NGX_HTTP_VERSION_11);
    ngx_conf_merge_bitmask_value(conf->gzip_proxied, prev->gzip_proxied,
                              (NGX_CONF_BITMASK_SET|NGX_HTTP_GZIP_PROXIED_OFF));

#if (NGX_PCRE)
    ngx_conf_merge_ptr_value(conf->gzip_disable, prev->gzip_disable, NULL);
#endif

    if (conf->gzip_disable_msie6 == 3) {
        conf->gzip_disable_msie6 =
            (prev->gzip_disable_msie6 == 3) ? 0 : prev->gzip_disable_msie6;
    }

#if (NGX_HTTP_DEGRADATION)

    if (conf->gzip_disable_degradation == 3) {
        conf->gzip_disable_degradation =
            (prev->gzip_disable_degradation == 3) ?
                 0 : prev->gzip_disable_degradation;
    }

#endif
#endif

#if (NGX_HAVE_OPENAT)
    ngx_conf_merge_uint_value(conf->disable_symlinks, prev->disable_symlinks,
                              NGX_DISABLE_SYMLINKS_OFF);
    ngx_conf_merge_ptr_value(conf->disable_symlinks_from,
                             prev->disable_symlinks_from, NULL);
#endif

    return NGX_CONF_OK;
}


static char *
ngx_http_core_listen(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_srv_conf_t *cscf = conf;

    ngx_str_t              *value, size;
    ngx_url_t               u;
    ngx_uint_t              n;
    ngx_http_listen_opt_t   lsopt;

    cscf->listen = 1;

    value = cf->args->elts;


	//解析listen指令第一个url参数
    ngx_memzero(&u, sizeof(ngx_url_t));

    u.url = value[1];
    u.listen = 1;
    u.default_port = 80;

    if (ngx_parse_url(cf->pool, &u) != NGX_OK) {
        if (u.err) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s in \"%V\" of the \"listen\" directive", u.err, &u.url);
        }

        return NGX_CONF_ERROR;
    }

	//根据第一个参数解析结果及后续参数设置lsopt相关值
    ngx_memzero(&lsopt, sizeof(ngx_http_listen_opt_t));
    ngx_memcpy(&lsopt.u.sockaddr, u.sockaddr, u.socklen);

    lsopt.socklen = u.socklen;
    lsopt.backlog = NGX_LISTEN_BACKLOG;
    lsopt.rcvbuf = -1;
    lsopt.sndbuf = -1;
#if (NGX_HAVE_SETFIB)
    lsopt.setfib = -1;
#endif
#if (NGX_HAVE_TCP_FASTOPEN)
    lsopt.fastopen = -1;
#endif
    lsopt.wildcard = u.wildcard;
#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    lsopt.ipv6only = 1;
#endif

    (void) ngx_sock_ntop(&lsopt.u.sockaddr, lsopt.socklen, lsopt.addr, NGX_SOCKADDR_STRLEN, 1);

    for (n = 2; n < cf->args->nelts; n++) {
        if (ngx_strcmp(value[n].data, "default_server") == 0 || ngx_strcmp(value[n].data, "default") == 0)
        {
            lsopt.default_server = 1;
            continue;
        }

        if (ngx_strcmp(value[n].data, "bind") == 0)
		{
            lsopt.set = 1;
            lsopt.bind = 1;
            continue;
        }

#if (NGX_HAVE_SETFIB)
        if (ngx_strncmp(value[n].data, "setfib=", 7) == 0) 
		{
            lsopt.setfib = ngx_atoi(value[n].data + 7, value[n].len - 7);
            lsopt.set = 1;
            lsopt.bind = 1;

            if (lsopt.setfib == NGX_ERROR)
			{
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid setfib \"%V\"", &value[n]);
                return NGX_CONF_ERROR;
            }

            continue;
        }
#endif

#if (NGX_HAVE_TCP_FASTOPEN)
        if (ngx_strncmp(value[n].data, "fastopen=", 9) == 0) 
		{
            lsopt.fastopen = ngx_atoi(value[n].data + 9, value[n].len - 9);
            lsopt.set = 1;
            lsopt.bind = 1;

            if (lsopt.fastopen == NGX_ERROR) 
			{
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid fastopen \"%V\"", &value[n]);
                return NGX_CONF_ERROR;
            }

            continue;
        }
#endif

        if (ngx_strncmp(value[n].data, "backlog=", 8) == 0) 
		{
            lsopt.backlog = ngx_atoi(value[n].data + 8, value[n].len - 8);
            lsopt.set = 1;
            lsopt.bind = 1;

            if (lsopt.backlog == NGX_ERROR || lsopt.backlog == 0) 
			{
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid backlog \"%V\"", &value[n]);
                return NGX_CONF_ERROR;
            }

            continue;
        }

        if (ngx_strncmp(value[n].data, "rcvbuf=", 7) == 0) 
		{
            size.len = value[n].len - 7;
            size.data = value[n].data + 7;

            lsopt.rcvbuf = ngx_parse_size(&size);
            lsopt.set = 1;
            lsopt.bind = 1;

            if (lsopt.rcvbuf == NGX_ERROR)
			{
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid rcvbuf \"%V\"", &value[n]);
                return NGX_CONF_ERROR;
            }

            continue;
        }

        if (ngx_strncmp(value[n].data, "sndbuf=", 7) == 0)
		{
            size.len = value[n].len - 7;
            size.data = value[n].data + 7;

            lsopt.sndbuf = ngx_parse_size(&size);
            lsopt.set = 1;
            lsopt.bind = 1;

            if (lsopt.sndbuf == NGX_ERROR)
			{
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid sndbuf \"%V\"", &value[n]);
                return NGX_CONF_ERROR;
            }

            continue;
        }

        if (ngx_strncmp(value[n].data, "accept_filter=", 14) == 0) {
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
            lsopt.accept_filter = (char *) &value[n].data[14];
            lsopt.set = 1;
            lsopt.bind = 1;
#else
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "accept filters \"%V\" are not supported "
				"on this platform, ignored", &value[n]);
#endif
            continue;
        }

        if (ngx_strcmp(value[n].data, "deferred") == 0) 
		{
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
            lsopt.deferred_accept = 1;
            lsopt.set = 1;
            lsopt.bind = 1;
#else
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the deferred accept is not supported on this platform, ignored");
#endif
            continue;
        }

        if (ngx_strncmp(value[n].data, "ipv6only=o", 10) == 0) 
		{
#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
            struct sockaddr  *sa;

            sa = &lsopt.u.sockaddr;

            if (sa->sa_family == AF_INET6) 
			{

                if (ngx_strcmp(&value[n].data[10], "n") == 0) {
                    lsopt.ipv6only = 1;

                } else if (ngx_strcmp(&value[n].data[10], "ff") == 0) {
                    lsopt.ipv6only = 0;

                } else {
                    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                       "invalid ipv6only flags \"%s\"",
                                       &value[n].data[9]);
                    return NGX_CONF_ERROR;
                }

                lsopt.set = 1;
                lsopt.bind = 1;

            } else {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "ipv6only is not supported "
                                   "on addr \"%s\", ignored", lsopt.addr);
            }

            continue;
#else
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "ipv6only is not supported on this platform");
            return NGX_CONF_ERROR;
#endif
        }

        if (ngx_strcmp(value[n].data, "reuseport") == 0) 
		{
#if (NGX_HAVE_REUSEPORT)
            lsopt.reuseport = 1;
            lsopt.set = 1;
            lsopt.bind = 1;
#else
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "reuseport is not supported on this platform, ignored");
#endif
            continue;
        }

        if (ngx_strcmp(value[n].data, "ssl") == 0) 
		{
#if (NGX_HTTP_SSL)
            lsopt.ssl = 1;
            continue;
#else
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the \"ssl\" parameter requires ngx_http_ssl_module");
            return NGX_CONF_ERROR;
#endif
        }

        if (ngx_strcmp(value[n].data, "http2") == 0) 
		{
#if (NGX_HTTP_V2)
            lsopt.http2 = 1;
            continue;
#else
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the \"http2\" parameter requires ngx_http_v2_module");
            return NGX_CONF_ERROR;
#endif
        }

        if (ngx_strcmp(value[n].data, "spdy") == 0)
		{
            ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "invalid parameter \"spdy\": ngx_http_spdy_module was superseded by ngx_http_v2_module");
            continue;
        }

        if (ngx_strncmp(value[n].data, "so_keepalive=", 13) == 0)
		{

            if (ngx_strcmp(&value[n].data[13], "on") == 0) {
                lsopt.so_keepalive = 1;

            } 
			else if (ngx_strcmp(&value[n].data[13], "off") == 0) {
                lsopt.so_keepalive = 2;

            } 
			else 
			{

#if (NGX_HAVE_KEEPALIVE_TUNABLE)
                u_char     *p, *end;
                ngx_str_t   s;

                end = value[n].data + value[n].len;
                s.data = value[n].data + 13;

                p = ngx_strlchr(s.data, end, ':');
                if (p == NULL) 
				{
                    p = end;
                }

                if (p > s.data) 
				{
                    s.len = p - s.data;

                    lsopt.tcp_keepidle = ngx_parse_time(&s, 1);
                    if (lsopt.tcp_keepidle == (time_t) NGX_ERROR) 
					{
                        goto invalid_so_keepalive;
                    }
                }

                s.data = (p < end) ? (p + 1) : end;

                p = ngx_strlchr(s.data, end, ':');
                if (p == NULL) 
				{
                    p = end;
                }

                if (p > s.data) 
				{
                    s.len = p - s.data;

                    lsopt.tcp_keepintvl = ngx_parse_time(&s, 1);
                    if (lsopt.tcp_keepintvl == (time_t) NGX_ERROR)
					{
                        goto invalid_so_keepalive;
                    }
                }

                s.data = (p < end) ? (p + 1) : end;

                if (s.data < end) {
                    s.len = end - s.data;

                    lsopt.tcp_keepcnt = ngx_atoi(s.data, s.len);
                    if (lsopt.tcp_keepcnt == NGX_ERROR) {
                        goto invalid_so_keepalive;
                    }
                }

                if (lsopt.tcp_keepidle == 0 && lsopt.tcp_keepintvl == 0
                    && lsopt.tcp_keepcnt == 0)
                {
                    goto invalid_so_keepalive;
                }

                lsopt.so_keepalive = 1;

#else

                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the \"so_keepalive\" parameter accepts only \"on\" or \"off\" on this platform");
                return NGX_CONF_ERROR;

#endif
            }

            lsopt.set = 1;
            lsopt.bind = 1;

            continue;

#if (NGX_HAVE_KEEPALIVE_TUNABLE)
        invalid_so_keepalive:

            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid so_keepalive value: \"%s\"", &value[n].data[13]);
            return NGX_CONF_ERROR;
#endif
        }

        if (ngx_strcmp(value[n].data, "proxy_protocol") == 0) 
		{
            lsopt.proxy_protocol = 1;
            continue;
        }

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid parameter \"%V\"", &value[n]);
        return NGX_CONF_ERROR;
    }

    if (ngx_http_add_listen(cf, cscf, &lsopt) == NGX_OK) {
        return NGX_CONF_OK;
    }

    return NGX_CONF_ERROR;
}


static char *
ngx_http_core_server_name(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_srv_conf_t *cscf = conf;

    u_char                   ch;
    ngx_str_t               *value;
    ngx_uint_t               i;
    ngx_http_server_name_t  *sn;

    value = cf->args->elts;

    for (i = 1; i < cf->args->nelts; i++) 
	{

        ch = value[i].data[0];

        if ((ch == '*' && (value[i].len < 3 || value[i].data[1] != '.')) || (ch == '.' && value[i].len < 2))
        {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "server name \"%V\" is invalid", &value[i]);
            return NGX_CONF_ERROR;
        }

        if (ngx_strchr(value[i].data, '/'))
		{
            ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "server name \"%V\" has suspicious symbols", &value[i]);
        }

        sn = ngx_array_push(&cscf->server_names);
        if (sn == NULL) 
		{
            return NGX_CONF_ERROR;
        }

#if (NGX_PCRE)
        sn->regex = NULL;
#endif
        sn->server = cscf;

        if (ngx_strcasecmp(value[i].data, (u_char *) "$hostname") == 0)
		{
            sn->name = cf->cycle->hostname;

        }
		else 
        {
            sn->name = value[i];
        }

        if (value[i].data[0] != '~') 
		{
            ngx_strlow(sn->name.data, sn->name.data, sn->name.len);
            continue;
        }

#if (NGX_PCRE)
        {
        u_char               *p;
        ngx_regex_compile_t   rc;
        u_char                errstr[NGX_MAX_CONF_ERRSTR];

        if (value[i].len == 1) 
		{
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "empty regex in server name \"%V\"", &value[i]);
            return NGX_CONF_ERROR;
        }

        value[i].len--;
        value[i].data++;

        ngx_memzero(&rc, sizeof(ngx_regex_compile_t));

        rc.pattern = value[i];
        rc.err.len = NGX_MAX_CONF_ERRSTR;
        rc.err.data = errstr;

        for (p = value[i].data; p < value[i].data + value[i].len; p++) {
            if (*p >= 'A' && *p <= 'Z') {
                rc.options = NGX_REGEX_CASELESS;
                break;
            }
        }

        sn->regex = ngx_http_regex_compile(cf, &rc);
        if (sn->regex == NULL) 
		{
            return NGX_CONF_ERROR;
        }

        sn->name = value[i];
        cscf->captures = (rc.captures > 0);
        }
#else
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "using regex \"%V\" requires PCRE library", &value[i]);

        return NGX_CONF_ERROR;
#endif
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_core_root(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf = conf;

    ngx_str_t                  *value;
    ngx_int_t                   alias;
    ngx_uint_t                  n;
    ngx_http_script_compile_t   sc;

    alias = (cmd->name.len == sizeof("alias") - 1) ? 1 : 0;

    if (clcf->root.data) 
	{

        if ((clcf->alias != 0) == alias)
		{
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "\"%V\" directive is duplicate", &cmd->name);
        } 
		else 
		{
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "\"%V\" directive is duplicate, "
				"\"%s\" directive was specified earlier", &cmd->name, clcf->alias ? "alias" : "root");
        }

        return NGX_CONF_ERROR;
    }

    if (clcf->named && alias) 
	{
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the \"alias\" directive cannot be used inside the named location");
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;

    if (ngx_strstr(value[1].data, "$document_root") || ngx_strstr(value[1].data, "${document_root}"))
    {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the $document_root variable cannot be used "
				"in the \"%V\" directive", &cmd->name);

        return NGX_CONF_ERROR;
    }

    if (ngx_strstr(value[1].data, "$realpath_root") || ngx_strstr(value[1].data, "${realpath_root}"))
    {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the $realpath_root variable cannot be used "
			"in the \"%V\" directive", &cmd->name);

        return NGX_CONF_ERROR;
    }

    clcf->alias = alias ? clcf->name.len : 0;
    clcf->root = value[1];

	/*去掉root字符串末端的'/'字符*/
    if (!alias && clcf->root.len > 0 && clcf->root.data[clcf->root.len - 1] == '/')
    {
        clcf->root.len--;
    }

    if (clcf->root.data[0] != '$')
	{
        if (ngx_conf_full_name(cf->cycle, &clcf->root, 0) != NGX_OK) 
		{
            return NGX_CONF_ERROR;
        }
    }

    n = ngx_http_script_variables_count(&clcf->root);

    ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));
    sc.variables = n;

#if (NGX_PCRE)
    if (alias && clcf->regex) 
	{
        clcf->alias = NGX_MAX_SIZE_T_VALUE;
        n = 1;
    }
#endif

    if (n) 
	{
        sc.cf = cf;
        sc.source = &clcf->root;
        sc.lengths = &clcf->root_lengths;
        sc.values = &clcf->root_values;
        sc.complete_lengths = 1;
        sc.complete_values = 1;

        if (ngx_http_script_compile(&sc) != NGX_OK)
		{
            return NGX_CONF_ERROR;
        }
    }

    return NGX_CONF_OK;
}


static ngx_http_method_name_t  ngx_methods_names[] = {
   { (u_char *) "GET",       (uint32_t) ~NGX_HTTP_GET },
   { (u_char *) "HEAD",      (uint32_t) ~NGX_HTTP_HEAD },
   { (u_char *) "POST",      (uint32_t) ~NGX_HTTP_POST },
   { (u_char *) "PUT",       (uint32_t) ~NGX_HTTP_PUT },
   { (u_char *) "DELETE",    (uint32_t) ~NGX_HTTP_DELETE },
   { (u_char *) "MKCOL",     (uint32_t) ~NGX_HTTP_MKCOL },
   { (u_char *) "COPY",      (uint32_t) ~NGX_HTTP_COPY },
   { (u_char *) "MOVE",      (uint32_t) ~NGX_HTTP_MOVE },
   { (u_char *) "OPTIONS",   (uint32_t) ~NGX_HTTP_OPTIONS },
   { (u_char *) "PROPFIND",  (uint32_t) ~NGX_HTTP_PROPFIND },
   { (u_char *) "PROPPATCH", (uint32_t) ~NGX_HTTP_PROPPATCH },
   { (u_char *) "LOCK",      (uint32_t) ~NGX_HTTP_LOCK },
   { (u_char *) "UNLOCK",    (uint32_t) ~NGX_HTTP_UNLOCK },
   { (u_char *) "PATCH",     (uint32_t) ~NGX_HTTP_PATCH },
   { NULL, 0 }
};


static char *
ngx_http_core_limit_except(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *pclcf = conf;

    char                      *rv;
    void                      *mconf;
    ngx_str_t                 *value;
    ngx_uint_t                 i;
    ngx_conf_t                 save;
    ngx_http_module_t         *module;
    ngx_http_conf_ctx_t       *ctx, *pctx;
    ngx_http_method_name_t    *name;
    ngx_http_core_loc_conf_t  *clcf;

    if (pclcf->limit_except) {
        return "duplicate";
    }

    pclcf->limit_except = 0xffffffff;

    value = cf->args->elts;

    for (i = 1; i < cf->args->nelts; i++) {
        for (name = ngx_methods_names; name->name; name++) {

            if (ngx_strcasecmp(value[i].data, name->name) == 0) {
                pclcf->limit_except &= name->method;
                goto next;
            }
        }

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid method \"%V\"", &value[i]);
        return NGX_CONF_ERROR;

    next:
        continue;
    }

    if (!(pclcf->limit_except & NGX_HTTP_GET)) {  //允许GET方法就意味着也允许HEAD方法
        pclcf->limit_except &= (uint32_t) ~NGX_HTTP_HEAD;
    }

    ctx = ngx_pcalloc(cf->pool, sizeof(ngx_http_conf_ctx_t));
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    pctx = cf->ctx;
    ctx->main_conf = pctx->main_conf;
    ctx->srv_conf = pctx->srv_conf;

    ctx->loc_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_http_max_module);
    if (ctx->loc_conf == NULL) {
        return NGX_CONF_ERROR;
    }

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->type != NGX_HTTP_MODULE) {
            continue;
        }

        module = ngx_modules[i]->ctx;

        if (module->create_loc_conf) {

            mconf = module->create_loc_conf(cf);
            if (mconf == NULL) {
                 return NGX_CONF_ERROR;
            }

            ctx->loc_conf[ngx_modules[i]->ctx_index] = mconf;
        }
    }


    clcf = ctx->loc_conf[ngx_http_core_module.ctx_index];  //获取ngx_http_core_module模块在limite_execpet{}块内的loc配置
    pclcf->limit_except_loc_conf = ctx->loc_conf;
    clcf->loc_conf = ctx->loc_conf;
    clcf->name = pclcf->name;
    clcf->noname = 1;
    clcf->lmt_excpt = 1;

    if (ngx_http_add_location(cf, &pclcf->locations, clcf) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    save = *cf;
    cf->ctx = ctx;
    cf->cmd_type = NGX_HTTP_LMT_CONF;

    rv = ngx_conf_parse(cf, NULL);

    *cf = save;

    return rv;
}


static char *
ngx_http_core_set_aio(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf = conf;

    ngx_str_t  *value;

    if (clcf->aio != NGX_CONF_UNSET) 
	{
        return "is duplicate";
    }

#if (NGX_THREADS)
    clcf->thread_pool = NULL;
    clcf->thread_pool_value = NULL;
#endif

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "off") == 0) 
	{
        clcf->aio = NGX_HTTP_AIO_OFF;
        return NGX_CONF_OK;
    }

    if (ngx_strcmp(value[1].data, "on") == 0) 
	{
#if (NGX_HAVE_FILE_AIO)
        clcf->aio = NGX_HTTP_AIO_ON;
        return NGX_CONF_OK;
#else
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "\"aio on\" is unsupported on this platform");
        return NGX_CONF_ERROR;
#endif
    }

#if (NGX_HAVE_AIO_SENDFILE)

    if (ngx_strcmp(value[1].data, "sendfile") == 0) {
        clcf->aio = NGX_HTTP_AIO_ON;

        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "the \"sendfile\" parameter of "
                           "the \"aio\" directive is deprecated");
        return NGX_CONF_OK;
    }

#endif

    if (ngx_strncmp(value[1].data, "threads", 7) == 0
        && (value[1].len == 7 || value[1].data[7] == '='))
    {
#if (NGX_THREADS)
        ngx_str_t                          name;
        ngx_thread_pool_t                 *tp;
        ngx_http_complex_value_t           cv;
        ngx_http_compile_complex_value_t   ccv;

        clcf->aio = NGX_HTTP_AIO_THREADS;

        if (value[1].len >= 8) {
            name.len = value[1].len - 8;
            name.data = value[1].data + 8;

            ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

            ccv.cf = cf;
            ccv.value = &name;
            ccv.complex_value = &cv;

            if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
                return NGX_CONF_ERROR;
            }

            if (cv.lengths != NULL) {
                clcf->thread_pool_value = ngx_palloc(cf->pool,
                                    sizeof(ngx_http_complex_value_t));
                if (clcf->thread_pool_value == NULL) {
                    return NGX_CONF_ERROR;
                }

                *clcf->thread_pool_value = cv;

                return NGX_CONF_OK;
            }

            tp = ngx_thread_pool_add(cf, &name);

        } else {
            tp = ngx_thread_pool_add(cf, NULL);
        }

        if (tp == NULL) {
            return NGX_CONF_ERROR;
        }

        clcf->thread_pool = tp;

        return NGX_CONF_OK;
#else
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"aio threads\" "
                           "is unsupported on this platform");
        return NGX_CONF_ERROR;
#endif
    }

    return "invalid value";
}


static char *
ngx_http_core_directio(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf = conf;

    ngx_str_t  *value;

    if (clcf->directio != NGX_CONF_UNSET)
	{
        return "is duplicate";
    }

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "off") == 0) 
	{
        clcf->directio = NGX_OPEN_FILE_DIRECTIO_OFF;
        return NGX_CONF_OK;
    }

    clcf->directio = ngx_parse_offset(&value[1]);
    if (clcf->directio == (off_t) NGX_ERROR) 
	{
        return "invalid value";
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_core_error_page(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf = conf;

    u_char                            *p;
    ngx_int_t                          overwrite;
    ngx_str_t                         *value, uri, args;
    ngx_uint_t                         i, n;
    ngx_http_err_page_t               *err;
    ngx_http_complex_value_t           cv;
    ngx_http_compile_complex_value_t   ccv;

    if (clcf->error_pages == NULL) {
        clcf->error_pages = ngx_array_create(cf->pool, 4,
                                             sizeof(ngx_http_err_page_t));
        if (clcf->error_pages == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    value = cf->args->elts;

    i = cf->args->nelts - 2;

    if (value[i].data[0] == '=') {
        if (i == 1) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid value \"%V\"", &value[i]);
            return NGX_CONF_ERROR;
        }

        if (value[i].len > 1) {
            overwrite = ngx_atoi(&value[i].data[1], value[i].len - 1);

            if (overwrite == NGX_ERROR) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "invalid value \"%V\"", &value[i]);
                return NGX_CONF_ERROR;
            }

        } else {
            overwrite = 0;
        }

        n = 2;

    } else {
        overwrite = -1;
        n = 1;
    }

    uri = value[cf->args->nelts - 1];

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &uri;
    ccv.complex_value = &cv;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    ngx_str_null(&args);

    if (cv.lengths == NULL && uri.len && uri.data[0] == '/') {
        p = (u_char *) ngx_strchr(uri.data, '?');

        if (p) {
            cv.value.len = p - uri.data;
            cv.value.data = uri.data;
            p++;
            args.len = (uri.data + uri.len) - p;
            args.data = p;
        }
    }

    for (i = 1; i < cf->args->nelts - n; i++) {
        err = ngx_array_push(clcf->error_pages);
        if (err == NULL) {
            return NGX_CONF_ERROR;
        }

        err->status = ngx_atoi(value[i].data, value[i].len);

        if (err->status == NGX_ERROR || err->status == 499) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid value \"%V\"", &value[i]);
            return NGX_CONF_ERROR;
        }

        if (err->status < 300 || err->status > 599) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "value \"%V\" must be between 300 and 599",
                               &value[i]);
            return NGX_CONF_ERROR;
        }

        err->overwrite = overwrite;

        if (overwrite == -1) {
            switch (err->status) {
                case NGX_HTTP_TO_HTTPS:
                case NGX_HTTPS_CERT_ERROR:
                case NGX_HTTPS_NO_CERT:
                    err->overwrite = NGX_HTTP_BAD_REQUEST;
                default:
                    break;
            }
        }

        err->value = cv;
        err->args = args;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_core_try_files(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf = conf;

    ngx_str_t                  *value;
    ngx_int_t                   code;
    ngx_uint_t                  i, n;
    ngx_http_try_file_t        *tf;
    ngx_http_script_compile_t   sc;
    ngx_http_core_main_conf_t  *cmcf;

    if (clcf->try_files) 
	{
        return "is duplicate";
    }

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    cmcf->try_files = 1;

    tf = ngx_pcalloc(cf->pool, cf->args->nelts * sizeof(ngx_http_try_file_t));
    if (tf == NULL) 
	{
        return NGX_CONF_ERROR;
    }

    clcf->try_files = tf;

    value = cf->args->elts;

    for (i = 0; i < cf->args->nelts - 1; i++) 
	{

        tf[i].name = value[i + 1];

        if (tf[i].name.len > 0
            && tf[i].name.data[tf[i].name.len - 1] == '/'
            && i + 2 < cf->args->nelts)
        {
            tf[i].test_dir = 1;
            tf[i].name.len--;
            tf[i].name.data[tf[i].name.len] = '\0';
        }

        n = ngx_http_script_variables_count(&tf[i].name);

        if (n) {
            ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

            sc.cf = cf;
            sc.source = &tf[i].name;
            sc.lengths = &tf[i].lengths;
            sc.values = &tf[i].values;
            sc.variables = n;
            sc.complete_lengths = 1;
            sc.complete_values = 1;

            if (ngx_http_script_compile(&sc) != NGX_OK) {
                return NGX_CONF_ERROR;
            }

        } else {
            /* add trailing '\0' to length */
            tf[i].name.len++;
        }
    }

    if (tf[i - 1].name.data[0] == '=') {

        code = ngx_atoi(tf[i - 1].name.data + 1, tf[i - 1].name.len - 2);

        if (code == NGX_ERROR || code > 999) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid code \"%*s\"",
                               tf[i - 1].name.len - 1, tf[i - 1].name.data);
            return NGX_CONF_ERROR;
        }

        tf[i].code = code;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_core_open_file_cache(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf = conf;

    time_t       inactive;
    ngx_str_t   *value, s;
    ngx_int_t    max;
    ngx_uint_t   i;

    if (clcf->open_file_cache != NGX_CONF_UNSET_PTR) 
	{
        return "is duplicate";
    }

    value = cf->args->elts;

    max = 0;
    inactive = 60;

    for (i = 1; i < cf->args->nelts; i++) 
	{

        if (ngx_strncmp(value[i].data, "max=", 4) == 0) 
		{

            max = ngx_atoi(value[i].data + 4, value[i].len - 4);
            if (max <= 0) 
			{
                goto failed;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "inactive=", 9) == 0) 
		{

            s.len = value[i].len - 9;
            s.data = value[i].data + 9;

            inactive = ngx_parse_time(&s, 1);
            if (inactive == (time_t) NGX_ERROR) 
			{
                goto failed;
            }

            continue;
        }

        if (ngx_strcmp(value[i].data, "off") == 0) 
		{
            clcf->open_file_cache = NULL;
			
            continue;
        }

    failed:

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid \"open_file_cache\" parameter \"%V\"", &value[i]);
        return NGX_CONF_ERROR;
    }

    if (clcf->open_file_cache == NULL) 
	{
        return NGX_CONF_OK;
    }

    if (max == 0) 
	{
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "\"open_file_cache\" must have the \"max\" parameter");
        return NGX_CONF_ERROR;
    }

    clcf->open_file_cache = ngx_open_file_cache_init(cf->pool, max, inactive);
    if (clcf->open_file_cache) 
	{
        return NGX_CONF_OK;
    }

    return NGX_CONF_ERROR;
}


static char *
ngx_http_core_error_log(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf = conf;

    return ngx_log_set_log(cf, &clcf->error_log);
}


static char *
ngx_http_core_keepalive(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf = conf;

    ngx_str_t  *value;

    if (clcf->keepalive_timeout != NGX_CONF_UNSET_MSEC)
	{
        return "is duplicate";
    }

    value = cf->args->elts;

    clcf->keepalive_timeout = ngx_parse_time(&value[1], 0);

    if (clcf->keepalive_timeout == (ngx_msec_t) NGX_ERROR) 
	{
        return "invalid value";
    }

    if (cf->args->nelts == 2) 
	{
        return NGX_CONF_OK;
    }

    clcf->keepalive_header = ngx_parse_time(&value[2], 1);

    if (clcf->keepalive_header == (time_t) NGX_ERROR) 
	{
        return "invalid value";
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_core_internal(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf = conf;

    if (clcf->internal != NGX_CONF_UNSET) {
        return "is duplicate";
    }

    clcf->internal = 1;

    return NGX_CONF_OK;
}


static char *
ngx_http_core_resolver(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf = conf;

    ngx_str_t  *value;

    if (clcf->resolver) 
	{
        return "is duplicate";
    }

    value = cf->args->elts;

    clcf->resolver = ngx_resolver_create(cf, &value[1], cf->args->nelts - 1);
    if (clcf->resolver == NULL) 
	{
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


#if (NGX_HTTP_GZIP)

static char *
ngx_http_gzip_disable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf = conf;

#if (NGX_PCRE)

    ngx_str_t            *value;
    ngx_uint_t            i;
    ngx_regex_elt_t      *re;
    ngx_regex_compile_t   rc;
    u_char                errstr[NGX_MAX_CONF_ERRSTR];

    if (clcf->gzip_disable == NGX_CONF_UNSET_PTR) {
        clcf->gzip_disable = ngx_array_create(cf->pool, 2,
                                              sizeof(ngx_regex_elt_t));
        if (clcf->gzip_disable == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    value = cf->args->elts;

    ngx_memzero(&rc, sizeof(ngx_regex_compile_t));

    rc.pool = cf->pool;
    rc.err.len = NGX_MAX_CONF_ERRSTR;
    rc.err.data = errstr;

    for (i = 1; i < cf->args->nelts; i++) {

        if (ngx_strcmp(value[i].data, "msie6") == 0) {
            clcf->gzip_disable_msie6 = 1;
            continue;
        }

#if (NGX_HTTP_DEGRADATION)

        if (ngx_strcmp(value[i].data, "degradation") == 0) {
            clcf->gzip_disable_degradation = 1;
            continue;
        }

#endif

        re = ngx_array_push(clcf->gzip_disable);
        if (re == NULL) {
            return NGX_CONF_ERROR;
        }

        rc.pattern = value[i];
        rc.options = NGX_REGEX_CASELESS;

        if (ngx_regex_compile(&rc) != NGX_OK) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%V", &rc.err);
            return NGX_CONF_ERROR;
        }

        re->regex = rc.regex;
        re->name = value[i].data;
    }

    return NGX_CONF_OK;

#else
    ngx_str_t   *value;
    ngx_uint_t   i;

    value = cf->args->elts;

    for (i = 1; i < cf->args->nelts; i++) {
        if (ngx_strcmp(value[i].data, "msie6") == 0) {
            clcf->gzip_disable_msie6 = 1;
            continue;
        }

#if (NGX_HTTP_DEGRADATION)

        if (ngx_strcmp(value[i].data, "degradation") == 0) {
            clcf->gzip_disable_degradation = 1;
            continue;
        }

#endif

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "without PCRE library \"gzip_disable\" supports "
                           "builtin \"msie6\" and \"degradation\" mask only");

        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;

#endif
}

#endif


#if (NGX_HAVE_OPENAT)

static char *
ngx_http_disable_symlinks(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf = conf;

    ngx_str_t                         *value;
    ngx_uint_t                         i;
    ngx_http_compile_complex_value_t   ccv;

    if (clcf->disable_symlinks != NGX_CONF_UNSET_UINT) {
        return "is duplicate";
    }

    value = cf->args->elts;

    for (i = 1; i < cf->args->nelts; i++) {

        if (ngx_strcmp(value[i].data, "off") == 0) {
            clcf->disable_symlinks = NGX_DISABLE_SYMLINKS_OFF;
            continue;
        }

        if (ngx_strcmp(value[i].data, "if_not_owner") == 0) {
            clcf->disable_symlinks = NGX_DISABLE_SYMLINKS_NOTOWNER;
            continue;
        }

        if (ngx_strcmp(value[i].data, "on") == 0) {
            clcf->disable_symlinks = NGX_DISABLE_SYMLINKS_ON;
            continue;
        }

        if (ngx_strncmp(value[i].data, "from=", 5) == 0) {
            value[i].len -= 5;
            value[i].data += 5;

            ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

            ccv.cf = cf;
            ccv.value = &value[i];
            ccv.complex_value = ngx_palloc(cf->pool,
                                           sizeof(ngx_http_complex_value_t));
            if (ccv.complex_value == NULL) {
                return NGX_CONF_ERROR;
            }

            if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
                return NGX_CONF_ERROR;
            }

            clcf->disable_symlinks_from = ccv.complex_value;

            continue;
        }

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid parameter \"%V\"", &value[i]);
        return NGX_CONF_ERROR;
    }

    if (clcf->disable_symlinks == NGX_CONF_UNSET_UINT) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"%V\" must have \"off\", \"on\" "
                           "or \"if_not_owner\" parameter",
                           &cmd->name);
        return NGX_CONF_ERROR;
    }

    if (cf->args->nelts == 2) {
        clcf->disable_symlinks_from = NULL;
        return NGX_CONF_OK;
    }

    if (clcf->disable_symlinks_from == NGX_CONF_UNSET_PTR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "duplicate parameters \"%V %V\"",
                           &value[1], &value[2]);
        return NGX_CONF_ERROR;
    }

    if (clcf->disable_symlinks == NGX_DISABLE_SYMLINKS_OFF) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"from=\" cannot be used with \"off\" parameter");
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

#endif


static char *
ngx_http_core_lowat_check(ngx_conf_t *cf, void *post, void *data)
{
#if (NGX_FREEBSD)
    ssize_t *np = data;

    if ((u_long) *np >= ngx_freebsd_net_inet_tcp_sendspace) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"send_lowat\" must be less than %d "
                           "(sysctl net.inet.tcp.sendspace)",
                           ngx_freebsd_net_inet_tcp_sendspace);

        return NGX_CONF_ERROR;
    }

#elif !(NGX_HAVE_SO_SNDLOWAT)
    ssize_t *np = data;

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                       "\"send_lowat\" is not supported, ignored");

    *np = 0;

#endif

    return NGX_CONF_OK;
}


static char *
ngx_http_core_pool_size(ngx_conf_t *cf, void *post, void *data)
{
    size_t *sp = data;

    if (*sp < NGX_MIN_POOL_SIZE) 
	{
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the pool size must be no less than %uz", NGX_MIN_POOL_SIZE);
        return NGX_CONF_ERROR;
    }

    if (*sp % NGX_POOL_ALIGNMENT) 
	{
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the pool size must be a multiple of %uz", NGX_POOL_ALIGNMENT);
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}
