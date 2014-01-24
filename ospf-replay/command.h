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
#include <string.h>
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

int process_command(char*);
void command_logging(char*);
void command_router(char*);
void command_if(char*);
void show_nbrs();
void print_prompt();

#endif /* COMMAND_H_ */
