/*
 * FreeRTOS Kernel V10.1.1
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/**
 * Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved
 *
 * @author 	Sandro Pinto <sandro2pinto@gmail.com>
 * @author 	José Martins <josemartins90@gmail.com>
 * 
 */

#include "FreeRTOSConfig.h"

#if __riscv_xlen == 64
	#error Not implemented yet - change lw to ld, and sw to sd.
	#define portWORD_SIZE 8
	#define LOAD	LD
	#define STORE	SD
#elif __riscv_xlen == 32
	#define portWORD_SIZE 4
	#define LOAD	LW
	#define STORE	SW
#else
	#error Assembler did not define __riscv_xlen
#endif

/* Only the standard core registers are stored by default.  Any additional
registers must be saved by the portasmSAVE_ADDITIONAL_REGISTERS and
portasmRESTORE_ADDITIONAL_REGISTERS macros - which can be defined in a chip
specific version of freertos_risc_v_port_specific_extensions.h.  See the notes
at the top of this file. */
#define portCONTEXT_SIZE ( 29 * portWORD_SIZE )
#define MCAUSE_OFFSET ( 29 * portWORD_SIZE )
#define MEPC_OFFSET ( 30 * portWORD_SIZE )


.section .data
.align portWORD_SIZE
/* Used to temporarily save the mcause value */
_temp_mcause:
	.word	0

/* Used to receive message from the zone's mailbox to get trap information */
_msg:
	.word 0
	.word 0
	.word 0
	.word 0

.section      .text.entry
.align 4

.global xPortStartFirstTask
.global vPortYield
.global vTaskIncrementTick
.global vPortEndScheduler

.global _timer_handler
.global _syncexception_entry
.global _interrupt_entry

.extern pxCurrentTCB

/* Macro for saving task context */
.macro pushREGFILE
	/* create stack frame */
	//addi	sp, sp, -portWORD_SIZE * 32
	addi	sp, sp, -portCONTEXT_SIZE

	/* Save Context */
	STORE	x1, 1 * portWORD_SIZE(sp)
	STORE	x5, 2 * portWORD_SIZE(sp)
	STORE	x6, 3 * portWORD_SIZE(sp)
	STORE	x7, 4 * portWORD_SIZE(sp)
	STORE	x8, 5 * portWORD_SIZE(sp)
	STORE	x9, 6 * portWORD_SIZE(sp)
	STORE	x10, 7 * portWORD_SIZE(sp)
	STORE	x11, 8 * portWORD_SIZE(sp)
	STORE	x12, 9 * portWORD_SIZE(sp)
	STORE	x13, 10 * portWORD_SIZE(sp)
	STORE	x14, 11 * portWORD_SIZE(sp)
	STORE	x15, 12 * portWORD_SIZE(sp)
	STORE	x16, 13 * portWORD_SIZE(sp)
	STORE	x17, 14 * portWORD_SIZE(sp)
	STORE	x18, 15 * portWORD_SIZE(sp)
	STORE	x19, 16 * portWORD_SIZE(sp)
	STORE	x20, 17 * portWORD_SIZE(sp)
	STORE	x21, 18 * portWORD_SIZE(sp)
	STORE	x22, 19 * portWORD_SIZE(sp)
	STORE	x23, 20 * portWORD_SIZE(sp)
	STORE	x24, 21 * portWORD_SIZE(sp)
	STORE	x25, 22 * portWORD_SIZE(sp)
	STORE	x26, 23 * portWORD_SIZE(sp)
	STORE	x27, 24 * portWORD_SIZE(sp)
	STORE	x28, 25 * portWORD_SIZE(sp)
	STORE	x29, 26 * portWORD_SIZE(sp)
	STORE	x30, 27 * portWORD_SIZE(sp)
	STORE	x31, 28 * portWORD_SIZE(sp)

.endm

/* save current sp (points to context) in active task tcb*/
.macro portSAVE_CONTEXT
	/* Store current stackpointer in task control block (TCB) */
	LOAD	t0, pxCurrentTCB	//pointer
	STORE	sp, 0x0(t0)
