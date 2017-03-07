/* ripstream.c
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
#include "cbuf2.h"
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

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static error_code
find_sep (RIP_MANAGER_INFO* rmi, u_long* pos1, u_long* pos2);
static error_code start_track_mp3 (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti);
static error_code end_track_mp3 (RIP_MANAGER_INFO* rmi, u_long pos1, 
				 u_long pos2, TRACK_INFO* ti);
static error_code end_track_ogg (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti);
static void
compute_cbuf2_size (RIP_MANAGER_INFO* rmi, 
		    SPLITPOINT_OPTIONS *sp_opt, 
		    int bitrate, 
		    int meta_interval);
static int ms_to_bytes (int ms, int bitrate);
static int bytes_to_secs (unsigned int bytes, int bitrate);
static void clear_track_info (TRACK_INFO* ti);
static int
ripstream_recvall (RIP_MANAGER_INFO* rmi, char* buffer, int size);
static error_code get_track_from_metadata (RIP_MANAGER_INFO* rmi, 
					   int size, char *newtrack);
static error_code
get_stream_data (RIP_MANAGER_INFO* rmi, char *data_buf, char *track_buf);
static error_code ripstream_rip_mp3 (RIP_MANAGER_INFO* rmi);
static error_code ripstream_rip_ogg (RIP_MANAGER_INFO* rmi);

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


/******************************************************************************
 * Public functions
 *****************************************************************************/
error_code
ripstream_init (RIP_MANAGER_INFO* rmi)
{
    rmi->track_count = 0;
    /* GCS RMK: Ripchunk_size is the metaint size, or default size
       if stream doesn't have meta data */
    rmi->cue_sheet_bytes = 0;
    rmi->getbuffer_size = (rmi->meta_interval == NO_META_INTERVAL) 
	    ? DEFAULT_META_INTERVAL : rmi->meta_interval;

    clear_track_info (&rmi->old_track);
    clear_track_info (&rmi->new_track);
    clear_track_info (&rmi->current_track);
    rmi->ripstream_first_time_through = 1;

    if ((rmi->getbuffer = malloc (rmi->getbuffer_size)) == NULL)
	return SR_ERROR_CANT_ALLOC_MEMORY;

    return SR_SUCCESS;
}

void
ripstream_clear(RIP_MANAGER_INFO* rmi)
{
    debug_printf ("RIPSTREAM_CLEAR\n");

    if (rmi->getbuffer) {free(rmi->getbuffer); rmi->getbuffer = NULL;}
    rmi->getbuffer_size = 0;

    rmi->find_silence = -1;
    rmi->cbuf2_size = 0;
    cbuf2_destroy (&rmi->cbuf2);

    clear_track_info (&rmi->old_track);
    clear_track_info (&rmi->new_track);
    clear_track_info (&rmi->current_track);
    rmi->ripstream_first_time_through = 1;

    rmi->no_meta_name[0] = '\0';
    rmi->track_count = 0;
}

static BOOL
is_track_changed (RIP_MANAGER_INFO* rmi)
{
    TRACK_INFO *old = &rmi->old_track;
    TRACK_INFO *cur = &rmi->current_track;

    /* We test the parsed fields instead of raw_metadata because the 
       parse rules may have stripped garbage out, causing the resulting 
       track info to be the same. */
    if (!strcmp(old->artist, cur->artist)
	&& !strcmp(old->title, cur->title)
	&& !strcmp(old->album, cur->album)
	&& !strcmp(old->track_p, cur->track_p)
	&& !strcmp(old->year, cur->year))
    {
	return 0;
    }

    /* Otherwise, there was a change. */
    return 1;
}

static void
debug_track_info (TRACK_INFO* ti, char* tag)
{
    debug_mprintf (m_("----- TRACK_INFO ") m_s m_("\n")
		   m_("HAVETI: %d\n")
		   m_("RAW_MD: ") m_s m_("\n")
		   m_("ARTIST: ") m_S m_("\n")
		   m_("TITLE:  ") m_S m_("\n")
		   m_("ALBUM:  ") m_S m_("\n")
		   m_("TRACK_P:") m_S m_("\n")
		   m_("TRACK_A:") m_S m_("\n")
		   m_("YEAR:  ") m_S m_("\n")
		   m_("SAVE:   %d\n"),
		   tag,
		   ti->have_track_info,
		   ti->raw_metadata,
		   ti->artist,
		   ti->title,
		   ti->album,
                   ti->track_p,
                   ti->track_a,
                   ti->year,
		   ti->save_track);
}

