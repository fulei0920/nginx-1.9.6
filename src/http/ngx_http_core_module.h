
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_CORE_H_INCLUDED_
#define _NGX_HTTP_CORE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#if (NGX_THREADS)
#include <ngx_thread_pool.h>
#endif


#define NGX_HTTP_GZIP_PROXIED_OFF       0x0002
#define NGX_HTTP_GZIP_PROXIED_EXPIRED   0x0004
#define NGX_HTTP_GZIP_PROXIED_NO_CACHE  0x0008
#define NGX_HTTP_GZIP_PROXIED_NO_STORE  0x0010
#define NGX_HTTP_GZIP_PROXIED_PRIVATE   0x0020
#define NGX_HTTP_GZIP_PROXIED_NO_LM     0x0040
#define NGX_HTTP_GZIP_PROXIED_NO_ETAG   0x0080
#define NGX_HTTP_GZIP_PROXIED_AUTH      0x0100
#define NGX_HTTP_GZIP_PROXIED_ANY       0x0200


#define NGX_HTTP_AIO_OFF                0
#define NGX_HTTP_AIO_ON                 1
#define NGX_HTTP_AIO_THREADS            2


#define NGX_HTTP_SATISFY_ALL            0
#define NGX_HTTP_SATISFY_ANY            1


#define NGX_HTTP_LINGERING_OFF          0
#define NGX_HTTP_LINGERING_ON           1
#define NGX_HTTP_LINGERING_ALWAYS       2


#define NGX_HTTP_IMS_OFF                0
#define NGX_HTTP_IMS_EXACT              1
#define NGX_HTTP_IMS_BEFORE             2


#define NGX_HTTP_KEEPALIVE_DISABLE_NONE    0x0002
#define NGX_HTTP_KEEPALIVE_DISABLE_MSIE6   0x0004
#define NGX_HTTP_KEEPALIVE_DISABLE_SAFARI  0x0008


typedef struct ngx_http_location_tree_node_s  ngx_http_location_tree_node_t;
typedef struct ngx_http_core_loc_conf_s  ngx_http_core_loc_conf_t;


typedef struct 
{
    union 
	{
        struct sockaddr        sockaddr;
        struct sockaddr_in     sockaddr_in;
#if (NGX_HAVE_INET6)
        struct sockaddr_in6    sockaddr_in6;
#endif
#if (NGX_HAVE_UNIX_DOMAIN)
        struct sockaddr_un     sockaddr_un;
#endif
        u_char                 sockaddr_data[NGX_SOCKADDRLEN];
    } u;

    socklen_t                  socklen;		//Ì×½Ó×ÖµØÖ·½á¹¹µÄ³¤¶È

    unsigned                   set:1;
    unsigned                   default_server:1;
    unsigned                   bind:1;
    unsigned                   wildcard:1;
#if (NGX_HTTP_SSL)
    unsigned                   ssl:1;
#endif
#if (NGX_HTTP_V2)
    unsigned                   http2:1;
#endif
#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    unsigned                   ipv6only:1;
#endif
#if (NGX_HAVE_REUSEPORT)
    unsigned                   reuseport:1;
#endif
    unsigned                   so_keepalive:2;
    unsigned                   proxy_protocol:1;

    int                        backlog;			//TCPÖÐµÄbacklog´óÐ¡
    int                        rcvbuf;
    int                        sndbuf;
#if (NGX_HAVE_SETFIB)
    int                        setfib;
#endif
#if (NGX_HAVE_TCP_FASTOPEN)
    int                        fastopen;
#endif
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
    int                        tcp_keepidle;
    int                        tcp_keepintvl;
    int                        tcp_keepcnt;
#endif

#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
    char                      *accept_filter;
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
    ngx_uint_t                 deferred_accept;
#endif
	
    u_char                     addr[NGX_SOCKADDR_STRLEN + 1];		//ipµØÖ·ºÍ¶Ë¿ÚµÄ×Ö·û´®±íÊ¾ -- "192.18.1.2:80"
} ngx_http_listen_opt_t;

//ngx_http_phases¶¨ÒåµÄ11¸ö½×¶ÎÊÇÓÐË³ÐòµÄ£¬±ØÐë°´ÕÕÆä¶¨ÒåµÄË³ÐòÖ´ÐÐ¡£
//NGX_HTTP_FIND_CONFIG_PHASE£¬NGX_HTTP_POST_REWRITE_PHASE£¬NGX_HTTP_POST_ACCESS_PHASEºÍNGX_HTTP_TRY_FILES_PHASE
//Õâ4¸ö½×¶ÎÔò²»ÔÊÐíHTTPÄ£¿é¼ÓÈë×Ô¼ºµÄngx_http_handler_pt·½·¨´¦ÀíÓÃ»§ÇëÇó£¬ËüÃÇÓÉHTTP¿ò¼ÜÊµÏÖ¡£
typedef enum 
{
//½ÓÊÕÍêÇëÇóÍ·Ö®ºóµÄµÚÒ»¸ö½×¶Î£¬ËüÎ»ÓÚuriÖØÐ´Ö®Ç°£¬Êµ¼ÊÉÏºÜÉÙÓÐÄ£¿é»á×¢²áÔÚ¸Ã½×¶Î£¬Ä¬ÈÏµÄÇé¿öÏÂ£¬¸Ã½×¶Î±»Ìø¹ý
//ÇëÇóÍ·¶ÁÈ¡Íê³ÉÖ®ºóµÄ½×¶Î
    NGX_HTTP_POST_READ_PHASE = 0,	
//server¼¶±ðµÄuriÖØÐ´½×¶Î£¬Ò²¾ÍÊÇ¸Ã½×¶ÎÖ´ÐÐ´¦ÓÚserver¿éÄÚ£¬location¿éÍâµÄÖØÐ´Ö¸Áî£¬(ÔÚ¶ÁÈ¡ÇëÇóÍ·µÄ¹ý³ÌÖÐnginx»á¸ù¾Ýhost¼°¶Ë¿ÚÕÒµ½¶ÔÓ¦µÄÐéÄâÖ÷»úÅäÖÃ)
//Õâ¸ö½×¶ÎÖ÷ÒªÊÇ´¦ÀíÈ«¾ÖµÄ(server block)µÄrewrite¡£
	NGX_HTTP_SERVER_REWRITE_PHASE,	
// Ñ°ÕÒlocationÅäÖÃ½×¶Î£¬¸Ã½×¶ÎÊ¹ÓÃÖØÐ´Ö®ºóµÄuriÀ´²éÕÒ¶ÔÓ¦µÄlocation£¬ÖµµÃ×¢ÒâµÄÊÇ¸Ã½×¶Î¿ÉÄÜ»á±»Ö´ÐÐ¶à´Î£¬ÒòÎªÒ²¿ÉÄÜÓÐlocation¼¶±ðµÄÖØÐ´Ö¸Áî
    NGX_HTTP_FIND_CONFIG_PHASE,		//Õâ¸ö½×¶ÎÖ÷ÒªÊÇÍ¨¹ýuriÀ´²éÕÒ¶ÔÓ¦µÄlocation¡£È»ºó½«uriºÍlocationµÄÊý¾Ý¹ØÁªÆðÀ´  
//location¼¶±ðµÄuriÖØÐ´½×¶Î£¬¸Ã½×¶ÎÖ´ÐÐlocation»ù±¾µÄÖØÐ´Ö¸Áî£¬Ò²¿ÉÄÜ»á±»Ö´ÐÐ¶à´Î£»
    NGX_HTTP_REWRITE_PHASE,			
//location¼¶±ðÖØÐ´µÄºóÒ»½×¶Î£¬ÓÃÀ´¼ì²éÉÏ½×¶ÎÊÇ·ñÓÐuriÖØÐ´£¬²¢¸ù¾Ý½á¹ûÌø×ªµ½ºÏÊÊµÄ½×¶Î£»
    NGX_HTTP_POST_REWRITE_PHASE,	//post rewrite£¬Õâ¸öÖ÷ÒªÊÇ½øÐÐÒ»Ð©Ð£ÑéÒÔ¼°ÊÕÎ²¹¤×÷£¬ÒÔ±ãÓÚ½»¸øºóÃæµÄÄ£¿é¡£
// ·ÃÎÊÈ¨ÏÞ¿ØÖÆµÄÇ°Ò»½×¶Î£¬¸Ã½×¶ÎÔÚÈ¨ÏÞ¿ØÖÆ½×¶ÎÖ®Ç°£¬Ò»°ãÒ²ÓÃÓÚ·ÃÎÊ¿ØÖÆ£¬±ÈÈçÏÞÖÆ·ÃÎÊÆµÂÊ£¬Á´½ÓÊýµÈ£»
    NGX_HTTP_PREACCESS_PHASE,		//±ÈÈçÁ÷¿ØÕâÖÖÀàÐÍµÄaccess¾Í·ÅÔÚÕâ¸öphase£¬Ò²¾ÍÊÇËµËüÖ÷ÒªÊÇ½øÐÐÒ»Ð©±È½Ï´ÖÁ£¶ÈµÄaccess¡£ 
//·ÃÎÊÈ¨ÏÞ¿ØÖÆ½×¶Î£¬±ÈÈç»ùÓÚipºÚ°×Ãûµ¥µÄÈ¨ÏÞ¿ØÖÆ£¬»ùÓÚÓÃ»§ÃûÃÜÂëµÄÈ¨ÏÞ¿ØÖÆµÈ£»
    NGX_HTTP_ACCESS_PHASE,			//Õâ¸ö±ÈÈç´æÈ¡¿ØÖÆ£¬È¨ÏÞÑéÖ¤¾Í·ÅÔÚÕâ¸öphase£¬Ò»°ãÀ´Ëµ´¦Àí¶¯×÷ÊÇ½»¸øÏÂÃæµÄÄ£¿é×öµÄ.Õâ¸öÖ÷ÒªÊÇ×öÒ»Ð©Ï¸Á£¶ÈµÄaccess¡£  
//·ÃÎÊÈ¨ÏÞ¿ØÖÆµÄºóÒ»½×¶Î£¬¸Ã½×¶Î¸ù¾ÝÈ¨ÏÞ¿ØÖÆ½×¶ÎµÄÖ´ÐÐ½á¹û½øÐÐÏàÓ¦´¦Àí£»
	NGX_HTTP_POST_ACCESS_PHASE,		//Ò»°ãÀ´Ëµµ±ÉÏÃæµÄaccessÄ£¿éµÃµ½access_codeÖ®ºó¾Í»áÓÉÕâ¸öÄ£¿é¸ù¾Ýaccess_codeÀ´½øÐÐ²Ù×÷  
//try_filesÖ¸ÁîµÄ´¦Àí½×¶Î£¬Èç¹ûÃ»ÓÐÅäÖÃtry_filesÖ¸Áî£¬Ôò¸Ã½×¶Î±»Ìø¹ý
    NGX_HTTP_TRY_FILES_PHASE,	
//ÄÚÈÝÉú³É½×¶Î£¬¸Ã½×¶Î²úÉúÏìÓ¦£¬²¢·¢ËÍµ½¿Í»§¶Ë£»
	NGX_HTTP_CONTENT_PHASE,			//ÄÚÈÝ´¦ÀíÄ£¿é£¬ÎÒÃÇÒ»°ãµÄhandle¶¼ÊÇ´¦ÓÚÕâ¸öÄ£¿é  
//ÈÕÖ¾¼ÇÂ¼½×¶Î£¬¸Ã½×¶Î¼ÇÂ¼·ÃÎÊÈÕÖ¾£»
    NGX_HTTP_LOG_PHASE				
} ngx_http_phases;

