//
//  decrypt_png.h
//  NYXPNGTools
//
//  Created by Nyx0uf on 03/02/12.
//  Copyright (c) 2012 Benjamin Godard. All rights reserved.
//  www.cocoaintheshell.com
//


#ifndef __NYXPNGTOOLS_DECRYPT_PNG_H__
#define __NYXPNGTOOLS_DECRYPT_PNG_H__

#include "internal.h"


/*!
 *	@function npt_read_chunks
 *	@abstract Parse all PNG chunks
 *	@param buffer [in] : PNG data
 *  @param chunks [out] : List of chunks, must be pre-allocated
 *	@return Number of IDATs section
 */
unsigned int npt_read_chunks(npt_byte_t* buffer, npt_png_chunk** chunks);

/*!
 *	@function npt_process_chunks
 *	@abstract Decrypt IDATs chunks
 *	@param chunks [in] : List of chunks obtained from npt_read_chunks()
 *	@return Newly allocated list of chunks, must be freed.
 */
npt_png_chunk** npt_process_chunks(npt_png_chunk** chunks);

/*!
 *	@function npt_process_chunks_simple
 *	@abstract Decrypt a PNG containing a single IDAT chunk (Uses npt_read_chunks() return value)
 *	@param chunks [in] : List of chunks obtained from npt_read_chunks()
 */
void npt_process_chunks_simple(npt_png_chunk** chunks);

/*!
 *	@function npt_create_decrypted_in_memory
 *	@abstract Create a PNG in memory
 *	@param chunks [in] : List of chunks
 *  @param size [out] : Output buffer size 
 *	@return Buffer containing PNG data
 */
npt_byte_t* npt_create_decrypted_in_memory(npt_png_chunk** chunks, unsigned int* size);

#endif /* __NYXPNGTOOLS_DECRYPT_PNG_H__ */
