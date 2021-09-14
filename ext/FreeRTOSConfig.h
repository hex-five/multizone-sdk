/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Here is a good place to include header files that are required across
your application. */
#include "platform.h"

#define configISR_STACK_SIZE_WORDS 		(100)
#define configMTIME_BASE_ADDRESS		( CLINT_BASE + CLINT_MTIME )
#define configMTIMECMP_BASE_ADDRESS		( CLINT_BASE + CLINT_MTIMECMP )

#define configUSE_PREEMPTION                    1
// #define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 1
#define configCPU_CLOCK_HZ                      ( (TickType_t) RTC_FREQ )
#define configTICK_RATE_HZ                      ( (TickType_t) 1000 )
#define configMAX_PRIORITIES                    4 	//5
#define configMINIMAL_STACK_SIZE                128 // 32-bit words
// #define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
// #define configIDLE_SHOULD_YIELD                 1
// #define configUSE_TASK_NOTIFICATIONS            1
// #define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
// #define configUSE_MUTEXES                       0
// #define configUSE_RECURSIVE_MUTEXES             0
// #define configUSE_COUNTING_SEMAPHORES           0
// #define configQUEUE_REGISTRY_SIZE               10
// #define configUSE_QUEUE_SETS                    0
// #define configUSE_TIME_SLICING                  1
// #define configUSE_NEWLIB_REENTRANT              0
// #define configENABLE_BACKWARD_COMPATIBILITY     0
// #define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5
// #define configSTACK_DEPTH_TYPE                  uint16_t
// #define configMESSAGE_BUFFER_LENGTH_TYPE        size_t

/* Memory allocation related definitions. */
// #define configSUPPORT_STATIC_ALLOCATION         1
// #define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   4*1024
// #define configAPPLICATION_ALLOCATED_HEAP        1

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          0 //2
#define configUSE_MALLOC_FAILED_HOOK            0 //1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* Run time and task stats gathering related definitions. */
// #define configGENERATE_RUN_TIME_STATS           0
// #define configUSE_TRACE_FACILITY                0
// #define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* Software timer related definitions. */
// #define configUSE_TIMERS                        1
// #define configTIMER_TASK_PRIORITY               3
// #define configTIMER_QUEUE_LENGTH                10
// #define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* Define to trap errors during development. */
// #define configASSERT( ( x ) ) if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ )
// #define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); __asm volatile( "ebreak" ); for( ;; ); }

/* Optional functions - most linkers will remove unused functions anyway. */
// #define INCLUDE_vTaskPrioritySet                1
// #define INCLUDE_uxTaskPriorityGet               1
// #define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
// #define INCLUDE_xTaskGetSchedulerState          1
// #define INCLUDE_xTaskGetCurrentTaskHandle       1
// #define INCLUDE_uxTaskGetStackHighWaterMark     0
// #define INCLUDE_xTaskGetIdleTaskHandle          0
// #define INCLUDE_eTaskGetState                   0
// #define INCLUDE_xEventGroupSetBitFromISR        1
// #define INCLUDE_xTimerPendFunctionCall          0
// #define INCLUDE_xTaskAbortDelay                 0
// #define INCLUDE_xTaskGetHandle                  0
// #define INCLUDE_xTaskResumeFromISR              1

/* A header file that defines trace macro can be included here. */

#endif /* FREERTOS_CONFIG_H */
