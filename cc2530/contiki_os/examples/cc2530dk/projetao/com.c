#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include <string.h>
#include "dev/leds.h"
#include "dev/uart0.h"
#include "dev/button-sensor.h"
#include "net/uip-debug.h"

PROCESS(com_process, "Com process");
AUTOSTART_PROCESSES(&com_process);

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UID_T uip_ipaddr_t
#define UID_SIZE (sizeof(UID_T))

#define BUF_SIZE 92 // < 240 (UIP_BUFSIZE), > 92 eh descaratdo
#define NAME_SIZE 16
#define IPTABLE_SIZE 12
#define CMD_SIZE 140
#define SENDOUT_SIZE 5
#define RID_SIZE 4 // >= 2, CmpRid depends on
#define RID_BLOCKS 8
#define RID_QTY_PER_BLOCK 16
#define RID_QTY (RID_QTY_PER_BLOCK*RID_BLOCKS) // se aumentar muito, lembrar de trocar pro uint16_t nas funcoes
#define PACKAGE_HEAD (RID_SIZE+1)
#define CONN_PORT 2570 // 0x0A0A

#define SOLVE_TIME 1
#define SOLVE_TRIES 20
#define ACK_TIME 1
#define ACK_TRIES 20
#define ANNOUNCE_TIME 30

#define NAME_REQUEST_M 'R'
#define IP_REQUEST_M 'r'
#define ANNOUNCEMENT_M 'A'
#define ACKNOWLEDGE_M 'a'
#define PACKAGE_M 'P'
#define IDENTIFIED_PACKAGE_M 'I'
#define MESH_M 'M'
#define COMMAND_M 'C'
#define NEIGHBOR_REQUEST_M 'N'

#define MESH_HEAD_SIZE (UID_SIZE+1)

char buf[BUF_SIZE+UID_SIZE+MESH_HEAD_SIZE];
char myName[NAME_SIZE];
#define myIp (&myIpaddr) //(&uip_lladdr)
static uip_ipaddr_t myIpaddr;
struct uip_udp_conn *conn;
int32_t curTime = 0; // 120 anos
void puts0(char*, uint16_t);
#define uip_ipaddr_copy(a,b) memcpy(a,b,sizeof(uip_ipaddr_t))

UID_T blocked;

// --------------------- Rand ID ---------------------------
//#define CmpRid(a,b) ((a)[0]==(b)[0]&&(a)[1]==(b)[1]&&(a)[2]==(b)[2]&&(a)[3]==(b)[3])
//#define CmpRid(a,b) (memcmp(a,b,RID_SIZE) == 0)
#define CmpRid(a,b) ( *(uint32_t *)(a) == *(uint32_t *)(b) )
#define CmpRid1(a,b) ((a)[1]==(b)[1]&&(a)[2]==(b)[2]&&(a)[3]==(b)[3])

uint8_t ridTable[RID_QTY][RID_SIZE];
uint8_t lastRid[RID_BLOCKS] = {0};
uint8_t existsRid(const uint8_t * h){
	static uint8_t i, block, ret;
	block = h[1]%RID_BLOCKS*RID_QTY_PER_BLOCK;
	printf("e: %d %d\n", (uint16_t)block, (uint16_t)(block+RID_QTY_PER_BLOCK));
	ret = 0;
	for( i = 0 ; i < RID_QTY_PER_BLOCK ; ++i ) {
		if( CmpRid1(ridTable[block+i], h) ) {
			if( ridTable[block+i][0] == h[0] ) return 1;
			ret = 2;
		}
	}
	return ret;
}
void registerRid(const uint8_t * h){
	static uint8_t block;
	block = h[1]%RID_BLOCKS;
	if( CmpRid(ridTable[block*RID_QTY_PER_BLOCK+lastRid[block]], h) ) return;
	lastRid[block] = (lastRid[block]+1)%RID_QTY_PER_BLOCK;
	printf("r: %d %d %d\n", (uint16_t)block, (uint16_t)(block*RID_QTY_PER_BLOCK), (uint16_t)(block*RID_QTY_PER_BLOCK+lastRid[block]));
	memcpy(ridTable[block*RID_QTY_PER_BLOCK+lastRid[block]], h, RID_SIZE);
}
void createRid(uint8_t * h) {
	uint8_t j;
	for( j = 0 ; j < RID_SIZE ; ++j ) {
		h[j] = (uint8_t)random_rand();
	}
}

