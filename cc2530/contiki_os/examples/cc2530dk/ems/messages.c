/*
 * messages.c
 *
 *  Created on: Jun 17, 2013
 *      Author: afonso
 */
#include <stdio.h>



void process_package(package_t * package, uint16_t size, uid_t * src) {
	static uint8_t * data;
	static uint8_t e_rid, src_is_neighbor;
	static uint16_t len; // size without heads
	static identified_head_t * ident;
	if( size < sizeof(package_t) ) return; // invalid
	e_rid = exists_rid(&package->rid);
	if( e_rid == 1 ) return ; // received
	src_is_neighbor = is_neighbor(src);
	if( src_is_neighbor ) register_rid(&package->rid);

	if( package->flags&PACKAGE_INTERMEDIATE ) {
		static intermediate_head_t * interm;
		static uid_table_t * uidt;
		if( size < sizeof(package_t) + sizeof(intermediate_head_t) ) return; // invalid

		interm = (intermediate_head_t *)package->data;
		if( uidt = find_table_by_uid(interm->end) ) {
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
	if( package->flags&ACKLY ) {
		if( package->flags&PACKAGE_IDENTIFIED ) {
			if( size < sizeof(package_t) + sizeof(identified_head_t) ) return; // invalid
			static identified_head_t * ident;
			ident = (identified_head_t *)interm->data;
			send_ack(&interm->src, &package->rid);
			data = interm->data;
		} else {
			send_broadcast_ack(&package->rid);
		}
	}
	if( e_rid == 2 ) return; // processed
	len = size - sizeof(package_t);
	if( package->flags&PACKAGE_IDENTIFIED ) {
		ident = (identified_head_t *)package->data;
		len -= sizeof(identified_head_t);
		data = ident->data;
	} else {
		ident = NULL;
		data = package->data;
	}
	if( package->flags&PACKAGE_ENCRYPTED ) ; // TODO decrypt

	if( len < 0 ) return; // invalid

	switch( package->type&MASK_TYPE ) {
	case PACKAGE_MSG:{
		if( ident ) {
			printf("from ");
			UID_PRINT(ident->src);
			printf(": ");
		}
		data[len] = 0;
		printf("%s", data);
	} break;
	case PACKAGE_NAME_REQUEST:{
		data[len] = 0;
		if( !strncmp(data,me.name,NAME_SIZE) ) {
			if( ident ) {
				announce_me_to(&ident->src);
			} else {
				announce_me();
			}
		} else {
			send_package(package, size, &broadcast_uid);
		}
	} break;
	case PACKAGE_IP_REQUEST:{
		if( len != UID_SIZE ) return;
		if( UID_CMP(data,my_uid) ) {
			if( ident ) {
				announce_me_to(&ident->src);
			} else {
				announce_me();
			}
		} else {
			send_package(package, size, &broadcast_uid);
		}
	} break;
	case PACKAGE_NEIGHBOR_REQUEST:{
		if( len == 1 ) {
			if( data[0]-- ) {
				send_package(package, size, &broadcast_uid);
			}
			if( ident ) {
				announce_me_to(&ident->src);
			} else {
				announce_me();
			}
		} else if( len == 0 ){
			if( ident ) {
				send_echo_to(&ident->src);
			} else {
				// invalid
			}
		} else {
			// invalid
		}
	} break;
	case PACKAGE_ANNOUNCEMENT:{
		if( len < UID_SIZE ) return; // invalid
		if( src_is_neighbor ) {
			if( len > UID_SIZE ) {
				data[len] = 0;
				register_uid(data, src, data+len);
			} else {
				register_uid(data, src, NULL);
			}
		}
		send_package(package, size, &broadcast_uid);
	} break;
	case PACKAGE_ECHO:{
		register_neighbor(src);
	} break;
	case PACKAGE_ACK:{
		if( len != sizeof(rid_t) ) return;
		acknowledge(data);
	} break;
	case PACKAGE_BROADCAST_ACK:{
		if( len != sizeof(rid_t) ) return;
		if( !acknowledge(data) ) {
			send_package(package, size, &broadcast_uid);
		}
	} break;
	case PACKAGE_NACK:{
		// TODO
	} break;
	case PACKAGE_COMMAND:{
		data[len] = 0;
		execute_cmd(data);
	} break;
	default:{
		// invalid
	}
	}
}