typedef struct ngx_http_phase_handler_s  ngx_http_phase_handler_t;


//Ò»¸öHTTP´¦Àí½×¶ÎÖÐµÄchecker¼ì²é·½·¨£¬½ö¿ÉÒÔÓÉHTTP¿ò¼ÜÊµÏÖ£¬ÒÔ´Ë¿ØÖÆHTTPÇëÇóµÄ´¦ÀíÁ÷³Ì
//·µ»ØÖµ 
//	NGX_OK -- ÒâÎ¶×Å°Ñ¿ØÖÆÈ¨½»»¹¸øNginxµÄÊÂ¼þÄ£¿é£¬ÓÉËü¸ù¾ÝÊÂ¼þ(ÍøÂçÊÂ¼þ¡¢¶¨Ê±Æ÷ÊÂ¼þ¡¢Òì²½I/OÊÂ¼þµÈ)ÔÙ´Îµ÷¶ÈÇëÇó
//	·ÇNGX_OK -- ÒâÎ¶×ÅÏòÏÂÖ´ÐÐphase_engineÖÐµÄ¸÷´¦Àí·½·¨  
typedef ngx_int_t (*ngx_http_phase_handler_pt)(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);

struct ngx_http_phase_handler_s 
{
	//ÔÚ´¦Àíµ½Ä³Ò»¸öHTTP½×¶ÎÊ±£¬HTTP¿ò¼Ü½«»áÔÚchecker·½·¨ÒÔÊµÏÖµÄÇ°ÌáÏÂÊ×ÏÈµ÷ÓÃchecker·½·¨À´´¦ÀíÇëÇó£¬
	//¶ø²»»áÖ±½Óµ÷ÓÃÈÎºÎ½×¶ÎÖÐµÄhandler·½·¨£¬Ö»ÓÐÔÚchecker·½·¨ÖÐ²Å»áÈ¥µ÷ÓÃhandler·½·¨¡£Òò´Ë£¬ÊÂÊµÉÏËùÓÐµÄchecker
	//·½·¨¶¼ÊÇÓÉ¿ò¼ÜÖÐµÄngx_http_core_moduleÄ£¿éÊµÏÖµÄ£¬ÇÒÆÕÍ¨µÄHTTPÄ£¿éÎÞ·¨ÖØ¶¨ÒåcheckerÄ£¿é
    ngx_http_phase_handler_pt  checker;		//½øÈë»Øµ÷º¯ÊýµÄÌõ¼þÅÐ¶Ïº¯Êý£¬ÏàÍ¬½×¶ÎµÄ½Úµã¾ßÓÐÏàÍ¬µÄcheckº¯Êý*/	
	/**/
    ngx_http_handler_pt        handler;		/*Ä£¿é×¢²áµÄ»Øµ÷º¯Êý*/
	//½«ÒªÖ´ÐÐµÄÏÂÒ»¸öHTTP´¦Àí½×¶ÎµÄÐòºÅ
    ngx_uint_t                 next;	
};


typedef struct 
{
	//ÓÉngx_http_phase_handler_t¹¹³ÉµÄÊý×éÊ×µØÖ·£¬Ëü±íÊ¾Ò»¸öÇëÇó¿ÉÄÜ¾­ÀúµÄËùÓÐ´¦Àí·½·¨
    ngx_http_phase_handler_t  *handlers;	
	//±íÊ¾NGX_HTTP_SERVER_REWRITE_PHASE½×¶ÎµÚÒ»¸öngx_http_phase_handler_t´¦Àí·½·¨ÔÚhandlersÊý×éÖÐµÄÐòºÅ£¬
	//ÓÃÓÚÔÚÖ´ÐÐHTTPÇëÇóµÄÈÎºÎ½×¶ÎÖÐ¿ìËÙÌø×ªµ½NGX_HTTP_SERVER_REWRITE_PHASE½×¶Î´¦ÀíÇëÇó
    ngx_uint_t                 server_rewrite_index;
	//±íÊ¾NGX_HTTP_REWRITE_PHASE½×¶ÎµÚÒ»¸öngx_http_phase_handler_t´¦Àí·½·¨ÔÚhandlersÊý×éÖÐµÄÐòºÅ£¬
	//ÓÃÓÚÔÚÖ´ÐÐHTTPÇëÇóµÄÈÎºÎ½×¶ÎÖÐ¿ìËÙÌø×ªµ½NGX_HTTP_REWRITE_PHASE½×¶Î´¦ÀíÇëÇó
    ngx_uint_t                 location_rewrite_index;	
} ngx_http_phase_engine_t;


