/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <platform.h>
#include <plic_driver.h>
#include <multizone.h>


#define LD1_RED_ON PWM_REG(PWM_CMP1)  = 0x0;
#define LD1_GRN_ON PWM_REG(PWM_CMP2)  = 0x0;
#define LD1_BLU_ON PWM_REG(PWM_CMP3)  = 0x0;

#define LD1_RED_OFF PWM_REG(PWM_CMP1)  = 0xFF;
#define LD1_GRN_OFF PWM_REG(PWM_CMP2)  = 0xFF;
#define LD1_BLU_OFF PWM_REG(PWM_CMP3)  = 0xFF;

// Global Instance data for the PLIC for use by the PLIC Driver.
plic_instance_t g_plic;

__attribute__((interrupt())) void trp_handler(void) { // synchronous traps (0)
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
__attribute__((interrupt())) void msi_handler(void) {} // machine software interrupt (3)
__attribute__((interrupt())) void tmr_handler(void) {} // machine timer interrupt (7)
__attribute__((interrupt())) void button_0_handler(void) { // global interrupt (11)

	plic_source int_num  = PLIC_claim_interrupt(&g_plic); // claim

	ECALL_SEND(1, ((int[4]){201,0,0,0}));

	LD1_GRN_ON; LD1_RED_OFF; LD1_BLU_OFF;

	const uint64_t T1 = ECALL_RDTIME() + 3*RTC_FREQ;
	while (ECALL_RDTIME() < T1) ECALL_YIELD();

	LD1_RED_OFF; LD1_GRN_OFF; LD1_BLU_OFF;

	GPIO_REG(GPIO_RISE_IP) |= (1<<BTN0); //clear gpio irq

	PLIC_complete_interrupt(&g_plic, int_num); // complete

}
__attribute__((interrupt())) void button_1_handler(void) { // local interrupt (16+5)

	ECALL_SEND(1, ((int[4]){190+16+LOCAL_INT_BTN_1,0,0,0}));

	LD1_RED_ON; LD1_GRN_OFF; LD1_BLU_OFF;

	const uint64_t T1 = ECALL_RDTIME() + 3*RTC_FREQ;
	while (ECALL_RDTIME() < T1) ECALL_YIELD();

	LD1_RED_OFF; LD1_GRN_OFF; LD1_BLU_OFF;

	GPIO_REG(GPIO_RISE_IP) |= (1<<BTN1); //clear gpio irq

}
__attribute__((interrupt())) void button_2_handler(void) { // local interrupt (16+6)

	ECALL_SEND(1, ((int[4]){190+16+LOCAL_INT_BTN_2,0,0,0}));

	LD1_BLU_ON; LD1_GRN_OFF; LD1_RED_OFF;

	const uint64_t T1 = ECALL_RDTIME() + 3*RTC_FREQ;
	while (ECALL_RDTIME() < T1) ECALL_YIELD();

	LD1_RED_OFF; LD1_GRN_OFF; LD1_BLU_OFF;

	GPIO_REG(GPIO_RISE_IP) |= (1<<BTN2); //clear gpio irq

}
__attribute__((aligned(2))) void trap_vector(void)  {

	asm (
		"j trp_handler;"	//  0
		"jr (x0);"	//  1
		"jr (x0);"	//  2
		"j msi_handler;" 	//  3
		"jr (x0);" 	//  4
		"jr (x0);" 	//  5
		"jr (x0);" 	//  6
		"j tmr_handler;" 	//  7
		"jr (x0);" 	//  8
		"jr (x0);" 	//  9
		"jr (x0);" 	// 10
		"j button_0_handler;" // 11
		"jr (x0);" 	// 12
		"jr (x0);" 	// 13
		"jr (x0);" 	// 14
		"jr (x0);" 	// 15
		"jr (x0);" 	// 16
		"jr (x0);" 	// 17
		"jr (x0);" 	// 18
		"jr (x0);" 	// 19
		"jr (x0);" 	// 20
		"j button_1_handler;" // 21
		"j button_2_handler;" // 22
		"jr (x0);" 	// 23
		"jr (x0);" 	// 24
		"jr (x0);" 	// 25
		"jr (x0);" 	// 26
		"jr (x0);" 	// 27
		"jr (x0);" 	// 28
		"jr (x0);" 	// 29
		"jr (x0);" 	// 30
		"jr (x0);" 	// 31
	);

}

/*configures Button0 as a global gpio irq*/
void b0_irq_init()  {

    uint32_t irqnum;

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN0);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN0);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN0);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN0);

    PLIC_init(&g_plic,
  	    PLIC_BASE,
  	    PLIC_NUM_INTERRUPTS,
  	    PLIC_NUM_PRIORITIES);

    // GPIO irq offset depends on the core version
    irqnum = CSRR(mvendorid) == 0x489 && CSRR(mimpid) > 0x20190000 ?
    		1 + BTN0 :
			GPIO_INT_BASE + BTN0;

    PLIC_enable_interrupt (&g_plic, irqnum);
    PLIC_set_priority(&g_plic, irqnum, 2);

    // enable PLIC ext irq
    CSRRS(mie, 1<<11);


}

