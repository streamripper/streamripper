/* track_info.c
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
#include "ripstream_ogg.h"
#include "debug.h"
#include "filelib.h"
#include "relaylib.h"
#include "socklib.h"
#include "external.h"
#include "ripogg.h"

/******************************************************************************
 * Public functions
 *****************************************************************************/
BOOL
track_info_different (TRACK_INFO *t1, TRACK_INFO *t2)
{
    /* We test the parsed fields instead of raw_metadata because the 
       parse rules may have stripped garbage out, causing the resulting 
       track info to be the same. */
    if (!strcmp(t1->artist, t2->artist)
	&& !strcmp(t1->title, t2->title)
	&& !strcmp(t1->album, t2->album)
	&& !strcmp(t1->track_p, t2->track_p)
	&& !strcmp(t1->year, t2->year))
    {
	return 0;
    }

    /* Otherwise, there was a change. */
    return 1;
}

void
track_info_debug (TRACK_INFO* ti, char* tag)
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

void
track_info_clear (TRACK_INFO* ti)
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

void
track_info_copy (TRACK_INFO* dest, TRACK_INFO* src)
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

