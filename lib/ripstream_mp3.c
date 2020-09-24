/* ripstream_mp3.c
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
#include "ripstream_mp3.h"
#include "ripstream_wav.h"
#include "debug.h"
#include "filelib.h"
#include "relaylib.h"
#include "socklib.h"
#include "external.h"
#include "track_info.h"
#include "callback.h"


/*****************************************************************************
 * Private functions
 *****************************************************************************/
static error_code
find_sep (RIP_MANAGER_INFO* rmi, 
	  Cbuf3_pointer *end_of_previous, 
	  Cbuf3_pointer *start_of_next);
static void
compute_cbuf2_size (RIP_MANAGER_INFO* rmi, 
		    SPLITPOINT_OPTIONS *sp_opt, 
		    int bitrate, 
		    int meta_interval);
static int ms_to_bytes (int ms, int bitrate);
static int bytes_to_secs (unsigned int bytes, int bitrate);
static error_code
write_id3v2_frame (RIP_MANAGER_INFO *rmi, 
		   Writer *writer, char* tag_name, mchar* data,
		   int charset, int *sent);
static error_code
ripstream_mp3_check_for_track_change (RIP_MANAGER_INFO* rmi);
static error_code
ripstream_mp3_end_track (RIP_MANAGER_INFO* rmi, 
			 Writer* writer);
static error_code
ripstream_mp3_check_bitrate (RIP_MANAGER_INFO* rmi);
static error_code
ripstream_mp3_write_oldest_node (RIP_MANAGER_INFO* rmi);


/*****************************************************************************
 * Private structs
 *****************************************************************************/
typedef struct ID3V1st
{
        char    tag[3];
        char    songtitle[30];
        char    artist[30];
        char    album[30];
        char    year[4];
        char    comment[30];
        char    genre;
} ID3V1Tag;

typedef struct ID3V2framest {
	char	id[4];
	int	size;
	char	pad[3];
} ID3V2frame;

#define HEADER_SIZE 1600


/******************************************************************************
 * Public functions
 *****************************************************************************/
