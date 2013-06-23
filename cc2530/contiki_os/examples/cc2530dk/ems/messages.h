/*
 * messages.h
 *
 *  Created on: Jun 17, 2013
 *      Author: afonso
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include "settings.h"
#include "package.h"
#include "transmission.h"
#include "sendout.h"

FUNCTION_PREFIX void process_package(package_t * package, uint16_t size, uid_t * src);

FUNCTION_PREFIX void send_ack(package_t * rid);
FUNCTION_PREFIX void send_broadcast_ack(rid_t * rid);
FUNCTION_PREFIX void send_uid_request(uid_t * uid);
FUNCTION_PREFIX void send_name_request(const char * name);
FUNCTION_PREFIX void send_broadcast_msg(char * msg);
FUNCTION_PREFIX void announce_me_to(uid_t * uid);
FUNCTION_PREFIX void discovery_neighborhood(uint8_t depth);
FUNCTION_PREFIX void send_neighboor_request_to(uid_t * uid);
FUNCTION_PREFIX uint8_t is_blocked(uid_t * uid);
FUNCTION_PREFIX void block_uid(uid_t * uid);

#endif