// ---------------------------- IP TABLE -------------

typedef struct {
	uip_ipaddr_t ipaddr;
	uip_ipaddr_t link;
	char name[NAME_SIZE];
	//uint32_t lastAnnounce = 0;
} iptable_t;
iptable_t iptable[IPTABLE_SIZE];
uint16_t lastIp = 0;

iptable_t * findTableByName(const char * name) {
	static uint8_t i, j;
	for( i = 0 ; i < IPTABLE_SIZE ; ++i ) {
		j = (lastIp-i-1+IPTABLE_SIZE)%IPTABLE_SIZE;
		if( !strncmp(iptable[j].name, name, NAME_SIZE) ) return iptable+j;
	}
	return NULL;
}
iptable_t * findTableByIp(const uip_ipaddr_t * ip) {
	static uint8_t i, j;
	for( i = 0 ; i < IPTABLE_SIZE ; ++i ) {
		j = (lastIp-i-1+IPTABLE_SIZE)%IPTABLE_SIZE;
		if( uip_ipaddr_cmp(&iptable[j].ipaddr, ip) ) return iptable+j;
	}
	return NULL;
}
void registerIp(const void * ip, const void * link, const char * d) {
	static iptable_t * ipt;
	ipt = findTableByIp(ip);
	printf("registrando: %s, ", d);
	PRINT6ADDR(ip);
	printf(" - ");
	PRINT6ADDR(link);
	puts("");
	if( !ipt ) {
		ipt = iptable+lastIp;
		lastIp = (lastIp+1)%IPTABLE_SIZE;
		memcpy(&ipt->ipaddr, ip, sizeof(uip_ipaddr_t));
		if( !d ) ipt->name[0] = 0;
	}
	memcpy(&ipt->link, link, sizeof(uip_ipaddr_t));
	if( d ) strncpy(ipt->name, d, NAME_SIZE);
	//iptable[lastIp].lastAnnounce = curTime;
}
// -------------------------------- PACKAGE SENDER -----------

#define SendPackage(data,len,ip) do{ \
		registerRid(data); \
		memcpy(&conn->ripaddr, ip, sizeof(uip_ipaddr_t)); \
		conn->rport = CONN_PORT; \
		uip_udp_packet_send(conn, data, len); \
		uip_create_unspecified(&conn->ripaddr); \
		conn->rport = 0; \
} while(0)
void sendPackage(const uint8_t * data, uint16_t len, const uip_ipaddr_t * ip) {
	SendPackage(data,len,ip);
}

#define SharePackage(data,len) do{ \
		registerRid(data); \
		uip_create_linklocal_allnodes_mcast(&conn->ripaddr); \
		conn->rport = CONN_PORT; \
		uip_udp_packet_send(conn, data, len); \
		uip_create_unspecified(&conn->ripaddr); \
		conn->rport = 0; \
} while(0)
void sharePackage(const uint8_t * data, uint16_t len) {
	SharePackage(data,len);
}

char * initPackage(char * d, char t) {
	createRid(d);
	d[PACKAGE_HEAD-1] = t;
	if( t == IDENTIFIED_PACKAGE_M || t == COMMAND_M ) {
		memcpy(d+PACKAGE_HEAD, myIp, UID_SIZE);
		return d+PACKAGE_HEAD+UID_SIZE;
	}
	return d+PACKAGE_HEAD;
}
void sendPackageByTable(char * data, uint16_t len, iptable_t * ipt) {
	if( !uip_ipaddr_cmp(&ipt->ipaddr, &ipt->link) ) {
		if( data[RID_SIZE] != MESH_M ) {
			static uint16_t i;
			for( i = len-1 ; i >= RID_SIZE ; --i ) {
				// buf can be data, reverse order to not override
				buf[i+MESH_HEAD_SIZE] = data[i];
			}
			for( i = 0 ; i < RID_SIZE ; ++i ) {
				buf[i] = data[i];
			}
			buf[RID_SIZE] = MESH_M;
			memcpy(buf+PACKAGE_HEAD, &ipt->ipaddr, UID_SIZE);
			data = buf;
			len += MESH_M;
		}
	} else if( data[RID_SIZE] == MESH_M ) {
		static uint16_t i;
		for( i = PACKAGE_HEAD+MESH_HEAD_SIZE ; i < len ; ++i ) {
			data[i-MESH_HEAD_SIZE] = data[i];
		}
		len -= MESH_HEAD_SIZE;
	}
	SendPackage(data, len, &ipt->link);
}

