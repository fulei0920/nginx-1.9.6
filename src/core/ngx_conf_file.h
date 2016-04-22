
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CONF_FILE_H_INCLUDED_
#define _NGX_CONF_FILE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 *        AAAA  number of arguments
 *      FF      command flags
 *    TT        command type, i.e. HTTP "location" or "server" command
 */

#define NGX_CONF_NOARGS      0x00000001			//≈‰÷√÷∏¡Ó≤ªΩ” ‹»Œ∫Œ≤Œ ˝
#define NGX_CONF_TAKE1       0x00000002			//≈‰÷√÷∏¡ÓΩ” ‹1∏ˆ≤Œ ˝
#define NGX_CONF_TAKE2       0x00000004			//≈‰÷√÷∏¡ÓΩ” ‹1∏ˆ≤Œ ˝	
#define NGX_CONF_TAKE3       0x00000008			//≈‰÷√÷∏¡ÓΩ” ‹1∏ˆ≤Œ ˝
#define NGX_CONF_TAKE4       0x00000010			
#define NGX_CONF_TAKE5       0x00000020
#define NGX_CONF_TAKE6       0x00000040
#define NGX_CONF_TAKE7       0x00000080

#define NGX_CONF_MAX_ARGS    8

#define NGX_CONF_TAKE12      (NGX_CONF_TAKE1|NGX_CONF_TAKE2)
#define NGX_CONF_TAKE13      (NGX_CONF_TAKE1|NGX_CONF_TAKE3)

#define NGX_CONF_TAKE23      (NGX_CONF_TAKE2|NGX_CONF_TAKE3)

#define NGX_CONF_TAKE123     (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3)
#define NGX_CONF_TAKE1234    (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3 |NGX_CONF_TAKE4)

#define NGX_CONF_ARGS_NUMBER 0x000000ff
#define NGX_CONF_BLOCK       0x00000100	//≈‰÷√÷∏¡Óø…“‘Ω” ‹µƒ÷µ «“ª∏ˆ≈‰÷√–≈œ¢øÈ
#define NGX_CONF_FLAG        0x00000200	//≈‰÷√÷∏¡Óø…“‘Ω” ‹µƒ÷µ «°±on°±ªÚ’ﬂ°±off°±£¨◊Ó÷’ª·±ª◊™≥…bool÷µ°£
#define NGX_CONF_ANY         0x00000400	//≈‰÷√÷∏¡Óø…“‘Ω” ‹µƒ»Œ“‚µƒ≤Œ ˝÷µ°£“ª∏ˆªÚ’ﬂ∂‡∏ˆ£¨ªÚ’ﬂ°±on°±ªÚ’ﬂ°±off°±£¨ªÚ’ﬂ «≈‰÷√øÈ°£
#define NGX_CONF_1MORE       0x00000800
#define NGX_CONF_2MORE       0x00001000
#define NGX_CONF_MULTI       0x00000000  /* compatibility */

#define NGX_DIRECT_CONF      0x00010000
#define NGX_MAIN_CONF        0x01000000
#define NGX_ANY_CONF         0x0F000000



#define NGX_CONF_UNSET       -1
#define NGX_CONF_UNSET_UINT  (ngx_uint_t) -1
#define NGX_CONF_UNSET_PTR   (void *) -1
#define NGX_CONF_UNSET_SIZE  (size_t) -1
#define NGX_CONF_UNSET_MSEC  (ngx_msec_t) -1


#define NGX_CONF_OK          NULL
#define NGX_CONF_ERROR       (void *) -1

#define NGX_CONF_BLOCK_START 1
#define NGX_CONF_BLOCK_DONE  2
#define NGX_CONF_FILE_DONE   3

#define NGX_CORE_MODULE      0x45524F43  /* "CORE" */
#define NGX_CONF_MODULE      0x464E4F43  /* "CONF" */


#define NGX_MAX_CONF_ERRSTR  1024


struct ngx_command_s 
{
    ngx_str_t             name;    	/*≈‰÷√÷∏¡Óµƒ√˚≥∆*/
    ngx_uint_t            type;		
    char               *(*set)(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);  /*≈‰÷√÷∏¡Ó¥¶¿Ìªÿµ˜∫Ø ˝*/
    ngx_uint_t            conf;   	/*¥Ê∑≈µƒ÷µ‘⁄…œœ¬Œƒ÷–µƒ∆´“∆¡ø*/
    ngx_uint_t            offset;	/*¥Ê∑≈µƒ÷µ‘⁄Ω·ππÃÂ÷–µƒ∆´“∆¡ø*/
    void                 *post;
};

#define ngx_null_command  { ngx_null_string, 0, NULL, 0, 0, NULL }


struct ngx_open_file_s 
{
    ngx_fd_t              fd;			/* ±Í◊ºIOŒƒº˛√Ë ˆ∑˚ */
    ngx_str_t             name;			/* Œƒº˛√˚≥∆ */
    void                (*flush)(ngx_open_file_t *file, ngx_log_t *log);
    void                 *data;
};