typedef struct 
{
	//ngx_http_handler_ptÀàÐÍµÄ¶¯Ì¬Êý×é£¬±£´æ×ÅÃ¿Ò»¸öHTTPÄ£¿é³õÊ¼»¯Ìí¼Óµ½µ±Ç°½×¶ÎµÄ´¦Àí·½·¨
    ngx_array_t                handlers;		
} ngx_http_phase_t;


typedef struct 
{
	//(ngx_http_core_srv_conf_t*)ÀàÐÍµÄ¶¯Ì¬Êý×é£¬Ã¿Ò»¸öÔªËØÖ¸Ïò±íÊ¾server¿éµÄngx_http_core_srv_conf_t½á¹¹ÌåµÄµØÖ·£¬´Ó¶ø¹ØÁªÁËËùÓÐserver¿é 
    ngx_array_t                servers;
	//ÓÉÏÂÃæ¸÷½×¶Î´¦Àí·½·¨¹¹³ÉµÄphasesÊý×é³ÉÔ±¹¹½¨µÄ½×¶ÎÒýÇæ²ÅÊÇÁ÷Ë®Ê½´¦ÀíHTTPÇëÇóµÄÊµ¼ÊÊý¾Ý½á¹¹
	//¿ØÖÆÔËÐÐ¹ý³ÌÖÐÒÔ¸öHTTPÇëÇóËùÒª¾­¹ýµÄHTTP´¦Àí½×¶Î£¬Ëü½«ÅäºÏngx_http_request_t½á¹¹ÌåÖÐµÄphase_handlerÊ¹ÓÃ
    ngx_http_phase_engine_t    phase_engine;

    ngx_hash_t                 headers_in_hash;

    ngx_hash_t                 variables_hash;
	///´æ´¢ÓÃ»§ÔÚÅäÖÃÎÄ¼þÖÐÊ¹ÓÃµÄ±äÁ¿£
	///ÓÃ»§ÔÚÅäÖÃÎÄ¼þÀïÊ¹ÓÃµÄ±äÁ¿»áÍ¨¹ýngx_http_get_variable_index()º¯Êý¶øÌí¼Óµ½variablesÖÐ
    ngx_array_t                variables;       /* array of ngx_http_variable_t */
    ngx_uint_t                 ncaptures;
	//±íÊ¾´æ´¢ËùÓÐserver_nameµÄÉ¢ÁÐ±íµÄÉ¢ÁÐÍ°µÄ¸öÊý
    ngx_uint_t                 server_names_hash_max_size;
	//±íÊ¾´æ´¢ËùÓÐserver_nameµÄÉ¢ÁÐ±íµÄÃ¿¸öÉ¢ÁÐÍ°Õ¼ÓÃµÄÄÚ´æ´óÐ¡
    ngx_uint_t                 server_names_hash_bucket_size;

    ngx_uint_t                 variables_hash_max_size;
    ngx_uint_t                 variables_hash_bucket_size;
	///´æ´¢nginxÄ£¿éÄÚ²¿¶¨ÒåÌá¹©¸øÍâ²¿Ê¹ÓÃµÄ±äÁ¿
    ngx_hash_keys_arrays_t    *variables_keys;  /* ÒÔnameÎªkey£¬ÒÔngx_http_variable_tÎªvalueµÄ±í*/

    ngx_array_t               *ports;			//´æ·Å×Å¸Ãhttp{}ÅäÖÃ¿éÏÂ¼àÌýµÄËùÓÐngx_http_conf_port_t¶Ë¿Ú

    ngx_uint_t                 try_files;       /* unsigned  try_files:1 */

	//ÓÃÓÚÔÚHTTP¿ò¼Ü³õÊ¼»¯Ê±°ïÖú¸÷¸öHTTPÄ£¿éÔÚÈÎÒâ½×¶ÎÖÐÌí¼ÓHTTP´¦Àí·½·¨£¬
	//ËüÊÇÒ»¸öÓÐ11¸ö³ÉÔ±µÄngx_http_phase_tÊý×é£¬ÆäÖÐÃ¿Ò»¸öngx_http_phase_t½á¹¹Ìå¶ÔÓ¦
	//Ò»¸öHTTP½×¶Î¡£ÔÚHTTP¿ò¼Ü³õÊ¼»¯Íê±Ïºó£¬ÔËÐÐ¹ý³ÌÖÐµÄphasesÊý×éÊÇÎÞÓÃµÄ
    ngx_http_phase_t           phases[NGX_HTTP_LOG_PHASE + 1];  	
} ngx_http_core_main_conf_t;


typedef struct 
{	
	//ÔÚ¿ªÊ¼´¦ÀíÒ»¸öHTTPÇëÇóÊ±£¬Nginx»áÈ¡³öheaderÖÐµÄHost£¬ÓëÃ¿¸öserverÖÐµÄserver_name½øÐÐÆ¥Åä£¬
	//ÒÔ´Ë¾ö¶¨µ½µ×ÓÉÄÄÒ»¸öserver¿éÀ´´¦ÀíÕâ¸öÇëÇó¡£
	//µ±Ò»¸öHostÓë¶à¸öserver¿éÖÐµÄsever_name¶¼Æ¥Åä£¬½«¸ù¾Ýserver_nameÓëHostÆ¥ÅäÓÅÏÈ¼¶À´Ñ¡ÔñÊµ¼Ê´¦ÀíµÄserver¿é
	//1>Ñ¡Ôñ×Ö·û´®ÍêÈ«Æ¥ÅäµÄserver_name£¬Èçwww.testweb.com
	//2>Ñ¡ÔñÍ¨Åä·ûÔÚÇ°ÃæµÄserver_name£¬Èç*.testweb.com
	//3>Ñ¡ÔñÍ¨Åä·ûÔÚºóÃæµÄserver_name£¬Èçwww.testweb.*
	//4>Ñ¡ÔñÊ¹ÓÃÕýÔò±í´ïÊ½²ÅÆ¥ÅäµÄserver_name£¬Èç~^\.testweb\.com$
	//Èç¹ûHostÓëËùÓÐµÄserver_name¶¼²»Æ¥Åä£¬½«°´Ò»ÏÂË³ÐòÑ¡Ôñ´¦ÀíµÄserver¿é
	//1>Ñ¡ÔñÔÚlistenÅäÖÃÏîºó¼ÓÈë[default | default_server]µÄserver¿é
	//2>Ñ¡ÔñÆ¥Åälisten¶Ë¿ÚµÄµÚÒ»¸öserver¿é
    ngx_array_t                 server_names;  		/* ngx_http_server_name_t, ±£´æÓÚ¸ÃserverÅäÖÃÏà¹ØµÄËùÓÐsever name*/
	//Ö¸Ïòµ±Ç°server¿éËùÊôµÄngx_http_conf_ctx_t½á¹¹Ìå
    ngx_http_conf_ctx_t        *ctx;	
	//µ±Ç°server¿éµÄÐéÄâÖ÷»úÃû£¬Èç¹û´æÔÚµÄ»°£¬Ôò»áÓëhttpÇëÇóÖÐµÄHostÍ·²¿×öÆ¥Åä£¬
	//Æ¥ÅäÉÏºóÔÙÓÉµ±Ç°ngx_http_core_srv_conf_t´¦ÀíÇëÇó
    ngx_str_t                   server_name;		
	//Ö¸¶¨Ã¿¸öTCPÁ¬½ÓÉÏ³õÊ¼ÄÚ´æ³Ø´óÐ¡
    size_t                      connection_pool_size;	
	//Ö¸¶¨Ã¿¸öHTTPÇëÇóµÄ³õÊ¼ÄÚ´æ³Ø´óÐ¡£¬ÓÃÓÚ¼õÉÙÄÚºË¶ÔÓÚÐ¡¿éÄÚ´æµÄ·ÖÅä´ÎÊý
	//TCPÁ¬½Ó¹Ø±ÕÊ±»áÏú»Ùconnection_pool_sizeÖ¸¶¨µÄÁ¬½ÓÄÚ´æ³Ø£¬HTTPÇëÇó½áÊøÊ±»á
	//Ïú»Ùrequest_pool_sizeÖ¸¶¨µÄHTTPÇëÇóÄÚ´æ³Ø£¬µ«ËüÃÇµÄ´´½¨¡¢Ïú»ÙÊ±¼ä²¢²»Ò»ÖÂ£¬
	//ÒòÎªÒ»¸öTCPÁ¬½Ó¿ÉÄÜ±»¸´ÓÃÓë¶à¸öHTTPÇëÇó
    size_t                      request_pool_size;
	//´æ´¢HTTPÍ·²¿(ÇëÇóÐÐºÍÇëÇóÍ·)Ê±·ÖÅäµÄÄÚ´æbuffer´óÐ¡
    size_t                      client_header_buffer_size;

    ngx_bufs_t                  large_client_header_buffers;
	//¶ÁÈ¡HTTPÍ·²¿(ÇëÇóÐÐºÍÇëÇóÍ·)µÄ³¬Ê±Ê±¼ä
    ngx_msec_t                  client_header_timeout;
	//Èç¹û½«ÆäÉèÖÃÎªoff£¬ÄÇÃ´µ±³öÏÖ²»ºÍ·¨µÄHTTPÍ·²¿Ê±£¬
	//Nginx»á¾Ü¾ø·þÎñ£¬²¢Ö±½ÓÏòÓÃ»§·¢ËÍ400(Bad Request)´íÎó
	//Èç¹û½«ÆäÉèÖÃÎªon£¬Ôò»áºöÂÔ´ËHTTPÍ·²¿
    ngx_flag_t                  ignore_invalid_headers;
	//ÊÇ·ñºÏ²¢ÏàÁÚµÄ'/'
	//ÀýÈç: //test///a.txt£¬ÔÚÅäÖÃÎªonÊ±£¬»á½«ÆäÆ¥ÅäÎª location /test/a.txt;
	//	Èç¹ûÅäÖÃÎªoff£¬Ôò²»»áÆ¥Åä£¬URIÈÔ½«ÊÇ//test///a.txt
    ngx_flag_t                  merge_slashes;
	
    ngx_flag_t                  underscores_in_headers; //ÊÇ·ñÔÊÐíHTTPÍ·²¿µÄÃû³ÆÖÐ´ø'_'(ÏÂ»®Ïß)

    unsigned                    listen:1;			//±íÊ¾ÊÇ·ñÅäÖÃlisten Ö¸Áî
#if (NGX_PCRE)
    unsigned                    captures:1;
#endif

    ngx_http_core_loc_conf_t  **named_locations;
} ngx_http_core_srv_conf_t;


