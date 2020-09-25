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

#include <stdio.h>

/*****************************************************************************
 * Private functions
 *****************************************************************************/
struct buffer {
    const unsigned char *ptr_mp3;
    unsigned long sz_mp3;
    unsigned char *ptr_wav;
    unsigned long sz_wav;
};

static int sfd;

static enum mad_flow input(void *data, struct mad_stream *stream)
{
    struct buffer *buffer = data;
    if (!buffer->sz_mp3)
        return MAD_FLOW_STOP;
    
    mad_stream_buffer(stream, buffer->ptr_mp3, buffer->sz_mp3);
    buffer->sz_mp3 = 0;
    
    return MAD_FLOW_CONTINUE;
}

/*这一段是处理采样后的pcm音频 */
static inline signed int scale(mad_fixed_t sample)
{
    sample += (1L << (MAD_F_FRACBITS - 16));
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;
    return sample >> (MAD_F_FRACBITS + 1 - 16);
}

static enum mad_flow output(void *data,
    struct mad_header const *header, struct mad_pcm *pcm)
{
    struct buffer *buffer = data;

    // TODO Refactor for any number of channels.
    unsigned int nchannels = pcm->channels;
    mad_fixed_t const *left_ch = pcm->samples[0];
    mad_fixed_t const *right_ch = pcm->samples[1];

    // The data length is 4 times the pcm audio sample.
    unsigned int nsamples = pcm->length;
    buffer->ptr_wav = realloc(buffer->ptr_wav, buffer->sz_wav + nsamples * 4);
    unsigned char* ptr = buffer->ptr_wav + buffer->sz_wav;
    buffer->sz_wav += nsamples * sizeof(int);
    while (nsamples--)
    {
        signed int sample;
        sample = scale(*left_ch++);
        *(ptr++) = sample >> 0;
        *(ptr++) = sample >> 8;
        if (nchannels == 2)
        {
            sample = scale(*right_ch++);
            *(ptr++) = sample >> 0;
            *(ptr++) = sample >> 8;
        }
    }
    return MAD_FLOW_CONTINUE;
}

static enum mad_flow error(void *data,
    struct mad_stream *stream, struct mad_frame *frame)
{
    struct buffer *buffer = data;

    fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %u\n",
        stream->error, mad_stream_errorstr(stream),
        (unsigned int)(stream->this_frame - buffer->ptr_mp3));

    return MAD_FLOW_CONTINUE;
}

/******************************************************************************
 * Public functions
 *****************************************************************************/
error_code
mp3_to_wav (unsigned char** ptr_wav, unsigned long* sz_wav,
            const unsigned char* ptr_mp3, const unsigned long sz_mp3)
{
    struct buffer buffer;
    buffer.ptr_mp3 = ptr_mp3;
    buffer.sz_mp3 = sz_mp3;
    buffer.ptr_wav = NULL;
    buffer.sz_wav = 0;

    /* Use MAD to decode mp3 into wav */
    struct mad_decoder decoder;
    mad_decoder_init(&decoder, &buffer, input, 0, 0, output, error, 0);
    mad_decoder_options(&decoder, 0);
    int result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
    mad_decoder_finish(&decoder);

    if (result == 0) {
        *ptr_wav = buffer.ptr_wav;
	*sz_wav = buffer.sz_wav;
        return SR_SUCCESS;
    }

    *ptr_wav = NULL;
    *sz_wav = 0;

    return SR_ERROR_DECODE_FAILURE;
}

