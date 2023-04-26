/* ripstream_ogg.h
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
#ifndef __RIPSTREAM_WAV_H__
#define __RIPSTREAM_WAV_H__

#include "srtypes.h"

error_code
mp3_to_wav_init (void** handle);

error_code
mp3_to_wav_run (void* handle, unsigned char** ptr_wav, unsigned long* sz_wav,
                const unsigned char* ptr_mp3, const unsigned long sz_mp3);

error_code
mp3_to_wav_finish (void* handle);

#endif
