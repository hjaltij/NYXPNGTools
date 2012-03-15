//
//  internal.h
//  NYXPNGTools
//
//  Created by Nyx0uf on 03/02/12.
//  Copyright (c) 2012 Benjamin Godard. All rights reserved.
//  www.cocoaintheshell.com
//


#ifndef _NYXPNGTOOLS_INTERNAL_H_
#define _NYXPNGTOOLS_INTERNAL_H_

#include <stdlib.h>

/* Macros */
#ifdef DEBUG
	#define NPT_DLOG(...) fprintf(stderr, __VA_ARGS__)
#else
	#define NPT_DLOG(...) do { } while (0)
#endif

#define NPT_MAX_CHUNKS 128 // Can handle a 32Mb PNG
#define NPT_BUFSIZE 5242880 // 1048576 4194304
#define NPT_MAX_IDAT_LENGTH 524288


/* Error codes */
typedef enum
{
	npt_err_ok = 0,
	npt_err_invalid_path = -1,
	npt_err_open_failed = -2,
	npt_err_read_failed = -3,
	npt_err_stat_failed = -4,
	npt_err_not_png = -10,
	npt_err_uncrush_failed = -11,
	npt_err_flip_channels_failed = -12
} npt_error_t;

/* PNG chunks */
static const size_t __pngheader = 727905341920923785; // {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}
static const unsigned int __idatchunk = 1413563465; // {0x49, 0x44, 0x41, 0x54}
static const unsigned int __iendchunk = 1145980233; // {0x49, 0x45, 0x4E, 0x44}
static const unsigned int __cgbichunk = 1229088579; // {0x43, 0x67, 0x42, 0x49}


/* Typedefs */
typedef unsigned char	npt_byte_t;
typedef unsigned int	npt_uint32_t;
typedef unsigned long	npt_ulong_t;

/*
 A chunk name is 4 char
 0 : Uppercase = Critical | Lowercase = Ancillary
 1 : Uppercase = Public chunk | Lowercase = Private chunk
 2 : Uppercase to conform PNG specs
 3 : If lowercase, the chunk may be safely copied regardless of the extent of modifications to the file. If uppercase, it may only be copied if the modifications have not touched any critical chunks.
 */
typedef struct s_npt_png_chunk_t
{
	npt_uint32_t _name;
	npt_uint32_t _size;
	npt_byte_t* _data;
	npt_uint32_t _crc;
} npt_png_chunk;

typedef struct s_npt_png_buffer_t
{
	npt_byte_t* _data;
	npt_uint32_t _size;
} npt_png_buffer;


#pragma mark - Public functions
/*!
 *	@function npt_free_png_chunks
 *	@abstract Reclaim memory from a PNG chunks list
 *	@param chunks [in] : Chunks list to free
 */
void
npt_free_png_chunks(npt_png_chunk** chunks);

/*!
 *	@function _npt_crc
 *	@abstract Calculate CRC
 *	@param name [in] : PNG chunk name
 *	@param buf [in] : PNG chunk data
 *	@param size [in] : Data length
 *	@return CRC
 */
npt_ulong_t
npt_crc(npt_byte_t* name, npt_byte_t* buf, const npt_uint32_t size);

#endif /* _NYXPNGTOOLS_INTERNAL_H_ */
