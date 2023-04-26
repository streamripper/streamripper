/* findsep.c
 * library routines for find silent points in mp3 data
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
 *
 * Portions are adapted from minimad.c, included with the
 * libmad library, distributed under the GNU General Public License.
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 */
#include "debug.h"
#include "findsep.h"
#include "list.h"
#include "mad.h"
#include "srtypes.h"
#include <assert.h>
#include <math.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RMS_SILENCE 32767 // max short
#define READSIZE 2000
// #define READSIZE	1000

/* Uncomment to dump an mp3 of the search window. */
//   #define MAKE_DUMP_MP3 1

typedef struct FRAME_LIST_struct FRAME_LIST;
struct FRAME_LIST_struct {
	const unsigned char *m_framepos;
	long m_samples;
	long m_pcmpos;
	LIST m_list;
};

typedef struct MIN_POSst {
	unsigned short volume;
	unsigned long pos;
} MIN_POS;

typedef struct DECODE_STRUCTst {
	unsigned char *mpgbuf;
	long mpgsize;
	long mpgpos;
	long len_to_sw_ms;
	long searchwindow_ms;
	long silence_ms;
	long silence_samples;
	unsigned long len_to_sw_start_samp;
	unsigned long len_to_sw_end_samp;
	unsigned long pcmpos;
	long samplerate;
	short prev_sample;
	LIST frame_list;
	unsigned short *maxvolume_buffer;
	unsigned long maxvolume_buffer_offs;
	int maxvolume_buffer_depth;
	int max_search_depth;
	MIN_POS *min_maxvolume_buffer;
} DECODE_STRUCT;

typedef struct GET_BITRATE_STRUCTst {
	unsigned long bitrate;
	unsigned char *mpgbuf;
	long mpgsize;
} GET_BITRATE_STRUCT;

/*****************************************************************************
 * Public functions
 *****************************************************************************/

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static void apply_padding(
    DECODE_STRUCT *ds,
    unsigned long silsplit,
    long padding1,
    long padding2,
    u_long *pos1,
    u_long *pos2);
static void free_frame_list(DECODE_STRUCT *ds);
static enum mad_flow input(void *data, struct mad_stream *ms);
static void search_for_silence(DECODE_STRUCT *ds, unsigned short vol);
static signed int scale(mad_fixed_t sample);
static enum mad_flow
output(void *data, struct mad_header const *header, struct mad_pcm *pcm);
static enum mad_flow
filter(void *data, struct mad_stream const *ms, struct mad_frame *frame);
static enum mad_flow
error(void *data, struct mad_stream *ms, struct mad_frame *frame);
static enum mad_flow header(void *data, struct mad_header const *pheader);

/*****************************************************************************
 * Private Vars
 *****************************************************************************/

/*****************************************************************************
 * Functions
 *****************************************************************************/
