/*
 *
 */
#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdlib.h>

#define NAME_SIZE 16

typedef struct settings_s {
	uint8_t saved;
	char name[NAME_SIZE];
	uint8_t time_to_solve;
	uint8_t solving_attempts;
	uint8_t time_to_send;
	uint8_t sending_attempts;
	uint8_t time_to_announce;
	uint8_t rf_tx_power;
	uint8_t rf_use_hgm;
	uint8_t uart_feedback;
} settings_t;

extern settings_t me;

void init_settings();
void set_tx_power();
void set_use_hgm();

#endif

