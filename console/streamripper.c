/* streamripper.c
 * This little app should be seen as a demo for how to use the stremripper lib. 
 * The only file you need from the /lib dir is rip_mananger.h, and perhapes 
 * util.h (for stuff like formating the number of bytes).
 * 
 * the two functions of interest are main() for how to call rip_mananger_start
 * and rip_callback, which is a how you find out whats going on with the rip.
 * the rest of this file is really just boring command line parsing code.
 * and a signal handler for when the user hits CTRL+C
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
#if WIN32
//#define sleep	Sleep
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "srtypes.h"
#include "rip_manager.h"
#include "prefs.h"
#include "mchar.h"
#include "filelib.h"
#include "debug.h"

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static void print_usage (FILE* stream);
static void print_status (RIP_MANAGER_INFO *rmi);
static void catch_sig (int code);
static void parse_arguments (STREAM_PREFS *prefs, int argc, char **argv);
static void rip_callback (RIP_MANAGER_INFO* rmi, int message, void *data);
static void parse_extended_options (STREAM_PREFS *prefs, char *rule);
static void verify_splitpoint_rules (STREAM_PREFS *prefs);
static void print_to_console (char* fmt, ...);

/*****************************************************************************
 * Private variables
 *****************************************************************************/
static char m_buffer_chars[] = {'\\', '|', '/', '-', '*'}; /* for formating */
//static RIP_MANAGER_INFO 	m_curinfo; /* from the rip_manager callback */
static BOOL			m_started = FALSE;
static BOOL			m_alldone = FALSE;
static BOOL			m_got_sig = FALSE;
static BOOL			m_dont_print = FALSE;
static BOOL			m_print_stderr = FALSE;
time_t				m_stop_time = 0;

/* main()
 * parse the aguments, tell the rip_mananger to start, we get in rip
 * status from our rip_callback function. m_opt was set from parse args
 * and contains all the various things one can do with the rip_mananger
 * like a relay server, auto-reconnects, output dir's stuff like that.
 *
 * Notice the crappy while loop, this is because the streamripper lib 
 * is asyncrouns (spelling?) only. It probably should have a blocking 
 * call as well, but i needed this for window'd apps.
 */

int
main (int argc, char *argv[])
{
    int ret;
    time_t temp_time;
    STREAM_PREFS prefs;
    RIP_MANAGER_INFO *rmi = 0;

    sr_set_locale ();

    signal (SIGINT, catch_sig);
    signal (SIGTERM, catch_sig);

    parse_arguments (&prefs, argc, argv);

    print_to_console ("Connecting...\n");
    
    rip_manager_init ();

    /* Launch the ripping thread */
    if ((ret = rip_manager_start (&rmi, &prefs, rip_callback)) != SR_SUCCESS) {
	fprintf(stderr, "Couldn't connect to %s\n", prefs.url);
	exit(1);
    }

    /* 
     * The m_got_sig thing is because you can't call into a thread 
     * (i.e. rip_manager_stop) from a signal handler.. or at least not
     * in FreeBSD 3.4, i don't know about linux or NT.
     */
    while (!m_got_sig && !m_alldone) {
	sleep(1);
	time(&temp_time);
	if (m_stop_time && (temp_time >= m_stop_time)) {
	    print_to_console ("\nTime to stop is here, bailing\n");
	    break; 
	}	
    }

    print_to_console ("shutting down\n");

    rip_manager_stop (rmi);
    rip_manager_cleanup ();

    return 0;
}

void
catch_sig(int code)
{
    print_to_console ("\n");
    if (!m_started)
        exit(2);
    m_got_sig = TRUE;
}

/* In 1.63 I have changed two things: sending output to stdout 
   instead of stderr, and adding fflush as per the sticky in the forum. */
static void
print_to_console (char* fmt, ...)
{
    va_list argptr;

    if (!m_dont_print) {
	va_start (argptr, fmt);

	if (m_print_stderr) {
	    vfprintf (stderr, fmt, argptr);
	    fflush (stderr);
	} else {
	    vfprintf (stdout, fmt, argptr);
	    fflush (stdout);
	}

	va_end (argptr);
    }
}

