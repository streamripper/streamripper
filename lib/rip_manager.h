/* rip_manager.h
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
#ifndef __RIP_MANANGER_H__
#define __RIP_MANANGER_H__

#include "external.h"
#include "prefs.h"
#include "srtypes.h"
#include "errors.h"
#include "compat.h"

#define SRVERSION	"1.64.6"

#if defined (WIN32)
#define SRPLATFORM      "windows"
#else
#define SRPLATFORM      "unix"
#endif

// Messages for status_callback hook in rip_manager_init()
// used for notifing to client whats going on *DO NOT* call 
// rip_mananger_start or rip_mananger_stop from
// these functions!!! it will cause a deadlock
#define RM_UPDATE	0x01		// returns a pointer RIP_MANAGER_INFO struct
#define RM_ERROR	0x02		// returns the error code
#define RM_DONE		0x03		// NULL
#define RM_STARTED	0x04		// NULL
#define RM_NEW_TRACK	0x05		// Name of the new track
#define RM_TRACK_DONE	0x06		// pull path of the track completed
// RM_OUTPUT_DIR is now OBSOLETE
#define RM_OUTPUT_DIR	0x07		// Full path of the output directory


// The following are the possible status values for RIP_MANAGER_INFO
#define RM_STATUS_BUFFERING		0x01
#define RM_STATUS_RIPPING		0x02
#define RM_STATUS_RECONNECTING		0x03


// Rip manager flags options
#define OPT_AUTO_RECONNECT	0x00000001	// reconnect automatically if dropped
#define OPT_SEPARATE_DIRS	0x00000002	// create a directory named after the server
#define OPT_SEARCH_PORTS	0x00000008	// relay server should search for a open port
#define OPT_MAKE_RELAY		0x00000010	// don't make a relay server
#define OPT_COUNT_FILES		0x00000020	// add a index counter to the filenames
#define OPT_OBSOLETE		0x00000040	// Used to be OPT_ADD_ID3, now ignored
#define OPT_DATE_STAMP		0x00000100	// add a date stamp to the output directory
#define OPT_CHECK_MAX_BYTES	0x00000200	// use the maxMB_rip_size value to know how much to rip
#define OPT_KEEP_INCOMPLETE	0x00000400	// overwrite files in the incomplete directory, add counter instead
#define OPT_SINGLE_FILE_OUTPUT	0x00000800	// enable ripping to single file
#define OPT_TRUNCATE_DUPS	0x00001000	// truncate file in the incomplete directory already present in complete
#define OPT_INDIVIDUAL_TRACKS	0x00002000	// should we write the individual tracks?
#define OPT_EXTERNAL_CMD	0x00004000	// use external command to get metadata?
#define OPT_ADD_ID3V1		0x00008000	// Add ID3V1
#define OPT_ADD_ID3V2		0x00010000	// Add ID3V2

#define OPT_FLAG_ISSET(flags, opt)	    ((flags & opt) > 0)
// #define OPT_FLAG_SET(flags, opt)	    (flags =| opt)
#define OPT_FLAG_SET(flags, opt, val)	    (val ? (flags |= opt) : (flags &= (~opt)))

#define GET_AUTO_RECONNECT(flags)		(OPT_FLAG_ISSET(flags, OPT_AUTO_RECONNECT))
#define GET_SEPARATE_DIRS(flags)		(OPT_FLAG_ISSET(flags, OPT_SEPARATE_DIRS))
#define GET_SEARCH_PORTS(flags)			(OPT_FLAG_ISSET(flags, OPT_SEARCH_PORTS))
#define GET_MAKE_RELAY(flags)			(OPT_FLAG_ISSET(flags, OPT_MAKE_RELAY))
#define GET_COUNT_FILES(flags)			(OPT_FLAG_ISSET(flags, OPT_COUNT_FILES))
// #define GET_ADD_ID3(flags)			(OPT_FLAG_ISSET(flags, OPT_ADD_ID3))
#define GET_DATE_STAMP(flags)			(OPT_FLAG_ISSET(flags, OPT_DATE_STAMP))
#define GET_CHECK_MAX_BYTES(flags)		(OPT_FLAG_ISSET(flags, OPT_CHECK_MAX_BYTES))
#define GET_KEEP_INCOMPLETE(flags)		(OPT_FLAG_ISSET(flags, OPT_KEEP_INCOMPLETE))
#define GET_SINGLE_FILE_OUTPUT(flags)		(OPT_FLAG_ISSET(flags, OPT_SINGLE_FILE_OUTPUT))
#define GET_TRUNCATE_DUPS(flags)		(OPT_FLAG_ISSET(flags, OPT_TRUNCATE_DUPS))
#define GET_INDIVIDUAL_TRACKS(flags)		(OPT_FLAG_ISSET(flags, OPT_INDIVIDUAL_TRACKS))
#define GET_EXTERNAL_CMD(flags)			(OPT_FLAG_ISSET(flags, OPT_EXTERNAL_CMD))
#define GET_ADD_ID3V1(flags)			(OPT_FLAG_ISSET(flags, OPT_ADD_ID3V1))
#define GET_ADD_ID3V2(flags)			(OPT_FLAG_ISSET(flags, OPT_ADD_ID3V2))

/* Public functions */
char *rip_manager_get_error_str(int code);
//u_short rip_mananger_get_relay_port();	
void set_rip_manager_options_defaults (STREAM_PREFS *m_opt);
void rip_manager_init (void);
error_code rip_manager_start (RIP_MANAGER_INFO **rmi, STREAM_PREFS *prefs,
			      RIP_MANAGER_CALLBACK status_callback);
void rip_manager_stop (RIP_MANAGER_INFO *rmi);
void rip_manager_cleanup (void);
error_code rip_manager_start_track (RIP_MANAGER_INFO *rmi, TRACK_INFO* ti);
error_code rip_manager_end_track (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti);
error_code rip_manager_put_data (RIP_MANAGER_INFO *rmi, char *buf, int size);

char* client_relay_header_generate (RIP_MANAGER_INFO* rmi, 
				    int icy_meta_support);
void client_relay_header_release (char *ch);

const char*
overwrite_opt_to_string (enum OverwriteOpt oo);
enum OverwriteOpt string_to_overwrite_opt (char* str);
int rip_manager_get_content_type (RIP_MANAGER_INFO* rmi);

#endif //__RIP_MANANGER_H__
