/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#ifndef HEXFIVE_PLATFORM_H
#define HEXFIVE_PLATFORM_H


#define CPU_FREQ	64998442 
#define RTC_FREQ 	   16416

// -----------------------------------------------------------------------------
// Ethernet
// -----------------------------------------------------------------------------
#define XEMAC_BASE 0x60000000UL

// -----------------------------------------------------------------------------
// RTC (CLINT)
// -----------------------------------------------------------------------------
#define CLINT_BASE	0x02000000UL

#define CLINT_MSIP	    0x0000
#define CLINT_MTIMECMP  0x4000
#define CLINT_MTIME	    0xBFF8

// -----------------------------------------------------------------------------
// UART
// -----------------------------------------------------------------------------
#define UART_BASE 	0x10013000UL

#define UART_TXFIFO 0x00
#define UART_RXFIFO 0x04
#define UART_TXCTRL 0x08
#define UART_RXCTRL 0x0c
#define UART_IE 	0x10
#define UART_IP 	0x14
#define UART_DIV 	0x18

#define UART_IRQ    11 // PLIC_SRC_UART 3

// -----------------------------------------------------------------------------
// PWM
// -----------------------------------------------------------------------------
#define PWM_BASE	0x10025000UL

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
#define GPIO_BASE 	0x10012000UL

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

#define BTN0_IRQ 16 // CLINT
#define BTN1_IRQ 17 // CLINT
#define BTN2_IRQ 18 // CLINT

// -----------------------------------------------------------------------------
// LED0 (GPIO)
// ------------------------------------------------------------------------------
#define LED_RED	 1
#define LED_GRN	 2
#define LED_BLUE 3

// -----------------------------------------------------------------------------
// PLIC (shared)
// ------------------------------------------------------------------------------
#define PLIC_BASE 	0x0C000000UL

#define PLIC_PRI 			0
#define PLIC_EN				0x002000
#define PLIC_IP				0x001000
#define PLIC_THRES			0x200000
#define PLIC_CLAIM			0x200004
#define PLIC_SHIFT_PER_TRG	1
#define PLIC_SHIFT_PER_SRC 	2

#define PLIC_SRC_UART		3
#define PLIC_SRC_XEMAC		21

// -----------------------------------------------------------------------------
// DMA (single channel mockup)
// ------------------------------------------------------------------------------
#define DMA_BASE 	  0x10040000UL

#define DMA_VER_OFF			0x00
#define DMA_CFG_OFF			0x10
#define DMA_CTRL_OFF		0x20
#define DMA_CH_STATUS_OFF	0x30 /* TC: 1<<ch+16, AB: 1<<ch+8,  ERR: 1<<ch+0 */
#define DMA_CH_ENABLE_OFF	0x34 /* 1<<ch */
#define DMA_CH_ABORT_OFF	0x40 /* 1<<ch */
#define DMA_CH_CTRL_OFF		0x44 /* +ch*0x14 */
#define DMA_TR_SRC_OFF		0x48 /* +ch*0x14 */
#define DMA_TR_DEST_OFF		0x4C /* +ch*0x14 */
#define DMA_TR_SIZE_OFF		0x50 /* +ch*0x14 */

#define DMA_IRQ				  19 /* Mockup */

// -----------------------------------------------------------------------------
// C Helper functions
// -----------------------------------------------------------------------------

#define _REG64(base, offset) (*(volatile uint64_t *)((base) + (offset)))
#define _REG32(base, offset) (*(volatile uint32_t *)((base) + (offset)))

#define CLINT_REG(offset) _REG64(CLINT_BASE, offset)
#define GPIO_REG(offset)  _REG32(GPIO_BASE, offset)
#define PWM_REG(offset)   _REG32(PWM_BASE, offset)
#define UART_REG(offset)  _REG32(UART_BASE, offset)
#define PLIC_REG(offset)  _REG32(PLIC_BASE, offset)
#define DMA_REG(offset)   _REG32(DMA_BASE, offset)


#endif /* HEXFIVE_PLATFORM_H */