/* 
 * This is to handle RM_UPDATE messages from rip_callback(), and more
 * importantly the RIP_MANAGER_INFO struct. Most of the code here
 * is for handling the pretty formating stuff otherwise it could be
 * much smaller.
 */
void
print_status (RIP_MANAGER_INFO *rmi)
{
    STREAM_PREFS *prefs = rmi->prefs;
    char status_str[128];
    char filesize_str[64];
    static int buffering_tick = 0;
    BOOL static printed_fullinfo = FALSE;

    if (m_dont_print)
	return;

    if (printed_fullinfo && rmi->filename[0]) {

	switch(rmi->status)
	{
	case RM_STATUS_BUFFERING:
	    buffering_tick++;
	    if (buffering_tick == 5)
		buffering_tick = 0;

	    sprintf(status_str,"buffering - %c ",
		    m_buffer_chars[buffering_tick]);

	    print_to_console ("[%14s] %.50s\r",
			      status_str,
			      rmi->filename);
	    break;

	case RM_STATUS_RIPPING:
	    if (rmi->track_count < prefs->dropcount) {
		strcpy(status_str, "skipping...   ");
	    } else {
		strcpy(status_str, "ripping...    ");
	    }
	    format_byte_size(filesize_str, rmi->filesize);
	    print_to_console ("[%14s] %.50s [%7s]\r",
			      status_str,
			      rmi->filename,
			      filesize_str);
	    break;
	case RM_STATUS_RECONNECTING:
	    strcpy(status_str, "re-connecting..");
	    print_to_console ("[%14s]\r", status_str);
	    break;
	}
			
    }
    if (!printed_fullinfo) {
	print_to_console ("stream: %s\n"
			  "server name: %s\n",
			  rmi->streamname,
			  rmi->server_name);
	if (rmi->http_bitrate > 0) {
	    print_to_console ("declared bitrate: %d\n",
			      rmi->http_bitrate);
	}
	if (rmi->meta_interval != NO_META_INTERVAL) {
	    print_to_console ("meta interval: %d\n",
			      rmi->meta_interval);
	}
	if (GET_MAKE_RELAY(prefs->flags)) {
	    print_to_console ("relay port: %d\n"
			      "[%14s]\r",
			      prefs->relay_port,
			      "getting track name... ");
	}

	printed_fullinfo = TRUE;
    }
}

/*
 * This will get called whenever anything interesting happens with the 
 * stream. Interesting are progress updates, error's, when the rip
 * thread stops (RM_DONE) starts (RM_START) and when we get a new track.
 *
 * for the most part this function just checks what kind of message we got
 * and prints out stuff to the screen.
 */
void
rip_callback (RIP_MANAGER_INFO* rmi, int message, void *data)
{
    ERROR_INFO *err;
    switch(message)
    {
    case RM_UPDATE:
	print_status (rmi);
	break;
    case RM_ERROR:
	err = (ERROR_INFO*)data;
	fprintf(stderr, "\nerror %d [%s]\n", err->error_code, err->error_str);
	m_alldone = TRUE;
	break;
    case RM_DONE:
	print_to_console ("bye..\n");
	m_alldone = TRUE;
	break;
    case RM_NEW_TRACK:
	print_to_console ("\n");
	break;
    case RM_STARTED:
	m_started = TRUE;
	break;
    }
}

/* Usage should be printed to stdout when it is not an error 
   http://www.gnu.org/prep/standards/standards.html */
