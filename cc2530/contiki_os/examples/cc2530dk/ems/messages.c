/*
 * messages.c
 *
 *  Created on: Jun 17, 2013
 *      Author: afonso
 */

#include "settings.h"
#include "package.h"
#include "transmission.h"
#include "messages.h"
#include "sendout.h"
#include "interface.h"
#include "util.h"
#include <stdio.h>

#define BLOCKED_LIST_SIZE 3
#define BUFFER_SIZE \
	(sizeof(package_t) + sizeof(intermediate_head_t) + sizeof(identified_head_t) + sizeof(encrypted_head_t) \
	+ UID_SIZE + NAME_SIZE + RID_SIZE + 1)

uint8_t messages_buffer[BUFFER_SIZE];

FUNCTION_PREFIX void send_ack(package_t * pkg){
	static package_t * package;
	static uid_table_t * uidt;
	if( pkg->flags & PACKAGE_IDENTIFIED ) {
		if( uidt = find_table_by_uid(&((identified_head_t *)package->data)->src) ) {
			package = (package_t *) messages_buffer;
			make_package_head(package, 0, PACKAGE_ACK);
			memcpy(package->data, &pkg->rid, RID_SIZE);
			send_package_by_table(package, sizeof(package_t)+RID_SIZE, uidt);
		}
	} else {
		package = (package_t *) messages_buffer;
		make_package_head(package, 0, PACKAGE_BROADCAST_ACK);
		memcpy(package->data, &pkg->rid, RID_SIZE);
		send_package(package, sizeof(package_t)+RID_SIZE, &broadcast_uid);
	}
}

FUNCTION_PREFIX void send_nack(uid_t * uid, rid_t * rid){
	static package_t * package;
	static uid_table_t * uidt;
	if( uidt = find_table_by_uid(uid) ) {
		package = (package_t *) messages_buffer;
		make_package_head(package, 0, PACKAGE_NACK);
		memcpy(package->data, rid, RID_SIZE);
		send_package_by_table(package, sizeof(package_t)+RID_SIZE, uidt);
	}
}

FUNCTION_PREFIX void announce_me_to(uid_t * uid) {
	static package_t * package;
	static uid_table_t * uidt;
	uidt = find_table_by_uid(uid);
	printf("announcing me?\n");
	if( !uidt ) return ; // error ?
	package = (package_t *) messages_buffer;
	make_package_head(package, PACKAGE_IDENTIFIED, PACKAGE_ANNOUNCEMENT);
	strncpy(((identified_head_t *)package->data)->data, me.name, NAME_SIZE);
	send_package_by_table(package, sizeof(package_t)+sizeof(identified_head_t)+NAME_SIZE, uidt);
}

static void send_echo_to(uid_t * uid){
	static package_t * package;
	package = (package_t *) messages_buffer;
	make_package_head(package, 0, PACKAGE_ECHO);
	send_package(package, sizeof(package_t), uid);
}

static void send_reecho_to(uid_t * uid){
	static package_t * package;
	package = (package_t *) messages_buffer;
	make_package_head(package, 0, PACKAGE_REECHO);
	send_package(package,sizeof(package_t), uid);
}

FUNCTION_PREFIX void send_uid_request(uid_t * uid){
	static package_t * package;
	package = (package_t *) messages_buffer;
	make_package_head(package, PACKAGE_IDENTIFIED, PACKAGE_UID_REQUEST);
	memcpy(((identified_head_t *)package->data)->data, uid, UID_SIZE);
	send_package(package, sizeof(package_t)+sizeof(identified_head_t)+UID_SIZE, &broadcast_uid);
}

FUNCTION_PREFIX void send_name_request(const char * name){
	static package_t * package;
	package = (package_t *) messages_buffer;
	make_package_head(package, PACKAGE_IDENTIFIED, PACKAGE_NAME_REQUEST);
	strncpy(((identified_head_t *)package->data)->data, name, NAME_SIZE);
	send_package(package, sizeof(package_t)+sizeof(identified_head_t)+NAME_SIZE, &broadcast_uid);
}

FUNCTION_PREFIX void discovery_neighborhood(uint8_t depth){
	static package_t * package;
	package = (package_t *) messages_buffer;
	make_package_head(package, PACKAGE_IDENTIFIED, PACKAGE_NEIGHBOR_REQUEST);
	((identified_head_t *)package->data)->data[0] = depth;
	send_package(package, sizeof(package_t)+sizeof(identified_head_t)+1, &broadcast_uid);
}

FUNCTION_PREFIX void send_neighboor_request_to(uid_t * uid){
	static package_t * package;
	package = (package_t *) messages_buffer;
	make_package_head(package, 0, PACKAGE_NEIGHBOR_REQUEST);
	send_package(package, sizeof(package_t), uid);
}