void sendNameRequest(char * name) {
	char * pack = initPackage(buf, NAME_REQUEST_M);
	strncpy(pack, name, NAME_SIZE);
	SharePackage(buf, PACKAGE_HEAD+NAME_SIZE);
}
void sendIpRequest(uip_ipaddr_t * ip) {
	char * pack = initPackage(buf, IP_REQUEST_M);
	memcpy(pack, ip, sizeof(uip_ipaddr_t));
	SharePackage(buf, PACKAGE_HEAD+sizeof(uip_ipaddr_t));
}
void announceMe() {
	char * pack = initPackage(buf, ANNOUNCEMENT_M);
	memcpy(pack, myIp, sizeof(uip_ipaddr_t));
	strncpy(pack + sizeof(uip_ipaddr_t), myName, NAME_SIZE);
	SharePackage(buf, PACKAGE_HEAD+sizeof(uip_ipaddr_t)+NAME_SIZE);
}
void sendAck(char * data, uip_ipaddr_t * ip) {
	static iptable_t * ipt;
	static char * pack;
	pack = initPackage(buf, ACKNOWLEDGE_M);
	memcpy(pack,data,RID_SIZE);
	if( ipt = findTableByIp(ip) ) {
		sendPackageByTable(data, PACKAGE_HEAD+RID_SIZE, ipt);
	}
}

void sendPacketByName(char * data, uint16_t len, char * name) {
	static iptable_t * ipt;
	if( ipt = findTableByName(name) ) {
		sendPackageByTable(data,len, ipt);
	} else {
		sendNameRequest(name);
	}
}
void sendPacketByIp(char * data, uint16_t len, uip_ipaddr_t * ip) {
	static iptable_t * ipt;
	if( ipt = findTableByIp(ip) ) {
		sendPackageByTable(data,len, ipt);
	} else {
		sendIpRequest(ip);
	}
}
void forward(char * data, uint16_t len) {
	static iptable_t * ipt;
	if( len < PACKAGE_HEAD + MESH_HEAD_SIZE ) return; // invalid
	if( len >= PACKAGE_HEAD + MESH_HEAD_SIZE + 1 + UID_SIZE
			&& data[PACKAGE_HEAD+MESH_HEAD_SIZE] == IDENTIFIED_PACKAGE_M ) { // || COMMAND_M FIXME
		registerIp(data+(PACKAGE_HEAD+MESH_HEAD_SIZE+1), &UIP_IP_BUF->srcipaddr, NULL);
	}
	if( ipt = findTableByIp((uip_ipaddr_t*)(data+PACKAGE_HEAD)) ) {
		sendPackageByTable(data,len,ipt);
	} else {
		// path not found, do nothing?
	}
}

// ------------------------------------ Send Out - "Safe" ------
typedef struct {
	union {
		char name[NAME_SIZE];
		uip_ipaddr_t ip;
	};
	char data[BUF_SIZE+UID_SIZE];
	uint16_t len;
	int16_t timeLeft;
	// timeLeft = 0 -> empty
	// timeLeft < 0 -> unsolved
	// timeLeft > 0 -> solved
} SendBuf_t;
SendBuf_t sendOut[SENDOUT_SIZE];
SendBuf_t * newSendBuf() {
	uint16_t i;
	for( i = 0 ; i < SENDOUT_SIZE ; ++i ) {
		if( sendOut[i].timeLeft == 0 ) return sendOut+i;
	}
	return NULL;
}

