
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

    unsigned           read:1;					//��־λ��Ϊ1��ʾ���յ�������Ӧ����
    unsigned           cacheable:1;				//��־λ����ʾ�Ƿ������ļ�����
    unsigned           single_buf:1;			//��־λ��Ϊ1��ʾÿ��ֻ�ܽ���һ��ngx_buf_t����������Ӧ���� 
    unsigned           free_bufs:1;
    unsigned           upstream_done:1;			//��־λ��Ϊ1��ʾ���������Ѿ��ر�
    unsigned           upstream_error:1;		//��־λ��Ϊ1��ʾ�������ӳ���
    unsigned           upstream_eof:1;			//��־λ��Ϊ1��ʾ��������ͨ���Ѿ�����
    unsigned           upstream_blocked:1;
    unsigned           downstream_done:1;		//��־λ��Ϊ1��ʾ���������Ѿ��ر�
    unsigned           downstream_error:1;		//��־λ��Ϊ1��ʾ�������ӳ���
    unsigned           cyclic_temp_file:1;

    ngx_int_t          allocated;				//������ǰ�Ѿ�����Ľ������ΰ���Ļ������ĸ���
    ngx_bufs_t         bufs;					//ָ���ܹ�����Ľ������ΰ���Ļ������Ĵ�С�͸���
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

    ngx_chain_t       *preread_bufs;		//Ԥ������Ӧ����(�ڽ�����Ӧͷ��ʱ���յ�����Ӧ����)�Ļ�����
    size_t             preread_size; 		//Ԥ������Ӧ����(�ڽ�����Ӧͷ��ʱ���յ�����Ӧ����)�ĳ���
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
