/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <string.h>
#include "multizone.h"


int main (void){

	while(1){

		// Message handler
		for(int zone=1; zone<4; zone++){

			char msg[16];
			if (MZONE_RECV(zone, msg)) {
				if (strcmp("ping", msg)==0) MZONE_SEND(zone, "pong");
				else MZONE_SEND(zone, msg);
			}

		}

		MZONE_WFI();

	}

}
