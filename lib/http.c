/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "sr_compat.h"
#include "srtypes.h"
#include "socklib.h"
#include "http.h"
#include "mchar.h"  /* for substrn_until, etc. */
#include "debug.h"

/******************************************************************************
 * Function prototypes
 *****************************************************************************/
static char* make_auth_header(const char *header_name, 
			      const char *username, const char *password);
static char* b64enc(const char *buf, int size);
static error_code
http_get_pls (RIP_MANAGER_INFO* rmi, HSOCKET *sock, SR_HTTP_HEADER *info);
static error_code
http_get_m3u (RIP_MANAGER_INFO* rmi, HSOCKET *sock, SR_HTTP_HEADER *info);
static error_code
http_get_sc_header(RIP_MANAGER_INFO* rmi, const char* url, 
		   HSOCKET *sock, SR_HTTP_HEADER *info);

/******************************************************************************
 * Private Vars
 *****************************************************************************/
#define MAX_PLS_LEN 8192
#define MAX_M3U_LEN 8192


/******************************************************************************
 * Public functions
 *****************************************************************************/
/* Connect to a shoutcast type stream, leaves when it's about to 
   get the header info */
error_code
http_sc_connect (RIP_MANAGER_INFO* rmi,
		 HSOCKET *sock, const char *url, const char *proxyurl, 
		 SR_HTTP_HEADER *info, char *useragent, char *if_name)
{
    char headbuf[MAX_HEADER_LEN];
    URLINFO url_info;
    int ret;

    while (1) {
	debug_printf ("***** URL = %s *****\n", url);
	debug_printf("http_sc_connect(): calling http_parse_url\n");
	if (proxyurl) {
	    debug_printf ("***** PROXY = %s *****\n", proxyurl);
	    if ((ret = http_parse_url (proxyurl, &url_info)) != SR_SUCCESS) {
		return ret;
	    }
	} else if ((ret = http_parse_url (url, &url_info)) != SR_SUCCESS) {
	    return ret;
	}

	debug_printf("http_sc_connect(): calling socklib_init\n");
	if ((ret = socklib_init()) != SR_SUCCESS)
	    return ret;

	debug_printf ("http_sc_connect(): calling socklib_open"
		      " host=%s, port=%d\n", url_info.host, url_info.port);
	ret = socklib_open (sock, url_info.host, url_info.port, if_name, 
			    rmi->prefs->timeout);
	if (ret != SR_SUCCESS) {
	    return ret;
	}

	debug_printf("http_sc_connect(): calling http_construct_sc_request\n");
	ret = http_construct_sc_request (url, proxyurl, headbuf, useragent);
	if (ret != SR_SUCCESS) {
	    return ret;
	}

	debug_printf("http_sc_connect(): calling socklib_sendall\n");
	ret = socklib_sendall (sock, headbuf, strlen(headbuf));
	if (ret < 0) {
	    return ret;
	}

	debug_printf("http_sc_connect(): calling http_get_sc_header\n");
	if ((ret = http_get_sc_header(rmi, url, sock, info)) != SR_SUCCESS)
	    return ret;

	if (*info->http_location) {
	    /* RECURSIVE CASE */
	    debug_printf ("Redirecting: %s\n", info->http_location);
	    url = info->http_location;
	    return http_sc_connect (rmi, sock, info->http_location, 
				    proxyurl, info, useragent, if_name);
	} else {
	    break;
	}
    }

    return SR_SUCCESS;
}

/******************************************************************************
 * Private functions
 *****************************************************************************/
/* See http://www.ietf.org/rfc/rfc3986.txt */
static void
unescape_pct_encoding (char* s)
{
    char* d = s;
    char c[3];

    while (*s) {
	if (*s == '%' && isxdigit(*(s+1)) && isxdigit(*(s+2))) {
	    c[0] = *(s+1);
	    c[1] = *(s+2);
	    c[2] = 0;
	    *d++ = (char) strtol (c, 0, 16);
	    s += 3;
	} else {
	    *d++ = *s++;
	}
    }
    *d = 0;
}