/** Called once per loop for mp3-style streams.
    \callgraph
*/
error_code
ripstream_mp3_rip (RIP_MANAGER_INFO* rmi)
{
    int rc;
    int real_rc = SR_SUCCESS;
    GList *node;
    Cbuf3 *cbuf3 = &rmi->cbuf3;

    debug_printf ("RIPSTREAM_RIP_MP3: top of loop\n");

    if (rmi->ripstream_first_time_through) {
	u_long min_chunks = 24;
	rc = cbuf3_init (&rmi->cbuf3, rmi->http_info.content_type,
			 GET_MAKE_RELAY(rmi->prefs->flags),
			 rmi->getbuffer_size,
			 min_chunks);
    }

    /* Get new data from the stream */
    node = cbuf3_request_free_node (rmi, cbuf3);
    rc = ripstream_get_data (rmi, node->data, rmi->current_track.raw_metadata);
    if (rc != SR_SUCCESS) {
	debug_printf ("get_stream_data bad return code: %d\n", rc);
	return rc;
    }

    /* If first time through, check the bitrate in the stream. */
    if (rmi->ripstream_first_time_through) {
	rc = ripstream_mp3_check_bitrate (rmi);
	if (rc != SR_SUCCESS) return rc;
    }

    /* Get the metadata for the new node */
    if (rmi->ep) {
	/* If getting metadata from external process, check for update */
	track_info_clear (&rmi->current_track);
	read_external (rmi, rmi->ep, &rmi->current_track);
    } else {
	/* Otherwise, apply parse rules to raw metadata */
	if (rmi->current_track.have_track_info) {
	    debug_printf ("parse_metadata\n");
	    parse_metadata (rmi, &rmi->current_track);
	} else {
	    debug_printf ("track_info_clear\n");
	    track_info_clear (&rmi->current_track);
	}
    }

    /* Copy the data into cbuffer */
    rc = cbuf3_insert_node (cbuf3, node);
    if (rc != SR_SUCCESS) {
	debug_printf ("cbuf3_insert had bad return code %d\n", rc);
	return rc;
    }

    /* Insert the metadata into cbuf */
    rc = cbuf3_insert_metadata (cbuf3, &rmi->current_track);
    if (rc != SR_SUCCESS) {
	debug_printf ("cbuf3_insert_metadata had bad return code %d\n", rc);
	return rc;
    }

    /* Write showfile immediately */
    rc = filelib_write_show (rmi, node->data, cbuf3->chunk_size);
    if (rc != SR_SUCCESS) {
        debug_printf("filelib_write_show had bad return code: %d\n", rc);
        return rc;
    }

    /* Set the track number */
    if (rmi->current_track.track_p[0]) {
	mstrcpy (rmi->current_track.track_a, rmi->current_track.track_p);
    } else {
	/* GCS FIX: Writer needs to do this... */
#if defined (commentout)
        msnprintf (rmi->current_track.track_a, MAX_HEADER_LEN,
		   m_("%d"), writer->m_track_no + 1);
#endif
    }

    /* First time through, so start a track. */
    if (rmi->ripstream_first_time_through) {
	int rc;
	Cbuf3_pointer first_byte;
	unsigned int secs;

	/* The first byte is not aligned with mp3 frame, but 
	   we don't worry about this for the first track. */
	first_byte.node = cbuf3->buf->tail;
	first_byte.offset = 0;

	debug_printf ("First time through, starting track.\n");
	if (!rmi->current_track.have_track_info) {
	    strcpy (rmi->current_track.raw_metadata, rmi->no_meta_name);
	}
	msnprintf (rmi->current_track.track_a, MAX_HEADER_LEN, m_("0"));

	/* Queue new writer, and notify callback */
	rc = ripstream_queue_writer (rmi, &rmi->current_track, first_byte);
	if (rc != SR_SUCCESS) {
	    debug_printf ("ripstream_mp3_start_track failed(#1): %d\n",rc);
	    return rc;
	}

	/* Add artist/title to cue sheet */
	secs = bytes_to_secs (rmi->cue_sheet_bytes, rmi->bitrate);
	rc = filelib_write_cue (rmi, &rmi->current_track, secs);
	if (rc != SR_SUCCESS) {
	    debug_printf ("filelib_write_cue failed %d\n", rc);
	    return rc;
	}

	rmi->ripstream_first_time_through = 0;
	track_info_copy (&rmi->old_track, &rmi->current_track);
    }

    /* Check for track change. */
    ripstream_mp3_check_for_track_change (rmi);

    /* If buffer is full, write oldest node to disk */
    rc = ripstream_mp3_write_oldest_node (rmi);
    if (rc != SR_SUCCESS) {
	debug_printf ("ripstream_mp3_write_oldest_node returned: %d\n", rc);
	return rc;
    }

    /* Give some feedback to the user about how much we ripped so far */
    callback_put_data (rmi, rmi->getbuffer_size);

#if defined (commentout)
    /* GCS FIX: Still need to fix cue sheet bytes */
    rmi->cue_sheet_bytes += curr_song_bytes;
#endif

    return real_rc;
}

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static error_code
ripstream_mp3_start_track (RIP_MANAGER_INFO* rmi, Writer *writer)
{
    TRACK_INFO *ti = &writer->m_ti;
    error_code rc;

    debug_printf ("ripstream_mp3_start_track (starting)\n");

    /* Open output file */
    rc = filelib_start (rmi, writer, ti);
    if (rc != SR_SUCCESS) {
        debug_printf ("filelib_start failed %d\n", rc);
        return rc;
    }

    /* Write ID3V2 */
    if (GET_ADD_ID3V2(rmi->prefs->flags)) {
	int i;
	char bigbuf[HEADER_SIZE] = "";
	int header_size = HEADER_SIZE;
	char header1[6] = "ID3\x03\0\0";
	int sent = 0;
	int id3_charset;

	memset(bigbuf, '\000', sizeof(bigbuf));

	/* Write header */
	rc = filelib_write_track (writer, header1, 6);
	if (rc != SR_SUCCESS) return rc;
	for (i = 0; i < 4; i++) {
	    char x = (header_size >> (3-i)*7) & 0x7F;
	    rc = filelib_write_track (writer, (char *)&x, 1);
	    if (rc != SR_SUCCESS) return rc;
	}

	/* ID3 V2.3 is only defined for ISO-8859-1 and UCS-2
	   If user specifies another codeset, we will use it, and 
	   report ISO-8859-1 in the encoding field */
	id3_charset = is_id3_unicode(rmi);

	/* Lead performer */
        rc = write_id3v2_frame (rmi, writer, "TPE1", 
				ti->artist, id3_charset, &sent);

        /* Title */
        rc = write_id3v2_frame (rmi, writer, "TIT2", 
				ti->title, id3_charset, &sent);

        /* Encoded by */
        rc = write_id3v2_frame (rmi, writer, "TENC",
				m_("Ripped with Streamripper"), 
				id3_charset, &sent);
	
        /* Album */
        rc = write_id3v2_frame (rmi, writer, "TALB", 
				ti->album, id3_charset, &sent);

        /* Track */
        rc = write_id3v2_frame (rmi, writer, "TRCK", 
				ti->track_a, id3_charset, &sent);

        /* Year */
        rc = write_id3v2_frame (rmi, writer, "TYER", 
				ti->year, id3_charset, &sent);

	/* Zero out padding */
	memset (bigbuf, '\000', sizeof(bigbuf));

	/* Pad up to header_size */
	rc = filelib_write_track (writer, bigbuf, HEADER_SIZE-sent);
	if (rc != SR_SUCCESS) {
	    return rc;
	}
    }

    return SR_SUCCESS;
}

