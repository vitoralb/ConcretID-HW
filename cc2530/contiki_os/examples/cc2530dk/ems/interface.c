/*
 *
 */

#include "interface.h"
#include "settings.h"
#include "package.h"
#include "transmission.h"
#include "messages.h"
#include "sendout.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include "dev/uart0.h"

#define CMD_SIZE 127
#define CMD_QTY 4

char cmd_buffer[CMD_QTY][CMD_SIZE+1];
volatile uint8_t cmd_next, cmd_qty;
uint16_t cmd_pt;

#define Try_arg(name) if( !strncmp(arg, name, sizeof(name)-1) && (arg += (sizeof(name)-1)) )


static void cmd_echo(char * arg){
	if( *arg++ != ' ' ) return;
	puts(arg);
}
static void cmd_send(char * arg) {
	static char * name;
	if( *arg++ != ' ' ) return;
	while( *arg == ' ' ) ++arg;
	if( !*arg ) return;
	name = arg;
	while( *arg && *arg != ' ') ++arg;
	if( *arg ) *arg++ = 0;
	else *arg = 0;
	send_out_by_name(name, arg, PACKAGE_MSG, strlen(arg));
}
static void cmd_command(char * arg) {
	static char * name;
	if( *arg++ != ' ' ) return;
	while( *arg == ' ' ) ++arg;
	if( !*arg ) return;
	name = arg;
	while( *arg && *arg != ' ') ++arg;
	if( *arg ) *arg++ = 0;
	else *arg = 0;
	send_out_by_name(name, arg, PACKAGE_COMMAND, strlen(arg));
}
static void cmd_set(char * arg) {
	static uint8_t i;
	if( *arg++ != ' ' ) return;
	Try_arg("name ") {
		for( i = 0 ; i < NAME_SIZE && arg[i] ; ++i ) {
			if( arg[i] == ' ' ) arg[i] = '_';
		}
		strncpy(me.name, arg, NAME_SIZE);
		return;
	}
	Try_arg("txpower ") {
		me.rf_tx_power = strtoi(arg,NULL,0);
		return;
	}
	Try_arg("hgm ") {
		if( arg[0] == '1' || arg[0] == '0' ) {
			me.rf_use_hgm = arg[0]-'0';
		}
		return;
	}
	// TODO
}
static void cmd_get(char * arg) {
	static uint8_t i;
	if( *arg++ != ' ' ) return;
	Try_arg("name") {
		Print_at_most(me.name, NAME_SIZE);
		return;
	}
	Try_arg("txpower") {
		printf("%#hhx\n", me.rf_tx_power);
		return;
	}
	Try_arg("hgm") {
		printf("%hhu\n", me.rf_use_hgm);
		return;
	}
	// TODO
}
extern uint8_t neighbor_time[];
static void cmd_list(char * arg) {
	static uint8_t i;
	if( *arg++ != ' ' ) return;
	Try_arg("table") {
		for( i = 0 ; i < UID_TABLE_SIZE ; ++i ) {
			Print_at_most(uid_table[i].name, NAME_SIZE);
			UID_PRINT(&uid_table[i].uid);
			printf(" - ");
			UID_PRINT(&uid_table[i].link);
			printf("\n");
		}
		return;
	}
	Try_arg("neighbors") {
		for( i = 0 ; i < NEIGHBORHOOD_SIZE ; ++i ) if( Valid_num(neighbor_time[i]) ){
			UID_PRINT(&neighbor[i]);
			puts("");
		}
		return ;
	}
}
static void cmd_block(char * arg) {
	static uid_t tmp;
	if( *arg++ != ' ' ) return;
	uid_create(&tmp, arg);
	block_uid(&tmp);
}
static void cmd_discovery(char * arg) {
	static uint8_t depth;
	if( *arg++ != ' ' ) return;
	depth = strtoi(arg,NULL,0);
	discovery_neighborhood(depth);
}

FUNCTION_PREFIX void execute_cmd(char * cmd) {
	puts(cmd);
#define try_cmd(name) do { \
	if( !strncmp(#name, cmd,sizeof(#name)-1) ) return cmd_##name(cmd+(sizeof(#name)-1)); \
} while(0)
	try_cmd(echo);
	try_cmd(send);
	try_cmd(command);
	try_cmd(set);
	try_cmd(get);
	try_cmd(list);
	try_cmd(block);
	try_cmd(discovery);
#undef try_cmd
}

int process_char(unsigned char x) {
	static char * cmd;
	if( cmd_qty == CMD_QTY ) return 0; // buffer chei
	cmd = cmd_buffer[(cmd_next+cmd_qty)%CMD_QTY];
	if( x == 0x7f ) {
		if( cmd_pt ) {
			cmd[--cmd_pt] = 0;
			if( me.uart_feedback ) {
				uart0_writeb('\b');
				uart0_writeb(' ');
				uart0_writeb('\b');
			}
		}
	} else 	if( x == '\n' ) {
		cmd[cmd_pt] = 0;
		if( me.uart_feedback ) {
			uart0_writeb('\n');
		}
		cmd_qty++;
		cmd_pt = 0;
	} else if( cmd_pt < CMD_SIZE ) {
		if( me.uart_feedback ) {
			uart0_writeb(x);
		}
		cmd[cmd_pt++] = x;
	} else {
		// overflow?
	}
	return 0;
}

FUNCTION_PREFIX uint8_t check_interface() {
	if( !cmd_qty ) return 0;
	execute_cmd(cmd_buffer[cmd_next]);
	cmd_next = (cmd_next+1)%CMD_QTY;
	cmd_qty--;
	return 1;
}

FUNCTION_PREFIX void init_interface() {
	uart0_set_input(process_char);
	cmd_next = 0;
	cmd_qty = 0;
	cmd_pt = 0;
}
