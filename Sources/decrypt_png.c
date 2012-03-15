//
//  decrypt_png.c
//  NYXPNGTools
//
//  Created by Nyx0uf on 03/02/12.
//  Copyright (c) 2012 Benjamin Godard. All rights reserved.
//  www.cocoaintheshell.com
//


#include "decrypt_png.h"
#include <string.h>
#include <zlib.h>
#include <stdio.h>


unsigned int
npt_read_chunks(npt_byte_t* buffer, npt_png_chunk** chunks)
{
	int i = -1;
	unsigned int idats = 0;
	
	/// Skip PNG header
	buffer += 8;
	/// Loop through all chunks 
	while (i++ < NPT_MAX_CHUNKS)
	{
		npt_png_chunk* chunk = (npt_png_chunk*)malloc(sizeof(npt_png_chunk));

		/// Size, 4 bytes, PNG specs say that integers are big-endian
		chunk->_size = ntohl(*((npt_uint32_t*)buffer));
		buffer += 4;

		/// Name, 4 bytes
		chunk->_name = *((npt_uint32_t*)buffer);
		buffer += 4;

		/// Data
		chunk->_data = (npt_byte_t*)malloc(sizeof(npt_byte_t) * chunk->_size);
		memcpy(chunk->_data, buffer, chunk->_size);
		buffer += chunk->_size;

		/// CRC, 4 bytes
		chunk->_crc = ntohl(*((npt_uint32_t*)buffer));
		buffer += 4;

		*(chunks + i) = chunk;

		if (__idatchunk == chunk->_name)
			idats++;

		else if (__iendchunk == chunk->_name)
			break;
	}

	return idats;
}

npt_png_chunk**
npt_process_chunks(npt_png_chunk** chunks)
{
	npt_png_chunk** outt = (npt_png_chunk**)malloc(sizeof(npt_png_chunk*) * NPT_MAX_CHUNKS);

	/// Look for first idat chunk and copy previous chunks
	unsigned int i = 0, j = 0;
	for (i = 0; i < NPT_MAX_CHUNKS; i++)
	{
		npt_png_chunk* chunk = *(chunks + i);
		if (__idatchunk == chunk->_name)
			break;
		else
		{
			npt_png_chunk* chk = (npt_png_chunk*)malloc(sizeof(npt_png_chunk));
			chk->_size = chunk->_size;
			chk->_name = chunk->_name;
			chk->_data = malloc(sizeof(npt_byte_t) * chunk->_size);
			memcpy(chk->_data, chunk->_data, chunk->_size);
			chk->_crc = chunk->_crc;
			*(outt + i) = chk;
		}
	}
	/// Concatenate IDATs into a single buffer
	npt_byte_t* idats_orig = NULL;
	size_t tmpSize = 0;
	for (j = i; j < NPT_MAX_CHUNKS - i; j++)
	{
		npt_png_chunk* chunk = *(chunks + j);
		if (__idatchunk == chunk->_name)
		{
			tmpSize += chunk->_size;
			idats_orig = realloc(idats_orig, tmpSize);
			memcpy(&(idats_orig[tmpSize - chunk->_size]), chunk->_data, chunk->_size);
		}
		else if (__iendchunk == chunk->_name)
			break;
	}

	/// Process buffer
	const size_t ss = tmpSize * 16;
	npt_byte_t* idats_new = (npt_byte_t*)malloc(sizeof(npt_byte_t) * ss);
	z_stream infstrm, defstrm;
	infstrm.zalloc = Z_NULL;
	infstrm.zfree = Z_NULL;
	infstrm.opaque = Z_NULL;
	infstrm.avail_in = (uInt)tmpSize;
	infstrm.next_in = idats_orig;
	infstrm.avail_out = (uInt)ss;
	infstrm.next_out = idats_new;

	/// Inflate using raw inflation, outt leaking if failure
	if (inflateInit2(&infstrm, -8) != Z_OK)
	{
		NPT_DLOG("inflateInit2\n");
		free(idats_orig);
		free(idats_new);
		return NULL;
	}
	const int ret = inflate(&infstrm, Z_NO_FLUSH);
	if (ret < 0)
	{
		NPT_DLOG("inflate %d\n", ret);
		inflateEnd(&infstrm);
		free(idats_orig);
		free(idats_new);
		return NULL;
	}
	inflateEnd(&infstrm);
	free(idats_orig);

	/// Deflate again, the regular PNG-compatible way
	npt_byte_t* idats_final = (npt_byte_t*)malloc(ss);
	defstrm.zalloc = Z_NULL;
	defstrm.zfree = Z_NULL;
	defstrm.opaque = Z_NULL;
	defstrm.avail_in = (uInt)infstrm.total_out;
	defstrm.next_in = idats_new;
	defstrm.avail_out = (uInt)ss;
	defstrm.next_out = idats_final;
	
	deflateInit(&defstrm, Z_DEFAULT_COMPRESSION);
	deflate(&defstrm, Z_FINISH);

	free(idats_new);
	
	/// Now, re-create the IDATs
	int to_copy = (int)defstrm.total_out;
	int copied = 0;
	j = i;
	while (to_copy > 0)
	{
		const npt_uint32_t blk_size = (to_copy > NPT_MAX_IDAT_LENGTH) ? NPT_MAX_IDAT_LENGTH : (npt_uint32_t)to_copy;
		
		npt_png_chunk* chk = (npt_png_chunk*)malloc(sizeof(npt_png_chunk));
		chk->_size = blk_size;
		chk->_name = __idatchunk;
		chk->_data = malloc(chk->_size);
		memcpy(chk->_data, &(idats_final[copied]), chk->_size);
		chk->_crc = (npt_uint32_t)npt_crc((npt_byte_t[4]){0x49, 0x44, 0x41, 0x54}, chk->_data, chk->_size);
		*(outt + j) = chk;
		
		copied += blk_size;
		to_copy -= blk_size;

		j++;
	}
	free(idats_final);

	/// Append the last chunks
	for (i = j; i < NPT_MAX_CHUNKS - j; i++)
	{
		npt_png_chunk* chunk = *(chunks + i);
		npt_png_chunk* chk = (npt_png_chunk*)malloc(sizeof(npt_png_chunk));
		chk->_size = chunk->_size;
		chk->_name = chunk->_name;
		chk->_data = malloc(chunk->_size);
		memcpy(chk->_data, chunk->_data, chunk->_size);
		chk->_crc = chunk->_crc;
		*(outt + j) = chk;

		if (__iendchunk == chunk->_name)
			break;
	}

	return outt;
}