static error_code
ripstream_mp3_write_oldest_node (RIP_MANAGER_INFO* rmi)
{
    int i;
    Cbuf3 *cbuf3 = &rmi->cbuf3;
    GQueue *write_list = cbuf3->write_list;
    GList *node;
    GList *p, *nextp;

    /* Only write oldest node if buffer is full */
    if (! cbuf3_is_full (cbuf3)) {
	return SR_SUCCESS;
    }

    /* Remove oldest node from used queue */
    node = cbuf3_extract_oldest_node (rmi, cbuf3);

    /* Put it on the free list */
    cbuf3_insert_free_node (cbuf3, node);

    debug_printf ("ripstream_mp3_write_oldest_node: %d, %d\n",
	GET_INDIVIDUAL_TRACKS (rmi->prefs->flags), rmi->write_data);

    /* If we're not writing tracks, return */
    /* GCS FIX - this logic is obsolete - writer shouldn't enter 
       write_list if not needed. */
    if (!GET_INDIVIDUAL_TRACKS (rmi->prefs->flags) || !rmi->write_data) {
	return SR_SUCCESS;
    }

    /* Loop through tracks that might need to be written */
    debug_printf ("Looping through write_list\n");
    i = 0;
    p = write_list->head;
    while (p) {
	Writer *writer = (Writer*) p->data;
	nextp = p->next;

	debug_printf ("Writer %02d: (%p), (%p,%p) %s\n", 
	    i++, 
	    node,
	    writer->m_next_byte.node, 
	    writer->m_last_byte.node,
	    writer->m_ti.raw_metadata);

	/* Check if the writer needs to write this node */
	if (writer->m_next_byte.node == node) {
	    char* write_ptr;
	    long write_sz;

	    debug_printf ("Writer requesed this node\n");

	    /* Open the file and write header */
	    if (!writer->m_started) {
		ripstream_mp3_start_track (rmi, writer);
		writer->m_started = 1;
	    }

	    /* Write the data to the file. */
	    if (writer->m_last_byte.node == node) {
		write_sz = writer->m_last_byte.offset 
		    - writer->m_next_byte.offset;
	    } else {
		write_sz = cbuf3->chunk_size 
		    - writer->m_next_byte.offset;
	    }
	    write_ptr = ((char*) node->data) + writer->m_next_byte.offset;
	    if (rmi->prefs->wav_output) {
                char* write_ptr_wav;
                long write_sz_wav;
		/* Use MAD to decode mp3 into wav */
		mp3_to_wav (&write_ptr_wav, &write_sz_wav, write_ptr, write_sz);
		filelib_write_track (writer, write_ptr_wav, write_sz_wav);
		free(write_ptr_wav);
            } else {
	        filelib_write_track (writer, write_ptr, write_sz);
            }

	    /* Check if we need to end the track */
	    if (writer->m_last_byte.node == node) {
		/* Yes we do, so add id3, close file, etc. */
		debug_printf ("Ending track\n");
		ripstream_mp3_end_track (rmi, writer);

		/* Free up writer */
		free (writer);
		write_list->head = g_list_delete_link (write_list->head, p);

	    } else {
		/* Not end of track, so advance writer pointer */
		debug_printf ("Advancing writer next node: %p -> %p\n",
		    writer->m_next_byte.node, cbuf3->buf->head);
		writer->m_next_byte.node = cbuf3->buf->head;
		writer->m_next_byte.offset = 0;
	    }
	}

	p = nextp;
    }
    return SR_SUCCESS;
}

