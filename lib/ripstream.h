/* ripstream.h
 *
 * buffer stream data, when a track changes decodes the audio and 
 * finds a silent point to split the track
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
#ifndef __ripstream_h__
#define __ripstream_h__

#include "sr_config.h"
#include "rip_manager.h"
#include "srtypes.h"
#include "prefs.h"
#include "socklib.h"

error_code
ripstream_init (RIP_MANAGER_INFO* rmi);
error_code ripstream_rip (RIP_MANAGER_INFO* rmi);
void ripstream_destroy (RIP_MANAGER_INFO* rmi);
error_code
ripstream_get_data (RIP_MANAGER_INFO* rmi, char *data_buf, char *track_buf);
error_code 
ripstream_put_data (RIP_MANAGER_INFO *rmi, char *buf, int size);
error_code
ripstream_start_track (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti);
error_code
ripstream_queue_writer (
    RIP_MANAGER_INFO* rmi, 
    TRACK_INFO* ti, 
    Cbuf3_pointer start_byte
);
error_code
ripstream_end_track (RIP_MANAGER_INFO* rmi, Writer *writer);

#endif