error_code
findsep_silence_2(
    const char *mpgbuf,
    long mpgsize,
    long len_to_sw,
    long searchwindow,
    long silence_length,
    long padding1,
    long padding2,
    u_long *pos1,
    u_long *pos2) {
	DECODE_STRUCT ds;
	struct mad_decoder decoder;
	int result;
	int bestsil;
	int i;
	double delta = 1;

	ds.mpgbuf = (unsigned char *)mpgbuf;
	ds.mpgsize = mpgsize;
	ds.pcmpos = 0;
	ds.mpgpos = 0;
	ds.samplerate = 0;
	ds.prev_sample = 0;
	ds.len_to_sw_ms = len_to_sw;
	ds.searchwindow_ms = searchwindow;
	ds.silence_ms = silence_length;
	ds.maxvolume_buffer = 0;
	ds.maxvolume_buffer_offs = 0;
	ds.maxvolume_buffer_depth = 0;
	ds.max_search_depth = 0;
	ds.min_maxvolume_buffer = 0;
	INIT_LIST_HEAD(&ds.frame_list);

	debug_printf(
	    "FINDSEP 2: %p -> %p (%d)\n", mpgbuf, mpgbuf + mpgsize, mpgsize);

#if defined(MAKE_DUMP_MP3)
	{
		FILE *fp = fopen("dump.mp3", "wb");
		fwrite(mpgbuf, mpgsize, 1, fp);
		fclose(fp);
	}
#endif

	/* Run decoder */
	mad_decoder_init(
	    &decoder,
	    &ds,
	    input /* input */,
	    header /* header */,
	    filter /* filter */,
	    output /* output */,
	    error /* error */,
	    NULL /* message */);
	result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
	mad_decoder_finish(&decoder);

	debug_printf("total length:    %d\n", ds.pcmpos);
	debug_printf("silence_length:  %d ms\n", ds.silence_ms);
	debug_printf("silence_samples: %d\n", ds.silence_samples);

	/* Search through siltrackers to find minimum volume point */
	assert(ds.mpgsize != 0);

	/* Start with the highest silence-length */
	bestsil = 0;

	for (i = 1; i <= ds.max_search_depth; ++i) {
		unsigned long current = ds.min_maxvolume_buffer[bestsil].volume;
		unsigned long candidate = ds.min_maxvolume_buffer[i].volume;

		delta *= 0.6;

		/* Only halve the silence-length if we can reduce the
		   max-volume by at least 40 % by doing so. */
		if (current * delta > candidate) {
			bestsil = i;
			delta = 1;
		}
	}

	debug_printf(
	    "Most silent region: depth %d, max-volume %d, pos %ld,"
	    "sample window %ld (%f ms)\n",
	    bestsil,
	    ds.min_maxvolume_buffer[bestsil].volume,
	    ds.min_maxvolume_buffer[bestsil].pos,
	    ds.silence_samples / (1 << bestsil),
	    ds.silence_samples * 1000.0 / (double)(ds.samplerate * (1 << bestsil)));

	/* Now that we have the silence position, let's add the padding */
	apply_padding(
	    &ds,
	    ds.min_maxvolume_buffer[bestsil].pos,
	    padding1,
	    padding2,
	    pos1,
	    pos2);

	/* Free the max volume buffers */
	free(ds.maxvolume_buffer);
	free(ds.min_maxvolume_buffer);

	/* Free the list of frame info */
	free_frame_list(&ds);

	return SR_SUCCESS;
}

static void
apply_padding(
    DECODE_STRUCT *ds,
    unsigned long silsplit,
    long padding1,
    long padding2,
    u_long *pos1,
    u_long *pos2) {
	/* Compute positions in samples */
	FRAME_LIST *pos;
	long pos1s, pos2s;

	pos1s = silsplit + padding1 * (ds->samplerate / 1000);
	pos2s = silsplit - padding2 * (ds->samplerate / 1000);

	debug_printf(
	    "Applying padding: p1,p2 = (%d,%d), pos1s,pos2s = (%d,%d)\n",
	    padding1,
	    padding2,
	    pos1s,
	    pos2s);

	/* GCS FIX: Need to check for pos == null */
	/* GCS FIX: Watch out for -1, might have mem error! */
	pos = list_entry(ds->frame_list.next, FRAME_LIST, m_list);
	if (pos1s < pos->m_pcmpos) {
		*pos1 = pos->m_framepos - ds->mpgbuf - 1;
	}
	if (pos2s < pos->m_pcmpos) {
		*pos2 = pos->m_framepos - ds->mpgbuf;
	}
	list_for_each_entry(pos, FRAME_LIST, &(ds->frame_list), m_list) {
		if (pos1s >= pos->m_pcmpos) {
			*pos1 = pos->m_framepos - ds->mpgbuf - 1;
		}
		if (pos2s >= pos->m_pcmpos) {
			*pos2 = pos->m_framepos - ds->mpgbuf;
		}
	}
	debug_printf(
	    "pos1, pos2 = %d,%d (%d) (%02x%02x)\n",
	    *pos1,
	    *pos2,
	    *pos1 - *pos2,
	    ds->mpgbuf[*pos2],
	    ds->mpgbuf[*pos2 + 1]);
}

static void
free_frame_list(DECODE_STRUCT *ds) {
	FRAME_LIST *pos, *n;
	/* GCS: This seems to be the best way to go through a list.
	   Note no compiler warnings. */
	list_for_each_entry_safe(pos, FRAME_LIST, n, &(ds->frame_list), m_list) {
		list_del(&(pos->m_list));
		free(pos);
	}
}

