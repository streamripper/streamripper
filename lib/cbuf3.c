/* cbuf3.c
 * circular buffer lib - version 3
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
#include <stdlib.h>
#include <string.h>
#include "srtypes.h"
#include "errors.h"
#include "cbuf3.h"
#include "threadlib.h"
#include "relaylib.h"
#include "debug.h"

static void
cbuf3_disconnect_slow_clients (RIP_MANAGER_INFO *rmi, Cbuf3 *cbuf3);


/******************************************************************************
 * Public functions
 *****************************************************************************/
error_code
cbuf3_init (struct cbuf3 *cbuf3, 
	    int content_type, 
	    int have_relay, 
	    unsigned long chunk_size, 
	    unsigned long num_chunks)
{
    debug_printf ("Initializing cbuf3\n");

    if (chunk_size == 0 || num_chunks == 0) {
        return SR_ERROR_INVALID_PARAM;
    }
    cbuf3->have_relay = have_relay;
    cbuf3->content_type = content_type;
    cbuf3->chunk_size = chunk_size;

    cbuf3->buf = g_queue_new ();
    cbuf3->free_list = g_queue_new ();
    cbuf3->pending = 0;
    cbuf3->num_chunks = 0;

    /* Ogg stuff */
    cbuf3->ogg_page_refs = g_queue_new ();

    /* Mp3 stuff */
    cbuf3->write_list = g_queue_new ();
    cbuf3->metadata_list = g_queue_new ();

    //    cbuf2->next_song = 0;        /* MP3 only */
    //    cbuf2->song_page = 0;        /* OGG only */
    //    cbuf2->song_page_done = 0;   /* OGG only */

    cbuf3->sem = threadlib_create_sem();
    threadlib_signal_sem (&cbuf3->sem);

    /* Allocate chunks */
    cbuf3_allocate_minimum (cbuf3, num_chunks);

    return SR_SUCCESS;
}

error_code
cbuf3_allocate_minimum (struct cbuf3 *cbuf3, 
			unsigned long num_chunks)
{
    int i;

    debug_printf ("Allocating cbuf3\n");

    if (num_chunks == 0) {
        return SR_ERROR_INVALID_PARAM;
    }

    threadlib_waitfor_sem (&cbuf3->sem);

    if (cbuf3->num_chunks >= num_chunks) {
	/* Already have enough chunks */
	threadlib_signal_sem (&cbuf3->sem);
	return SR_SUCCESS;
    }

    for (i = 0; i < num_chunks - cbuf3->num_chunks; i++) {
	char* chunk = (char*) malloc (cbuf3->chunk_size);
	if (!chunk) {
	    threadlib_signal_sem (&cbuf3->sem);
	    return SR_ERROR_CANT_ALLOC_MEMORY;
	}
	g_queue_push_head (cbuf3->free_list, chunk);
    }
    cbuf3->num_chunks = num_chunks;

    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("Allocating cbuf3 [complete]\n");
    return SR_SUCCESS;
}

void
cbuf3_destroy (struct cbuf3 *cbuf3)
{
    char *c;

    /* Remove buffer */
    if (cbuf3->buf) {
	while ((c = g_queue_pop_head (cbuf3->buf)) != 0) {
	    free (c);
	}
	g_queue_free (cbuf3->buf);
	cbuf3->buf = 0;
    }

    /* Remove free_list */
    if (cbuf3->free_list) {
	while ((c = g_queue_pop_head (cbuf3->free_list)) != 0) {
	    free (c);
	}
	g_queue_free (cbuf3->free_list);
	cbuf3->free_list = 0;
    }

    /* Remove ogg page references */
    if (cbuf3->ogg_page_refs) {
	while ((c = g_queue_pop_head (cbuf3->ogg_page_refs)) != 0) {
	    free (c);
	}
	g_queue_free (cbuf3->ogg_page_refs);
	cbuf3->ogg_page_refs = 0;
    }
}

void
cbuf3_debug_free_list (Cbuf3 *cbuf3)
{
    debug_printf ("Free_list: %d nodes.\n", cbuf3->free_list->length);
}

/** Return 1 if there are no more empty nodes. */
int
cbuf3_is_full (Cbuf3 *cbuf3)
{
    return g_queue_is_empty (cbuf3->free_list);
}

