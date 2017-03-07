/* ripogg.c
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
 * This file is adapted from ogginfo.c of the vorbis-tools project.
 * Copyright 2002 Michael Smith <msmith@xiph.org>
 * Licensed under the GNU GPL, distributed with this program.
 */
#include "srconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include "srtypes.h"
#include "cbuf2.h"
#include "ripogg.h"
#include "utf8.h"
#include "list.h"
#include "debug.h"
#include "mchar.h"

#if (HAVE_OGG_VORBIS)
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <locale.h>


#define CHUNK 4500
// #define CHUNK 1

#define _(a) (a)


struct vorbis_release {
    char *vendor_string;
    char *desc;
} releases[] = {
    {"Xiphophorus libVorbis I 20000508", "1.0 beta 1 or beta 2"},
    {"Xiphophorus libVorbis I 20001031", "1.0 beta 3"},
    {"Xiphophorus libVorbis I 20010225", "1.0 beta 4"},
    {"Xiphophorus libVorbis I 20010615", "1.0 rc1"},
    {"Xiphophorus libVorbis I 20010813", "1.0 rc2"},
    {"Xiphophorus libVorbis I 20011217", "1.0 rc3"},
    {"Xiphophorus libVorbis I 20011231", "1.0 rc3"},
    {"Xiph.Org libVorbis I 20020717", "1.0"},
    {"Xiph.Org libVorbis I 20030909", "1.0.1"},
    {NULL, NULL},
};

/* TODO:
 *
 * - detect violations of muxing constraints
 * - detect granulepos 'gaps' (possibly vorbis-specific). (seperate from 
 *   serial-number gaps)
 */
typedef struct {
    stream_processor *streams;
    int allocated;
    int used;

    int in_headers;
} stream_set;

typedef struct {
    vorbis_info vi;
    vorbis_comment vc;

    ogg_int64_t bytes;
    ogg_int64_t lastgranulepos;
    ogg_int64_t firstgranulepos;

    int doneheaders;
} misc_vorbis_info;


/*****************************************************************************
 * Private Vars
 *****************************************************************************/
//static int printinfo = 1;
//static int printwarn = 1;
//static int verbose = 1;
//static int flawed;

//static ogg_sync_state ogg_sync;
//static ogg_page page;
//static stream_processor stream;
//static char* ogg_curr_header;
//static int ogg_curr_header_len;

#define CONSTRAINT_PAGE_AFTER_EOS   1
#define CONSTRAINT_MUXING_VIOLATED  2


/*****************************************************************************
 * Functions
 *****************************************************************************/
static void warn(char *format, ...) 
{
    va_list ap;

    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}