/* list of structures to find core_srv_conf quickly at run time */


typedef struct
{
#if (NGX_PCRE)
    ngx_http_regex_t          *regex;
#endif
    ngx_http_core_srv_conf_t  *server;   /* virtual name server conf */
    ngx_str_t                  name;
} ngx_http_server_name_t;


typedef struct {
     ngx_hash_combined_t       names;

     ngx_uint_t                nregex;
     ngx_http_server_name_t   *regex;
} ngx_http_virtual_names_t;


struct ngx_http_addr_conf_s
{
    /* the default server configuration for this address:port */
	//Õâ¸ö¼àÌýµØÖ·¶ÔÓ¦µÄserver¿éÅäÖÃÐÅÏ¢
    ngx_http_core_srv_conf_t  *default_server;

    ngx_http_virtual_names_t  *virtual_names;

#if (NGX_HTTP_SSL)
    unsigned                   ssl:1;
#endif
#if (NGX_HTTP_V2)
    unsigned                   http2:1;
#endif
    unsigned                   proxy_protocol:1;
};


typedef struct 
{
	/*IPµØÖ·£¬ÍøÂç×Ö½ÚÐò*/
    in_addr_t                  addr;  
    ngx_http_addr_conf_t       conf;
} ngx_http_in_addr_t;


#if (NGX_HAVE_INET6)

typedef struct 
{
    struct in6_addr            addr6;
    ngx_http_addr_conf_t       conf;
} ngx_http_in6_addr_t;

#endif


typedef struct 
{
    //ngx_http_in_addr_t or ngx_http_in6_addr_t 
    void                      *addrs;
	//addrsÖ¸ÏòµÄ¶ÔÏóµÄÔªËØµÄ¸öÊý
    ngx_uint_t                 naddrs;		
} ngx_http_port_t;


typedef struct
{
    ngx_int_t                  family;		//socketµØÖ·¼Ò×å
    in_port_t                  port;		//¼àÌý¶Ë¿Ú(ÍøÂç×Ö½ÚÐò)
    ngx_array_t                addrs;     	//¼àÌý¶Ë¿ÚÏÂ¶ÔÓ¦×ÅµÄËùÓÐngx_http_conf_addr_tµØÖ·
} ngx_http_conf_port_t;



//Ã¿Ò»¸öngx_http_conf_addr_t¶¼»áÓÐÒ»¸öngx_listening_tÓëÆäÏà¶ÔÓ¦
typedef struct 
{
    ngx_http_listen_opt_t      opt;			//¼àÌýÌ×½Ó×ÖµÄ¸÷ÖÖÊôÐÔ

	//ÒÔÏÂ3¸öÉ¢ÁÐ±íÓÃÓÚ¼ÓËÙÑ°ÕÒµ½¶ÔÓ¦¼àÌý¶Ë¿ÚÉÏµÄÐÂÁ¬½Ó£¬È·¶¨µ½µ×Ê¹ÓÃÄÄ¸öserver{}ÐéÄâÖ÷»úÏÂµÄÅäÖÃÀ´´¦ÀíËü¡£
	//ËùÒÔÉ¢ÁÐ±íµÄÖµ¾ÍÊÇngx_http_core_srv_conf_t½á¹¹ÌåµÄµØÖ·
    ngx_hash_t                 hash;		//ÍêÈ«Æ¥Åäserver nameµÄÉ¢ÁÐ±í
    ngx_hash_wildcard_t       *wc_head;		//Í¨Åä·ûÇ°ÖÃµÄÉ¢ÁÐ±í
    ngx_hash_wildcard_t       *wc_tail;		//Í¨Åä·ûºóÖÃµÄÉ¢ÁÐ±í

#if (NGX_PCRE)
    ngx_uint_t                 nregex;		//³ÉÔ±regexÊý×éÖÐµÄÔªËØ¸öÊý
    ngx_http_server_name_t    *regex;		//Ö¸Ïò¾²Ì¬Êý×é£¬ÆäÊý×é³ÉÔ±¾ÍÊÇngx_http_server_name½á¹¹Ìå£¬±íÊ¾ÕýÔò±í´ïÊ½¼°ÆäÆ¥ÅäµÄserver{}ÐéÄâ»ú
#endif

    /* the default server configuration for this address:port */
    ngx_http_core_srv_conf_t  *default_server;	//¸Ã¼àÌý¶Ë¿ÚÏÂ¶ÔÓ¦µÄÄ¬ÈÏserver{}ÐéÄâÖ÷»ú
    ngx_array_t                servers;  	/* array of (ngx_http_core_srv_conf_t*), ¼ÇÂ¼ÏàÍ¬IPÏÂµÄ²»Í¬server{}ÅäÖÃ*/
} ngx_http_conf_addr_t;


typedef struct {
    ngx_int_t                  status;
    ngx_int_t                  overwrite;
    ngx_http_complex_value_t   value;
    ngx_str_t                  args;
} ngx_http_err_page_t;


typedef struct 
{
    ngx_array_t               *lengths;
    ngx_array_t               *values;
    ngx_str_t                  name;

    unsigned                   code:10;
    unsigned                   test_dir:1;
} ngx_http_try_file_t;