.endm


 /* restore sp (pointing to context) from active task tcb */
.macro portRESTORE_CONTEXT
	/* Load stack pointer from the current TCB */
	LOAD	sp, pxCurrentTCB
	LOAD	sp, 0x0(sp)
.endm


/* Macro for restoring task context */
.macro popREGFILE

	/* Restore registers */
	LOAD	x1, 1 * portWORD_SIZE(sp)
	LOAD	x5, 2 * portWORD_SIZE(sp)
	LOAD	x6, 3 * portWORD_SIZE(sp)
	LOAD	x7, 4 * portWORD_SIZE(sp)
	LOAD	x8, 5 * portWORD_SIZE(sp)
	LOAD	x9, 6 * portWORD_SIZE(sp)
	LOAD	x10, 7 * portWORD_SIZE(sp)
	LOAD	x11, 8 * portWORD_SIZE(sp)
	LOAD	x12, 9 * portWORD_SIZE(sp)
	LOAD	x13, 10 * portWORD_SIZE(sp)
	LOAD	x14, 11 * portWORD_SIZE(sp)
	LOAD	x15, 12 * portWORD_SIZE(sp)
	LOAD	x16, 13 * portWORD_SIZE(sp)
	LOAD	x17, 14 * portWORD_SIZE(sp)
	LOAD	x18, 15 * portWORD_SIZE(sp)
	LOAD	x19, 16 * portWORD_SIZE(sp)
	LOAD	x20, 17 * portWORD_SIZE(sp)
	LOAD	x21, 18 * portWORD_SIZE(sp)
	LOAD	x22, 19 * portWORD_SIZE(sp)
	LOAD	x23, 20 * portWORD_SIZE(sp)
	LOAD	x24, 21 * portWORD_SIZE(sp)
	LOAD	x25, 22 * portWORD_SIZE(sp)
	LOAD	x26, 23 * portWORD_SIZE(sp)
	LOAD	x27, 24 * portWORD_SIZE(sp)
	LOAD	x28, 25 * portWORD_SIZE(sp)
	LOAD	x29, 26 * portWORD_SIZE(sp)
	LOAD	x30, 27 * portWORD_SIZE(sp)
	LOAD	x31, 28 * portWORD_SIZE(sp)

	/* destroy stack frame */
	addi	sp, sp, portCONTEXT_SIZE

.endm

/** 
 * These macros enable/disable the zone's interrupts using the Multizone API
 * saving the minimal context possible on the stack. Ideally this would be done
 * atomically in one instruction.
*/
.macro minimalInterruptDisable
	/* Prepare ecall */
	addi	sp, sp, -4
	STORE   a0, 0(sp)
	li	    a0, 18
	/* ECALL_CSRC_MIE (a0=id=18) */
	ecall
	LOAD	a0, 0(sp)
	addi    sp, sp, 4
.endm

.macro minimalInterruptEnable
	/* Re-Enable IRQs */
	/* Prepare ecall */
	addi	sp, sp, -4
	STORE   a0, 0(sp)
	li	    a0, 17
	/* ECALL_CSRS_MIE (a0=id=17) */
	ecall
	LOAD	a0, 0(sp)
	addi    sp, sp, 4
.endm


.macro NESTED_TRAP_ENTRY
	/* Disable zone's interrupts */
	minimalInterruptDisable
	/* Save RegFile context */
	pushREGFILE
    /* Save mcause */
	LOAD	s0, MCAUSE_OFFSET(sp)
	STORE	zero, MCAUSE_OFFSET(sp)
	/* Increment IRQ nesting */
	la		t0, ulPortInterruptNesting
	LOAD	t1, 0(t0)
	addi 	t2, t1, 1
	STORE	t2, 0(t0)
	bne     t1, zero, 1f // Skip saving context if we are in a nested IRQ
	/* Save Task context*/
	portSAVE_CONTEXT
