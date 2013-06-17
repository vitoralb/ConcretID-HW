/*
 *
 */

#include "transmission.h"

#define BUF_SIZE 92
#define UID_TABLE_SIZE 12
#define SENDOUT_SIZE 5
#define CONN_PORT 2570 // 0x0A0A

uid_t myUid;
uint8_t buf[BUF_SIZE];
uid_table_t uid_table[UID_TABLE_SIZE];
uint8_t last_update;

// ---------------- DEPENDENT -------------------
void getSelfUid(uid_t * uid) {
	static uint8_t i, state;
	for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
		state = uip_ds6_if.addr_list[i].state;
		if(uip_ds6_if.addr_list[i].isused && (state == ADDR_TENTATIVE || state
				== ADDR_PREFERRED)) {
			memcpy(uid, &uip_ds6_if.addr_list[i].ipaddr, sizeof(uip_ipaddr_t));
			if(state == ADDR_TENTATIVE) {
				uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
			}
		}
	}
}

extern struct uip_udp_conn * conn;
uid_t broadcast_uid;

#define Send_package(package, size, end) do {\
	register_rid(&package->rid); \
	memcpy(&conn->ripaddr, end, UID_SIZE); \
	conn->rport = CONN_PORT; \
	uip_udp_packet_send(conn, package, size); \
	uip_create_unspecified(&conn->ripaddr); \
	conn->rport = 0; \
} while(0)

void send_package(package_t * package, uint16_t size, uid_t * end) { // DEPENDENT
	Send_package(package, size, end);
}
// ------------------------------------------------


void init_transmission() {
	static uint8_t i;
	last_update = 0;
	for( i = 0 ; i < UID_TABLE_SIZE ; ++i ) uid_table[i].update = 0;
	getSelfUid(my_uid);
	uip_create_linklocal_allnodes_mcast(&broadcast_uid);
}

uid_table_t * find_table_by_name(const char * name) {
	static uint8_t i;
	for( i = 0 ; i < UID_TABLE_SIZE ; ++i ) {
		if( !strncmp(uid_table[i].name, name, NAME_SIZE) ) return uid_table+i;
	}
	return NULL;
}
uid_table_t * find_table_by_uid(const uid_t * uid) {
	static uint8_t i;
	for( i = 0 ; i < UID_TABLE_SIZE ; ++i ) {
		if( UID_CMP(&uid_table[j].uid, uid) ) return uid_table+j;
	}
	return NULL;
}
void register_uid(const uit_t * uid, const uid_t * link, const char * name) {
	static uid_table_t * uidt;
	static uint8_t i;
	uidt = findTableByIp(ip);
	if( !uidt ) {
		uidt = uid_table;
		for( i = 1 ; i < UID_TABLE_SIZE ; ++i ) {
			if( last_update - uid_table[i].update > last_update - uidt->update ) {
				uidt = uid_table+i;
			}
		}
		memcpy(&uidt->ipaddr, uid, UID_SIZE);
		if( !d ) ipt->name[0] = 0;
	}
	memcpy(&uidt->link, link, UID_SIZE);
	if( d ) strncpy(ipt->name, d, NAME_SIZE);
	uidt->update = last_update++;
}
void send_package_by_table(package_t * package, uint16_t size, const uid_table_t * uidt) {
	if( UID_CMP(&uidt->uid, &uidt->link) ) {
		if( package->flags&PACKAGE_INTERMEDIATE ) {
			static uint16_t i;
			static intermediate_head_t * interm;
			interm = (intermediate_head_t *)package->data;
			size -= sizeof(intermediate_head_t) + sizeof(package_t);
			for( i = 0 ; i < size ; ++i ) {
				package->data[i] = interm->data[i];
			}
			package->flags &= ~PACKAGE_INTERMEDIATE;
			size += sizeof(package_t);
		}
	} else { // uid != link
		if( ~package->flags&PACKAGE_INTERMEDIATE ) {
			static uint16_t i;
			static intermediate_head_t * interm;
			interm = (intermediate_head_t *)package->data;
			size -= sizeof(package_t);
			for( i = size-1 ; i >= 0 ; --i ) {
				interm->data[i] = package->data[i];
			}
			memcpy(&interm->end, uidt->uid, UID_SIZE);
			package->flags |= PACKAGE_INTERMEDIATE;
			size += sizeof(package_t) + sizeof(intermediate_head_t);
		}
	}
	Send_package(package, size, &uidt->link);
}


