/* relaylib.h
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
#ifndef __RELAYLIB_H__
#define __RELAYLIB_H__

#include "srtypes.h"
#include "errors.h"
#include "compat.h"

/*****************************************************************************
 * Global variables
 *****************************************************************************/
//extern RELAY_LIST* g_relay_list;
//extern unsigned long g_relay_list_len;
//extern HSEM g_relay_list_sem;


/*****************************************************************************
 * Function prototypes
 *****************************************************************************/
error_code relaylib_set_response_header(char *http_header);
error_code
relaylib_start (RIP_MANAGER_INFO* rmi,
		BOOL search_ports, u_short relay_port, u_short max_port, 
		u_short *port_used, char *if_name, int max_connections, 
		char *relay_ip, int have_metadata);
//error_code relaylib_start(RIP_MANAGER_INFO* rmi);
error_code relaylib_send(char *data, int len, int accept_new, int is_meta);
void relaylib_stop (RIP_MANAGER_INFO* rmi);
BOOL relaylib_isrunning();
error_code relaylib_send_meta_data(char *track);
void relaylib_disconnect (RIP_MANAGER_INFO* rmi, 
			  RELAY_LIST* prev, RELAY_LIST* ptr);

#endif //__RELAYLIB__
