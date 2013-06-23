/*
 * Engenhacao Mesh System
 */

#include "contiki.h"
#include "contiki-net.h"
#include <stdio.h>
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

//*
#define FUNCTION_PREFIX
#include "util.h"
#include "settings.h"
#include "package.h"
#include "transmission.h"
#include "messages.h"
#include "sendout.h"
#include "interface.h"
/*/
#define FUNCTION_PREFIX static
#include "util.c"
#include "settings.c"
#include "package.c"
#include "transmission.c"
#include "messages.c"
#include "sendout.c"
#include "interface.c"
//*/

PROCESS(ems_process, "EMS process");
AUTOSTART_PROCESSES(&ems_process);

struct uip_udp_conn *conn;

PROCESS_THREAD(ems_process, ev, data) {
	static struct etimer et1, et2;
	PROCESS_BEGIN();

	cur_time = 0;
	init_settings();
	init_package();
	init_transmission();
	init_interface();

	conn = udp_new(NULL, 0, NULL);
	//if( !conn ) PRINTF("udp_new conn error.\n");
	udp_bind(conn, UIP_HTONS(CONN_PORT));

	send_broadcast_msg("Hello");
	etimer_set(&et1, CLOCK_SECOND/10);
	etimer_set(&et2, CLOCK_SECOND);
	while(1) {
		PROCESS_YIELD();
		if(etimer_expired(&et1)) {
			etimer_restart(&et1);
			check_interface();
		} else if( etimer_expired(&et2) ) {
			etimer_restart(&et2);
			cur_time++;
			check_send_out();
			check_transmission();
		} else if(ev == tcpip_event) {
			if( uip_newdata() ) {
				process_package(uip_appdata, uip_datalen(), &UIP_IP_BUF->srcipaddr);
				uip_create_unspecified(&conn->ripaddr);
				conn->rport = 0;
			}
		}
	}

	PROCESS_END();
}
