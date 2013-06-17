/*
 *
 */
#ifndef TRANSMISSION_H_
#define TRANSMISSION_H_

#include <stdlib.h>
#include "package.h"
#include "contiki.h"
#include "contiki-net.h"
#define CONN_PORT 0x0707

typedef struct uid_table_s {
	uid_t uid;
	uid_t link;
	char name[NAME_SIZE];
	uint8_t update = 0;
} uid_table_t;

extern uid_t my_uid;

void init_transmission();
void process_package(package_t * package, uint16_t size, uid_t * src);
void send_package(package_t * package, uint16_t size, uid_t * end);
uid_table_t * find_table_by_name(const char * name);

#endif