/*
 * Parse's a url as in http://host:port/path or host/path, etc..
 * and now http://username:password@server:4480
 */
error_code
http_parse_url(const char *url, URLINFO *urlinfo)
{ 
    int ret;
    char *s;
    const char *start_path;
    const char *start_login;
    const char *start_port;

    debug_printf ("http_parse_url: %s\n", url);

    /* if we have a proto, just skip it. should we care about
       the proto? like fail if it's not http? */
    s = strstr(url, "://");
    if (s) url = s + strlen("://");
    memcpy(urlinfo->path, (void *)"/\0", 2);

    /* the path may contain ':' as well, so
       check it there is a path */
    start_path = strchr(url, '/');
    if (NULL == start_path) {
        /* no path found, just assume the end of the string */
        start_path = url + strlen(url);
    }

    /* search for a login '@' token */
    start_login = strchr(url, '@');
    if ((start_login != NULL) && (start_login < start_path)) {
        ret = sscanf(url, "%1023[^:]:%1023[^@]",
                     urlinfo->username, urlinfo->password);
        if (ret < 1) {
            return SR_ERROR_PARSE_FAILURE;
        } else if (ret == 1) {
            urlinfo->password[0] = '\0';
        }
        url = start_login + 1;
        debug_printf ("Username (escaped): %s\n", urlinfo->username);
        debug_printf ("Password (escaped): %s\n", urlinfo->password);
        unescape_pct_encoding (urlinfo->username);
        unescape_pct_encoding (urlinfo->password);
        debug_printf ("Username (unescaped): %s\n", urlinfo->username);
        debug_printf ("Password (unescaped): %s\n", urlinfo->password);
    } else {
        urlinfo->username[0] = '\0';
        urlinfo->password[0] = '\0';
    }

    /* search for a port seperator */
    start_port = strchr(url, ':');
    if ((start_port != NULL) && (start_port < start_path)) {
        debug_printf ("Branch 1 (%s)\n", url);
        ret = sscanf(url, "%511[^:]:%hu/%252s", urlinfo->host,
                     (short unsigned int*)&urlinfo->port, urlinfo->path+1);
        if (urlinfo->port < 1) return SR_ERROR_PARSE_FAILURE;
        ret -= 1;
    } else {
        debug_printf ("Branch 2 (%s)\n", url);
        urlinfo->port = 80;
        ret = sscanf(url, "%511[^/]/%252s", urlinfo->host, urlinfo->path+1);
    }
    if (ret < 1) return SR_ERROR_INVALID_URL;

    return SR_SUCCESS;
}

