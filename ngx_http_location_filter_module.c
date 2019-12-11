/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ip_parse.c"

static ngx_int_t ngx_http_location_filter_init(ngx_conf_t *cf);
static void *ngx_http_location_filter_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_location_filter_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

typedef struct {
	ngx_flag_t enable;
	ngx_str_t  white_list;
} ngx_http_location_filter_conf_t;


static ngx_http_module_t  ngx_http_location_filter_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_location_filter_init,          /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_location_filter_create_loc_conf,                                  /* create location configuration */
    ngx_http_location_filter_merge_loc_conf                                   /* merge location configuration */
};

static ngx_command_t ngx_http_location_filter_commands[] = {
	{ ngx_string("location_filter"),
	  NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_FLAG,
	  ngx_conf_set_flag_slot,
	  NGX_HTTP_LOC_CONF_OFFSET,
	  offsetof(ngx_http_location_filter_conf_t, enable),
	  NULL },
	{ ngx_string("location_filter_white_list"),
	  NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
	  ngx_conf_set_str_slot,
	  NGX_HTTP_LOC_CONF_OFFSET,
	  offsetof(ngx_http_location_filter_conf_t, white_list),
	  NULL },
	ngx_null_command
};

ngx_module_t  ngx_http_location_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_location_filter_module_ctx,   /* module context */
    ngx_http_location_filter_commands,                                  /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t
ngx_http_location_header_filter(ngx_http_request_t *r)
{
	ngx_http_location_filter_conf_t *conf;
	conf = ngx_http_get_module_loc_conf(r, ngx_http_location_filter_module);
	if(conf->enable == 0){
		return NGX_OK;
	}

	/*get the ip location address
	  use the ip lib from cz88.net
	*/
	char *location_addr;
	location_addr = (char*)ngx_palloc(r->pool, MAXBUF);
	struct sockaddr_in *sin = (struct sockaddr_in*)(r->connection->sockaddr);
	getLocation(inet_ntoa(sin->sin_addr), location_addr);

	/*the white list*/
	char *white_location;
	const char delim[2] = " ";
	white_location = strtok((char *)conf->white_list.data, delim);
	ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          location_addr);
	/*comparison*/
	while(white_location != NULL){
		if(strstr(location_addr, white_location) != NULL) {
			ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "i am here");
			return NGX_OK;
		}
     	white_location = strtok(NULL, delim);
	}

	/*if not allowed, send 403*/
    return NGX_HTTP_FORBIDDEN;
}

static ngx_int_t
ngx_http_location_filter_init(ngx_conf_t *cf)
{
	ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_location_header_filter;
    return NGX_OK;
}

static void* ngx_http_location_filter_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_location_filter_conf_t *lfcf;
 
	lfcf = (ngx_http_location_filter_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_http_location_filter_conf_t));
	if (lfcf == NULL)
		return NULL;
 
	lfcf->enable = NGX_CONF_UNSET;
	return lfcf;
}
 
static char* ngx_http_location_filter_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
	ngx_http_location_filter_conf_t *prev = (ngx_http_location_filter_conf_t *)parent;
	ngx_http_location_filter_conf_t *conf = (ngx_http_location_filter_conf_t *)child;
 
	ngx_conf_merge_value(conf->enable, prev->enable, 0);
	ngx_conf_merge_str_value(conf->white_list, prev->white_list, "");
 
	return NGX_CONF_OK;
}

