/**
 * Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved
 *
 * @author 	Sandro Pinto <sandro2pinto@gmail.com>
 * @author 	José Martins <josemartins90@gmail.com>
 *
 * TODO: add credits to original sifive port
 * 
 */

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"

/*SiFive Includes */
#include "platform.h"

/*MultiZone Includes */
#include "libhexfive.h"

/* Standard Includes */
#include <stdlib.h>
#include <unistd.h>


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

StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
	register int gp asm("gp");
	/**
	 * Simulate the stack frame as it would be created by an interrupt. 
	 * This includes the initial multizone stack frame with the mepc and mcause at the bottom.
	 * 
	 * TODO: initialize all stack registers with a default value.
	 */
	pxTopOfStack--;
	*pxTopOfStack = (StackType_t)pxCode;	 /* Start address @ mepc MultiZone*/
	pxTopOfStack--;
	*pxTopOfStack = 0;	/* Set initial mcause to innocous value of non-existing interrupt (user software) */
	pxTopOfStack -= 22;
	*pxTopOfStack = (StackType_t)pvParameters;	/* Register a0 */
	pxTopOfStack -=7;
	*pxTopOfStack = (StackType_t)gp;
	pxTopOfStack -=2;
	*pxTopOfStack = (StackType_t)prvTaskExitError; /* Register ra */
	pxTopOfStack--;

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

extern void _timer_handler();
extern void _syncexception_entry();
extern void _interrupt_handler();

void vConfigureTickInterrupt()	{

	// Setup mtimer handler
	ECALL_TRP_VECT(0x3, _timer_handler);

	/* Calculate first tick timer compare */
	const uint64_t now = ECALL_CSRR_MTIME();
	const uint64_t then = now + (configRTC_CLOCK_HZ / configTICK_RATE_HZ);

	/* Request first tick interrupt */
    ECALL_CSRW_MTIMECMP(then);
}
/*-----------------------------------------------------------*/


void vPortSetup(){

	/**
	 * MULTIZONE: all available synchronous exception handlers are registered 
	 * to the same initial handler, which will decode the mcause that arrives 
	 * in a message from the nanokernel and redirect to the final handler.
	 * 
	 * TODO: if more exceptions become available in the API they should be 
	 * added here.
	 */

	// ECALL_TRP_VECT(0x0, _syncexception_entry); // 0x0 Instruction address misaligned
	// ECALL_TRP_VECT(0x1, _syncexception_entry); // 0x1 Instruction access fault
	// ECALL_TRP_VECT(0x2, _syncexception_entry); // 0x2 Illegal Instruction
    // ECALL_TRP_VECT(0x4, _syncexception_entry); // 0x4 Load address misaligned
    // ECALL_TRP_VECT(0x5, _syncexception_entry); // 0x5 Load access fault
    // ECALL_TRP_VECT(0x6, _syncexception_entry); // 0x6 Store/AMO address misaligned
	// ECALL_TRP_VECT(0x7, _syncexception_entry); // 0x7 Store access fault

	vConfigureTickInterrupt();
	uxCriticalNesting = 0;
}
/*-----------------------------------------------------------*/
