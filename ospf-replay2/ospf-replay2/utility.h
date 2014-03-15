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
#include "checksum.h"

using namespace std;

namespace rUtility {

	class rList {

		rList *next;
		void *object;

		rList::rList();
		rList::rList(void*);
		rList* add(void*);
		rList* remove(rList*);
		rList* find(void*);
		rList* merge(rList*);
		void removeAll();
		void deleteAll();
	};

	class rnList : rList {

		unsigned long long key;

		rnList::rnList();
		rnList::rnList(void*,unsigned long long);
		rnList* add(void*,unsigned long long);
		rnList* merge(rnList*);

	};

	class token {
		char tokens[];
		string strings[];
		int count;

		string* split(string s);

		token();
		token(char*,int);
	};

	void replay_error(char*);

	void replay_log(char*);


	const char* byte_to_binary(int);

	uint32_t get_net(uint32_t,uint32_t);

	u_int8_t ip_in_net(u_int32_t,u_int32_t,u_int32_t);

	uint32_t bits2mask(int);

	int mask2bits(uint32_t);

}
#endif /* UTILITY_H_ */