error_code
http_construct_sc_request(const char *url, const char* proxyurl, char *buffer, char *useragent)
{
    int ret;
    URLINFO ui;
    URLINFO proxyui;
    char myurl[MAX_URL_LEN];
    if ((ret = http_parse_url(url, &ui)) != SR_SUCCESS)
	return ret;

    if (proxyurl) {
	sprintf(myurl, "http://%s:%d%s", ui.host, ui.port, ui.path);
	if ((ret = http_parse_url(proxyurl, &proxyui)) != SR_SUCCESS)
	    return ret;
    } else {
	strcpy(myurl, ui.path);
    }

#if defined (commentout)
    /* This is the old header */
    snprintf(buffer, MAX_HEADER_LEN + MAX_HOST_LEN + SR_MAX_PATH,
	     "GET %s HTTP/1.0\r\n"
	     "Host: %s:%d\r\n"
	     "User-Agent: %s\r\n"
	     "Icy-MetaData:1\r\n", 
	     myurl, 
	     ui.host, 
	     ui.port, 
	     useragent[0] ? useragent : "Streamripper/1.x");
#endif

    /* This is the header suggested Florian Stoehr */
    snprintf(buffer, MAX_HEADER_LEN + MAX_HOST_LEN + SR_MAX_PATH,
	     "GET %s HTTP/1.1\r\n"
	     "Accept: */*\r\n"
	     "Cache-Control: no-cache\r\n"
	     "User-Agent: %s\r\n"
	     "Icy-Metadata: 1\r\n"
	     "Connection: close\r\n"
	     "Host: %s:%d\r\n",
	     myurl,
	     useragent[0] ? useragent: "Streamripper/1.x",
	     ui.host,
	     ui.port);

    // http authentication (not proxy, see below for that)
    if (ui.username[0] && ui.password[0]) {
	char *authbuf = make_auth_header("Authorization", 
					 ui.username,
					 ui.password);
	strcat(buffer, authbuf);
	free(authbuf);
    }

#if defined (commentout)
    // proxy auth stuff
    if (proxyurl && proxyui.username[0] && proxyui.password[0]) {
	char *authbuf = make_auth_header("Proxy-Authorization",
					 proxyui.username,
					 proxyui.password);
	strcat(buffer, authbuf);
	free(authbuf);
    }
#endif
    /* GCS Testing... Proxy authentication w/o password bug */
    if (proxyurl && proxyui.username[0]) {
	char *authbuf = make_auth_header("Proxy-Authorization",
					 proxyui.username,
					 proxyui.password);
	strcat(buffer, authbuf);
	free(authbuf);
    }

    
    strcat(buffer, "\r\n");

    debug_printf ("SC REQUEST:\n%s", buffer);

    return SR_SUCCESS;
}

// Make the 'Authorization: Basic xxxxxx\r\n' or 'Proxy-Authorization...' 
// headers for the HTTP request.
static char*
make_auth_header (const char *header_name, const char *username, 
		  const char *password) 
{
    char *authbuf = malloc(strlen(header_name) 
			   + strlen(username)
			   + strlen(password)
			   + MAX_URI_STRING);
    char *auth64;
    sprintf(authbuf, "%s:%s", username, password);
    auth64 = b64enc(authbuf, strlen(authbuf));
    sprintf(authbuf, "%s: Basic %s\r\n", header_name, auth64);
    free(auth64);
    return authbuf;
}

// Here we pretend we're IE 5, hehe
error_code
http_construct_page_request (const char *url, BOOL proxyformat, char *buffer)
{
    int ret;
    URLINFO ui;
    char myurl[MAX_URL_LEN];
    if ((ret = http_parse_url(url, &ui)) != SR_SUCCESS)
	return ret;

    if (proxyformat)
	sprintf(myurl, "http://%s:%d%s", ui.host, ui.port, ui.path);
    else
	strcpy(myurl, ui.path);

    snprintf(buffer, MAX_HEADER_LEN + MAX_HOST_LEN + SR_MAX_PATH,
		"GET %s HTTP/1.0\r\n"
		"Host: %s:%d\r\n"
		"Accept: */*\r\n"
		"Accept-Language: en-us\r\n"
		"Accept-Encoding: gzip, deflate\r\n"
		"User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n"
		"Connection: Keep-Alive\r\n\r\n", 
		myurl, 
		ui.host, 
		ui.port);


    return SR_SUCCESS;
}

/* Return 1 if a match was found, 0 if not found */
int
extract_header_value (char *header, char *dest, char *match, int maxlen)
{
    char *start = header;
    size_t len = strlen (match);
    while (*start) {
	if (!g_ascii_strncasecmp (start, match, len)) {
	    subnstr_until (start + len, "\n", dest, maxlen);
	    return 1;
	}
	start ++;
    }
    return 0;
}