static void
clear_track_info (TRACK_INFO* ti)
{
    ti->have_track_info = 0;
    ti->raw_metadata[0] = 0;
    ti->artist[0] = 0;
    ti->title[0] = 0;
    ti->album[0] = 0;
    ti->track_p[0] = 0;
    ti->track_a[0] = 0;
    ti->year[0] = 0;
    ti->composed_metadata[0] = 0;
    ti->save_track = TRUE;
}

static void
copy_track_info (TRACK_INFO* dest, TRACK_INFO* src)
{
    dest->have_track_info = src->have_track_info;
    strcpy (dest->raw_metadata, src->raw_metadata);
    mstrcpy (dest->artist, src->artist);
    mstrcpy (dest->title, src->title);
    mstrcpy (dest->album, src->album);
    mstrcpy (dest->track_p, src->track_p);
    mstrcpy (dest->track_a, src->track_a);
    mstrcpy (dest->year, src->year);
    strcpy (dest->composed_metadata, src->composed_metadata);
    dest->save_track = src->save_track;
}

/**** The main loop for ripping ****/
error_code
ripstream_rip (RIP_MANAGER_INFO* rmi)
{
    if (rmi->http_info.content_type == CONTENT_TYPE_OGG) {
	return ripstream_rip_ogg (rmi);
    } else {
	return ripstream_rip_mp3 (rmi);
    }
}

/******************************************************************************
 * Private functions
 *****************************************************************************/
