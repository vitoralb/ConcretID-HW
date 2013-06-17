/*
 * Engenhacao Mesh System
 */

#include "settings.h"
#include "package.h"
#include "interface.h"
#include "transmission.h"
#include "sendout.h"
#include "messages.h"

#include "contiki.h"

PROCESS(com_process, "EMS process");
AUTOSTART_PROCESSES(&ems_process);

struct uip_udp_conn *conn;

struct etimer et;
PROCESS_THREAD(ems_process, ev, data) {
	PROCESS_BEGIN();

	init_settings();
	init_package();
	init_transmission();
	init_interface();

	conn = udp_new(NULL, 0, NULL);
	if( !conn ) PRINTF("udp_new conn error.\n");
	udp_bind(conn, UIP_HTONS(CONN_PORT));

	etimer_set(&et, CLOCK_SECOND);
	while(1) {
		PROCESS_YIELD();
		if(etimer_expired(&et)) {
			check_send_out_all();
			etimer_restart(&et);
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
