#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "cc253x.h"
#include <stdio.h>
#include <string.h>

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len])

#define BUF_LEN 40
static char buf[BUF_LEN];

static struct uip_udp_conn *server_conn;
static uint16_t len;
static uint32_t number;

static void
tcpip_handler(void)
{
  putchar(',');
  if(uip_newdata()) {
	putchar('.');

    switch( ((char*)uip_appdata)[0] ) {
    case 'P': // PING
    	buf[0] = 'A'; // ANUNCIO
    	memcpy(buf+1,&number,sizeof(uint32_t));
    	len = 1+sizeof(uint32_t);

    	uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
    	server_conn->rport = UIP_UDP_BUF->srcport;
    	uip_udp_packet_send(server_conn, buf, len);
      uip_create_unspecified(&server_conn->ripaddr);
      server_conn->rport = 0;
    	break;
    }
  }
  return;
}

/*---------------------------------------------------------------------------*/
__xdata unsigned char *macp = &X_IEEE_ADDR;
#define MAC_SIZE 6
PROCESS(mobile_process, "Mobile process");
AUTOSTART_PROCESSES(&mobile_process);
PROCESS_THREAD(mobile_process, ev, data)
{
  static uint16_t i;
  PROCESS_BEGIN();
  number = 0;
  for( i = 0 ; i < MAC_SIZE ; ++i ) number ^= (uint32_t)*macp++ << i%sizeof(number)*8;
  printf("Number: %lu\n", number);

  server_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(server_conn, UIP_HTONS(3000));
  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
	  tcpip_handler();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
