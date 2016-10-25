
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_SLAB_H_INCLUDED_
#define _NGX_SLAB_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_slab_page_s  ngx_slab_page_t;

struct ngx_slab_page_s 
{
    uintptr_t         slab;
    ngx_slab_page_t  *next;
    uintptr_t         prev;
};

//在共享内存中分配
typedef struct 
{
    ngx_shmtx_sh_t    lock;

    size_t            min_size;		/*最小分配单元  1 << min_shift */
    size_t            min_shift;   	/*最小分配单元对应的位移，默认值3*/

    ngx_slab_page_t  *pages;		/*页数组首地址*/
    ngx_slab_page_t  *last;			/*页数组末端地址*/
    ngx_slab_page_t   free;			/*空闲页链表*/

    u_char           *start;		/*可分配空间的起始地址*/
    u_char           *end;			/*内存块的结束地址*/ 

    ngx_shmtx_t       mutex;

    u_char           *log_ctx;
    u_char            zero;

    unsigned          log_nomem:1;

    void             *data;			//该内存块的私有数据(在共享内存中分配)
    void             *addr;			/*内存块的起始地址*/
} ngx_slab_pool_t;


void ngx_slab_init(ngx_slab_pool_t *pool);
void *ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_calloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_calloc_locked(ngx_slab_pool_t *pool, size_t size);
void ngx_slab_free(ngx_slab_pool_t *pool, void *p);
void ngx_slab_free_locked(ngx_slab_pool_t *pool, void *p);


#endif /* _NGX_SLAB_H_INCLUDED_ */