static error_code
ripstream_rip_ogg (RIP_MANAGER_INFO* rmi)
{
    int ret;
    int real_ret = SR_SUCCESS;
    u_long extract_size;

    /* get the data from the stream */
    debug_printf ("RIPSTREAM_RIP_OGG: top of loop\n");
    ret = get_stream_data (rmi, rmi->getbuffer, rmi->current_track.raw_metadata);
    if (ret != SR_SUCCESS) {
	debug_printf("get_stream_data bad return code: %d\n", ret);
	return ret;
    }

    if (rmi->ripstream_first_time_through) {
	/* Allocate circular buffer */
	rmi->detected_bitrate = -1;
	rmi->bitrate = -1;
	rmi->getbuffer_size = 1024;
	rmi->cbuf2_size = 128;
	ret = cbuf2_init (&rmi->cbuf2, rmi->http_info.content_type, 
			  GET_MAKE_RELAY(rmi->prefs->flags),
			  rmi->getbuffer_size, rmi->cbuf2_size);
	if (ret != SR_SUCCESS) return ret;
	rmi->ogg_track_state = 0;
	/* Warm up the ogg decoding routines */
	rip_ogg_init (rmi);
	/* Done! */
	rmi->ripstream_first_time_through = 0;
    }

    /* Copy the data into cbuffer.  This calls rip_ogg_process_chunk,
       which sets TRACK_INFO. */
    clear_track_info (&rmi->current_track);
    ret = cbuf2_insert_chunk (rmi, &rmi->cbuf2, 
			      rmi->getbuffer, rmi->getbuffer_size,
			      rmi->http_info.content_type, 
			      &rmi->current_track);
    if (ret != SR_SUCCESS) {
	debug_printf("start_track had bad return code %d\n", ret);
	return ret;
    }

    ret = filelib_write_show (rmi, rmi->getbuffer, rmi->getbuffer_size);
    if (ret != SR_SUCCESS) {
        debug_printf("filelib_write_show had bad return code: %d\n", ret);
        return ret;
    }


#if defined (commentout)
    /* If we have unwritten pages for the current track, write them */
    if (rmi->ogg_have_track) {
	do {
	    error_code ret;
	    unsigned long amt_filled;
	    int got_eos;
	    ret = cbuf2_ogg_peek_song (&rmi->cbuf2, rmi->getbuffer, 
				       rmi->getbuffer_size,
				       &amt_filled, &got_eos);
	    debug_printf ("^^^ogg_peek: %d %d\n", amt_filled, got_eos);
	    if (ret != SR_SUCCESS) {
		debug_printf ("cbuf2_ogg_peek_song: %d\n", ret);
		return ret;
	    }
	    if (amt_filled == 0) {
		/* No more pages */
		break;
	    }
	    ret = rip_manager_put_data (rmi, rmi->getbuffer, amt_filled);
	    if (ret != SR_SUCCESS) {
		debug_printf ("rip_manager_put_data(#1): %d\n",ret);
		return ret;
	    }
	    if (got_eos) {
		//end_track_ogg (rmi, &rmi->old_track);
		rmi->ogg_have_track = 0;
		//break;
	    }
	} while (1);
    }

    debug_track_info (&rmi->current_track, "current");

    /* If we got a new track, then start a new file */
    if (rmi->current_track.have_track_info) {
	ret = rip_manager_start_track (rmi, &rmi->current_track);
	if (ret != SR_SUCCESS) {
	    debug_printf ("rip_manager_start_track failed(#1): %d\n",ret);
	    return ret;
	}
	filelib_write_cue (rmi, &rmi->current_track, 0);
	copy_track_info (&rmi->old_track, &rmi->current_track);

	rmi->ogg_have_track = 1;
    }
#endif

    debug_printf ("ogg_track_state[a] = %d\n", rmi->ogg_track_state);

    /* If we have unwritten pages for the current track, write them */
    if (rmi->ogg_track_state == 1) {
	do {
	    error_code ret;
	    unsigned long amt_filled;
	    int got_eos;

	    ret = cbuf2_ogg_peek_song (&rmi->cbuf2, rmi->getbuffer, 
				       rmi->getbuffer_size,
				       &amt_filled, &got_eos);
	    debug_printf ("^^^ogg_peek: %d %d\n", amt_filled, got_eos);
	    if (ret != SR_SUCCESS) {
		debug_printf ("cbuf2_ogg_peek_song: %d\n", ret);
		return ret;
	    }
	    if (amt_filled == 0) {
		/* No more pages */
		break;
	    }
	    ret = rip_manager_put_data (rmi, rmi->getbuffer, amt_filled);
	    if (ret != SR_SUCCESS) {
		debug_printf ("rip_manager_put_data(#1): %d\n",ret);
		return ret;
	    }
	    if (got_eos) {
		rmi->ogg_track_state = 2;
		break;
	    }

	} while (1);
    }

    debug_printf ("ogg_track_state[b] = %d\n", rmi->ogg_track_state);
    debug_track_info (&rmi->current_track, "current");

    /* If we got a new track, then (maybe) start a new file */
    if (rmi->current_track.have_track_info) {
	if (is_track_changed (rmi)) {
	    if (rmi->ogg_track_state == 2) {
		end_track_ogg (rmi, &rmi->old_track);
	    }
	    ret = rip_manager_start_track (rmi, &rmi->current_track);
	    if (ret != SR_SUCCESS) {
		debug_printf ("rip_manager_start_track failed(#1): %d\n",ret);
		return ret;
	    }
	    copy_track_info (&rmi->old_track, &rmi->current_track);
	}
	rmi->ogg_track_state = 1;
    }

    /* If buffer almost full, advance the buffer */
    if (cbuf2_get_free(&rmi->cbuf2) < rmi->getbuffer_size) {
	debug_printf ("cbuf2_get_free < getbuffer_size\n");
	extract_size = rmi->getbuffer_size - cbuf2_get_free(&rmi->cbuf2);

        ret = cbuf2_advance_ogg (rmi, &rmi->cbuf2, rmi->getbuffer_size);

        if (ret != SR_SUCCESS) {
	    debug_printf("cbuf2_advance_ogg had bad return code %d\n", ret);
	    return ret;
	}
    }

    return real_ret;
}

