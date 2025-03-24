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

#ifndef _ARAW_H_
#define _ARAW_H_

#include <stdint.h>
#include <unistd.h>

#include <audio-defs/adefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* To be used for all public API */
#ifdef ARAW_API_EXPORTS
#	ifdef _WIN32
#		define ARAW_API __declspec(dllexport)
#	else /* !_WIN32 */
#		define ARAW_API __attribute__((visibility("default")))
#	endif /* !_WIN32 */
#else /* !ARAW_API_EXPORTS */
#	define ARAW_API
#endif /* !ARAW_API_EXPORTS */


/* Forward declarations */
struct araw_reader;
struct araw_writer;


/* Frame data */
struct araw_frame {
	/* Samples data pointers */
	union {
		/* To be used for example by the reader */
		uint8_t *data;

		/* To be used for example by the writer */
		const uint8_t *cdata;
	};

	size_t cdata_length;

	/* Audio frame metadata */
	struct adef_frame frame;
};


/* Reader configuration */
struct araw_reader_config {
	int data_length;

	/* Raw format (can be empty for wav files, mandatory otherwise) */
	struct adef_format format;

	/* WAVE file format */
	enum adef_wave_format wave_format;

	/* Number of samples per frame */
	unsigned int frame_length;
};


/* Writer configuration */
struct araw_writer_config {
	/* Data format (mandatory) */
	struct adef_format format;
};


/**
 * Create a file reader instance.
 * The configuration structure must be filled.
 * The instance handle is returned through the ret_obj parameter.
 * When no longer needed, the instance must be freed using the
 * araw_reader_destroy() function.
 * @param filename: file name
 * @param config: reader configuration
 * @param ret_obj: reader instance handle (output)
 * @return 0 on success, negative errno value in case of error
 */
ARAW_API int araw_reader_new(const char *filename,
			     const struct araw_reader_config *config,
			     struct araw_reader **ret_obj);


/**
 * Free a reader instance.
 * This function frees all resources associated with a reader instance.
 * @param self: reader instance handle
 * @return 0 on success, negative errno value in case of error
 */
ARAW_API int araw_reader_destroy(struct araw_reader *self);


/**
 * Get the reader configuration.
 * The configuration structure is filled by the function.
 * @param self: reader instance handle
 * @param config: reader configuration (output)
 * @return 0 on success, negative errno value in case of error
 */
ARAW_API int araw_reader_get_config(struct araw_reader *self,
				    struct araw_reader_config *config);


/**
 * Get the minimum buffer size for reading a frame.
 * @param self: reader instance handle
 * @return buffer size on success, negative errno value in case of error
 */
ARAW_API ssize_t araw_reader_get_min_buf_size(struct araw_reader *self);


/**
 * Read a frame.
 * Reads a frame from the file into the provided data buffer.
 * The frame structure is filled by the function with the frame metadata.
 * @param self: reader instance handle
 * @param data: pointer on the buffer to fill
 * @param len: buffer size
 * @param frame: frame metadata (output)
 * @return 0 on success, negative errno value in case of error
 */
ARAW_API int araw_reader_frame_read(struct araw_reader *self,
				    uint8_t *data,
				    size_t len,
				    struct araw_frame *frame);


/**
 * Create a file writer instance.
 * The configuration structure must be filled.
 * The instance handle is returned through the ret_obj parameter.
 * When no longer needed, the instance must be freed using the
 * araw_writer_destroy() function.
 * @param filename: file name
 * @param config: writer configuration
 * @param ret_obj: writer instance handle (output)
 * @return 0 on success, negative errno value in case of error
 */
ARAW_API int araw_writer_new(const char *filename,
			     const struct araw_writer_config *config,
			     struct araw_writer **ret_obj);


/**
 * Free a writer instance.
 * This function frees all resources associated with a writer instance.
 * @param self: writer instance handle
 * @return 0 on success, negative errno value in case of error
 */
ARAW_API int araw_writer_destroy(struct araw_writer *self);


/**
 * Write a frame.
 * Writes a frame to the file. The profided frame structure must be filled
 * with the frame metadata.
 * @param self: writer instance handle
 * @param frame: frame metadata
 * @return 0 on success, negative errno value in case of error
 */
ARAW_API int araw_writer_frame_write(struct araw_writer *self,
				     const struct araw_frame *frame);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !_ARAW_H_ */
