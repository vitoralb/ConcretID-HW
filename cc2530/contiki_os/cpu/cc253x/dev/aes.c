/**
 * \file
 *         cc2530 AES encrypton, based on the document DN108,
 *         using CBC as default (optimal MAC)
 *         Without DMA support
 *
 * \author
 *         Luiz Afonso
 *
 */

#include "dev/aes.h"
#include "8051def.h"
#include "cc253x.h"
#include <stdio.h>

const char base64_to_char[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define CHAR_TO_BASE64(x) \
	('A' <= x && x <= 'Z' ? x-'A' : \
	 'a' <= x && x <= 'z' ? x-('a'-26) : \
	 '0' <= x && x <= '9' ? x-('0'-52) : \
	 x == '+' || x == ' ' ? 62 : 63 )

static uint8_t current_key[AES_KEY_SIZE] =
	{0xC0,0x4F,0x41,0x45,0xF4,0x27,0x77,0x4B,0xE9,0x7D,0xB6,0xE0,0xAC,0x31,0xD8,0x35};

void aes_set_key_base64(const char * new_key){
	uint8_t i = 0, j = 0;
	for(;;) {
		current_key[i] = CHAR_TO_BASE64(new_key[j])<<2 | CHAR_TO_BASE64(new_key[j])>>2;
		++j;
		if( ++i == AES_KEY_SIZE ) break;
		current_key[i] = CHAR_TO_BASE64(new_key[j])<<4 | CHAR_TO_BASE64(new_key[j+1])>>2;
		++j;
		if( ++i == AES_KEY_SIZE ) break;
		current_key[i] = CHAR_TO_BASE64(new_key[j])<<6 | CHAR_TO_BASE64(new_key[j+1]);
		j += 2;
		if( ++i == AES_KEY_SIZE ) break;
	}
#if !AES_ALWAYS_SET_KEY
	// set Key (always needed?)
	ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_KEY | AES_START;
	for( i = 0 ; i < AES_KEY_SIZE ; ++i ) ENCDI = current_key[i];
	// wait key load
	while( !(ENCCS & AES_READY) );
#endif
}

void aes_set_key(const uint8_t * key) {
	uint8_t i;
	for( i = 0 ; i < AES_KEY_SIZE ; ++i ){
		current_key[i] = key[i];
	}
#if !AES_ALWAYS_SET_KEY
	// set Key (always needed?)
	ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_KEY | AES_START;
	for( i = 0 ; i < AES_KEY_SIZE ; ++i ) ENCDI = current_key[i];
	// wait key load
	while( !(ENCCS & AES_READY) );
#endif
}

void aes_init(char * new_key) {
	if( new_key ) aes_set_key_base64(new_key);
	else aes_set_key(current_key);
}

uint16_t aes_encrypt(uint8_t * output, const uint8_t * data, uint16_t size) {
	static uint16_t i;
	static uint8_t j;
	static uint8_t *current_iv;
	watchdog_periodic();

#if AES_ALWAYS_SET_KEY
	// set Key
	ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_KEY | AES_START;
	for( i = 0 ; i < AES_KEY_SIZE ; ++i ) ENCDI = current_key[i];
#endif

	// generate, set and load IV
	current_iv = output+((size-1)/AES_BLOCK_SIZE+1)*AES_BLOCK_SIZE;
	// first byte of IV stores how many bytes is necessary to complete the last block
	current_iv[0] = ((size-1)/AES_BLOCK_SIZE+1)*AES_BLOCK_SIZE-size;
	for( i = 1 ; i < AES_BLOCK_SIZE ; ++i ) current_iv[i] = random_rand();

#if AES_ALWAYS_SET_KEY
	// wait key load
	while( !(ENCCS & AES_READY) );
#endif

	ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_IV | AES_START;
	for( i = 0 ; i < AES_BLOCK_SIZE ; ++i ) ENCDI = current_iv[i];

	// wait IV load
	while( !(ENCCS & AES_READY) );

	// load data
	ENCCS = AES_DEFAULT_MODE | AES_CMD_ENCRYPT | AES_START;
	for( i = 0 ; i < size ; i += AES_BLOCK_SIZE ) {
		for( j = 0 ; j < AES_BLOCK_SIZE ; ++j ) {
			ENCDI = i+j<size?data[i+j]:0; // 0 can be anything here, rand...
		}
		for( j = 0 ; j < AES_BLOCK_SIZE ; ++j ) {
			output[i+j] = ENCDO;
		}
		// wait data load
		while( !(ENCCS & AES_READY) );
	}
	size += current_iv[0] + AES_BLOCK_SIZE;

	// load MAC
#if AES_USE_MAC
	for( i = 0 ; i < AES_BLOCK_SIZE ; ++i ) {
		ENCDI = 0;
	}
	for( i = 0 ; i < AES_BLOCK_SIZE ; ++i ) {
		output[size+i] = ENCDO;
	}
	size += AES_BLOCK_SIZE;
	// wait MAC load
	while( !(ENCCS & AES_READY) );
#endif
	return size;
}

uint16_t aes_decrypt(uint8_t * output, const uint8_t * data, uint16_t size) {
	static uint16_t i;
	static uint8_t j;
	static uint8_t *current_iv;
	watchdog_periodic();
#if AES_USE_MAC
	{
		static uint8_t *last_cipher, *mac;
		static uint8_t cipher[AES_BLOCK_SIZE];
		// the message can be empty
		if( size == 2*AES_BLOCK_SIZE ) last_cipher = data;
		else last_cipher = data+size-3*AES_BLOCK_SIZE;
		mac = data+size-AES_BLOCK_SIZE;

#if AES_ALWAYS_SET_KEY
		// set Key
		ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_KEY | AES_START;
		for( i = 0 ; i < AES_KEY_SIZE ; ++i ) ENCDI = current_key[i];
		// wait key load
		while( !(ENCCS & AES_READY) );
#endif

		// set IV
		ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_IV | AES_START;
		for( i = 0 ; i < AES_BLOCK_SIZE ; ++i ) ENCDI = last_cipher[i];
		// wait IV load
		while( !(ENCCS & AES_READY) );

		// set data
		ENCCS = AES_DEFAULT_MODE | AES_CMD_ENCRYPT | AES_START;
		for( i = 0 ; i < AES_BLOCK_SIZE ; ++i ) ENCDI = 0;
		for( i = 0 ; i < AES_BLOCK_SIZE ; ++i ) cipher[i] = ENCDO;
		// wait data load
		while( !(ENCCS & AES_READY) );

		for( i = 0 ; i < AES_BLOCK_SIZE ; ++i )
			if( cipher[i] != mac[i] ) return -1;
	}
#endif

#if AES_ALWAYS_SET_KEY
	// set Key (always needed?)
	ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_KEY | AES_START;
	for( i = 0 ; i < AES_KEY_SIZE ; ++i ) ENCDI = current_key[i];
	// wait key load
	while( !(ENCCS & AES_READY) );
#endif

	// get IV
#if AES_USE_MAC
	size -= 2*AES_BLOCK_SIZE;
#else
	size -= AES_BLOCK_SIZE;
#endif
	current_iv = data+size;
	ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_IV | AES_START;
	for( i = 0 ; i < AES_BLOCK_SIZE ; ++i ) ENCDI = current_iv[i];
	// wait IV load
	while( !(ENCCS & AES_READY) );

	// load data
	ENCCS = AES_DEFAULT_MODE | AES_CMD_DECRYPT | AES_START;
	size -= current_iv[0]%AES_BLOCK_SIZE;
	for( i = 0 ; i < size ; i += AES_BLOCK_SIZE ) {
		for( j = 0 ; j < AES_BLOCK_SIZE ; ++j ) {
			ENCDI = data[i+j];
		}
		for( j = 0 ; j < AES_BLOCK_SIZE && i+j < size ; ++j ) {
			output[i+j] = ENCDO;
		}
		// wait load data
		while( !(ENCCS & AES_READY) );
	}
	return size;
}
