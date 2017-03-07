/* srtypes.h
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
#ifndef __SRTYPES_H__
#define __SRTYPES_H__

#include <glib.h>

//#include "regex.h"
#include "srconfig.h"
#include "compat.h"
#include "list.h"
#if WIN32
/* Warning: Not allowed to mix windows.h & cdk.h */
#include <windows.h>
#else
#include <sys/types.h>
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#if HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

/******************************************************************************
 * Types
 *****************************************************************************/
/* Note: uint32_t is standardized in ISO C99, so let's use that one */
#if !HAVE_UINT32_T
# if HAVE_U_INT32_T
typedef u_int32_t uint32_t;
# else
typedef unsigned int uint32_t;
# endif
#endif

#if HAVE_WCHAR_SUPPORT
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
#if HAVE_WCTYPE_H
#include <wctype.h>
#endif
#endif
#if STDC_HEADERS
#include <stddef.h>
#endif

#if (HAVE_OGG_VORBIS)
#include <ogg/ogg.h>
#endif

#define BOOL	int
#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#define USE_GLIB_REGEX          1   /* New PCRE regex */

#define NO_META_INTERVAL	-1

/* GCS - Grr. I don't care.  Max path is 254 until I get around to
    fixing this for other platforms. */
#define SR_MAX_PATH		254
#define MAX_HOST_LEN		512
#define MAX_IP_LEN			3+1+3+1+3+1+3+1
#define MAX_HEADER_LEN		8192
#define MAX_URL_LEN		8192
#define MAX_VERSION_LEN		256
#define MAX_ICY_STRING		4024
#define MAX_SERVER_LEN		1024
//#define MAX_TRACK_LEN		MAX_PATH
#define MAX_TRACK_LEN		SR_MAX_PATH /* GCS - be careful here... */
#define MAX_URI_STRING		1024
#define MAX_ERROR_STR           (4096)
#define MAX_USERAGENT_STR	1024
#define MAX_AUTH_LEN            255
//#define MAX_DROPSTRING_LEN      255

#define MAX_METADATA_LEN (127*16)

#define MAX_STATUS_LEN		256
#define MAX_STREAMNAME_LEN	1024
#define MAX_SERVER_LEN		1024
#define MAX_EXT_LINE_LEN 255

#define DEFAULT_META_INTERVAL	1024


#ifdef WIN32
  #ifndef _WINSOCKAPI_
    #define __DEFINE_TYPES__
  #endif
#endif

#ifdef __DEFINE_TYPES__
typedef unsigned long u_long;
typedef unsigned char u_char;
typedef unsigned short u_short;
#endif

/* Different types of streams */
#define CONTENT_TYPE_MP3		1
#define CONTENT_TYPE_NSV		2
#define CONTENT_TYPE_OGG    		3
#define CONTENT_TYPE_ULTRAVOX		4
#define CONTENT_TYPE_AAC		5
#define CONTENT_TYPE_PLS		6
#define CONTENT_TYPE_M3U		7
#define CONTENT_TYPE_UNKNOWN		99

/* 
 * IO_DATA_INPUT is a interface for socket input data, it has one 
 * method 'get_data' and is called by a "ripper" which is effectivly 
 * only ripshout.c (and the R.I.P. riplive365.c)
 */
typedef struct IO_DATA_INPUTst{
	int (*get_input_data)(char* buffer, int size);
} IO_DATA_INPUT;

#define NO_TRACK_STR	"No track info..."

/* 
 * IO_GET_STREAM is an interface for getting data and track info from
 * a better splite on the track seperation. it keeps a back buffer and 
 * does the "find silent point" shit.
 */
#if defined (commentout)
typedef struct IO_GET_STREAMst{
	int (*get_stream_data)(char* data_buf, char *track_buf);
	u_long getsize;
} IO_GET_STREAM;
#endif

/* 
 * SPLITPOINT_OPTIONS are the options used to tweek how the silence 
 * separation is done.
 */
