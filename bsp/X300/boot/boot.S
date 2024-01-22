/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

/*
* Hardware setup / OEM low-level boot code
* Link to HEX format and pass to the multizone.jar via --boot option
* Runs in privileged machine mode "M" (after policy integrity check)
* Return execution to MultiZone via trap 0xb (ecall from M mode)
*/

#include "../platform.h"

.section .text.boot
.global boot

boot:

	# Reset PLIC
	la t0, PLIC_BASE; la t1, PLIC_BASE + 0x200008
 	1: 	sw zero, (t0)
 		addi t0, t0, 4
 		bltu t0, t1, 1b

	# Enable multiplexed GPIO HW functions: UART0
	.equ IOF0_UART0_MASK, 1<<16 | 1<<17

	la t0, GPIO_BASE
	li t1, IOF0_UART0_MASK; sw t1, GPIO_IOF_EN(t0)
	li t1,~IOF0_UART0_MASK; sw t1, GPIO_IOF_SEL(t0)

	# exit to MultiZone
	ecall
