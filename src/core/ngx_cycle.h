
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CYCLE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#ifndef NGX_CYCLE_POOL_SIZE
#define NGX_CYCLE_POOL_SIZE     NGX_DEFAULT_POOL_SIZE
#endif


#define NGX_DEBUG_POINTS_STOP   1
#define NGX_DEBUG_POINTS_ABORT  2


typedef struct ngx_shm_zone_s  ngx_shm_zone_t;

typedef ngx_int_t (*ngx_shm_zone_init_pt) (ngx_shm_zone_t *zone, void *data);

struct ngx_shm_zone_s 
{
    void                     *data;
    ngx_shm_t                 shm;
    ngx_shm_zone_init_pt      init;
    void                     *tag;
    ngx_uint_t                noreuse;  /* unsigned  noreuse:1; */
};


struct ngx_cycle_s 
{
    void                  ****conf_ctx;   		//NGX_CORE_MODULE���͵�ģ���������
    ngx_pool_t               *pool;

    ngx_log_t                *log;
    ngx_log_t                 new_log;

    ngx_uint_t                log_use_stderr;  		/* unsigned  log_use_stderr:1; */

    ngx_connection_t        **files;				/*�����ļ�*/
    ngx_connection_t         *free_connections;		/*�������������ͷ*/	
    ngx_uint_t                free_connection_n;	/*������������������*/

    ngx_queue_t               reusable_connections_queue; 	/*�������Ӷ���*/

    ngx_array_t               listening;		/*ngx_listening_t���͵�����*/
    ngx_array_t               paths;			/*ngx_path_t*���͵�����*/
    ngx_array_t               config_dump;		/*ngx_conf_dump_t���͵�����*/
    ngx_list_t                open_files;		/*ngx_open_file_t ���͵�����*/
    ngx_list_t                shared_memory;    /*ngx_shm_zone_t ���͵�����*/

    ngx_uint_t                connection_n;		/*connections����Ĵ�С, ÿ���������̵����������*/	
    ngx_uint_t                files_n;

    ngx_connection_t         *connections;     	/*ָ������������е�connection*/
    ngx_event_t              *read_events;
    ngx_event_t              *write_events;

    ngx_cycle_t              *old_cycle;

    ngx_str_t                 conf_file;			/*nginx.conf�����ļ�·��*/
    ngx_str_t                 conf_param;			/*�洢��������-g���õ�ֵ�??*/
    ngx_str_t                 conf_prefix;			/*����·��ǰ׺*/
    ngx_str_t                 prefix;				/*ǰ׺·��*/
    ngx_str_t                 lock_file;
    ngx_str_t                 hostname;
};


typedef struct 
{
     ngx_flag_t               daemon;
     ngx_flag_t               master;

     ngx_msec_t               timer_resolution;

     ngx_int_t                worker_processes;		/*�������̵ĸ���*/
     ngx_int_t                debug_points;

     ngx_int_t                rlimit_nofile;  		/*�����������������??*/
     off_t                    rlimit_core;			/*CoreDump�ļ���С*/

     int                      priority;

     ngx_uint_t               cpu_affinity_n;	/*cpu_affinity�������*/
     uint64_t                *cpu_affinity;  	/*uint64_t���͵����飬ÿ��Ԫ�ر�ʾһ���������̵�CPU�׺�������*/

     char                    *username;  	/*�û���*/
     ngx_uid_t                user;			/*�û�uid*/
     ngx_gid_t                group;		/*�û�gid*/

     ngx_str_t                working_directory;  /*ָ�����̵�ǰ����Ŀ¼*/
     ngx_str_t                lock_file;

     ngx_str_t                pid;
     ngx_str_t                oldpid;

     ngx_array_t              env;    		/*ngx_str_t���͵�����, �洢��������*/
     char                   **environment;
} ngx_core_conf_t;


#define ngx_is_init_cycle(cycle)  (cycle->conf_ctx == NULL)


ngx_cycle_t *ngx_init_cycle(ngx_cycle_t *old_cycle);
ngx_int_t ngx_create_pidfile(ngx_str_t *name, ngx_log_t *log);
void ngx_delete_pidfile(ngx_cycle_t *cycle);
ngx_int_t ngx_signal_process(ngx_cycle_t *cycle, char *sig);
void ngx_reopen_files(ngx_cycle_t *cycle, ngx_uid_t user);
char **ngx_set_environment(ngx_cycle_t *cycle, ngx_uint_t *last);
ngx_pid_t ngx_exec_new_binary(ngx_cycle_t *cycle, char *const *argv);
uint64_t ngx_get_cpu_affinity(ngx_uint_t n);
ngx_shm_zone_t *ngx_shared_memory_add(ngx_conf_t *cf, ngx_str_t *name,
    size_t size, void *tag);


extern volatile ngx_cycle_t  *ngx_cycle;
extern ngx_array_t            ngx_old_cycles;
extern ngx_module_t           ngx_core_module;
extern ngx_uint_t             ngx_test_config;
extern ngx_uint_t             ngx_dump_config;
extern ngx_uint_t             ngx_quiet_mode;


#endif /* _NGX_CYCLE_H_INCLUDED_ */