static void
print_usage (FILE* stream)
{
    fprintf(stream, "Usage: streamripper URL [OPTIONS]\n");
    fprintf(stream, "Opts: -h             - Print this listing\n");
    fprintf(stream, "      -v             - Print version info and quit\n");
    fprintf(stream, "      -a [file]      - Rip to single file, default name is timestamped\n");
    fprintf(stream, "      -A             - Don't write individual tracks\n");
    fprintf(stream, "      -d dir         - The destination directory\n");
    fprintf(stream, "      -D pattern     - Write files using specified pattern\n");
    fprintf(stream, "      -s             - Don't create a directory for each stream\n");
    fprintf(stream, "      -r [[ip:]port] - Create relay server on base ip:port, default port 8000\n");
    fprintf(stream, "      -R #connect    - Max connections to relay, default 1, -R 0 is no limit\n");
    fprintf(stream, "      -L file        - Create a relay playlist file\n");
    fprintf(stream, "      -z             - Don't scan for free ports if base port is not avail\n");
    fprintf(stream, "      -p url         - Use HTTP proxy server at <url>\n");
    fprintf(stream, "      -o (always|never|larger|version)    - When to write tracks in complete\n");
    fprintf(stream, "      -t             - Don't overwrite tracks in incomplete\n");
    fprintf(stream, "      -c             - Don't auto-reconnect\n");
    fprintf(stream, "      -l seconds     - Number of seconds to run, otherwise runs forever\n");
    fprintf(stream, "      -M megabytes   - Stop ripping after this many megabytes\n");
    fprintf(stream, "      -q [start]     - Add sequence number to output file\n");
    fprintf(stream, "      -u useragent   - Use a different UserAgent than \"Streamripper\"\n");
    fprintf(stream, "      -w rulefile    - Parse metadata using rules in file.\n");
    fprintf(stream, "      -m timeout     - Number of seconds before force-closing stalled conn\n");
    fprintf(stream, "      -k count       - Leave <count> tracks in incomplete\n");
#if !defined (WIN32)
    fprintf(stream, "      -I interface   - Rip from specified interface (e.g. eth0)\n");
#endif
    fprintf(stream, "      -T             - Truncate duplicated tracks in incomplete\n");
    fprintf(stream, "      -E command     - Run external command to fetch metadata\n");
    fprintf(stream, "      --quiet        - Don't print ripping status to console\n");
    fprintf(stream, "      --stderr       - Print ripping status to stderr (old behavior)\n");
    fprintf(stream, "      --debug        - Save debugging trace\n");
    fprintf(stream, "ID3 opts (mp3/aac/nsv):  [The default behavior is adding ID3V2.3 only]\n");
    fprintf(stream, "      -i                           - Don't add any ID3 tags to output file\n");
    fprintf(stream, "      --with-id3v1                 - Add ID3V1 tags to output file\n");
    fprintf(stream, "      --without-id3v2              - Don't add ID3V2 tags to output file\n");
    fprintf(stream, "Splitpoint opts (mp3 only):\n");
    fprintf(stream, "      --xs-none                    - Don't search for silence\n");
    fprintf(stream, "      --xs-offset=num              - Shift relative to metadata (msec)\n");
    fprintf(stream, "      --xs-padding=num:num         - Add extra to prev:next track (msec)\n");
    fprintf(stream, "      --xs-search-window=num:num   - Search window relative to metadata (msec)\n");
    fprintf(stream, "      --xs-silence-length=num      - Expected length of silence (msec)\n");
    fprintf(stream, "      --xs2                        - Use new algorithm for silence detection\n");
    fprintf(stream, "Codeset opts:\n");
    fprintf(stream, "      --codeset-filesys=codeset    - Specify codeset for the file system\n");
    fprintf(stream, "      --codeset-id3=codeset        - Specify codeset for id3 tags\n");
    fprintf(stream, "      --codeset-metadata=codeset   - Specify codeset for metadata\n");
    fprintf(stream, "      --codeset-relay=codeset      - Specify codeset for the relay stream\n");
}

/* 
 * Bla, boring agument parsing crap, only reason i didn't use getopt
 * (which i did for an earlyer version) is because i couldn't find a good
 * port of it under Win32.. there probably is one, maybe i didn't look 
 * hard enough. 
 */