void acknowledge(char * data) {
	static uint16_t i;
	for( i = 0 ; i < SENDOUT_SIZE ; ++i ) {
		if( sendOut[i].timeLeft > 0 && CmpRid(data, sendOut[i].data) ) {
			// ack recebido
			printf("ack recebido...\n");
			sendOut[i].timeLeft = 0;
			return;
		}
	}
}

#define CheckSendOut1(s) do { \
		static iptable_t * ipt; \
		if( (s).timeLeft < 0 && (s).timeLeft%SOLVE_TIME == 0 ) { \
			if( ipt = findTableByName((s).name) ) { \
				(s).timeLeft = ACK_TIME*ACK_TRIES; \
				memcpy(&(s).ip, &ipt->ipaddr, sizeof(uip_ipaddr_t)); \
				sendPackageByTable((s).data, (s).len, ipt); \
			} else { \
				sendNameRequest((s).name); \
			} \
		} \
		if( (s).timeLeft > 0 && (s).timeLeft%ACK_TIME == 0 ) { \
			if( ipt = findTableByIp(&(s).ip) ) { \
				((char*)((s).data))[0]++; \
				sendPackageByTable((s).data, (s).len, ipt); \
			} else { \
				sendIpRequest(&(s).ip); \
			} \
		} \
} while(0)

void checkSendOutAll(){
	static uint16_t i;
	for( i = 0 ; i < SENDOUT_SIZE ; ++i ) {
		if( sendOut[i].timeLeft == 0 ) continue;
		if( sendOut[i].timeLeft < 0 ) sendOut[i].timeLeft++;
		if( sendOut[i].timeLeft > 0 ) sendOut[i].timeLeft--;
		CheckSendOut1(sendOut[i]);
		// if( sendOut[i].timeLeft == 0 ) ; //pacote perdido
	}
}
uint8_t safeSendByIp(char * data, uint16_t len, uip_ipaddr_t * ip) {
	// requires data be a identified package
	static SendBuf_t * sb;
	if( sb = newSendBuf() ) {
		memcpy(sb->data,data,len);
		memcpy(&sb->ip, ip, sizeof(uip_ipaddr_t));
		sb->len = len;
		sb->timeLeft = ACK_TIME*ACK_TRIES;
		CheckSendOut1(*sb);
		return 1;
	} else {
		// chei
		return 0;
	}
}
uint8_t safeSendByName(char * data, uint16_t len, char * name) {
	// requires data be a identified package
	static SendBuf_t * sb;
	static iptable_t * ipt;
	if( sb = newSendBuf() ) {
		memcpy(sb->data,data,len);
		sb->len = len;
		if( ipt = findTableByName(name) ) {
			memcpy(&sb->ip, &ipt->ipaddr, sizeof(uip_ipaddr_t));
			sb->timeLeft = ACK_TIME*ACK_TRIES;
		} else {
			strncpy(sb->name, name, NAME_SIZE);
			sb->timeLeft = -SOLVE_TIME*SOLVE_TRIES;
		}
		CheckSendOut1(*sb);
		return 1;
	} else {
		// chei
		return 0;
	}
}

// -------------------------------- UART CONTROL ----------------

#define PUT_NL0() uart0_writeb('\n')
void puts0(const char * d, uint16_t size) {
	while( size-- && *d ) uart0_writeb(*d++);
}
void printbyte0(unsigned char * d, uint16_t len){
	while(len--) {
		if( (*d>>4) < 10 ) uart0_writeb((*d>>4)+'0');
		else uart0_writeb((*d>>4)-10+'A');
		if( (*d&15) < 10 ) uart0_writeb((*d&0xf)+'0');
		else uart0_writeb((*d&0xf)-10+'A');
		d++;
	}
}

