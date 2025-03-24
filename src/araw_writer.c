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
#include <stddef.h>
#include <stdio.h>

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


struct araw_writer {
	char *filename;
	FILE *file;
	struct araw_writer_config cfg;
	struct wave_header header;
	uint32_t data_length;
};


static int wave_header_write(struct araw_writer *self)
{
	int ret;

	self->header.chunk_id = FOURCC_RIFF;
	self->header.chunk_size = 0; /* Fill this in on file-close */
	self->header.format = FOURCC_WAVE;
	self->header.subchunk1_id = FOURCC_fmt_;
	self->header.subchunk1_size = 16; /* PCM */
	self->header.audio_format = ADEF_WAVE_FORMAT_PCM;
	self->header.num_channels = self->cfg.format.channel_count;
	self->header.sample_rate = self->cfg.format.sample_rate;
	self->header.byte_rate = self->cfg.format.sample_rate *
				 self->cfg.format.channel_count *
				 (self->cfg.format.bit_depth / 8);
	self->header.block_align = self->cfg.format.channel_count *
				   (self->cfg.format.bit_depth / 8);
	self->header.bits_per_sample = 8 * (self->cfg.format.bit_depth / 8);
	self->header.subchunk2_id = FOURCC_data;
	self->header.subchunk2_size = 0; /* Fill this in on file-close */

	/* Write WAVE header */
	ret = fwrite(&self->header, sizeof(self->header), 1, self->file);
	if (ret < 0) {
		ret = -errno;
		ULOG_ERRNO("fwrite", -ret);
		return ret;
	}

	return 0;
}


int araw_writer_new(const char *filename,
		    const struct araw_writer_config *config,
		    struct araw_writer **ret_obj)
{
	int ret = 0;
	struct araw_writer *self = NULL;

	(void)pthread_once(&supported_formats_is_init,
			   initialize_supported_formats);

	ULOG_ERRNO_RETURN_ERR_IF(filename == NULL, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(config == NULL, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(!adef_format_intersect(&config->format,
							supported_formats,
							NB_SUPPORTED_FORMATS),
				 EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(ret_obj == NULL, EINVAL);

	self = calloc(1, sizeof(*self));
	if (self == NULL)
		return -ENOMEM;

	self->cfg = *config;

	self->filename = strdup(filename);
	if (self->filename == NULL) {
		ret = -ENOMEM;
		goto error;
	}

	self->file = fopen(self->filename, "wb");
	if (self->file == NULL) {
		ret = -errno;
		ULOG_ERRNO("fopen('%s')", -ret, self->filename);
		goto error;
	}

	/* Write WAV file headers */
	ret = wave_header_write(self);
	if (ret < 0)
		goto error;

	*ret_obj = self;
	return 0;

error:
	(void)araw_writer_destroy(self);
	*ret_obj = NULL;
	return ret;
}


int araw_writer_destroy(struct araw_writer *self)
{
	int ret;

	if (self == NULL)
		return 0;

	if (self->file == NULL) {
		ret = -EINVAL;
		goto out;
	}

	self->header.subchunk2_size = self->data_length;
	self->header.chunk_size = offsetof(struct wave_header, subchunk2_id) +
				  self->header.subchunk2_size;

	/* Seek to "chunk_size" in WAVE header */
	ret = fseek(
		self->file, offsetof(struct wave_header, chunk_size), SEEK_SET);
	if (ret != 0) {
		ret = -errno;
		ULOG_ERRNO("fseek", -ret);
		goto out;
	}

	/* Fill "self->header.chunk_size" in on file-close */
	ret = fwrite(&self->header.chunk_size,
		     sizeof(self->header.chunk_size),
		     1,
		     self->file);
	if (ret != 1) {
		ret = -errno;
		ULOG_ERRNO("fwrite", -ret);
		goto out;
	}

	/* Seek to "subchunk2_size" in WAVE header */
	ret = fseek(self->file,
		    offsetof(struct wave_header, subchunk2_size),
		    SEEK_SET);
	if (ret != 0) {
		ret = -errno;
		ULOG_ERRNO("fseek", -ret);
		goto out;
	}

	/* Fill "self->header.subchunk2_size" in on file-close */
	ret = fwrite(&self->header.subchunk2_size,
		     sizeof(self->header.subchunk2_size),
		     1,
		     self->file);
	if (ret != 1) {
		ret = -errno;
		ULOG_ERRNO("fwrite", -ret);
		goto out;
	}

	ret = 0;
out:
	if (self->file != NULL)
		fclose(self->file);

	free(self->filename);
	free(self);
	return ret;
}


int araw_writer_frame_write(struct araw_writer *self,
			    const struct araw_frame *frame)
{
	int ret = 0;
	const uint8_t *ptr;

	ULOG_ERRNO_RETURN_ERR_IF(self == NULL, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(frame == NULL, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(
		!adef_format_cmp(&frame->frame.format, &self->cfg.format),
		EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(self->file == NULL, EPROTO);

	/* Write PCM data to file */
	ptr = frame->cdata;
	ret = fwrite(ptr, frame->cdata_length, 1, self->file);
	if (ret != 1) {
		ret = -errno;
		ULOG_ERRNO("fwrite", -ret);
		return ret;
	}

	self->data_length += frame->cdata_length;
	return 0;
}