#define NGX_MODULE_V1          0, 0, 0, 0, 0, 0, 1
#define NGX_MODULE_V1_PADDING  0, 0, 0, 0, 0, 0, 0, 0


//Nginxƒ£øÈª˘±æΩ”ø⁄
struct ngx_module_s 
{
	//µ±«∞ƒ£øÈ‘ŸÕ¨¿‡ƒ£øÈ÷–µƒ–Ú∫≈
    ngx_uint_t            ctx_index;	
	//µ±«∞ƒ£øÈ‘⁄À˘”–ƒ£øÈ(ngx_modules ˝◊È)÷–µƒ–Ú∫≈
    ngx_uint_t            index;       
    ngx_uint_t            spare0;
    ngx_uint_t            spare1;
    ngx_uint_t            spare2;
    ngx_uint_t            spare3;
	//µ±«∞ƒ£øÈ∞Ê±æ∫≈
    ngx_uint_t            version;		
	//ƒ≥“ª¿‡ƒ£øÈµƒÃÿ∂®Õ®”√Ω”ø⁄£¨
	//¿˝»Á∫À–ƒƒ£øÈµƒÕ®”√Ω”ø⁄ «ngx_core_module_tΩ·ππÃÂ£¨∂¯ ¬º˛ƒ£øÈµƒÕ®”√Ω”ø⁄ «ngx_event_module_tΩ·ππÃÂ
    void                 *ctx;			/*ƒ£øÈ…œœ¬Œƒ*/
	//ƒ£øÈ÷∏¡Ó
    ngx_command_t        *commands;
	//ƒ£øÈ¿‡–Õ:NGX_CONF_MODULE | NGX_CORE_MODULE | NGX_EVENT_MODULE | NGX_HTTP_MODULE | NGX_MAIL_MODULE | NGX_STREAM_MODULE
    ngx_uint_t            type;			
	

    ngx_int_t           (*init_master)(ngx_log_t *log);
    ngx_int_t           (*init_module)(ngx_cycle_t *cycle);
    ngx_int_t           (*init_process)(ngx_cycle_t *cycle);
    ngx_int_t           (*init_thread)(ngx_cycle_t *cycle);
    void                (*exit_thread)(ngx_cycle_t *cycle);
    void                (*exit_process)(ngx_cycle_t *cycle);
    void                (*exit_master)(ngx_cycle_t *cycle);

    uintptr_t             spare_hook0;
    uintptr_t             spare_hook1;
    uintptr_t             spare_hook2;
    uintptr_t             spare_hook3;
    uintptr_t             spare_hook4;
    uintptr_t             spare_hook5;
    uintptr_t             spare_hook6;
    uintptr_t             spare_hook7;
};

/*∫À–ƒƒ£øÈ≈‰÷√…œœ¬Œƒ*/
typedef struct 
{
    ngx_str_t             name;
    void               *(*create_conf)(ngx_cycle_t *cycle);
    char               *(*init_conf)(ngx_cycle_t *cycle, void *conf);
} ngx_core_module_t;

/*±Ì æ“ª∏ˆ≈‰÷√Œƒº˛*/
typedef struct 
{
    ngx_file_t            file;		
    ngx_buf_t            *buffer;	//ª∫¥Ê¥”file÷–∂¡»°µƒƒ⁄»›£¨»ª∫Û∂‘bufferƒ⁄»›Ω¯––≈‰÷√∑÷Œˆ
    ngx_buf_t            *dump;		//∂‘¥”file÷–∂¡»°µƒƒ⁄»›Ω¯––±∏∑›£¨”√”⁄œ‘ æ≈‰÷√Œƒº˛
    ngx_uint_t            line;  	//º«¬º∑÷ŒˆŒƒº˛ ±µ±«∞À˘‘⁄µƒŒƒº˛––∫≈
} ngx_conf_file_t;


typedef struct 
{
    ngx_str_t             name;		//≈‰÷√Œƒº˛√˚
    ngx_buf_t            *buffer;	//≈‰÷√Œƒº˛ƒ⁄»›
} ngx_conf_dump_t;


typedef char *(*ngx_conf_handler_pt)(ngx_conf_t *cf, ngx_command_t *dummy, void *conf);


struct ngx_conf_s 
{
    char                 *name;
    ngx_array_t          *args;  			//ngx_str_t¿‡–Õµƒ ˝◊È£¨¥Ê¥¢Ω‚ŒˆµΩµƒ√¸¡Óº∞≤Œ ˝¡–±Ì

    ngx_cycle_t          *cycle;			
    ngx_pool_t           *pool;				/*÷∏œÚcycleµƒpool*/
    ngx_pool_t           *temp_pool;		/*¡Ÿ ±±‰¡ø∑÷≈‰≥ÿ*/
    ngx_conf_file_t      *conf_file;		/*≈‰÷√Œƒº˛*/
    ngx_log_t            *log;				/*÷∏œÚ»’÷æ*/