static error_code
ripstream_mp3_check_for_track_change (RIP_MANAGER_INFO* rmi)
{
    error_code rc;
    GQueue *write_list = rmi->cbuf3.write_list;

    debug_printf ("rmi->current_track.have_track_info = %d\n", 
		  rmi->current_track.have_track_info);
    if (rmi->current_track.have_track_info 
	&& track_info_different (&rmi->old_track, &rmi->current_track)) {
	/* Set m_find_silence equal to the number of additional blocks 
	   needed until we can do silence separation. */
	debug_printf ("VERIFIED TRACK CHANGE (find_silence counter = %d)\n",
	    rmi->find_silence);
	track_info_copy (&rmi->new_track, &rmi->current_track);
	if (rmi->find_silence < 0) {
	    if (rmi->mic_to_cb_end > 0) {
		rmi->find_silence = rmi->mic_to_cb_end;
	    } else {
		rmi->find_silence = 0;
	    }
	}
    }

    track_info_debug (&rmi->old_track, "old");
    track_info_debug (&rmi->new_track, "new");
    track_info_debug (&rmi->current_track, "current");

    if (rmi->find_silence == 0) {
	Writer *prev_writer;
	Cbuf3_pointer end_of_previous, start_of_next;
	unsigned int secs;

	/* Find separation point */
	debug_printf ("m_find_silence == 0\n");
	rc = find_sep (rmi, &end_of_previous, &start_of_next);
	if (rc != SR_SUCCESS) {
	    debug_printf ("find_sep had bad return code: %s\n", 
		errors_get_string (rc));
	    return rc;
	}

	/* Add end point to prev writer in list */
	prev_writer = (Writer*) write_list->tail->data;
	prev_writer->m_last_byte = end_of_previous;
	prev_writer->m_ended = 1;

	/* Create file, queue new writer, and notify callback */
	rc = ripstream_queue_writer (rmi, &rmi->new_track, start_of_next);
	if (rc != SR_SUCCESS) {
	    debug_printf ("ripstream_mp3_start_track had bad "
			  "return code %d\n", rc);
	    return rc;
	}

	/* Add artist/title to cue sheet */
	secs = bytes_to_secs (rmi->cue_sheet_bytes, rmi->bitrate);
	rc = filelib_write_cue (rmi, &rmi->new_track, secs);
	if (rc != SR_SUCCESS) {
	    debug_printf ("filelib_write_cue failed %d\n", rc);
	    return rc;
	}

#if defined (commentout)
	/* GCS kkk: This is needed to set cue sheet info (until we 
	   decode everything) */
	rmi->cue_sheet_bytes += pos2;
#endif

	rmi->find_silence = -1;

	track_info_copy (&rmi->old_track, &rmi->new_track);
    }
    if (rmi->find_silence >= 0) rmi->find_silence --;

    return SR_SUCCESS;
}