static error_code
ripstream_rip_mp3 (RIP_MANAGER_INFO* rmi)
{
    int ret;
    int real_ret = SR_SUCCESS;
    u_long extract_size;

    /* get the data & meta-data from the stream */
    debug_printf ("RIPSTREAM_RIP_MP3: top of loop\n");
    ret = get_stream_data(rmi, rmi->getbuffer, rmi->current_track.raw_metadata);
    if (ret != SR_SUCCESS) {
	debug_printf("get_stream_data bad return code: %d\n", ret);
	return ret;
    }

    /* First time through, need to determine the bitrate. 
       The bitrate is needed to do the track splitting parameters 
       properly in seconds.  See the readme file for details.  */
    /* GCS FIX: For VBR streams, the header value may be more reliable. */
    if (rmi->ripstream_first_time_through) {
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
	    /* Couldn't decode from mp3.  If http header reported a bitrate,
	       we'll use that.  Otherwise we use 24. */
	    if (rmi->http_bitrate > 0)
		rmi->bitrate = rmi->http_bitrate;
	    else
		rmi->bitrate = 24;
	} else {
	    rmi->bitrate = rmi->detected_bitrate;
	}

        compute_cbuf2_size (rmi, &rmi->prefs->sp_opt, 
			    rmi->bitrate, rmi->getbuffer_size);
	ret = cbuf2_init (&rmi->cbuf2, rmi->http_info.content_type, 
			  GET_MAKE_RELAY(rmi->prefs->flags),
			  rmi->getbuffer_size, rmi->cbuf2_size);
	if (ret != SR_SUCCESS) return ret;
    }

    if (rmi->ep) {
	/* If getting metadata from external process, check for update */
	clear_track_info (&rmi->current_track);
	read_external (rmi, rmi->ep, &rmi->current_track);
    } else {
	/* Otherwise, apply parse rules to raw metadata */
	if (rmi->current_track.have_track_info) {
	    parse_metadata (rmi, &rmi->current_track);
	} else {
	    clear_track_info (&rmi->current_track);
	}
    }

    /* Copy the data into cbuffer */
    ret = cbuf2_insert_chunk (rmi, &rmi->cbuf2, 
			      rmi->getbuffer, rmi->getbuffer_size,
			      rmi->http_info.content_type, 
			      &rmi->current_track);
    if (ret != SR_SUCCESS) {
	debug_printf("cbuf2_insert_chunk had bad return code %d\n", ret);
	return ret;
    }

    ret = filelib_write_show (rmi, rmi->getbuffer, rmi->getbuffer_size);
    if (ret != SR_SUCCESS) {
        debug_printf("filelib_write_show had bad return code: %d\n", ret);
        return ret;
    }

    /* Set the track number */
    if (rmi->current_track.track_p[0]) {
	mstrcpy (rmi->current_track.track_a, rmi->current_track.track_p);
    } else {
        msnprintf (rmi->current_track.track_a, MAX_HEADER_LEN,
		   m_("%d"), rmi->track_count + 1);
    }

    /* First time through, so start a track. */
    if (rmi->ripstream_first_time_through) {
	int ret;
	debug_printf ("First time through...\n");
	if (!rmi->current_track.have_track_info) {
	    strcpy (rmi->current_track.raw_metadata, rmi->no_meta_name);
	}
	msnprintf (rmi->current_track.track_a, MAX_HEADER_LEN, m_("0"));
	ret = start_track_mp3 (rmi, &rmi->current_track);
	if (ret != SR_SUCCESS) {
	    debug_printf ("start_track_mp3 failed(#1): %d\n",ret);
	    return ret;
	}
	rmi->ripstream_first_time_through = 0;
	copy_track_info (&rmi->old_track, &rmi->current_track);
    }

    /* Check for track change. */
    debug_printf ("rmi->current_track.have_track_info = %d\n", 
		  rmi->current_track.have_track_info);
    if (rmi->current_track.have_track_info && is_track_changed(rmi)) {
	/* Set m_find_silence equal to the number of additional blocks 
	   needed until we can do silence separation. */
	debug_printf ("VERIFIED TRACK CHANGE (find_silence = %d)\n",
		      rmi->find_silence);
	copy_track_info (&rmi->new_track, &rmi->current_track);
	if (rmi->find_silence < 0) {
	    if (rmi->mic_to_cb_end > 0) {
		rmi->find_silence = rmi->mic_to_cb_end;
	    } else {
		rmi->find_silence = 0;
	    }
	}
    }

    debug_track_info (&rmi->old_track, "old");
    debug_track_info (&rmi->new_track, "new");
    debug_track_info (&rmi->current_track, "current");

    if (rmi->find_silence == 0) {
	/* Find separation point */
	u_long pos1, pos2;
	debug_printf ("m_find_silence == 0\n");
	ret = find_sep (rmi, &pos1, &pos2);
	if (ret == SR_ERROR_REQUIRED_WINDOW_EMPTY) {
	    /* If this happens, the previous song should be truncated to 
	       zero bytes. */
	    pos1 = -1;
	    pos2 = 0;
	}
	else if (ret != SR_SUCCESS) {
	    debug_printf("find_sep had bad return code %d\n", ret);
	    return ret;
	}

	/* Write out previous track */
	ret = end_track_mp3 (rmi, pos1, pos2, &rmi->old_track);
	if (ret != SR_SUCCESS)
	    real_ret = ret;
	rmi->cue_sheet_bytes += pos2;

	/* Start next track */
	ret = start_track_mp3 (rmi, &rmi->new_track);
	if (ret != SR_SUCCESS)
	    real_ret = ret;
	rmi->find_silence = -1;

	copy_track_info (&rmi->old_track, &rmi->new_track);
    }
    if (rmi->find_silence >= 0) rmi->find_silence --;

    /* If buffer almost full, dump extra to current song. */
    if (cbuf2_get_free(&rmi->cbuf2) < rmi->getbuffer_size) {
	u_long curr_song;
	debug_printf ("cbuf2_get_free < getbuffer_size\n");
	extract_size = rmi->getbuffer_size - cbuf2_get_free(&rmi->cbuf2);
        ret = cbuf2_extract(rmi, &rmi->cbuf2, rmi->getbuffer, 
			    extract_size, &curr_song);
        if (ret != SR_SUCCESS) {
	    debug_printf("cbuf2_extract had bad return code %d\n", ret);
	    return ret;
	}

	/* Post to caller */
	if (curr_song < extract_size) {
	    u_long curr_song_bytes = extract_size - curr_song;
	    rmi->cue_sheet_bytes += curr_song_bytes;
	    ret = rip_manager_put_data (rmi, &rmi->getbuffer[curr_song], 
					curr_song_bytes);
            if (ret != SR_SUCCESS) {
                debug_printf ("rip_manager_put_data returned: %d\n",ret);
                return ret;
            }
	}
    }

    return real_ret;
}

