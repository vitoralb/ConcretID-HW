/*
 *
 */
#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdint.h>
#include <string.h>

#ifndef FUNCTION_PREFIX
#define FUNCTION_PREFIX
#endif

// dependent
#define CONN_PORT 2570 // 0x0A0A

#define NAME_SIZE 16

typedef struct settings_s {
	uint8_t saved;
	char name[NAME_SIZE];
	uint8_t time_to_solve;
	uint8_t solving_attempts;
	uint8_t time_to_send;
	uint8_t sending_attempts;
	uint8_t time_to_announce;
	uint8_t neighbor_good_till;
	uint8_t rf_tx_power;
	uint8_t rf_use_hgm;
	uint8_t rf_low_rx_power;
	uint8_t uart_feedback;
} settings_t;

extern settings_t me;
extern uint32_t cur_time;

FUNCTION_PREFIX void init_settings();
FUNCTION_PREFIX void set_tx_power(uint8_t power);
FUNCTION_PREFIX void set_use_hgm(uint8_t hgm);
FUNCTION_PREFIX void set_low_rx_power(uint8_t low_rx);

#endif