typedef struct SPLITPOINT_OPTIONSst
{
    int	xs;
    int xs_min_volume;
    int xs_silence_length;
    int xs_search_window_1;
    int xs_search_window_2;
    int xs_offset;
    //int xd_offset;
    //int xpadding_1;
    //int xpadding_2;
    int xs_padding_1;
    int xs_padding_2;
} SPLITPOINT_OPTIONS;

/* 
 * CODESET_OPTIONS are the options used to decide how to parse
 * and convert the metadata
 */
#define MAX_CODESET_STRING 128
typedef struct CODESET_OPTIONSst
{
    char codeset_locale[MAX_CODESET_STRING];
    char codeset_filesys[MAX_CODESET_STRING];
    char codeset_id3[MAX_CODESET_STRING];
    char codeset_metadata[MAX_CODESET_STRING];
    char codeset_relay[MAX_CODESET_STRING];
} CODESET_OPTIONS;

/* 
 * Various CODESET types
 */
#define CODESET_UTF8          1
#define CODESET_LOCALE        2
#define CODESET_FILESYS       3
#define CODESET_ID3           4
#define CODESET_METADATA      5
#define CODESET_RELAY         6

/* The Lstring is not null-terminated, and can belong to any locale */
struct lstring {
    gsize num_bytes;
    gchar* data;
};
typedef struct lstring Lstring;

/* The mchar is a legacy data structure, and should be removed */
typedef gchar mchar;

/* 
 * Parse_Rule is a single line of the parse rules file
 */
struct parse_rule {
    int cmd;
    int flags;
    int artist_idx;
    int title_idx;
    int album_idx;
    int trackno_idx;
    int year_idx;
#if defined (USE_GLIB_REGEX)
    GRegex* reg;
#else
    regex_t* reg;
#endif
    mchar* match;
    mchar* subst;
};
typedef struct parse_rule Parse_Rule;

/* 
 * TRACK_INFO is the parsed metadata
 */
typedef struct TRACK_INFOst
{
    int have_track_info;
    char raw_metadata[MAX_TRACK_LEN];
    mchar w_raw_metadata[MAX_TRACK_LEN];
    mchar artist[MAX_TRACK_LEN];
    mchar title[MAX_TRACK_LEN];
    mchar album[MAX_TRACK_LEN];
    mchar track_p[MAX_TRACK_LEN];                /* Track# parsed */
    mchar track_a[MAX_TRACK_LEN];                /* Track# assigned to id3 */
    mchar year[MAX_TRACK_LEN];
    char composed_metadata[MAX_METADATA_LEN+1];  /* For relay stream */
    BOOL save_track;
} TRACK_INFO;

#ifndef WIN32
typedef int SOCKET;
#endif

typedef struct HSOCKETst
{
	SOCKET	s;
	BOOL	closed;
} HSOCKET;

/* 
 * OverwriteOpt controls how files in complete directory are overwritten
 */
enum OverwriteOpt {
    OVERWRITE_UNKNOWN,	// Error case
    OVERWRITE_ALWAYS,	// Always replace file in complete with newer
    OVERWRITE_NEVER,	// Never replace file in complete with newer
    OVERWRITE_LARGER,	// Replace file in complete if newer is larger
    OVERWRITE_VERSION	// Never overwrite, instead make a new version
};

/* Information extracted from the stream's HTTP header */
typedef struct SR_HTTP_HEADERst
{
    int content_type;
    int meta_interval;
    int have_icy_name;
    char icy_name[MAX_ICY_STRING];
    int icy_code;
    int icy_bitrate;
    char icy_genre[MAX_ICY_STRING];
    char icy_url[MAX_ICY_STRING];
    char http_location[MAX_HOST_LEN];
    char server[MAX_SERVER_LEN];
} SR_HTTP_HEADER;

typedef struct URLINFOst
{
	char host[MAX_HOST_LEN];
	char path[SR_MAX_PATH];
	u_short port;
	char username[MAX_URI_STRING];
	char password[MAX_URI_STRING];
} URLINFO;