static error_code
find_sep (RIP_MANAGER_INFO* rmi, u_long* pos1, u_long* pos2)
{
    SPLITPOINT_OPTIONS* sp_opt = &rmi->prefs->sp_opt;
    int rw_start, rw_end, sw_sil;
    int ret;

    debug_printf ("*** Finding separation point\n");

    /* First, find the search region w/in cbuffer. */
    rw_start = rmi->cbuf2.item_count - rmi->rw_start_to_cb_end;
    if (rw_start < 0) {
	return SR_ERROR_REQUIRED_WINDOW_EMPTY;
    }
    rw_end = rmi->cbuf2.item_count - rmi->rw_end_to_cb_end;
    if (rw_end < 0) {
	return SR_ERROR_REQUIRED_WINDOW_EMPTY;
    }

    debug_printf ("search window (bytes): [%d,%d] within %d\n", 
		    rw_start, rw_end,
		    rmi->cbuf2.item_count);

    if (rmi->http_info.content_type != CONTENT_TYPE_MP3) {
	sw_sil = (rw_end + rw_start) / 2;
	debug_printf ("(not mp3) taking middle: sw_sil=%d\n", sw_sil);
	*pos1 = rw_start + sw_sil - 1;
	*pos2 = rw_start + sw_sil;
    } else {
	int bufsize = rw_end - rw_start;
	char* buf = (char*) malloc (bufsize);
	ret = cbuf2_peek_rgn (&rmi->cbuf2, buf, rw_start, bufsize);
	if (ret != SR_SUCCESS) {
	    debug_printf ("PEEK FAILED: %d\n", ret);
	    free(buf);
	    return ret;
	}
	debug_printf ("PEEK OK\n");

	/* Find silence point */
	if (sp_opt->xs == 2) {
	    ret = findsep_silence_2 (buf, 
				   bufsize, 
				   rmi->rw_start_to_sw_start,
				   sp_opt->xs_search_window_1 
				   + sp_opt->xs_search_window_2,
				   sp_opt->xs_silence_length,
				   sp_opt->xs_padding_1,
				   sp_opt->xs_padding_2,
				   pos1, pos2);
	} else {
	    ret = findsep_silence (buf, 
				   bufsize, 
				   rmi->rw_start_to_sw_start,
				   sp_opt->xs_search_window_1 
				   + sp_opt->xs_search_window_2,
				   sp_opt->xs_silence_length,
				   sp_opt->xs_padding_1,
				   sp_opt->xs_padding_2,
				   pos1, pos2);
	}
	*pos1 += rw_start;
	*pos2 += rw_start;
	free(buf);
    }
    return SR_SUCCESS;
}