    void                 *ctx;				/*÷∏œÚ…œœ¬Œƒ???*/
    ngx_uint_t            module_type;		/*÷∏∂®µ±«∞Ω‚Œˆµƒƒ£øÈ¿‡–Õ*/
    ngx_uint_t            cmd_type;			/*÷∏∂®µ±«∞Ω‚Œˆµƒ√¸¡Ó¿‡–Õ*/

    ngx_conf_handler_pt   handler;
    char                 *handler_conf;
};


typedef char *(*ngx_conf_post_handler_pt) (ngx_conf_t *cf, void *data, void *conf);

typedef struct 
{
    ngx_conf_post_handler_pt  post_handler;
} ngx_conf_post_t;


typedef struct {
    ngx_conf_post_handler_pt  post_handler;
    char                     *old_name;
    char                     *new_name;
} ngx_conf_deprecated_t;


typedef struct {
    ngx_conf_post_handler_pt  post_handler;
    ngx_int_t                 low;
    ngx_int_t                 high;
} ngx_conf_num_bounds_t;


typedef struct {
    ngx_str_t                 name;
    ngx_uint_t                value;
} ngx_conf_enum_t;


#define NGX_CONF_BITMASK_SET  1

typedef struct 
{
    ngx_str_t                 name;
    ngx_uint_t                mask;
} ngx_conf_bitmask_t;



char * ngx_conf_deprecated(ngx_conf_t *cf, void *post, void *data);
char *ngx_conf_check_num_bounds(ngx_conf_t *cf, void *post, void *data);

//ªÒ»°ƒ≥∏ˆƒ£øÈµƒ≈‰÷√Â
#define ngx_get_conf(conf_ctx, module)  conf_ctx[module.index]



#define ngx_conf_init_value(conf, default)                                   \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = default;                                                      \
    }

#define ngx_conf_init_ptr_value(conf, default)                               \
    if (conf == NGX_CONF_UNSET_PTR) {                                        \
        conf = default;                                                      \
    }

#define ngx_conf_init_uint_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_UINT) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_init_size_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_SIZE) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_init_msec_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_MSEC) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_merge_value(conf, prev, default)                            \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_ptr_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET_PTR) {                                        \
        conf = (prev == NGX_CONF_UNSET_PTR) ? default : prev;                \
    }

#define ngx_conf_merge_uint_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_UINT) {                                       \
        conf = (prev == NGX_CONF_UNSET_UINT) ? default : prev;               \
    }

#define ngx_conf_merge_msec_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_MSEC) {                                       \
        conf = (prev == NGX_CONF_UNSET_MSEC) ? default : prev;               \
    }

#define ngx_conf_merge_sec_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_size_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_SIZE) {                                       \
        conf = (prev == NGX_CONF_UNSET_SIZE) ? default : prev;               \
    }

#define ngx_conf_merge_off_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_str_value(conf, prev, default)                        \
    if (conf.data == NULL) {                                                 \
        if (prev.data) {                                                     \
            conf.len = prev.len;                                             \
            conf.data = prev.data;                                           \
        } else {                                                             \
            conf.len = sizeof(default) - 1;                                  \
            conf.data = (u_char *) default;                                  \
        }                                                                    \
    }

#define ngx_conf_merge_bufs_value(conf, prev, default_num, default_size)     \
    if (conf.num == 0) {                                                     \
        if (prev.num) {                                                      \
            conf.num = prev.num;                                             \
            conf.size = prev.size;                                           \
        } else {                                                             \
            conf.num = default_num;                                          \
            conf.size = default_size;                                        \
        }                                                                    \
    }

#define ngx_conf_merge_bitmask_value(conf, prev, default)                    \
    if (conf == 0) {                                                         \
        conf = (prev == 0) ? default : prev;                                 \
    }


char *ngx_conf_param(ngx_conf_t *cf);
char *ngx_conf_parse(ngx_conf_t *cf, ngx_str_t *filename);
char *ngx_conf_include(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


ngx_int_t ngx_conf_full_name(ngx_cycle_t *cycle, ngx_str_t *name, ngx_uint_t conf_prefix);
ngx_open_file_t *ngx_conf_open_file(ngx_cycle_t *cycle, ngx_str_t *name);
void ngx_cdecl ngx_conf_log_error(ngx_uint_t level, ngx_conf_t *cf,
    ngx_err_t err, const char *fmt, ...);


char *ngx_conf_set_flag_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_str_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_str_array_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
char *ngx_conf_set_keyval_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_num_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_size_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_off_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_msec_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_sec_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_bufs_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_enum_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_bitmask_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


extern ngx_uint_t     ngx_max_module;
extern ngx_module_t  *ngx_modules[];


#endif /* _NGX_CONF_FILE_H_INCLUDED_ */