//ÍêÕûµÄÃèÐðÁËÒ»¸ölocation¿éµÄÐÅÏ¢£¬
//ÒÔ¼°Æ¥ÅäÕâ¸ölocationµÄÇëÇóµÄhttp°üµÄÒ»Ð©×ÊÔ´ÐÅÏ¢
struct ngx_http_core_loc_conf_s 
{
	//locationÃû³Æ£¬¼´locationÖ¸ÁîºóµÄ±í´ïÊ½
    ngx_str_t     name;        
#if (NGX_PCRE)
	//±íÊ¾ÎªÕýÔòÂ·¾¶
    ngx_http_regex_t  *regex;
#endif
	//±íÊ¾ÎªÎÞÃûÂ·¾¶
    unsigned      noname:1;   			/* "if () {}" block or limit_except */
    unsigned      lmt_excpt:1;			//±íÊ¾µ±Ç°locÅäÖÃÊôÓÚlimit_except{}¿éÄÚµÄÅäÖÃ
	//±íÊ¾ÎªÃüÃûÂ·¾¶£¬½öÓÃÓÚserverÄÚ²¿Ìø×ª
    unsigned      named:1;				
	//±íÊ¾Îª¾«È·Æ¥ÅäµÄÂ·¾¶
    unsigned      exact_match:1;	
	//±íÊ¾ÎªÇÀÕ¼Ê½Ç°×ºÆ¥ÅäµÄÂ·¾¶
    unsigned      noregex:1;

    unsigned      auto_redirect:1;
#if (NGX_HTTP_GZIP)
    unsigned      gzip_disable_msie6:2;
#if (NGX_HTTP_DEGRADATION)
    unsigned      gzip_disable_degradation:2;
#endif
#endif
	//nginxËÑË÷Â·¾¶Ê±£¬ÕýÔòÆ¥ÅäÂ·¾¶ºÍÆäËûÂ·¾¶·Ö¿ªËÑË÷£¬Â·¾¶Ê±¿ÉÒÔÇ¶Ì×µÄ
	//ÆäËûÂ·¾¶°üº¬ÈçÏÂÕâÐ©:
	//1.ÆÕÍ¨µÄÇ°×ºÆ¥ÅäµÄÂ·¾¶£¬ ÀýÈç location / {}
	//2.ÇÀÕ¼Ê½Ç°×ºÆ¥ÅäµÄÂ·¾¶£¬ ÀýÈç location / ^~/ {}
	//3.¾«È·Æ¥ÅäµÄÂ·¾¶£¬ÀýÈç location = / {}
	//4.ÃüÃûÂ·¾¶£¬ ÀýÈç location @a {}
	//5.ÎÞÃûÂ·¾¶£¬ ÀýÈç if {} »òÕß limit_except {}Éú³ÉµÄÂ·¾¶
	
    ngx_http_location_tree_node_t   *static_locations;
#if (NGX_PCRE)
	//ÕýÔò±í´ïÊ½Â·¾¶ÒÔÖ¸ÕëÊý×éµÄÐÎÊ½´æ´¢
    ngx_http_core_loc_conf_t       **regex_locations; 
#endif

    //Ö¸ÏòËùÊôlocation¿éÄÚngx_http_conf_ctx_t½á¹¹ÌåÖÐµÄloc_confÖ¸ÕëÊý×é£¬Ëü±£´æ×Åµ±Ç°location¿éÄÚËùÓÐHTTPÄ£¿écreate_loc_conf·½·¨²úÉúµÄ½á¹¹ÌåÖ¸Õë
    void        **loc_conf;		

    uint32_t      limit_except;
	//Ö¸ÏòËùÊôlocation¿éÄÚµÄlimit_except¿éÄÚngx_http_conf_ctx_t½á¹¹ÌåÖÐµÄloc_confÖ¸ÕëÊý×é£¬Ëü±£´æ×Åµ±Ç°limit_execept¿éÄÚËùÓÐHTTPÄ£¿écreate_loc_conf·½·¨²úÉúµÄ½á¹¹ÌåÖ¸Õë
    void        **limit_except_loc_conf;

    ngx_http_handler_pt  handler;

    /* location name length for inclusive location with inherited alias */
    size_t        alias;
    ngx_str_t     root;                    /* root, alias */
    ngx_str_t     post_action;

    ngx_array_t  *root_lengths;
    ngx_array_t  *root_values;
	/* array of ngx_hash_key_t 
	[config] text/html   html htm shtml;
	[result] key -- html value -- text/html 
	[result] key -- htm value -- text/html
	[result] key -- shtml value -- text/html
	*/
    ngx_array_t  *types;    		/*array of ngx_hash_key_t*/   			
    ngx_hash_t    types_hash;
	//µ±ÕÒ²»µ½ÏàÓ¦µÄMIME typeÓëÎÄ¼þÀ©Õ¹ÃûÖ®¼äµÄÓ³ÉäÊ±£¬
	//Ê¹ÓÃÄ¬ÈÏµÄMIME type×÷ÎªHTTP headerÖÐµÄContent-type
    ngx_str_t     default_type;   /*[config]defatult_type  application/octet-stream*/
	//HTTPÇëÇó°üÌåµÄ×î´óÖµ
    off_t         client_max_body_size;   
	//Ö¸¶¨ÔÚFreeBSDºÍLinuxÏµÍ³ÉÏÊ¹ÓÃO_DIRECTÑ¡ÏîÈ¥¶ÁÈ¡ÎÄ¼þÊ±£¬
	//»º³åÇøµÄ´óÐ¡£¬Í¨³£¶Ô´óÎÄ¼þµÄ¶ÁÈ¡ËÙ¶ÈÓÐÓÅ»¯×÷ÓÃ¡£
	//×¢Òâ£¬ËüÓësendfile¹¦ÄÜÊÇ»¥³âµÄ
    off_t         directio;                
	//ÓëdirectioÅäºÏÊ¹ÓÃ£¬Ö¸¶¨ÒÔdirectio·½Ê½¶ÁÈ¡ÎÄ¼þÊ±µÄ¶ÔÆë·½Ê½¡£
	//Ò»°ãÇé¿öÏÂ512BÒÑ¾­×ã¹»ÁË£¬µ«Õë¶ÔÒ»Ð©¸ßÐÔÄÜÎÄ¼þÏµÍ³£¬ÈçLinuxÏÂµÄXFSÎÄ¼þÏµÍ³£¬
	//¿ÉÄÜÐèÒªÉèÖÃµ½4KB×÷Îª¶ÔÆë·½Ê½
    off_t         directio_alignment;      /* directio_alignment */
	//´æ´¢HTTP°üÌåµÄÄÚ´æ»º³åÇø´óÐ¡
	//HTTP°üÌå»áÏÈ½ÓÊÕµ½Ö¸¶¨µÄÕâ¿é»º´æÖÐ£¬Ö®ºó²Å¾ö¶¨ÊÇ·ñÐ´Èë´ÅÅÌ
	//Èç¹ûÓÃ»§ÇëÇóÖÐ°üº¬ÓÐHTTPÍ·²¿Content-Length£¬²¢ÇÒÆä±êÊ¶µÄ³¤¶ÈÐ¡ÓÚ¶¨ÒåµÄ»º³åÇø£¬
	//ÄÇÃ´Nginx»á×Ô¶¯½µµÍ±¾´ÎÇëÇóËùÊ¹ÓÃµÄÄÚ´æbuffer£¬ÒÔ½µµÍÄÚ´æÏûºÄ
    size_t        client_body_buffer_size; /* client_body_buffer_size */
    size_t        send_lowat;              /* send_lowat */
    size_t        postpone_output;         /* postpone_output */
	//¶Ô¿Í»§¶ËÇëÇóÏÞÖÆÃ¿Ãë´«ÊäµÄ×Ö½ÚÊý£¬0±íÊ¾²»ÏÞËÙ
    size_t        limit_rate;   
	//Ïò¿Í»§¶Ë·¢ËÍµÄÏìÓ¦³¤¶È³¬¹ýlimit_rate_afterºó²Å¿ªÊ¼ÏÞËÙ
    size_t        limit_rate_after;        
    size_t        sendfile_max_chunk;      /* sendfile_max_chunk */
    size_t        read_ahead;              /* read_ahead */
	//¶ÁÈ¡HTTP°üÌåµÄ³¬Ê±Ê±¼ä
    ngx_msec_t    client_body_timeout;     /* client_body_timeout */
	//·¢ËÍÏìÓ¦µÄ³¬Ê±Ê±¼ä
    ngx_msec_t    send_timeout;            /* send_timeout */
	//keepalive³¬Ê±Ê±¼ä
    ngx_msec_t    keepalive_timeout;       
	//±£³ÖÑÓ³Ù¹Ø±Õ×´Ì¬µÄ×î³¤³ÖÐøÊ±¼ä
    ngx_msec_t    lingering_time;          
	//ÑÓ³Ù¹Ø±ÕµÄ³¬Ê±Ê±¼ä
	//ÑÓ³Ù¹Ø±Õ×´Ì¬ÖÐ£¬Èô³¬¹ýlingering_timeoutÊ±¼äºó»¹Ã»ÓÐÊý¾Ý¿É¶Á£¬¾ÍÖ±½Ó¹Ø±ÕÁ¬½Ó
    ngx_msec_t    lingering_timeout;       /* lingering_timeout */
	//DNS½âÎöµÄ³¬Ê±Ê±¼ä
    ngx_msec_t    resolver_timeout;        

