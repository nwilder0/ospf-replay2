/*
 * utility.h
 *
 *  Created on: May 27, 2013
 *      Author: nathan
 */

#ifndef UTILITY_H_
#define UTILITY_H_

struct replay_list {
	struct replay_list *next;
};

void replay_error(char*);

void replay_log(char*);

struct replay_list* add_to_list(struct replay_list*,struct replay_list*);

void remove_from_list(struct replay_list*,struct replay_list*);

const char* byte_to_binary(int);

uint32_t get_net(uint32_t,uint32_t);

uint32_t bits2mask(int);

int mask2bits(uint32_t);

#endif /* UTILITY_H_ */