static void
parse_arguments (STREAM_PREFS* prefs, int argc, char **argv)
{
    int i;
    char *c;

    if (argc < 2) {
	print_usage (stdout);
	exit(2);
    }

    // Get URL
    strncpy (prefs->url, argv[1], MAX_URL_LEN);

    // Load prefs
    prefs_load ();
    prefs_get_stream_prefs (prefs, prefs->url);
    prefs_save ();

    // Parse arguments
    for (i = 1; i < argc; i++) {
	if (argv[i][0] != '-')
	    continue;

	c = strchr ("dDEfIklLmMopRuw", argv[i][1]);
        if (c != NULL) {
            if ((i == (argc-1)) || (argv[i+1][0] == '-')) {
		fprintf(stderr, "option %s requires an argument\n", argv[i]);
		exit(1);
	    }
	}
	switch (argv[i][1])
	{
	case 'a':
	    /* Create single file output + cue sheet */
	    OPT_FLAG_SET (prefs->flags, OPT_SINGLE_FILE_OUTPUT, 1);
	    prefs->showfile_pattern[0] = 0;
	    if (i == (argc-1) || argv[i+1][0] == '-')
		break;
	    i++;
	    strncpy (prefs->showfile_pattern, argv[i], SR_MAX_PATH);
	    break;
	case 'A':
	    OPT_FLAG_SET (prefs->flags, OPT_INDIVIDUAL_TRACKS, 0);
	    break;
	case 'c':
	    OPT_FLAG_SET (prefs->flags, OPT_AUTO_RECONNECT, 0);
	    break;
	case 'd':
	    i++;
	    strncpy(prefs->output_directory, argv[i], SR_MAX_PATH);
	    break;
	case 'D':
	    i++;
	    strncpy(prefs->output_pattern, argv[i], SR_MAX_PATH);
	    break;
	case 'E':
	    OPT_FLAG_SET (prefs->flags, OPT_EXTERNAL_CMD, 1);
	    i++;
	    strncpy(prefs->ext_cmd, argv[i], SR_MAX_PATH);
	    break;
	case 'f':
	    i++;
	    fprintf (stderr, "Error: -f dropstring option is obsolete. "
		     "Please use -w parse_rules instead.\n");
	    exit (1);
	case 'h':
	case '?':
	    print_usage (stdout);
            exit(0);
	    break;
	case 'i':
	    OPT_FLAG_SET(prefs->flags, OPT_ADD_ID3V1, 0);
	    OPT_FLAG_SET(prefs->flags, OPT_ADD_ID3V2, 0);
	    break;
	case 'I':
	    i++;
	    strncpy(prefs->if_name, argv[i], SR_MAX_PATH);
	    break;
	case 'k':
	    i++;
	    prefs->dropcount = atoi(argv[i]);
	    break;
	case 'l':
	    i++;
	    time(&m_stop_time);
	    m_stop_time += atoi(argv[i]);
	    break;
	case 'L':
	    i++;
	    strncpy(prefs->pls_file, argv[i], SR_MAX_PATH);
	    break;
	case 'm':
	    i++;
	    prefs->timeout = atoi(argv[i]);
	    break;
 	case 'M':
 	    i++;
 	    prefs->maxMB_rip_size = atoi(argv[i]);
	    OPT_FLAG_SET(prefs->flags, OPT_CHECK_MAX_BYTES, 1);
 	    break;
	case 'o':
	    i++;
	    prefs->overwrite = string_to_overwrite_opt (argv[i]);
	    if (prefs->overwrite == OVERWRITE_UNKNOWN) {
		fprintf (stderr, "Error: -o option requires an argument\n");
		exit (1);
	    }
	    break;
	case 'p':
	    i++;
	    strncpy(prefs->proxyurl, argv[i], MAX_URL_LEN);
	    break;
	case 'P':
	    i++;
	    fprintf (stderr, "Error: -P prefix option is obsolete. "
		     "Please use -D pattern instead.\n");
	    exit (1);
	case 'q':
	    OPT_FLAG_SET(prefs->flags, OPT_COUNT_FILES, 1);
	    prefs->count_start = -1;     /* -1 means auto-detect */
	    if (i == (argc-1) || argv[i+1][0] == '-')
		break;
	    i++;
	    prefs->count_start = atoi(argv[i]);
	    break;
	case 'r':
	    OPT_FLAG_SET(prefs->flags, OPT_MAKE_RELAY, 1);
	    if (i == (argc-1) || argv[i+1][0] == '-')
		break;
	    i++;
	    c = strstr(argv[i], ":");
	    if (NULL == c) {
	    	prefs->relay_port = atoi(argv[i]);
	    } else {
	    	*c = '\0';
		strncpy(prefs->relay_ip, argv[i], SR_MAX_PATH);
		prefs->relay_port = atoi(++c);
 	    }
	    break;
	case 'R':
	    i++;
	    prefs->max_connections = atoi(argv[i]);
	    break;
	case 's':
	    OPT_FLAG_SET(prefs->flags, OPT_SEPARATE_DIRS, 0);
	    break;
	case 't':
	    OPT_FLAG_SET(prefs->flags, OPT_KEEP_INCOMPLETE, 1);
	    break;
	case 'T':
	    OPT_FLAG_SET(prefs->flags, OPT_TRUNCATE_DUPS, 1);
	    break;
	case 'u':
	    i++;
	    strncpy(prefs->useragent, argv[i], MAX_USERAGENT_STR);
	    break;
	case 'v':
	    printf("Streamripper %s\n", SRVERSION);
	    exit(0);
	case 'w':
	    i++;
	    strncpy(prefs->rules_file, argv[i], SR_MAX_PATH);
	    break;
	case 'z':
	    OPT_FLAG_SET(prefs->flags, OPT_SEARCH_PORTS, 0);
	    prefs->max_port = prefs->relay_port+1000;
	    break;
	case '-':
	    parse_extended_options (prefs, &argv[i][2]);
	    break;
	}
    }

    /* Need to verify that splitpoint rules were sane */
    verify_splitpoint_rules (prefs);

    /* Verify that first parameter is URL */
    if (argv[1][0] == '-') {
	fprintf(stderr, "*** The first parameter MUST be the URL\n\n");
	exit(2);
    }
}