    ngx_resolver_t  *resolver;             /* resolver */
	//Nginx·¢ËÍ¸ø¿Í»§¶ËÏìÓ¦Í·ÖÐµÄKeep-AliveÓòµÄÊ±¼ä
    time_t        keepalive_header;        
	//Ò»¸ökeepalive³¤Á¬½ÓÉÏÔÊÐí³ÐÔØµÄÇëÇó×î´óÊýÄ¿
    ngx_uint_t    keepalive_requests;      /* keepalive_requests */
	//¶ÔÄ³Ð©ä¯ÀÀÆ÷½ûÓÃkeepalive¹¦ÄÜ
	//HTTPÇëÇóÖÐµÄkeepalive¹¦ÄÜÊÇÎªÁËÈÃ¶à¸öÇëÇó¸´ÓÃÒ»¸öHTTP³¤Á¬½Ó
	//µ«ÊÇÓÐÐ©ä¯ÀÀÆ÷¶Ô³¤Á¬½Ó´¦ÀíÓÐÎÊÌâ
    ngx_uint_t    keepalive_disable;       
	//½ö¿ÉÈ¡ÖµÎªNGX_HTTP_SATISFY_ALL»òÕßNGX_HTTP_SATISFY_ANY
    ngx_uint_t    satisfy;                 /* satisfy */
	//¿ØÖÆNginx¹Ø±ÕÓÃ»§Á¬½ÓµÄ·½Ê½
	//always±íÊ¾¹Ø±ÕÓÃ»§Á¬½ÓÇ°±ØÐëÎÞÌõ¼þµØ´¦ÀíÁ¬½ÓÉÏËùÓÐÓÃ»§·¢ËÍµÄÊý¾Ý
	//off±íÊ¾¹Ø±ÕÁ¬½ÓÊ±ÍêÈ«²»¹ÜÁ¬½ÓÉÏÊÇ·ñÒÑ¾­ÓÐ×¼±¸¾ÍÐ÷µÄÀ´×ÔÓÃ»§µÄÊý¾Ý
	//onÊÇÖÐ¼äÖµ£¬Ò»°ãÇé¿öÏÂÔÚ¹Ø±ÕÁ¬½ÓÇ°¶¼»á´¦ÀíÁ¬½ÓÉÏµÄÓÃ»§·¢ËÍµÄÊý¾Ý£¬
	//³ýÁËÓÐÐ©Çé¿öÏÂÔÚÒµÎñÉÏÈÏ¶¨ÕâÖ®ºóµÄÊý¾ÝÊÇ²»±ØÒªµÄ
    ngx_uint_t    lingering_close;         /* lingering_close */
	//¶ÔIf_Modified_SinceÍ·²¿µÄ´¦Àí²ßÂÔ
	//´¦ÓÚÐÔÄÜµÄ¿¼ÂÇ£¬Webä¯ÀÀÆ÷Ò»°ã»áÔÚ¿Í»§¶Ë±¾µØ»º´æÒ»Ð©ÎÄ¼þ£¬²¢´æ´¢µ±Ê±»ñÈ¡µÄÊ±¼ä¡£
	//ÕâÑù£¬ÏÂ´ÎÏòWeb·þÎñÆ÷»ñÈ¡»º´æ¹ýµÄ×ÊÔ´Ê±£¬¾Í¿ÉÒÔÓÃIf-Modified-SinceÍ·²¿°ÑÉÏ´Î»ñÈ¡
	//µÄÊ±¼äÉÓ´øÉÏ£¬¶øNginx½«¸ù¾Ýif_modified_sinceµÄÖµÀ´¾ö¶¨ÈçºÎ´¦ÀíIf-Modified-SinceÍ·²¿
	//off--±íÊ¾ºöÂÔÓÃ»§ÇëÇóÖÐµÄIf_Modified_SinceÍ·²¿¡£ÕâÊ±£¬Èç¹û»ñÈ¡Ò»¸öÎÄ¼þ£¬
	//	ÄÇÃ´»áÕý³£µØ·µ»ØÎÄ¼þÄÚÈÝ¡£HTTPÏìÓ¦ÂëÍ¨³£ÊÇ200
	//exact--½«If_Modified_SinceÍ·²¿°üº¬µÄÊ±¼äÓë½«Òª·µ»ØµÄÎÄ¼þÉÏ´ÎÐÞ¸ÄµÄÊ±¼ä×ö¾«È·±È½Ï£¬
	//	Èç¹ûÃ»ÓÐÆ¥ÅäÉÏ£¬Ôò·µ»Ø200ºÍÎÄ¼þµÄÊµ¼ÊÄÚÈÝ£¬Èç¹ûÆ¥ÅäÉÏ£¬Ôò±íÊ¾ä¯ÀÀÆ÷»º´æµÄÎÄ¼þÄÚÈÝÒÑ¾­ÊÇ×îÐÂµÄÁË£¬
	//	Ã»ÓÐ±ØÒªÔÙ·µ»ØÎÄ¼þ´Ó¶øÀË·ÑÊ±¼äÓë´ø¿íÁË£¬ÕâÊ±»á·µ»Ø304 Not Modified£¬ä¯ÀÀÆ÷ÊÕµ½ºó»áÖ±½Ó¶ÁÈ¡×Ô¼ºµÄ±¾µØ»º´æ
	//before--ÊÇ±Èexact¸ü¿íËÉµÄ±È½Ï¡£Ö»ÒªÎÄ¼þµÄÉÏ´ÎÐÞ¸ÄÊ±¼äµÈÓÚ»òÔçÓÚÓÃ»§ÇëÇóÖÐµÄIf_Modified_SinceÍ·²¿Ê±¼ä£¬
	//	¾Í»áÏò¿Í»§¶Ë·µ»Ø304 Not Modified¡£
    ngx_uint_t    if_modified_since;       
    ngx_uint_t    max_ranges;              /* max_ranges */
    ngx_uint_t    client_body_in_file_only; /* client_body_in_file_only */

	//
    ngx_flag_t    client_body_in_single_buffer;
                                           /* client_body_in_singe_buffer */
	//ÄÚ²¿location£¬ÕâÖÖlocationÖ»ÄÜÓÉ×ÓÇëÇó»òÕßÄÚ²¿Ìø×ª·ÃÎÊ
    ngx_flag_t    internal;                /* internal */
	//È·¶¨ÊÇ·ñÆôÓÃsendfileÏµÍ³µ÷ÓÃÀ´·¢ËÍÎÄ¼þ
	//sendfileÏµÍ³µ÷ÓÃ¼õÉÙÁËÄÚºËÌ¬ÓëÓÃ»§Ì¬Ö®¼äµÄÁ½´ÎÄÚ´æ¸´ÖÆ£¬ÕâÑù¾Í»á´Ó´ÅÅÌÖÐ¶ÁÈ¡ÎÄ¼þºóÖ±½Ó
	//ÔÚÄÚºËÌ¬·¢ËÍµ½Íø¿¨Éè±¸£¬Ìá¸ßÁË·¢ËÍÎÄ¼þµÄÐ§ÂÊ
    ngx_flag_t    sendfile;        

	//ÊÇ·ñÔÚFreeBSD»òLinuxÏµÍ³ÉÏÆôÓÃÄÚºË¼¶±ðµÄÒì²½ÎÄ¼þI/O¹¦ÄÜ¡£
	//×¢Òâ: ËüÓësendfile¹¦ÄÜÊÇ»¥³âµÄ
    ngx_flag_t    aio;                   
	
