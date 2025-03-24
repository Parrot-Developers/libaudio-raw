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

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "araw_priv.h"

#define ULOG_TAG araw
#include <ulog.h>

#include <pthread.h>
#define NB_SUPPORTED_FORMATS 24
static struct adef_format supported_formats[NB_SUPPORTED_FORMATS];
static pthread_once_t supported_formats_is_init = PTHREAD_ONCE_INIT;
static void initialize_supported_formats(void)
{
	int i = 0;
	supported_formats[i++] = adef_pcm_16b_8000hz_mono;
	supported_formats[i++] = adef_pcm_16b_8000hz_stereo;
	supported_formats[i++] = adef_pcm_16b_11025hz_mono;
	supported_formats[i++] = adef_pcm_16b_11025hz_stereo;
	supported_formats[i++] = adef_pcm_16b_12000hz_mono;
	supported_formats[i++] = adef_pcm_16b_12000hz_stereo;
	supported_formats[i++] = adef_pcm_16b_16000hz_mono;
	supported_formats[i++] = adef_pcm_16b_16000hz_stereo;
	supported_formats[i++] = adef_pcm_16b_22050hz_mono;
	supported_formats[i++] = adef_pcm_16b_22050hz_stereo;
	supported_formats[i++] = adef_pcm_16b_24000hz_mono;
	supported_formats[i++] = adef_pcm_16b_24000hz_stereo;
	supported_formats[i++] = adef_pcm_16b_32000hz_mono;
	supported_formats[i++] = adef_pcm_16b_32000hz_stereo;
	supported_formats[i++] = adef_pcm_16b_44100hz_mono;
	supported_formats[i++] = adef_pcm_16b_44100hz_stereo;
	supported_formats[i++] = adef_pcm_16b_48000hz_mono;
	supported_formats[i++] = adef_pcm_16b_48000hz_stereo;
	supported_formats[i++] = adef_pcm_16b_64000hz_mono;
	supported_formats[i++] = adef_pcm_16b_64000hz_stereo;
	supported_formats[i++] = adef_pcm_16b_88200hz_mono;
	supported_formats[i++] = adef_pcm_16b_88200hz_stereo;
	supported_formats[i++] = adef_pcm_16b_96000hz_mono;
	supported_formats[i++] = adef_pcm_16b_96000hz_stereo;
}


struct araw_reader {
	char *filename;
	FILE *file;
	struct araw_reader_config cfg;
	struct wave_header header;
	uint32_t data_length;
	int index;
	float timestamp;
	size_t frame_size;
};