error_code
http_parse_sc_header (const char *url, char *header, SR_HTTP_HEADER *info)
{
    int rc;
    char *start;
    char versionbuf[64];
    char stempbr[MAX_ICY_STRING];
    URLINFO url_info;
    int url_path_len;
    int content_type_by_url;

    if (!header || !info)
	return SR_ERROR_INVALID_PARAM;

    /* Parse the url here, before doing memset, because in the 
       recursive case, it is the location referrer */
    rc = http_parse_url (url, &url_info);
    if (rc != SR_SUCCESS) return rc;

    memset(info, 0, sizeof(SR_HTTP_HEADER));

    debug_printf("http header:\n%s\n", header);

    // Get the ICY code.
    start = (char*) strstr (header, "ICY ");
    if (!start) {
	start = (char*) strstr (header, "HTTP/1.");
	if (!start) {
	    debug_printf ("Failed to find ICY or HTTP\n");
	    return SR_ERROR_NO_RESPONSE_HEADER;
	}
    }
    start = strstr (start, " ") + 1;
    sscanf (start, "%i", &info->icy_code);
    if (info->icy_code >= 400) {
	switch (info->icy_code) {
	case 400:
	    return SR_ERROR_HTTP_400_ERROR;
	case 404:
	    return SR_ERROR_HTTP_404_ERROR;
	case 401:
	    return SR_ERROR_HTTP_401_ERROR;
	case 403:
	    return SR_ERROR_HTTP_403_ERROR;
	case 407:
	    return SR_ERROR_HTTP_407_ERROR;
	case 502:
	    return SR_ERROR_HTTP_502_ERROR;
	default:
	    return SR_ERROR_NO_ICY_CODE;
	}
    }

    // read generic headers
    extract_header_value(header, info->http_location, "Location:", 
	sizeof(info->http_location));
    extract_header_value(header, info->server, "Server:", 
	sizeof(info->server));
    rc = extract_header_value(header, info->icy_name, "icy-name:", 
	sizeof(info->icy_name));
    if (rc == 0) {
	/* Icecast 2.0.1 */
	rc = extract_header_value(header, info->icy_name, "ice-name:", 
	    sizeof(info->icy_name));
    }
    info->have_icy_name = rc;
    extract_header_value(header, info->icy_url, "icy-url:", 
	sizeof(info->icy_url));
    rc = extract_header_value(header, stempbr, 
	"icy-br:", sizeof(stempbr));
    if (rc) {
	info->icy_bitrate = atoi(stempbr);
    }

    /* interpret the content type from http header */
    rc = extract_header_value(header, stempbr, 
	"Content-Type:", sizeof(stempbr));
    if (rc == 0) {
	info->content_type = CONTENT_TYPE_UNKNOWN;
    }
    else if (strstr(stempbr,"audio/mpeg")) {
	info->content_type = CONTENT_TYPE_MP3;
    }
    else if (strstr(stempbr,"video/nsv")) {
	info->content_type = CONTENT_TYPE_NSV;
    }
    else if (strstr(stempbr,"misc/ultravox")) {
	info->content_type = CONTENT_TYPE_ULTRAVOX;
    }
    else if (strstr(stempbr,"application/ogg")) {
	info->content_type = CONTENT_TYPE_OGG;
    }
    else if (strstr(stempbr,"audio/aac")) {
	info->content_type = CONTENT_TYPE_AAC;
    }
    else if (strstr(stempbr,"audio/x-scpls")) {
	info->content_type = CONTENT_TYPE_PLS;
    }
    else if (strstr(stempbr,"text/html")) {
	if (!info->http_location[0]) {
	    return SR_ERROR_NO_RESPONSE_HEADER;
	}
    }
    else {
	info->content_type = CONTENT_TYPE_UNKNOWN;
    }

    /* Look at url for more content type hints */
    url_path_len = strlen(url_info.path);
    content_type_by_url = CONTENT_TYPE_UNKNOWN;
    if (url_path_len >= 4) {
	if (!strcmp (&url_info.path[url_path_len-4], ".aac")) {
	    content_type_by_url = CONTENT_TYPE_AAC;
	} else if (!strcmp (&url_info.path[url_path_len-4], ".ogg")) {
	    content_type_by_url = CONTENT_TYPE_OGG;
	} else if (!strcmp (&url_info.path[url_path_len-4], ".mp3")) {
	    content_type_by_url = CONTENT_TYPE_MP3;
	} else if (!strcmp (&url_info.path[url_path_len-4], ".nsv")) {
	    content_type_by_url = CONTENT_TYPE_NSV;
	} else if (!strcmp (&url_info.path[url_path_len-4], ".pls")) {
	    content_type_by_url = CONTENT_TYPE_PLS;
	} else if (!strcmp (&url_info.path[url_path_len-4], ".m3u")) {
	    content_type_by_url = CONTENT_TYPE_M3U;
	}
    }

    // Try to guess the server

    // Check for Streamripper relay
    if ((start = (char *)strstr(header, "[relay stream]")) != NULL) {
	strcpy(info->server, "Streamripper relay server");
    }
    // Check for Shoutcast
    else if ((start = (char *)strstr(header, "SHOUTcast")) != NULL) {
	strcpy(info->server, "SHOUTcast/");
	if ((start = (char *)strstr(start, "Server/")) != NULL) {
	    sscanf(start, "Server/%63[^<]<", versionbuf);
	    strcat(info->server, versionbuf);
	}

    }
    // Check for Icecast 2
    else if ((start = (char *)strstr(header, "Icecast 2")) != NULL) {
	/* aac on icecast 2.0-2.1 declares content type of audio/mpeg */
	/* In addition, there is at least one stream with a url 
	   radioorenovscotia.ogg, but is actually audio/mpeg */
	if (info->content_type == CONTENT_TYPE_MP3 
	    && content_type_by_url != CONTENT_TYPE_UNKNOWN
	    && content_type_by_url != CONTENT_TYPE_OGG) {
	    info->content_type = content_type_by_url;
	}
    }

    // Check for Icecast 1
    else if ((start = (char *)strstr(header, "icecast")) != NULL) {
	if (!info->server[0]) {
	    strcpy(info->server, "icecast/");
	    if ((start = (char *)strstr(start, "version ")) != NULL) {
		sscanf(start, "version %63[^<]<", versionbuf);
		strcat(info->server, versionbuf);
	    }
	}

	// icecast 1.x headers.
	extract_header_value(header, info->icy_url, "x-audiocast-server-url:",
	    sizeof(info->icy_url));
	rc = extract_header_value(header, info->icy_name, "x-audiocast-name:",
	    sizeof(info->icy_name));
	info->have_icy_name |= rc;
	extract_header_value(header, info->icy_genre, "x-audiocast-genre:",
	    sizeof(info->icy_genre));
	rc = extract_header_value(header, stempbr, "x-audiocast-bitrate:",
	    sizeof(stempbr));
	if (rc) {
	    info->icy_bitrate = atoi(stempbr);
	}
    }

    // Check for Apache
    else if (((start = (char *)strstr(header, "Apache")) != NULL) &&
	((start = (char *)strstr(header, "x-audiocast-name")) != NULL)) {
	// Streaming straight from apache with audiocast headers
	extract_header_value(header, info->icy_url, "x-audiocast-server-url:",
	    sizeof(info->icy_url));
	rc = extract_header_value(header, info->icy_name, "x-audiocast-name:",
	    sizeof(info->icy_name));
	info->have_icy_name |= rc;
	extract_header_value(header, info->icy_genre, "x-audiocast-genre:",
	    sizeof(info->icy_genre));
	rc = extract_header_value(header, stempbr, "x-audiocast-bitrate:",
	    sizeof(stempbr));
	if (rc) {
	    info->icy_bitrate = atoi(stempbr);
	}
    }

    /* Last chance to deduce content type */
    if (info->content_type == CONTENT_TYPE_UNKNOWN) {
	if (content_type_by_url == CONTENT_TYPE_UNKNOWN) {
	    info->content_type = CONTENT_TYPE_MP3;
	} else {
	    info->content_type = content_type_by_url;
	}
    }

    debug_printf ("Deduced content type: %d\n", info->content_type);

    // Make sure we don't have any CRLF's at the end of our strings
    trim(info->icy_url);
    trim(info->icy_genre);
    trim(info->icy_name);
    trim(info->http_location);
    trim(info->server);

    //get the meta interval
    start = (char*) strstr (header, "icy-metaint:");
    if (start) {
        sscanf (start, "icy-metaint:%i", &info->meta_interval);
        if (info->meta_interval < 1) {
	    info->meta_interval = NO_META_INTERVAL;
        }
    } else {
        info->meta_interval = NO_META_INTERVAL;
    }

    return SR_SUCCESS;
}

