/*
 * utility.c
 *
 *  Created on: May 27, 2013
 *      Author: nathan
 */

#include "replay.h"
#include "utility.h"

extern struct replay_config *replay0;

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
				return list;
			}
			else {
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

struct in_addr* get_net(struct in_addr *addr, struct in_addr *mask) {

	struct in_addr *net;
	net = (struct in_addr *) malloc(sizeof(struct in_addr));
	bzero((char *) &net, sizeof(net));

	net->s_addr = addr->s_addr & mask->s_addr;

	return net;
}

uint32_t bits2mask(int bits) {

	uint32_t mask = 0;




}