static int wave_header_read(struct araw_reader *self)
{
	int ret;

	ret = fread(&self->header, sizeof(self->header), 1, self->file);
	if (ret < 0) {
		ret = -errno;
		ULOG_ERRNO("fread", -ret);
		return ret;
	}

	ULOG_ERRNO_RETURN_ERR_IF(self->header.chunk_id != FOURCC_RIFF, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(self->header.format != FOURCC_WAVE, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(self->header.subchunk1_id != FOURCC_fmt_,
				 EINVAL);

	bool additional_header_data_present = false;
	while (self->header.subchunk2_id != FOURCC_data) {
#if UINT32_MAX > LONG_MAX
		ULOG_ERRNO_RETURN_ERR_IF(
			self->header.subchunk2_size >= LONG_MAX, EINVAL);
#endif
		ret = fseek(self->file, self->header.subchunk2_size, SEEK_CUR);
		if (ret < 0) {
			ret = -errno;
			ULOG_ERRNO("fseek", -ret);
			return ret;
		}
		ret = fread(&self->header.subchunk2_id,
			    sizeof(self->header.subchunk2_id),
			    1,
			    self->file);
		if (ret < 0) {
			ret = -errno;
			ULOG_ERRNO("fread", -ret);
			return ret;
		}
		ret = fread(&self->header.subchunk2_size,
			    sizeof(self->header.subchunk2_size),
			    1,
			    self->file);
		if (ret < 0) {
			ret = -errno;
			ULOG_ERRNO("fread", -ret);
			return ret;
		}
		additional_header_data_present = true;
	}

	ULOG_ERRNO_RETURN_ERR_IF(self->header.subchunk2_id != FOURCC_data,
				 EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(
		self->header.audio_format != ADEF_WAVE_FORMAT_PCM, EINVAL);

	self->data_length = self->header.subchunk2_size;

	/* Fill format */
	self->cfg.format.encoding = ADEF_ENCODING_PCM;
	self->cfg.wave_format = self->header.format;
	self->cfg.format.bit_depth = self->header.bits_per_sample;
	self->cfg.format.channel_count = self->header.num_channels;
	self->cfg.format.sample_rate = self->header.sample_rate;
	/* RIFF WAV file format: little endian
	 * FIFX WAV file format: big endian */
	self->cfg.format.pcm.little_endian = true;
	self->cfg.format.pcm.interleaved = true;
	/**
	 * Format      Maximum Value   Minimum Value     Midpoint Value
	 * 8-bit PCM   255 (0xFF)      0                 128 (0x80)
	 * 16-bit PCM  32767 (0x7FFF)  -32768 (-0x8000)  0
	 */
	self->cfg.format.pcm.signed_val = (self->cfg.format.bit_depth > 8);
	self->cfg.data_length = self->data_length;

	return 0;
}


static int
wave_read_data(struct araw_reader *self, unsigned char *data, size_t len)
{
	int n;
	if (self->file == NULL)
		return -EINVAL;
	if (len > self->data_length)
		len = self->data_length;
	n = fread(data, 1, len, self->file);
	self->data_length -= len;
	return n;
}


int araw_reader_new(const char *filename,
		    const struct araw_reader_config *config,
		    struct araw_reader **ret_obj)
{
	int ret = 0;
	struct araw_reader *self = NULL;

	(void)pthread_once(&supported_formats_is_init,
			   initialize_supported_formats);

	ULOG_ERRNO_RETURN_ERR_IF(filename == NULL, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(config == NULL, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(ret_obj == NULL, EINVAL);

	self = calloc(1, sizeof(*self));
	if (self == NULL)
		return -ENOMEM;

	self->cfg = *config;

	if (self->cfg.frame_length == 0)
		self->cfg.frame_length = DEFAULT_FRAME_LENGTH;

	self->filename = strdup(filename);
	if (self->filename == NULL) {
		ret = -ENOMEM;
		goto error;
	}

	self->file = fopen(self->filename, "rb");
	if (self->file == NULL) {
		ret = -errno;
		ULOG_ERRNO("fopen('%s')", -ret, self->filename);
		goto error;
	}

	/* Read WAVE file header */
	ret = wave_header_read(self);
	if (ret < 0)
		goto error;

	self->frame_size = self->cfg.frame_length *
			   self->cfg.format.channel_count *
			   (self->cfg.format.bit_depth / 8);

	*ret_obj = self;

	return 0;

error:
	(void)araw_reader_destroy(self);
	*ret_obj = NULL;
	return ret;
}


int araw_reader_destroy(struct araw_reader *self)
{
	if (self == NULL)
		return 0;

	if (self->file != NULL)
		fclose(self->file);

	free(self->filename);
	free(self);
	return 0;
}


int araw_reader_get_config(struct araw_reader *self,
			   struct araw_reader_config *config)
{
	ULOG_ERRNO_RETURN_ERR_IF(self == NULL, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(config == NULL, EINVAL);

	*config = self->cfg;
	return 0;
}


ssize_t araw_reader_get_min_buf_size(struct araw_reader *self)
{
	ULOG_ERRNO_RETURN_ERR_IF(self == NULL, EINVAL);

	return self->frame_size;
}


int araw_reader_frame_read(struct araw_reader *self,
			   uint8_t *data,
			   size_t len,
			   struct araw_frame *frame)
{
	int ret;
	ULOG_ERRNO_RETURN_ERR_IF(self == NULL, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(data == NULL, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(len == 0, ENOBUFS);
	ULOG_ERRNO_RETURN_ERR_IF(frame == NULL, EINVAL);

	ULOG_ERRNO_RETURN_ERR_IF(len < self->frame_size, ENOBUFS);
	ULOG_ERRNO_RETURN_ERR_IF(self->file == NULL, EPROTO);

	/* Read the PCM data */
	ret = wave_read_data(self, data, len);
	if (ret < 0) {
		ULOG_ERRNO("wave_read_data", -ret);
		return ret;
	} else if ((size_t)ret != len) {
		return -ENOENT;
	}

	/* Fill the frame info */
	frame->frame.format = self->cfg.format;
	frame->frame.info.timestamp = (uint64_t)self->timestamp;
	frame->frame.info.timescale = 1000000;
	frame->frame.info.index = self->index;

	self->index++;
	self->timestamp += 1000000ULL * self->cfg.frame_length /
			   self->cfg.format.sample_rate;

	return 0;
}
