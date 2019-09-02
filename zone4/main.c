/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

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
		default: break;
	}

}

int main (void){

	CSRW(mtvec, trap_handler);

	while(1) ECALL_YIELD();
	//while(1) ECALL_WFI();
	//volatile int i=0; while(1){i++;}

}