/* Returns a free node */
GList*
cbuf3_request_free_node (RIP_MANAGER_INFO *rmi,
			  struct cbuf3 *cbuf3)
{
    /* If there is a free chunk, return it */
    /* No need to lock, only the main thread accesses free_list */
    if (! g_queue_is_empty (cbuf3->free_list)) {
	debug_printf ("Free node from empty list [%d].\n",
		      cbuf3->free_list->length);
	return g_queue_pop_head_link (cbuf3->free_list);
    }

    /* Otherwise, we have to eject the oldest chunk from buf. */
    debug_printf ("Free node from used list.\n");
    return cbuf3_extract_oldest_node (rmi, cbuf3);
}

error_code
cbuf3_insert_node (struct cbuf3 *cbuf3, GList *node)
{
    int i;
    GList *p;

    /* Insert node to cbuf */
    debug_printf ("cbuf3_insert is waiting for cbuf3->sem\n");
    threadlib_waitfor_sem (&cbuf3->sem);
    debug_printf ("cbuf3_insert got cbuf3->sem\n");

    debug_printf ("CBUF_INSERT\n");
    debug_printf ("       Node       Data\n");

    node->prev = node->next = 0;
    g_queue_push_tail_link (cbuf3->buf, node);

    for (i = 0, p = cbuf3->buf->head; p; i++, p = p->next) {
	debug_printf ("[%3d]  %p, %p\n", i, p, p->data);
    }
    debug_printf ("filled = %d x %d = %d\n", i, 
	cbuf3->chunk_size, i * cbuf3->chunk_size);
    debug_printf ("size   = %d x %d = %d\n", cbuf3->num_chunks, 
	cbuf3->chunk_size, cbuf3->num_chunks * cbuf3->chunk_size);

    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("cbuf3_insert released cbuf3->sem\n");
    return SR_SUCCESS;
}

void
cbuf3_insert_free_node (struct cbuf3 *cbuf3, GList *node)
{
    /* No need to lock, only the main thread accesses free_list */
    debug_printf ("Inserting free node\n");
    node->prev = node->next = 0;
    g_queue_push_head_link (cbuf3->free_list, node);
}

/** Destroy ogg page references in the oldest node. */
error_code
cbuf3_ogg_remove_old_page_references (Cbuf3 *cbuf3)
{
    Ogg_page_reference *opr;

    if (!cbuf3_is_full (cbuf3)) {
	return SR_ERROR_BUFFER_NOT_FULL;
    }

    /* Loop through page references, starting at head, looking for 
       opr which point to the head node in the buffer */
    opr = (Ogg_page_reference *) cbuf3->ogg_page_refs->head->data;
    debug_printf ("Testing old page references:\n"
	"  opr head = (%p,%d)\n"
	"  cbuf3 head = (%p)\n",
	opr->m_cbuf3_loc.node, opr->m_cbuf3_loc.offset,
	cbuf3->buf->head);
    while (cbuf3->ogg_page_refs->head 
	&& (opr = (Ogg_page_reference *) cbuf3->ogg_page_refs->head->data)
	&& (opr->m_cbuf3_loc.node == cbuf3->buf->head))
    {
	/* Remove opr from the queue */
	opr = g_queue_pop_head (cbuf3->ogg_page_refs);

	debug_printf ("Removed ogg page reference: [%p,%5d,%5d]\n",
	    opr->m_cbuf3_loc.node,
	    opr->m_cbuf3_loc.offset,
	    opr->m_page_len);

	/* If there could be a really large ogg page (like 100K length) 
	   which is not yet completely downloaded, we will drop the page, 
	   but try not to crash. */
	if (cbuf3->written_page == opr->m_cbuf3_loc.node) {
	    cbuf3->written_page = 0;
	}
	/* GCS FIX: Do equivalent for the writer */
	
	/* The opr might be the last one which points to the ogg 
	   track header.  In this case, we have some extra work to 
	   free the memory stored in the ref. */
	if (opr->m_page_flags & OGG_PAGE_EOS) {
	    free (opr->m_header_buf_ptr);
	}

	/* Free the opr */
	free (opr);
    }
    return SR_SUCCESS;
}

/** Return oldest node.  Oldest nodes are at the head. */
GList*
cbuf3_extract_oldest_node (RIP_MANAGER_INFO *rmi,
			   struct cbuf3 *cbuf3)
{
    GList *node;