/* Constructs a HTTP response header from the SR_HTTP_HEADER struct, 
 * if data is null it is not added to the header
 */
error_code
http_construct_sc_response(SR_HTTP_HEADER *info, char *header, int size, int icy_meta_support)
{
    char *buf = (char *) malloc (size);

#if defined (commentout)
    char* test_header = 
	"HTTP/1.0 200 OK\r\n"
	"Content-Type: application/ogg\r\n"
	"ice-audio-info: ice-samplerate=44100\r\n"
	"ice-bitrate: Quality 0\r\n"
	"ice-description: This is my server desription\r\n"
	"ice-genre: Rock\r\n"
	"ice-name: This is my server desription\r\n"
	"ice-private: 0\r\n"
	"ice-public: 1\r\n"
	"ice-url: http://www.oddsock.org\r\n"
	"Server: Icecast 2.0.1\r\n\r\n";
#endif

    if (!info || !header || size < 1) {
	free (buf);
	return SR_ERROR_INVALID_PARAM;
    }

    memset(header, 0, size);

    /* GCS: The code used to give HTTP instead of ICY for the response header, 
	like this: sprintf(buf, "HTTP/1.0 200 OK\r\n"); */
    sprintf (buf, "ICY 200 OK\r\n");
    strcat(header, buf);

    if (info->http_location[0])
    {
	sprintf(buf, "Location:%s\r\n", info->http_location);
	strcat(header, buf);
    }

    if (info->server[0])
    {
	sprintf(buf, "Server:%s\r\n", info->server);
	strcat(header, buf);
    }

    if (info->have_icy_name) {
	sprintf(buf, "icy-name:%s\r\n", info->icy_name);
	strcat(header, buf);
    }

    if (info->icy_url[0])
    {
	sprintf(buf, "icy-url:%s\r\n", info->icy_url);
	strcat(header, buf);
    }

    if (info->icy_bitrate)
    {
	sprintf(buf, "icy-br:%d\r\n", info->icy_bitrate);
	strcat(header, buf);
    }

    if (info->icy_genre[0])
    {
	sprintf(buf, "icy-genre:%s\r\n", info->icy_genre);
	strcat(header, buf);
    }

    if ((info->meta_interval > 0) && icy_meta_support)
    {
	sprintf(buf, "icy-metaint:%d\r\n", info->meta_interval);
	strcat(header, buf);
    }

    switch (info->content_type) {
    case CONTENT_TYPE_MP3:
	sprintf (buf, "Content-Type: audio/mpeg\r\n");
	strcat(header, buf);
	break;
    case CONTENT_TYPE_NSV:
	sprintf (buf, "Content-Type: video/nsv\r\n");
	strcat(header, buf);
	break;
    case CONTENT_TYPE_OGG:
	sprintf (buf, "Content-Type: application/ogg\r\n");
	strcat(header, buf);
	break;
    case CONTENT_TYPE_ULTRAVOX:
	sprintf (buf, "Content-Type: misc/ultravox\r\n");
	strcat(header, buf);
	break;
    case CONTENT_TYPE_AAC:
	sprintf (buf, "Content-Type: audio/aac\r\n");
	strcat(header, buf);
	break;
    }

    free(buf);
    strcat(header, "\r\n");

    debug_printf ("Constructed response header:\n");
    debug_printf ("%s", header);

    return SR_SUCCESS;
}

