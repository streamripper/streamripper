/* callback.c
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
/******************************************************************************
 * Rip Manager API
 *
 *   Callback Function
 *     void (*status_callback)(RIP_MANAGER_INFO* rmi, 
 *                              int message, void *data));
 *   Functions
 *     void rip_manager_init (void);
 *     error_code rip_manager_start (RIP_MANAGER_INFO **rmi, 
 *	  STREAM_PREFS *prefs, RIP_MANAGER_CALLBACK status_callback);
 *     void rip_manager_stop (RIP_MANAGER_INFO *rmi);
 *     void rip_manager_cleanup (void);
 *
 *****************************************************************************/
/*! \file */
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

#include "errors.h"
#include "filelib.h"
#include "socklib.h"
#include "mchar.h"
#include "findsep.h"
#include "relaylib.h"
#include "rip_manager.h"
#include "ripstream.h"
#include "threadlib.h"
#include "debug.h"
#include "sr_compat.h"
#include "callback.h"


/******************************************************************************
 * Public functions
 *****************************************************************************/
void
callback_post_status (RIP_MANAGER_INFO* rmi, int status)
{
    if (status != 0)
        rmi->status = status;
    rmi->status_callback (rmi, RM_UPDATE, 0);
}

void
callback_post_error (RIP_MANAGER_INFO* rmi, error_code err)
{
    ERROR_INFO err_info;
    err_info.error_code = err;
    strcpy(err_info.error_str, errors_get_string (err));
    debug_printf ("post_error: %d %s\n", err_info.error_code, 
		  err_info.error_str);
    rmi->status_callback (rmi, RM_ERROR, &err_info);
}

/* 
 * This is called by ripstream when we get a new track. 
 * most logic is handled by filelib_start() so we just
 * make sure there are no bad characters in the name and 
 * update via the callback 
 */
error_code
callback_start_track (RIP_MANAGER_INFO *rmi, TRACK_INFO* ti)
{
    mchar console_string[SR_MAX_PATH];

    /* Compose the string for the console output */
    msnprintf (console_string, SR_MAX_PATH, m_S m_(" - ") m_S, 
	       ti->artist, ti->title);
    rmi->callback_filesize = 0;
    string_from_gstring (rmi, rmi->callback_filename, 
			 SR_MAX_PATH, console_string, 
			 CODESET_LOCALE);
    rmi->callback_filename[SR_MAX_PATH-1] = '\0';
    rmi->status_callback (rmi, RM_NEW_TRACK, (void*) rmi->callback_filename);
    callback_post_status (rmi, 0);

    return SR_SUCCESS;
}

void
callback_put_data (RIP_MANAGER_INFO *rmi, u_long size)
{
    /* This is used by the GUI */
    rmi->callback_filesize += size;

    /* This is used to determine when to quit */
    rmi->bytes_ripped += size;
    while (rmi->bytes_ripped >= 1048576) {
	rmi->bytes_ripped -= 1048576;
	rmi->megabytes_ripped++;
    }
}