static void
parse_extended_options (STREAM_PREFS* prefs, char* rule)
{
    int x,y;

    /* Version */
    if (!strcmp(rule,"version")) {
	printf("Streamripper %s\n", SRVERSION);
	exit(0);
    }

    /* Logging options */
    if (!strcmp(rule,"debug")) {
	debug_enable();
	return;
    }
    if (!strcmp(rule,"stderr")) {
	m_print_stderr = TRUE;
	return;
    }
    if (!strcmp(rule,"quiet")) {
	m_dont_print = TRUE;
	return;
    }

    /* Splitpoint options */
    if ((!strcmp(rule,"xs-none"))
	|| (!strcmp(rule,"xs_none"))) {
	prefs->sp_opt.xs = 0;
	debug_printf ("Disable silence detection");
	return;
    }
    if (!strcmp(rule,"xs2")) {
	prefs->sp_opt.xs = 2;
	debug_printf ("Setting xs2\n");
	return;
    }
    if ((1==sscanf(rule,"xs-min-volume=%d",&x)) 
	|| (1==sscanf(rule,"xs_min_volume=%d",&x))) {
	prefs->sp_opt.xs_min_volume = x;
	debug_printf ("Setting minimum volume to %d\n",x);
	return;
    }
    if ((1==sscanf(rule,"xs-silence-length=%d",&x))
	|| (1==sscanf(rule,"xs_silence_length=%d",&x))) {
	prefs->sp_opt.xs_silence_length = x;
	debug_printf ("Setting silence length to %d\n",x);
	return;
    }
    if ((2==sscanf(rule,"xs-search-window=%d:%d",&x,&y))
	|| (2==sscanf(rule,"xs_search_window=%d:%d",&x,&y))) {
	prefs->sp_opt.xs_search_window_1 = x;
	prefs->sp_opt.xs_search_window_2 = y;
	debug_printf ("Setting search window to (%d:%d)\n",x,y);
	return;
    }
    if ((1==sscanf(rule,"xs-offset=%d",&x))
	|| (1==sscanf(rule,"xs_offset=%d",&x))) {
	prefs->sp_opt.xs_offset = x;
	debug_printf ("Setting silence offset to %d\n",x);
	return;
    }
    if ((2==sscanf(rule,"xs-padding=%d:%d",&x,&y))
	|| (2==sscanf(rule,"xs_padding=%d:%d",&x,&y))) {
	prefs->sp_opt.xs_padding_1 = x;
	prefs->sp_opt.xs_padding_2 = y;
	debug_printf ("Setting file output padding to (%d:%d)\n",x,y);
	return;
    }

    /* id3 options */
    if (!strcmp(rule,"with-id3v2")) {
	OPT_FLAG_SET(prefs->flags,OPT_ADD_ID3V2,1);
	return;
    }
    if (!strcmp(rule,"without-id3v2")) {
	OPT_FLAG_SET(prefs->flags,OPT_ADD_ID3V2,0);
	return;
    }
    if (!strcmp(rule,"with-id3v1")) {
	OPT_FLAG_SET(prefs->flags,OPT_ADD_ID3V1,1);
	return;
    }
    if (!strcmp(rule,"without-id3v1")) {
	OPT_FLAG_SET(prefs->flags,OPT_ADD_ID3V1,0);
	return;
    }

    /* codeset options */
    x = strlen("codeset-filesys=");
    if (!strncmp(rule,"codeset-filesys=",x)) {
	strncpy (prefs->cs_opt.codeset_filesys, &rule[x], MAX_CODESET_STRING);
	debug_printf ("Setting filesys codeset to %s\n",
		      prefs->cs_opt.codeset_filesys);
	return;
    }
    x = strlen("codeset-id3=");
    if (!strncmp(rule,"codeset-id3=",x)) {
	strncpy (prefs->cs_opt.codeset_id3, &rule[x], MAX_CODESET_STRING);
	debug_printf ("Setting id3 codeset to %s\n",
		      prefs->cs_opt.codeset_id3);
	return;
    }
    x = strlen("codeset-metadata=");
    if (!strncmp(rule,"codeset-metadata=",x)) {
	strncpy (prefs->cs_opt.codeset_metadata, &rule[x], MAX_CODESET_STRING);
	debug_printf ("Setting metadata codeset to %s\n",
		      prefs->cs_opt.codeset_metadata);
	return;
    }
    x = strlen("codeset-relay=");
    if (!strncmp(rule,"codeset-relay=",x)) {
	strncpy (prefs->cs_opt.codeset_relay, &rule[x], MAX_CODESET_STRING);
	debug_printf ("Setting relay codeset to %s\n",
		      prefs->cs_opt.codeset_relay);
	return;
    }

    /* All rules failed */
    fprintf (stderr, "Can't parse command option: --%s\n", rule);
    exit (-1);
}

