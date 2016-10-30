
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_VARIABLES_H_INCLUDED_
#define _NGX_HTTP_VARIABLES_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
/*
�����ɱ������ͱ���ֵ��ɡ� ����ͬһ���������� ���ų����Ĳ�ͬ����ж����ͬ��
ֵ�� �����Ϊ����ֵ�ͱ�����һһ��Ӧ�Ӷ�ʹ��һ���ṹ���ʾ�� �������ʻ��д����ڴ���
������ͬ�ı������Ĵ洢�ϡ� ��ˣ� Nginx����һ������������Ľṹ�壬 ����
ngx_http_variable_t�� ��һ���������ֵ�Ľṹ�壬����


*/

typedef ngx_variable_value_t  ngx_http_variable_value_t;

#define ngx_http_variable(v)     { sizeof(v) - 1, 1, 0, 0, 0, (u_char *) v }

typedef struct ngx_http_variable_s  ngx_http_variable_t;

typedef void (*ngx_http_set_variable_pt) (ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);
typedef ngx_int_t (*ngx_http_get_variable_pt) (ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);

//���������ʱ�������ͬ���������򷵻ظñ���������ᱨ����Ϊ��������ͻ
//��ʾ��������ǿɱ��
#define NGX_HTTP_VAR_CHANGEABLE   1
//������ֵ��Ӧ�ñ����档������ȡֵ�󣬱���ֵ��no_cacheable����Ϊ1
//��ʾ�������ÿ�ζ�Ҫȥȡֵ��������ֱ�ӷ����ϴ�cache��ֵ(��϶�Ӧ�Ľӿ�)
#define NGX_HTTP_VAR_NOCACHEABLE  2
//��ʾ�������������洢��cmcf->variables���飬�����ı�������ͨ������ֱֵ���ҵ�
//��ʾ�������ʹ��������ȡ��
#define NGX_HTTP_VAR_INDEXED      4
//���Ὣ�ñ����洢��cmcf->variables_hash��ϣ��
//��ʾ�����������Ҫ��hash
#define NGX_HTTP_VAR_NOHASH       8

//��ʾ������
//ָ��һ���������ַ������Լ���������Ӧ����ֵ�ķ���
//���еı���������ngx_http_variable_t���ᱣ����ȫ��Ψһ��ngx_http_core_main_conf_t������
struct ngx_http_variable_s 
{
	//��������
    ngx_str_t                     name;   		/* must be first to build the hash */
	//��ֵ��������Ҫ����"set"ָ���������ʱִ��setָ��ʱ����
	ngx_http_set_variable_pt      set_handler;
	//ȡֵ����������ȡ�ñ���ʱ�����ú����õ�����ֵ
    ngx_http_get_variable_pt      get_handler;
	//���ݸ�set_handler��get_handler�Ĳ���
    uintptr_t                     data;
	//�������Ա�־ NGX_HTTP_VAR_CHANGEABLE, NGX_HTTP_VAR_NOCACHEABLE, NGX_HTTP_VAR_INDEXED, NGX_HTTP_VAR_NOHASH
    ngx_uint_t                    flags;
	//������cmcf->variables�е�����
    ngx_uint_t                    index;
};


ngx_http_variable_t *ngx_http_add_variable(ngx_conf_t *cf, ngx_str_t *name, ngx_uint_t flags);
ngx_int_t ngx_http_get_variable_index(ngx_conf_t *cf, ngx_str_t *name);
ngx_http_variable_value_t *ngx_http_get_indexed_variable(ngx_http_request_t *r, ngx_uint_t index);
ngx_http_variable_value_t *ngx_http_get_flushed_variable(ngx_http_request_t *r, ngx_uint_t index);

ngx_http_variable_value_t *ngx_http_get_variable(ngx_http_request_t *r, ngx_str_t *name, ngx_uint_t key);

ngx_int_t ngx_http_variable_unknown_header(ngx_http_variable_value_t *v, ngx_str_t *var, ngx_list_part_t *part, size_t prefix);


#if (NGX_PCRE)

typedef struct {
    ngx_uint_t                    capture;
    ngx_int_t                     index;
} ngx_http_regex_variable_t;


typedef struct {
    ngx_regex_t                  *regex;
    ngx_uint_t                    ncaptures;
    ngx_http_regex_variable_t    *variables;
    ngx_uint_t                    nvariables;
    ngx_str_t                     name;
} ngx_http_regex_t;


typedef struct {
    ngx_http_regex_t             *regex;
    void                         *value;
} ngx_http_map_regex_t;


ngx_http_regex_t *ngx_http_regex_compile(ngx_conf_t *cf, ngx_regex_compile_t *rc);
ngx_int_t ngx_http_regex_exec(ngx_http_request_t *r, ngx_http_regex_t *re, ngx_str_t *s);

#endif


typedef struct {
    ngx_hash_combined_t           hash;
#if (NGX_PCRE)
    ngx_http_map_regex_t         *regex;
    ngx_uint_t                    nregex;
#endif
} ngx_http_map_t;


void *ngx_http_map_find(ngx_http_request_t *r, ngx_http_map_t *map, ngx_str_t *match);


ngx_int_t ngx_http_variables_add_core_vars(ngx_conf_t *cf);
ngx_int_t ngx_http_variables_init_vars(ngx_conf_t *cf);


extern ngx_http_variable_value_t  ngx_http_variable_null_value;
extern ngx_http_variable_value_t  ngx_http_variable_true_value;


#endif /* _NGX_HTTP_VARIABLES_H_INCLUDED_ */
