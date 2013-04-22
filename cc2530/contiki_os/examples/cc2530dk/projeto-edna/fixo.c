/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include <stdio.h>
#include <string.h>

#define SEND_INTERVAL  	CLOCK_SECOND
#define BUF_LEN		40
#define INTERV_BEG 5
#define INTERV_END 10
static char buf[BUF_LEN];

#define HISTORY_SIZE 100
static struct { uint32_t id, time; } history[HISTORY_SIZE];
uint32_t cur_time;

static struct uip_udp_conn *conn;

/*---------------------------------------------------------------------------*/
PROCESS(fixed_process, "Fixed process");
AUTOSTART_PROCESSES(&fixed_process);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  static uint32_t number;
  static uint16_t loc,i;
  if(uip_newdata()) {
    switch( ((char*)uip_appdata)[0] ) {
    case 'A': // anuncio
      memcpy(&number, (char*)uip_appdata+1, sizeof(uint32_t));
      //printf("anuncio %lu\n", number);
      loc = -1;
      for( i = 0 ; loc == -1 && i < HISTORY_SIZE ; ++ i ) {
    	  if( history[i].id == number ) {
    	    loc = i;
    	  }
      }
      if( loc != -1 ) {
    	  if( cur_time-INTERV_END <= history[loc].time && history[loc].time <= cur_time-INTERV_BEG ) {
    	    printf("abrir %lu\n", number);
    	    history[loc].time = 0;
      	} else if( history[loc].time < cur_time-INTERV_END ) {
    	    history[loc].time = cur_time;
    	  }
      } else {
    	  loc = 0;
    	  for( i = 1 ; i < HISTORY_SIZE ; ++ i ) {
		    if( history[i].time < history[loc].time ) {
		      loc = i;
		    }
		  }
		  history[loc].id = number;
		  history[loc].time = cur_time;
      }
      break;
    default: putchar(((char*)uip_appdata)[0]);
    }
  }

}
/*---------------------------------------------------------------------------*/
static void
timeout_handler(void)
{
  cur_time++;
  buf[0] = 'P';
  uip_create_linklocal_allnodes_mcast(&conn->ripaddr);
  conn->rport = UIP_HTONS(3000);
  uip_udp_packet_send(conn, buf, 1);
  uip_create_unspecified(&conn->ripaddr);
  conn->rport = 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(fixed_process, ev, data)
{
  static struct etimer et;
  static uip_ipaddr_t ipaddr;
  PROCESS_BEGIN();

  conn = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(conn, UIP_HTONS(3000));
  memset(history,0,sizeof(history));

  etimer_set(&et, SEND_INTERVAL);
  cur_time = 1;
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
/*---------------------------------------------------------------------------*/