static enum mad_flow
input(void *data, struct mad_stream *ms) {
	DECODE_STRUCT *ds = (DECODE_STRUCT *)data;
	long frameoffset = 0;
	long espnextpos = ds->mpgpos + READSIZE;

	/* GCS FIX: This trims the last READSIZE from consideration */
	if (espnextpos > ds->mpgsize) {
		return MAD_FLOW_STOP;
	}

	if (ms->next_frame) {
		frameoffset = &(ds->mpgbuf[ds->mpgpos]) - ms->next_frame;
		/* GCS July 8, 2004
		   This is the famous frameoffset != READSIZE bug.
		   What appears to be happening is libmad is not syncing
		   properly on the broken initial frame.  Therefore,
		   if there is no header yet (hence no ds->samplerate),
		   we'll nudge along the buffer to try to resync.
		 */
		if (frameoffset == READSIZE) {
			if (!ds->samplerate) {
				frameoffset--;
			} else {
				FILE *fp;
				debug_printf(
				    "%p | %p | %p | %p | %d\n",
				    ds->mpgbuf,
				    ds->mpgpos,
				    &(ds->mpgbuf[ds->mpgpos]),
				    ms->next_frame,
				    frameoffset);
				fprintf(stderr, "ERROR: frameoffset != READSIZE\n");
				debug_printf("ERROR: frameoffset != READSIZE\n");
				fp = fopen("gcs1.txt", "w");
				fwrite(ds->mpgbuf, 1, ds->mpgsize, fp);
				fclose(fp);
				exit(-1);
			}
		}
	}
	debug_printf(
	    "%p | %p | %p |< %p | %p >| %d\n",
	    ds->mpgbuf,
	    ds->mpgpos,
	    &(ds->mpgbuf[ds->mpgpos]),
	    ms->this_frame,
	    ms->next_frame,
	    frameoffset);

	mad_stream_buffer(
	    ms,
	    (const unsigned char *)(ds->mpgbuf + ds->mpgpos) - frameoffset,
	    READSIZE);
	ds->mpgpos += READSIZE - frameoffset;

	return MAD_FLOW_CONTINUE;
}

static void
propagate_max_value(
    unsigned short *max_buffer, unsigned long node_pos, int depth) {
	unsigned long current_pos = node_pos;
	unsigned long parent_pos = current_pos / 2;

	while (depth-- > 0) {
		unsigned long sibling_pos = current_pos ^ 1;

		unsigned short current_val = max_buffer[current_pos];
		unsigned short sibling_val = max_buffer[sibling_pos];

		if (current_val > sibling_val)
			max_buffer[parent_pos] = current_val;
		else
			max_buffer[parent_pos] = sibling_val;

		current_pos = parent_pos;
		parent_pos /= 2;
	}
}

static void
insert_value(DECODE_STRUCT *ds, unsigned short vol, unsigned long pos) {
	int i;
	unsigned long current_pos = 1;
	unsigned short prev = ds->maxvolume_buffer[ds->maxvolume_buffer_offs];
	int depth = 1;

	ds->maxvolume_buffer[ds->maxvolume_buffer_offs] = vol;
	vol = prev;

	for (i = 1; i < ds->maxvolume_buffer_depth; ++i) {
		unsigned long index =
		    ds->maxvolume_buffer_offs + current_pos + (pos % current_pos);

		prev = ds->maxvolume_buffer[index];
		ds->maxvolume_buffer[index] = vol;
		vol = prev;

		propagate_max_value(ds->maxvolume_buffer, index, depth++);

		current_pos *= 2;
	}
}

static void
search_for_silence(DECODE_STRUCT *ds, unsigned short vol) {
	unsigned long window_size = 1;
	unsigned long window_pos = ds->pcmpos - ds->len_to_sw_start_samp;
	unsigned long node_pos = ds->maxvolume_buffer_offs;
	int i;

	insert_value(ds, vol, ds->pcmpos);

	window_size = 1;
	for (i = ds->maxvolume_buffer_depth - 1; i >= 0; --i) {
		if (i <= ds->max_search_depth && window_pos >= window_size
		    && ds->maxvolume_buffer[node_pos]
		           < ds->min_maxvolume_buffer[i].volume) {
			ds->min_maxvolume_buffer[i].volume = ds->maxvolume_buffer[node_pos];
			ds->min_maxvolume_buffer[i].pos = ds->pcmpos - window_size / 2;
		}

		node_pos /= 2;
		window_size *= 2;
	}
}

