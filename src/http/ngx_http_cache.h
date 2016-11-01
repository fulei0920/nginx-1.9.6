
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_CACHE_H_INCLUDED_
#define _NGX_HTTP_CACHE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#define NGX_HTTP_CACHE_MISS          1
#define NGX_HTTP_CACHE_BYPASS        2
#define NGX_HTTP_CACHE_EXPIRED       3
#define NGX_HTTP_CACHE_STALE         4
#define NGX_HTTP_CACHE_UPDATING      5
#define NGX_HTTP_CACHE_REVALIDATED   6
#define NGX_HTTP_CACHE_HIT           7
#define NGX_HTTP_CACHE_SCARCE        8

#define NGX_HTTP_CACHE_KEY_LEN       16
#define NGX_HTTP_CACHE_ETAG_LEN      42
#define NGX_HTTP_CACHE_VARY_LEN      42

#define NGX_HTTP_CACHE_VERSION       3


typedef struct {
    ngx_uint_t                       status;  	//响应码
    time_t                           valid;		//缓存时间
} ngx_http_cache_valid_t;

///保存每个缓存文件在内存中的描述信息。
//这些信息需要存储于共享内存中，以便多个 worker 进程共享。
//所以，为了提高利用率，此结构体多个字段使用了位域 (Bit field)，
//同时，缓存 key 中用作查询树键值( ngx_rbtree_key_t ) 的部分字节不再重复存储
typedef struct {
    ngx_rbtree_node_t                node;		//红黑树结点
    ngx_queue_t                      queue;		//LRU链表结点

    u_char                           key[NGX_HTTP_CACHE_KEY_LEN - sizeof(ngx_rbtree_key_t)];

    unsigned                         count:20;			//引用计数
    unsigned                         uses:10;			///已经被多少请求使用
    unsigned                         valid_msec:10;
    unsigned                         error:10;			//
    unsigned                         exists:1;			//是否存在对应的cache文件
    unsigned                         updating:1;		//是否在更新
    unsigned                         deleting:1;		///表示对应的缓存文件正在被删除
                                     /* 11 unused bits */

    ngx_file_uniq_t                  uniq;
    time_t                           expire;			//失效时间点
    time_t                           valid_sec;			//max-age?
    size_t                           body_start;		//body起始位置
    off_t                            fs_size;			//文件大小  //文件占用系统块的个数	
    ngx_msec_t                       lock_time;
} ngx_http_file_cache_node_t;

//每个http request对应的缓存条目的完整信息 
//(请求使用的缓存 file_cache 、缓存条目对应的缓存节点信息 node 、缓存文件 file 、key 值及其检验 crc32 等等) 
//都临时保存于ngx_http_cache_t (ngx_http_request_t->cache) 结构体中，
//这个结构体中的信息量基本上相当于ngx_http_file_cache_header_t 和 ngx_http_file_cache_node_t 的总和
struct ngx_http_cache_s 
{
    ngx_file_t                       file;
    ngx_array_t                      keys;
    uint32_t                         crc32;
    u_char                           key[NGX_HTTP_CACHE_KEY_LEN];
    u_char                           main[NGX_HTTP_CACHE_KEY_LEN];

    ngx_file_uniq_t                  uniq;
    time_t                           valid_sec;
    time_t                           last_modified;
    time_t                           date;

    ngx_str_t                        etag;
    ngx_str_t                        vary;
    u_char                           variant[NGX_HTTP_CACHE_KEY_LEN];

    size_t                           header_start;
    size_t                           body_start;	//httpbody相对http请求的偏移位置--http头的长度
    off_t                            length;		//文件大小
    off_t                            fs_size;		//文件占用系统块的个数	

    ngx_uint_t                       min_uses;   //响应被缓存的最小请求次数
    ngx_uint_t                       error;
    ngx_uint_t                       valid_msec;

    ngx_buf_t                       *buf;

    ngx_http_file_cache_t           *file_cache;	//请求对应的文件缓存管理节点
    ngx_http_file_cache_node_t      *node;    		///请求对应的缓存文件节点

#if (NGX_THREADS)
    ngx_thread_task_t               *thread_task;
#endif

    ngx_msec_t                       lock_timeout;
    ngx_msec_t                       lock_age;
    ngx_msec_t                       lock_time;
    ngx_msec_t                       wait_time;

    ngx_event_t                      wait_event;

