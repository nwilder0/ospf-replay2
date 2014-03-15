/*
 * command.h
 *
 *  Created on: Sep 8, 2013
 *      Author: nathan
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/time.h>
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
#include "prefix.h"
#include "utility.h"
#include "replay.h"
#include "packet.h"
using namespace std;

int process_command(char*);
void command_logging(char*);
void command_router(char*);
void command_if(char*);
void show_nbrs();
void print_prompt();

enum cmd_type {log,rtr,iface};

class command {

	cmd_type type;
	string full, cmd;
	string param[];

	command(string str);
	virtual int run();
	virtual int display();
	virtual ~command();
};

class show : command {

	show(string str);

};

class rtr : command {

	rtr(string str);

};

class log : command {

	log(string str);
};

class iface : command {

	iface(string str);

};

class show_nbrs : show {

	show_nbrs(string str);
};

#endif /* COMMAND_H_ */