/*
[playlist]
numberofentries=1
File1=http://localhost:8000/
Title1=(#1 - 530/18385) GCS hit radio
Length1=-1
Version=2
*/
static error_code
http_get_pls (RIP_MANAGER_INFO* rmi, HSOCKET *sock, SR_HTTP_HEADER *info)
{
    int s, bytes;
    error_code rc;
    char buf[MAX_PLS_LEN];
    char location_buf[MAX_PLS_LEN];
    char title_buf[MAX_PLS_LEN];

    debug_printf ("Reading pls\n");
    bytes = socklib_recvall (rmi, sock, buf, MAX_PLS_LEN, rmi->prefs->timeout);
    if (bytes < SR_SUCCESS) return bytes;
    if (bytes == 0 || bytes == MAX_PLS_LEN) {
	debug_printf("Failed in getting PLS (%d bytes)\n", bytes);
	return SR_ERROR_CANT_PARSE_PLS;
    }
    buf[bytes] = 0;

    debug_printf ("Parsing pls\n");
    debug_printf ("%s\n", buf);
    debug_printf ("---\n");

    /* Try to find an open server based on the number of open slots 
       in the Title field.  */
    rc = SR_ERROR_CANT_PARSE_PLS;
    for (s = 1; s <= 99; s++) {
	char buf1[20];
	int num_scanned, used, total, open;
	int best_open = 0;

	sprintf (buf1, "File%d=", s);
	debug_printf ("Searching for %s in %s\n", buf1, buf);
	if (!extract_header_value (buf, location_buf, buf1, 
				   sizeof(location_buf))) {
	    /* No more URLs in pls */
	    debug_printf ("s = %d, no more URLs\n", s);
	    break;
	}
	if (s == 1) {
	    /* Default to first parsed URL */
	    debug_printf ("Set URL to %s\n", location_buf);
	    sr_strncpy (info->http_location, location_buf, MAX_HOST_LEN);
	    rc = SR_SUCCESS;
	}
	
	sprintf (buf1, "Title%d=", s);
	if (!extract_header_value (buf, title_buf, buf1, sizeof(title_buf))) {
	    /* Remaining URLs have not title information */
	    debug_printf ("s = %d, no more title info\n", s);
	    break;
	}
	num_scanned = sscanf (title_buf, "(#%*[0-9] - %d/%d", &used, &total);
	if (num_scanned != 2) {
	    /* Title information doesn't have open slots information */
	    debug_printf ("s = %d, no more open slots info\n", s);
	    break;
	}
	open = total - used;
	if (open > best_open) {
	    sr_strncpy (info->http_location, location_buf, MAX_HOST_LEN);
	    best_open = open;
	}
    }

    debug_printf ("info->http_location = %s\n", info->http_location);
    //sr_strncpy (info->http_location, location_buf, MAX_HOST_LEN);

    return rc;
}