FUNCTION_PREFIX void send_neighboor_request_to_all(){
	static package_t * package;
	package = (package_t *) messages_buffer;
	make_package_head(package, 0, PACKAGE_NEIGHBOR_REQUEST);
	send_package(package, sizeof(package_t), &broadcast_uid);
}

FUNCTION_PREFIX void send_broadcast_msg(char * msg){
	static package_t * package;
	package = (package_t *) messages_buffer;
	make_package_head(package, 0, PACKAGE_BROADCAST_MSG);
	strcpy(package->data, msg);
	send_package(package, sizeof(package_t) + strlen(msg), &broadcast_uid);
}



uid_t blocked_list[BLOCKED_LIST_SIZE];
uint8_t last_blocked = 0;

FUNCTION_PREFIX void block_uid(uid_t * uid) {
	memcpy(blocked_list+last_blocked, uid, UID_SIZE);
	last_blocked = (last_blocked+1)%BLOCKED_LIST_SIZE;
}

FUNCTION_PREFIX uint8_t is_blocked(uid_t * uid) {
	static uint8_t i;
	for( i = 0 ; i < BLOCKED_LIST_SIZE ; ++i ) {
		if( UID_CMP(uid, blocked_list+i) ) return 1;
	}
	return 0;
}

char lala[10];