static error_code
find_sep (RIP_MANAGER_INFO* rmi, 
    Cbuf3_pointer *end_of_previous, 
    Cbuf3_pointer *start_of_next)
{
    SPLITPOINT_OPTIONS* sp_opt = &rmi->prefs->sp_opt;
    Cbuf3 *cbuf3 = &rmi->cbuf3;
    Cbuf3_pointer rw_start, rw_end;
    Cbuf3_pointer cbuf_end;
    error_code rc;
    u_long rw_size;

    debug_printf ("*** Finding separation point\n");

    /* First, find the search region w/in cbuffer. */
    cbuf3_get_tail (cbuf3, &cbuf_end);
    rc = cbuf3_pointer_add (cbuf3, &rw_start, &cbuf_end, 
	- rmi->rw_start_to_cb_end);
    if (rc == SR_ERROR_BUFFER_TOO_SMALL) {
	debug_printf ("SR_ERROR_BUFFER_TOO_SMALL 1\n");
	debug_printf ("(%p,%d) + (%d)\n", rw_start.node, rw_start.offset,
	    - rmi->rw_start_to_cb_end);
	cbuf3_get_head (cbuf3, &rw_start);
    }
    rc = cbuf3_pointer_add (cbuf3, &rw_end, &cbuf_end, 
	- rmi->rw_end_to_cb_end);
    if (rc == SR_ERROR_BUFFER_TOO_SMALL) {
	debug_printf ("SR_ERROR_BUFFER_TOO_SMALL 2\n");
	debug_printf ("(%p,%d) + (%d)\n", rw_end.node, rw_end.offset,
	    - rmi->rw_end_to_cb_end);
	cbuf3_get_tail (cbuf3, &rw_end);
    }

    rc = cbuf3_pointer_subtract (cbuf3, &rw_size, &rw_start, &rw_end);
    if (rc == SR_ERROR_BUFFER_TOO_SMALL) {
	debug_printf ("SR_ERROR_BUFFER_TOO_SMALL 3\n");
	return rc;
    }

    debug_printf (
	"search window : [%p,%d] to [%p,%d] (%lu bytes)\n",
	rw_start.node,
	rw_start.offset,
	rw_end.node,
	rw_end.offset,
	rw_size
    );

    if (rmi->http_info.content_type != CONTENT_TYPE_MP3) {
	long midpoint = rw_size / 2;
	debug_printf ("(not mp3) taking middle: sw_sil=%d\n", midpoint);
	cbuf3_pointer_add (cbuf3, end_of_previous, &rw_start, midpoint - 1);
	cbuf3_pointer_add (cbuf3, start_of_next, &rw_start, midpoint);
    } else {
	u_long bufsize = rw_size;
	char* buf = (char*) malloc (bufsize);
	u_long pos1, pos2;

	rc = cbuf3_peek (&rmi->cbuf3, buf, &rw_start, bufsize);
	if (rc != SR_SUCCESS) {
	    debug_printf ("PEEK FAILED: %d\n", rc);
	    free(buf);
	    return rc;
	}
	debug_printf ("PEEK OK\n");

	/* Find silence point */
	if (sp_opt->xs == 2) {
	    rc = findsep_silence_2 (buf, 
		bufsize, 
		rmi->rw_start_to_sw_start,
		sp_opt->xs_search_window_1 
		+ sp_opt->xs_search_window_2,
		sp_opt->xs_silence_length,
		sp_opt->xs_padding_1,
		sp_opt->xs_padding_2,
		&pos1, &pos2);
	} else {
	    rc = findsep_silence (buf, 
		bufsize, 
		rmi->rw_start_to_sw_start,
		sp_opt->xs_search_window_1 
		+ sp_opt->xs_search_window_2,
		sp_opt->xs_silence_length,
		sp_opt->xs_padding_1,
		sp_opt->xs_padding_2,
		&pos1, &pos2);
	}

	cbuf3_pointer_add (cbuf3, end_of_previous, &rw_start, pos1);
	cbuf3_pointer_add (cbuf3, start_of_next, &rw_start, pos2);
	free(buf);
    }

    return SR_SUCCESS;
}

static error_code
ripstream_mp3_end_track (RIP_MANAGER_INFO* rmi, 
    Writer* writer)
{
    error_code rc;

    /* Add id3v1 if requested */
    if (GET_ADD_ID3V1(rmi->prefs->flags)) {
	ID3V1Tag id3v1;
	memset (&id3v1, '\000',sizeof(id3v1));
	strncpy (id3v1.tag, "TAG", strlen("TAG"));
	string_from_gstring (rmi, id3v1.artist, sizeof(id3v1.artist),
	    writer->m_ti.artist, CODESET_ID3);
	string_from_gstring (rmi, id3v1.songtitle, sizeof(id3v1.songtitle),
	    writer->m_ti.title, CODESET_ID3);
	string_from_gstring (rmi, id3v1.album, sizeof(id3v1.album),
	    writer->m_ti.album, CODESET_ID3);
	string_from_gstring (rmi, id3v1.year, sizeof(id3v1.year),
	    writer->m_ti.year, CODESET_ID3);
	id3v1.genre = (char) 0xFF; // see http://www.id3.org/id3v2.3.0.html#secA
	rc = filelib_write_track (writer, (char *)&id3v1, sizeof(id3v1));
	if (rc != SR_SUCCESS) {
	    return rc;
	}
    }

    /* Close file, rename file, post status, destroy writer */
    rc = ripstream_end_track (rmi, writer);
    if (rc != SR_SUCCESS) {
	return rc;
    }

    return SR_SUCCESS;
}