	//È·¶¨ÊÇ·ñ¿ªÆôFreeBSDÏµÍ³ÉÏµÄTCP_NOPUSH»òLinuxÏµÍ³ÉÏµÄTCP_CORK¹¦ÄÜ¡£
	//½öÔÚÊ¹ÓÃsendfileµÄÊ±ºò²ÅÓÐÐ§
	//´ò¿ªtcp_nopushºó£¬½«»áÔÚ·¢ËÍÏìÓ¦Ê±°ÑÕû¸öÏìÓ¦°üÍ··Åµ½Ò»¸öTCP°üÖÐ·¢ËÍ
    ngx_flag_t    tcp_nopush;     
	
	//¿ªÆô»ò¹Ø±ÕNginxÊ¹ÓÃTCP_NODELAYÑ¡ÏîµÄ¹¦ÄÜ 
	//Õâ¸öÑ¡ÏîÔÚ½«Á¬½Ó×ª±äÎª³¤Á¬½ÓµÄÊ±ºò²Å±»ÆôÓÃ£¬ÔÚupstream·¢ËÍÏìÓ¦µ½¿Í»§¶ËÊ±Ò²»áÆôÓÃ
    ngx_flag_t    tcp_nodelay;      
	
	//Á¬½Ó³¬Ê±ºó½«Í¨¹ýÏò¿Í»§¶Ë·¢ËÍRST°üÀ´Ö±½ÓÖØÖÃÁ¬½Ó
	//Ïà±ÈÕý³£µÄ¹Ø±Õ·½Ê½£¬ËüÊ¹µÃ·þÎñÆ÷±ÜÃâ²úÉúÐí¶à´¦ÓÚFIN_WAIT_1,FIN_WAIT_2/TIME_WAIT×´Ì¬µÄTCPÁ¬½Ó
	//×¢Òâ£¬Ê¹ÓÃRSTÖØÖÃ°ü¹Ø±ÕÁ¬½Ó»á´øÀ´Ò»Ð©ÎÊÌâ£¬Ä¬ÈÏÇé¿öÏÂ²»»á¿ªÆô
    ngx_flag_t    reset_timedout_connection; 
	//ÖØ¶¨ÏòÖ÷»úÃû³ÆµÄ´¦Àí£¬ÐèÒªÅäºÏserver_nameÅäÖÃÊ¹ÓÃ
	//on--±íÊ¾ÔÚÖØ¶¨ÏòÇëÇóÊ±»áÊ¹ÓÃserver_nameÀïÅäÖÃµÄµÚÒ»¸öÖ÷»úÃû´úÌæÔ­ÏÈÇëÇóµÄHostÍ·²¿£¬
	//off--±íÊ¾ÔÚÖØ¶¨ÏòÇëÇóÊ±Ê¹ÓÃÇëÇó±¾ÉíµÄHostÍ·²¿
    ngx_flag_t    server_name_in_redirect; 
    ngx_flag_t    port_in_redirect;        /* port_in_redirect */
    ngx_flag_t    msie_padding;            /* msie_padding */
    ngx_flag_t    msie_refresh;            /* msie_refresh */
	//µ±´¦ÀíÓÃ»§ÇëÇóÇÒÐèÒª·ÃÎÊÎÄ¼þÊ±£¬Èç¹ûÃ»ÓÐÕÒµ½ÎÄ¼þ£¬ÊÇ·ñ½«´íÎóÈÕÖ¾
	//¼ÇÂ¼µ½erro.logÎÄ¼þÖÐ¡£Õâ½öÓÃÓÚ¶¨Î»ÎÊÌâ
	ngx_flag_t    log_not_found;           
    ngx_flag_t    log_subrequest;          /* log_subrequest */
    ngx_flag_t    recursive_error_pages;   /* recursive_error_pages */
	//´¦ÀíÇëÇó³ö´íÊ±ÊÇ·ñÔÚÏìÓ¦µÄServerÍ·²¿ÖÐ±íÃ÷Nginx°æ±¾£¬ÕâÊÇÎªÁË·½±ãÎÊÌâ
    ngx_flag_t    server_tokens;          
    ngx_flag_t    chunked_transfer_encoding; /* chunked_transfer_encoding */
    ngx_flag_t    etag;                    /* etag */

#if (NGX_HTTP_GZIP)
    ngx_flag_t    gzip_vary;               /* gzip_vary */

    ngx_uint_t    gzip_http_version;       /* gzip_http_version */
    ngx_uint_t    gzip_proxied;            /* gzip_proxied */

#if (NGX_PCRE)
    ngx_array_t  *gzip_disable;            /* gzip_disable */
#endif
#endif

#if (NGX_THREADS)
    ngx_thread_pool_t         *thread_pool;
    ngx_http_complex_value_t  *thread_pool_value;
#endif

#if (NGX_HAVE_OPENAT)
    ngx_uint_t    disable_symlinks;        /* disable_symlinks */
    ngx_http_complex_value_t  *disable_symlinks_from;
#endif

    ngx_array_t  *error_pages;             /* error_page */
    ngx_http_try_file_t    *try_files;     /* try_files */

	//HTTP°üÌåµÄÁÙÊ±´æ·ÅÄ¿Â¼
    ngx_path_t   *client_body_temp_path;   /* client_body_temp_path */

	//ÎÄ¼þ»º´æ£¬Í¨¹ý¶ÁÈ¡»º´æ¾Í¼õÉÙÁË¶Ô´ÅÅÌµÄ²Ù×÷
    ngx_open_file_cache_t  *open_file_cache;
	//¼ìÑéÎÄ¼þ»º´æÖÐÔªËØÓÐÐ§ÐÔµÄÆµÂÊ
    time_t        open_file_cache_valid;
	//Ö¸¶¨ÎÄ¼þ»º´æÖÐ£¬ÔÚinactiveÖ¸¶¨µÄÊ±¼ä¶ÎÄÚ£¬·ÃÎÊ´ÎÊý³¬¹ýÁË
	//open_file_cache_min_usesÖ¸¶¨µÄ×îÐ¡´ÎÊý£¬ÄÇÃ´½«²»»á±»ÌÔÌ­³ö»º´æ
    ngx_uint_t    open_file_cache_min_uses;
	//ÊÇ·ñÔÚÎÄ¼þ»º´æÖÐ»º´æ´ò¿ªÎÄ¼þÊ±³öÏÖµÄÕÒ²»µ½Â·¾¶¡¢Ã»ÓÐÈ¨ÏÞµÈ´íÎóÐÅÏ¢
    ngx_flag_t    open_file_cache_errors;
    ngx_flag_t    open_file_cache_events;

    ngx_log_t    *error_log;
	//ÎªÁË¿ìËÙÑ°ÕÒµ½ÏàÓ¦MIME type£¬NginxÊ¹ÓÃÉ¢ÁÐ±íÀ´´æ´¢ÎÄ¼þÀ©Õ¹Ãûµ½MIME typeµÄÓ³Éä
	//types_hash_max_sizeÉ¢ÁÐ±íÖÐµÄ³åÍ»Í°µÄ¸öÊý
    ngx_uint_t    types_hash_max_size;
	//ÎªÁË¿ìËÙÑ°ÕÒµ½ÏàÓ¦MIME type£¬NginxÊ¹ÓÃÉ¢ÁÐ±íÀ´´æ´¢ÎÄ¼þÀ©Õ¹Ãûµ½MIME typeµÄÓ³Éä
	//types_hash_bucket_sizeÉèÖÃÁËÉ¢ÁÐ±íÖÐÃ¿¸ö³åÍ»Í°Í°Õ¼ÓÃµÄÄÚ´æ´óÐ¡
    ngx_uint_t    types_hash_bucket_size;

	//ÊôÓÚµ±Ç°¿éµÄËùÓÐlocation¿éÍ¨¹ýngx_http_location_queue_t½á¹¹Ìå¹¹³ÉµÄË«ÏòÁ´±í
    ngx_queue_t  *locations;   		//ngx_http_location_queue_tÀàÐÍµÄË«Á´±í£¬½«Í¬Ò»¸öserver¿éÄÚ¶à¸ö±í´ïlocation¿éµÄngx_http_core_loc_conf_t½á¹¹ÌåÒÔË«ÏòÁ´±í·½Ê½×éÖ¯ÆðÀ´

#if 0
    ngx_http_core_loc_conf_t  *prev_location;
#endif
};