1:
	/* Enable IRQs (allowing nesting) */
	//jal 	ECALL_CSRS_MIE
.endm

.macro NESTED_TRAP_EXIT
	/* Disable IRQs (handling nesting) */
	//jal		ECALL_CSRC_MIE
	/* Decrement IRQ nesting */
	la		t0, ulPortInterruptNesting
	LOAD	t1, 0(t0)
	addi 	t2, t1, -1
	STORE	t2, 0(t0)
	bne		t2, zero, 3f
	/* Check if IRQ handler requested a Context Switch
	(please refer to FreeRTOS APIs x[something]FromISR ) */
	la		t0, ulPortYieldRequired
	LOAD	t1, 0(t0)
	beq		t1, zero, 2f
	STORE	zero, 0(t0)
	/* Context Switch */
	jal		vTaskSwitchContext
2:
	/* Restoring Task context and previous mcause */
	portRESTORE_CONTEXT
	STORE	s0, MCAUSE_OFFSET(sp)
3:
	/* Restore RegFile context */
	popREGFILE
	/* Re-enable zone's interrupts */
	minimalInterruptEnable
	/* Return from interrupt (will trap to the nanokernel) */
	uret
.endm

.align 4
_syncexception_entry:

	minimalInterruptDisable

	/* saved preempted context */
	pushREGFILE

	/* Receive from own zone's mailbox to get excpetion register informations */
	li 	a0, MULTIZONE_ZONE
	la  a1, _msg
	jal	ECALL_RECV
	la  t0, _msg

	/* pass the obtained register information to the handler */
	LOAD a0, 0(t0)	//mcause
	LOAD a1, 1*portWORD_SIZE(t0) //mtval
	LOAD a2, 2*portWORD_SIZE(t0) //mepc
	jal  handle_syncexception

	/* the value returned by the handler is the new mepc. update it on the stack */
	//STORE	a0, 33*portWORD_SIZE(sp)
	STORE	a0, MEPC_OFFSET(sp)

	/* restore preempted context */
	popREGFILE

	minimalInterruptEnable

	/* return from handler (it will trap to the nanokernel) */
	uret


.align 4
_interrupt_entry:
	NESTED_TRAP_ENTRY
	/* Call IRQ handler (a0 = s0 = mcause) */
	mv      a0, s0
	jal 	handle_interrupt
	NESTED_TRAP_EXIT

.align 4
_timer_handler:
	NESTED_TRAP_ENTRY
	/* Call SysTick handler */
	jal		vPortSysTickHandler
	NESTED_TRAP_EXIT

.align 4
xPortStartFirstTask:
	/* Setup exceptions and timer */
	//jal vPortSetup

	/* Restore first task context */
	portRESTORE_CONTEXT
	popREGFILE
	/**
	 * Enable IRQs as they might have been disable before during internal
	 * FreeRTOS initialization.
	 */
	minimalInterruptEnable
	/* Jump to task, through the MultiZone nanokernel */
	uret


/** 
 * In this Multizone port, tasks directly call vPortYield instead of going
 * through the ecall trap.
 */
 .align 4
vPortYield:
	/* Simulate stack frame created by the nanokernel on interrupts */
	addi sp, sp, -2*portWORD_SIZE
	STORE	ra, portWORD_SIZE(sp) /* store ra as mepc */
	STORE	zero, 0(sp)      /* mcause as innocous/nonexistent interrupt (user software) */

	/* From now on, treat this as a trap */
	minimalInterruptDisable
	pushREGFILE
	portSAVE_CONTEXT
	jal		vTaskSwitchContext
	portRESTORE_CONTEXT
	popREGFILE
	minimalInterruptEnable
	uret


/**
 *	Stub for the interrupt handler if this is not defined in the application.
 */
.weak handle_interrupt
handle_interrupt:
	j	.

/**
 *	Stub for the trap handler if this is not defined in the application.
 */
.weak handle_syncexception
handle_syncexception:
	j	.