typedef struct external_process External_Process;
struct external_process
{
#if defined (WIN32)
    HANDLE mypipe;   /* read from child stdout */
    HANDLE hproc;
    DWORD pid;
#else
    int mypipe[2];   /* 0 is for parent reading, 1 is for parent writing */
    pid_t pid;
#endif
    int line_buf_idx;
    char line_buf[MAX_EXT_LINE_LEN];
    char album_buf[MAX_EXT_LINE_LEN];
    char artist_buf[MAX_EXT_LINE_LEN];
    char title_buf[MAX_EXT_LINE_LEN];
    char metadata_buf[MAX_EXT_LINE_LEN];
};

/* Each ogg page boundary within the cbuf gets this struct */
typedef struct OGG_PAGE_LIST_struct OGG_PAGE_LIST;
struct OGG_PAGE_LIST_struct
{
    unsigned long m_page_start;
    unsigned long m_page_len;
    unsigned long m_page_flags;
    char *m_header_buf_ptr;
    unsigned long m_header_buf_len;
    LIST m_list;
};

typedef struct CBUF2_struct
{
    char*	buf;
    int         content_type;
    int         have_relay;
    u_long	num_chunks;
    u_long	chunk_size;
    u_long	size;        /* size is chunk_size * num_chunks */
    u_long	base_idx;
    u_long	item_count;  /* Amount filled */
    u_long	next_song;   /* start of next song (mp3 only) */
    OGG_PAGE_LIST* song_page;    /* current page being written (ogg only) */
    u_long      song_page_done;  /* amount finished in current page (ogg) */

    HSEM        cbuf_sem;

    LIST        metadata_list;
    LIST        ogg_page_list;
    LIST        frame_list;
} CBUF2;


/* Each relay server gets this list of clients */
typedef struct RELAY_LIST_struct RELAY_LIST;
struct RELAY_LIST_struct
{
    SOCKET m_sock;
    int m_is_new;
    
    char* m_buffer;            // large enough for 1 block & 1 metadata
    u_long m_buffer_size;
    u_long m_cbuf_offset;      // must lie along chunck boundary for mp3
    u_long m_offset;
    u_long m_left_to_send;
    int m_icy_metadata;        // true if client requested metadata

    char* m_header_buf_ptr;    // for ogg header pages
    u_long m_header_buf_len;   // for ogg header pages
    u_long m_header_buf_off;   // for ogg header pages

    RELAY_LIST* m_next;
};


#if (HAVE_OGG_VORBIS)
typedef struct _stream_processor {
    void (*process_end)(struct _stream_processor *);
    int isillegal;
    int constraint_violated;
    int shownillegal;
    int isnew;
    long seqno;
    int lostseq;

    int start;
    int end;

    int num;
    char *type;

    ogg_uint32_t serial; /* must be 32 bit unsigned */
    ogg_stream_state os;
    void *data;
} stream_processor;
#endif

typedef struct RELAYLIB_INFO_struct RELAYLIB_INFO;
struct RELAYLIB_INFO_struct
{
    HSEM m_sem_not_connected;
    char m_http_header[MAX_HEADER_LEN];
    SOCKET m_listensock;
    BOOL m_running;
    BOOL m_running_accept;
    BOOL m_running_send;
    THREAD_HANDLE m_hthread_accept;
    THREAD_HANDLE m_hthread_send;
};

#define DATEBUF_LEN 50

