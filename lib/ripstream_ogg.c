/* This program is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/types.h>
#include <netinet/in.h>
#endif
#include "srtypes.h"
#include "cbuf3.h"
#include "findsep.h"
#include "mchar.h"
#include "parse.h"
#include "rip_manager.h"
#include "ripstream.h"
#include "debug.h"
#include "filelib.h"
#include "relaylib.h"
#include "socklib.h"
#include "external.h"
#include "ripogg.h"
#include "track_info.h"

/******************************************************************************
 * Private functions
 *****************************************************************************/
static error_code
ripstream_ogg_handle_bos (
    RIP_MANAGER_INFO* rmi, 
    TRACK_INFO* ti,
    Ogg_page_reference *opr
)
{
    Cbuf3 *cbuf3 = &rmi->cbuf3;
    GQueue *write_list = cbuf3->write_list;
    if (!rmi->write_data) {
	return SR_SUCCESS;
    }

    debug_printf ("ripstream_ogg_handle_bos: testing track_info\n");
    if (rmi->current_track.have_track_info) {
	if (track_info_different (&rmi->old_track, &rmi->current_track)) {
	    error_code rc;

	    if (rmi->ogg_track_state == 2) {
		Writer *writer = (Writer*) g_queue_pop_head (write_list);

		debug_printf ("ripstream_ogg_handle_bos: ending track\n");

		/* Close file, etc. */
		rc = ripstream_end_track (rmi, writer);
		if (rc != SR_SUCCESS) {
		    return rc;
		}

		/* Free up writer */
		free (writer);
	    }

	    debug_printf ("ripstream_ogg_handle_bos: starting track\n");

	    rc = ripstream_queue_writer (rmi, &rmi->current_track, 
		opr->m_cbuf3_loc);
	    if (rc != SR_SUCCESS) {
		debug_printf ("ripstream_queue_writer: returned bad error "
		    "code: %d\n", rc);
		return rc;
	    }
	    track_info_copy (&rmi->old_track, &rmi->current_track);
	}
	rmi->ogg_track_state = 1;
    }
    return SR_SUCCESS;
}

static error_code
ripstream_ogg_write_page (RIP_MANAGER_INFO *rmi, Ogg_page_reference *opr)
{
    Cbuf3 *cbuf3 = &rmi->cbuf3;
    GQueue *write_list = cbuf3->write_list;
    error_code rc;
    u_long bytes_remaining;
    Writer *writer;

    /* Get writer -- there will always be only one writer for ogg */
    if (!write_list->head) {
	debug_printf ("No write_list->head.  Why?\n");
    }

    writer = (Writer*) write_list->head->data;
    debug_printf ("Writer: (%p,%p) %s\n", 
	writer->m_next_byte.node, 
	writer->m_last_byte.node,
	writer->m_ti.raw_metadata);

    /* Open output file if needed */
    if (!writer->m_started) {
	rc = filelib_start (rmi, writer, &writer->m_ti);
	if (rc != SR_SUCCESS) {
	    debug_printf ("filelib_start failed %d\n", rc);
	    return rc;
	}
	writer->m_started = 1;
    }

    /* Loop, writing up to one node at a time */
    bytes_remaining = opr->m_page_len;
    while (bytes_remaining > 0) {
	GList *node;
	u_long node_bytes;
	char* write_ptr;
	long write_sz;

	/* Compute unwritten bytes left in this node */
	node = writer->m_next_byte.node;
	node_bytes = cbuf3->chunk_size - writer->m_next_byte.offset;

	/* Compare unwritten bytes in this chunk with unwritten bytes 
	   for the ogg page. */
	if (node_bytes < bytes_remaining) {
	    write_sz = node_bytes;
	} else {
	    write_sz = bytes_remaining;
	}
	write_ptr = ((char*) node->data) + writer->m_next_byte.offset;
	
	debug_printf ("computed write at (%p,%d), len = %d, br = %d\n",
	    writer->m_next_byte.node,
	    writer->m_next_byte.offset,
	    write_sz, bytes_remaining);

	/* Do the actual write -- showfile */
	rc = filelib_write_show (rmi, write_ptr, write_sz);
	if (rc != SR_SUCCESS) {
	    debug_printf("filelib_write_show had bad return code: %d\n", rc);
	    return rc;
	}

	/* Do the actual write -- individual tracks */
	if (GET_INDIVIDUAL_TRACKS (rmi->prefs->flags) && (rmi->write_data)) {
	    rc = filelib_write_track (writer, write_ptr, write_sz);
	    if (rc != SR_SUCCESS) {
		debug_printf ("filelib_write_track returned: %d\n", rc);
		return rc;
	    }
	}
	bytes_remaining -= write_sz;

	/* Advance writer */
	cbuf3_pointer_add (cbuf3, &writer->m_next_byte, 
	    &writer->m_next_byte, write_sz);
    }

    return SR_SUCCESS;
}