    unsigned                         lock:1;
    unsigned                         waiting:1;

    unsigned                         updated:1;
    unsigned                         updating:1;
    unsigned                         exists:1;
    unsigned                         temp_file:1;
    unsigned                         reading:1;
    unsigned                         secondary:1;
};

//每个文件系统中的缓存文件都有固定的存储格式，其中 ngx_http_file_cache_header_t为包头结构，
//存储缓存文件的相关信息 (修改时间、缓存 key 的 crc32 值、和用于指明
//HTTP 响应包头和包体在缓存文件中偏移位置的字段等)：
//缓存文件格式 [ngx_http_file_cache_header_t]["\nKEY: "][orig_key]["\n"][header][body]
typedef struct {
    ngx_uint_t                       version;
    time_t                           valid_sec;
    time_t                           last_modified;
    time_t                           date;
    uint32_t                         crc32;
    u_short                          valid_msec;
    u_short                          header_start;
    u_short                          body_start;
    u_char                           etag_len;
    u_char                           etag[NGX_HTTP_CACHE_ETAG_LEN];
    u_char                           vary_len;
    u_char                           vary[NGX_HTTP_CACHE_VARY_LEN];
    u_char                           variant[NGX_HTTP_CACHE_KEY_LEN];
} ngx_http_file_cache_header_t;


typedef struct {
    ngx_rbtree_t                     rbtree;		//红黑树，用于快速查找key对应的文件信息(file_cache_node)
    ngx_rbtree_node_t                sentinel;		//红黑树NIL结点
    ngx_queue_t                      queue;			//LRU队列
    ngx_atomic_t                     cold;			//表示这个cache(对应目录下的文件)已经被cache loader进程加载完成
    ngx_atomic_t                     loading;		///表示cache loader进程正在加载这个cache(对应目录下的文件)
    off_t                            size;			///所有的缓存文件大小总和
} ngx_http_file_cache_sh_t;


struct ngx_http_file_cache_s {
    ngx_http_file_cache_sh_t        *sh;			//维护 LRU 队列和红黑树，以及缓存文件的当前状态 (是否正在从磁盘加载、当前缓存大小等)
    ngx_slab_pool_t                 *shpool;		///share pool

    ngx_path_t                      *path;				//缓存文件存放目录
    ngx_path_t                      *temp_path;

    off_t                            max_size;				//缓存数据的条目上限，由cache manager管理，超出则根据LRU策略删除
    size_t                           bsize;					//文件缓存目录所在文件系统的块大小

    time_t                           inactive;				//强制更新缓存时间，规定时间内没有访问则从内存删除

    ngx_uint_t                       files;					//统计cache loader进程每次迭代过程中加载的文件的数目

    ngx_uint_t                       loader_files;			//cache loader进程每次迭代加载文件的数目的最大值
    ngx_msec_t                       last;					//cache loader进程或者cache manager进程每次迭代过程开始的时间
    ngx_msec_t                       loader_sleep;			//cache loader进程每次迭代之间暂停的时间
    ngx_msec_t                       loader_threshold;		//cache loader进程每次迭代过程的持续时间的最大值

    ngx_shm_zone_t                  *shm_zone;		///存放key和缓存文件路径散列表的共享内存
};


ngx_int_t ngx_http_file_cache_new(ngx_http_request_t *r);
ngx_int_t ngx_http_file_cache_create(ngx_http_request_t *r);
void ngx_http_file_cache_create_key(ngx_http_request_t *r);
ngx_int_t ngx_http_file_cache_open(ngx_http_request_t *r);
ngx_int_t ngx_http_file_cache_set_header(ngx_http_request_t *r, u_char *buf);
void ngx_http_file_cache_update(ngx_http_request_t *r, ngx_temp_file_t *tf);
void ngx_http_file_cache_update_header(ngx_http_request_t *r);
ngx_int_t ngx_http_cache_send(ngx_http_request_t *);
void ngx_http_file_cache_free(ngx_http_cache_t *c, ngx_temp_file_t *tf);
time_t ngx_http_file_cache_valid(ngx_array_t *cache_valid, ngx_uint_t status);

char *ngx_http_file_cache_set_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_http_file_cache_valid_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


extern ngx_str_t  ngx_http_cache_status[];


#endif /* _NGX_HTTP_CACHE_H_INCLUDED_ */