/*
#EXTM3U
#EXTINF:111,3rd Bass - Al z A-B-Cee z
mp3/3rd Bass/3rd bass - Al z A-B-Cee z.mp3
*/
static error_code
http_get_m3u (RIP_MANAGER_INFO* rmi, HSOCKET *sock, SR_HTTP_HEADER *info)
{
    int bytes;
    char buf[MAX_M3U_LEN];
    char* p;

    debug_printf ("Reading m3u\n");
    bytes = socklib_recvall (rmi, sock, buf, MAX_M3U_LEN, rmi->prefs->timeout);
    if (bytes < SR_SUCCESS) return bytes;
    if (bytes == 0 || bytes == MAX_M3U_LEN) {
	debug_printf("Failed in getting M3U (%d bytes)\n", bytes);
	return SR_ERROR_CANT_PARSE_M3U;
    }
    buf[bytes] = 0;

    debug_printf ("Parsing m3u\n");
    debug_printf ("%s\n", buf);
    debug_printf ("---\n");

    for (p = strtok (buf,"\n"); p!= 0; p = strtok (0, "\n")) {
	size_t len;
	if (p[0] == '#') {
	    continue;
	}
	len = strlen (p);
	if (len > 4 && !strcmp (&p[len-4], ".mp3")) {
	    continue;
	}
	sr_strncpy (info->http_location, p, MAX_HOST_LEN);
	debug_printf ("Redirecting from M3U to: %s\n", p);
	return SR_SUCCESS;
    }

    debug_printf ("Failed parsing M3U\n");
    return SR_ERROR_CANT_PARSE_M3U;
}