    /* Relay threads access used list, so we need to lock. */
    if (cbuf3->have_relay) {
	debug_printf ("cbuf3_extract_oldest_node is waiting for rmi->relay_list_sem\n");
	threadlib_waitfor_sem (&rmi->relay_list_sem);
	debug_printf ("cbuf3_extract_oldest_node got rmi->relay_list_sem\n");
    }

    debug_printf ("cbuf3_extract_oldest_node is waiting for cbuf3->sem\n");
    threadlib_waitfor_sem (&cbuf3->sem);
    debug_printf ("cbuf3_extract_oldest_node got cbuf3->sem\n");

    /* Disconnect clients which reference the node at head of buf */
    if (cbuf3->have_relay) {
	cbuf3_disconnect_slow_clients (rmi, cbuf3);
    }

    /* Remove the chunk */
    node = g_queue_pop_head_link (cbuf3->buf);

    /* Done */
    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("cbuf3_extract_oldest_node released cbuf3->sem\n");
    if (cbuf3->have_relay) {
	threadlib_signal_sem (&rmi->relay_list_sem);
	debug_printf ("cbuf3_extract_oldest_node released rmi->relay_list_sem\n");
    }

    return node;
}

error_code
cbuf3_insert_metadata (struct cbuf3 *cbuf3, TRACK_INFO* ti)
{
    if (ti && ti->have_track_info) {
	Metadata *metadata;
	threadlib_waitfor_sem (&cbuf3->sem);

	metadata = (Metadata*) malloc (sizeof(Metadata));
	if (!metadata) {
	    threadlib_signal_sem (&cbuf3->sem);
	    return SR_ERROR_CANT_ALLOC_MEMORY;
	}

	metadata->m_node = cbuf3->buf->tail;
	memcpy (metadata->m_composed_metadata, 
		ti->composed_metadata, 
		MAX_METADATA_LEN+1);
	g_queue_push_tail (cbuf3->metadata_list, metadata);
	threadlib_signal_sem (&cbuf3->sem);
    }
    return SR_SUCCESS;
}

/* Return cbuf3_pointer that points to first byte */
void
cbuf3_get_head (
    Cbuf3 *cbuf3,
    struct cbuf3_pointer *out_ptr)
{
    out_ptr->node = cbuf3->buf->head;
    out_ptr->offset = 0;
}

/* Return cbuf3_pointer that points just after last byte */
void
cbuf3_get_tail (
    Cbuf3 *cbuf3,
    struct cbuf3_pointer *out_ptr)
{
    out_ptr->node = cbuf3->buf->tail;
    out_ptr->offset = cbuf3->chunk_size;
}

void
cbuf3_pointer_copy (
    struct cbuf3_pointer *out_ptr, 
    struct cbuf3_pointer *in_ptr)
{
    out_ptr->node = in_ptr->node;
    out_ptr->offset = out_ptr->offset;
}

error_code
cbuf3_pointer_add (struct cbuf3 *cbuf3, 
		   struct cbuf3_pointer *out_ptr, 
		   struct cbuf3_pointer *in_ptr, 
		   long len)
{
    debug_printf ("add: %p,%d + %d\n", in_ptr->node, in_ptr->offset, len);
    out_ptr->node = in_ptr->node;
    out_ptr->offset = in_ptr->offset;

    /* For positive len */
    while (len > 0) {
	out_ptr->offset += len;
	len = 0;
	/* Check for advancing to next node */
	if (out_ptr->offset >= cbuf3->chunk_size) {
	    len = out_ptr->offset - cbuf3->chunk_size;
	    out_ptr->offset = 0;
	    out_ptr->node = out_ptr->node->next;
	    /* Check for overflow */
	    if (!out_ptr->node) {
		return SR_ERROR_BUFFER_TOO_SMALL;
	    }
	}
    }

    /* For negative len - note offset is unsigned */
    while (len < 0) {
	long tmp = (long) out_ptr->offset + len;
	len = 0;
	/* Check for rewinding to prev node */
	if (tmp < 0) {
	    len = tmp;
	    out_ptr->offset = cbuf3->chunk_size;
	    out_ptr->node = out_ptr->node->prev;
	    /* Check for overflow */
	    if (!out_ptr->node) {
		//debug_printf ("     (overflow)\n");
		return SR_ERROR_BUFFER_TOO_SMALL;
	    }
	} else {
	    out_ptr->offset = (u_long) tmp;
	}
    }

    //debug_printf ("     %p,%d\n", out_ptr->chunk, out_ptr->offset);
    return SR_SUCCESS;
}

