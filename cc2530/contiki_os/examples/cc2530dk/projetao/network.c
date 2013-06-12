#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include <string.h>
#include "dev/leds.h"
#include "dev/uart0.h"
#include "dev/button-sensor.h"
#include "net/uip-debug.h"

PROCESS(com_process, "GPS process");
AUTOSTART_PROCESSES(&gps_process);

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define BUF_SIZE 92 // < 240 (UIP_BUFSIZE), > 92 eh descaratdo
#define NAME_SIZE 16
#define IPTABLE_SIZE 12
#define CMD_SIZE 140
#define SENDOUT_SIZE 5
#define RID_SIZE 4
#define RID_QTY 160
#define PACKAGE_HEAD (RID_SIZE+1)
#define CONN_PORT 2570

#define SOLVE_TIME 1
#define SOLVE_TRIES 20
#define ACK_TIME 1
#define ACK_TRIES 20
#define ANNOUNCE_TIME 30

char buf[BUF_SIZE];
char myName[NAME_SIZE];
#define myIp (&myIpaddr)//(&uip_lladdr)
static uip_ipaddr_t myIpaddr;
struct uip_udp_conn *conn;
int32_t curTime = 0; // 120 anos
void puts0(char*, uint16_t);
#define uip_ipaddr_copy(a,b) memcpy(a,b,sizeof(uip_ipaddr_t))

// --------------------- Rand ID ---------------------------
uint8_t ridTable[RID_QTY][RID_SIZE];
uint16_t lastRid = 0;
uint8_t existsRid(const uint8_t * h){
	static uint8_t i, j;
	for( i = 0 ; i < RID_QTY ; ++i ) {
		for( j = 1 ; j < RID_SIZE ; ++j ) {
			if( ridTable[i][j] != h[j] ) goto mismatch;
		}
		return ridTable[i][0] == h[0] ? 1 : 2;
		mismatch:;
	}
	return 0;
}
void registerRid(const uint8_t * h){
	memcpy(ridTable[lastRid], h, RID_SIZE);
	lastRid = (lastRid+1)%RID_QTY;
}
void createRid(uint8_t * h) {
	uint8_t j;
	for( j = 0 ; j < RID_SIZE ; ++j ) {
		h[j] = (uint8_t)random_rand();
	}
}
//#define CmpRid(a,b) (a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2]&&a[3]==b[3])
#define CmpRid(a,b) (memcmp(a,b,RID_SIZE) == 0)

// ---------------------------- IP TABLE -------------

typedef struct {
	uip_ipaddr_t ipaddr;
	uip_ipaddr_t link;
	char name[NAME_SIZE];
	//uint32_t lastAnnounce = 0;
} iptable_t;
iptable_t iptable[IPTABLE_SIZE];
uint16_t lastIp = 0;

void registerIp(const void * ip, const void * link, const char * d) {
	uip_ipaddr_copy(&iptable[lastIp].ipaddr, ip);
	uip_ipaddr_copy(&iptable[lastIp].link, link);
	strncpy(iptable[lastIp].name, d, NAME_SIZE);
	//iptable[lastIp].lastAnnounce = curTime;
	lastIp = (lastIp+1)%IPTABLE_SIZE;
}
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

// -------------------------------- PACKAGE SENDER -----------

#define SendPackage(data,len,ip) do{ \
		registerRid(data); \
		uip_ipaddr_copy(&conn->ripaddr, ip); \
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

void * initPackage(char * d, char t) {
	createRid(d);
	d[PACKAGE_HEAD-1] = t;
	return d+PACKAGE_HEAD;
}
void sendPackageByTable(char * data, uint16_t len, iptable_t * ipt) {
	if( uip_ipaddr_cmp(&ipt->ipaddr, &ipt->link) ) {
		SendPackage(data, len, &ipt->ipaddr);
	} else {
		puts("como assim mesh?");
		// mesh TODO
	}
}

