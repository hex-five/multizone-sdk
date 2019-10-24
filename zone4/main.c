/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <stdint.h>
#include <platform.h>
#include <multizone.h>

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

		case 0x80000000+7 :
			ECALL_SEND(1, "IRQ TMR");
			ECALL_WRTIMECMP(ECALL_RDTIME() + 5*RTC_FREQ); // clear mip
			break;

		case 0x80000000+16+BTN3 :
			//volatile unsigned long mstatus = CSRR(mstatus);
			//volatile unsigned long mie = CSRR(mie);
			//volatile unsigned long mip = CSRR(mip);
			//CSRC(mstatus, 0x80);
			//ECALL_SEND(1, "IRQ BTN3");
			GPIO_REG(GPIO_RISE_IP) |= (1<<BTN3); //clear mip
			break;

	}

}

int main (void){

	//volatile int i=0; while(1){i++;}
	//while(1) ECALL_YIELD();
	while(1) ECALL_WFI();

	CSRW(mtvec, trap_handler);

    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN3);
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN3);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN3);
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN3);

    // enable BTN3 irq
    CSRS(mie, 1<<(16+BTN3));

    // set timer += 10sec
	ECALL_WRTIMECMP(ECALL_RDTIME() + 10*RTC_FREQ);
    CSRS(mie, 1<<7);

    // enable global interrupts
    CSRS(mstatus, 1<<3);

	while(1){

		ECALL_WFI();

		int msg[4]={0,0,0,0};
		if (ECALL_RECV(1, msg)) {
			switch (msg[0]) {
			case '1': CSRS(mstatus, 1<<3);	break;
			case '0': CSRC(mstatus, 1<<3);	break;
			case 'p': ECALL_SEND(1, ((int[4]){'p','o','n','g'})); break;
			}
		}

	}

}
