/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <string.h>
#include "multizone.h"

int main (void){

	//volatile int i=0; while(1){i++;}
	//while(1) MZONE_YIELD();
	//while(1) MZONE_WFI();

	while(1){

		// Message handler
		for(int zone=1; zone<4; zone++){

			char msg[16];
			if (MZONE_RECV(zone, msg)) {
				if (strcmp("ping", msg)==0) MZONE_SEND(zone, "pong");
				else if (strcmp("PING", msg)==0) MZONE_SEND(zone, "PONG");
				else MZONE_SEND(zone, msg);
			}

		}

		MZONE_WFI();

	}

}
