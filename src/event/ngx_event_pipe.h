
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_PIPE_H_INCLUDED_
#define _NGX_EVENT_PIPE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


typedef struct ngx_event_pipe_s  ngx_event_pipe_t;

typedef ngx_int_t (*ngx_event_pipe_input_filter_pt)(ngx_event_pipe_t *p,
                                                    ngx_buf_t *buf);
typedef ngx_int_t (*ngx_event_pipe_output_filter_pt)(void *data,
                                                     ngx_chain_t *chain);


struct ngx_event_pipe_s 
{
    ngx_connection_t  *upstream;
    ngx_connection_t  *downstream;

    ngx_chain_t       *free_raw_bufs;
    ngx_chain_t       *in;
    ngx_chain_t      **last_in;

    ngx_chain_t       *out;
    ngx_chain_t       *free;
    ngx_chain_t       *busy;

    /*
     * the input filter i.e. that moves HTTP/1.1 chunks
     * from the raw bufs to an incoming chain
     */

    ngx_event_pipe_input_filter_pt    input_filter;
    void                             *input_ctx;

    ngx_event_pipe_output_filter_pt   output_filter;
    void                             *output_ctx;

    unsigned           read:1;					//标志位，为1表示接收到上游响应包体
    unsigned           cacheable:1;				//标志位，表示是否启用文件缓存
    unsigned           single_buf:1;			//标志位，为1表示每次只能接收一个ngx_buf_t缓冲区的响应包体 
    unsigned           free_bufs:1;
    unsigned           upstream_done:1;			//标志位，为1表示上游连接已经关闭
    unsigned           upstream_error:1;		//标志位，为1表示上游连接出错
    unsigned           upstream_eof:1;			//标志位，为1表示上游连接通信已经结束
    unsigned           upstream_blocked:1;
    unsigned           downstream_done:1;		//标志位，为1表示下游连接已经关闭
    unsigned           downstream_error:1;		//标志位，为1表示下游连接出错
    unsigned           cyclic_temp_file:1;

    ngx_int_t          allocated;				//表明当前已经分配的接收上游包体的缓冲区的个数
    ngx_bufs_t         bufs;					//指定能够分配的接收上游包体的缓冲区的大小和个数
    ngx_buf_tag_t      tag;

    ssize_t            busy_size;

    off_t              read_length;
    off_t              length;

    off_t              max_temp_file_size;
    ssize_t            temp_file_write_size;

    ngx_msec_t         read_timeout;
    ngx_msec_t         send_timeout;
    ssize_t            send_lowat;

    ngx_pool_t        *pool;
    ngx_log_t         *log;

    ngx_chain_t       *preread_bufs;		//预接收响应包体(在接收响应头部时接收到的响应包体)的缓冲区
    size_t             preread_size; 		//预接收响应包体(在接收响应头部时接收到的响应包体)的长度
    ngx_buf_t         *buf_to_file;

    size_t             limit_rate;
    time_t             start_sec;

    ngx_temp_file_t   *temp_file;

    /* STUB */ int     num;
};


ngx_int_t ngx_event_pipe(ngx_event_pipe_t *p, ngx_int_t do_write);
ngx_int_t ngx_event_pipe_copy_input_filter(ngx_event_pipe_t *p, ngx_buf_t *buf);
ngx_int_t ngx_event_pipe_add_free_buf(ngx_event_pipe_t *p, ngx_buf_t *b);


#endif /* _NGX_EVENT_PIPE_H_INCLUDED_ */
