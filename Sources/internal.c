//
//  internal.c
//  NYXPNGTools
//
//  Created by Nyx0uf on 07/02/12.
//  Copyright (c) 2012 Benjamin Godard. All rights reserved.
//  www.cocoaintheshell.com
//


#include "internal.h"
#include <zlib.h>


#pragma mark - Public functions
npt_ulong_t
npt_crc(npt_byte_t* name, npt_byte_t* buf, const npt_uint32_t size)
{
	npt_ulong_t crc = crc32(0, name, 4);
	return crc32(crc, buf, size);
}

void
npt_free_png_chunks(npt_png_chunk** chunks)
{
	for (npt_uint32_t i = 0; i < NPT_MAX_CHUNKS; i++)
	{
		npt_png_chunk* chunk = *(chunks + i);
		free(chunk->_data);
		if (__iendchunk == chunk->_name)
		{
			free(chunk);
			break;
		}
		free(chunk);
	}
	free(chunks), chunks = NULL;
}