/* Note: ptr2 must be after ptr1 */
error_code
cbuf3_pointer_subtract (
    Cbuf3 *cbuf3,                 /* Input */
    u_long *diff,                 /* Output */
    struct cbuf3_pointer *ptr1,   /* Input */
    struct cbuf3_pointer *ptr2    /* Input */
)
{
    struct cbuf3_pointer p;

    cbuf3_pointer_copy (&p, ptr1);
    *diff = 0;
    
    do {
	if (p.node == ptr2->node) {
	    if (p.offset > ptr2->offset) {
		return SR_ERROR_BUFFER_TOO_SMALL;
	    }
	    *diff += ptr2->offset - p.offset;
	    return SR_SUCCESS;
	} else {
	    *diff += cbuf3->chunk_size - p.offset;
	    p.node = p.node->next;
	    p.offset = 0;
	}
    } while (p.node);
    return SR_ERROR_BUFFER_TOO_SMALL;
}

error_code
cbuf3_set_uint32 (struct cbuf3 *cbuf3, 
		  struct cbuf3_pointer *in_ptr, 
		  uint32_t val)
{
    u_long i;
    struct cbuf3_pointer tmp;
    error_code rc;

    //debug_printf ("set: %p,%d\n", in_ptr->node, in_ptr->offset);
    tmp.node = in_ptr->node;
    tmp.offset = in_ptr->offset;

    for (i = 0; i < 4; i++) {
	char *buf;
	rc = cbuf3_pointer_add (cbuf3, &tmp, in_ptr, i);
	if (rc != SR_SUCCESS) {
	    return rc;
	}
	buf = (char*) tmp.node->data;
	buf[tmp.offset] = (val & 0xff);
	val >>= 8;
    }
    
    return SR_SUCCESS;
}

void
cbuf3_debug_ogg_page_ref (struct ogg_page_reference *opr, void *user_data)
{
    debug_printf ("  chk/off/len: [%p,%5d,%5d] hdr: [%p,%5d] flg: %2d\n", 
		  opr->m_cbuf3_loc.node,
		  opr->m_cbuf3_loc.offset,
		  opr->m_page_len,
		  opr->m_header_buf_ptr,
		  opr->m_header_buf_len,
		  opr->m_page_flags);
}

void
cbuf3_debug_ogg_page_list (struct cbuf3 *cbuf3)
{
    debug_printf ("Ogg page references:\n");
    g_queue_foreach (cbuf3->ogg_page_refs, 
		     (GFunc) cbuf3_debug_ogg_page_ref, 0);
}

void
cbuf3_splice_page_list (struct cbuf3 *cbuf3, 
			GList **new_pages)
{
    /* Do I need to lock?  If so, read access should lock too? */
    debug_printf ("cbuf3_splice_page_list is waiting for cbuf3->sem\n");
    threadlib_waitfor_sem (&cbuf3->sem);
    debug_printf ("cbuf3_splice_page_list got cbuf3->sem\n");

    while (*new_pages) {
	GList *ele = (*new_pages);
	(*new_pages) = g_list_remove_link ((*new_pages), ele);
	g_queue_push_tail_link (cbuf3->ogg_page_refs, ele);
    }

    cbuf3_debug_ogg_page_list (cbuf3);
    
    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("cbuf3_splice_page_list released cbuf3->sem\n");
}

/* Sets (*page_node) if a page was found */
void
cbuf3_ogg_peek_page (Cbuf3 *cbuf3, 
		     GList **page_node)
{
    if (!cbuf3->written_page) {
	if (cbuf3->ogg_page_refs->head) {
	    cbuf3->written_page = cbuf3->ogg_page_refs->head;
	} else {
	    (*page_node) = 0;
	    return;
	}
    }
    
    if (cbuf3->written_page->next) {
	(*page_node) = cbuf3->written_page;
    } else {
	(*page_node) = 0;
    }
}