/* Return 1 if the page is a header page */
static int
vorbis_process (RIP_MANAGER_INFO* rmi, stream_processor *stream, 
		ogg_page *page, TRACK_INFO* ti)
{
    ogg_packet packet;
    misc_vorbis_info *inf = stream->data;
    int i, header=0;
    int k;

    ogg_stream_pagein(&stream->os, page);
    if (inf->doneheaders < 3)
        header = 1;

    while (ogg_stream_packetout(&stream->os, &packet) > 0) {
        if (inf->doneheaders < 3) {
            if (vorbis_synthesis_headerin(&inf->vi, &inf->vc, &packet) < 0) {
                warn(_("Warning: Could not decode vorbis header "
                       "packet - invalid vorbis stream (%d)\n"), stream->num);
                continue;
            }
            inf->doneheaders++;
            if (inf->doneheaders == 3) {
                if(ogg_page_granulepos(page) != 0 || ogg_stream_packetpeek(&stream->os, NULL) == 1)
                    warn(_("Warning: Vorbis stream %d does not have headers "
                           "correctly framed. Terminal header page contains "
                           "additional packets or has non-zero granulepos\n"),
			 stream->num);
                debug_printf("Vorbis headers parsed for stream %d, "
			     "information follows...\n", stream->num);

                debug_printf("Version: %d\n", inf->vi.version);
                k = 0;
                while(releases[k].vendor_string) {
                    if(!strcmp(inf->vc.vendor, releases[k].vendor_string)) {
                        debug_printf("Vendor: %s (%s)\n", inf->vc.vendor, 
				     releases[k].desc);
                        break;
                    }
                    k++;
                }
                if(!releases[k].vendor_string)
                    debug_printf("Vendor: %s\n", inf->vc.vendor);
                debug_printf("Channels: %d\n", inf->vi.channels);
                debug_printf("Rate: %ld\n\n", inf->vi.rate);

                if(inf->vi.bitrate_nominal > 0)
                    debug_printf("Nominal bitrate: %f kb/s\n", 
				 (double)inf->vi.bitrate_nominal / 1000.0);
                else
                    debug_printf("Nominal bitrate not set\n");

                if(inf->vi.bitrate_upper > 0)
                    debug_printf("Upper bitrate: %f kb/s\n",
				 (double)inf->vi.bitrate_upper / 1000.0);
                else
                    debug_printf("Upper bitrate not set\n");

                if(inf->vi.bitrate_lower > 0)
                    debug_printf("Lower bitrate: %f kb/s\n", 
				 (double)inf->vi.bitrate_lower / 1000.0);
                else
                    debug_printf("Lower bitrate not set\n");

                if(inf->vc.comments > 0)
                    debug_printf ("User comments section follows...\n");

                for(i=0; i < inf->vc.comments; i++) {
                    char *sep = strchr(inf->vc.user_comments[i], '=');
                    char *decoded;
                    int j;
                    int broken = 0;
                    unsigned char *val;
                    int bytes;
                    int remaining;

                    if(sep == NULL) {
                        warn(_("Warning: Comment %d in stream %d is invalidly "
                               "formatted, does not contain '=': \"%s\"\n"), 
			     i, stream->num, inf->vc.user_comments[i]);
                        continue;
                    }

                    for(j=0; j < sep-inf->vc.user_comments[i]; j++) {
                        if(inf->vc.user_comments[i][j] < 0x20 ||
                           inf->vc.user_comments[i][j] > 0x7D) {
                            warn(_("Warning: Invalid comment fieldname in "
                                   "comment %d (stream %d): \"%s\"\n"),
				 i, stream->num, inf->vc.user_comments[i]);
                            broken = 1;
                            break;
                        }
                    }

                    if(broken)
                        continue;

                    val = (unsigned char*) inf->vc.user_comments[i];

                    j = sep-inf->vc.user_comments[i]+1;
                    while(j < inf->vc.comment_lengths[i])
                    {
                        remaining = inf->vc.comment_lengths[i] - j;
                        if((val[j] & 0x80) == 0)
                            bytes = 1;
                        else if((val[j] & 0x40) == 0x40) {
                            if((val[j] & 0x20) == 0)
                                bytes = 2;
                            else if((val[j] & 0x10) == 0)
                                bytes = 3;
                            else if((val[j] & 0x08) == 0)
                                bytes = 4;
                            else if((val[j] & 0x04) == 0)
                                bytes = 5;
                            else if((val[j] & 0x02) == 0)
                                bytes = 6;
                            else {
                                warn(_("Warning: Illegal UTF-8 sequence in "
                                       "comment %d (stream %d): length "
                                       "marker wrong\n"),
				     i, stream->num);
                                broken = 1;
                                break;
                            }
                        }
                        else {
                            warn(_("Warning: Illegal UTF-8 sequence in comment "
                                   "%d (stream %d): length marker wrong\n"),
				 i, stream->num);
                            broken = 1;
                            break;
                        }

                        if(bytes > remaining) {
                            warn(_("Warning: Illegal UTF-8 sequence in comment "
                                   "%d (stream %d): too few bytes\n"),
				 i, stream->num);
                            broken = 1;
                            break;
                        }

                        switch(bytes) {
			case 1:
			    /* No more checks needed */
			    break;
			case 2:
			    if((val[j+1] & 0xC0) != 0x80)
				broken = 1;
			    if((val[j] & 0xFE) == 0xC0)
				broken = 1;
			    break;
			case 3:
			    if(!((val[j] == 0xE0 && val[j+1] >= 0xA0 && 
				  val[j+1] <= 0xBF && 
				  (val[j+2] & 0xC0) == 0x80) ||
				 (val[j] >= 0xE1 && val[j] <= 0xEC &&
				  (val[j+1] & 0xC0) == 0x80 &&
				  (val[j+2] & 0xC0) == 0x80) ||
				 (val[j] == 0xED && val[j+1] >= 0x80 &&
				  val[j+1] <= 0x9F &&
				  (val[j+2] & 0xC0) == 0x80) ||
				 (val[j] >= 0xEE && val[j] <= 0xEF &&
				  (val[j+1] & 0xC0) == 0x80 &&
				  (val[j+2] & 0xC0) == 0x80)))
				broken = 1;
			    if(val[j] == 0xE0 && (val[j+1] & 0xE0) == 0x80)
				broken = 1;
			    break;
			case 4:
			    if(!((val[j] == 0xF0 && val[j+1] >= 0x90 &&
				  val[j+1] <= 0xBF &&
				  (val[j+2] & 0xC0) == 0x80 &&
				  (val[j+3] & 0xC0) == 0x80) ||
				 (val[j] >= 0xF1 && val[j] <= 0xF3 &&
				  (val[j+1] & 0xC0) == 0x80 &&
				  (val[j+2] & 0xC0) == 0x80 &&
				  (val[j+3] & 0xC0) == 0x80) ||
				 (val[j] == 0xF4 && val[j+1] >= 0x80 &&
				  val[j+1] <= 0x8F &&
				  (val[j+2] & 0xC0) == 0x80 &&
				  (val[j+3] & 0xC0) == 0x80)))
				broken = 1;
			    if(val[j] == 0xF0 && (val[j+1] & 0xF0) == 0x80)
				broken = 1;
			    break;
                            /* 5 and 6 aren't actually allowed at this point*/
			case 5:
			    broken = 1;
			    break;
			case 6:
			    broken = 1;
			    break;
                        }

                        if(broken) {
                            warn(_("Warning: Illegal UTF-8 sequence in comment "
                                   "%d (stream %d): invalid sequence\n"),
				 i, stream->num);
                            broken = 1;
                            break;
                        }

                        j += bytes;
                    }

                    if(!broken) {
                        if(utf8_decode(sep+1, &decoded) < 0) {
                            warn(_("Warning: Failure in utf8 decoder. This "
                                   "should be impossible\n"));
                            continue;
                        }
                        *sep = 0;
                        debug_printf ("\t%s=%s\n", 
				      inf->vc.user_comments[i], decoded);
			
			/* GCS FIX: Need case insensitive compare */
			if (!strcmp(inf->vc.user_comments[i],"artist")
			    || !strcmp(inf->vc.user_comments[i],"ARTIST")
			    || !strcmp(inf->vc.user_comments[i],"Artist")) {
			    /* GCS FIX: This is a bit funky, maybe I need 
			       to get rid of the ogg built-in utf8 decoder */
			    gstring_from_string (rmi, ti->artist, 
						 MAX_TRACK_LEN, 
						 decoded, CODESET_LOCALE);
			} else if (!strcmp(inf->vc.user_comments[i],"title")
				   || !strcmp(inf->vc.user_comments[i],"TITLE")
				   || !strcmp(inf->vc.user_comments[i],"Title")) {
			    /* GCS FIX: This is a bit funky, maybe I need 
			       to get rid of the ogg built-in utf8 decoder */
			    gstring_from_string (rmi, ti->title, MAX_TRACK_LEN, 
						 decoded, CODESET_LOCALE);
			    ti->have_track_info = 1;
			} else if (!strcmp(inf->vc.user_comments[i],"album")
				   || !strcmp(inf->vc.user_comments[i],"ALBUM")
				   || !strcmp(inf->vc.user_comments[i],"Album")) {
			    /* GCS FIX: This is a bit funky, maybe I need 
			       to get rid of the ogg built-in utf8 decoder */
			    gstring_from_string (rmi, ti->album, MAX_TRACK_LEN, 
						 decoded, CODESET_LOCALE);
			} else if (!strcmp(inf->vc.user_comments[i],"tracknumber")
				   || !strcmp(inf->vc.user_comments[i],"TRACKNUMBER")
				   || !strcmp(inf->vc.user_comments[i],"Tracknumber")) {
			    /* GCS FIX: This is a bit funky, maybe I need 
			       to get rid of the ogg built-in utf8 decoder */
			    gstring_from_string (rmi, ti->track_p, 
						 MAX_TRACK_LEN, 
						 decoded, CODESET_LOCALE);
			}
                        free(decoded);
                    }
                }
		/* Done looping through vorbis comments.  If we didn't find 
		    a title, give a default title. */
		if (!ti->have_track_info) {
		    strncpy (ti->title, "Title Unknown", MAX_TRACK_LEN);
		    ti->have_track_info = 1;
		}
            }
        }
    }

    if(!header) {
        ogg_int64_t gp = ogg_page_granulepos(page);
        if(gp > 0) {
            if(gp < inf->lastgranulepos)
#ifdef _WIN32
                warn(_("Warning: granulepos in stream %d decreases from %I64d to %I64d" ),
		     stream->num, inf->lastgranulepos, gp);
#else
	    warn(_("Warning: granulepos in stream %d decreases from %lld to %lld" ),
		 stream->num, inf->lastgranulepos, gp);
#endif
            inf->lastgranulepos = gp;
        }
        else {
            warn(_("Negative granulepos on vorbis stream outside of headers. This file was created by a buggy encoder\n"));
        }
        if(inf->firstgranulepos < 0) { /* Not set yet */
        }
        inf->bytes += page->header_len + page->body_len;
    }
    return header;
}

