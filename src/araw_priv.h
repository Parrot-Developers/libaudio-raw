/**
 * Copyright (c) 2023 Parrot Drones SAS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Parrot Drones SAS Company nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE PARROT DRONES SAS COMPANY BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARAW_PRIV_H_
#define _ARAW_PRIV_H_

#include <audio-raw/araw.h>

#define DEFAULT_FRAME_LENGTH 1024

#define MAKE_FOURCC(a, b, c, d)                                                \
	((uint32_t)((a) | (b) << 8 | (c) << 16 | (d) << 24))

#define FOURCC_RIFF MAKE_FOURCC('R', 'I', 'F', 'F')
#define FOURCC_WAVE MAKE_FOURCC('W', 'A', 'V', 'E')
#define FOURCC_fmt_ MAKE_FOURCC('f', 'm', 't', ' ')
#define FOURCC_data MAKE_FOURCC('d', 'a', 't', 'a')

/* See: http://soundfile.sapp.org/doc/WaveFormat/ */
struct wave_header {
	/* Contains the letters "RIFF" in ASCII form */
	uint32_t chunk_id;
	/* This is the size of the rest of the chunk following this number. */
	uint32_t chunk_size;
	/* Contains the letters "WAVE" */
	uint32_t format;
	/* Contains the letters "fmt " */
	uint32_t subchunk1_id;
	/* 16 for PCM.  This is the size of the rest of the Subchunk which
	 * follows this number. */
	uint32_t subchunk1_size;
	/* PCM = 1 (i.e. Linear quantization). Values other than 1 indicate some
	 * form of compression. */
	uint16_t audio_format;
	/* Mono = 1, Stereo = 2, etc. */
	uint16_t num_channels;
	/* 8000, 44100, etc. */
	uint32_t sample_rate;
	/* == SampleRate * NumChannels * BitsPerSample/8 */
	uint32_t byte_rate;
	/* == NumChannels * BitsPerSample/8 */
	uint16_t block_align;
	/* 8 bits = 8, 16 bits = 16, etc. */
	uint16_t bits_per_sample;
	/* Contains the letters "data" */
	uint32_t subchunk2_id;
	/* == NumSamples * NumChannels * BitsPerSample/8 */
	uint32_t subchunk2_size;
};

#endif /* !_ARAW_PRIV_H_ */