/******************************************************************************
 * Public functions
 *****************************************************************************/
error_code
ripstream_ogg_rip (RIP_MANAGER_INFO* rmi)
{
    error_code rc;
    GList *node;
    Cbuf3 *cbuf3 = &rmi->cbuf3;

    debug_printf ("RIPSTREAM_RIP_OGG: top of loop\n");

    if (rmi->ripstream_first_time_through) {
	/* Allocate circular buffer */
	rmi->detected_bitrate = -1;
	rmi->bitrate = -1;
	rmi->getbuffer_size = 1024;
	rmi->cbuf2_size = 128;
	rc = cbuf3_init (cbuf3, rmi->http_info.content_type, 
	    GET_MAKE_RELAY(rmi->prefs->flags),
	    rmi->getbuffer_size, rmi->cbuf2_size);
	if (rc != SR_SUCCESS) {
	    return rc;
	}
	rmi->ogg_track_state = 0;
	rmi->ogg_fixed_page_no = 0;
	/* Warm up the ogg decoding routines */
	ripogg_init (rmi);
	/* Done! */
	rmi->ripstream_first_time_through = 0;
    }

    /* get the data from the stream */
    node = cbuf3_request_free_node (rmi, cbuf3);
    rc = ripstream_get_data (rmi, node->data, rmi->current_track.raw_metadata);
    if (rc != SR_SUCCESS) {
	debug_printf ("get_stream_data bad return code: %d\n", rc);
	return rc;
    }

    /* Copy the data into cbuffer */
    rc = cbuf3_insert_node (cbuf3, node);
    if (rc != SR_SUCCESS) {
	debug_printf ("cbuf3_insert had bad return code %d\n", rc);
	return rc;
    }

    /* Fill in this_page_list with ogg page references */
    track_info_clear (&rmi->current_track);
    ripogg_process_chunk (rmi, node->data, cbuf3->chunk_size, 
	&rmi->current_track);

    debug_printf ("ogg_track_state[a] = %d\n", rmi->ogg_track_state);

    /* For ogg, we write immediately upon receipt of complete ogg pages.
       But we still use the writers to encapsulate the cbuf3 pointers. */
    do {
	GList *opr_node;
	Ogg_page_reference *opr;

	/* Get an unwritten page */
	cbuf3_ogg_peek_page (cbuf3, &opr_node);
	if (!opr_node) {
	    break;
	}

	/* We got an unwritten page */
	opr = (Ogg_page_reference *) opr_node->data;

	/* If new track, then (maybe) open a new file */
	debug_printf ("Testing unwritten page [%p, %p]\n", opr_node, opr);
	if (opr->m_page_flags & OGG_PAGE_BOS) {
	    /* If the BOS page is the last page, it means 
	       we haven't yet parsed the PAGE2. */
	    if (!opr_node->next) {
		break;
	    }
	    rc = ripstream_ogg_handle_bos (rmi, &rmi->current_track, opr);
	    if (rc != SR_SUCCESS) {
		return rc;
	    }
	}

	/* Write page to disk */
	debug_printf ("Trying to write ogg page to disk\n");
	ripstream_ogg_write_page (rmi, opr);

	if (opr->m_page_flags & OGG_PAGE_EOS) {
	    /* We don't immediately close the file here.  If the next track
	       has the same title, we concatenate it to the current track. */
	    rmi->ogg_track_state = 2;
	}

	/* Advance pointer to next page to write */
	debug_printf ("Advancing ogg written page in cbuf3\n");
	cbuf3_ogg_advance_page (cbuf3);

    } while (1);

    /* If buffer is full eject oldest node */
    rc = cbuf3_ogg_remove_old_page_references (cbuf3);
    if (rc != SR_SUCCESS) {
	debug_printf ("ripstream_ogg_eject_oldest_node returned: %d\n", rc);
	return rc;
    }

    /* Give some feedback to the user about how much we ripped so far */
    callback_put_data (rmi, rmi->getbuffer_size);

    return SR_SUCCESS;
}