void
cbuf3_ogg_advance_page (Cbuf3 *cbuf3)
{
    cbuf3->written_page = cbuf3->written_page->next;
}

/* This sets the m_cbuf_ptr (and related items) within the 
   relay_client.  If it fails, m_cbuf_ptr.node is left unchanged 
   at zero.  */
error_code
cbuf3_initialize_relay_client_ptr (struct cbuf3 *cbuf3,
		       struct relay_client *relay_client,
		       u_long burst_request)
{
    debug_printf ("cbuf3_add_relay_entry is waiting for cbuf3->sem\n");
    threadlib_waitfor_sem (&cbuf3->sem);
    debug_printf ("cbuf3_add_relay_entry got cbuf3->sem\n");

    /* Find a Cbuf3_pointer for the relay to start sending */
    if (cbuf3->content_type == CONTENT_TYPE_OGG) {
	GList *ogg_page_ptr;
	Ogg_page_reference *opr;
	u_long burst_amt = 0;

	/* For ogg, walk through the ogg pages in reverse order to 
	   find page at location > burst_request. */
	ogg_page_ptr = cbuf3->ogg_page_refs->tail;
	if (!ogg_page_ptr) {
	    debug_printf ("Error.  No data for relay\n");
	    threadlib_signal_sem (&cbuf3->sem);
	    return SR_ERROR_NO_DATA_FOR_RELAY;
	}
	opr = (Ogg_page_reference *) ogg_page_ptr->data;
	burst_amt += opr->m_page_len;
	debug_printf ("OGG_ADD_RELAY_ENTRY: BA=%d\n", burst_amt);

	while (burst_amt < burst_request) {
	    if (!ogg_page_ptr->prev) {
		debug_printf ("Hit list head while spinning back\n");
		break;
	    }
	    ogg_page_ptr = ogg_page_ptr->prev;
	    opr = (Ogg_page_reference *) ogg_page_ptr->data;
	    burst_amt += opr->m_page_len;
	    debug_printf ("OGG_ADD_RELAY_ENTRY: BA=%d\n", burst_amt);
	}

	/* If the desired ogg page is a header page, spin forward 
	   to a non-header page */
	while (opr->m_page_flags & (OGG_PAGE_BOS | OGG_PAGE_2)) {
	    if (!ogg_page_ptr->next) {
		debug_printf ("Error.  No data for relay\n");
		threadlib_signal_sem (&cbuf3->sem);
		return SR_ERROR_NO_DATA_FOR_RELAY;
	    }
	    ogg_page_ptr = ogg_page_ptr->next;
	    opr = (Ogg_page_reference *) ogg_page_ptr->data;
	}
	relay_client->m_cbuf_ptr.node = opr->m_cbuf3_loc.node;
	relay_client->m_cbuf_ptr.offset = opr->m_cbuf3_loc.offset;
	relay_client->m_header_buf_ptr = opr->m_header_buf_ptr;
	relay_client->m_header_buf_len = opr->m_header_buf_len;
	relay_client->m_header_buf_off = 0;

    } else {
	GList *node_ptr;
	u_long burst_amt = 0;

	/* For mp3 et al., walk through nodes in reverse order */
	node_ptr = cbuf3->buf->tail;
	if (!node_ptr) {
	    debug_printf ("Error.  No data for relay\n");
	    threadlib_signal_sem (&cbuf3->sem);
	    return SR_ERROR_NO_DATA_FOR_RELAY;
	}
	burst_amt += cbuf3->chunk_size;

	while (burst_amt < burst_request) {
	    node_ptr = node_ptr->prev;
	    if (!node_ptr) {
		break;
	    }
	    burst_amt += cbuf3->chunk_size;
	}

	relay_client->m_cbuf_ptr.node = node_ptr;
	relay_client->m_cbuf_ptr.offset = 0;
    }

    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("cbuf3_add_relay_entry released cbuf3->sem\n");
    return SR_SUCCESS;
}

