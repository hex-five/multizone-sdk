/**
 * Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved
 *
 * @author 	Sandro Pinto <sandro2pinto@gmail.com>
 * @author 	José Martins <josemartins90@gmail.com>
 *
 * TODO: add credits to original sifive port
 * 
 */

#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif



/* MultiZone Includes*/
#include "libhexfive.h"

/*-----------------------------------------------------------
 * Port specific definitions.
 *
 * The settings in this file configure FreeRTOS correctly for the
 * given hardware and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */


/* Type definitions. */
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	uint32_t
#define portBASE_TYPE	long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

typedef uint64_t TickType_t;
/**
 * MULTIZONE: despite the target being a 32 or 64-bit platform,
 * the timer is always 64 bits and accessing is always atomic since
 * it corresponds to an ecall to the nanokernel.
 */
#define portMAX_DELAY ( TickType_t ) 0xffffffffffffffffUL
#define portTICK_TYPE_IS_ATOMIC 1
/*-----------------------------------------------------------*/

/* Architecture specifics. */
#define portSTACK_GROWTH			( -1 )
#define portTICK_PERIOD_MS			( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT			8
/*-----------------------------------------------------------*/

/* Architecture specifics. */
extern void vPortYield(); //in portasm.S
extern int xPortSetInterruptMask(); //in port.c
extern void vPortClearInterruptMask( int uxSavedStatusValue ); //in port.c

/*-----------------------------------------------------------*/


/* Scheduler utilities. */
#define portEND_SWITCHING_ISR( xSwitchRequired )\
{												\
	extern uint32_t ulPortYieldRequired;        \
	if( xSwitchRequired != pdFALSE )			\
	{											\
		ulPortYieldRequired = pdTRUE;       	\
	}											\
}	

#define portYIELD_FROM_ISR( x ) portEND_SWITCHING_ISR( x )
#define portYIELD() vPortYield()

/* Critical section management. */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
#define portSET_INTERRUPT_MASK_FROM_ISR()                             xPortSetInterruptMask()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedStatusValue )       vPortClearInterruptMask( uxSavedStatusValue )
#define portDISABLE_INTERRUPTS()				ECALL_CSRC_MIE()
#define portENABLE_INTERRUPTS()					ECALL_CSRS_MIE()
#define portENTER_CRITICAL()					vPortEnterCritical()
#define portEXIT_CRITICAL()						vPortExitCritical()

/*-----------------------------------------------------------*/

/* Task function macros as described on the FreeRTOS.org WEB site.  These are
not necessary for to use this port.  They are defined so the common demo files
(which build with all the ports) will build. */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )
/*-----------------------------------------------------------*/

/* Tickless idle/low power functionality. */
#ifndef portSUPPRESS_TICKS_AND_SLEEP
	extern void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime );
	#define portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime ) vPortSuppressTicksAndSleep( xExpectedIdleTime )
#endif
/*-----------------------------------------------------------*/


#define portINLINE	__inline

#ifndef portFORCE_INLINE
	#define portFORCE_INLINE inline __attribute__(( always_inline))
#endif


#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */

