//
//  flip_channels.c
//  NYXPNGTools
//
//  Created by Nyx0uf on 03/02/12.
//  Copyright (c) 2012 Benjamin Godard. All rights reserved.
//  www.cocoaintheshell.com
//


#include "flip_channels.h"
#include "png.h"


typedef struct
{
	npt_byte_t* _buffer;
	npt_uint32_t _read;
} npt_custom_read_s;

typedef struct
{
	npt_byte_t* _buffer;
	size_t _size;
} npt_custom_write_s;

static void npt_png_read_from_memory(png_structp png_ptr, png_bytep data, png_size_t length);
static void npt_png_write_to_memory(png_structp png_ptr, png_bytep data, png_size_t length);


npt_byte_t*
npt_flip_channels(npt_byte_t* buffer, npt_uint32_t* size)
{
	/* READ */
    /// Initialize stuff
	*size = 0;
	png_bytep* row_pointers = NULL;
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		fprintf(stderr, "[+] NYXPNGTools: png_create_read_struct failed\n");
		return NULL;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fprintf(stderr, "[+] NYXPNGTools: png_create_info_struct failed\n");
		return NULL;
	}

	/// Read buffer, skip the first 8 bytes as it's the PNG header
	npt_custom_read_s source = (npt_custom_read_s){._buffer = buffer, ._read = 8};
	/// Custom callback function to read from a buffer
	png_set_read_fn(png_ptr, (png_voidp)&source, npt_png_read_from_memory);
	png_set_sig_bytes(png_ptr, 8);
	/// Read infos
	png_read_info(png_ptr, info_ptr);

	/// Get / Set Image infos
	const png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
	const png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
	//png_byte color_type = png_get_color_type(png_ptr, info_ptr);
	const png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	png_read_update_info(png_ptr, info_ptr);

	/// Read file failed
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		for (npt_uint32_t j = 0; j < height; j++)
			free(row_pointers[j]);
		free(row_pointers);
		fprintf(stderr, "[+] NYXPNGTools: png_read_image failed\n");
		return NULL;
	}

	/// Read buffer
	row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
	const png_size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);
	for (npt_uint32_t j = 0; j < height; j++)
		row_pointers[j] = (png_byte*)malloc(row_bytes);
	
	png_read_image(png_ptr, row_pointers);

	/* PROCCESS */
	const png_byte color_type = png_get_color_type(png_ptr, info_ptr);
	if (color_type != PNG_COLOR_TYPE_RGBA)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		for (npt_uint32_t j = 0; j < height; j++)
			free(row_pointers[j]);
		free(row_pointers);
		fprintf(stderr, "[+] NYXPNGTools: color_type is not PNG_COLOR_TYPE_RGBA (is %d)\n", color_type);
		return NULL;
	}
	/// Flip R & B
	for (npt_uint32_t i = 0; i < height; i++)
	{
		for (npt_uint32_t j = 0; j < width * 4; j += 4)
		{
			png_byte tmp = *(*(row_pointers + i) + j);
			*(*(row_pointers + i) + j) = *(*(row_pointers + i) + j + 2);
			*(*(row_pointers+i) + j + 2) = tmp;
		}
	}

	/* WRITE */
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	/// Inits...
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		for (npt_uint32_t j = 0; j < height; j++)
			free(row_pointers[j]);
		free(row_pointers);
		fprintf(stderr, "[+] NYXPNGTools: png_create_write_struct failed\n");
		return NULL;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, NULL);
		for (npt_uint32_t j = 0; j < height; j++)
			free(row_pointers[j]);
		free(row_pointers);
		fprintf(stderr, "[+] NYXPNGTools: png_create_info_struct failed\n");
		return NULL;
	}

	/// Custom write function
	npt_custom_write_s ctx = (npt_custom_write_s){._buffer = NULL, ._size = 0};
	png_set_write_fn(png_ptr, &ctx, npt_png_write_to_memory, NULL);

	/// Write header
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		for (npt_uint32_t j = 0; j < height; j++)
			free(row_pointers[j]);
		free(row_pointers);
		free(ctx._buffer);
		fprintf(stderr, "[+] NYXPNGTools: png_write_info failed\n");
		return NULL;
	}
	png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(png_ptr, info_ptr);

    /// write bytes
    if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		for (npt_uint32_t j = 0; j < height; j++)
			free(row_pointers[j]);
		free(row_pointers);
		free(ctx._buffer);
		fprintf(stderr, "[+] NYXPNGTools: png_write_image failed\n");
		return NULL;
	}
	png_write_image(png_ptr, row_pointers);

    /// End write
    if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		for (npt_uint32_t j = 0; j < height; j++)
			free(row_pointers[j]);
		free(row_pointers);
		free(ctx._buffer);
		fprintf(stderr, "[+] NYXPNGTools: png_write_end failed\n");
		return NULL;
	}	
	png_write_end(png_ptr, NULL);
	
	/// Cleanup
	png_destroy_write_struct(&png_ptr, &info_ptr);
    for (npt_uint32_t j = 0; j < height; j++)
        free(row_pointers[j]);
    free(row_pointers);

	*size = (npt_uint32_t)ctx._size;
	return ctx._buffer;
}

#pragma mark - Private
void npt_png_read_from_memory(png_structp png_ptr, png_bytep data, png_size_t length)
{
	/// Get the context
	npt_custom_read_s* ctx = (npt_custom_read_s*)png_get_io_ptr(png_ptr);
	/// Copy the requested amount of data
	memcpy(data, &(ctx->_buffer[ctx->_read]), length);
	/// Increase index
	ctx->_read += length;
}

void npt_png_write_to_memory(png_structp png_ptr, png_bytep data, png_size_t length)
{
	/// Get the context
	npt_custom_write_s* ctx = (npt_custom_write_s*)png_get_io_ptr(png_ptr);
	/// Compute new size
	const size_t new_size = length + ctx->_size;
	/// Update buffer
	ctx->_buffer = realloc(ctx->_buffer, new_size);
	memcpy(&(ctx->_buffer[ctx->_size]), data, length);
	/// Update size
	ctx->_size = new_size;
}
