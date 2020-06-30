/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <string.h>
#include "multizone.h"

int main (void){

	//volatile int i=0; while(1){i++;}
	//while(1) ECALL_YIELD();
	//while(1) ECALL_WFI();

	while(1){

		// Message handler
		for(int zone=1; zone<4; zone++){

			char msg[16];
			if (ECALL_RECV(zone, msg)) {
				if (strcmp("ping", msg)==0) ECALL_SEND(zone, "pong");
				else if (strcmp("PING", msg)==0) ECALL_SEND(zone, "PONG");
				else ECALL_SEND(zone, msg);
			}

		}

		ECALL_WFI();

	}

}
