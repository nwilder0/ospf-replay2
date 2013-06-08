/*
 * utility.c
 *
 *  Created on: May 27, 2013
 *      Author: nathan
 */
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
#include <math.h>

#include "event.h"
#include "interface.h"
#include "load.h"
#include "neighbor.h"
#include "packet.h"
#include "prefix.h"
#include "utility.h"
#include "replay.h"

void replay_error(char* mesg) {
	if(replay0->errors) {
		fprintf(replay0->errors,"%s\n",mesg);
	}
	else {
		printf("%s\n", mesg);
	}

}

void replay_log(char* mesg) {
	if(replay0->events) {
		fprintf(replay0->events, "%s\n",mesg);
	}
	else {
		printf("%s\n",mesg);
	}

}

const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

struct replay_list* add_to_list(struct replay_list *list, struct replay_list *new) {

	struct replay_list *curr;
	curr = list;
	if(new) {
		if(list) {
			while(curr->next) {
				curr = curr->next;
			}
			curr->next = new;
			return list;
		}
		else {
			return new;
		}
	}
	else {
		return list;
	}
}

struct replay_list* remove_from_list(struct replay_list *list, struct replay_list *remove) {

	struct replay_list *curr, *prev;
	curr = list;
	prev = NULL;
	if(remove) {
		if(list) {
			while(curr != remove) {
				prev = curr;
				curr = curr->next;
			}
			if(prev) {
				prev->next = curr->next;
				free(curr);
				return list;
			}
			else {
				free(curr);
				return curr->next;
			}
			return list;
		}
		else {
			return list;
		}
	}
	else {
		return list;
	}
}

uint32_t get_net(uint32_t addr, uint32_t mask) {

	uint32_t net_addr;

	net_addr = addr & mask;

	return net_addr;
}

uint32_t bits2mask(int bits) {

	uint32_t mask = 0;

	mask = pow(2,bits)-1;

	mask = mask << (32-bits);

	mask = htonl(mask);

	return mask;
}

int mask2bits(uint32_t mask) {

	int bits = 0;

	for(bits = 0; mask; mask >>= 1)
	{
	  bits += mask & 1;
	}

	return bits;
}

