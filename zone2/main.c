/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <string.h>

#include "platform.h"
#include "multizone.h"

#define LD1_RED_ON PWM_REG(PWM_CMP1)  = 0x0;
#define LD1_GRN_ON PWM_REG(PWM_CMP2)  = 0x0;
#define LD1_BLU_ON PWM_REG(PWM_CMP3)  = 0x0;

#define LD1_RED_OFF PWM_REG(PWM_CMP1)  = 0xFF;
#define LD1_GRN_OFF PWM_REG(PWM_CMP2)  = 0xFF;
#define LD1_BLU_OFF PWM_REG(PWM_CMP3)  = 0xFF;

static volatile char msg[16] = {'\0'};

// ------------------------------------------------------------------------
#ifdef E21
    static void (*trap_vect[173])(void) = {};
#else
    static void (*trap_vect[__riscv_xlen])(void) = {};
#endif
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

	for(;;);

}
__attribute__((interrupt())) void msi_handler(void)  { // machine software interrupt (3)

	char const tmp[16];

	if (MZONE_RECV(1, tmp))
		memcpy((char *)msg, tmp, sizeof msg);

}
__attribute__((interrupt())) void tmr_handler(void)  { // machine timer interrupt (7)

	static uint16_t r=0x3F;
	static uint16_t g=0;
	static uint16_t b=0;

	if (r > 0 && b == 0) {r--; g++;}
	if (g > 0 && r == 0) {g--; b++;}
	if (b > 0 && g == 0) {r++; b--;}

#ifdef FE310
	PWM_REG(PWM_CMP1) = r;
	PWM_REG(PWM_CMP2) = g;
	PWM_REG(PWM_CMP3) = b;
#else
	PWM_REG(PWM_CMP1) = 0xFF - (r >> 2);
	PWM_REG(PWM_CMP2) = 0xFF - (g >> 2);
	PWM_REG(PWM_CMP3) = 0xFF - (b >> 2);
#endif

	// set timer (clears mip)
	MZONE_ADTIMECMP((uint64_t)5*RTC_FREQ/1000);

}
__attribute__((interrupt())) void btn0_handler(void) {

	static uint64_t debounce = 0;
	const uint64_t T = MZONE_RDTIME();
	if (T > debounce){
		debounce = T + 250*RTC_FREQ/1000;
		MZONE_SEND(1, (char[16]){"IRQ BTN0"});
		LD1_RED_OFF; LD1_GRN_ON; LD1_BLU_OFF;
		MZONE_ADTIMECMP((uint64_t)250*RTC_FREQ/1000);
	}
	BITSET(GPIO_BASE+GPIO_HIGH_IP, 1<<BTN0); //clear gpio irq

}
__attribute__((interrupt())) void btn1_handler(void) {

	static uint64_t debounce = 0;
	const uint64_t T = MZONE_RDTIME();
	if (T > debounce){
		debounce = T + 250*RTC_FREQ/1000;
		MZONE_SEND(1, (char[16]){"IRQ BTN1"});
		LD1_RED_ON; LD1_GRN_OFF; LD1_BLU_OFF;
		MZONE_ADTIMECMP((uint64_t)250*RTC_FREQ/1000);
	}
    BITSET(GPIO_BASE+GPIO_HIGH_IP, 1<<BTN1); //clear gpio irq

}
__attribute__((interrupt())) void btn2_handler(void) {

	static uint64_t debounce = 0;
	const uint64_t T = MZONE_RDTIME();
	if (T > debounce){
		debounce = T + 250*RTC_FREQ/1000;
		MZONE_SEND(1, (char[16]){"IRQ BTN2"});
		LD1_RED_OFF; LD1_GRN_OFF; LD1_BLU_ON;
		MZONE_ADTIMECMP((uint64_t)250*RTC_FREQ/1000);
	}
    BITSET(GPIO_BASE+GPIO_HIGH_IP, 1<<BTN2); //clear gpio irq

}

// ------------------------------------------------------------------------

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
#ifdef CLIC_BASE
    *(volatile uint8_t *)(CLIC_BASE + CLIC_INT_ENABLE + BTN0_IRQ) = 0x1;
#else
    CSRS(mie, 1<<(BTN0_IRQ));
#endif

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

#ifdef CLIC_BASE
    *(volatile uint8_t *)(CLIC_BASE + CLIC_INT_ENABLE + BTN1_IRQ) = 0x1;
#else
    CSRS(mie, 1<<(BTN1_IRQ));
#endif

}

// configures Button2 as local interrupt
void b2_irq_init()  {

    // disable hw io function
    GPIO_REG(GPIO_IOF_EN ) &= ~(1 << BTN2);

    // set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BTN2);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BTN2);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_HIGH_IE)    |= (1<<BTN2);

#ifdef CLIC_BASE
    *(volatile uint8_t *)(CLIC_BASE + CLIC_INT_ENABLE + BTN2_IRQ) = 0x1;
#else
    CSRS(mie, 1<<(BTN2_IRQ));
#endif

}

int main (void){

	//while(1) MZONE_WFI();
	//while(1) MZONE_YIELD();
	//while(1);

    // setup vectored trap handler
	trap_vect[0] = trp_handler;
	trap_vect[3] = msi_handler;
	trap_vect[7] = tmr_handler;
	trap_vect[BTN0_IRQ] = btn0_handler;
	trap_vect[BTN1_IRQ] = btn1_handler;
	trap_vect[BTN2_IRQ] = btn2_handler;
	CSRW(mtvec, trap_vect);
	CSRS(mtvec, 0x1);

	PWM_REG(PWM_CFG)   = (PWM_CFG_ENALWAYS | PWM_CFG_ZEROCMP);
	PWM_REG(PWM_CMP0)  = 0xFE;

	b0_irq_init();
	b1_irq_init();
	b2_irq_init();

    // set & enable timer
    MZONE_ADTIMECMP((uint64_t)5*RTC_FREQ/1000);
    CSRS(mie, 1 << 7);

    // enable msip/inbox interrupt
	CSRS(mie, 1<<3);

	// enable global interrupts
    CSRS(mstatus, 1<<3);

    while (1) {

        // Message handler
        CSRC(mie, 1 << 3);

        if (msg[0] != '\0') {

            if (strncmp("ping", (char*) msg, sizeof msg[0]) == 0)
                MZONE_SEND(1, (char[16]){"pong"});
            else if (strcmp("mie=0", (char*) msg) == 0)
                CSRC(mstatus, 1 << 3);
            else if (strcmp("mie=1", (char*) msg) == 0)
                CSRS(mstatus, 1 << 3);
            else if (strcmp("block", (char*) msg) == 0) {
                CSRC(mstatus, 1 << 3);
                for (;;)
                    ;
            } else
                MZONE_SEND(1, msg);

            msg[0] = '\0';

        }

        CSRS(mie, 1 << 3);

        // Wait For Interrupt
        MZONE_WFI();

    }

}