error_code 
cbuf3_extract_relay (Cbuf3 *cbuf3,
		     Relay_client *relay_client)
{
    GList *node;
    char *chunk;
    u_long offset, remaining;
    error_code ec;

    debug_printf ("cbuf3_extract_relay is waiting for cbuf3->sem\n");
    threadlib_waitfor_sem (&cbuf3->sem);
    debug_printf ("cbuf3_extract_relay got cbuf3->sem\n");

    debug_printf ("EXTRACT_RELAY\n");

    node = relay_client->m_cbuf_ptr.node;
    offset = relay_client->m_cbuf_ptr.offset;
    chunk = (char*) node->data;
    remaining = cbuf3->chunk_size - offset;


    if (node->next == NULL) {
	ec = SR_ERROR_BUFFER_EMPTY;
    } else {
	debug_printf ("Client %d node %p\n", relay_client->m_sock, node);
	memcpy (relay_client->m_buffer, &chunk[offset], remaining);
	relay_client->m_left_to_send = remaining;
	relay_client->m_cbuf_ptr.node = node->next;
	relay_client->m_cbuf_ptr.offset = 0;
	ec = SR_SUCCESS;
    }

    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("cbuf3_extract_relay released cbuf3->sem\n");
    return ec;
}

error_code 
cbuf3_peek (Cbuf3 *cbuf3,
	    char *buf,
	    Cbuf3_pointer *ptr,
	    u_long len)
{
    int bidx = 0;
    Cbuf3_pointer cur;

    debug_printf ("cbuf3_peek, %d bytes at cbuf3_ptr (%p,%d)\n",
		  len, ptr->node, ptr->offset);

    cur.node = ptr->node;
    cur.offset = ptr->offset;
    while (len > 0) {
	u_long this_len;
	char *chunk = (char*) cur.node->data;

	/* Check for overflow */
	if (!cur.node) {
	    return SR_ERROR_BUFFER_TOO_SMALL;
	}

	/* Compute length to peek from this chunk */
	if (cur.offset + len > cbuf3->chunk_size) {
	    this_len = cbuf3->chunk_size - cur.offset;
	} else {
	    this_len = len;
	}

	debug_printf ("Copying %d bytes (%d).\n", this_len, len);

	/* Peek */
	memcpy (&buf[bidx], &chunk[cur.offset], this_len);
	len -= this_len;
	bidx += this_len;

	/* Go to next node */
	cur.node = cur.node->next;
	cur.offset = 0;
    }

    return SR_SUCCESS;
}

/* Copy data from the cbuf3 into the caller's buffer.  Note: This 
   updates the caller's cbuf3_ptr. */
error_code 
cbuf3_extract (Cbuf3 *cbuf3,
	       Cbuf3_pointer *cbuf3_ptr,
	       char *buf,
	       u_long req_size,
	       u_long *bytes_read)
{
    char *chunk;
    u_long offset, chunk_remaining;

    if (!cbuf3_ptr->node) {
	return SR_ERROR_BUFFER_EMPTY;
    }

    debug_printf ("cbuf3_extract is waiting for cbuf3->sem\n");
    threadlib_waitfor_sem (&cbuf3->sem);
    debug_printf ("cbuf3_extract got cbuf3->sem\n");

    chunk = (char*) cbuf3_ptr->node->data;
    offset = cbuf3_ptr->offset;
    chunk_remaining = cbuf3->chunk_size - offset;
    if (req_size < chunk_remaining) {
	(*bytes_read) = req_size;
	cbuf3_ptr->offset += req_size;
    } else {
	(*bytes_read) = chunk_remaining;
	cbuf3_ptr->node = cbuf3_ptr->node->next;
	cbuf3_ptr->offset = 0;
    }

    memcpy (buf, &chunk[offset], (*bytes_read));

    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("cbuf3_extract released cbuf3->sem\n");
    return SR_SUCCESS;
}

/******************************************************************************
 * Private functions
 *****************************************************************************/
static void
cbuf3_disconnect_slow_clients (RIP_MANAGER_INFO *rmi, Cbuf3 *cbuf3)
{

    GList *rlist_node = rmi->relay_list->head;
    GList *cbuf3_head = cbuf3->buf->head;

    while (rlist_node) {
	Relay_client *relay_client = (Relay_client *) rlist_node->data;
	GList *next = rlist_node->next;

	if (relay_client->m_cbuf_ptr.node == cbuf3_head) {
	    debug_printf ("Relay: Client %d couldn't keep up with cbuf\n", 
			  relay_client->m_sock);
	    relaylib_disconnect (rmi, rlist_node);
	}
	rlist_node = next;
    }
}
