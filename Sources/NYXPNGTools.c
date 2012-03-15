//
//  NYXPNGTools.c
//  NYXPNGTools
//
//  Created by Nyx0uf on 02/02/12.
//  Copyright (c) 2012 Benjamin Godard. All rights reserved.
//  www.cocoaintheshell.com
//


#include "NYXPNGTools.h"
#include "internal.h"
#include "decrypt_png.h"
#include "flip_channels.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <zlib.h>


#pragma mark - Public functions
int
npt_is_png(const char* path, int* error)
{
	if (!path)
	{
		*error = npt_err_invalid_path;
		return 0;
	}
	
	/// Open the file
	int fd = open(path, O_RDONLY);
	if (-1 == fd)
	{
		*error = npt_err_open_failed;
		fprintf(stderr, "[+] NYXPNGTools: Couldn't open '%s' : %s (%d)\n", path, strerror(errno), errno);
		return 0;
	}
	
	/// Copy the file in a buffer
	npt_byte_t buffer[8];
	const ssize_t count = read(fd, buffer, 8);
	/// Don't need the descriptor anymore
	close(fd);
	/// Check that the read call was OK
	if (count != 8)
	{
		*error = npt_err_read_failed;
		fprintf(stderr, "[+] NYXPNGTools: Couldn't read '%s' : %s (%d)\n", path, strerror(errno), errno);
        return 0;
    }
	
	*error = npt_err_ok;
	
	return (__pngheader == *((size_t*)buffer));
}

int
npt_is_apple_crushed_png(const char* path, int* error)
{
	if (!path)
	{
		*error = npt_err_invalid_path;
		return 0;
	}

	/// Open the file
	int fd = open(path, O_RDONLY);
	if (-1 == fd)
	{
		*error = npt_err_open_failed;
		fprintf(stderr, "[+] NYXPNGTools: Couldn't open '%s' : %s (%d)\n", path, strerror(errno), errno);
		return 0;
	}
	
	/// Copy the file in a buffer
	npt_byte_t buffer[20];
	const ssize_t count = read(fd, buffer, 20);
	/// Don't need the descriptor anymore
	close(fd);
	/// Check that the read call was OK
	if (count != 20)
	{
		*error = npt_err_read_failed;
		fprintf(stderr, "[+] NYXPNGTools: Couldn't read '%s' : %s (%d)\n", path, strerror(errno), errno);
        return 0;
    }

	*error = npt_err_ok;

	return (__cgbichunk == *((unsigned int*)&(buffer[12])));
}

unsigned char*
npt_create_uncrushed_from_file(const char* path, unsigned int* size, int* error)
{
	if (!path)
	{
		*error = npt_err_invalid_path;
		return NULL;
	}

	*size = 0;
	/// Stat the file to obtain its size
	struct stat st;
	if (stat(path, &st) < 0)
	{
		*error = npt_err_stat_failed;
		fprintf(stderr, "[+] NYXPNGTools: Couldn't stat '%s' : %s (%d)\n", path, strerror(errno), errno);
        return NULL;
	}

	/// Open the file
	int fd = open(path, O_RDONLY);
	if (-1 == fd)
	{
		*error = npt_err_open_failed;
		fprintf(stderr, "[+] NYXPNGTools: Couldn't open '%s' : %s (%d)\n", path, strerror(errno), errno);
		return NULL;
	}

	/// Copy the file in a buffer
	npt_byte_t* buffer = (npt_byte_t*)malloc(sizeof(npt_byte_t) * (size_t)st.st_size);
	const ssize_t count = read(fd, buffer, (size_t)st.st_size);
	/// Don't need the descriptor anymore
	close(fd);
	/// Check that the read call was OK
	if (count != st.st_size)
	{
		*error = npt_err_read_failed;
		fprintf(stderr, "[+] NYXPNGTools: Couldn't read '%s' : %s (%d)\n", path, strerror(errno), errno);
		free(buffer);
        return NULL;
    }

	/// Check that we have a PNG
	if (__pngheader != *((size_t*)buffer))
	{
		*error = npt_err_not_png;
		fprintf(stderr, "[+] NYXPNGTools: '%s' doesn't appear to be a PNG file\n", path);
		free(buffer);
        return NULL;
	}

	/// Read PNG chunks
	npt_png_chunk** chunks = NULL;
	if (st.st_size < 524288) // Small image, max 512Kb
	{
		chunks = (npt_png_chunk**)malloc(sizeof(npt_png_chunk*) * 8);
		(void)npt_read_chunks(buffer, chunks);
		npt_process_chunks_simple(chunks);
	}
	else
	{
		chunks = (npt_png_chunk**)malloc(sizeof(npt_png_chunk*) * NPT_MAX_CHUNKS);
		const npt_uint32_t idats = npt_read_chunks(buffer, chunks);
		/// Process them
		if (idats > 1)
		{
			npt_png_chunk** ret = npt_process_chunks(chunks);
			npt_free_png_chunks(chunks);
			if (!ret)
			{
				return NULL;
			}
			chunks = ret;
		}
		else
		{
			npt_process_chunks_simple(chunks);
		}
	}
	/// No longer needed
	free(buffer);

	/// Create the decrypted PNG in memory
	npt_byte_t* decrytped = npt_create_decrypted_in_memory(chunks, size);
	npt_free_png_chunks(chunks);

	if (!decrytped)
	{
		*error = npt_err_uncrush_failed;
		fprintf(stderr, "[+] NYXPNGTools: Couldn't create uncrushed PNG from '%s'\n", path);
		return NULL;
	}

    /// Apple's crushed PNG are RGBA, let's flip the channels
	npt_byte_t* out = npt_flip_channels(decrytped, size);
	free(decrytped);

	*error = (out) ? npt_err_ok : npt_err_flip_channels_failed;

	return out;
}

char*
npt_error_message(const int error)
{
	char* msg = NULL;
	switch (error)
	{
		case npt_err_ok:
			msg = "No error.";
			break;
		case npt_err_invalid_path:
			msg = "The specified path was invalid (NULL).";
			break;
		case npt_err_open_failed:
			msg = "Couldn't open the file.";
			break;
		case npt_err_read_failed:
			msg = "Error while reading the file.";
			break;
		case npt_err_stat_failed:
			msg = "Couldn't stat the file.";
			break;
		case npt_err_not_png:
			msg = "The file isn't a PNG image.";
			break;
		case npt_err_uncrush_failed:
			msg = "Failed to uncrush the PNG image.";
			break;
		case npt_err_flip_channels_failed:
			msg = "Failed to flip channels (RGBA -> BGRA) for uncrushed PNG image.";
			break;
		default:
			msg = "Unknown error.";
	}
	return msg;
}
