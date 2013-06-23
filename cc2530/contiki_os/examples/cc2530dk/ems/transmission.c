/*
 *
 */

#include "contiki-net.h"
#include "settings.h"
#include "package.h"
#include "transmission.h"
#include "messages.h"
#include "util.h"

uid_table_t uid_table[UID_TABLE_SIZE];
uint8_t table_order[UID_TABLE_SIZE], last_table;
#define NEIGHBORHOOD_HEAD_SIZE 8

// ---------------- DEPENDENT -------------------
static void get_self_uid(uid_t * uid) {
	static uint8_t i, state;
	for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
		state = uip_ds6_if.addr_list[i].state;
		if(uip_ds6_if.addr_list[i].isused && (state == ADDR_TENTATIVE || state
				== ADDR_PREFERRED)) {
			printf("One of: ");
			UID_PRINT(&uip_ds6_if.addr_list[i].ipaddr);
			printf("\n");
			memcpy(uid, &uip_ds6_if.addr_list[i].ipaddr, sizeof(uip_ipaddr_t));
			if(state == ADDR_TENTATIVE) {
				uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
			}
		}
	}
}

extern struct uip_udp_conn * conn;
uid_t broadcast_uid, null_uid;

#define Send_package(package, size, end) do {\
	/*register_rid(&package->rid);*/ \
	memcpy(&conn->ripaddr, end, UID_SIZE); \
	conn->rport = CONN_PORT; \
	uip_udp_packet_send(conn, package, size); \
	uip_create_unspecified(&conn->ripaddr); \
	conn->rport = 0; \
} while(0)


// ------------------------------------------------

FUNCTION_PREFIX void send_package(package_t * package, uint16_t size, uid_t * end) { // DEPENDENT
	Send_package(package, size, end);
}

uint8_t neighbor_head[NEIGHBORHOOD_HEAD_SIZE+1];
uid_t neighbor[NEIGHBORHOOD_SIZE];
uint8_t neighbor_prev[NEIGHBORHOOD_SIZE];
uint8_t neighbor_next[NEIGHBORHOOD_SIZE];
uint8_t neighbor_time[NEIGHBORHOOD_SIZE];
#define Neighbor_head_trash neighbor_head[NEIGHBORHOOD_HEAD_SIZE]

#define UID_LAST_BYTE(uid) (((uint8_t*)(uid))[UID_SIZE-1])
#define Correct_table_order() do { \
	static uint8_t i, j; \
	for( i = 0, j = 0 ; i < last_table ; ++i ) { \
		if( Valid_num(uid_table[table_order[i]].link) ) { \
			if( i != j ) Swap_uint8_t(table_order[i], table_order[j]); \
			j++; \
		} \
	}\
	last_table = j; \
} while(0)


#define Del_neighbor(x) do { \
	static uint8_t i;\
	if( Valid_num(neighbor_prev[(x)]) ) { \
		neighbor_next[neighbor_prev[(x)]] = neighbor_next[(x)]; \
	} else { \
		neighbor_head[UID_LAST_BYTE(neighbor[(x)])%NEIGHBORHOOD_HEAD_SIZE] = neighbor_next[(x)]; \
	} \
	if( Valid_num(neighbor_next[(x)]) ) { \
		neighbor_prev[neighbor_next[(x)]] = neighbor_prev[(x)]; \
	} \
	for( i = 0 ; i < last_table ; ++i ) { \
		if( uid_table[table_order[i]].link == (x) ) { \
			uid_table[table_order[i]].link = -1;\
		}\
	}\
	Correct_table_order(); \
	neighbor_next[(x)] = Neighbor_head_trash; \
	neighbor_time[(x)] = -1; \
	Neighbor_head_trash = (x); \
} while(0);

static uint8_t new_neighbor(uid_t * uid) {
	static uint8_t ret, i;
	if( !Valid_num(Neighbor_head_trash) ) {
		ret = 0;
		for( i = 1 ; i < NEIGHBORHOOD_SIZE ; ++i ) {
			if( neighbor_time[i] > neighbor_time[ret] ) ret = i;
		}
		Del_neighbor(ret);
	}
	ret = Neighbor_head_trash;
	Neighbor_head_trash = neighbor_next[ret];
	i = UID_LAST_BYTE(uid)%NEIGHBORHOOD_HEAD_SIZE;
	if( Valid_num(neighbor_head[i]) ) {
		neighbor_prev[neighbor_head[i]] = ret;
	}
	neighbor_next[ret] = neighbor_head[i];
	neighbor_head[i] = ret;
	neighbor_time[ret] = 0;
	memcpy(neighbor+ret, uid, UID_SIZE);
	return ret;
}

FUNCTION_PREFIX void register_neighbor(uid_t * uid) {
	static uint8_t hh, loc;
	printf("Registering neighbor ");
	UID_PRINT(uid);
	loc = find_neighbor(uid);
	if( Valid_num(loc) ) {
		neighbor_time[loc] = 0;
	} else {
		loc = new_neighbor(uid);
	}
	printf(" in %hhu\n", loc);
}

