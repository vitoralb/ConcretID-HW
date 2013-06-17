/*
 *
 */
#ifndef PACKAGE_H_
#define PACKAGE_H_

#include <stdlib.h>
#include <string.h>
#include "contiki-net.h"

typedef uip_ipaddr_t uid_t;
#define UID_SIZE sizeof(uid_t)
#define UID_CMP(a,b) (!memcmp(a,b,UID_SIZE))
#define PRINT_HEX(x) printf("%c%c", \
	(x>>4) < 10 ? (x>>4)+'0' : (x>>4)+('A'-10), \
	(x&15) < 10 ? (x&15)+'0' : (x&15)+('A'-10))
#define UID_PRINT(uid) do { \
	static uint8_t i, * _uid; \
	_uid = (uint8_t *) (uid); \
	for( int i = 0 ; i < UID_SIZE ; ++i ) { \
		PRINT_HEX(_uid[i]); \
	} \
while(0)

typedef uint32_t rid_t;

// Package flags
#define PACKAGE_INTERMEDIATE (1<<7)
#define PACKAGE_IDENTIFIED (1<<6)
#define PACKAGE_ACKLY (1<<5)
#define PACKAGE_ENCRYPTED (1<<4)

// Package types
#define MASK_TYPE 0xf // 15
#define PACKAGE_MSG 1
#define PACKAGE_NAME_REQUEST 2
#define PACKAGE_IP_REQUEST 3
#define PACKAGE_NEIGHBOR_REQUEST 4
#define PACKAGE_ANNOUNCEMENT 5
#define PACKAGE_ECHO 6
#define PACKAGE_ACK 7
#define PACKAGE_BROADCAST_ACK 8
#define PACKAGE_NACK 9
#define PACKAGE_COMMAND 10

// for PACKAGE_INTERMEDIATE
typedef struct intermediate_head_s {
	uid_t end;
	uint8_t data[0];
} intermediate_head_t;

// for PACKAGE_IDENTIFIED
typedef struct identified_head_s {
	uid_t src;
	uint8_t data[0];
} identified_head_t;

// for PACKAGE_ENCRYPTED
typedef struct encrypted_head_s {
	// TODO
	uint8_t data[0];
} encrypted_head_t;

// package main head
typedef struct package_s {
	rid_t rid;
	union {
		uint8_t flags;
		uint8_t type;
	};
	uint8_t data[0];
} package_t;

void init_package();

#define Package_expandible(package) \
	((package)->flags&PACKAGE_INTERMEDIATE ? 1 : sizeof(intermediate_head_s)+1)

/*
 *  Rand ID
 */


#define RID_FLEX 0xffUL
#define RID_FIXED (~RID_FLEX)
#define RID_HASH(rid) ((rid)>>8&(RID_BLOCKS-1))

#define RID_INC(rid) do{ \
	*(rid) = *(rid)&RID_FIXED | *(rid)+1&RID_FLEX; \
} while(0)
extern rid_t ridTable[];
void exists_rid(const rid_t * rid);
void register_rid(const rid_t * rid);
void create_rid(rid_t * rid);

#endif
