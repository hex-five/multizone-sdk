/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <string.h>	// strcmp()
#include <stdint.h>

#include "platform.h"
#include "multizone.h"

__attribute__((interrupt())) void trap_handler(void){

	switch(ECALL_CSRR(CSR_MCAUSE)){
		case 0 : break; // Instruction address misaligned
		case 1 : break; // Instruction access fault
		case 3 : break; // Breakpoint
		case 4 : break; // Load address misaligned
		case 5 : break; // Load access fault
		case 6 : break; // Store/AMO address misaligned
		case 7 : break; // Store access fault
		case 8 : break; // Environment call from U-mode

		case 0x80000007 : {
			ECALL_SEND(1, "IRQ 7 [TMR]");
			ECALL_SETTIMECMP(+5*RTC_FREQ); // clear mip
		} break;

		case 0x80000000+16+BTN3 : {
			static uint64_t debounce = 0;
			const uint64_t T = ECALL_RDTIME();
			if (T > debounce){
				debounce = T + 250*RTC_FREQ/1000;
				ECALL_SEND(1, "IRQ 23 [BTN3]");
			}
			GPIO_REG(GPIO_RISE_IP) |= (1<<BTN3); //clear gpio irq
		} break;

	}

}

int main (void){

	//volatile int i=0; while(1){i++;}
	//while(1) ECALL_YIELD();
	//while(1) ECALL_WFI();

	CSRW(mtvec, trap_handler);

    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN3);
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN3);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN3);
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN3);

    // enable BTN3 irq
    CSRS(mie, 1<<(16+BTN3));

    // set timer += 5sec
	ECALL_SETTIMECMP(+5*RTC_FREQ);
    CSRS(mie, 1<<7);
    //CSRS(mstatus, 1<<3);

	while(1){

		// Message handler
		char msg[16]; if (ECALL_RECV(1, msg)) {
			if (strcmp("ping", msg)==0) ECALL_SEND(1, "pong");
			else if (strcmp("mie=0", msg)==0) CSRC(mstatus, 1<<3);
			else if (strcmp("mie=1", msg)==0) CSRS(mstatus, 1<<3);
			else if (strcmp("block", msg)==0) {volatile int i=0; while(1) i++; }
			else if (strcmp("loop",  msg)==0) {while(1) ECALL_YIELD();}
		}

		ECALL_WFI();

	}

}
