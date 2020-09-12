/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <string.h>
#include "multizone.h"


int main (void){

	while(1){

		// Message handler
		char msg[16];
		if (MZONE_RECV(1, msg)) {
			if (strcmp("ping", msg)==0) MZONE_SEND(1, "pong");
			else if (strcmp("mie=0", msg)==0) CSRC(mstatus, 1<<3);
			else if (strcmp("mie=1", msg)==0) CSRS(mstatus, 1<<3);
			else if (strcmp("block", msg)==0) {
				CSRC(mstatus, 1<<3);
				while(!MZONE_RECV(1, msg)) {;}
				CSRS(mstatus, 1<<3);
			}
			else MZONE_SEND(1, msg);
		}

		MZONE_WFI();

	}

}
