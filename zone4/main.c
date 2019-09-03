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

		case 0x80000000+7 : ECALL_SEND(1, "IRQ TMR");
			ECALL_WRTIMECMP(ECALL_RDTIME() + 3000*RTC_FREQ/1000); // clear mip
			break;

		case 0x80000000+16+BTN3 : ECALL_SEND(1, "IRQ BTN3");
			GPIO_REG(GPIO_RISE_IP) |= (1<<BTN3); //clear mip
			break;

		default: break;
	}

}

int main (void){

	//volatile int i=0; while(1){i++;}
	//while(1) ECALL_YIELD();

	CSRW(mtvec, trap_handler);

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN3);
    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN3);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN3);
    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN3);

    // enable irq
    CSRRS(mie, 1<<(16+BTN3));

    // set timer += 3sec
	ECALL_WRTIMECMP(ECALL_RDTIME() + 3000*RTC_FREQ/1000);
    CSRRS(mie, 1<<7);


	while(1){

		//ECALL_YIELD();

		while (!ECALL_SEND(1, ("WFI in")));
		ECALL_WFI();
		while (!ECALL_SEND(1, ("WFI out")));

	}

}
