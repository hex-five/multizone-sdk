/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <string.h>
#include "multizone.h"

__attribute__(( interrupt())) void trap_handler(void){

	for( ;; );

}

int main (void){

	//while(1) MZONE_WFI();
	//while(1) MZONE_YIELD();
	//while(1);

	CSRW(mtvec, trap_handler); // register trap handler
	CSRS(mie, 1<<3); 		   // wake up on msip/inbox
	CSRC(mstatus, 1<<3);	   // disable global interrupts - no irq taken

	while (1) {

	    // Synchronous message handling example
		char msg[16]; if (MZONE_RECV(1, msg)) {

			if (strcmp("ping", msg) == 0)
				MZONE_SEND(1, (char[16]){"pong"});

			/* test: wfi resume with global irq disabled - no irq taken */
			else if (strcmp("mstatus.mie=0", msg)==0)
				CSRC(mstatus, 1<<3);

			/* test: wfi resume with global irq enabled - irqs taken */
			else if (strcmp("mstatus.mie=1", msg)==0)
				CSRS(mstatus, 1<<3);

            /* test: preemptive scheduler - block for good */
            else if (strcmp("block", msg) == 0)
                for( ;; );

			else
				MZONE_SEND(1, msg);
		}

		// Suspend waiting for incoming msg
		MZONE_WFI();

	}

}
