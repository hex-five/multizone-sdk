/**
 * Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved
 *
 * @author 	Sandro Pinto <sandro2pinto@gmail.com>
 * @author 	José Martins <josemartins90@gmail.com>
 *
 * TODO: add credits to original sifive port
 * 
 */

/* Kernel includes. */
#include "FreeRTOS.h" /* Must come first. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */

/* TODO Add any manufacture supplied header files can be included
here. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "platform.h"
#include "plic_driver.h"

#include <libhexfive.h>



/*-----------------------------------------------------------*/

extern void _interrupt_entry();
static void prvSetupHardware( void );
static void prvTask( void *pvParameters );

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

#define RED_LED_OFFSET   0
#define GREEN_LED_OFFSET 1
#define BLUE_LED_OFFSET  2

#define BUTTON_0_OFFSET 15
#define BUTTON_1_OFFSET 30
#define BUTTON_2_OFFSET 31

#define GPIO_INT_BASE 7
#define INT_DEVICE_BUTTON_0 (GPIO_INT_BASE + BTN0)
#define INT_DEVICE_BUTTON_1 (GPIO_INT_BASE + BTN1)
#define INT_DEVICE_BUTTON_2 (GPIO_INT_BASE + BTN2)

#define LD1_RED_ON PWM_REG(PWM_CMP1)  = 0x00;
#define LD1_GRN_ON PWM_REG(PWM_CMP2)  = 0x00;
#define LD1_BLU_ON PWM_REG(PWM_CMP3)  = 0x00;

#define LD1_RED_OFF PWM_REG(PWM_CMP1)  = 0xFF;
#define LD1_GRN_OFF PWM_REG(PWM_CMP2)  = 0xFF;
#define LD1_BLU_OFF PWM_REG(PWM_CMP3)  = 0xFF;

#define IRQ_M_LOCAL        16
#define MCAUSE_CAUSE       0x3FFUL
#define IRQ_M_EXT          11

// Application specific.
static volatile int led = GREEN_LED_OFFSET;

/*-------------------------------------------------------------*/

