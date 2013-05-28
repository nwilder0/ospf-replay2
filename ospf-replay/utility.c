/*
 * utility.c
 *
 *  Created on: May 27, 2013
 *      Author: nathan
 */

#include "replay.h"

extern struct replay_config *replay;

void replay_error(char* mesg) {
	if(replay->errors) {
		fprintf(replay->errors,"%s\n",mesg);
	}
	else {
		printf("%s\n", mesg);
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
