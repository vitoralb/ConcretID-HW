/*
 *
 */

#include "package.h"

#define RID_BLOCKS 8 // power of 2
#define RID_QTY 16 // power of 2
#define RID_ROOM 8
rid_t rid_table[RID_BLOCKS][RID_QTY];
rid_t last_rid[RID_BLOCKS];
uid_t my_uid;

FUNCTION_PREFIX void uid_create(uid_t * uid, const char * str) {
	static uint8_t i, * _uid;
	_uid = (uint8_t *) uid;
	for( i = 0 ; i < 2*UID_SIZE ; ++i ) {
		if( '0' <= str[i] && str[i] <= '9' ) *_uid = str[i] - '0' << 4;
		else if( 'A' <= str[i] && str[i] <= 'F' ) *_uid = str[i] - 'A' + 10 << 4;
		else if( 'a' <= str[i] && str[i] <= 'f' ) *_uid = str[i] - 'a' + 10 << 4;
		else return ;
		++i;
		if( '0' <= str[i] && str[i] <= '9' ) *_uid |= str[i] - '0';
		else if( 'A' <= str[i] && str[i] <= 'F' ) *_uid |= str[i] - 'A' + 10;
		else if( 'a' <= str[i] && str[i] <= 'f' ) *_uid |= str[i] - 'a' + 10;
		else return ;
		_uid++;
	}
}
static void create_rid(rid_t * rid) {
	static uint8_t i, * _rid;
	_rid = (int8_t *) rid;
	for( i = 0 ; i < sizeof(rid_t) ; ++i ) {
		_rid[i] = (uint8_t)random_rand();
	}
}

FUNCTION_PREFIX uint8_t exists_rid(const rid_t * rid) {
	static uint8_t i, block, ret;
	block = RID_HASH(*rid);
	ret = 0;
	for( i = 0 ; i < RID_QTY ; ++i ) {
		if( (*rid & RID_FIXED) == (rid_table[block][i] & RID_FIXED) ) {
			if( *rid == rid_table[block][i] ) return 1;
			ret = 2;
		}
	}
	return ret;
}
FUNCTION_PREFIX void register_rid(const rid_t * rid){
	static uint8_t block, i;
	block = RID_HASH(*rid);
	for( i = 0 ; i < RID_ROOM ; ++i ) {
		if( *rid == rid_table[block][last_rid[block]-i & RID_QTY-1] ) {
			return ;
		}
	}
	last_rid[block] = last_rid[block]+1 & RID_QTY-1;
	memcpy(&rid_table[block][last_rid[block]], rid, sizeof(rid_t));
}

FUNCTION_PREFIX void make_package_head(package_t * package, uint8_t flags, uint8_t type) {
	create_rid(&package->rid);
	package->flags = 0;
	package->type = 0;
	package->flags |= flags;
	package->type |= type;
	if( flags & PACKAGE_IDENTIFIED ) {
		static identified_head_t * ident;
		ident = (identified_head_t *) package->data;
		memcpy(&ident->src, &my_uid, UID_SIZE);
	}
}

FUNCTION_PREFIX void init_package() {
	// empty
}
