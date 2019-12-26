/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */
#include <string.h>	// strcmp()
#include <inttypes.h> // uint16_t, ...

#include "platform.h"
#include "multizone.h"

#define LD1_RED_ON PWM_REG(PWM_CMP1)  = 0x0;
#define LD1_GRN_ON PWM_REG(PWM_CMP2)  = 0x0;
#define LD1_BLU_ON PWM_REG(PWM_CMP3)  = 0x0;

#define LD1_RED_OFF PWM_REG(PWM_CMP1)  = 0xFF;
#define LD1_GRN_OFF PWM_REG(PWM_CMP2)  = 0xFF;
#define LD1_BLU_OFF PWM_REG(PWM_CMP3)  = 0xFF;

__attribute__((interrupt())) void trp_handler(void)	 { // trap handler

	const unsigned long mcause = ECALL_CSRR(CSR_MCAUSE);

	switch (mcause) {
	case 0:	break; // Instruction address misaligned
	case 1:	break; // Instruction access fault
	case 3:	break; // Breakpoint
	case 4:	break; // Load address misaligned
	case 5:	break; // Load access fault
	case 6:	break; // Store/AMO address misaligned
	case 7:	break; // Store access fault
	case 8:	break; // Environment call from U-mode
	}

	asm volatile("ebreak");
}
__attribute__((interrupt())) void msi_handler(void)  { // machine software interrupt (3)
}
__attribute__((interrupt())) void tmr_handler(void)  { // machine timer interrupt (7)

	static uint16_t r=0x3F;
	static uint16_t g=0;
	static uint16_t b=0;

	if (r > 0 && b == 0) {r--; g++;}
	if (g > 0 && r == 0) {g--; b++;}
	if (b > 0 && g == 0) {r++; b--;}

	PWM_REG(PWM_CMP1) = 0xFF - (r >> 2);
	PWM_REG(PWM_CMP2) = 0xFF - (g >> 2);
	PWM_REG(PWM_CMP3) = 0xFF - (b >> 2);

	// set timer (clears mip)
	const uint64_t T = ECALL_RDTIME(); ECALL_WRTIMECMP(T + 25*RTC_FREQ/1000);

}

__attribute__((interrupt())) void btn0_handler(void) { // global interrupt (11)

	static uint64_t debounce = 0;

	const uint64_t T = ECALL_RDTIME();

	if (T > debounce){

		debounce = T + 250*RTC_FREQ/1000;

		ECALL_WRTIMECMP(debounce);

		LD1_RED_OFF; LD1_GRN_ON; LD1_BLU_OFF;

		ECALL_SEND(1, "IRQ 20 [BTN0]");

		GPIO_REG(GPIO_RISE_IP) |= (1<<BTN0); //clear gpio irq

	}

}
__attribute__((interrupt())) void btn1_handler(void) { // local interrupt (16+5)

	static uint64_t debounce = 0;

	const uint64_t T = ECALL_RDTIME();

	if (T > debounce){

		debounce = T + 250*RTC_FREQ/1000;

		ECALL_WRTIMECMP(debounce);

		LD1_RED_ON; LD1_GRN_OFF; LD1_BLU_OFF;

		ECALL_SEND(1, "IRQ 21 [BTN1]");

		GPIO_REG(GPIO_RISE_IP) |= (1<<BTN1); //clear gpio irq

	}

}
__attribute__((interrupt())) void btn2_handler(void) { // local interrupt (16+6)

	static uint64_t debounce = 0;

	const uint64_t T = ECALL_RDTIME();

	if (T > debounce){

		debounce = T + 250*RTC_FREQ/1000;

		ECALL_WRTIMECMP(debounce);

		LD1_BLU_ON; LD1_GRN_OFF; LD1_RED_OFF;

		ECALL_SEND(1, "IRQ 22 [BTN2]");

		GPIO_REG(GPIO_RISE_IP) |= (1<<BTN2); //clear gpio irq

	}

}

__attribute__((naked, aligned(4))) void trap_vect(void) { // irqs vector

	asm (
		"j trp_handler;"	//  0
		".word 0x0;" //  1
		".word 0x0;" //  2
		"j msi_handler;" 	//  3
		".word 0x0;" //  4
		".word 0x0;" //  5
		".word 0x0;" //  6
		"j tmr_handler;" 	//  7
		".word 0x0;" //  8
		".word 0x0;" //  9
		".word 0x0;" // 10
		".word 0x0;" // 11
		".word 0x0;" // 12
		".word 0x0;" // 13
		".word 0x0;" // 14
		".word 0x0;" // 15
		".word 0x0;" // 16
		".word 0x0;" // 17
		".word 0x0;" // 18
		".word 0x0;" // 19
		"j btn0_handler;"	// 20
		"j btn1_handler;"	// 21
		"j btn2_handler;"	// 22
		".word 0x0;" // 23
		".word 0x0;" // 24
		".word 0x0;" // 25
		".word 0x0;" // 26
		".word 0x0;" // 27
		".word 0x0;" // 28
		".word 0x0;" // 29
		".word 0x0;" // 30
		".word 0x0;" // 31
	);
}

/*configures Button0 as global interrupt*/
void b0_irq_init()  {

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN0);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN0);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN0);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN0);

    // enable irq
    CSRS(mie, 1<<(16+BTN0));

}

/*configures Button1 as local interrupt*/
void b1_irq_init()  {

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN1);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN1);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN1);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN1);

    // enable irq
    CSRS(mie, 1<<(16+BTN1));

}

/*configures Button2 as local interrupt*/
void b2_irq_init()  {

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN2);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN2);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN2);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN2);

    // enable irq
    CSRS(mie, 1<<(16+BTN2));
}

int main (void){

	//volatile int w=0; while(1){w++;}
	//while(1) ECALL_YIELD();
	//while(1) ECALL_WFI();

	PWM_REG(PWM_CFG)   = (PWM_CFG_ENALWAYS) | (PWM_CFG_ZEROCMP) | (PWM_CFG_DEGLITCH);
	PWM_REG(PWM_COUNT) = 0;
	PWM_REG(PWM_CMP0)  = 0xFE;

	b0_irq_init();
	b1_irq_init();
	b2_irq_init();

	// vectored trap handler
	CSRW(mtvec, trap_vect); CSRS(mtvec, 0b1);

    // set & enable timer
	const uint64_t T = ECALL_RDTIME(); ECALL_WRTIMECMP(T + 25*RTC_FREQ/1000);
    CSRS(mie, 1<<7);

    // enable global interrupts (BTN0, BTN1, BTN2, TMR)
    CSRS(mstatus, 1<<3);

    const unsigned long MIE = CSRR(mie); // save default value

	while(1){

		// Message handler
		char msg[16]; if (ECALL_RECV(1, msg)) {
			if (strcmp("ping", msg)==0) ECALL_SEND(1, "pong");
			else if (strcmp("mie=0", msg)==0) CSRC(mstatus, 1<<3);
			else if (strcmp("mie=1", msg)==0) CSRS(mstatus, 1<<3);
		}

		// Wait For Interrupt
		ECALL_WFI();

	}

}



