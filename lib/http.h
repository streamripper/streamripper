#ifndef __HTTP_H__
#define __HTTP_H__

#include "rip_manager.h"
#include "srtypes.h"


error_code http_parse_sc_header(const char* url, char *header, SR_HTTP_HEADER *info);
error_code http_construct_sc_request(const char *url, const char* proxyurl, char *buffer, char *useragent);
error_code http_construct_page_request(const char *url, BOOL proxyformat, char *buffer);
error_code http_construct_sc_response(SR_HTTP_HEADER *info, char *header, int size, int icy_meta_support);
error_code inet_get_webpage_alloc(HSOCKET *sock, const char *url,
					 const char *proxyurl, 
					 char **buffer, unsigned long *size);
error_code http_sc_connect (RIP_MANAGER_INFO* rmi,
			    HSOCKET *sock, const char *url, 
			    const char *proxyurl, 
			    SR_HTTP_HEADER *info, char *useragent, 
			    char *if_name);


#endif //__HTTP_H__
