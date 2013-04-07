/*
 * Copyright (c) 2010, Loughborough University - Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *         Header file for the cc2530 AES encrypton
 *         based on the document DN108, using CBC as default (optimal MAC)
 *         Without DMA support
 *
 * \author
 *         Luiz Afonso
 */

#ifndef __AES_H__
#define __AES_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "8051def.h"
#include "cc253x.h"

#define AES_KEY_SIZE 16
#define AES_BLOCK_SIZE 16

/*
 * Caution: According to a sample code of TI (swrc135b - CC2530-Software Examples),
 * "Counter, Output Feedback and Cipher Feedback operates on 4 bytes and not 16 bytes."
 * aes_encrypt and aes_decrypt don't consider this
 */

#define AES_CBC_MODE (0<<4)
#define AES_CFB_MODE (1<<4)
#define AES_OFB_MODE (2<<4)
#define AES_CTR_MODE (3<<4)
#define AES_ECB_MODE (4<<4) /* Not recommended */
#define AES_CBCMAC_MODE (5<<4) /* Don`t use */

#define AES_READY (1<<3)

#define AES_CMD_ENCRYPT (0<<1)
#define AES_CMD_DECRYPT (1<<1)
#define AES_CMD_LOAD_KEY (2<<1)
#define AES_CMD_LOAD_IV (3<<1)

#define AES_START 1


#ifdef AES_CONF_ALWAYS_SET_KEY
#define  AES_ALWAYS_SET_KEY AES_CONF_ALWAYS_SET_KEY
#else
#define  AES_ALWAYS_SET_KEY 0
#endif

#ifdef AES_CONF_DEFAULT_MODE
#define AES_DEFAULT_MODE AES_CONF_DEFAULT_MODE
#else
#define AES_DEFAULT_MODE AES_CBC_MODE
#endif

#ifdef AES_CONF_USE_MAC
#define AES_USE_MAC AES_CONF_USE_MAC
#else
#define AES_USE_MAC 1
#endif

/**
 * \param new_key 	If null, it will be used a default_key
 *
 */
void aes_set_key(const uint8_t * key);

/**
 * \param output 	Can be the same as data. Make sure there is enough
 * 						space in it: size+47 in the worst case, more precisely:
 * 						ceil(size/AES_BLOCK_SIZE + AES_USE_MAC)*AES_BLOCK_SIZE
 * \retval			The size of the data encrypted in output
 *
 */
uint16_t aes_encrypt(uint8_t * output, const uint8_t * data, uint16_t size);

/**
 * \param output 	Can be the same as data.
 * \retval			The size of the data decrypted in output,
 * 						or -1 (0xffff) if there is a MAC and the key is wrong
 *
 */
uint16_t aes_decrypt(uint8_t * output, const uint8_t * data, uint16_t size);

#endif /* __AES_H__ */

