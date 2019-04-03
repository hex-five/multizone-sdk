/* Copyright(C) 2019 Hex Five Security, Inc. - All Rights Reserved */

#ifndef HEXFIVE_PLATFORM_H
#define HEXFIVE_PLATFORM_H

#define CPU_FREQ	64998442
#define RTC_FREQ 	   16416

// -----------------------------------------------------------------------------
// RTC (MTIME)
// -----------------------------------------------------------------------------
#define RTC_BASE	0x02000000
#define RTC_MTIME	    0xBFF8


// -----------------------------------------------------------------------------
// UART
// -----------------------------------------------------------------------------
#define UART_BASE 	0x10013000

#define UART_TXFIFO 0x00
#define UART_RXFIFO 0x04
#define UART_TXCTRL 0x08
#define UART_RXCTRL 0x0c
#define UART_IE 	0x10
#define UART_IP 	0x14
#define UART_DIV 	0x18

// -----------------------------------------------------------------------------
// PWM
// -----------------------------------------------------------------------------
#define PWM_BASE	0x10025000

#define PWM_CFG		0x00
#define PWM_COUNT	0x08
#define PWM_S		0x10
#define PWM_CMP0	0x20
#define PWM_CMP1 	0x24	// LED1 Red
#define PWM_CMP2 	0x28	// LED1 Green
#define PWM_CMP3 	0x2C	// LED1 Blue

#define PWM_CFG_SCALE       0x0000000F
#define PWM_CFG_STICKY      0x00000100
#define PWM_CFG_ZEROCMP     0x00000200
#define PWM_CFG_DEGLITCH    0x00000400
#define PWM_CFG_ENALWAYS    0x00001000
#define PWM_CFG_ONESHOT     0x00002000
#define PWM_CFG_CMP0CENTER  0x00010000
#define PWM_CFG_CMP1CENTER  0x00020000
#define PWM_CFG_CMP2CENTER  0x00040000
#define PWM_CFG_CMP3CENTER  0x00080000
#define PWM_CFG_CMP0GANG    0x01000000
#define PWM_CFG_CMP1GANG    0x02000000
#define PWM_CFG_CMP2GANG    0x04000000
#define PWM_CFG_CMP3GANG    0x08000000
#define PWM_CFG_CMP0IP      0x10000000
#define PWM_CFG_CMP1IP      0x20000000
#define PWM_CFG_CMP2IP      0x40000000
#define PWM_CFG_CMP3IP      0x80000000

// -----------------------------------------------------------------------------
// GPIO
// ------------------------------------------------------------------------------
#define GPIO_BASE 	0x10012000
#define GPIO_INT_BASE 22

#define GPIO_INPUT_VAL  0x00
#define GPIO_INPUT_EN   0x04
#define GPIO_OUTPUT_EN  0x08
#define GPIO_OUTPUT_VAL 0x0C
#define GPIO_PULLUP_EN  0x10
#define GPIO_DRIVE      0x14
#define GPIO_RISE_IE    0x18
#define GPIO_RISE_IP    0x1C
#define GPIO_FALL_IE    0x20
#define GPIO_FALL_IP    0x24
#define GPIO_HIGH_IE    0x28
#define GPIO_HIGH_IP    0x2C
#define GPIO_LOW_IE     0x30
#define GPIO_LOW_IP     0x34
#define GPIO_IOF_EN     0x38
#define GPIO_IOF_SEL    0x3C
#define GPIO_OUTPUT_XOR 0x40

// -----------------------------------------------------------------------------
// Buttons (GPIO and IRQ assignments)
// ------------------------------------------------------------------------------
#define BTN0  15
#define BTN1  30
#define BTN2  31

#define LOCAL_INT_BTN_0  0
#define LOCAL_INT_BTN_1  1
#define LOCAL_INT_BTN_2  2

// -----------------------------------------------------------------------------
// LEDs
// ------------------------------------------------------------------------------
#define LED_RED   (1 << 1)
#define LED_GREEN (1 << 2)
#define LED_BLUE  (1 << 3)

// -----------------------------------------------------------------------------
// PLIC
// ------------------------------------------------------------------------------
#define PLIC_BASE 	0x0C000000

#define PLIC_NUM_INTERRUPTS 28
#define PLIC_NUM_PRIORITIES 7

// -----------------------------------------------------------------------------
// C Helper functions
// -----------------------------------------------------------------------------

#define _REG64(base, offset) (*(volatile uint64_t *)((base) + (offset)))
#define _REG32(base, offset) (*(volatile uint32_t *)((base) + (offset)))
#define _REG16(base, offset) (*(volatile uint16_t *)((base) + (offset)))

#define RTC_REG(offset)  _REG64(RTC_BASE, offset)
#define GPIO_REG(offset) _REG32(GPIO_BASE, offset)
#define PWM_REG(offset)  _REG32(PWM_BASE, offset)
#define UART_REG(offset) _REG32(UART_BASE, offset)


#define IOF0_UART0_MASK         0x00030000UL
#define IOF1_PWM1_MASK          0x00680000UL

#endif /* HEXFIVE_PLATFORM_H */
