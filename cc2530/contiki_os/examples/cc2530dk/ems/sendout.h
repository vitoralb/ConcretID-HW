/*
 * sendout.h
 *
 *  Created on: Jun 17, 2013
 *      Author: afonso
 */

#ifndef SENDOUT_H_
#define SENDOUT_H_

#include "settings.h"
#include "package.h"

FUNCTION_PREFIX uint8_t acknowledge(rid_t * rid);
FUNCTION_PREFIX void check_send_out();
FUNCTION_PREFIX void init_send_out();
FUNCTION_PREFIX uint8_t send_out_by_name(char * name, uint8_t * data, uint8_t type, uint16_t size);

#endif