typedef struct
{
    ngx_queue_t                      queue;  	//queue½«×÷Îªngx_queue_tË«ÏòÁ´±íÈÝÆ÷£¬´Ó¶ø½«ngx_http_location_queue_t½á¹¹ÌåÁ¬½ÓÆðÀ´
    ngx_http_core_loc_conf_t        *exact;		//Èç¹ûlocationÖÐµÄ×Ö·û´®¿ÉÒÔ¾«È·Æ¥Åä(°üÀ¨ÕýÔò±í´ïÊ½)£¬exact½«Ö¸Ïò¶ÔÓ¦µÄngx_http_core_loc_conf_t½á¹¹Ìå£¬·ñÔòÖµÎªNULL
    ngx_http_core_loc_conf_t        *inclusive;	//Èç¹ûlocationÖÐµÄ×Ö·û´®ÎÞ·¨¾«È·Æ¥Åä(°üÀ¨ÁË×Ô¶¨ÒåµÄÍ¨Åä·û),inclusizve½«Ö¸Ïò¶ÔÓ¦µÄngx_http_core_loc_conf_t½á¹¹Ìå£¬·ñÔòÖµÎªNULL
    ngx_str_t                       *name;		//Ö¸ÏòlocationµÄÃû³Æ
    u_char                          *file_name;	//ÅäÖÃÎÄ¼þÃû
    ngx_uint_t                       line;		//ÃüÁîÔÚÅäÖÃÎÄ¼þÖÐµÄÐÐºÅ(location|limit_exceptµÈ)
    ngx_queue_t                      list;
} ngx_http_location_queue_t;


struct ngx_http_location_tree_node_s 
{
    ngx_http_location_tree_node_t   *left;		//×ó×ÓÊ÷
    ngx_http_location_tree_node_t   *right;		//ÓÒ×ÓÊ÷
    ngx_http_location_tree_node_t   *tree;		//ÎÞ·¨ÍêÈ«Æ¥ÅäµÄlocation×é³ÉµÄÊ÷

    ngx_http_core_loc_conf_t        *exact;		//Èç¹ûlocation¶ÔÓ¦µÄURIÆ¥Åä×Ö·û´®ÊôÓÚÄÜ¹»ÍêÈ«Æ¥ÅäµÄÀàÐÍ£¬ÔòexactÖ¸ÏòÆä¶ÔÓ¦µÄngx_http_core_loc_conf_t½á¹¹Ìå£¬·ñÔòÎªNULL
    //Èç¹ûlocation¶ÔÓ¦µÄURIÆ¥Åä×Ö·û´®ÊôÓÚÎÞ·¨ÍêÈ«Æ¥ÅäµÄÀàÐÍ£¬ÔòinclusiveÖ¸ÏòÆä¶ÔÓ¦µÄngx_http_core_loc_conf_t½á¹¹Ìå£¬·ñÔòÎªNULL
    ngx_http_core_loc_conf_t        *inclusive;	
	//×Ô¶¯ÖØ¶¨Ïò±êÖ¾
    u_char                           auto_redirect;	
	//name×Ö·û´®Êµ¼Ê³¤¶È
    u_char                           len;		
	//nameÖ¸Ïòlocation¶ÔÓ¦µÄURIÆ¥Åä±í´ïÊ½
    u_char                           name[1];	
};


void ngx_http_core_run_phases(ngx_http_request_t *r);
ngx_int_t ngx_http_core_generic_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_rewrite_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_find_config_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_post_rewrite_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_access_phase(ngx_http_request_t *r, ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_post_access_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_try_files_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);
ngx_int_t ngx_http_core_content_phase(ngx_http_request_t *r,
    ngx_http_phase_handler_t *ph);


void *ngx_http_test_content_type(ngx_http_request_t *r, ngx_hash_t *types_hash);
ngx_int_t ngx_http_set_content_type(ngx_http_request_t *r);
void ngx_http_set_exten(ngx_http_request_t *r);
ngx_int_t ngx_http_set_etag(ngx_http_request_t *r);
void ngx_http_weak_etag(ngx_http_request_t *r);
ngx_int_t ngx_http_send_response(ngx_http_request_t *r, ngx_uint_t status, ngx_str_t *ct, ngx_http_complex_value_t *cv);
u_char *ngx_http_map_uri_to_path(ngx_http_request_t *r, ngx_str_t *name,
    size_t *root_length, size_t reserved);
ngx_int_t ngx_http_auth_basic_user(ngx_http_request_t *r);
#if (NGX_HTTP_GZIP)
ngx_int_t ngx_http_gzip_ok(ngx_http_request_t *r);
#endif


ngx_int_t ngx_http_subrequest(ngx_http_request_t *r,
    ngx_str_t *uri, ngx_str_t *args, ngx_http_request_t **sr,
    ngx_http_post_subrequest_t *psr, ngx_uint_t flags);
ngx_int_t ngx_http_internal_redirect(ngx_http_request_t *r,
    ngx_str_t *uri, ngx_str_t *args);
ngx_int_t ngx_http_named_location(ngx_http_request_t *r, ngx_str_t *name);


ngx_http_cleanup_t *ngx_http_cleanup_add(ngx_http_request_t *r, size_t size);


typedef ngx_int_t (*ngx_http_output_header_filter_pt)(ngx_http_request_t *r);
typedef ngx_int_t (*ngx_http_output_body_filter_pt)(ngx_http_request_t *r, ngx_chain_t *chain);
typedef ngx_int_t (*ngx_http_request_body_filter_pt) (ngx_http_request_t *r, ngx_chain_t *chain);


ngx_int_t ngx_http_output_filter(ngx_http_request_t *r, ngx_chain_t *chain);
ngx_int_t ngx_http_write_filter(ngx_http_request_t *r, ngx_chain_t *chain);
ngx_int_t ngx_http_request_body_save_filter(ngx_http_request_t *r,
   ngx_chain_t *chain);


ngx_int_t ngx_http_set_disable_symlinks(ngx_http_request_t *r,
    ngx_http_core_loc_conf_t *clcf, ngx_str_t *path, ngx_open_file_info_t *of);

ngx_int_t ngx_http_get_forwarded_addr(ngx_http_request_t *r, ngx_addr_t *addr,
    ngx_array_t *headers, ngx_str_t *value, ngx_array_t *proxies,
    int recursive);


extern ngx_module_t  ngx_http_core_module;

extern ngx_uint_t ngx_http_max_module;

extern ngx_str_t  ngx_http_core_get_method;


#define ngx_http_clear_content_length(r)                                      \
                                                                              \
    r->headers_out.content_length_n = -1;                                     \
    if (r->headers_out.content_length) {                                      \
        r->headers_out.content_length->hash = 0;                              \
        r->headers_out.content_length = NULL;                                 \
    }

#define ngx_http_clear_accept_ranges(r)                                       \
                                                                              \
    r->allow_ranges = 0;                                                      \
    if (r->headers_out.accept_ranges) {                                       \
        r->headers_out.accept_ranges->hash = 0;                               \
        r->headers_out.accept_ranges = NULL;                                  \
    }

#define ngx_http_clear_last_modified(r)                                       \
                                                                              \
    r->headers_out.last_modified_time = -1;                                   \
    if (r->headers_out.last_modified) {                                       \
        r->headers_out.last_modified->hash = 0;                               \
        r->headers_out.last_modified = NULL;                                  \
    }

#define ngx_http_clear_location(r)                                            \
                                                                              \
    if (r->headers_out.location) {                                            \
        r->headers_out.location->hash = 0;                                    \
        r->headers_out.location = NULL;                                       \
    }

#define ngx_http_clear_etag(r)                                                \
                                                                              \
    if (r->headers_out.etag) {                                                \
        r->headers_out.etag->hash = 0;                                        \
        r->headers_out.etag = NULL;                                           \
    }


#endif /* _NGX_HTTP_CORE_H_INCLUDED_ */