static signed int
scale(mad_fixed_t sample) {
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	/* quantize */
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

static enum mad_flow
filter(void *data, struct mad_stream const *ms, struct mad_frame *frame) {
	DECODE_STRUCT *ds = (DECODE_STRUCT *)data;
	FRAME_LIST *fl;

	fl = (FRAME_LIST *)malloc(sizeof(FRAME_LIST));
	fl->m_framepos = ms->this_frame;
	fl->m_samples = 0;
	fl->m_pcmpos = 0;
	list_add_tail(&(fl->m_list), &(ds->frame_list));

#if defined(commentout)
	debug_printf(
	    "FILTER: %p (%02x%02x) | %p\n",
	    ms->this_frame,
	    ms->this_frame[0],
	    ms->this_frame[1],
	    ms->next_frame);
#endif

	return MAD_FLOW_CONTINUE;
}

static enum mad_flow
output(void *data, struct mad_header const *header, struct mad_pcm *pcm) {
	DECODE_STRUCT *ds = (DECODE_STRUCT *)data;
	FRAME_LIST *fl;
	unsigned int nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;
	static signed int sample;
	double v;

	nchannels = pcm->channels;
	nsamples = pcm->length;
	left_ch = pcm->samples[0];
	right_ch = pcm->samples[1];

	/* Get frame entry */
	fl = list_entry(ds->frame_list.prev, FRAME_LIST, m_list);
	fl->m_samples = nsamples;
	fl->m_pcmpos = ds->pcmpos;

	if (ds->pcmpos > ds->len_to_sw_start_samp
	    && ds->pcmpos < ds->len_to_sw_end_samp) {
		debug_printf("* %d\n", ds->pcmpos);
	} else {
		debug_printf("- %d\n", ds->pcmpos);
	}
#if defined(commentout)
#endif

	while (nsamples--) {
		/* output sample(s) in 16-bit signed little-endian PCM */
		/* GCS FIX: Does this work on big endian machines??? */
		sample = (short)scale(*left_ch++);
		//	fwrite(&sample, sizeof(short), 1, fp);

		if (nchannels == 2) {
			// make mono
			sample = (sample + scale(*right_ch++)) / 2;
		}

		// get the instantanous volume
		v = (ds->prev_sample * ds->prev_sample) + (sample * sample);
		v = sqrt(v / 2);
		if (ds->pcmpos > ds->len_to_sw_start_samp
		    && ds->pcmpos < ds->len_to_sw_end_samp) {
			search_for_silence(ds, (short)v);
		}
		ds->pcmpos++;
		ds->prev_sample = sample;
	}

	return MAD_FLOW_CONTINUE;
}

static unsigned long
next_power_of_two(long value) {
	long result = 1;
	while (result < value)
		result *= 2;
	return result;
}

static void
init_maxvolume_buffer(DECODE_STRUCT *ds) {
	unsigned long buffer_offset = 1;
	unsigned long temp = ds->silence_samples;
	unsigned long depth = 0;
	unsigned long i;
	while (temp > 0) {
		++depth;
		temp /= 2;
		buffer_offset += temp;
	}

	ds->maxvolume_buffer_depth = depth;

	/* Let the minimum silence-length be 10 ms. */
	ds->max_search_depth =
	    depth - (int)ceil(log(10 * (ds->samplerate / 1000.0)) / log(2)) - 1;

	/* Unless the user specified silence-length is lower. */
	if (ds->max_search_depth < 0)
		ds->max_search_depth = 0;

	ds->maxvolume_buffer = (unsigned short *)calloc(
	    buffer_offset + ds->silence_samples, sizeof(unsigned short));
	ds->maxvolume_buffer_offs = buffer_offset;

	ds->min_maxvolume_buffer = (MIN_POS *)malloc(depth * sizeof(MIN_POS));

	for (i = 0; i < depth; ++i) {
		ds->min_maxvolume_buffer[i].volume = MAX_RMS_SILENCE;
		ds->min_maxvolume_buffer[i].pos = 0;
	}
}

static enum mad_flow
header(void *data, struct mad_header const *pheader) {
	DECODE_STRUCT *ds = (DECODE_STRUCT *)data;
	if (!ds->samplerate) {
		ds->samplerate = pheader->samplerate;
		ds->silence_samples =
		    next_power_of_two(ds->silence_ms * (ds->samplerate / 1000));
		ds->len_to_sw_start_samp = ds->len_to_sw_ms * (ds->samplerate / 1000);
		ds->len_to_sw_end_samp =
		    (ds->len_to_sw_ms + ds->searchwindow_ms) * (ds->samplerate / 1000);
		init_maxvolume_buffer(ds);
		debug_printf("Setting samplerate: %ld\n", ds->samplerate);
	}
	return MAD_FLOW_CONTINUE;
}

static enum mad_flow
error(void *data, struct mad_stream *ms, struct mad_frame *frame) {
	if (MAD_RECOVERABLE(ms->error)) {
		debug_printf("mad error 0x%04x\n", ms->error);
		return MAD_FLOW_CONTINUE;
	}

	debug_printf("unrecoverable mad error 0x%04x\n", ms->error);
	return MAD_FLOW_BREAK;
}