void sendNameRequest(char * name) {
	char * pack = initPackage(buf, 'R');
	float x = 10./name[0];
	strncpy(pack, name, NAME_SIZE);
	SharePackage(buf, PACKAGE_HEAD+NAME_SIZE);
}
void sendIpRequest(uip_ipaddr_t * ip) {
	char * pack = initPackage(buf, 'r');
	uip_ipaddr_copy(pack, ip);
	SharePackage(buf, PACKAGE_HEAD+sizeof(uip_ipaddr_t));
}
void announceMe() {
	char * pack = initPackage(buf, 'A');
	uip_ipaddr_copy(pack, myIp);
	strncpy(pack + sizeof(uip_ipaddr_t), myName, NAME_SIZE);
	SharePackage(buf, PACKAGE_HEAD+sizeof(uip_ipaddr_t)+NAME_SIZE);
}
void sendAck(char * data, uip_ipaddr_t * ip) {
	char * pack = initPackage(buf, 'a');
	memcpy(pack,data,RID_SIZE);
	SendPackage(buf, PACKAGE_HEAD+RID_SIZE, ip);
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

// ------------------------------------ Send Out - "Safe" ------
typedef struct {
	union {
		char name[NAME_SIZE];
		uip_ipaddr_t ip;
	};
	char data[BUF_SIZE];
	uint16_t len;
	int16_t timeLeft;
	// timeLeft = 0 -> empty
	// timeLeft < 0 -> unsolved
	// timeLeft > 0 -> solved
} SendBuf_t;
SendBuf_t sendOut[SENDOUT_SIZE];
SendBuf_t * emptySendBuf() {
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
				uip_ipaddr_copy(&(s).ip, &ipt->ipaddr); \
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
		if( sendOut[i].timeLeft < 0 ) sendOut[i].timeLeft++;
		if( sendOut[i].timeLeft > 0 ) sendOut[i].timeLeft--;
		CheckSendOut1(sendOut[i]);
		// if( sendOut[i].timeLeft == 0 ) ; //pacote perdido
	}
}
uint8_t safeSendByIp(char * data, uint16_t len, uip_ipaddr_t * ip) {
	static SendBuf_t * sb;
	if( sb = emptySendBuf() ) {
		memcpy(sb->data,data,len);
		uip_ipaddr_copy(&sb->ip, ip);
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
	static SendBuf_t * sb;
	static iptable_t * ipt;
	if( sb = emptySendBuf() ) {
		memcpy(sb->data,data,len);
		sb->len = len;
		if( ipt = findTableByName(name) ) {
			uip_ipaddr_copy(&sb->ip, &ipt->ipaddr);
			sb->timeLeft = ACK_TIME*ACK_TRIES;
		} else {
			strncpy(sb->name, name, NAME_SIZE);
			sb->timeLeft = -SOLVE_TIME*SOLVE_TRIES;
		}
		CheckSendOut1(*sb);
		return 1;
	} else {
		return 0;
	}
}

// -------------------------------- UART CONTROL ----------------

void puts0(const char * d, uint16_t size) {
	while( size-- && *d ) uart0_writeb(*d++);
	uart0_writeb('\n');
}
void printbyte0(unsigned char * d, uint16_t len){
	while(len--) {
		if( (*d&15) < 10 ) uart0_writeb((*d&0xf)+'0');
		else uart0_writeb((*d&0xf)-10+'A');
		if( (*d>>4) < 10 ) uart0_writeb((*d>>4)+'0');
		else uart0_writeb((*d>>4)-10+'A');
		d++;
	}
	uart0_writeb('\n');
}

void executeCmd(char * cmd) {
	static char *name, *pack;
	cmd++;
	switch( cmd[-1] ){
	case 'N': // name
		strncpy(myName, cmd, NAME_SIZE);
		registerIp(myIp, myIp, myName);
		//announceMe();
		break;
	case 'M': // me
		puts0(myName, NAME_SIZE);
		break;
	case 'S': // send
		name = cmd;
		while( *cmd && *cmd != ' ' ) cmd++;
		if( !*cmd ) return;
		*cmd++ = 0;
		pack = initPackage(buf, 'P');
		if( strlen(cmd) < BUF_SIZE-PACKAGE_HEAD ) {
			strcpy(pack, cmd);
			safeSendByName(buf, PACKAGE_HEAD+strlen(cmd), name);
		} else {
			memcpy(pack, cmd, BUF_SIZE-PACKAGE_HEAD);
			safeSendByName(buf, BUF_SIZE, name);
		}
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
		//printbyte0(data,len);
		if( len <= PACKAGE_HEAD ) return;
		eRid = existsRid(data);
		if( eRid == 1 ) return;
		registerRid(data);
		data += PACKAGE_HEAD;
		len -= PACKAGE_HEAD;
		// com acks
		if( (data[-1] != 'P') && eRid == 2 ) return;

		switch( data[-1] ) {
		static iptable_t * ipt;
		case 'A': // annoucement
			data[len-1] = 0;
			if( len <= sizeof(uip_ipaddr_t) ) break;
			registerIp(data, &UIP_IP_BUF->srcipaddr, data+sizeof(uip_ipaddr_t));
			sharePackage(data-PACKAGE_HEAD, len+PACKAGE_HEAD);
			break;
		case 'R': // request
			data[len-1] = 0;
			if( !strncmp(data,myName, NAME_SIZE) ) {
				announceMe();
			} else {
				sharePackage(data-PACKAGE_HEAD, len+PACKAGE_HEAD);
			}
			break;
		case 'r': // request ip
			data[len-1] = 0;
			if( uip_ipaddr_cmp(data,myIp) ) {
				announceMe();
			} else {
				sharePackage(data-PACKAGE_HEAD, len+PACKAGE_HEAD);
			}
			break;
		case 'a': // ACK
			acknowledge(data);
			break;
		case 'P': // package (without id)
			if( eRid == 0 ) { // novo mesmo
				puts0(data, len);
			}
			sendAck(data-PACKAGE_HEAD, &UIP_IP_BUF->srcipaddr);
			break;
		case 'I': // package (with id)
			// TODO
		case 'M': // Mesh
			// TODO
		}
		// sempre de conn ?
		uip_create_unspecified(&conn->ripaddr);
		conn->rport = 0;
}
