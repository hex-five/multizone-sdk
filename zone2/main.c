/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <platform.h>
#include <plic_driver.h>
#include <libhexfive.h>

#define LD1_RED_ON PWM_REG(PWM_CMP1)  = 0x00;
#define LD1_GRN_ON PWM_REG(PWM_CMP2)  = 0x00;
#define LD1_BLU_ON PWM_REG(PWM_CMP3)  = 0x00;

#define LD1_RED_OFF PWM_REG(PWM_CMP1)  = 0xFF;
#define LD1_GRN_OFF PWM_REG(PWM_CMP2)  = 0xFF;
#define LD1_BLU_OFF PWM_REG(PWM_CMP3)  = 0xFF;

// Global Instance data for the PLIC
// for use by the PLIC Driver.
plic_instance_t g_plic;

void button_0_handler(void)__attribute__((interrupt("user")));
void button_0_handler(void){ // global interrupt

	int sent = ECALL_SEND(1, (int[4]){201,0,0,0});

	plic_source int_num  = PLIC_claim_interrupt(&g_plic); // claim

	LD1_GRN_ON; LD1_RED_OFF; LD1_BLU_OFF;

	const uint64_t T1 = ECALL_CSRR_MTIME() + 3*RTC_FREQ;
	while (ECALL_CSRR_MTIME() < T1) ECALL_YIELD();

	LD1_RED_OFF; LD1_GRN_OFF; LD1_BLU_OFF;

	GPIO_REG(GPIO_RISE_IP) |= (1<<BTN0); //clear gpio irq

	PLIC_complete_interrupt(&g_plic, int_num); // complete

}

void button_1_handler(void)__attribute__((interrupt("user")));
void button_1_handler(void){ // local interrupt

	ECALL_SEND(1, (int[4]){190+16+LOCAL_INT_BTN_1,0,0,0} );

	LD1_RED_ON; LD1_GRN_OFF; LD1_BLU_OFF;

	const uint64_t T1 = ECALL_CSRR_MTIME() + 3*RTC_FREQ;
	while (ECALL_CSRR_MTIME() < T1) ECALL_YIELD();

	LD1_RED_OFF; LD1_GRN_OFF; LD1_BLU_OFF;

	GPIO_REG(GPIO_RISE_IP) |= (1<<BTN1); //clear gpio irq

}

void button_2_handler(void)__attribute__((interrupt("user")));
void button_2_handler(void){ // local interrupt

	ECALL_SEND(1, (int[4]){190+16+LOCAL_INT_BTN_2,0,0,0});

	LD1_BLU_ON; LD1_GRN_OFF; LD1_RED_OFF;

	const uint64_t T1 = ECALL_CSRR_MTIME() + 3*RTC_FREQ;
	while (ECALL_CSRR_MTIME() < T1) ECALL_YIELD();

	LD1_RED_OFF; LD1_GRN_OFF; LD1_BLU_OFF;

	GPIO_REG(GPIO_RISE_IP) |= (1<<BTN2); //clear gpio irq

}

/*configures Button0 as a global gpio irq*/
void b0_irq_init()  {

    uint64_t mvendid;
    uint64_t mimpid;
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

    mvendid = ECALL_CSRR_MVENDID();
    mimpid = ECALL_CSRR_MIMPID();
    // GPIO irq offset depends on the core version
    if (mvendid == 0x489 && mimpid > 0x20190000) {
        irqnum = 1 + BTN0;
    } else {
        irqnum = GPIO_INT_BASE + BTN0;
    }

    PLIC_enable_interrupt (&g_plic, irqnum);
    PLIC_set_priority(&g_plic, irqnum, 2);

    ECALL_IRQ_VECT(11, button_0_handler);

}

/*configures Button1 as a local interrupt*/
void b1_irq_init()  {

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN )    &=  ~(1 << BTN1);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN1);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN1);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BTN1);

    //enable the interrupt
    ECALL_IRQ_VECT(16+LOCAL_INT_BTN_1, button_1_handler);
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

    //enable the interrupt
    ECALL_IRQ_VECT(16+LOCAL_INT_BTN_2, button_2_handler);
}

int main (void){

  //volatile int w=0; while(1){w++;}
  //while(1) ECALL_YIELD();

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

		const uint64_t T1 = ECALL_CSRR_MTIME() + 12*RTC_FREQ/1000;
		while (ECALL_CSRR_MTIME() < T1)	ECALL_YIELD();

		if (r > 0 && b == 0) {r--; g++;}
		if (g > 0 && r == 0) {g--; b++;}
		if (b > 0 && g == 0) {r++; b--;}

		PWM_REG(PWM_CMP1) = 0xFF - (r >> 2);
		PWM_REG(PWM_CMP2) = 0xFF - (g >> 2);
		PWM_REG(PWM_CMP3) = 0xFF - (b >> 2);

		if (ECALL_RECV(1, msg)) {
			switch (msg[0]) {
			case '1': ECALL_CSRS_MIE();	break;
			case '0': ECALL_CSRC_MIE();	break;
			case 'p': ECALL_SEND(1, (int[4] ) {'p','o','n','g'}); break;
			}
		}

	}

}
