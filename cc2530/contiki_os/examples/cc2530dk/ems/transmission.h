/*
 *
 */
#ifndef TRANSMISSION_H_
#define TRANSMISSION_H_

#include "settings.h"
#include "package.h"

typedef struct uid_table_s {
	uid_t uid;
	uint8_t link;
	char name[NAME_SIZE];
	//uint8_t vigor;
} uid_table_t;

#define UID_TABLE_SIZE 4
#define NEIGHBORHOOD_SIZE 16
extern uid_t neighbor[NEIGHBORHOOD_SIZE];

extern uid_table_t uid_table[UID_TABLE_SIZE];
extern uid_t my_uid;
extern uid_t broadcast_uid, null_uid;

extern uid_t neighbor[NEIGHBORHOOD_SIZE];

FUNCTION_PREFIX void register_neighbor(uid_t * uid);
FUNCTION_PREFIX uint8_t find_neighbor(uid_t * uid);

FUNCTION_PREFIX void init_transmission();
FUNCTION_PREFIX void check_transmission();

FUNCTION_PREFIX void send_package(package_t * package, uint16_t size, uid_t * end);
FUNCTION_PREFIX uint16_t send_package_by_table(package_t * package, uint16_t size, const uid_table_t * uidt);
FUNCTION_PREFIX uid_table_t * find_table_by_name(const char * name);
FUNCTION_PREFIX uid_table_t * find_table_by_uid(const uid_t * uid);
FUNCTION_PREFIX void register_uid(const uid_t * uid, const uint8_t link, const char * name);
FUNCTION_PREFIX void promote_uid_table(const uid_table_t * uidt);


#endif
