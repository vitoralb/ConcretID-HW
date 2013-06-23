/*
 * util.c
 *
 *  Created on: Jun 18, 2013
 *      Author: afonso
 */

#include <stdint.h>
#include "util.h"

int strtoi(char * pt, char ** end_pt, uint8_t base) {
	static uint8_t neg;
	static int ret;
	while( *pt && *pt == ' ' ) pt++;
	if( *pt == '-' ) neg = 1, pt++;
	else if( *pt == '+' ) neg = 0, pt++;
	else neg = 0;
	if( base == 0 ) {
		if( pt[0] == '0' ) {
			if( pt[1] == 'x' || pt[1] == 'X' ) base = 16, pt += 2;
			else if( pt[1] == 'b' || pt[1] == 'B' ) base = 2, pt += 2;
			else base = 8, pt++;
		} else {
			base = 10;
		}
	}
	ret = 0;
	if( base > 10 ) {
		while( 1 ) {
			if( (uint8_t)(*pt-'0') < 10 ) ret = ret * base + *pt++ - '0';
			else if( (uint8_t)(*pt+(10-'a')) < base ) ret = ret * base + *pt++ + (10-'a');
			else if( (uint8_t)(*pt+(10-'A')) < base ) ret = ret * base + *pt++ + (10-'A');
			else break;
		}
	} else {
		while( (uint8_t)(*pt-'0') < base ) ret = ret * base + *pt++ - '0';
	}
	if( end_pt ) *end_pt = pt;
	return ret;
}
