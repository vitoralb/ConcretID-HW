/*
 * util.h
 *
 *  Created on: Jun 18, 2013
 *      Author: afonso
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>
#include <stdint.h>

int strtoi(char * pt, char ** end, uint8_t base);
char base64_to_byte(uint8_t * out, char * in, uint16_t size);
char byte_to_base64(char * out, uint8_t * in, uint16_t size);

#define Valid_num(x) ( (~x) & (sizeof(x) == 1 ? 0xff : sizeof(x) == 2 ? 0xffffU : 0xffffffffUL ) )

#define Print_at_most(str, len) do { \
	static uint16_t _len; \
	static char * _str; \
	_str = str; _len = len; \
	if( _str ) while( *_str && _len ) putchar( *_str++ ), _len--; \
} while(0)


#define Min(a,b) ((a)<(b)?(a):(b))
#define Swap_uint8_t(a,b) do { \
	static uint8_t c; \
	c = a; a = b; b = c; \
} while(0)


#endif /* UTIL_H_ */
