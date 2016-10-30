
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
变量由变量名和变量值组成。 对于同一个变量名， 随着场景的不同会具有多个不同的
值， 如果认为变量值和变量名一一对应从而使用一个结构体表示， 毫无疑问会有大量内存浪
费在相同的变量名的存储上。 因此， Nginx中有一个保存变量名的结构体， 叫做
ngx_http_variable_t， 和一个保存变量值的结构体，叫做


*/

typedef ngx_variable_value_t  ngx_http_variable_value_t;

#define ngx_http_variable(v)     { sizeof(v) - 1, 1, 0, 0, 0, (u_char *) v }

typedef struct ngx_http_variable_s  ngx_http_variable_t;

typedef void (*ngx_http_set_variable_pt) (ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);
typedef ngx_int_t (*ngx_http_get_variable_pt) (ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);

//变量被添加时如果已有同名变量，则返回该变量，否则会报错认为变量名冲突
//表示这个变量是可变的
#define NGX_HTTP_VAR_CHANGEABLE   1
//变量的值不应该被缓存。变量被取值后，变量值的no_cacheable被置为1
//表示这个变量每次都要去取值，而不是直接返回上次cache的值(配合对应的接口)
#define NGX_HTTP_VAR_NOCACHEABLE  2
//表示变量被索引，存储在cmcf->variables数组，这样的变量可以通过索引值直接找到
//表示这个变量使用索引读取的
#define NGX_HTTP_VAR_INDEXED      4
//不会将该变量存储在cmcf->variables_hash哈希表
//表示这个变量不需要被hash
#define NGX_HTTP_VAR_NOHASH       8

//表示变量名
//指定一个变量名字符串，以及解析出相应变量值的方法
//所有的变量名定义ngx_http_variable_t都会保存在全局唯一的ngx_http_core_main_conf_t对象中
struct ngx_http_variable_s 
{
	//变量名称
    ngx_str_t                     name;   		/* must be first to build the hash */
	//赋值函数，主要用于"set"指令，处理请求时执行set指令时调用
	ngx_http_set_variable_pt      set_handler;
	//取值函数，当读取该变量时调动该函数得到变量值
    ngx_http_get_variable_pt      get_handler;
	//传递给set_handler和get_handler的参数
    uintptr_t                     data;
	//变量属性标志 NGX_HTTP_VAR_CHANGEABLE, NGX_HTTP_VAR_NOCACHEABLE, NGX_HTTP_VAR_INDEXED, NGX_HTTP_VAR_NOHASH
    ngx_uint_t                    flags;
	//变量在cmcf->variables中的索引
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
