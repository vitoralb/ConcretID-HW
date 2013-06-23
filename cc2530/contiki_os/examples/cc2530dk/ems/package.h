/*
 *
 */
#ifndef PACKAGE_H_
#define PACKAGE_H_

#include <stdlib.h>
#include <string.h>
#include "settings.h"
#include "contiki-net.h"

typedef uip_ipaddr_t uid_t; // dependent

/*
 *  UID
 */

#define UID_SIZE sizeof(uid_t)
#define UID_CMP(a,b) (!memcmp(a,b,UID_SIZE))
#define PRINT_HEX(x) do { \
	putchar((x>>4) < 10 ? (x>>4)+'0' : (x>>4)+('A'-10)); \
	putchar((x&15) < 10 ? (x&15)+'0' : (x&15)+('A'-10)); \
} while(0)
#define UID_PRINT(uid) do { \
	static uint8_t _i, * _uid; \
	_uid = (uint8_t *) (uid); \
	for( _i = 0 ; _i < UID_SIZE ; ++_i ) { \
		PRINT_HEX(_uid[_i]); \
	} \
} while(0)
FUNCTION_PREFIX void uid_create(uid_t * uid, const char * str);

extern uid_t my_uid;

/*
 *  Package Heads
 */

typedef uint32_t rid_t;
#define RID_SIZE (sizeof(rid_t))

// Package flags
#define PACKAGE_INTERMEDIATE (1U<<7)
#define PACKAGE_IDENTIFIED (1U<<6)
#define PACKAGE_ACKLY (1U<<5)
#define PACKAGE_ENCRYPTED (1U<<4)

// Package types
#define MASK_TYPE 0xf // 15
#define PACKAGE_SEQUENCE 0
#define PACKAGE_MSG 1
#define PACKAGE_MSG_TO 2
#define PACKAGE_BROADCAST_MSG 3
#define PACKAGE_NAME_REQUEST 4
#define PACKAGE_UID_REQUEST 5
#define PACKAGE_NEIGHBOR_REQUEST 6
#define PACKAGE_ECHO 7
#define PACKAGE_REECHO 8
#define PACKAGE_ANNOUNCEMENT 9
#define PACKAGE_ACK 10
#define PACKAGE_BROADCAST_ACK 11
#define PACKAGE_NACK 12
#define PACKAGE_COMMAND 13
#define PACKAGE_BASE64 14
#define PACKAGE_FIRST 15

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
	union {
		uint8_t flags;
		uint8_t rem;
	};
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

FUNCTION_PREFIX void make_package_head(package_t * package, uint8_t flags, uint8_t type);
FUNCTION_PREFIX void init_package();

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

FUNCTION_PREFIX uint8_t exists_rid(const rid_t * rid);
FUNCTION_PREFIX void register_rid(const rid_t * rid);

#endif
