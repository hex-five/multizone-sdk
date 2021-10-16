/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <string.h>	// strcmp()

#include "platform.h"
#include "multizone.h"
#include "owi_sequence.h"

#define SPI_TDI 11 	// in
#define SPI_TCK 10	// out (master)
#define SPI_TDO  9  // out
//#define SPI_SYN  8  // out - not used

#define MAN_CMD_TIME RTC_FREQ/1000*200 // 200ms
#define KEEP_ALIVE_TIME 1*RTC_FREQ 	   // 1 sec
#define LED_TIME RTC_FREQ/1000*20      //  20ms

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

	return rx_data;
}

#define CMD_DUMMY 0xFFFFFF
#define CMD_STOP  0x000000

static volatile char msg[16] = {'\0'};
static volatile uint32_t usb_state = 0;
static volatile uint32_t man_cmd = CMD_STOP;
static volatile int LED = LED_RED;

typedef enum {zone1=1, zone2, zone3, zone4} Zone;

typedef enum {
	ROBOT_SEQ_TASK,
	ROBOT_CMD_TASK,
	KEEP_ALIVE_TASK,
	LED_OFF_TASK
} Task;

uint64_t robot_seq_task(); 	// OWI sequence
uint64_t robot_cmd_task(); 	// Manual cmd stop
uint64_t keep_alive_task(); // Keep alive
uint64_t led_off_task(); 	// LED off

static struct {
	uint64_t (*task)(void);
	uint64_t timecmp;
} timer[] = {{robot_seq_task, UINT64_MAX},
	{robot_cmd_task, UINT64_MAX},
	{keep_alive_task, UINT64_MAX},
	{led_off_task, UINT64_MAX}
};

void timer_set(const Task task, const uint64_t timecmp){

	timer[task].timecmp = timecmp;

	uint64_t timecmp_min = UINT64_MAX;
	for (size_t i=0; i<sizeof(timer)/sizeof(timer[0]); i++)
		timecmp_min = timer[i].timecmp < timecmp_min ? timer[i].timecmp : timecmp_min;

	MZONE_WRTIMECMP(timecmp_min);

}
void timer_handler(const uint64_t time){

	for (size_t i=0; i<sizeof(timer)/sizeof(timer[0]); i++)
		if(time >= timer[i].timecmp){
			timer_set(i, timer[i].task());
			break;
		}
}

uint64_t robot_seq_task(){ // OWI sequence

	uint64_t timecmp = UINT64_MAX;

	if (usb_state==0x12670000){

		if (owi_sequence_next()!=-1){
			spi_rw(owi_sequence_get_cmd());
			timecmp = MZONE_RDTIME() + owi_sequence_get_ms()*RTC_FREQ/1000;
		}

	}

	return timecmp;

}
uint64_t robot_cmd_task(){ // Manual cmd stop
	spi_rw(man_cmd = CMD_STOP);
	return UINT64_MAX;
}
uint64_t keep_alive_task(){ // Keep alive 1sec

	// Send keep alive packet and check ret value
	volatile uint32_t rx_data = spi_rw(CMD_DUMMY);

    // Update USB state (0xFFFFFFFF no spi/usb adapter)
    if (rx_data != usb_state){
    	if (rx_data==0x12670000){
    		LED = LED_GRN;
    		MZONE_SEND(1, (char[16]){"USB ID 12670000"});
    	} else if (usb_state==0x12670000){
    		LED = LED_RED;
    		MZONE_SEND(1, (char[16]){"USB DISCONNECT"});
    		owi_sequence_stop();
    	}
    	usb_state=rx_data;
    }

	const uint64_t time = MZONE_RDTIME();

    // Turn on LED & start LED timer
	BITSET(GPIO_BASE+GPIO_OUTPUT_VAL, 1<<LED);
    timer_set(LED_OFF_TASK, time + LED_TIME);

	return time + KEEP_ALIVE_TIME;
}
uint64_t led_off_task(){ // LED off
    BITCLR(GPIO_BASE+GPIO_OUTPUT_VAL, 1<<LED);
	return UINT64_MAX;
}

