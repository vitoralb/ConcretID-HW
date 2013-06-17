/*
 *
 */

#include "settings.h"
#include "cc253x.h"

settings_t me = {0};
const settings_t me_mory = {0};

static void make_name_from_mac(char * name) {
	static __xdata uint8_t *macp = &X_IEEE_ADDR;
	const int MAC_SIZE = 6;
	static uint8_t i, j;
	static uint16_t rem;
	const consonants[20] = "qwrtypsdfghjklzxcvbnm", vowels[5] = "aeiou";
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
	*name++ = volwes[rem%5];
	rem /= 5;

	while( ++j < NAME_SIZE ) *name++ = 0;
}

void init_settings() {
	if( me_mory.saved ) {
		make_name_from_mac(me.name);
		me.time_to_solve = 1;
		me.solving_attempts = 20;
		me.time_to_send = 1;
		me.sending_attempts = 20;
		me.time_to_announce = 0;
		me.rf_tx_power = 0x05;
		me.rf_use_hgm = 1;
		me.uart_feedback = 0;
	} else {
		memcpy(&me,&me_mory,sizeof(settings_t));
	}
	// TODO
	set_tx_power(me.rf_tx_power);
	set_use_hgm(me.rf_use_hgm);
}