// pos1 is pos of last byte in previous track
// pos2 is pos of first byte in next track
// These positions are relative to cbuf2->read_index
static error_code
end_track_mp3 (RIP_MANAGER_INFO* rmi, u_long pos1, u_long pos2, TRACK_INFO* ti)
{
    int ret;
    char *buf;

    /* GCS pos1 is byte position. Here we convert it into a "count". */
    pos1++;

    /* I think pos can be zero if the silence is right at the beginning
       i.e. it is a bug in s.r. */
    buf = (char*) malloc (pos1);

    /* First, dump the part only in prev track */
    ret = cbuf2_peek (&rmi->cbuf2, buf, pos1);
    if (ret != SR_SUCCESS) goto BAIL;

    /* Let cbuf know about the start of the next track */
    cbuf2_set_next_song (&rmi->cbuf2, pos2);

    /* Write that out to the current file
       GCS FIX: m_bytes_ripped is incorrect when there is padding */
    if ((ret = rip_manager_put_data (rmi, buf, pos1)) != SR_SUCCESS)
	goto BAIL;

    /* Add id3v1 if requested */
    if (GET_ADD_ID3V1(rmi->prefs->flags)) {
	ID3V1Tag id3v1;
	memset (&id3v1, '\000',sizeof(id3v1));
	strncpy (id3v1.tag, "TAG", strlen("TAG"));
	string_from_gstring (rmi, id3v1.artist, sizeof(id3v1.artist),
			     ti->artist, CODESET_ID3);
	string_from_gstring (rmi, id3v1.songtitle, sizeof(id3v1.songtitle),
			     ti->title, CODESET_ID3);
	string_from_gstring (rmi, id3v1.album, sizeof(id3v1.album),
			     ti->album, CODESET_ID3);
	string_from_gstring (rmi, id3v1.year, sizeof(id3v1.year),
			     ti->year, CODESET_ID3);
	id3v1.genre = (char) 0xFF; // see http://www.id3.org/id3v2.3.0.html#secA
	ret = rip_manager_put_data (rmi, (char *)&id3v1, sizeof(id3v1));
	if (ret != SR_SUCCESS) {
	    goto BAIL;
	}
    }

    /* Only save this track if we've skipped over enough cruft 
       at the beginning of the stream */
    debug_printf ("Current track number %d (skipping if less than %d)\n", 
		  rmi->track_count, rmi->prefs->dropcount);
    if (rmi->track_count >= rmi->prefs->dropcount)
	if ((ret = rip_manager_end_track (rmi, ti)) != SR_SUCCESS)
	    goto BAIL;

 BAIL:
    free(buf);
    return ret;
}

