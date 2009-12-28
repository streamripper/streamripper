/* track_info.h
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
#ifndef __TRACK_INFO_H__
#define __TRACK_INFO_H__

#include "srtypes.h"

BOOL
track_info_different (TRACK_INFO *t1, TRACK_INFO *t2);
void
track_info_debug (TRACK_INFO* ti, char* tag);
void
track_info_clear (TRACK_INFO* ti);
void
track_info_copy (TRACK_INFO* dest, TRACK_INFO* src);

#endif
