/*
 *
 */

#include "interface.h"
#include "settings.h"

#define CMD_SIZE 100

void execute_cmd(const char * cmd) {

}

int process_char(unsigned char x) {
	static char cmd[CMD_SIZE+1];
	static uint16_t cmd_pt = 0;
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
		execute_cmd(cmd);
		cmd_pt = 0;
	} else if( cmdPt < CMD_SIZE ) {
		if( me.uart_feedback ) {
			uart0_writeb(x);
		}
		cmd[cmd_pt++] = x;
	} else {
		// overflow?
	}
}

void init_interface() {
	uart0_set_input(process_char);
}