// Write data to a new frame, using tag_name e.g. TPE1, and increment
// *sent with the number of bytes written
static error_code
write_id3v2_frame (RIP_MANAGER_INFO *rmi, 
		   Writer *writer, char* tag_name, mchar* data,
		   int charset, int *sent)
{
    int ret;
    int rc;
    char bigbuf[HEADER_SIZE] = "";
    ID3V2frame id3v2frame;
#ifndef WIN32
    __uint32_t framesize = 0;
#else
    unsigned long int framesize = 0;
#endif

    memset(&id3v2frame, '\000', sizeof(id3v2frame));
    strncpy(id3v2frame.id, tag_name, 4);
    id3v2frame.pad[2] = charset;
    rc = string_from_gstring (rmi, bigbuf, HEADER_SIZE, data, CODESET_ID3);
    framesize = htonl (rc+1);
    ret = filelib_write_track (writer, (char *)&(id3v2frame.id), 4);
    if (ret != SR_SUCCESS) return ret;
    *sent += 4;
    ret = filelib_write_track (writer, (char *)&(framesize),
			      sizeof(framesize));
    if (ret != SR_SUCCESS) return ret;
    *sent += sizeof(framesize);
    ret = filelib_write_track (writer, (char *)&(id3v2frame.pad), 3);
    if (ret != SR_SUCCESS) return ret;
    *sent += 3;
    ret = filelib_write_track (writer, bigbuf, rc);
    if (ret != SR_SUCCESS) return ret;
    *sent += rc;

    return SR_SUCCESS;
}

/* First time through, need to determine the bitrate. 
   The bitrate is needed to do the track splitting parameters 
   properly in seconds.  See the readme file for details.  */
/* GCS FIX: For VBR streams, the detected_bitrate is unreliable */
static error_code
ripstream_mp3_check_bitrate (RIP_MANAGER_INFO* rmi)
{
    error_code rc;
    unsigned long test_bitrate;

    debug_printf("Querying stream for bitrate - first time.\n");

    if (rmi->http_info.content_type == CONTENT_TYPE_MP3) {
	find_bitrate(&test_bitrate, rmi->getbuffer, rmi->getbuffer_size);
	rmi->detected_bitrate = test_bitrate / 1000;
	debug_printf("Detected bitrate: %d\n",rmi->detected_bitrate);
    } else {
	rmi->detected_bitrate = 0;
    }

    if (rmi->detected_bitrate == 0) {
	if (rmi->http_bitrate > 0)
	    /* Didn't decode bitrate from mp3.
	       Did get bitrate from http header. */
	    rmi->bitrate = rmi->http_bitrate;
	else
	    /* Didn't decode bitrate from mp3.
	       Didn't get bitrate from http header. */
	    rmi->bitrate = 24;
    } else {
	if (rmi->http_bitrate > 0) {
	    /* Did decode bitrate from mp3.
	       Did get bitrate from http header. */
	    if (rmi->detected_bitrate > rmi->http_bitrate) {
		rmi->bitrate = rmi->detected_bitrate;
	    } else {
		rmi->bitrate = rmi->http_bitrate;
	    }
	} else {
	    /* Did decode bitrate from mp3.
	       Didn't get bitrate from http header. */
	    rmi->bitrate = rmi->detected_bitrate;
	}
    }

    compute_cbuf2_size (rmi, &rmi->prefs->sp_opt, 
			rmi->bitrate, rmi->getbuffer_size);
    rc = cbuf3_allocate_minimum (&rmi->cbuf3, 
				 rmi->cbuf2_size);
    return rc;
}

#if defined (commentout)
/* GCS: This converts either positive or negative ms to blocks,
   and must work for rounding up and rounding down */
static int
ms_to_blocks (int ms, int bitrate, int round_up)
{
    int ms_abs = ms > 0 ? ms : -ms;
    int ms_sign = ms > 0 ? 1 : 0;
    int bits = ms_abs * bitrate;
    int bits_per_block = 8 * rmi->getbuffer_size;
    int blocks = bits / bits_per_block;
    if (bits % bits_per_block > 0) {
	if (!(round_up ^ ms_sign)) {
	    blocks++;
	}
    }
    if (!ms_sign) {
	blocks = -blocks;
    }
    return blocks;
}
#endif

/* Simpler routine, rounded toward zero */
static int
ms_to_bytes (int ms, int bitrate)
{
    int bits = ms * bitrate;
    if (bits > 0)
	return bits / 8;
    else
	return -((-bits)/8);
}

