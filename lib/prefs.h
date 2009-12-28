/* prefs.h
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
#ifndef __PREFS_H__
#define __PREFS_H__

#include <glib.h>

#define DEFAULT_SKINFILE	"srskin.bmp"

/* Prototypes */
int prefs_load (void);
void prefs_save (void);
void prefs_get_global_prefs (GLOBAL_PREFS *global_prefs);
void prefs_set_global_prefs (GLOBAL_PREFS *global_prefs);
void prefs_get_stream_prefs (STREAM_PREFS *prefs, char *label);
void prefs_set_stream_prefs (STREAM_PREFS *prefs, char *label);
void prefs_get_wstreamripper_prefs (WSTREAMRIPPER_PREFS *wsr_prefs);
void prefs_set_wstreamripper_prefs (WSTREAMRIPPER_PREFS *wsr_prefs);
void debug_stream_prefs (STREAM_PREFS* prefs);

#endif
