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

struct in_addr* get_net(struct in_addr*,struct in_addr*);



#endif /* UTILITY_H_ */
