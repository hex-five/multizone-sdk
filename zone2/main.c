/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */
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

__attribute__((interrupt())) void trp_handler(void)	 { // trap handler (0)

	const unsigned long mcause = MZONE_CSRR(CSR_MCAUSE);

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

}
__attribute__((interrupt())) void msi_handler(void)  { // machine software interrupt (3)
	asm volatile("ebreak");
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
	MZONE_ADTIMECMP((uint64_t)25*RTC_FREQ/1000);

}

__attribute__((interrupt())) void btn0_handler(void) {

	static uint64_t debounce = 0;
	const uint64_t T = MZONE_RDTIME();
	if (T > debounce){
		debounce = T + 250*RTC_FREQ/1000;
		MZONE_SEND(1, "IRQ BTN0");
		LD1_RED_OFF; LD1_GRN_ON; LD1_BLU_OFF;
		MZONE_ADTIMECMP((uint64_t)250*RTC_FREQ/1000);
	}
	GPIO_REG(GPIO_HIGH_IP) |= (1<<BTN0); //clear gpio irq

}
__attribute__((interrupt())) void btn1_handler(void) {

	static uint64_t debounce = 0;
	const uint64_t T = MZONE_RDTIME();
	if (T > debounce){
		debounce = T + 250*RTC_FREQ/1000;
		MZONE_SEND(1, "IRQ BTN1");
		LD1_RED_ON; LD1_GRN_OFF; LD1_BLU_OFF;
		MZONE_ADTIMECMP((uint64_t)250*RTC_FREQ/1000);
	}
	GPIO_REG(GPIO_HIGH_IP) |= (1<<BTN1); //clear gpio irq

}
__attribute__((interrupt())) void btn2_handler(void) {

	static uint64_t debounce = 0;
	const uint64_t T = MZONE_RDTIME();
	if (T > debounce){
		debounce = T + 250*RTC_FREQ/1000;
		MZONE_SEND(1, "IRQ BTN2");
		LD1_RED_OFF; LD1_GRN_OFF; LD1_BLU_ON;
		MZONE_ADTIMECMP((uint64_t)250*RTC_FREQ/1000);
	}
	GPIO_REG(GPIO_HIGH_IP) |= (1<<BTN2); //clear gpio irq

}

// configures Button0 as local interrupt
void b0_irq_init()  {

    // disable hw io function
    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN0);

    // set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN0);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN0);

    // set to interrupt on rising edge
    GPIO_REG(GPIO_HIGH_IE)    |= (1<<BTN0);

    // enable irq
    CSRS(mie, 1<<(BTN0_IRQ));

}

// configures Button1 as local interrupt
void b1_irq_init()  {

    // disable hw io function
    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN1);

    // set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN1);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN1);

    // set to interrupt on rising edge
    GPIO_REG(GPIO_HIGH_IE)    |= (1<<BTN1);

    // enable irq
    CSRS(mie, 1<<(BTN1_IRQ));

}

// configures Button2 as local interrupt
void b2_irq_init()  {

    // disable hw io function
    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN2);

    // set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN2);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN2);

    //s et to interrupt on rising edge
    GPIO_REG(GPIO_HIGH_IE)    |= (1<<BTN2);

    // enable irq
    CSRS(mie, 1<<(BTN2_IRQ));
}

int main (void){

	//while(1) MZONE_WFI();
	//while(1) MZONE_YIELD();
	//while(1);

	// vectored trap handler
	static void (*trap_vect[32])(void) = {};
	trap_vect[0] = trp_handler;
	trap_vect[3] = msi_handler;
	trap_vect[7] = tmr_handler;
	trap_vect[BTN0_IRQ] = btn0_handler;
	trap_vect[BTN1_IRQ] = btn1_handler;
	trap_vect[BTN2_IRQ] = btn2_handler;
	CSRW(mtvec, trap_vect);	CSRS(mtvec, 0x1);

	PWM_REG(PWM_CFG)   = (PWM_CFG_ENALWAYS | PWM_CFG_ZEROCMP);
	PWM_REG(PWM_CMP0)  = 0xFE;

	b0_irq_init();
	b1_irq_init();
	b2_irq_init();

    // set & enable timer
	MZONE_ADTIMECMP((uint64_t)25*RTC_FREQ/1000);
    CSRS(mie, 1<<7);

    // enable global interrupts (BTN0, BTN1, BTN2, TMR)
    CSRS(mstatus, 1<<3);

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

		// Wait For Interrupt
		MZONE_WFI();

	}

}



