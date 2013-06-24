/*
 * sendout.c
 *
 *  Created on: Jun 17, 2013
 *      Author: afonso
 */

#include "sendout.h"
#include "package.h"
#include "messages.h"
#include "transmission.h"
#include "util.h"

#define SENDOUT_BUFFER_SIZE 60
#define SENDOUT_QTY 2

typedef struct sendout_package_s {
	union {
		uid_t uid;
		char name[NAME_SIZE];
	};
	int8_t time;
	uint16_t size;
	union {
		package_t package;
		uint8_t buffer[SENDOUT_BUFFER_SIZE];
	};
} sendout_package_t;

sendout_package_t sendout_package[SENDOUT_QTY];


FUNCTION_PREFIX uint8_t acknowledge(rid_t * rid){
	static uint8_t i;
	for( i = 0 ; i < SENDOUT_QTY ; ++i ) if( !((*rid^sendout_package[i].package.rid)&RID_FIXED) ) {
		sendout_package[i].time = 0;
		return 1;
	}
	return 0;
}

FUNCTION_PREFIX void check_send_out() {
	static uint8_t i;
	static uid_table_t * uidt;
	for( i = 0 ; i < SENDOUT_QTY ; ++i ) {
		if( sendout_package[i].time < 0 ) {
			printf("%hhu not solved...\n", i);
			if( uidt = find_table_by_name(sendout_package[i].name) ) {
				memcpy(&sendout_package[i].uid, &uidt->uid, UID_SIZE);
				sendout_package[i].time = me.time_to_send*me.sending_attempts;
				sendout_package[i].size = send_package_by_table(&sendout_package[i].package, sendout_package[i].size, uidt);
			} else {
				sendout_package[i].time++;
				if( sendout_package[i].time%me.time_to_solve == 0 ) {
					send_name_request(sendout_package[i].name);
				}
			}
		} else if( sendout_package[i].time > 0 ) {
			printf("%hhu not acked...\n", i);
			sendout_package[i].time--;
			if( sendout_package[i].time%me.time_to_send == 0 ) {
				if( uidt = find_table_by_uid(&sendout_package[i].uid) ) {
					RID_INC(&sendout_package[i].package.rid);
					sendout_package[i].size = send_package_by_table(&sendout_package[i].package, sendout_package[i].size, uidt);
				} else {
					send_uid_request(&sendout_package[i].uid);
				}
			}
		}
	}
}

FUNCTION_PREFIX void init_send_out() {
	static uint8_t i;
	for( i= 0 ; i < SENDOUT_QTY ; ++i ) sendout_package[i].time = 0;
}

FUNCTION_PREFIX uint8_t send_out_by_name(char * name, uint8_t * data, uint8_t type, uint16_t len) {
	static uint8_t i;
	static uid_table_t * uidt;
	for( i = 0 ; i < SENDOUT_QTY ; ++i ) {
		if( sendout_package[i].time == 0 ) break;
	}
	if( i == SENDOUT_QTY ) return 0;

	sendout_package[i].size = len + sizeof(package_t) + sizeof(identified_head_t);
	if( SENDOUT_BUFFER_SIZE < sendout_package[i].size + sizeof(intermediate_head_t) ) return 0;
	make_package_head(&sendout_package[i].package, PACKAGE_ACKLY|PACKAGE_IDENTIFIED, type);
	memcpy(((identified_head_t *)sendout_package[i].package.data)->data, data, len);

	if( uidt = find_table_by_name(name) ) {
		memcpy(&sendout_package[i].uid, &uidt->uid, UID_SIZE);
		sendout_package[i].time = me.time_to_send*me.sending_attempts;
		sendout_package[i].size = send_package_by_table(&sendout_package[i].package, sendout_package[i].size, uidt);
	} else {
		strncpy(sendout_package[i].name, name, NAME_SIZE);
		sendout_package[i].time = -me.time_to_solve*me.solving_attempts;
		send_name_request(name);
	}
	return 1;
}