FUNCTION_PREFIX uint8_t find_neighbor(uid_t * uid) {
	static uint8_t i;
	i = UID_LAST_BYTE(uid)%NEIGHBORHOOD_HEAD_SIZE;
	for( i = neighbor_head[i] ; Valid_num(i) ; i = neighbor_next[i] ) {
		if( UID_CMP(neighbor+i, uid) ) return i;
	}
	return -1;
}

FUNCTION_PREFIX void init_transmission() {
	static uint8_t i;
	get_self_uid(&my_uid);
	printf("My uid: ");
	UID_PRINT(&my_uid);
	printf("\n");
	uip_create_linklocal_allnodes_mcast(&broadcast_uid); // dependent
	memset(&null_uid, 0, sizeof(null_uid));
	memset(neighbor_time, -1, sizeof(neighbor_time));
	memset(neighbor_head, -1, sizeof(neighbor_head));
	memset(neighbor, 0, sizeof(neighbor));
	Neighbor_head_trash = 0;
	for( i = 1 ; i < NEIGHBORHOOD_SIZE ; ++i ) neighbor_next[i-1] = i;
	neighbor_next[NEIGHBORHOOD_SIZE-1] = -1;
	last_table = 0;
	for( i = 0 ; i < UID_TABLE_SIZE ; ++i ) table_order[i] = i;
}

FUNCTION_PREFIX void check_transmission() {
	static uint8_t i, count = 0;
	for( i = 0 ; i < NEIGHBORHOOD_SIZE ; ++i ) if( Valid_num(neighbor_time[i]) ){
		neighbor_time[i]++;
		if( neighbor_time[i] > me.neighbor_good_till ) {
			send_neighboor_request_to(neighbor+i);
		} else if( neighbor_time[i] > 2*me.neighbor_good_till ) {
			Del_neighbor(i);
		}
	}
	if( count++ == me.neighbor_good_till/2 ) {
		count = 0;
		send_neighboor_request_to_all();
	}
}

FUNCTION_PREFIX uid_table_t * find_table_by_name(const char * name) {
	static uint8_t i;
	for( i = 0 ; i < last_table ; ++i ) {
		if( !strncmp(uid_table[table_order[i]].name, name, NAME_SIZE) ) return uid_table+table_order[i];
	}
	return NULL;
}
FUNCTION_PREFIX uid_table_t * find_table_by_uid(const uid_t * uid) {
	static uint8_t i;
	for( i = 0 ; i < last_table ; ++i ) {
		if( UID_CMP(&uid_table[table_order[i]].uid, uid) ) return uid_table+table_order[i];
	}
	return NULL;
}
FUNCTION_PREFIX void promote_uid_table(const uid_table_t * uidt){
	static uint8_t x;
	x = uidt - uid_table;
	while( x-- ) {
		Swap_uint8_t(table_order[x], table_order[x+1]);
	}
}

FUNCTION_PREFIX uint16_t send_package_by_table(package_t * package, uint16_t size, const uid_table_t * uidt) {
	static uint16_t i;
	static intermediate_head_t * interm;
	if( UID_CMP(&uidt->uid, &uidt->link) ) {
		printf("Sending directly\n");
		if( package->flags&PACKAGE_INTERMEDIATE ) {
			interm = (intermediate_head_t *)package->data;
			size -= sizeof(intermediate_head_t) + sizeof(package_t);
			for( i = 0 ; i < size ; ++i ) {
				package->data[i] = interm->data[i];
			}
			package->flags &= ~PACKAGE_INTERMEDIATE;
			size += sizeof(package_t);
		}
	} else { // uid != link
		printf("Sending by mesh\n");
		if( ~package->flags&PACKAGE_INTERMEDIATE ) {
			printf("Making mesh\n");
			interm = (intermediate_head_t *)package->data;
			size -= sizeof(package_t);
			for( i = size-1 ; Valid_num(i)  ; --i ) {
				interm->data[i] = package->data[i];
			}
			memcpy(&interm->end, &uidt->uid, UID_SIZE);
			package->flags |= PACKAGE_INTERMEDIATE;
			size += sizeof(package_t) + sizeof(intermediate_head_t);
		}
	}
	i = uidt - uid_table;
	if( i != 0 ) Swap_uint8_t(table_order[i], table_order[i-1]);
	Send_package(package, size, &uidt->link);
	return size;
}

FUNCTION_PREFIX void register_uid(const uid_t * uid, const uint8_t link, const char * name) {
	static uid_table_t * uidt;
	printf("Registering uid: ");
	UID_PRINT(uid);
	printf(" by %hhu ", link);
	UID_PRINT(&neighbor[link]);
	printf(" as ");
	Print_at_most(name, NAME_SIZE);
	puts("");

	uidt = find_table_by_uid(uid);
	if( !uidt ) {
		if( last_table == UID_TABLE_SIZE ) last_table--;
		uidt = uid_table + table_order[last_table++];
		memcpy(&uidt->uid, uid, UID_SIZE);
		if( !name ) uidt->name[0] = 0;
	}
	uidt->link = link;
	if( name ) strncpy(uidt->name, name, NAME_SIZE);
}