FUNCTION_PREFIX void process_package(package_t * package, uint16_t size, uid_t * src) {
	static uint8_t * data;
	static uint8_t e_rid, src_as_neighbor;
	static uint16_t len; // size without heads
	static identified_head_t * ident;
	if( size < sizeof(package_t) ) return; // invalid
	if( is_blocked(src) ) return;
	e_rid = exists_rid(&package->rid);
	if( e_rid == 1 ) return ; // received
	src_as_neighbor = find_neighbor(src);

	if( size < sizeof(package_t) ) return; // invalid
	len = size - sizeof(package_t);
	sprintf(lala,"%Pac: hhx %hhx", package->type&MASK_TYPE,package->flags&~MASK_TYPE);
	if( (package->type&MASK_TYPE) != PACKAGE_BROADCAST_MSG ) send_broadcast_msg(lala);
	if( package->flags&PACKAGE_INTERMEDIATE ) {
		static intermediate_head_t * interm;
		static uid_table_t * uidt;
		printf("Meshing...\n");
		send_broadcast_msg("Oi!");
		if( size < sizeof(package_t) + sizeof(intermediate_head_t) ) return; // invalid
		register_rid(&package->rid);

		interm = (intermediate_head_t *)package->data;
		if( uidt = find_table_by_uid(&interm->end) ) {
			if( (package->type&MASK_TYPE) == PACKAGE_ANNOUNCEMENT && package->flags&PACKAGE_IDENTIFIED ) {
				promote_uid_table(uidt);
				ident = (identified_head_t *)interm->data;
				if( uidt = find_table_by_uid(&ident->src) ) {
					promote_uid_table(uidt);
				} else {
					// broken path ?
				}
			}
			send_broadcast_msg("Foi?");
			send_package_by_table(package, size, uidt);
		} else {
			/*
			if( package->flags&PACKAGE_ACKLY ) {
				if( package->flags&PACKAGE_IDENTIFIED ) {
					static identified_head_t * ident;
					ident = (identified_head_t *)interm->data;
					send_nack(&interm->src, &package->rid); // ?
				}
				send_uid_request(&interm->end); // ?
			}
			*/
		}
		return;
	}

	if( package->flags&PACKAGE_IDENTIFIED ) {
		if( len < sizeof(identified_head_t) ) return; // invalid
		ident = (identified_head_t *)package->data;
		len -= sizeof(identified_head_t);
		data = ident->data;
	} else {
		ident = NULL;
		data = package->data;
	}

	if( e_rid == 2 ) {
		register_rid(&package->rid);
		if( package->flags&PACKAGE_ACKLY ) send_ack(package);
		return; // processed
	}

	if( Valid_num(src_as_neighbor) ) {
		register_rid(&package->rid);
	} else {
		switch( package->type&MASK_TYPE ){
		case PACKAGE_MSG_TO:
		case PACKAGE_BROADCAST_MSG:
		case PACKAGE_NEIGHBOR_REQUEST:
		case PACKAGE_ECHO:
		case PACKAGE_REECHO:
		case PACKAGE_BROADCAST_ACK:
			register_rid(&package->rid);
			break;
		case PACKAGE_NAME_REQUEST:
		case PACKAGE_UID_REQUEST:
		case PACKAGE_ANNOUNCEMENT:
			send_neighboor_request_to(src);
			return;
		case PACKAGE_SEQUENCE:
		case PACKAGE_FIRST:
		case PACKAGE_MSG:
		case PACKAGE_ACK:
		case PACKAGE_NACK:
		case PACKAGE_COMMAND:
		case PACKAGE_BASE64:
			send_neighboor_request_to(src);
			if( package->flags&PACKAGE_ACKLY ) {
				return ;
			} else {
				register_rid(&package->rid);
				break ;
			}
		default:
			return;
		}
	}

	if( package->flags&PACKAGE_ENCRYPTED ) {
		// TODO decrypt
	}

	printf("Processing %hhu...\n", (uint8_t)(package->type&MASK_TYPE));

	switch( package->type&MASK_TYPE ) {
	case PACKAGE_SEQUENCE: {
		if( package->flags&PACKAGE_ACKLY ) send_ack(package);
		// TODO
	} break;
	case PACKAGE_MSG:{
		if( package->flags&PACKAGE_ACKLY ) send_ack(package);
		if( ident ) {
			printf("new msg from ");
			UID_PRINT(&ident->src);
			printf(": ");
		} else {
			printf("new msg: ");
		}
		printf("%.*s", len, data);
	} break;
	case PACKAGE_MSG_TO:{
		if( len < UID_SIZE ) return ; // invalid
		if( UID_CMP(&my_uid, data) ) {
			if( package->flags&PACKAGE_ACKLY ) send_ack(package);
			if( ident ) {
				printf("new msg from ");
				UID_PRINT(&ident->src);
				printf(": ");
			} else {
				printf("new msg: ");
			}
			printf("%.*s", len-UID_SIZE, data+UID_SIZE);
		} else {
			send_package(package, size, &broadcast_uid);
		}
	} break;
	case PACKAGE_BROADCAST_MSG:{
		printf("broadcast msg: ");
		Print_at_most(data, len);
		send_package(package, size, &broadcast_uid);
	} break;
	case PACKAGE_NAME_REQUEST:{
		if( !len ) return ; // invalid
		if( ident ) {
			register_uid(&ident->src, src_as_neighbor, NULL);
		}
		data[len] = 0;
		if( !strncmp(data,me.name,NAME_SIZE) ) {
			if( ident ) {
				promote_uid_table( find_table_by_uid(&ident->src) );
				announce_me_to(&ident->src);
			} else {
				// invalid?
			}
		} else {
			send_package(package, size, &broadcast_uid);
		}
	} break;
	case PACKAGE_UID_REQUEST:{
		if( len != UID_SIZE ) return;
		if( ident ) {
			register_uid(&ident->src, src_as_neighbor, NULL);
		}
		if( UID_CMP(data,&my_uid) ) {
			if( ident ) {
				promote_uid_table( find_table_by_uid(&ident->src) );
				announce_me_to(&ident->src);
			} else {
				// invalid?
			}
		} else {
			send_package(package, size, &broadcast_uid);
		}
	} break;
	case PACKAGE_NEIGHBOR_REQUEST:{
		if( len == 1 ) {
			if( Valid_num(src_as_neighbor) ) {
				if( data[0]-- ) {
					send_package(package, size, &broadcast_uid);
				}
				if( ident ) {
					register_uid(&ident->src, src_as_neighbor, NULL);
					announce_me_to(&ident->src);
				} else {
					// invalid
				}
			} else {
				send_neighboor_request_to(src);
			}
		} else if( len == 0 ){
			send_echo_to(src);
		} else {
			// invalid
		}
	} break;
	case PACKAGE_ECHO:{
		register_neighbor(src);
		send_reecho_to(src);
	} break;
	case PACKAGE_REECHO:{
		register_neighbor(src);
	} break;
	case PACKAGE_ANNOUNCEMENT: {
		if( !ident ) return; // invalid
		if( len ) {
			data[len] = 0;
			register_uid(&ident->src, src_as_neighbor, data);
		} else {
			register_uid(&ident->src, src_as_neighbor, NULL);
		}
		promote_uid_table( find_table_by_uid(&ident->src) );
	} break;
	case PACKAGE_ACK:{
		if( len != sizeof(rid_t) ) return; // invalid
		acknowledge((rid_t *)data);
	} break;
	case PACKAGE_BROADCAST_ACK:{
		if( len != sizeof(rid_t) ) return; // invalid
		if( !acknowledge((rid_t *)data) ) {
			send_package(package, size, &broadcast_uid);
		}
	} break;
	case PACKAGE_NACK:{
		// TODO
	} break;
	case PACKAGE_COMMAND:{
		if( package->flags&PACKAGE_ACKLY ) send_ack(package);
		data[len] = 0;
		execute_cmd(data);
	} break;
	case PACKAGE_BASE64:{
		if( package->flags&PACKAGE_ACKLY ) send_ack(package);
		// TODO
	} break;
	default:{
		// invalid
	}
	}
}
