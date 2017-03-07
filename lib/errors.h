#ifndef __ERRORS_H__
#define __ERRORS_H__

#include "srtypes.h"

typedef int error_code;

////////////////////////////////////////////////
// StreamRipper Codes
////////////////////////////////////////////////
// JCBUG -- no way to make custom error strings for http errors, also errors
// are not organized at all, should have space to insert in places.
//
/* ************** IMPORTANT IF YOU ADD ERROR CODES!!!! ***********************/
#define NUM_ERROR_CODES					((0x44)+1)
/* ************** IMPORTANT IF YOU ADD ERROR CODES!!!! ***********************/
#define SR_SUCCESS				  0x00
#define SR_SUCCESS_BUFFERING			  0x01
#define SR_ERROR_CANT_FIND_TRACK_SEPERATION	- 0x01
#define SR_ERROR_DECODE_FAILURE			- 0x02
#define SR_ERROR_INVALID_URL			- 0x03
#define	SR_ERROR_WIN32_INIT_FAILURE		- 0x04
#define SR_ERROR_CONNECT_FAILED			- 0x05
#define SR_ERROR_CANT_RESOLVE_HOSTNAME		- 0x06
#define SR_ERROR_RECV_FAILED			- 0x07
#define SR_ERROR_SEND_FAILED			- 0x08
#define SR_ERROR_PARSE_FAILURE			- 0x09
#define SR_ERROR_NO_RESPONSE_HEADER		- 0x0a
#define SR_ERROR_NO_ICY_CODE			- 0x0b
#define SR_ERROR_NO_META_INTERVAL		- 0x0c
#define SR_ERROR_INVALID_PARAM			- 0x0d
#define SR_ERROR_NO_HTTP_HEADER			- 0x0e
#define SR_ERROR_CANT_GET_LIVE365_ID		- 0x0f
#define SR_ERROR_CANT_ALLOC_MEMORY		- 0x10
#define SR_ERROR_CANT_FIND_IP_PORT		- 0x11
#define SR_ERROR_CANT_FIND_MEMBERNAME		- 0x12
#define SR_ERROR_CANT_FIND_TRACK_NAME		- 0x13
#define SR_ERROR_NULL_MEMBER_NAME		- 0x14
#define SR_ERROR_CANT_FIND_TIME_TAG		- 0x15
#define SR_ERROR_BUFFER_EMPTY			- 0x16
#define	SR_ERROR_BUFFER_FULL			- 0x17
#define	SR_ERROR_CANT_INIT_XAUDIO		- 0x18
#define SR_ERROR_BUFFER_TOO_SMALL		- 0x19
#define SR_ERROR_CANT_CREATE_THREAD		- 0x1A
#define SR_ERROR_CANT_FIND_MPEG_HEADER		- 0x1B
#define SR_ERROR_INVALID_METADATA		- 0x1C
#define SR_ERROR_NO_TRACK_INFO			- 0x1D
#define SR_EEROR_CANT_FIND_SUBSTR		- 0x1E
#define SR_ERROR_CANT_BIND_ON_PORT		- 0x1F
#define SR_ERROR_HOST_NOT_CONNECTED		- 0x20
#define SR_ERROR_HTTP_404_ERROR			- 0x21
#define SR_ERROR_HTTP_401_ERROR			- 0x22	
#define SR_ERROR_HTTP_502_ERROR			- 0x23  // Connection Refused
#define SR_ERROR_CANT_CREATE_FILE		- 0x24
#define SR_ERROR_CANT_WRITE_TO_FILE		- 0x25
#define SR_ERROR_CANT_CREATE_DIR		- 0x26
#define SR_ERROR_HTTP_400_ERROR			- 0x27	// Server Full
#define SR_ERROR_CANT_SET_SOCKET_OPTIONS	- 0x28
#define SR_ERROR_SOCK_BASE			- 0x29
#define SR_ERROR_INVALID_DIRECTORY		- 0x2a
#define SR_ERROR_FAILED_TO_MOVE_FILE		- 0x2b
#define SR_ERROR_CANT_LOAD_MPGLIB		- 0x2c
#define SR_ERROR_CANT_INIT_MPGLIB		- 0x2d
#define SR_ERROR_CANT_UNLOAD_MPGLIB		- 0x2e
#define SR_ERROR_PCM_BUFFER_TO_SMALL		- 0x2f
#define SR_ERROR_CANT_DECODE_MP3		- 0x30
#define SR_ERROR_SOCKET_CLOSED			- 0x31
#define SR_ERROR_LIVE365			- 0x32
#define SR_ERROR_MAX_BYTES_RIPPED		- 0x33
#define SR_ERROR_CANT_WAIT_ON_THREAD		- 0x34
#define SR_ERROR_CANT_CREATE_EVENT		- 0x35
#define SR_ERROR_NOT_SHOUTCAST_STREAM		- 0x36
#define SR_ERROR_HTTP_407_ERROR			- 0x37
#define	SR_ERROR_HTTP_403_ERROR			- 0x38
#define SR_ERROR_DIR_PATH_TOO_LONG		- 0x39
#define SR_ERROR_PROGRAM_ERROR			- 0x3a
#define SR_ERROR_TIMEOUT                        - 0x3b
#define SR_ERROR_SELECT_FAILED                  - 0x3c
#define SR_ERROR_REQUIRED_WINDOW_EMPTY          - 0x3d  // Not an error
#define SR_ERROR_CANT_BIND_ON_INTERFACE		- 0x3e
#define SR_ERROR_NO_OGG_PAGES_FOR_RELAY		- 0x3f
#define SR_ERROR_CANT_PARSE_PLS	                - 0x40
#define SR_ERROR_CANT_PARSE_M3U	                - 0x41
#define SR_ERROR_CANT_CREATE_SOCKET	        - 0x42
#define SR_ERROR_CREATE_PIPE_FAILED	        - 0x43
#define SR_ERROR_ABORT_PIPE_SIGNALLED           - 0x44  // Not an error

typedef struct ERROR_INFOst
{
    char error_str[MAX_ERROR_STR];
    error_code error_code;
} ERROR_INFO;


void errors_init (void);
char* errors_get_string (error_code code);

#endif
