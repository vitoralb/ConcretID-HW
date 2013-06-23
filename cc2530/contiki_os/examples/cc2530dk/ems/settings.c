/*
 *
 */

#include "settings.h"
#include "cc253x.h"

settings_t me;
uint32_t cur_time;
const settings_t me_mory = {0};

static void make_name_from_mac(char * name) {
	static uint8_t *macp;
	const int MAC_SIZE = 6;
	static uint8_t i, j;
	static uint16_t rem;
	const char consonants[] = "qwrtpsdfghjklzxcvbnm", vowels[] = "aeiou";
	macp = &X_IEEE_ADDR;
	*name++ = '~';
	j = 1;
	rem = 0;
	for( i = 0 ; i < MAC_SIZE ; ++i, ++macp ) {
		rem = (rem<<8) + *macp++;
		if( ++j >= NAME_SIZE ) return;
		*name++ = consonants[rem%20];
		rem /= 20;
		if( ++j >= NAME_SIZE ) return;
		*name++ = vowels[rem/5];
		rem /= 5;
	}
	if( ++j >= NAME_SIZE ) return;
	*name++ = consonants[rem%20];
	rem /= 20;
	if( ++j >= NAME_SIZE ) return;
	*name++ = vowels[rem%5];
	rem /= 5;

	while( ++j < NAME_SIZE ) *name++ = 0;
}

FUNCTION_PREFIX void init_settings() {
	cur_time = 0;
	if( me_mory.saved ) {
		memcpy(&me,&me_mory,sizeof(settings_t));
	} else {
		me.saved = 0;
		make_name_from_mac(me.name);
		me.time_to_solve = 1;
		me.solving_attempts = 20;
		me.time_to_send = 1;
		me.sending_attempts = 20;
		me.time_to_announce = 0;
		me.neighbor_good_till = 30;
		set_tx_power(0x05);
		set_use_hgm(0);
		//set_low_rx_power(0);
		//set_channel();
		me.uart_feedback = 0;
	}
}

FUNCTION_PREFIX void set_tx_power(uint8_t power) {
	me.rf_tx_power = power;
	TXPOWER = power;
	// TODO
}
FUNCTION_PREFIX void set_use_hgm(uint8_t hgm) {
	me.rf_use_hgm = hgm;
	if( hgm ) {
		P0_7 = 1;
	} else {
		P0_7 = 0;
	}
}

FUNCTION_PREFIX void set_low_rx_power(uint8_t low_rx) {
	me.rf_low_rx_power = low_rx;
	if( low_rx ) {
		RXCTRL = 0x00;
		FSCTRL = 0x50;
	} else {
		RXCTRL = 0x3F;
		FSCTRL = 0x55;
	}
}
