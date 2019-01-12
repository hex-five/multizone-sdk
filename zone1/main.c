/**
 * Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved
 *
 * @author 	Sandro Pinto <sandro2pinto@gmail.com>
 * @author 	José Martins <josemartins90@gmail.com>
 * 
 */

/* Kernel includes. */
#include <FreeRTOS.h> /* Must come first. */
#include <task.h>     /* RTOS task related API prototypes. */
#include <queue.h>   /* RTOS queue related API prototypes. */
#include <timers.h>   /* Software timer related API prototypes. */
#include <semphr.h>   /* Semaphore related API prototypes. */
#include <event_groups.h>

/* TODO Add any manufacture supplied header files can be included
here. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "platform.h"
#include "plic_driver.h"

#include <libhexfive.h>
#include <comm.h>



/*-----------------------------------------------------------*/

extern void _interrupt_entry();
static void prvSetupHardware( void );
static void ledFadeTask( void *pvParameters );
static void pingTask( void *pvParameters );
static void cliEchoTask( void *pvParameters);

TaskHandle_t ledfade_task;
TimerHandle_t ledfade_timer;
EventGroupHandle_t ledfade_event;

struct mz_channel cli;

void ledfade_callback( TimerHandle_t xTimer );

/*-----------------------------------------------------------*/

// Structures for registering different interrupt handlers
// for different parts of the application.
typedef void (*function_ptr_t) (void);
void no_interrupt_handler (void) {};
function_ptr_t g_ext_interrupt_handlers[PLIC_NUM_INTERRUPTS];
function_ptr_t localISR[32]; 

// Instance data for the PLIC.
plic_instance_t g_plic;

/*-----------------------------------------------------------*/

#define LOCAL_INT_BTN_0  0
#define LOCAL_INT_BTN_1  1
#define LOCAL_INT_BTN_2  2

#define BUTTON_0_OFFSET 15
#define BUTTON_1_OFFSET 30
#define BUTTON_2_OFFSET 31

#define LD1_GRN_ON PWM_REG(PWM_CMP1)  = 0x00;
#define LD1_BLU_ON PWM_REG(PWM_CMP2)  = 0x00;
#define LD1_RED_ON PWM_REG(PWM_CMP3)  = 0x00;

#define LD1_GRN_OFF PWM_REG(PWM_CMP1)  = 0xFF;
#define LD1_BLU_OFF PWM_REG(PWM_CMP2)  = 0xFF;
#define LD1_RED_OFF PWM_REG(PWM_CMP3)  = 0xFF;

#define IRQ_M_LOCAL        16
#define MCAUSE_CAUSE       0x3FFUL
#define IRQ_M_EXT          11

/*-------------------------------------------------------------*/

int main(void)
{

	/* Setup platform-specific hardware. */
    prvSetupHardware();

    ledfade_timer = xTimerCreate("ledfade_timer", 3000/portTICK_PERIOD_MS, 
                pdFALSE, ( void * ) 0, ledfade_callback);

    ledfade_event = xEventGroupCreate();
    xEventGroupSetBits( ledfade_event, 1);

    /* Create the task. */
	xTaskCreate(ledFadeTask, "ledFadeTask", configMINIMAL_STACK_SIZE, NULL, 0x02,
            &ledfade_task);
    // xTaskCreate(pingTask, "pingTask", configMINIMAL_STACK_SIZE, NULL, 0x00, /* must have idle priority */
    //         NULL);
    xTaskCreate(cliEchoTask, "cliEchoTask", configMINIMAL_STACK_SIZE, NULL, 0x00,
            NULL);


    /* Start the tasks and timer running. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details.  */
    for( ;; );
}
/*-----------------------------------------------------------*/

void ledfade_callback( TimerHandle_t xTimer ){
    xEventGroupSetBits(ledfade_event, 1);
    //vTaskResume(ledfade_task);
}

static void pingTask( void *pvParameters )
{
	while(1){

		int msg[4]={0,0,0,0};
		ECALL_RECV(4, msg);
		if (msg[0]) ECALL_SEND(4, msg);

        mz_channel_update(&cli);

		taskYIELD();

	}
}