__attribute__(( interrupt())) void trap_handler(void){

	#define IRQ ( 1UL<< (__riscv_xlen-1) )

	switch(MZONE_CSRR(CSR_MCAUSE)){
		case 0 : break; // Instruction address misaligned
		case 1 : break; // Instruction access fault
		case 3 : break; // Breakpoint
		case 4 : break; // Load address misaligned
		case 5 : break; // Load access fault
		case 6 : break; // Store/AMO address misaligned
		case 7 : break; // Store access fault
		case 8 : break; // Environment call from U-mode

		case IRQ | 3 :  // Software interrupt (inbox)
			;char const tmp[16];
			if (MZONE_RECV(1, tmp))
				memcpy((char *)msg, tmp, sizeof msg);
			return;

		case IRQ | 7 :  // Muliplexed timer
			timer_handler(MZONE_RDTIME());
			return;

	}

	for( ;; );

}

void msg_handler(const Zone zone, const char *msg){

	if (strcmp("ping", msg)==0){
		MZONE_SEND(zone, "pong");

	} else if (usb_state==0x12670000 && man_cmd==CMD_STOP){

		if (strcmp("stop", msg)==0) owi_sequence_stop_req();

		else if (!owi_sequence_is_running()){

			     if (strcmp("start", msg)==0) {owi_sequence_start(MAIN);   timer_set(ROBOT_SEQ_TASK, 0);}
			else if (strcmp("fold",  msg)==0) {owi_sequence_start(FOLD);   timer_set(ROBOT_SEQ_TASK, 0);}
			else if (strcmp("unfold",msg)==0) {owi_sequence_start(UNFOLD); timer_set(ROBOT_SEQ_TASK, 0);}

			// Manual single-command adjustments
			else if (strcmp("q", msg)==0) man_cmd = 0x000001; // grip close
			else if (strcmp("a", msg)==0) man_cmd = 0x000002; // grip open
			else if (strcmp("w", msg)==0) man_cmd = 0x000004; // wrist up
			else if (strcmp("s", msg)==0) man_cmd = 0x000008; // wrist down
			else if (strcmp("e", msg)==0) man_cmd = 0x000010; // elbow up
			else if (strcmp("d", msg)==0) man_cmd = 0x000020; // elbow down
			else if (strcmp("r", msg)==0) man_cmd = 0x000040; // shoulder up
			else if (strcmp("f", msg)==0) man_cmd = 0x000080; // shoulder down
			else if (strcmp("t", msg)==0) man_cmd = 0x000100; // base clockwise
			else if (strcmp("g", msg)==0) man_cmd = 0x000200; // base counterclockwise
			else if (strcmp("y", msg)==0) man_cmd = 0x010000; // light on

			if (man_cmd != CMD_STOP){
				spi_rw(man_cmd);
				timer_set(ROBOT_CMD_TASK, MZONE_RDTIME() + MAN_CMD_TIME);
			}

		}

	}

}

int main (void){

	//while(1) MZONE_WFI();
	//while(1) MZONE_YIELD();
	//while(1);

	GPIO_REG(GPIO_INPUT_EN)  |=  (1 << SPI_TDI);
	GPIO_REG(GPIO_PULLUP_EN) |=  (1 << SPI_TDI);
	GPIO_REG(GPIO_OUTPUT_EN) |=  (1 << SPI_TCK | 1<< SPI_TDO | 1 << LED_RED | 1 << LED_GRN );
    GPIO_REG(GPIO_DRIVE)     |=  (1 << SPI_TCK | 1<< SPI_TDO );

	CSRW(mtvec, trap_handler);  // register trap handler
	CSRS(mie, 1<<3);    		// enable msip/inbox interrupt
	CSRS(mie, 1<<7); 			// enable timer interrupts
    CSRS(mstatus, 1<<3);		// enable global interrupts

    // Start task2: Hartbeat LED, USB status, Keep alive pkt
    timer_set(KEEP_ALIVE_TASK, 0);

	while(1){

		// Message handler
		CSRC(mie, 1 << 3);
			if (msg[0] != '\0') {
				msg_handler(zone1, (char *)msg);
				msg[0] = '\0';
			}
		CSRS(mie, 1 << 3);

		// wait for next message
	    MZONE_WFI();

	}

}