void executeCmd(char * cmd) {
	static char *name, *pack;
	static uint8_t i;
	static iptable_t *ipt;
	cmd++;
	switch( cmd[-1] ){
	case 'N': // name
		strncpy(myName, cmd, NAME_SIZE);
		registerIp(myIp, myIp, myName);
		//announceMe();
		break;
	case 'M': // me
		puts0(myName, NAME_SIZE);
		PUT_NL0();
		break;
	case 'S': // send
		name = cmd;
		while( *cmd && *cmd != ' ' ) cmd++;
		if( !*cmd ) return;
		*cmd++ = 0;
		pack = initPackage(buf, IDENTIFIED_PACKAGE_M);
		if( strlen(cmd) < BUF_SIZE-PACKAGE_HEAD ) {
			strcpy(pack, cmd);
			safeSendByName(buf, PACKAGE_HEAD+strlen(cmd), name);
		} else {
			memcpy(pack, cmd, BUF_SIZE-PACKAGE_HEAD);
			safeSendByName(buf, BUF_SIZE, name);
		}
		break;
	case 'D': // discovery neighborhood
		pack = initPackage(buf, NEIGHBOR_REQUEST_M);
		if( '0' <= cmd[0] && cmd[0] <= '9' ) {
			pack[0] = cmd[0]-'0';
			sharePackage(buf, PACKAGE_HEAD+1);
		} else {
			sharePackage(buf, PACKAGE_HEAD);
		}
		break;
	case 'L': // list
		for( i = 0 ; i < IPTABLE_SIZE ; ++i ){
			printf("%s: ", iptable[i].name);
			PRINT6ADDR(iptable[i].ipaddr);
			printf(" - ");
			PRINT6ADDR(iptable[i].link);
			puts("");
		}
		break;
	case 'B': // block
		if( ipt = findTableByName(cmd) ) {
			uip_ipaddr_copy(&blocked, &ipt->ipaddr);
			puts("Done");
		} else {
			puts("Not found");
		}
		break;
	case 'C': // command
		name = cmd;
		while( *cmd && *cmd != ' ' ) cmd++;
		if( !*cmd ) return;
		*cmd++ = 0;
		pack = initPackage(buf, COMMAND_M);
		if( strlen(cmd) < BUF_SIZE-(pack-buf) ) {
			strcpy(pack, cmd);
			safeSendByName(buf, (pack-buf)+strlen(cmd), name);
		} else {
			memcpy(pack, cmd, BUF_SIZE-(pack-buf) );
			safeSendByName(buf, BUF_SIZE, name);
		}
		break;
	}
}
#define UART_FEEDBACK 0
int getc0(unsigned char x) {
	static char cmd[CMD_SIZE+1];
	static uint16_t cmdPt = 0;
	if( x == 0x7f ) {
		if( cmdPt ) {
			cmd[--cmdPt] = 0;
#if UART_FEEDBACK
			uart0_writeb('\b');
			uart0_writeb(' ');
			uart0_writeb('\b');
#endif
		}
	} else 	if( x == '\n' ) {
		cmd[cmdPt] = 0;
#if UART_FEEDBACK
		uart0_writeb('\n');
#endif
		executeCmd(cmd);
		cmdPt = 0;
	} else if( cmdPt < CMD_SIZE ) {
#if UART_FEEDBACK
		uart0_writeb(x);
#endif
		cmd[cmdPt++] = x;
	} else {
		// overflow?
	}
}

// ---------------------------- PACKAGE HANDLE -------

