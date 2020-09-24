/* This program is free software; you can redistribute it and/or modify
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
#include "errors.h"
#include "mad.h"
#include "srtypes.h"

/******************************************************************************
 * Public functions
 *****************************************************************************/
error_code
mp3_to_wav (char** write_ptr_wav, long* write_sz_wav, const char* write_ptr, const long write_sz)
{
#if 0
    struct mad_decoder decoder;
    int result;

    /* Run decoder */
    mad_decoder_init (&decoder, &ds, input, header, filter, output,
        error, NULL);
    result = mad_decoder_run (&decoder, MAD_DECODER_MODE_SYNC);
    mad_decoder_finish (&decoder);
#endif

    /* Use MAD to decode mp3 into wav */
    return SR_SUCCESS;
}
