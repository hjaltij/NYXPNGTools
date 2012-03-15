//
//  flip_channels.h
//  NYXPNGTools
//
//  Created by Nyx0uf on 03/02/12.
//  Copyright (c) 2012 Benjamin Godard. All rights reserved.
//  www.cocoaintheshell.com
//


#ifndef __NYXPNGTOOLS_FLIP_CHANNELS_H__
#define __NYXPNGTOOLS_FLIP_CHANNELS_H__

#include "internal.h"


/*!
 *	@function npt_flip_channels
 *	@abstract Flip the RGB channels
 *	@param buffer [in] : PNG data
 *  @param size [out] : Size of the returned buffer
 *	@return PNG buffer with flipped channels
 */
npt_byte_t* npt_flip_channels(npt_byte_t* buffer, npt_uint32_t* size);

#endif /* __NYXPNGTOOLS_FLIP_CHANNELS_H__ */
