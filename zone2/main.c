/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <unistd.h>  // NULL
#include <platform.h>
#include <plic_driver.h>
#include <libhexfive.h>


#define LD1_RED_ON PWM_REG(PWM_CMP1)  = 0x00;
#define LD1_GRN_ON PWM_REG(PWM_CMP2)  = 0x00;
#define LD1_BLU_ON PWM_REG(PWM_CMP3)  = 0x00;

#define LD1_RED_OFF PWM_REG(PWM_CMP1)  = 0xFF;
#define LD1_GRN_OFF PWM_REG(PWM_CMP2)  = 0xFF;
#define LD1_BLU_OFF PWM_REG(PWM_CMP3)  = 0xFF;

// Global Instance data for the PLIC for use by the PLIC Driver.
plic_instance_t g_plic;

__attribute__((interrupt())) void button_0_handler(void) { // global interrupt

	int sent = ECALL_SEND(1, (int[4]){201,0,0,0});

	plic_source int_num  = PLIC_claim_interrupt(&g_plic); // claim

	LD1_GRN_ON; LD1_RED_OFF; LD1_BLU_OFF;

	volatile unsigned long T1 = CSRR(time) + 3*RTC_FREQ;
	while (CSRR(time) < T1) ECALL_YIELD();

	LD1_RED_OFF; LD1_GRN_OFF; LD1_BLU_OFF;

	GPIO_REG(GPIO_RISE_IP) |= (1<<BTN0); //clear gpio irq

	PLIC_complete_interrupt(&g_plic, int_num); // complete

}
__attribute__((interrupt())) void button_1_handler(void) { // local interrupt

	ECALL_SEND(1, (int[4]){190+16+LOCAL_INT_BTN_1,0,0,0} );

	LD1_RED_ON; LD1_GRN_OFF; LD1_BLU_OFF;

	volatile unsigned long T1 = CSRR(time) + 3*RTC_FREQ;
	while (CSRR(time) < T1) ECALL_YIELD();

	LD1_RED_OFF; LD1_GRN_OFF; LD1_BLU_OFF;

	GPIO_REG(GPIO_RISE_IP) |= (1<<BTN1); //clear gpio irq

}
__attribute__((interrupt())) void button_2_handler(void) { // local interrupt

	ECALL_SEND(1, (int[4]){190+16+LOCAL_INT_BTN_2,0,0,0});

	LD1_BLU_ON; LD1_GRN_OFF; LD1_RED_OFF;

	volatile unsigned long T1 = CSRR(time) + 3*RTC_FREQ;
	while (CSRR(time) < T1) ECALL_YIELD();

	LD1_RED_OFF; LD1_GRN_OFF; LD1_BLU_OFF;

	GPIO_REG(GPIO_RISE_IP) |= (1<<BTN2); //clear gpio irq

}

/*configures Button0 as a global gpio irq*/
void b0_irq_init()  {

    uint32_t irqnum;

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN )    &=  ~(1 << BTN0);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN0);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN0);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN0);

    PLIC_init(&g_plic,
  	    PLIC_BASE,
  	    PLIC_NUM_INTERRUPTS,
  	    PLIC_NUM_PRIORITIES);

    uint64_t mvendid = CSRR(mvendorid);
    uint64_t mimpid = CSRR(mimpid);
    // GPIO irq offset depends on the core version
    if (mvendid == 0x489 && mimpid > 0x20190000) {
        irqnum = 1 + BTN0;
    } else {
        irqnum = GPIO_INT_BASE + BTN0;
    }

    PLIC_enable_interrupt (&g_plic, irqnum);
    PLIC_set_priority(&g_plic, irqnum, 2);

}

/*configures Button1 as a local interrupt*/
void b1_irq_init()  {

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN ) &=  ~(1 << BTN1);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN1);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN1);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN1);

    // enable irq
    CSRW(mie, 1<<(16+BTN1));

}

/*configures Button2 as a local interrupt*/
void b2_irq_init()  {

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN )    &=  ~(1 << BTN2);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN2);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN2);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN2);

}

static const void (* trap_vector[32])(void) __attribute__((aligned(64))) = {
	NULL,				// 0
	NULL,				// 1
	NULL,				// 2
	NULL,				// 3
	NULL,				// 4
	NULL,				// 5
	NULL,				// 6
	NULL,				// 7
	NULL,				// 8
	NULL,				// 9
	NULL,				// 10
	button_0_handler, 	// 11 (PLIC - Source LOCAL_INT_BTN_0)
	NULL,				// 12
	NULL,				// 13
	NULL,				// 14
	NULL,				// 15
	NULL,				// 16
	NULL,				// 17
	NULL,				// 18
	NULL,				// 19
	NULL,				// 20
	button_1_handler, 	// 21 (16 + LOCAL_INT_BTN_1)
	button_2_handler, 	// 22 (16 + LOCAL_INT_BTN_2)
	NULL,				// 23
	NULL,				// 24
	NULL,				// 25
	NULL,				// 26
	NULL,				// 27
	NULL,				// 28
	NULL,				// 29
	NULL,				// 30
	NULL,				// 31
};

int main (void){

  //volatile int w=0; while(1){w++;}
  while(1) ECALL_YIELD();

  // vectored trap handler
  CSRW(mtvec, ((unsigned long)trap_vector) | 1UL);

  b0_irq_init();
  b1_irq_init();
  b2_irq_init();

  uint16_t r=0x3F;
  uint16_t g=0;
  uint16_t b=0;

  #ifdef IOF1_PWM1_MASK
    GPIO_REG(GPIO_IOF_EN) |= IOF1_PWM1_MASK;
    GPIO_REG(GPIO_IOF_SEL) |= IOF1_PWM1_MASK;
  #endif

  PWM_REG(PWM_CFG)   = 0;
  PWM_REG(PWM_CFG)   = (PWM_CFG_ENALWAYS) | (PWM_CFG_ZEROCMP) | (PWM_CFG_DEGLITCH);
  PWM_REG(PWM_COUNT) = 0;

  // The LEDs are intentionally left somewhat dim.
  PWM_REG(PWM_CMP0)  = 0xFE;

  int msg[4]={0,0,0,0};

	while(1){

		volatile uint64_t T1 = CLINT_REG(CLINT_MTIME) + 12*RTC_FREQ/1000;

		while (CLINT_REG(CLINT_MTIME) < T1)	ECALL_YIELD();

		if (r > 0 && b == 0) {r--; g++;}
		if (g > 0 && r == 0) {g--; b++;}
		if (b > 0 && g == 0) {r++; b--;}

		PWM_REG(PWM_CMP1) = 0xFF - (r >> 2);
		PWM_REG(PWM_CMP2) = 0xFF - (g >> 2);
		PWM_REG(PWM_CMP3) = 0xFF - (b >> 2);

		if (ECALL_RECV(1, msg)) {
			switch (msg[0]) {
			//case '1': ECALL_CSRS_MIE();	break;
			//case '0': ECALL_CSRC_MIE();	break;
			case 'p': ECALL_SEND(1, (int[4] ) {'p','o','n','g'}); break;
			}
		}

	}

}