typedef struct FILELIB_INFO_struct FILELIB_INFO;
struct FILELIB_INFO_struct
{
    FHANDLE m_file;
    FHANDLE m_show_file;
    FHANDLE m_cue_file;
    int m_count;
    int m_do_show;
    mchar m_default_pattern[SR_MAX_PATH];
    mchar m_default_showfile_pattern[SR_MAX_PATH];
    mchar m_output_directory[SR_MAX_PATH];
    mchar m_output_pattern[SR_MAX_PATH];
    mchar m_incomplete_directory[SR_MAX_PATH];
    mchar m_incomplete_filename[SR_MAX_PATH];
    mchar m_showfile_directory[SR_MAX_PATH];
    mchar m_showfile_pattern[SR_MAX_PATH];
    BOOL m_keep_incomplete;
    int m_max_filename_length;
    mchar m_show_name[SR_MAX_PATH];
    mchar m_cue_name[SR_MAX_PATH];
    mchar m_icy_name[SR_MAX_PATH];
    mchar* m_extension;
    BOOL m_do_individual_tracks;
    mchar m_session_datebuf[DATEBUF_LEN];
    mchar m_stripped_icy_name[SR_MAX_PATH];
    int m_track_no;
};


#define RIPLIST_LEN 10

typedef struct stream_prefs STREAM_PREFS;
struct stream_prefs
{
    char label[MAX_URL_LEN];		// logical name for this stream
    char url[MAX_URL_LEN];		// url of the stream to connect to
    char proxyurl[MAX_URL_LEN];		// url of a http proxy server, 
                                        //  '\0' otherwise
    char output_directory[SR_MAX_PATH];	// base directory to output files too
    char output_pattern[SR_MAX_PATH];	// filename pattern when ripping 
                                        //  with splitting
    char showfile_pattern[SR_MAX_PATH];	// filename base when ripping to
                                        //  single file without splitting
    char if_name[SR_MAX_PATH];		// local interface to use
    char rules_file[SR_MAX_PATH];       // file that holds rules for 
                                        //  parsing metadata
    char pls_file[SR_MAX_PATH];		// optional, where to create a 
                                        //  rely .pls file
    char relay_ip[SR_MAX_PATH];		// optional, ip to bind relaying 
                                        //  socket to
    char ext_cmd[SR_MAX_PATH];          // cmd to spawn for external metadata
    char useragent[MAX_USERAGENT_STR];	// optional, use a different useragent
    u_short relay_port;			// port to use for the relay server
					//  GCS 3/30/07 change to u_short
    u_short max_port;			// highest port the relay server 
                                        //  can look if it needs to search
    u_long max_connections;             // max number of connections 
                                        //  to relay stream
					//  GCS 8/18/07 change int to u_long
    u_long maxMB_rip_size;		// max number of megabytes that 
                                        //  can by writen out before we stop
    u_long flags;			// all booleans logically OR'd 
                                        //  together (see above)
    u_long timeout;			// timeout, in seconds, before a 
                                        //  stalled connection is forcefully 
                                        //  closed
					//  GCS 8/18/07 change int to u_long
    u_long dropcount;			// number of tracks at beginning 
                                        //  of connection to always ignore
					//  GCS 8/18/07 change int to u_long
    int count_start;                    // which number to start counting?
    enum OverwriteOpt overwrite;	// overwrite file in complete?
    SPLITPOINT_OPTIONS sp_opt;		// options for splitpoint rules
    CODESET_OPTIONS cs_opt;             // which codeset should i use?
};

typedef struct wstreamripper_prefs WSTREAMRIPPER_PREFS;
struct wstreamripper_prefs
{
    int		m_add_finished_tracks_to_playlist;
    int		m_start_minimized;
    long	oldpos_x;
    long	oldpos_y;
    int		m_enabled;
    char	localhost[MAX_HOST_LEN];	// hostname of 'localhost' 
    char	default_skin[SR_MAX_PATH];
    int		use_old_playlist_ret;
    char	riplist[RIPLIST_LEN][MAX_URL_LEN];
};

typedef struct global_prefs GLOBAL_PREFS;
struct global_prefs
{
    char version[MAX_VERSION_LEN];             // default stream
    char url[MAX_URL_LEN];                     // default stream
    STREAM_PREFS stream_prefs;                 // default prefs for new streams
    WSTREAMRIPPER_PREFS wstreamripper_prefs;   // prefs for winamp plugin
};

