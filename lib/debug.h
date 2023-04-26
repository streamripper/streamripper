/* debug.h
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
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "srtypes.h"
#include "time.h"
#include <stdio.h>

#define debug_mprintf debug_printf

void debug_open(void);
void debug_set_filename(char *filename);
void debug_close(void);
void debug_printf(char *fmt, ...);
// void debug_mprintf (mchar* fmt, ...);
void debug_enable(void);
void debug_print_error(void);

#endif