void
npt_process_chunks_simple(npt_png_chunk** chunks)
{
	for (unsigned int i = 0; i < NPT_MAX_CHUNKS; i++)
	{
		npt_png_chunk* chunk = *(chunks + i);
		
		if (__idatchunk == chunk->_name)
		{
			npt_byte_t* inflatedbuf = (npt_byte_t*)malloc(sizeof(npt_byte_t) * NPT_BUFSIZE);
			z_stream infstrm, defstrm;
			infstrm.zalloc = Z_NULL;
			infstrm.zfree = Z_NULL;
			infstrm.opaque = Z_NULL;
			infstrm.avail_in = chunk->_size;
			infstrm.next_in = chunk->_data;
			infstrm.avail_out = NPT_BUFSIZE;
			infstrm.next_out = inflatedbuf;
			
			/// Inflate using raw inflation
			if (inflateInit2(&infstrm, -8) != Z_OK)
			{
				NPT_DLOG("inflateInit2\n");
				free(inflatedbuf);
				return;
			}
			
			const int ret = inflate(&infstrm, Z_NO_FLUSH);
			if (ret < 0)
			{
				NPT_DLOG("inflate %d\n", ret);
				inflateEnd(&infstrm);
				free(inflatedbuf);
				return;
			}
			inflateEnd(&infstrm);
			
			/// Deflate again, the regular PNG-compatible way
			npt_byte_t* deflatedbuf = (npt_byte_t*)malloc(sizeof(npt_byte_t) * NPT_BUFSIZE);
			defstrm.zalloc = Z_NULL;
			defstrm.zfree = Z_NULL;
			defstrm.opaque = Z_NULL;
			defstrm.avail_in = (uInt)infstrm.total_out;
			defstrm.next_in = inflatedbuf;
			defstrm.avail_out = NPT_BUFSIZE;
			defstrm.next_out = deflatedbuf;
			
			deflateInit(&defstrm, Z_DEFAULT_COMPRESSION);
			deflate(&defstrm, Z_FINISH);
	
			/// Update chunk
			free(chunk->_data);
			chunk->_size = (npt_uint32_t)defstrm.total_out;
			chunk->_data = malloc(sizeof(npt_byte_t) * chunk->_size);
			memcpy(chunk->_data, deflatedbuf, chunk->_size);
			chunk->_crc = (npt_uint32_t)npt_crc((npt_byte_t[4]){0x49, 0x44, 0x41, 0x54}, chunk->_data, chunk->_size);
			
			free(deflatedbuf);
			free(inflatedbuf);
			break;
		}
	}
}

npt_byte_t*
npt_create_decrypted_in_memory(npt_png_chunk** chunks, unsigned int* size)
{
	/// PNG header
	npt_byte_t* outt = malloc(8);
	memcpy(outt, &__pngheader, 8);
	unsigned int len = 8;

	/// Append PNG chunks, except CgBI
	for (unsigned int i = 0; i < NPT_MAX_CHUNKS; i++)
	{
		npt_png_chunk* chunk = *(chunks + i);

		if (__cgbichunk != chunk->_name)
		{
			const npt_uint32_t tmp = htonl(chunk->_size);
			chunk->_crc = htonl(chunk->_crc);

			/// Size
			len += 4;
			outt = realloc(outt, len);
			memcpy(&(outt[len - 4]), &tmp, 4);

			/// Name
			len += 4;
			outt = realloc(outt, len);
			memcpy(&(outt[len - 4]), &chunk->_name, 4);

			/// Data
			len += chunk->_size;
			outt = realloc(outt, len);
			memcpy(&(outt[len - chunk->_size]), chunk->_data, chunk->_size);

			/// CRC
			len += 4;
			outt = realloc(outt, len);
			memcpy(&(outt[len - 4]), &chunk->_crc, 4);
		}
		
		if (__iendchunk == chunk->_name)
			break;
	}
	*size = len;
	return outt;
}