/* Assume positive, round toward zero */
static int
bytes_to_secs (unsigned int bytes, int bitrate)
{
    /* divided by 125 because 125 = 1000 / 8 */
    int secs = (bytes / bitrate) / 125;
    return secs;
}

/* --------------------------------------------------------------------------
   Buffering for silence splitting & padding

   We may flush the circular buffer as soon as anything that 
   needs to go into the next song has passed.   For simplicity, 
   we also buffer up to the latest point that can go into the 
   next song.  This is called the "required window."

   The "required window" is part that is decoded, even though 
   we don't need volume data for all of it.  We simply mark the 
   frame boundaries so we don't chop any frames.

   The circular buffer is a bit bigger than the required window. 
   This includes all of the stuff which cannot be flushed out of 
   the cbuf, because cbuf is flushed in blocks.

   Some abbreviations:
     mic      meta inf change
     cb	      cbuf3, aka circular buffer
     rw	      required window
     sw	      search window

   This is the complete picture:

    A---------A---------A----+----B---------B     meta intervals

                                  /mic            meta-inf change (A to B)
                             +--->|
                             /mi                  meta-inf point
                             |
                   |---+-----+------+---|         search window
                       |            |   
                       |            |   
                   |---+---|    |---+---|         silence length
                       |            |
         |-------------|            |-----|
              prepad                postpad

         |--------------------------------|       required window

                        |---------|               minimum rw (1 meta int, 
                                                  includes sw, need not 
                                                  be aligned to metadata)

    |---------------------------------------|     cbuf

                   |<-------------+               mic_to_sw_start (usu neg)
                                  +---->|         mic_to_sw_end   (usu pos)
         |<-----------------------+               mic_to_rw_start
                                  +------>|       mic_to_rw_end
    |<----------------------------+               mic_to_cb_start
                                  +-------->|     mic_to_cb_end

   ------------------------------------------------------------------------*/
