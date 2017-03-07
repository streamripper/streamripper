/* cbuf2.h
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
#ifndef __CBUF2_H__
#define __CBUF2_H__

#include "srtypes.h"
#include "threadlib.h"
#include "list.h"
#include "relaylib.h"

/* Each metadata within the cbuf gets this struct */
typedef struct METADATA_LIST_struct METADATA_LIST;
struct METADATA_LIST_struct
{
    unsigned long m_chunk;
    /* m_composed_metadata includes 1 byte for size*16 */
    char m_composed_metadata[MAX_METADATA_LEN+1];
    LIST m_list;
};

#define OGG_PAGE_BOS        0x01
#define OGG_PAGE_EOS        0x02
#define OGG_PAGE_2          0x04


/*****************************************************************************
 * Global variables
 *****************************************************************************/
extern CBUF2 g_cbuf2;


/*****************************************************************************
 * Function prototypes
 *****************************************************************************/
error_code
cbuf2_init (CBUF2 *cbuf2, int content_type, int have_relay, 
	    unsigned long chunk_size, unsigned long num_chunks);
void cbuf2_destroy(CBUF2 *buffer);
error_code cbuf2_extract (RIP_MANAGER_INFO* rmi, CBUF2 *cbuf2, char *data, 
			  u_long count, u_long* curr_song);
error_code cbuf2_peek(CBUF2 *buffer, char *items, u_long count);
error_code cbuf2_insert_chunk (RIP_MANAGER_INFO* rmi, 
			       CBUF2 *cbuf2, const char *data, u_long count,
			       int content_type, TRACK_INFO* ti);
error_code cbuf2_fastforward (CBUF2 *buffer, u_long count);
error_code cbuf2_peek_rgn (CBUF2 *buffer, char *out_buf, 
			   u_long start, u_long length);

u_long cbuf2_get_free(CBUF2 *buffer);
u_long cbuf2_get_free_tail (CBUF2 *cbuf2);
u_long cbuf2_write_index (CBUF2 *cbuf2);

void cbuf2_set_next_song (CBUF2 *cbuf2, u_long pos);

error_code cbuf2_init_relay_entry (CBUF2 *cbuf2, RELAY_LIST* ptr, 
				   u_long burst_request);

error_code
cbuf2_ogg_peek_song (CBUF2 *cbuf2, char* out_buf, 
		     unsigned long buf_size,
		     unsigned long* amt_filled,
		     int* eos);

#if defined (commentout)
error_code cbuf2_extract_relay (CBUF2 *cbuf2, char *data, u_long *pos, 
				u_long *len, int icy_metadata);
#endif
error_code cbuf2_extract_relay (CBUF2 *cbuf2, RELAY_LIST* ptr);
error_code cbuf2_advance_ogg (RIP_MANAGER_INFO* rmi, CBUF2 *cbuf2, int requested_free_size);

#endif