void tcpip_handler(void) {
	static char * data;
	static uint16_t len;
	static uint8_t eRid;
	if(!uip_newdata()) return;
		len = uip_datalen();
		data = uip_appdata;
		printf("vei pac %d ", len);
		printbyte0((char *)&UIP_IP_BUF->srcipaddr, UID_SIZE);
		printf(": ");
		printbyte0(data,len);
		puts("");

		if( uip_ipaddr_cmp(&blocked,&UIP_IP_BUF->srcipaddr) ) return;
		//printbyte0(data,len);
		if( len < PACKAGE_HEAD ) return;
		eRid = existsRid(data);
		printf("%d %d\n", (uint16_t)eRid, (uint16_t)data[RID_SIZE]);
		if( eRid == 1 ) return;
		registerRid(data);
		data += PACKAGE_HEAD;
		len -= PACKAGE_HEAD;
		// com acks
		if( (data[-1] != 'P') && eRid == 2 ) return;

		switch( data[-1] ) {
		static iptable_t * ipt;
		case ANNOUNCEMENT_M: // annoucement
			data[len] = 0;
			if( len < sizeof(uip_ipaddr_t) ) break;
			registerIp(data, &UIP_IP_BUF->srcipaddr, data+sizeof(uip_ipaddr_t));
			sharePackage(data-PACKAGE_HEAD, len+PACKAGE_HEAD);
			break;
		case NEIGHBOR_REQUEST_M:
			announceMe();
			if( len && --data[0] ) {
				sharePackage(data-PACKAGE_HEAD, len+PACKAGE_HEAD);
			}
			break;
		case NAME_REQUEST_M: // request
			data[len] = 0;
			if( !strncmp(data,myName, NAME_SIZE) ) {
				announceMe();
			} else {
				sharePackage(data-PACKAGE_HEAD, len+PACKAGE_HEAD);
			}
			break;
		case IP_REQUEST_M: // request ip
			if( uip_ipaddr_cmp(data,myIp) ) {
				announceMe();
			} else {
				sharePackage(data-PACKAGE_HEAD, len+PACKAGE_HEAD);
			}
			break;
		case ACKNOWLEDGE_M: // ACK
			acknowledge(data);
			break;
		case PACKAGE_M: // package (without id)
			if( eRid == 0 ) { // novo mesmo
				puts0("New Package:", -1);
				puts0(data, len);
				PUT_NL0();
			}
			break;
		case IDENTIFIED_PACKAGE_M:
			if( eRid == 0 ) { // novo mesmo
				puts0("New Package from ", -1);
				printbyte0(data,UID_SIZE);
				puts0(":", -1);
				puts0(data+UID_SIZE, len);
				PUT_NL0();
			}
			registerIp(data, &UIP_IP_BUF->srcipaddr, NULL);
			sendAck(data-PACKAGE_HEAD, (uip_ipaddr_t *)data);
			break;
		case MESH_M: // Mesh
			forward(data-PACKAGE_HEAD, len+PACKAGE_HEAD);
			break;
		case COMMAND_M: // command
			if( eRid == 0 ) { // novo mesmo
				executeCmd(data);
			}
			registerIp(data, &UIP_IP_BUF->srcipaddr, NULL);
			sendAck(data-PACKAGE_HEAD, (uip_ipaddr_t *)data);
			break;
		}
		// sempre de conn ?
		uip_create_unspecified(&conn->ripaddr);
		conn->rport = 0;
}

// ----------------------------- MAIN -------------

void timeout_handler(void) {
	curTime++;
	//if( curTime % ANNOUNCE_TIME == 0 ) announceMe();
	checkSendOutAll();
}

static void getLocalAdress(void) {
	static uint8_t i, state;

	for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
		state = uip_ds6_if.addr_list[i].state;
		if(uip_ds6_if.addr_list[i].isused && (state == ADDR_TENTATIVE || state
				== ADDR_PREFERRED)) {
			memcpy(myIp, &uip_ds6_if.addr_list[i].ipaddr, sizeof(uip_ipaddr_t));
			if(state == ADDR_TENTATIVE) {
				uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
			}
		}
	}
}

static void defineInitialName(void) {
	static __xdata unsigned char *macp = &X_IEEE_ADDR;
	const int MAC_SIZE = 6;
	static uint8_t i;
	myName[0] = '~';
	for( i = 1 ; i < MAC_SIZE && i < NAME_SIZE/2 ; ++i ) {
		myName[2*i-1] = *macp%26 + 'a';
		myName[2*i] = *macp/26 + 'a';
		macp++;
		myName[2*i+1] = 0;
	}
}

struct etimer et;
PROCESS_THREAD(com_process, ev, data) {
	PROCESS_BEGIN();

	defineInitialName();
	getLocalAdress();

	uart0_set_input(getc0);
	etimer_set(&et, CLOCK_SECOND);
	//announceMe();
	conn = udp_new(NULL, 0, NULL);
	if( !conn ) PRINTF("udp_new conn error.\n");
	udp_bind(conn, UIP_HTONS(CONN_PORT));

	printf("Oi, %s\n", myName);

	while(1) {
		PROCESS_YIELD();
		if(etimer_expired(&et)) {
			timeout_handler();
			etimer_restart(&et);
		} else if(ev == tcpip_event) {
			tcpip_handler();
		}
	}

	PROCESS_END();
}
