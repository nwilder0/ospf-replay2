/*
 * utility.h
 *
 *  Created on: May 27, 2013
 *      Author: nathan
 */
#ifndef UTILITY_H_
#define UTILITY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "event.h"
#include "interface.h"
#include "load.h"
#include "neighbor.h"
#include "packet.h"
#include "prefix.h"
#include "replay.h"

struct replay_object {

};

struct replay_list {
	struct replay_list *next;
	struct replay_object *object;
};


void replay_error(char*);

void replay_log(char*);

struct replay_list* add_to_list(struct replay_list*,struct replay_list*);

struct replay_list* remove_from_list(struct replay_list*,struct replay_list*);

const char* byte_to_binary(int);

uint32_t get_net(uint32_t,uint32_t);

uint32_t bits2mask(int);

int mask2bits(uint32_t);

#endif /* UTILITY_H_ */
