
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
    void                     *data;		/*共享内存特定数据*/	
    ngx_shm_t                 shm;
    ngx_shm_zone_init_pt      init;		/*共享内存特定初始化函数*/
    void                     *tag;		/*共享内存标签，用于标明该共享内存属于哪个模块*/
    ngx_uint_t                noreuse;  /* unsigned  noreuse:1; */
};


struct ngx_cycle_s 
{
	//保存着所有核心模块存配置结构体的指针
    void                  ****conf_ctx;   		
    ngx_pool_t               *pool;				//内存池
												
    ngx_log_t                *log;				//日志模块中提供了生成基本ngx_log_t日志对象的功能，这里的log实际上是在还没有执行ngx_init_cycle方法前，也就是还没有解析配置前，如果有信需要输出到日志，就会暂时使用log对象，它会输出到屏幕。在ngx_init_cycle方法执行后，将会根据nginx.conf配置文件中的配置项，构造出正确的日志文件，此时会对log重新赋值
    ngx_log_t                 new_log;			//由nginx.conf配置文件读取到日志文件路径后，将开始初始化error_log日志文件，由于log对象还在用于输出日志到屏幕，这时会用new_log对象暂时性的替代log日志，待初始化成功后，会用new_log的地址覆盖上面的log指针

    ngx_uint_t                log_use_stderr;  		/* unsigned  log_use_stderr:1; */
	//对于poll、rtsig这样的事件模块，会以有效文件句柄数来预先建立这些ngx_connection_t结构体，以加速事件的收集、分发。
	//这时files就会保存所有ngx_connection_t的指针组成的数组，files_n就是指针的总数，而文件句柄的值用来访问files数组成员
    ngx_connection_t        **files;				
    ngx_connection_t         *free_connections;				//可用连接池，与free_connection_n配合使用
    ngx_uint_t                free_connection_n;			//可用连接池中连接的总数
	//ngx_connection_t类型的双向链表容器，表示可重复使用的连接的队列
    ngx_queue_t               reusable_connections_queue; 	
	//ngx_listening_t类型的动态数组，表示监听端口及相关参数
    ngx_array_t               listening;	
	//ngx_path_t*类型的动态数组，保存着Nginx所有要操作的目录。如果有目录不存在，而创建目录失败会导致Nginx启动失败。
	//例如，上传文件的临时目录也在pathes中，如果没有权限创建，则会导致Nginx无法启动
    ngx_array_t               paths;			
    ngx_array_t               config_dump;		/*ngx_conf_dump_t类型的数组*/
	//ngx_open_file_t 类型的单链表，表示Nginx已经打开的所有文件。
	//事实上，Nginx框架不会向open_files链表中添加文件，而是由感兴趣的模块向其中添加文件路径名，
	//Nginx框架会在ngx_init_cycle方法中打开这些文件
    ngx_list_t                open_files;		
	//ngx_shm_zone_t 类型的单链表，存储所有模块分配的共享内存
    ngx_list_t                shared_memory;    
	//当前进程中所有连接对象的总数，与connections成员配合使用
    ngx_uint_t                connection_n;		
    //与上面的files成员配合使用，指出files数组里元素的总数
    ngx_uint_t                files_n;			

    ngx_connection_t         *connections;     	//指向当前进程中的所有连接对象，与connection_n成员配合使用
    ngx_event_t              *read_events;		//指向当前进程中的所有读事件对象，connection_n同时表示所有写事件的总数
    ngx_event_t              *write_events;		//指向当前进程中的所有写事件对象，connection_n同时表示所有写事件的总数
	//旧的ngx_cycle_t对象用于引用上一个ngx_cycle_t对象中的成员。
	//例如，ngx_init_cycle方法，在启动初期，需要建立一个临时的ngx_cycle_t对象保存一些变量，
	//再调用ngx_init_cycle方法时就可以把旧的ngx_cycle_t对象传递进去，
	//而这时old_cycle对象就会保存这个前期的ngx_cycle_t对象
    ngx_cycle_t              *old_cycle;		

    ngx_str_t                 conf_file;		//配置文件(一般是nginx.conf)相对于安装目录的路径名称  //配置文件(一般是nginx.conf)的绝对路径
    ngx_str_t                 conf_param;		//Nginx处理配置文件时需要特殊处理的命名携带的参数，一般是-g选项携带的参数
    ngx_str_t                 conf_prefix;		//Nginx配置文件所在目录的路径
    ngx_str_t                 prefix;			//Nginx安装目录的路径
    ngx_str_t                 lock_file;		//用于进程间同步的文件锁名称
    //使用gethostname系统调用得到的主机名
    ngx_str_t                 hostname;			
};


typedef struct 
{
	//是否以守护进程的方式允许Nginx，守护进程是脱离终端并且在后台允许的进程
	ngx_flag_t               daemon;
	//是否以master/worker方式工作
	//如果关闭了master_process工作方式，就不会fork出worker子进程来处理请求，而是用master进程自身来处理请求
	ngx_flag_t               master;

	//系统调用gettimeofday的执行频率
	//默认情况下，每次内核的事件调用(如epoll)返回时，都会执行一次getimeofday，
	//实现用内核时钟来更新Nginx中的缓存时钟，若设置timer_resolution则定期更新Nginx中的缓存时钟               
	ngx_msec_t               timer_resolution;

	ngx_int_t                worker_processes;		/*工作进程的个数*/
	//Nginx在一些关键的错误逻辑中设置了调试点。
	//如果设置了debug_points为NGX_DEBUG_POINTS_STOP，那么Nginx执行到这些调试点时就会发出SIGSTOP信号以用于调试
	//如果设置了debug_points为NGX_DEBUG_POINTS_ABORT，那么Nginx执行到这些调试点时就会产生一个coredump文件，
	//可以使用gdb来查看Nginx当时的各种信息
	ngx_int_t                debug_points;
	//一个worker进程可以打开的最大句柄描述符个数
	ngx_int_t                rlimit_nofile;  	
	//限制coredump核心转储文件的大小
	//在Linux系统中，当进程发生错误或收到信号而终止时，系统会将进程执行时的内存内容(核心映像)写入一个文件(core文件)，
	//以作调试之用，这就是所谓的核心转储(core dump)
	off_t                    rlimit_core;			/*CoreDump文件大小*/

	//指定Nginx worker进程的nice优先级
	int                      priority;

	ngx_uint_t               cpu_affinity_n;	/*cpu_affinity数组个数*/
	uint64_t                *cpu_affinity;  	/*uint64_t类型的数组，每个元素表示一个工作进程的CPU亲和性掩码*/

	char                    *username;  	/*用户名*/
	ngx_uid_t                user;			/*用户uid*/
	ngx_gid_t                group;			/*用户gid*/
	//指定进程当前工作目录
	ngx_str_t                working_directory;  
	//lock文件的路径
	ngx_str_t                lock_file;
	//保存master进程ID的pid文件存放路径
	ngx_str_t                pid;
	ngx_str_t                oldpid;

	ngx_array_t              env;    		/*ngx_str_t类型的动态数组, 存储环境变量*/
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
ngx_shm_zone_t *ngx_shared_memory_add(ngx_conf_t *cf, ngx_str_t *name, size_t size, void *tag);


extern volatile ngx_cycle_t  *ngx_cycle;
extern ngx_array_t            ngx_old_cycles;
extern ngx_module_t           ngx_core_module;
extern ngx_uint_t             ngx_test_config;
extern ngx_uint_t             ngx_dump_config;
extern ngx_uint_t             ngx_quiet_mode;


#endif /* _NGX_CYCLE_H_INCLUDED_ */
