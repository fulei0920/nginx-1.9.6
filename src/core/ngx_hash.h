
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HASH_H_INCLUDED_
#define _NGX_HASH_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct 
{
    void             *value;		//字符串key对应的value
    u_short           len;			//字符串key的长度
    u_char            name[1];		//字符串key
} ngx_hash_elt_t;


typedef struct 
{
    ngx_hash_elt_t  **buckets;
    ngx_uint_t        size;
} ngx_hash_t;


typedef struct 
{
    ngx_hash_t        hash;
    void             *value;
} ngx_hash_wildcard_t;


typedef struct 
{
    ngx_str_t         key;			//key
    ngx_uint_t        key_hash;		//hash key
    void             *value;		//value
} ngx_hash_key_t;


typedef ngx_uint_t (*ngx_hash_key_pt) (u_char *data, size_t len);


typedef struct {
    ngx_hash_t            hash;
    ngx_hash_wildcard_t  *wc_head;
    ngx_hash_wildcard_t  *wc_tail;
} ngx_hash_combined_t;


typedef struct 
{
    ngx_hash_t       *hash;				//hash构建完成后的查找表
    ngx_hash_key_pt   key;				//hash函数

    ngx_uint_t        max_size;			//冲突桶的最大个数
    ngx_uint_t        bucket_size;		//冲突桶的最大大小

    char             *name;
    ngx_pool_t       *pool;
    ngx_pool_t       *temp_pool;
} ngx_hash_init_t;


#define NGX_HASH_SMALL            1
#define NGX_HASH_LARGE            2

#define NGX_HASH_LARGE_ASIZE      16384
#define NGX_HASH_LARGE_HSIZE      10007

#define NGX_HASH_WILDCARD_KEY     1
#define NGX_HASH_READONLY_KEY     2



//构建hash表的辅助数据结构
//用于在构建hash表前，存储所有的需要构建的key
typedef struct 
{
	//将要构建的hash表的桶的个数。对于使用这个结构中包含的信息构建的三种类型的hash表都会使用此参数。
    ngx_uint_t        hsize;    	

	//构建这些hash表使用的pool
    ngx_pool_t       *pool;		
	//在构建这个类型以及最终的三个hash表过程中可能用到临时pool。该temp_pool可以在构建完成以后，被销毁掉。这里只是存放临时的一些内存消耗
    ngx_pool_t       *temp_pool;	

	//ngx_hash_key_t类型的动态数组，存放所有非通配符key的数组。
    ngx_array_t       keys;			
   	//key的name字符串的散列表
    //该值在调用的过程中用来保存和检测是否有冲突的key值，也就是是否有重复。
    ngx_array_t      *keys_hash;	

	//ngx_hash_key_t类型的动态数组，存放前向通配符key被处理完成以后的值。比如：“*.abc.com” 被处理完成以后，变成 “com.abc.” 被存放在此数组中。
    ngx_array_t       dns_wc_head;	
	//key的name字符串的散列表
	//该值在调用的过程中用来保存和检测是否有冲突的前向通配符的key值，也就是是否有重复。
    ngx_array_t      *dns_wc_head_hash;			

	//ngx_hash_key_t类型的动态数组，存放后向通配符key被处理完成以后的值。比如：“mail.xxx.*” 被处理完成以后，变成 “mail.xxx.” 被存放在此数组中。
    ngx_array_t       dns_wc_tail;	
	//key的name字符串的散列表
	//该值在调用的过程中用来保存和检测是否有冲突的后向通配符的key值，也就是是否有重复。
    ngx_array_t      *dns_wc_tail_hash;		
} ngx_hash_keys_arrays_t;


typedef struct 
{
    ngx_uint_t        hash;   		//hash value of key
    ngx_str_t         key;
    ngx_str_t         value;
    u_char           *lowcase_key;
} ngx_table_elt_t;


void *ngx_hash_find(ngx_hash_t *hash, ngx_uint_t key, u_char *name, size_t len);
void *ngx_hash_find_wc_head(ngx_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ngx_hash_find_wc_tail(ngx_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ngx_hash_find_combined(ngx_hash_combined_t *hash, ngx_uint_t key,
    u_char *name, size_t len);

ngx_int_t ngx_hash_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts);
ngx_int_t ngx_hash_wildcard_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts);

#define ngx_hash(key, c)   ((ngx_uint_t) key * 31 + c)
ngx_uint_t ngx_hash_key(u_char *data, size_t len);
ngx_uint_t ngx_hash_key_lc(u_char *data, size_t len);
ngx_uint_t ngx_hash_strlow(u_char *dst, u_char *src, size_t n);


ngx_int_t ngx_hash_keys_array_init(ngx_hash_keys_arrays_t *ha, ngx_uint_t type);
ngx_int_t ngx_hash_add_key(ngx_hash_keys_arrays_t *ha, ngx_str_t *key, void *value, ngx_uint_t flags);


#endif /* _NGX_HASH_H_INCLUDED_ */
