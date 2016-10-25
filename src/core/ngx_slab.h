
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

//�ڹ����ڴ��з���
typedef struct 
{
    ngx_shmtx_sh_t    lock;

    size_t            min_size;		/*��С���䵥Ԫ  1 << min_shift */
    size_t            min_shift;   	/*��С���䵥Ԫ��Ӧ��λ�ƣ�Ĭ��ֵ3*/

    ngx_slab_page_t  *pages;		/*ҳ�����׵�ַ*/
    ngx_slab_page_t  *last;			/*ҳ����ĩ�˵�ַ*/
    ngx_slab_page_t   free;			/*����ҳ����*/

    u_char           *start;		/*�ɷ���ռ����ʼ��ַ*/
    u_char           *end;			/*�ڴ��Ľ�����ַ*/ 

    ngx_shmtx_t       mutex;

    u_char           *log_ctx;
    u_char            zero;

    unsigned          log_nomem:1;

    void             *data;			//���ڴ���˽������(�ڹ����ڴ��з���)
    void             *addr;			/*�ڴ�����ʼ��ַ*/
} ngx_slab_pool_t;


void ngx_slab_init(ngx_slab_pool_t *pool);
void *ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_calloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_calloc_locked(ngx_slab_pool_t *pool, size_t size);
void ngx_slab_free(ngx_slab_pool_t *pool, void *p);
void ngx_slab_free_locked(ngx_slab_pool_t *pool, void *p);


#endif /* _NGX_SLAB_H_INCLUDED_ */
