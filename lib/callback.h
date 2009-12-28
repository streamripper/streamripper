/* callback.h
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
#ifndef __CALLBACK_H__
#define __CALLBACK_H__

#include "srtypes.h"

// Messages for status_callback hook in rip_manager_init()
// used for notifing to client whats going on *DO NOT* call 
// rip_mananger_start or rip_mananger_stop from
// these functions!!! it will cause a deadlock
#define RM_UPDATE	0x01	// returns a pointer RIP_MANAGER_INFO struct
#define RM_ERROR	0x02	// returns the error code
#define RM_DONE		0x03	// NULL
#define RM_STARTED	0x04	// NULL
#define RM_NEW_TRACK	0x05	// Name of the new track
#define RM_TRACK_DONE	0x06	// pull path of the track completed
// RM_OUTPUT_DIR is now OBSOLETE
#define RM_OUTPUT_DIR	0x07	// Full path of the output directory

// The following are the possible status values for RIP_MANAGER_INFO
#define RM_STATUS_BUFFERING		0x01
#define RM_STATUS_RIPPING		0x02
#define RM_STATUS_RECONNECTING		0x03

/* Public functions */
void
callback_post_status (RIP_MANAGER_INFO* rmi, int status);
void
callback_post_error (RIP_MANAGER_INFO* rmi, error_code err);
error_code
callback_start_track (RIP_MANAGER_INFO *rmi, TRACK_INFO* ti);
void
callback_put_data (RIP_MANAGER_INFO *rmi, u_long size);

#endif //__CALLBACK_H__
