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
using namespace std;
using namespace rUtility;
using namespace rList;

token::token(){

	count = 3;
	tokens = new char[3];
	token[0] = ' ';
	token[1] = '\n';
	token[2] = '\t';

}

token::token(char* tks, int count) {
	this->count = count;
	tokens = new char[count];
	while(--count>=0) {

		tokens[count] = &(tks + sizeof(char)*count);

	}

}

string* token::split(string s) {
	delete strings;

	int tCount=0;
	for(int i = 0; i<s.size();i++) {
		for(int j = 0; j<count; j++) {
			if(s[i]==tokens[j]) tCount++;
		}
	}
	strings = new string[tCount];
	tCount = 0;
	bool bSplit=false;
	for(int i = 0; i<s.size();i++) {
		for(int j = 0; j<count; j++) {
			if(s[i]==tokens[j]) bSplit = true;
		}
		if(bSplit) {
			tCount++;
		} else {
			strings[tCount] = strings[tCount] + s[i];
		}
	}
	return strings;

}

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

rList::rList() {

	this->next = NULL;
	this->object = NULL;
}

rList::rList(void *obj) {
	this->next = NULL;
	this->object = obj;
}

rList* rList::add(void *obj) {

	rList *curr,*newL;

	newL = new rList(obj);

	curr = this;
	if(newL) {
		if(this) {
			while(curr->next) {
				curr = curr->next;
			}
			curr->next = newL;
			return this;
		}
		else {
			return newL;
		}
	}
	else {
		return this;
	}
}

rList* rList::find(void *obj) {
	struct rList *curr, *found;
	found = NULL;
	curr = this;
	if(obj && curr) {
		while(curr->next && (curr->object != obj)) curr = curr->next;
		if(curr->object == obj) found = curr;
	}
	return found;
}

rList* rList::remove(rList *remove) {

	rList *curr, *prev, *list;
	list = this;
	curr = list;
	prev = NULL;
	if(remove && list) {
		while(curr && (curr != remove)) {
			prev = curr;
			curr = curr->next;
		}
		if(curr==remove) {
			if(prev) {
				prev->next = curr->next;
				free(curr);
			}
			else {
				list = curr->next;
				free(curr);
			}
		}
	}
	return list;
}



void rList::removeAll() {
	rList *next,*curr;
	curr=this;
	while(curr) {
		next=curr->next;
		free(curr);
		curr=next;
	}
}

void rList::deleteAll() {
	rList *next,*curr;
	curr = this;
	while(curr) {
		next = curr->next;
		free(curr->object);
		free(curr);
		curr = next;
	}
}





rnList::rnList() : rList() {

	key = 0;

}

rnList::rnList(void *obj, unsigned long long key) : rList(obj) {

	this->key = key;

}

rnList* rnList::add(void *obj,unsigned long long key) {

	rnList *curr, *prev, *newL, *list;
	curr = list = this;
	prev = NULL;
	if(obj) {
		newL = new rnList::rnList(obj,key);

		if(list) {
			while(curr && (newL->key > curr->key)) {
				prev = curr;
				curr = curr->next;
			}
			if(prev) {
				prev->next = newL;
			} else {
				list = newL;
			}
			newL->next = curr;
		}
		else {
			list = newL;
		}
	}
	return list;

}

rList* rList::merge(rList *list2) {
	rList *curr;
	curr = list2;
	while(curr) {
		if(!(find(curr->object))) {
			this = this->add(curr->object);
		}
		curr = curr->next;
	}
	return this;
}

rnList* rnList::merge(rnList *list2) {
	rnList *curr;
	curr = list2;
	while(curr) {
		if(!(find(curr->object))) {
			this = this->add(curr->object,curr->key);
		}
		curr = curr->next;
	}
	return this;
}
