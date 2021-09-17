/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <string.h>	// strcmp()

#include "platform.h"
#include "multizone.h"
#include "owi_sequence.h"

#include "FreeRTOS.h"
#include "task.h"     /* RTOS task related API prototypes. */

typedef enum {zone1=1, zone2, zone3, zone4} Zone;

#define SPI_TDI 11 	// in
#define SPI_TCK 10	// out (master)
#define SPI_TDO  9  // out
//#define SPI_SYN  8  // out - not used

static uint8_t CRC8(const uint8_t bytes[]){

    const uint8_t generator = 0x1D;
    uint8_t crc = 0;

    for(int b=0; b<3; b++) {

        crc ^= bytes[b]; /* XOR-in the next input byte */

        for (int i = 0; i < 8; i++)
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ generator);
            else
                crc <<= 1;
    }

    return crc;
}
static uint32_t spi_rw(const uint32_t cmd){

    taskENTER_CRITICAL();

	const uint8_t bytes[] = {(uint8_t)cmd, (uint8_t)(cmd>>8), (uint8_t)(cmd>>16)};
	const uint32_t tx_data = bytes[0]<<24 | bytes[1]<<16 | bytes[2]<<8 | CRC8(bytes);

	uint32_t rx_data = 0;

    for (uint32_t i = 1<<31; i != 0; i >>= 1){

        BITSET(GPIO_BASE+GPIO_OUTPUT_VAL, 1 << SPI_TCK);

        if (tx_data & i)
            BITSET(GPIO_BASE+GPIO_OUTPUT_VAL, 1 << SPI_TDO);
        else
            BITCLR(GPIO_BASE+GPIO_OUTPUT_VAL, 1 << SPI_TDO);

        BITCLR(GPIO_BASE+GPIO_OUTPUT_VAL, 1 << SPI_TCK);

        if( GPIO_REG(GPIO_INPUT_VAL) & (1<< SPI_TDI) )
            rx_data |= i;

    }

    taskEXIT_CRITICAL();

	return rx_data;
}

#define CMD_DUMMY 0xFFFFFF
#define CMD_STOP  0x000000

static volatile char inbox[16] = {'\0'};
static volatile uint32_t usb_state = 0;
static volatile uint32_t man_cmd = CMD_STOP;

void msg_handler_task(void *pvParameters); TaskHandle_t msg_handler_task_handle;
void spi_poll_task(void *pvParameters); TaskHandle_t spi_poll_task_handle;
void robot_cmd_task(void *pvParameters); TaskHandle_t robot_cmd_task_handle;
void robot_seq_task(void *pvParameters); TaskHandle_t robot_seq_task_handle;

int main (void){

	//while(1) MZONE_WFI();
	//while(1) MZONE_YIELD();
	//while(1);

    /* Setup hardware */
    GPIO_REG(GPIO_INPUT_EN)  |= (1 << SPI_TDI);
    GPIO_REG(GPIO_PULLUP_EN) |= (1 << SPI_TDI);
    GPIO_REG(GPIO_OUTPUT_EN) |= ((1 << SPI_TCK) | (1<< SPI_TDO) | (1 << LED_RED) | (1 << LED_GRN));
    GPIO_REG(GPIO_DRIVE)     |= ((1 << SPI_TCK) | (1<< SPI_TDO));

    CSRS(mie, 1<<3); // enable msip/inbox interrupts

    /* Create the task. */
	xTaskCreate(msg_handler_task, "msg_handler_task", configMINIMAL_STACK_SIZE, NULL, 1, &msg_handler_task_handle);

    /* Create the task. */
	xTaskCreate(spi_poll_task, "spi_poll_task", configMINIMAL_STACK_SIZE, NULL, 0, &spi_poll_task_handle);

    /* Create the task. */
	xTaskCreate(robot_cmd_task, "robot_cmd_task", configMINIMAL_STACK_SIZE, NULL, 1, &robot_cmd_task_handle);

    /* Create the task. */
	xTaskCreate(robot_seq_task, "robot_seq_task", configMINIMAL_STACK_SIZE, NULL, 1, &robot_seq_task_handle);

    /* Start the tasks and timer running. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details.  */
    for( ;; );

}