static void
vorbis_end(stream_processor *stream)
{
    misc_vorbis_info *inf = stream->data;
    long minutes, seconds;
    double bitrate, time;

    /* This should be lastgranulepos - startgranulepos, or something like that*/
    time = (double)inf->lastgranulepos / inf->vi.rate;
    minutes = (long)time / 60;
    seconds = (long)time - minutes*60;
    bitrate = inf->bytes*8 / time / 1000.0;

#ifdef _WIN32
    debug_printf ("Vorbis stream %d:\n"
		  "\tTotal data length: %I64d bytes\n"
		  "\tPlayback length: %ldm:%02lds\n"
		  "\tAverage bitrate: %f kbps\n", 
		  stream->num,inf->bytes, minutes, seconds, bitrate);
#else
    debug_printf ("Vorbis stream %d:\n"
		  "\tTotal data length: %lld bytes\n"
		  "\tPlayback length: %ldm:%02lds\n"
		  "\tAverage bitrate: %f kbps\n", 
		  stream->num,inf->bytes, minutes, seconds, bitrate);
#endif

    vorbis_comment_clear(&inf->vc);
    vorbis_info_clear(&inf->vi);

    free(stream->data);
}

static void 
vorbis_start(stream_processor *stream)
{
    misc_vorbis_info *info;

    stream->type = "vorbis";
    stream->process_end = vorbis_end;

    stream->data = calloc(1, sizeof(misc_vorbis_info));

    info = stream->data;

    vorbis_comment_init(&info->vc);
    vorbis_info_init(&info->vi);
}

