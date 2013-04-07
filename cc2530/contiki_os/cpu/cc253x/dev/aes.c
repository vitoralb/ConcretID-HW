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

uint8_t current_key[AES_KEY_SIZE] =
	{0xC0,0x4F,0x41,0x45,0xF4,0x27,0x77,0x4B,0xE9,0x7D,0xB6,0xE0,0xAC,0x31,0xD8,0x35};
uint8_t zero[AES_BLOCK_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

#define AES_WAIT() while( !(ENCCS & AES_READY) )
#define AES_DOWNLOAD(x,y) do{ \
	uint8_t _count; \
	for( _count = 0 ; _count < (y) ; ++_count ) ENCDI = (x)[_count]; \
} while(0)
#define AES_UPLOAD(x,y) do{ \
	uint8_t _count; \
	for( _count = 0 ; _count < (y) ; ++_count ) (x)[_count] = ENCDO; \
} while(0)


void aes_set_key(const uint8_t * key) {
	if( key ) memcpy(current_key, key, AES_KEY_SIZE);
#if !AES_ALWAYS_SET_KEY
	ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_KEY | AES_START;
	AES_DOWNLOAD(current_key, AES_KEY_SIZE);
	AES_WAIT();
#endif
}

uint16_t aes_encrypt(uint8_t * output, const uint8_t * data, uint16_t size) {
	static uint16_t i;
	static uint8_t *current_iv;
	watchdog_periodic();
	current_iv = output+(size+AES_BLOCK_SIZE-(size-1)%AES_BLOCK_SIZE-1);

#if AES_ALWAYS_SET_KEY
	ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_KEY | AES_START;
	AES_DOWNLOAD(current_key, AES_KEY_SIZE);
	AES_WAIT();
#endif

#if AES_USE_MAC
	// load MAC with IV 0
	ENCCS = AES_CBCMAC_MODE | AES_CMD_LOAD_IV | AES_START;
	AES_DOWNLOAD(zero, AES_BLOCK_SIZE);
	AES_WAIT();
	// load data
	ENCCS = AES_CBCMAC_MODE | AES_CMD_LOAD_IV;
	for( i = 0 ; i < size-AES_BLOCK_SIZE ; i += AES_BLOCK_SIZE ) {
		ENCCS |= AES_START;
		AES_DOWNLOAD(data+i,AES_BLOCK_SIZE);
		AES_WAIT();
	}
	ENCCS = AES_CBC_MODE | AES_CMD_LOAD_IV | AES_START;
	AES_DOWNLOAD(data+i,size-i);
	AES_DOWNLOAD(zero,AES_BLOCK_SIZE-(size-i));
	AES_WAIT();
	AES_UPLOAD(current_iv+AES_BLOCK_SIZE,AES_BLOCK_SIZE);
#endif

	// first byte of IV stores how many bytes is necessary to complete the last block
	current_iv[0] = (current_iv-output)-size;
	for( i = 1 ; i < AES_BLOCK_SIZE ; ++i ) current_iv[i] = random_rand();

	ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_IV | AES_START;
	AES_DOWNLOAD(current_iv, AES_BLOCK_SIZE);
	AES_WAIT();
	// load data
	ENCCS = AES_DEFAULT_MODE | AES_CMD_ENCRYPT;
	for( i = 0 ; i < size-AES_BLOCK_SIZE ; i += AES_BLOCK_SIZE ) {
		ENCCS |= AES_START;
		AES_DOWNLOAD(data+i,AES_BLOCK_SIZE);
		AES_WAIT();
		AES_UPLOAD(output+i,AES_BLOCK_SIZE);
	}
	// last block
	ENCCS |= AES_START;
	AES_DOWNLOAD(data+i,size-i);
	AES_DOWNLOAD(zero,AES_BLOCK_SIZE-(size-i));
	AES_WAIT();
	AES_UPLOAD(output+i,AES_BLOCK_SIZE);

	size += current_iv[0] + AES_BLOCK_SIZE;
#if AES_USE_MAC
	size += AES_BLOCK_SIZE;
#endif
	return size;
}

uint16_t aes_decrypt(uint8_t * output, const uint8_t * data, uint16_t size) {
	static uint16_t i;
	static uint8_t *current_iv;
	watchdog_periodic();

#if AES_ALWAYS_SET_KEY
	ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_KEY | AES_START;
	AES_DOWNLOAD(current_key, AES_KEY_SIZE);
	AES_WAIT();
#endif

#if AES_USE_MAC
	size -= 2*AES_BLOCK_SIZE;
#else
	size -= AES_BLOCK_SIZE;
#endif

	current_iv = data+size;
	ENCCS = AES_DEFAULT_MODE | AES_CMD_LOAD_IV | AES_START;
	AES_DOWNLOAD(current_iv, AES_BLOCK_SIZE);
	AES_WAIT();

	ENCCS = AES_DEFAULT_MODE | AES_CMD_DECRYPT;
	for( i = 0 ; i < size-AES_BLOCK_SIZE ; i += AES_BLOCK_SIZE ) {
		ENCCS |= AES_START;
		AES_DOWNLOAD(data+i,AES_BLOCK_SIZE);
		AES_WAIT();
		AES_UPLOAD(output+i,AES_BLOCK_SIZE);
	}
	// last block
	ENCCS |= AES_START;
	AES_DOWNLOAD(data+i,AES_BLOCK_SIZE);
	AES_WAIT();
	AES_UPLOAD(output+i,size-i);

#if AES_USE_MAC
	// load MAC with IV 0
	ENCCS = AES_CBCMAC_MODE | AES_CMD_LOAD_IV | AES_START;
	AES_DOWNLOAD(zero, AES_BLOCK_SIZE);
	AES_WAIT();
	// load data
	ENCCS = AES_CBCMAC_MODE | AES_CMD_LOAD_IV;
	for( i = 0 ; i < size-AES_BLOCK_SIZE ; i += AES_BLOCK_SIZE ) {
		ENCCS |= AES_START;
		AES_DOWNLOAD(output+i,AES_BLOCK_SIZE);
		AES_WAIT();
	}
	ENCCS = AES_CBC_MODE | AES_CMD_LOAD_IV | AES_START;
	AES_DOWNLOAD(output+i,size-i);
	AES_DOWNLOAD(zero,AES_BLOCK_SIZE-(size-i));
	AES_WAIT();
	for( i = 0 ; i < AES_BLOCK_SIZE ; ++i ) {
		uint8_t tmp = ENCDO;
		if( (current_iv+AES_BLOCK_SIZE)[i] != tmp ) return -1; // 0xffff
	}
#endif

	return size;
}
