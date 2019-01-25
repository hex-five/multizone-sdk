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
 * 
 */

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"

/*Platform includes */
#include "platform.h"

/*MultiZone includes */
#include "libhexfive.h"



/* Each task maintains its own interrupt status in the critical nesting
variable. */
UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

/* Set to 1 to pend a context switch from an ISR. */
volatile UBaseType_t ulPortYieldRequired = pdFALSE;

/* Counts the interrupt nesting depth.  A context switch is only performed if
if the nesting depth is 0. */
volatile UBaseType_t ulPortInterruptNesting = 0UL;

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void prvTaskExitError( void );

/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{

	portDISABLE_INTERRUPTS();

	uxCriticalNesting++;
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
	configASSERT( uxCriticalNesting );
	uxCriticalNesting--;
	if( uxCriticalNesting == 0 )
	{
		portENABLE_INTERRUPTS();
	}

	return;
}

/*-----------------------------------------------------------*/

/* Clear current interrupt mask and set given mask */
void vPortClearInterruptMask(int mask)
{
	/**
	 * MULTIZONE: the nanokernel does not allow a zone to directly write
	 * machine registers.
	 * TODO: Implement this using the multizone exception register/deregister
	 * API.
	 */
	//	write_csr(mie,mask);
}
/*-----------------------------------------------------------*/

/* Set interrupt mask and return current interrupt enable register */
int xPortSetInterruptMask()
{
	//uint32_t ret;
	/**
	 * MULTIZONE: the nanokernel does not allow a zone to directly write
	 * machine registers.
	 * TODO: Implement this using the multizone exception register/deregister
	 * API.
	 */
	//ret = read_csr(mie);
	//write_csr(mie,0);
	//return ret;
	return 0;
}

/*-----------------------------------------------------------*/

/*
 *
 * As per the standard RISC-V ABI pxTopcOfStack is passed in in a0, pxCode in
 * a1, and pvParameters in a2.  The new top of stack is passed out in a0.
 *
 * RISC-V maps registers to ABI names as follows (X1 to X31 integer registers
 * for the 'I' profile, X1 to X15 for the 'E' profile, currently I assumed).
 *
 * Register		ABI Name	Description						Saver
 * x0			zero		Hard-wired zero					-
 * x1			ra			Return address					Caller
 * x2			sp			Stack pointer					Callee
 * x3			gp			Global pointer					-
 * x4			tp			Thread pointer					-
 * x5-7			t0-2		Temporaries						Caller
 * x8			s0/fp		Saved register/Frame pointer	Callee
 * x9			s1			Saved register					Callee
 * x10-11		a0-1		Function Arguments/return values Caller
 * x12-17		a2-7		Function arguments				Caller
 * x18-27		s2-11		Saved registers					Callee
 * x28-31		t3-6		Temporaries						Caller
 *
 * The RISC-V context is saved to FreeRTOS tasks in the following stack frame,
 * where the global and thread pointers are currently assumed to be constant so
 * are not saved:
 *
 * x31
 * x30
 * x29
 * x28
 * x27
 * x26
 * x25
 * x24
 * x23
 * x22
 * x21
 * x20
 * x19
 * x18
 * x17
 * x16
 * x15
 * x14
 * x13
 * x12
 * x11
 * pvParameters
 * x9
 * x8
 * x7
 * x6
 * x5
 * portTASK_RETURN_ADDRESS
 * pxCode
 */
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
	/**
	 * Simulate the stack frame as it would be created by an interrupt. 
	 * This includes the initial multizone stack frame with the mepc and mcause at the bottom.
	 * 
	 * TODO: initialize all stack registers with a default value.
	 */
	pxTopOfStack--;
	*pxTopOfStack = (StackType_t)pxCode;	 /* Start address @ mepc MultiZone - a0*/
	pxTopOfStack--;
	*pxTopOfStack = 0;	/* Set initial mcause to innocous value of non-existing interrupt (user software) */
	pxTopOfStack -= 22;
	*pxTopOfStack = (StackType_t)pvParameters;	/* Register a0 */
	pxTopOfStack -=7;

	return pxTopOfStack;
}
/*-----------------------------------------------------------*/

void prvTaskExitError( void )
{
	/* A function that implements a task must not exit or attempt to return to
	its caller as there is nothing to return to.  If a task wants to exit it
	should instead call vTaskDelete( NULL ).
	Artificially force an assert() to be triggered if configASSERT() is
	defined, then stop here so application writers can catch the error. */
	configASSERT( uxCriticalNesting == ~0UL );
	portDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

/*Entry Point for Machine Timer Interrupt Handler*/
void vPortSysTickHandler(){
	
	/* Calculate next compare value */
	const uint64_t now = ECALL_CSRR_MTIME();
	const uint64_t then = now + (configRTC_CLOCK_HZ / configTICK_RATE_HZ);

	/* Increment the RTOS tick. */
	if( xTaskIncrementTick() != pdFALSE )
	{
		ulPortYieldRequired = pdTRUE;
	}

	/* Request next timer interrupt */
	ECALL_CSRW_MTIMECMP(then);
}
/*-----------------------------------------------------------*/



void vPortSetupTimerInterrupt()	{

extern void _timer_handler( void );

	/* Calculate first tick timer compare */
	const uint64_t ullCurrentTime = ECALL_CSRR_MTIME();
	const uint64_t ullNextTime = ullCurrentTime + (configRTC_CLOCK_HZ / configTICK_RATE_HZ);

	// Setup mtimer handler
	ECALL_TRP_VECT(0x3, _timer_handler);

	/* Request first tick interrupt */
    ECALL_CSRW_MTIMECMP(ullNextTime);

}
/*-----------------------------------------------------------*/


BaseType_t xPortStartScheduler( void )
{

extern void _syncexception_entry( void );
extern void xPortStartFirstTask( void );

	/**
	 * MULTIZONE: all available synchronous exception handlers are registered 
	 * to the same initial handler, which will decode the mcause that arrives 
	 * in a message from the nanokernel and redirect to the final handler.
	 * 
	 * TODO: if more exceptions become available in the API they should be 
	 * added here.
	 *
	 */

	ECALL_TRP_VECT(0x0, _syncexception_entry); // 0x0 Instruction address misaligned
	ECALL_TRP_VECT(0x1, _syncexception_entry); // 0x1 Instruction access fault
	ECALL_TRP_VECT(0x2, _syncexception_entry); // 0x2 Illegal Instruction
	ECALL_TRP_VECT(0x4, _syncexception_entry); // 0x4 Load address misaligned
	ECALL_TRP_VECT(0x5, _syncexception_entry); // 0x5 Load access fault
	ECALL_TRP_VECT(0x6, _syncexception_entry); // 0x6 Store/AMO address misaligned
	ECALL_TRP_VECT(0x7, _syncexception_entry); // 0x7 Store access fault

	/* Make sure all previously registered interrupts are enabled */
	ECALL_CSRS_MIE();

	vPortSetupTimerInterrupt();

	uxCriticalNesting = 0;

	xPortStartFirstTask();

	/* Should not get here as after calling xPortStartFirstTask() only tasks
	should be executing. */
	return pdFAIL;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* Not implemented. */
	for( ;; );
}