static void
compute_cbuf2_size (RIP_MANAGER_INFO* rmi, 
		    SPLITPOINT_OPTIONS *sp_opt, 
		    int bitrate, 
		    int meta_interval)
{
    long sws, sl;
    long mi_to_mic;
    long prepad, postpad;
    long offset;
    long rw_len;
    long mic_to_sw_start, mic_to_sw_end;
    long mic_to_rw_start, mic_to_rw_end;
    long mic_to_cb_start, mic_to_cb_end;
    int xs_silence_length;
    int xs_search_window_1;
    int xs_search_window_2;
    int xs_offset;
    int xs_padding_1;
    int xs_padding_2;

    debug_printf ("---------------------------------------------------\n");
    debug_printf ("xs: %d\n", sp_opt->xs);
    debug_printf ("xs_search_window: %d,%d\n",
		  sp_opt->xs_search_window_1,sp_opt->xs_search_window_2);
    debug_printf ("xs_silence_length: %d\n", sp_opt->xs_silence_length);
    debug_printf ("xs_padding: %d,%d\n", sp_opt->xs_padding_1,
		  sp_opt->xs_padding_2);
    debug_printf ("xs_offset: %d\n", sp_opt->xs_offset);
    debug_printf ("---------------------------------------------------\n");
    debug_printf ("bitrate = %d, meta_inf = %d\n", bitrate, meta_interval);
    debug_printf ("---------------------------------------------------\n");
    
    /* If xs-none, clear out the other xs options */
    if (sp_opt->xs == 0){
	xs_silence_length = 0;
	xs_search_window_1 = 0;
	xs_search_window_2 = 0;
	xs_offset = 0;
	xs_padding_1 = 0;
	xs_padding_2 = 0;
    } else {
	xs_silence_length = sp_opt->xs_silence_length;
	xs_search_window_1 = sp_opt->xs_search_window_1;
	xs_search_window_2 = sp_opt->xs_search_window_2;
	xs_offset = sp_opt->xs_offset;
	xs_padding_1 = sp_opt->xs_padding_1;
	xs_padding_2 = sp_opt->xs_padding_2;
    }


    /* mi_to_mic is the "half of a meta-inf" from the meta inf 
       change to the previous (non-changed) meta inf */
    mi_to_mic = meta_interval / 2;
    debug_printf ("mi_to_mic: %d\n", mi_to_mic);

    /* compute the search window size (sws) */
    sws = ms_to_bytes (xs_search_window_1, bitrate) 
	    + ms_to_bytes (xs_search_window_2, bitrate);
    debug_printf ("sws: %d\n", sws);

    /* compute the silence length (sl) */
    sl = ms_to_bytes (xs_silence_length, bitrate);
    debug_printf ("sl: %d\n", sl);

    /* compute padding */
    prepad = ms_to_bytes (xs_padding_1, bitrate);
    postpad = ms_to_bytes (xs_padding_2, bitrate);
    debug_printf ("padding: %d %d\n", prepad, postpad);

    /* compute offset */
    offset = ms_to_bytes (xs_offset,bitrate);
    debug_printf ("offset: %d\n", offset);

    /* compute interval from mi to search window */
    mic_to_sw_start = - mi_to_mic + offset 
	    - ms_to_bytes(xs_search_window_1,bitrate);
    mic_to_sw_end = - mi_to_mic + offset 
	    + ms_to_bytes(xs_search_window_2,bitrate);
    debug_printf ("mic_to_sw_start: %d\n", mic_to_sw_start);
    debug_printf ("mic_to_sw_end: %d\n", mic_to_sw_end);

    /* compute interval from mi to required window */
    mic_to_rw_start = mic_to_sw_start + sl / 2 - prepad;
    if (mic_to_rw_start > mic_to_sw_start) {
	mic_to_rw_start = mic_to_sw_start;
    }
    mic_to_rw_end = mic_to_sw_end - sl / 2 + postpad;
    if (mic_to_rw_end < mic_to_sw_end) {
	mic_to_rw_end = mic_to_sw_end;
    }
    debug_printf ("mic_to_rw_start: %d\n", mic_to_rw_start);
    debug_printf ("mic_to_rw_end: %d\n", mic_to_rw_end);

    /* if rw is not long enough, make it longer */
    rw_len = mic_to_rw_end - mic_to_rw_start;
    if (rw_len < meta_interval) {
	long start_extra = (meta_interval - rw_len) / 2;
	long end_extra = meta_interval - start_extra;
	mic_to_rw_start -= start_extra;
	mic_to_rw_end += end_extra;
	debug_printf ("mic_to_rw_start (2): %d\n", mic_to_rw_start);
	debug_printf ("mic_to_rw_end (2): %d\n", mic_to_rw_end);
    }

    /* This code replaces the 3 cases (see OBSOLETE in gcs_notes.txt) */
    mic_to_cb_start = mic_to_rw_start;
    mic_to_cb_end = mic_to_rw_end;
    if (mic_to_cb_start > -meta_interval) {
	mic_to_cb_start = -meta_interval;
    }
    if (mic_to_cb_end < 0) {
	mic_to_cb_end = 0;
    }

    /* Convert to chunks & compute cbuf size */
    mic_to_cb_end = (mic_to_cb_end + (meta_interval-1)) / meta_interval;
    mic_to_cb_start = -((-mic_to_cb_start + (meta_interval-1)) 
			/ meta_interval);
    rmi->cbuf2_size = - mic_to_cb_start + mic_to_cb_end;
    if (rmi->cbuf2_size < 3) {
	rmi->cbuf2_size = 3;
    }
    debug_printf ("mic_to_cb_start: %d\n", mic_to_cb_start * meta_interval);
    debug_printf ("mic_to_cb_end: %d\n", mic_to_cb_end * meta_interval);
    debug_printf ("CBUF2 (BLOCKS): %d:%d -> %d\n", mic_to_cb_start,
		  mic_to_cb_end, rmi->cbuf2_size);

    /* Set some global variables to be used by splitting algorithm */
    rmi->mic_to_cb_end = mic_to_cb_end;
    rmi->rw_start_to_cb_end = mic_to_cb_end * meta_interval - mic_to_rw_start;
    rmi->rw_end_to_cb_end = mic_to_cb_end * meta_interval - mic_to_rw_end;
    rmi->rw_start_to_sw_start = xs_padding_1 - xs_silence_length / 2;
    if (rmi->rw_start_to_sw_start < 0) {
	rmi->rw_start_to_sw_start = 0;
    }
    debug_printf ("m_mic_to_cb_end: %d\n", rmi->mic_to_cb_end);
    debug_printf ("m_rw_start_to_cb_end: %d\n", rmi->rw_start_to_cb_end);
    debug_printf ("m_rw_end_to_cb_end: %d\n", rmi->rw_end_to_cb_end);
    debug_printf ("m_rw_start_to_sw_start: %d\n", rmi->rw_start_to_sw_start);
}