static error_code
http_get_sc_header(RIP_MANAGER_INFO* rmi, const char* url, 
		   HSOCKET *sock, SR_HTTP_HEADER *info)
{
    int ret;
    char headbuf[MAX_HEADER_LEN] = {'\0'};

    ret = socklib_read_header (rmi, sock, headbuf, MAX_HEADER_LEN);
    if (ret != SR_SUCCESS) {
	debug_printf ("socklib_read_header failed\n");
	return ret;
    }

    if ((ret = http_parse_sc_header(url, headbuf, info)) != SR_SUCCESS) {
	debug_printf ("http_parse_sc_header failed\n");
	return ret;
    }

    if (info->content_type == CONTENT_TYPE_PLS) {
	ret = http_get_pls (rmi, sock, info);
	if (ret != SR_SUCCESS) {
	    debug_printf ("http_get_pls failed\n");
	    return ret;
	}
    } else if (info->content_type == CONTENT_TYPE_M3U) {
	ret = http_get_m3u (rmi, sock, info);
	if (ret != SR_SUCCESS) {
	    debug_printf ("http_get_m3u failed\n");
	    return ret;
	}
    }

    return SR_SUCCESS;
}

/*
 * Copyright (c) 2000-2001 Virtual Unlimited B.V.
 *
 * Author: Bob Deblier <bob@virtualunlimited.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* thanks bob ;) */
#define CHARS_PER_LINE  72
static char* 
b64enc(const char *inbuf, int size)
{
    /* encode 72 characters per line */
    static const char* to_b64 =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    int div = size / 3;
    int rem = size % 3;
    int chars = div*4 + rem + 1;
    int newlines = (chars + CHARS_PER_LINE - 1) / CHARS_PER_LINE;

    const char* data = inbuf;
    char* string = (char*) malloc(chars + newlines + 1 + 100);

    if (string) {
	register char* buf = string;

	chars = 0;

	/*@+charindex@*/
	while (div > 0) {
	    buf[0] = to_b64[ (data[0] >> 2) & 0x3f];
	    buf[1] = to_b64[((data[0] << 4) & 0x30) + ((data[1] >> 4) & 0xf)];
	    buf[2] = to_b64[((data[1] << 2) & 0x3c) + ((data[2] >> 6) & 0x3)];
	    buf[3] = to_b64[  data[2] & 0x3f];
	    data += 3;
	    buf += 4;
	    div--;
	    chars += 4;
	    if (chars == CHARS_PER_LINE) {
		chars = 0;
		*(buf++) = '\n';
	    }
	}
        switch (rem) {
        case 2:
	    buf[0] = to_b64[ (data[0] >> 2) & 0x3f];
	    buf[1] = to_b64[((data[0] << 4) & 0x30) + ((data[1] >> 4) & 0xf)];
	    buf[2] = to_b64[ (data[1] << 2) & 0x3c];
	    buf[3] = '=';
	    buf += 4;
	    chars += 4;
	    break;
        case 1:
	    buf[0] = to_b64[ (data[0] >> 2) & 0x3f];
	    buf[1] = to_b64[ (data[0] << 4) & 0x30];
	    buf[2] = '=';
	    buf[3] = '=';
	    buf += 4;
	    chars += 4;
	    break;
	}
	/* *(buf++) = '\n'; This would result in a buffer overrun */
	*buf = '\0';
    }
    return string;
}

