/* Copyright(C) 2024 Hex Five Security, Inc. - All Rights Reserved */

/*
| GPIO | ARTY CONNECTION | CLINT IRQ | PLIC IRQ |
| -----| ----------------| --------- | -------- |
|    0 | IO8             |           |       22 |
|    1 |                 |           |          |
|    2 |                 |           |          |
|    3 |                 |           |          |
|    4 | IO12            |           |       26 |
|    5 | IO13            |           |       27 |
|    6 |                 |           |          |
|    7 |                 |           |          |
|    8 | PMODA [1]       |           |       30 |
|    9 | PMODA [2]       |           |       31 |
|   10 | PMODA [3]       |           |       32 |
|   11 | PMODA [4]       |           |       33 |
|   12 |                 |           |          |
|   13 |                 |           |          |
|   14 |                 |           |          |
|   15 | BTN0            |        16 |       37 |
|   16 |                 |           |          |
|   17 |                 |           |          |
|   18 | IO02            |           |       40 |
|   19 |                 |           |          |
|   20 | IO04            |           |       42 |
|   21 |                 |           |          |
|   22 |                 |           |          |
|   23 | IO07            |           |       45 |
|   24 |                 |           |          |
|   25 |                 |           |          |
|   26 |                 |           |          |
|   27 |                 |           |          |
|   28 |                 |           |          |
|   29 |                 |           |          |
|   30 | BTN1            |        17 |       52 |
|   31 | BTN2            |        18 |       53 |
*/

#include <stdio.h>
#include "platform.h"
#include "multizone.h"

__attribute__((interrupt())) void trp_isr(void)  {

    char str[16] = "";

    const unsigned long mcause = MZONE_CSRR(CSR_MCAUSE);

    if (mcause == 0x8000000b ) {

        // plic interrupt

        const uint32_t plic_int = PLIC_REG(PLIC_CLAIM); // PLIC claim

        GPIO_REG(GPIO_RISE_IP) = 0xffffffff;

        PLIC_REG(PLIC_CLAIM) = plic_int; // PLIC complete

        snprintf(str, sizeof str, "plic %ld", plic_int); MZONE_SEND(1, str);

    } else if ( (mcause & 0xffff0000) != 0){

        // local interrupt
        snprintf(str, sizeof str, "ip    %08lx", GPIO_REG(GPIO_RISE_IP)); MZONE_SEND(1, str);
        snprintf(str, sizeof str, "mip   %08lx", MZONE_CSRR(CSR_MIP)); MZONE_SEND(1, str);

        GPIO_REG(GPIO_RISE_IP) = 0xffffffff;

    } else {

       //  trap

       snprintf(str, sizeof str, "mcause %08lx", mcause); MZONE_SEND(1, str);

       for(;;);

    }

}

int main (void){

    // setup GPIO ports
    GPIO_REG(GPIO_INPUT_EN)  = 0xffffffff;
    GPIO_REG(GPIO_PULLUP_EN) = 0xffffffff;
    GPIO_REG(GPIO_RISE_IP)   = 0xffffffff;
    GPIO_REG(GPIO_RISE_IE)   = 0xffffffff;

#if 0
    // enable local interrupts
    CSRS(mie, 0xFFFF0000);
#else
    // enable plic interrupts
    CSRS(mie, 1<<11);
    for (int src = 22; src<54; src++) {
        PLIC_REG(PLIC_EN + ((src/32)<<PLIC_SHIFT_PER_SRC)) |= (1 << (src%32));
        PLIC_REG(PLIC_PRI + (src<<PLIC_SHIFT_PER_SRC)) = 1;
    }
#endif

    // trap mode "direct"
    CSRC(mtvec, 1);

    // enable global interrupts
    CSRS(mstatus, 1<<3);

    char str[16] = "";

    uint32_t gpio_input_val = GPIO_REG(GPIO_INPUT_VAL);

    snprintf(str, sizeof str, "gpio 0x%08lx", gpio_input_val); MZONE_SEND(1, str);

    while (1) {

        const uint32_t val = GPIO_REG(GPIO_INPUT_VAL);

        if (val != gpio_input_val){

            snprintf(str, sizeof str, "xor 0x%08lx", gpio_input_val ^ val); MZONE_SEND(1, str);

            gpio_input_val = val;

        }

        MZONE_YIELD();

    }

}