int main(void)
{

    // while(1){
	// 	int msg[4]={0,0,0,0};
	// 	ECALL_RECV(4, msg);
	// 	if (msg[0]) ECALL_SEND(4, msg);
	// 	ECALL_YIELD();
	// }


    /* Configure the system ready to run the demo.  The clock configuration
    can be done here if it was not done before main() was called. */
    prvSetupHardware();

    /* Create the task. */
	xTaskCreate(    prvTask,
					"prvTask",
					configMINIMAL_STACK_SIZE,
					NULL,
					0x02,
					NULL );


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


static void prvTask( void *pvParameters )
{

    uint16_t r=0x3F;
    uint16_t g=0;
    uint16_t b=0;

    PWM_REG(PWM_CFG)   = 0;
    PWM_REG(PWM_CFG)   = (PWM_CFG_ENALWAYS) | (PWM_CFG_ZEROCMP) | (PWM_CFG_DEGLITCH);
    PWM_REG(PWM_COUNT) = 0;

    // The LEDs are intentionally left somewhat dim.
    PWM_REG(PWM_CMP0)  = 0xFE;

    while(1){

        // const uint64_t T1 = ECALL_CSRR_MTIME() + 400; //12*RTC_FREQ/1000;
        // while (ECALL_CSRR_MTIME() < T1) ECALL_YIELD();
        vTaskDelay(40/portTICK_PERIOD_MS);

        if(r > 0 && b == 0){ r--; g++; }
        if(g > 0 && r == 0){ g--; b++; }
        if(b > 0 && g == 0){ r++; b--; }

        PWM_REG(PWM_CMP1)  = 0xFF - (r >> 2);
        PWM_REG(PWM_CMP2)  = 0xFF - (g >> 2);
        PWM_REG(PWM_CMP3)  = 0xFF - (b >> 2);

    }// While (1)
}

/*ISR triggered by Button 3 */

void button_1_handler(void){ // local interrupt

	// GPIO_REG(GPIO_OUTPUT_VAL) &= ~(0x1 << led); // LED OFF

	// led = (led==GREEN_LED_OFFSET ? BLUE_LED_OFFSET :
	// 	   led==BLUE_LED_OFFSET  ? RED_LED_OFFSET :
	// 			   	   	   	   	   GREEN_LED_OFFSET
	// );

    ECALL_SEND(1, ((int[]){'p',0,0,0}));

	// volatile uint64_t * now, then;
	// now = (volatile uint64_t*)(CLINT_CTRL_ADDR + CLINT_MTIME);
	// then = *now + 500*32768/1000;
	// while (*now < then) ECALL_YIELD();

	//GPIO_REG(GPIO_RISE_IP) |= (1<<BUTTON_3_OFFSET);

}

/*-----------------------------------------------------------*/

void handle_interrupt(unsigned long mcause){

  /* check if global*/
  if(!((mcause & MCAUSE_CAUSE) == IRQ_M_EXT))   {
    localISR[mcause & MCAUSE_CAUSE]();
  } else {
      //global interrupt
    plic_source int_num  = PLIC_claim_interrupt(&g_plic);
    g_ext_interrupt_handlers[int_num]();
    PLIC_complete_interrupt(&g_plic, int_num);
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
    ECALL_IRQ_VECT(int_num, _interrupt_entry);
}
/*-----------------------------------------------------------*/

/*
 *enables the plic and programs handlers
**/
void interrupts_init(  ) {
    
    /**
     * MULTIZONE: multizone API does not allow us to enable or disable
     * interrupts. After we register an handler for a interrupt it is 
     * permantently enabled. At this point, no handler was registered,
     * so we know all interrupts are disabled.
     */
    // Disable the machine & timer interrupts until setup is done.
    //clear_csr(mie, MIP_MEIP);
    //clear_csr(mie, MIP_MTIP);


  //setup PLIC
  PLIC_init(&g_plic,
	    PLIC_BASE,
	    PLIC_NUM_INTERRUPTS,
	    PLIC_NUM_PRIORITIES);

  //assign interrupts to default handler
  for (int ii = 0; ii < PLIC_NUM_INTERRUPTS; ii ++){
    g_ext_interrupt_handlers[ii] = no_interrupt_handler;
  }

  for (int isr = 0; isr < 32; isr++){
    localISR[isr] = no_interrupt_handler;
  }

    /**
     * MULTIZONE: multizone API has no notion of a global exertnal interrupt
     * enable. As mentioned in the comment above, after an handler for an
     * interrupt is registered, that interrupt is enabled (permantely).
     */
    // Enable the Machine-External bit in MIE
    //set_csr(mie, MIP_MEIP);

}
/*-----------------------------------------------------------*/


/*configures Button3 as a local interrupt*/
void b1_irq_init() {

    //dissable hw io function
    GPIO_REG(GPIO_IOF_EN ) &=  ~(1 << BUTTON_1_OFFSET);

    //set to input
    GPIO_REG(GPIO_INPUT_EN)   |= (1<<BUTTON_1_OFFSET);
    GPIO_REG(GPIO_PULLUP_EN)  |= (1<<BUTTON_1_OFFSET);

    //set to interrupt on rising edge
    GPIO_REG(GPIO_RISE_IE)    |= (1<<BUTTON_1_OFFSET);

    //enable the interrupt
    ECALL_IRQ_VECT(16+LOCAL_INT_BTN_0, _interrupt_entry);
    ECALL_IRQ_VECT(16+LOCAL_INT_BTN_1, _interrupt_entry);
    ECALL_IRQ_VECT(16+LOCAL_INT_BTN_2, _interrupt_entry);
    localISR[IRQ_M_LOCAL + LOCAL_INT_BTN_0] = button_1_handler;
    localISR[IRQ_M_LOCAL + LOCAL_INT_BTN_1] = button_1_handler;
    localISR[IRQ_M_LOCAL + LOCAL_INT_BTN_2] = button_1_handler;
    //enable_interrupt(INT_DEVICE_BUTTON_3, 2, &b0_ISR);
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{

    interrupts_init();
    b1_irq_init();
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


