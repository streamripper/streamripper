/* sr_config.h.cmake
 *
 * This file is processed automatically by cmake to produce sr_config.h
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
#ifndef __sr_config_h__
#define __sr_config_h__

#cmakedefine OGG_FOUND 1
#cmakedefine VORBIS_FOUND 1

#if (OGG_FOUND && VORBIS_FOUND)
#define OGG_VORBIS_FOUND 1
#endif

#endif
