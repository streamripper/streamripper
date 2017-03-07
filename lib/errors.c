/* errors.c
 *
 * This program is free software; you can redistribute it and/or modify
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
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <sys/types.h>
#if !defined (WIN32)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "filelib.h"
#include "socklib.h"
#include "http.h"
#include "mchar.h"
#include "findsep.h"
#include "relaylib.h"
#include "rip_manager.h"
#include "ripstream.h"
#include "threadlib.h"
#include "debug.h"
#include "compat.h"
#include "parse.h"

/******************************************************************************
 * Private Vars
 *****************************************************************************/
static char m_error_str[NUM_ERROR_CODES][MAX_ERROR_STR];
#define SET_ERR_STR(str, code)	strncpy(m_error_str[code], str, MAX_ERROR_STR);

/******************************************************************************
 * Public functions
 *****************************************************************************/
void
errors_init (void)
{
    SET_ERR_STR("SR_SUCCESS",					0x00);
    SET_ERR_STR("SR_ERROR_CANT_FIND_TRACK_SEPERATION",		0x01);
    SET_ERR_STR("SR_ERROR_DECODE_FAILURE",			0x02);
    SET_ERR_STR("SR_ERROR_INVALID_URL",				0x03);
    SET_ERR_STR("SR_ERROR_WIN32_INIT_FAILURE",			0x04);
    SET_ERR_STR("Could not connect to the stream. Try checking that the stream is up\n"
		"and that your proxy settings are correct.",	0x05);
    SET_ERR_STR("SR_ERROR_CANT_RESOLVE_HOSTNAME",		0x06);
    SET_ERR_STR("SR_ERROR_RECV_FAILED",				0x07);
    SET_ERR_STR("SR_ERROR_SEND_FAILED",				0x08);
    SET_ERR_STR("SR_ERROR_PARSE_FAILURE",			0x09);
    SET_ERR_STR("SR_ERROR_NO_RESPONSE_HEADER: Server is not a shoutcast stream",		0x0a);
    SET_ERR_STR("Server returned an unknown error code",	0x0b);
    SET_ERR_STR("SR_ERROR_NO_META_INTERVAL",			0x0c);
    SET_ERR_STR("SR_ERROR_INVALID_PARAM",			0x0d);
    SET_ERR_STR("SR_ERROR_NO_HTTP_HEADER",			0x0e);
    SET_ERR_STR("SR_ERROR_CANT_GET_LIVE365_ID",			0x0f);
    SET_ERR_STR("SR_ERROR_CANT_ALLOC_MEMORY",			0x10);
    SET_ERR_STR("SR_ERROR_CANT_FIND_IP_PORT",			0x11);
    SET_ERR_STR("SR_ERROR_CANT_FIND_MEMBERNAME",		0x12);
    SET_ERR_STR("SR_ERROR_CANT_FIND_TRACK_NAME",		0x13);
    SET_ERR_STR("SR_ERROR_NULL_MEMBER_NAME",			0x14);
    SET_ERR_STR("SR_ERROR_CANT_FIND_TIME_TAG",			0x15);
    SET_ERR_STR("SR_ERROR_BUFFER_EMPTY",			0x16);
    SET_ERR_STR("SR_ERROR_BUFFER_FULL",				0x17);
    SET_ERR_STR("SR_ERROR_CANT_INIT_XAUDIO",			0x18);
    SET_ERR_STR("SR_ERROR_BUFFER_TOO_SMALL",			0x19);
    SET_ERR_STR("SR_ERROR_CANT_CREATE_THREAD",			0x1A);
    SET_ERR_STR("SR_ERROR_CANT_FIND_MPEG_HEADER",		0x1B);
    SET_ERR_STR("SR_ERROR_INVALID_METADATA",			0x1C);
    SET_ERR_STR("SR_ERROR_NO_TRACK_INFO",			0x1D);
    SET_ERR_STR("SR_EEROR_CANT_FIND_SUBSTR",			0x1E);
    SET_ERR_STR("SR_ERROR_CANT_BIND_ON_PORT",			0x1F);
    SET_ERR_STR("SR_ERROR_HOST_NOT_CONNECTED",			0x20);
    SET_ERR_STR("HTTP:404 Not Found",				0x21);
    SET_ERR_STR("HTTP:401 Unauthorized",			0x22);
    SET_ERR_STR("HTTP:502 Bad Gateway",				0x23);	// Connection Refused
    SET_ERR_STR("SR_ERROR_CANT_CREATE_FILE",			0x24);
    SET_ERR_STR("SR_ERROR_CANT_WRITE_TO_FILE",			0x25);
    SET_ERR_STR("SR_ERROR_CANT_CREATE_DIR",			0x26);
    SET_ERR_STR("HTTP:400 Bad Request ",			0x27);	// Server Full
    SET_ERR_STR("SR_ERROR_CANT_SET_SOCKET_OPTIONS",		0x28);
    SET_ERR_STR("SR_ERROR_SOCK_BASE",				0x29);
    SET_ERR_STR("SR_ERROR_INVALID_DIRECTORY",			0x2a);
    SET_ERR_STR("SR_ERROR_FAILED_TO_MOVE_FILE",			0x2b);
    SET_ERR_STR("SR_ERROR_CANT_LOAD_MPGLIB",			0x2c);
    SET_ERR_STR("SR_ERROR_CANT_INIT_MPGLIB",			0x2d);
    SET_ERR_STR("SR_ERROR_CANT_UNLOAD_MPGLIB",			0x2e);
    SET_ERR_STR("SR_ERROR_PCM_BUFFER_TO_SMALL",			0x2f);
    SET_ERR_STR("SR_ERROR_CANT_DECODE_MP3",			0x30);
    SET_ERR_STR("SR_ERROR_SOCKET_CLOSED",			0x31);
    SET_ERR_STR("Due to legal reasons Streamripper can no longer work with Live365(tm).\r\n"
		"See streamripper.sourceforge.net for more on this matter.", 0x32);
    SET_ERR_STR("The maximum number of bytes ripped has been reached", 0x33);
    SET_ERR_STR("SR_ERROR_CANT_WAIT_ON_THREAD",			0x34);
    SET_ERR_STR("SR_ERROR_CANT_CREATE_EVENT",			0x35);
    SET_ERR_STR("SR_ERROR_NOT_SHOUTCAST_STREAM",		0x36);
    SET_ERR_STR("HTTP:407 - Proxy Authentication Required",	0x37);
    SET_ERR_STR("HTTP:403 - Access Forbidden (try changing the UserAgent)", 0x38);
    SET_ERR_STR("The output directory length is too long",      0x39);
    SET_ERR_STR("SR_ERROR_PROGRAM_ERROR",                       0x3a);
    SET_ERR_STR("SR_ERROR_TIMEOUT",                             0x3b);
    SET_ERR_STR("SR_ERROR_SELECT_FAILED",                       0x3c);
    SET_ERR_STR("SR_ERROR_RESERVED_WINDOW_EMPTY",               0x3d);
    SET_ERR_STR("SR_ERROR_CANT_BIND_ON_INTERFACE",              0x3e);
    SET_ERR_STR("SR_ERROR_NO_OGG_PAGES_FOR_RELAY",              0x3f);
    SET_ERR_STR("SR_ERROR_CANT_PARSE_PLS",                      0x40);
    SET_ERR_STR("SR_ERROR_CANT_PARSE_M3U",                      0x41);
    SET_ERR_STR("SR_ERROR_CANT_CREATE_SOCKET",                  0x42);
    SET_ERR_STR("SR_ERROR_CREATE_PIPE_FAILED",                  0x43);
}

char*
errors_get_string (error_code code)
{
    if (code > 0 || code < -NUM_ERROR_CODES)
        return NULL;
    return m_error_str[-code];
}