typedef struct RIP_MANAGER_INFOst RIP_MANAGER_INFO;
typedef void(*RIP_MANAGER_CALLBACK)(RIP_MANAGER_INFO* rmi, 
				    int message, void *data);
struct RIP_MANAGER_INFOst
{
    STREAM_PREFS *prefs;
    char streamname[MAX_STREAMNAME_LEN];
    char server_name[MAX_SERVER_LEN];
    u_long filesize;
    int	status;
    int	meta_interval;

    /* Bitrate as reported by http header */
    int	http_bitrate;

    /* Bitrate detected in first frame */
    int detected_bitrate;

    /* Bitrate used to compute buffer size */
    int bitrate;

    /* it's not the filename, it's the trackname */
    char filename[SR_MAX_PATH];

    /* Process id & handle for external process */
    External_Process *ep;

#if __UNIX__
    /* Write to the abort pipe to exit select() of the ripping thread */
    int abort_pipe[2];
#endif

    /* Is rip manager active? */
    int started;
    HSEM started_sem;

    /* Info from stream http header.  It's used for generating relay 
       header, and formatting strings for winamp playlist. */
    SR_HTTP_HEADER http_info;

    /* Socket for connection to stream */
    HSOCKET stream_sock;

    /* Handle to ripping thread */
    THREAD_HANDLE hthread_ripper;

    /* Callback function */
    //void (*m_status_callback)(RIP_MANAGER_INFO* rmi, int message, void *data);
    RIP_MANAGER_CALLBACK status_callback;

    /* Bytes ripped is stored to send to callback, also used to decide 
       when to stop based on bytes ripped. */
    u_long bytes_ripped;
    u_long megabytes_ripped;

    /* Should we write a file for this track? */
    BOOL write_data;

    /* Used by ripstream ogg logic */
    /* 0 : Track inactive, file closed */
    /* 1 : Track active, file open */
    /* 2 : Got eos, file open pending confirmed title change */
    int ogg_track_state;

    /* Title & artist info */
    TRACK_INFO old_track;	    /* The track that's being ripped now */
    TRACK_INFO new_track;	    /* The track that's gonna start soon */
    TRACK_INFO current_track;       /* The metadata as I'm parsing it */

    /* After the first buffer, we collect statistics about the stream */
    int ripstream_first_time_through;

    /* What is the title if there is no metadata? */
    char no_meta_name[MAX_TRACK_LEN];

    /* A temporary buffer used by ripstream.c */
    char* getbuffer;
    unsigned long getbuffer_size;

    /* A counter that keeps track of how many more blocks to read before 
       we can do silence detection.  Used by ripstream.c */
    int find_silence;

    /* Keep track of bytes ripped, to use by cue sheet to compute time. */
    unsigned int cue_sheet_bytes;

    /* Keep track of how many tracks we have ripped */
    unsigned int track_count;

    /* The circular buffer */
    CBUF2 cbuf2;

    /* CBuf size variables.  Used by ripstream.c */
    int cbuf2_size;             /* blocks */
    int rw_start_to_cb_end;     /* bytes */
    int rw_start_to_sw_start;   /* milliseconds */
    int rw_end_to_cb_end;       /* bytes */
    int mic_to_cb_end;          /* blocks */

    /* The Relay list */
    RELAY_LIST* relay_list;
    unsigned long relay_list_len;
    HSEM relay_list_sem;

    /* Private data used by filelib.c */
    FILELIB_INFO filelib_info;

    /* Private data used by relaylib.c */
    RELAYLIB_INFO relaylib_info;

    /* Private data used by parse.c */
    Parse_Rule* parse_rules;

#if (HAVE_OGG_VORBIS)
    /* Ogg state, used by ripogg.c */
    ogg_sync_state ogg_sync;
    ogg_page ogg_pg;
    stream_processor stream;
    char* ogg_curr_header;
    int ogg_curr_header_len;
#endif

    /* Mchar codesets -- these shadow prefs codesets */
    CODESET_OPTIONS mchar_cs;
};


#endif //__SRIPPER_H__
