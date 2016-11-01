
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
    ngx_uint_t                       status;  	//��Ӧ��
    time_t                           valid;		//����ʱ��
} ngx_http_cache_valid_t;

///����ÿ�������ļ����ڴ��е�������Ϣ��
//��Щ��Ϣ��Ҫ�洢�ڹ����ڴ��У��Ա��� worker ���̹���
//���ԣ�Ϊ����������ʣ��˽ṹ�����ֶ�ʹ����λ�� (Bit field)��
//ͬʱ������ key ��������ѯ����ֵ( ngx_rbtree_key_t ) �Ĳ����ֽڲ����ظ��洢
typedef struct {
    ngx_rbtree_node_t                node;		//��������
    ngx_queue_t                      queue;		//LRU������

    u_char                           key[NGX_HTTP_CACHE_KEY_LEN - sizeof(ngx_rbtree_key_t)];

    unsigned                         count:20;			//���ü���
    unsigned                         uses:10;			///�Ѿ�����������ʹ��
    unsigned                         valid_msec:10;
    unsigned                         error:10;			//
    unsigned                         exists:1;			//�Ƿ���ڶ�Ӧ��cache�ļ�
    unsigned                         updating:1;		//�Ƿ��ڸ���
    unsigned                         deleting:1;		///��ʾ��Ӧ�Ļ����ļ����ڱ�ɾ��
                                     /* 11 unused bits */

    ngx_file_uniq_t                  uniq;
    time_t                           expire;			//ʧЧʱ���
    time_t                           valid_sec;			//max-age?
    size_t                           body_start;		//body��ʼλ��
    off_t                            fs_size;			//�ļ���С  //�ļ�ռ��ϵͳ��ĸ���	
    ngx_msec_t                       lock_time;
} ngx_http_file_cache_node_t;

//ÿ��http request��Ӧ�Ļ�����Ŀ��������Ϣ 
//(����ʹ�õĻ��� file_cache ��������Ŀ��Ӧ�Ļ���ڵ���Ϣ node �������ļ� file ��key ֵ������� crc32 �ȵ�) 
//����ʱ������ngx_http_cache_t (ngx_http_request_t->cache) �ṹ���У�
//����ṹ���е���Ϣ���������൱��ngx_http_file_cache_header_t �� ngx_http_file_cache_node_t ���ܺ�
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
    size_t                           body_start;	//httpbody���http�����ƫ��λ��--httpͷ�ĳ���
    off_t                            length;		//�ļ���С
    off_t                            fs_size;		//�ļ�ռ��ϵͳ��ĸ���	

    ngx_uint_t                       min_uses;   //��Ӧ���������С�������
    ngx_uint_t                       error;
    ngx_uint_t                       valid_msec;

    ngx_buf_t                       *buf;

    ngx_http_file_cache_t           *file_cache;	//�����Ӧ���ļ��������ڵ�
    ngx_http_file_cache_node_t      *node;    		///�����Ӧ�Ļ����ļ��ڵ�

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

//ÿ���ļ�ϵͳ�еĻ����ļ����й̶��Ĵ洢��ʽ������ ngx_http_file_cache_header_tΪ��ͷ�ṹ��
//�洢�����ļ��������Ϣ (�޸�ʱ�䡢���� key �� crc32 ֵ��������ָ��
//HTTP ��Ӧ��ͷ�Ͱ����ڻ����ļ���ƫ��λ�õ��ֶε�)��
//�����ļ���ʽ [ngx_http_file_cache_header_t]["\nKEY: "][orig_key]["\n"][header][body]
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
    ngx_rbtree_t                     rbtree;		//����������ڿ��ٲ���key��Ӧ���ļ���Ϣ(file_cache_node)
    ngx_rbtree_node_t                sentinel;		//�����NIL���
    ngx_queue_t                      queue;			//LRU����
    ngx_atomic_t                     cold;			//��ʾ���cache(��ӦĿ¼�µ��ļ�)�Ѿ���cache loader���̼������
    ngx_atomic_t                     loading;		///��ʾcache loader�������ڼ������cache(��ӦĿ¼�µ��ļ�)
    off_t                            size;			///���еĻ����ļ���С�ܺ�
} ngx_http_file_cache_sh_t;


struct ngx_http_file_cache_s {
    ngx_http_file_cache_sh_t        *sh;			//ά�� LRU ���кͺ�������Լ������ļ��ĵ�ǰ״̬ (�Ƿ����ڴӴ��̼��ء���ǰ�����С��)
    ngx_slab_pool_t                 *shpool;		///share pool

    ngx_path_t                      *path;				//�����ļ����Ŀ¼
    ngx_path_t                      *temp_path;

    off_t                            max_size;				//�������ݵ���Ŀ���ޣ���cache manager�������������LRU����ɾ��
    size_t                           bsize;					//�ļ�����Ŀ¼�����ļ�ϵͳ�Ŀ��С

    time_t                           inactive;				//ǿ�Ƹ��»���ʱ�䣬�涨ʱ����û�з�������ڴ�ɾ��

    ngx_uint_t                       files;					//ͳ��cache loader����ÿ�ε��������м��ص��ļ�����Ŀ

    ngx_uint_t                       loader_files;			//cache loader����ÿ�ε��������ļ�����Ŀ�����ֵ
    ngx_msec_t                       last;					//cache loader���̻���cache manager����ÿ�ε������̿�ʼ��ʱ��
    ngx_msec_t                       loader_sleep;			//cache loader����ÿ�ε���֮����ͣ��ʱ��
    ngx_msec_t                       loader_threshold;		//cache loader����ÿ�ε������̵ĳ���ʱ������ֵ

    ngx_shm_zone_t                  *shm_zone;		///���key�ͻ����ļ�·��ɢ�б�Ĺ����ڴ�
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
