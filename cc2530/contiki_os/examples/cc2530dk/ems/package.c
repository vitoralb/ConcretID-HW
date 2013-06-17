/*
 *
 */

#include "package.h"

void init_package() {
}

#define RID_BLOCKS 8 // power of 2
#define RID_QTY 16 // power of 2
#define RID_ROOM 8
rid_t ridTable[RID_BLOCKS][RID_QTY];
rid_t lastRid[RID_BLOCKS];

uint8_t exists_rid(const rid_t * rid) {
	static uint8_t i, block, ret;
	block = RID_HASH(*rid);
	ret = 0;
	for( i = 0 ; i < RID_QTY_PER_BLOCK ; ++i ) {
		if( (*rid & RID_FIXED) == (ridTable[block][i] & RID_FIXED) ) {
			if( *rid == ridTable[block][i] ) return 1;
			ret = 2;
		}

	}
	return ret;
}
void register_rid(const rid_t * rid){
	static uint8_t block, i;
	block = RID_HASH(*rid);
	for( i = 0 ; i < RID_ROOM ; ++i ) {
		if( *rid == ridTable[block][lastRid[block]-i&RID_QTY-1] ) {
			return ;
		}
	}
	lastRid[block] = lastRid[block]+1%RID_QTY-1;
	memcpy(&ridTable[block][lastRid[block]], rid, sizeof(rid_t));
}
void create_rid(rid_t * rid) {
	uint8_t j;
	for( j = 0 ; j < RID_SIZE ; ++j ) {
		rid |= (uint8_t)random_rand() << 0xff;
	}
}