#define ACK    0
#define IND    1
#define CTL    2
#define DAT    3

#define CTL_ACK    (1 << 0)
#define CTL_DAT    (1 << 1)

static void cliEchoTask( void *pvParameters){

    static int ack_pending = 0;
    static int ack_index = 0;
    static int resend = 0;
    static int msg_out[4] = {-1,0,0,0};

    int msg[4] = {0,0,0,0};


    while(1){

        ECALL_RECV(2, (void*)msg);

        if((msg[CTL] & CTL_DAT) != 0){

                msg_out[DAT] = msg[DAT];
                msg_out[IND] =  msg[IND];
                msg_out[ACK] = msg[IND];
                msg_out[CTL] |= CTL_ACK | CTL_DAT;

                ECALL_SEND(2, (void*)msg_out);
        }

        //portYIELD();
        ECALL_YIELD();  
    }

}

static void ledFadeTask( void *pvParameters )
{

    uint16_t r=0x3F;
    uint16_t g=0;
    uint16_t b=0;

    GPIO_REG(GPIO_IOF_EN) |= IOF1_PWM1_MASK;
    GPIO_REG(GPIO_IOF_SEL) |= IOF1_PWM1_MASK;

    PWM_REG(PWM_CFG)   = 0;
    PWM_REG(PWM_CFG)   = (PWM_CFG_ENALWAYS) | (PWM_CFG_ZEROCMP) | (PWM_CFG_DEGLITCH);
    PWM_REG(PWM_COUNT) = 0;

    // The LEDs are intentionally left somewhat dim.
    PWM_REG(PWM_CMP0)  = 0xFE;

    while(1){

        // const uint64_t T1 = ECALL_CSRR_MTIME() + 400; //12*RTC_FREQ/1000;
        // while (ECALL_CSRR_MTIME() < T1) ECALL_YIELD();
        vTaskDelay(6/portTICK_PERIOD_MS);

        xEventGroupWaitBits(ledfade_event, 1, pdFALSE, pdFALSE, portMAX_DELAY );

        if(r > 0 && b == 0){ r--; g++; }
        if(g > 0 && r == 0){ g--; b++; }
        if(b > 0 && g == 0){ r++; b--; }

        PWM_REG(PWM_CMP1)  = 0xFF - (r >> 2);
        PWM_REG(PWM_CMP2)  = 0xFF - (g >> 2);
        PWM_REG(PWM_CMP3)  = 0xFF - (b >> 2);

    }// While (1)
}

/*ISR triggered by Button 3 */