static void
verify_splitpoint_rules (STREAM_PREFS *prefs)
{
#if defined (commentout)
    /* This is still not complete, but the warning causes people to 
       wonder what is going on. */
    fprintf (stderr, "Warning: splitpoint sanity check not yet complete.\n");
#endif
    
    /* xs_silence_length must be non-negative and divisible by two */
    if (prefs->sp_opt.xs_silence_length < 0) {
	prefs->sp_opt.xs_silence_length = 0;
    }
    if (prefs->sp_opt.xs_silence_length % 2) {
        prefs->sp_opt.xs_silence_length ++;
    }

    /* search_window values must be non-negative */
    if (prefs->sp_opt.xs_search_window_1 < 0) {
	prefs->sp_opt.xs_search_window_1 = 0;
    }
    if (prefs->sp_opt.xs_search_window_2 < 0) {
	prefs->sp_opt.xs_search_window_2 = 0;
    }

    /* if silence_length is 0, then search window should be zero */
    if (prefs->sp_opt.xs_silence_length == 0) {
	prefs->sp_opt.xs_search_window_1 = 0;
	prefs->sp_opt.xs_search_window_2 = 0;
    }

    /* search_window values must be longer than silence_length */
    if (prefs->sp_opt.xs_search_window_1 + prefs->sp_opt.xs_search_window_2
	    < prefs->sp_opt.xs_silence_length) {
	/* if this happens, disable search */
	prefs->sp_opt.xs_search_window_1 = 0;
	prefs->sp_opt.xs_search_window_2 = 0;
	prefs->sp_opt.xs_silence_length = 0;
    }

    /* search window lengths must be at least 1/2 of silence_length */
    if (prefs->sp_opt.xs_search_window_1 < prefs->sp_opt.xs_silence_length) {
	prefs->sp_opt.xs_search_window_1 = prefs->sp_opt.xs_silence_length;
    }
    if (prefs->sp_opt.xs_search_window_2 < prefs->sp_opt.xs_silence_length) {
	prefs->sp_opt.xs_search_window_2 = prefs->sp_opt.xs_silence_length;
    }
}