#define HEADER_SIZE 1600
// Write data to a new frame, using tag_name e.g. TPE1, and increment
// *sent with the number of bytes written
static error_code
write_id3v2_frame(RIP_MANAGER_INFO* rmi, char* tag_name, mchar* data,
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
    ret = rip_manager_put_data (rmi, (char *)&(id3v2frame.id), 4);
    if (ret != SR_SUCCESS) return ret;
    *sent += 4;
    ret = rip_manager_put_data (rmi, (char *)&(framesize),
                                sizeof(framesize));
    if (ret != SR_SUCCESS) return ret;
    *sent += sizeof(framesize);
    ret = rip_manager_put_data (rmi, (char *)&(id3v2frame.pad), 3);
    if (ret != SR_SUCCESS) return ret;
    *sent += 3;
    ret = rip_manager_put_data (rmi, bigbuf, rc);
    if (ret != SR_SUCCESS) return ret;
    *sent += rc;

    return SR_SUCCESS;
}

static error_code
start_track_mp3 (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti)
{
    int ret;
    int i;
    unsigned int secs;

    debug_printf ("calling rip_manager_start_track(#2)\n");
    ret = rip_manager_start_track (rmi, ti);
    if (ret != SR_SUCCESS) {
	debug_printf ("rip_manager_start_track failed(#2): %d\n",ret);
        return ret;
    }

    /* Dump to artist/title to cue sheet */
    secs = bytes_to_secs (rmi->cue_sheet_bytes, rmi->bitrate);
    ret = filelib_write_cue (rmi, ti, secs);
    if (ret != SR_SUCCESS)
        return ret;

    /* Oddsock's ID3 stuff, (oddsock@oddsock.org) */
    if (GET_ADD_ID3V2(rmi->prefs->flags)) {
	char bigbuf[HEADER_SIZE] = "";
	int header_size = HEADER_SIZE;
	char header1[6] = "ID3\x03\0\0";
	int sent = 0;
	int id3_charset;

	memset(bigbuf, '\000', sizeof(bigbuf));

	/* Write header */
	ret = rip_manager_put_data (rmi, header1, 6);
	if (ret != SR_SUCCESS) return ret;
	for (i = 0; i < 4; i++) {
	    char x = (header_size >> (3-i)*7) & 0x7F;
	    ret = rip_manager_put_data (rmi, (char *)&x, 1);
	    if (ret != SR_SUCCESS) return ret;
	}

	/* ID3 V2.3 is only defined for ISO-8859-1 and UCS-2
	   If user specifies another codeset, we will use it, and 
	   report ISO-8859-1 in the encoding field */
	id3_charset = is_id3_unicode(rmi);

	/* Lead performer */
        ret = write_id3v2_frame(rmi, "TPE1", ti->artist, id3_charset, &sent);

        /* Title */
        ret = write_id3v2_frame(rmi, "TIT2", ti->title, id3_charset, &sent);

        /* Encoded by */
        ret = write_id3v2_frame(rmi, "TENC",
				m_("Ripped with Streamripper"), 
				id3_charset, &sent);
	
        /* Album */
        ret = write_id3v2_frame(rmi, "TALB", ti->album, id3_charset, &sent);

        /* Track */
        ret = write_id3v2_frame(rmi, "TRCK", ti->track_a, id3_charset, &sent);

        /* Year */
        ret = write_id3v2_frame(rmi, "TYER", ti->year, id3_charset, &sent);

	/* Zero out padding */
	memset (bigbuf, '\000', sizeof(bigbuf));

	/* Pad up to header_size */
	ret = rip_manager_put_data (rmi, bigbuf, HEADER_SIZE-sent);
	if (ret != SR_SUCCESS) return ret;
    }

    if (!rmi->ripstream_first_time_through) {
        rmi->track_count ++;
        debug_printf ("Changed track count to %d\n", rmi->track_count);
    }

    return SR_SUCCESS;
}