void button_0_handler(void){ // local interrupt

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	GPIO_REG(GPIO_RISE_IP)  |= (1<<BUTTON_0_OFFSET);

	LD1_RED_OFF; LD1_GRN_ON; LD1_BLU_OFF;

    xEventGroupClearBitsFromISR(ledfade_event, 1);
    //vTaskSuspend(ledfade_task);
    xTimerResetFromISR(ledfade_timer, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
   
    ECALL_SEND(4, (int[4]){216,0,0,0} );
   
}

void button_1_handler(void){ // local interrupt

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	GPIO_REG(GPIO_RISE_IP)  |= (1<<BUTTON_1_OFFSET);

	LD1_RED_OFF; LD1_GRN_OFF; LD1_BLU_ON;

    xEventGroupClearBitsFromISR(ledfade_event, 1);
    //vTaskSuspend(ledfade_task);
    xTimerResetFromISR(ledfade_timer, &xHigherPriorityTaskWoken);
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
   
    ECALL_SEND(4, (int[4]){217,0,0,0} );
    
}

void button_2_handler(void){ // local interrupt

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	GPIO_REG(GPIO_RISE_IP)  |= (1<<BUTTON_2_OFFSET);

	LD1_RED_ON; LD1_GRN_OFF; LD1_BLU_OFF;

    xEventGroupClearBitsFromISR(ledfade_event, 1);
    //vTaskSuspend(ledfade_task);
    xTimerResetFromISR(ledfade_timer, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
   
    ECALL_SEND(4, (int[4]){218,0,0,0} );
    
}

/*-----------------------------------------------------------*/

void handle_interrupt(unsigned long mcause){

  /* check if global*/
  if(!((mcause & MCAUSE_CAUSE) == IRQ_M_EXT))   {
    localISR[mcause & MCAUSE_CAUSE]();
  } 

}

/*-----------------------------------------------------------*/

unsigned long handle_syncexception(unsigned long mcause, unsigned long mtval, unsigned long mepc){

    switch(mcause){

        case 0x0: // Instruction address misaligned 
            break;
        case 0x1: // Instruction access fault       
            break;
        case 0x2: // Illegal Instruction            
            break;
        case 0x4: // Load address misaligned        
            break; 
        case 0x5: // Load access fault              
            break;
        case 0x6: // Store/AMO address misaligned   
            break;
        case 0x7: // Store access fault             
            break;
        default:
            break;
    }

    /**
     *  MULTIZONE: this method must rearrange the epc, and set it to where it 
     *  wants to return.
     */
    mepc += 4;
    return mepc;
}

/*-----------------------------------------------------------*/


//enables interrupt and assigns handler
void enable_interrupt(uint32_t int_num, uint32_t int_priority, function_ptr_t handler) {
    g_ext_interrupt_handlers[int_num] = handler;
    PLIC_set_priority(&g_plic, int_num, int_priority);
    PLIC_enable_interrupt (&g_plic, int_num);
    /**
     * MULTIZONE: every interrupt must be registered to the same initial handler. 
     * After decoding the mcause and aknowleding the interrupt on the PLIC (in case
     * of a global interrupt) execution will be redirected to the final handler.
     */
    ECALL_IRQ_VECT(IRQ_M_EXT, _interrupt_entry);
}
/*-----------------------------------------------------------*/


void interrupts_init( ) {
    
  for (int isr = 0; isr < 32; isr++){
    localISR[isr] = no_interrupt_handler;
  }

}
/*-----------------------------------------------------------*/

void b0_irq_init() {

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN ) &=  ~(1 << BUTTON_0_OFFSET);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BUTTON_0_OFFSET);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BUTTON_0_OFFSET);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BUTTON_0_OFFSET);

    //enable the interrupt
    ECALL_IRQ_VECT(16+LOCAL_INT_BTN_0, _interrupt_entry);
    localISR[IRQ_M_LOCAL + LOCAL_INT_BTN_0] = button_0_handler;
}


void b1_irq_init() {

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN ) &=  ~(1 << BUTTON_1_OFFSET);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BUTTON_1_OFFSET);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BUTTON_1_OFFSET);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BUTTON_1_OFFSET);

    //enable the interrupt
    ECALL_IRQ_VECT(16+LOCAL_INT_BTN_1, _interrupt_entry);
    localISR[IRQ_M_LOCAL + LOCAL_INT_BTN_1] = button_1_handler;
}

void b2_irq_init() {

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN ) &=  ~(1 << BUTTON_2_OFFSET);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BUTTON_2_OFFSET);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BUTTON_2_OFFSET);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BUTTON_2_OFFSET);

    //enable the interrupt
    ECALL_IRQ_VECT(16+LOCAL_INT_BTN_2, _interrupt_entry);
    localISR[IRQ_M_LOCAL + LOCAL_INT_BTN_2] = button_2_handler;
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
    interrupts_init();
    b0_irq_init();
    b1_irq_init();
    b2_irq_init();
}


/*--- Definition of hooks useful for debugging purposes ---*/

void vApplicationMallocFailedHook( void )
{
    /* The malloc failed hook is enabled by setting
    configUSE_MALLOC_FAILED_HOOK to 1 in FreeRTOSConfig.h.

    Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
    write(1,"malloc failed\n", 14);
    
    for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) xTask;

    /* Run time stack overflow checking is performed if
    configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected.  pxCurrentTCB can be
    inspected in the debugger if the task name passed into this function is
    corrupt. */
    write(1, "Stack Overflow\n", 15);
    for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
    ECALL_YIELD();
}