void
rip_ogg_process_chunk (RIP_MANAGER_INFO* rmi, 
		       LIST* page_list, const char* buf, u_long size,
		       TRACK_INFO* ti)
{
    OGG_PAGE_LIST* ol;
    int header;
    int ret;
    char *buffer;
    //    static ogg_int64_t written = 0;
    //    static unsigned int written = 0;
    //    static int ogg_page2 = 0;

    INIT_LIST_HEAD (page_list);

    debug_printf ("-- rip_ogg_process_chunk (%d)\n", size);

    buffer = ogg_sync_buffer (&rmi->ogg_sync, size);
    memcpy (buffer, buf, size);
    ogg_sync_wrote (&rmi->ogg_sync, size);

    do {
	switch (ret = ogg_sync_pageout (&rmi->ogg_sync, &rmi->ogg_pg)) {
	case -1:
	    /* -1 if we were not properly synced and had to skip some bytes */
	    debug_printf ("Hole in ogg, skipping bytes\n");
	    break;
	case 0:
	    /* 0 if we need more data to verify a page */
	    debug_printf ("Ogg needs more data\n");
	    break;
	case 1:
	    /* 1 if we have a page */
	    debug_printf ("Found an ogg page!\n");

	    /* Do stuff needed for decoding vorbis */
	    if (ogg_page_bos (&rmi->ogg_pg)) {
		int rc;
		ogg_packet packet;
		ogg_stream_init (&rmi->stream.os, 
				 ogg_page_serialno (&rmi->ogg_pg));
		ogg_stream_pagein (&rmi->stream.os, &rmi->ogg_pg);
		rc = ogg_stream_packetout(&rmi->stream.os, &packet);
		if (rc <= 0) {
		    printf ("Warning: Invalid header page, no packet found\n");
		    // null_start (&rmi->stream);
		    exit (1);
		} else if (packet.bytes >= 7 
			   && memcmp(packet.packet, "\001vorbis", 7)==0) {
		    vorbis_start (&rmi->stream);
		}
	    }
	    header = vorbis_process (rmi, &rmi->stream, &rmi->ogg_pg, ti);
	    if (ogg_page_eos (&rmi->ogg_pg)) {
		vorbis_end (&rmi->stream);
	    }

	    /* Create ogg page boundary struct */
	    ol = (OGG_PAGE_LIST*) malloc (sizeof(OGG_PAGE_LIST));
	    if (!ol) {
		printf ("Malloc error\n");
		exit (1);
	    }
	    ol->m_page_len = rmi->ogg_pg.header_len + rmi->ogg_pg.body_len;
	    ol->m_page_flags = 0;

	    /* *****************************************************
               Create header buffer for relay stream. A pointer to the 
	       header buffer will attached to all pages after page 2.
	       If a relay connects in the middle of a song, we send 
	       the header to the relay. Finally, the memory for the 
	       header is freed when the last page of the song is 
	       ejected from the cbuf. 
	    ** ******************************************************/
	    if (ogg_page_bos (&rmi->ogg_pg)) {
		/* First page in song */
		ol->m_page_flags |= OGG_PAGE_BOS;
		ol->m_header_buf_ptr = 0;
		ol->m_header_buf_len = 0;
		rmi->ogg_curr_header = (char*) malloc (ol->m_page_len);
		rmi->ogg_curr_header_len = ol->m_page_len;
		memcpy (rmi->ogg_curr_header, 
			rmi->ogg_pg.header, rmi->ogg_pg.header_len);
		memcpy (rmi->ogg_curr_header+rmi->ogg_pg.header_len, 
			rmi->ogg_pg.body, rmi->ogg_pg.body_len);
	    } else if (header) {
		/* Second or third page in song */
		ol->m_page_flags |= OGG_PAGE_2;
		ol->m_header_buf_ptr = 0;
		ol->m_header_buf_len = 0;
		rmi->ogg_curr_header = (char*) 
			realloc (rmi->ogg_curr_header,
				 rmi->ogg_curr_header_len + ol->m_page_len);
		memcpy (rmi->ogg_curr_header+rmi->ogg_curr_header_len,
			rmi->ogg_pg.header, rmi->ogg_pg.header_len);
		memcpy (rmi->ogg_curr_header + rmi->ogg_curr_header_len 
			+ rmi->ogg_pg.header_len,
			rmi->ogg_pg.body, rmi->ogg_pg.body_len);
		rmi->ogg_curr_header_len += ol->m_page_len;
	    } else if (!ogg_page_eos (&rmi->ogg_pg)) {
		/* Middle pages in song */
		ol->m_header_buf_ptr = rmi->ogg_curr_header;
		ol->m_header_buf_len = rmi->ogg_curr_header_len;
	    } else {
		/* Last page in song */
		ol->m_page_flags |= OGG_PAGE_EOS;
		ol->m_header_buf_ptr = rmi->ogg_curr_header;
		ol->m_header_buf_len = rmi->ogg_curr_header_len;
		rmi->ogg_curr_header = 0;
		rmi->ogg_curr_header_len = 0;
	    }

	    debug_printf ("OGG_PAGE\n"
			  "  header_len = %d\n"
			  "  body_len = %d\n"
			  "  serial no = %d\n"
			  "  page no = %d\n"
			  "  bos? = %d\n"
			  "  eos? = %d\n",
			  rmi->ogg_pg.header_len,
			  rmi->ogg_pg.body_len,
			  ogg_page_serialno (&rmi->ogg_pg),
			  ogg_page_pageno (&rmi->ogg_pg),
			  ogg_page_bos (&rmi->ogg_pg),
			  ogg_page_eos (&rmi->ogg_pg));
	    list_add_tail (&(ol->m_list), page_list);
	    break;
	}
    } while (ret != 0);

    debug_printf ("OGG_SYNC state:\n"
		  "  storage = %d\n"
		  "  fill = %d\n"
		  "  returned = %d\n"
		  "  unsynced = %d\n"
		  "  headerbytes = %d\n"
		  "  bodybytes = %d\n",
		  rmi->ogg_sync.storage,
		  rmi->ogg_sync.fill,
		  rmi->ogg_sync.returned,
		  rmi->ogg_sync.unsynced,
		  rmi->ogg_sync.headerbytes,
		  rmi->ogg_sync.bodybytes);
    //    return 1;
}

void
rip_ogg_get_current_header (RIP_MANAGER_INFO* rmi, char** ptr, int* len)
{
    *ptr = rmi->ogg_curr_header;
    *len = rmi->ogg_curr_header_len;
}

void
rip_ogg_init (RIP_MANAGER_INFO* rmi)
{
    ogg_sync_init (&rmi->ogg_sync);
    memset (&rmi->stream, 0, sizeof(stream_processor));
    rmi->ogg_curr_header = 0;
    rmi->ogg_curr_header_len = 0;
}

#else /* HAVE_OGG_VORBIS == 0 */

void
rip_ogg_process_chunk (RIP_MANAGER_INFO* rmi, 
		       LIST* page_list, const char* buf, u_long size,
		       TRACK_INFO* ti)
{
    INIT_LIST_HEAD (page_list);
}

void
rip_ogg_get_current_header (RIP_MANAGER_INFO* rmi, char** ptr, int* len)
{
    *ptr = 0;
    *len = 0;
}

void
rip_ogg_init (RIP_MANAGER_INFO* rmi)
{
}

#endif /* HAVE_OGG_VORBIS */