// Only save this track if we've skipped over enough cruft 
// at the beginning of the stream
static error_code
end_track_ogg (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti)
{
    error_code ret;
    debug_printf ("Current track number %d (skipping if less than %d)\n", 
		  rmi->track_count, rmi->prefs->dropcount);
    if (rmi->track_count >= rmi->prefs->dropcount) {
	ret = rip_manager_end_track (rmi, ti);
    } else {
	ret = SR_SUCCESS;
    }
    rmi->track_count ++;
    return ret;
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
     cb	      cbuf2, aka circular buffer
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

/* GCS: This used to be myrecv in rip_manager.c */
static int
ripstream_recvall (RIP_MANAGER_INFO* rmi, char* buffer, int size)
{
    int ret;
    ret = socklib_recvall (rmi, &rmi->stream_sock, buffer, size, 
			   rmi->prefs->timeout);
    if (ret >= 0 && ret != size) {
	debug_printf ("rip_manager_recv: expected %d, got %d\n",size,ret);
	ret = SR_ERROR_RECV_FAILED;
    }
    return ret;
}

/* Data followed by meta-data */
static error_code
get_stream_data (RIP_MANAGER_INFO* rmi, char *data_buf, char *track_buf)
{
    int ret = 0;
    char c;
    char newtrack[MAX_TRACK_LEN];

    *track_buf = 0;
    rmi->current_track.have_track_info = 0;
    ret = ripstream_recvall (rmi, data_buf, rmi->getbuffer_size);
    if (ret <= 0)
	return ret;

    if (rmi->meta_interval == NO_META_INTERVAL) {
	return SR_SUCCESS;
    }

    if ((ret = ripstream_recvall (rmi, &c, 1)) <= 0)
	return ret;

    debug_printf ("METADATA LEN: %d\n",(int)c);
    if (c < 0) {
	debug_printf ("Got invalid metadata: %d\n",c);
	return SR_ERROR_INVALID_METADATA;
    } else if (c == 0) {
	/* We didn't get any metadata this time. */
	return SR_SUCCESS;
    } else {
	/* We got metadata this time. */
	ret = get_track_from_metadata (rmi, c * 16, newtrack);
	if (ret != SR_SUCCESS) {
	    debug_printf("get_trackname had a bad return %d\n", ret);
	    return ret;
	}

	strncpy(track_buf, newtrack, MAX_TRACK_LEN);
	rmi->current_track.have_track_info = 1;
    }
    return SR_SUCCESS;
}

static error_code
get_track_from_metadata (RIP_MANAGER_INFO* rmi, int size, char *newtrack)
{
    int i;
    int ret;
    char *namebuf;
    char *p;
    gchar *gnamebuf;

    /* Default is no track info */
    *newtrack = 0;

    if ((namebuf = malloc(size)) == NULL)
	return SR_ERROR_CANT_ALLOC_MEMORY;

    ret = ripstream_recvall (rmi, namebuf, size);
    if (ret <= 0) {
	free(namebuf);
	return ret;
    }

    debug_printf ("METADATA TITLE\n");
    for (i=0; i<size; i++) {
	debug_printf ("%2x ",(unsigned char)namebuf[i]);
	if (i % 20 == 19) {
	    debug_printf ("\n");
	}
    }
    debug_printf ("\n");
    for (i=0; i<size; i++) {
	if (namebuf[i] != 0) {
	    debug_printf ("%2c ",namebuf[i]);
	}
	if (i % 20 == 19) {
	    debug_printf ("\n");
	}
    }
    debug_printf ("\n");

    /* Depending on version, Icecast/Shoutcast use one of the following.
         StreamTitle='Title';StreamURL='URL';
         StreamTitle='Title';
       Limecast has no semicolon, and only example I've seen had no title.
          StreamTitle=' '
    */

    /* GCS NOTE: This assumes ASCII-compatible charset for quote & semicolon.
       Shoutcast protocol has no specification on this... */
    if (!g_str_has_prefix (namebuf, "StreamTitle='")) {
	free (namebuf);
	return SR_SUCCESS;
    }
    gnamebuf = g_strdup (namebuf+strlen("StreamTitle='"));
    free(namebuf);

    if ((p = strstr (gnamebuf, "';"))) {
	*p = 0;
    }
    else if ((p = strrchr (gnamebuf, '\''))) {
	*p = 0;
    }
    g_strstrip (gnamebuf);
    debug_printf ("gnamebuf (stripped) = %s\n", gnamebuf);

    if (strlen (gnamebuf) == 0) {
	g_free (gnamebuf);
	return SR_SUCCESS;
    }

    g_strlcpy (newtrack, gnamebuf, MAX_TRACK_LEN);
    g_free (gnamebuf);

    return SR_SUCCESS;
}