/*configures Button1 as a local interrupt*/
void b1_irq_init()  {

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN1);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN1);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN1);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN1);

    // enable irq
    CSRRS(mie, 1<<(16+BTN1));

}

/*configures Button2 as a local interrupt*/
void b2_irq_init()  {

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN2);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN2);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN2);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN2);

    // enable irq
    CSRRS(mie, 1<<(16+BTN2));
}

int main (void){

	//volatile int w=0; while(1){w++;}
	//while(1) ECALL_YIELD();

	// vectored trap handler
	CSRW(mtvec, trap_vector+1);

	b0_irq_init();
	b1_irq_init();
	b2_irq_init();

	#ifdef IOF1_PWM1_MASK
	GPIO_REG(GPIO_IOF_EN) |= IOF1_PWM1_MASK;
	GPIO_REG(GPIO_IOF_SEL) |= IOF1_PWM1_MASK;
	#endif

	PWM_REG(PWM_CFG)   = 0;
	PWM_REG(PWM_CFG)   = (PWM_CFG_ENALWAYS) | (PWM_CFG_ZEROCMP) | (PWM_CFG_DEGLITCH);
	PWM_REG(PWM_COUNT) = 0;

	// The LEDs are intentionally left somewhat dim.
	PWM_REG(PWM_CMP0)  = 0xFE;

	uint16_t r=0x3F;
	uint16_t g=0;
	uint16_t b=0;

	unsigned long T1 = 0;

	while(1){

		uint64_t T = ECALL_RDTIME();

		if (T > T1){

			T1 = T + 12*RTC_FREQ/1000;

			if (r > 0 && b == 0) {r--; g++;}
			if (g > 0 && r == 0) {g--; b++;}
			if (b > 0 && g == 0) {r++; b--;}

			PWM_REG(PWM_CMP1) = 0xFF - (r >> 2);
			PWM_REG(PWM_CMP2) = 0xFF - (g >> 2);
			PWM_REG(PWM_CMP3) = 0xFF - (b >> 2);

		}

		int msg[4]={0,0,0,0};
		if (ECALL_RECV(1, msg)) {
			switch (msg[0]) {
			//case '1': ECALL_CSRS_MIE();	break;
			//case '0': ECALL_CSRC_MIE();	break;
			case 'p': ECALL_SEND(1, ((int[4]){'p','o','n','g'})); break;
			}
		}

		ECALL_YIELD();

	}

}


/*
//static const void (* trap_vector[32])(void) __attribute__((aligned(64))) = {
  static void (* const trap_vector[32])(void) __attribute__((aligned(64))) = {
	NULL,				// 0
	button_0_handler, 	// 11 (PLIC - Source LOCAL_INT_BTN_0)
	NULL,				// 31
};
*/