void msg_handler_task( void *pvParameters ){ // msg_handler_task

	for( ;; ){

		vTaskSuspend(NULL); // wait for message

		// get a thread-safe copy of inbox[]
	    taskENTER_CRITICAL();
        char msg[16]; memcpy(msg, (const char *)inbox, sizeof msg);
        taskEXIT_CRITICAL();

		if (strcmp("ping", msg)==0){

			MZONE_SEND(zone1, (char [16]){"pong"});

		} else if (usb_state==0x12670000 && man_cmd==CMD_STOP){

			if (strcmp("stop", msg)==0)

			    owi_sequence_stop_req();

			else if (!owi_sequence_is_running()){

                if (strcmp("start", msg) == 0) {
                    owi_sequence_start(MAIN);
                    vTaskResume(robot_seq_task_handle);

                } else if (strcmp("fold", msg) == 0) {
                    owi_sequence_start(FOLD);
                    vTaskResume(robot_seq_task_handle);

                } else if (strcmp("unfold", msg) == 0) {
                    owi_sequence_start(UNFOLD);
                    vTaskResume(robot_seq_task_handle);

                } else if (strnlen(msg, sizeof msg)==1){

                    // Manual single-command adjustments
                    switch (msg[0]){
                        case 'q' : man_cmd = 0x000001; break; // grip close
                        case 'a' : man_cmd = 0x000002; break; // grip open
                        case 'w' : man_cmd = 0x000004; break; // wrist up
                        case 's' : man_cmd = 0x000008; break; // wrist down
                        case 'e' : man_cmd = 0x000010; break; // elbow up
                        case 'd' : man_cmd = 0x000020; break; // elbow down
                        case 'r' : man_cmd = 0x000040; break; // shoulder up
                        case 'f' : man_cmd = 0x000080; break; // shoulder down
                        case 't' : man_cmd = 0x000100; break; // base clockwise
                        case 'g' : man_cmd = 0x000200; break; // base counterclockwise
                        case 'y' : man_cmd = 0x010000; break; // light on
                    }

                    if (man_cmd != CMD_STOP) vTaskResume(robot_cmd_task_handle);

                }

			}

		}

	}

}

void spi_poll_task( void *pvParameters ){ // spi_poll_task

	for( ;; ){

		/* Send keep alive packet, read spi, update USB state
		rx_data == 0xFFFFFFFF : no spi/usb adapter, robot uknown
		rx_data == 0x00000000: spi/usb adabter on, robot off
		rx_data == 0xFFFFFFFF : spi/usb adapter on, robot on */
		const uint32_t rx_data = spi_rw(CMD_DUMMY);

		/* Report USB state changes */
	    if (rx_data != usb_state){

	    	if (rx_data==0x12670000){
				MZONE_SEND(zone1, (char [16]){"USB ID 12670000"});

	    	} else if (usb_state==0x12670000){
                owi_sequence_stop();
                MZONE_SEND(zone1, (char [16]){"USB DISCONNECT"});
	    	}
	    }

	    /* update USB state */
	    usb_state = rx_data;

	    /* Blink LED */
	    const int LED = (usb_state==0x12670000 ? LED_GRN : LED_RED);

	    BITSET(GPIO_BASE+GPIO_OUTPUT_VAL, 1<<LED);

	    vTaskDelay( (TickType_t) (25 / portTICK_PERIOD_MS) );

        BITCLR(GPIO_BASE+GPIO_OUTPUT_VAL, 1<<LED);

	    /* loop approx every 1 sec */
	    vTaskDelay( (TickType_t) (975 / portTICK_PERIOD_MS) );

	}

}

void robot_cmd_task( void *pvParameters ){ // robot_cmd_task

	for( ;; ){

		vTaskSuspend(NULL);

		spi_rw(man_cmd);

		vTaskDelay( (TickType_t) (200 / portTICK_PERIOD_MS) );

	    spi_rw(man_cmd = CMD_STOP);


	}

}

void robot_seq_task( void *pvParameters ){ // robot_seq_task

    for (;;) {

        vTaskSuspend(NULL);

        TickType_t xLastWakeTime = xTaskGetTickCount();

        while (usb_state == 0x12670000 && owi_sequence_next() != -1) {

            spi_rw(owi_sequence_get_cmd());

            vTaskDelayUntil(&xLastWakeTime, (TickType_t) (owi_sequence_get_ms() / portTICK_PERIOD_MS));

        }

    }

}

void vApplicationIdleHook( void ){

	/* The idle task can optionally call an application defined hook (or callback)
	function – the idle hook. The idle task runs at the very lowest priority, so
	such an idle hook function will only get executed when there are no tasks of
	higher priority that are able to run. This makes the idle hook function an
	ideal place to put the processor into a low power state – providing an
	automatic power saving whenever there is no processing to be performed.
	see also https://freertos.org/low-power-tickless-rtos.html */

	MZONE_WFI();

	/* MultiZone deep-sleep implementation:
	set configUSE_TICKLESS_IDLE 1 and configUSE_IDLE_HOOK 0 to enable
	MultiZone vPortSuppressTicksAndSleep() */

}

void vApplicationMallocFailedHook( void ){

	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */

	taskDISABLE_INTERRUPTS();
	for( ;; );
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName ){

	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */

	taskDISABLE_INTERRUPTS();
	for( ;; );
}

// freertos_risc_v_chip_specific_extensions.h: #define portasmHANDLE_INTERRUPT trap_handler
void trap_handler(uint32_t cause){

	switch (cause) {

	case 0x80000003:	// msip/inbox

		// read the incoming message & clear msip
		;char buff[16];
		if (MZONE_RECV(1, buff))
			memcpy((char *)inbox, buff, sizeof buff);

		// Resume the suspended task.
		const BaseType_t xYieldRequired = xTaskResumeFromISR( msg_handler_task_handle );

		// We should switch context so the ISR returns to a different task.
		portYIELD_FROM_ISR( xYieldRequired );

		break;

	default:

		taskDISABLE_INTERRUPTS();
		for (;;) ;

	}

}
