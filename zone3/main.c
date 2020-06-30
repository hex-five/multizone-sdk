/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <string.h>	// strcmp()

#include "platform.h"
#include "multizone.h"
#include "owi_sequence.h"

#define SPI_TDI 11 	// in
#define SPI_TCK 10	// out (master)
#define SPI_TDO  9  // out
#define SPI_SYN  8  // out - not used

#define MAN_CMD_TIME 250*RTC_FREQ/1000 // 250ms
#define KEEP_ALIVE_TIME 1*RTC_FREQ // 1 sec
#define LED_TIME 20*RTC_FREQ/1000 //  20ms

static uint8_t CRC8(const uint8_t const bytes[]){

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

	const uint8_t const bytes[] = {(uint8_t)cmd, (uint8_t)(cmd>>8), (uint8_t)(cmd>>16)};
	const uint32_t tx_data = bytes[0]<<24 | bytes[1]<<16 | bytes[2]<<8 | CRC8(bytes);

	uint32_t rx_data = 0;

	for (int i=32-1, bit; i>=0; i--){

		bit = (tx_data >> i) & 1U;
		GPIO_REG(GPIO_OUTPUT_VAL) = (bit==1 ? GPIO_REG(GPIO_OUTPUT_VAL) | (1 << SPI_TDO) :
											  GPIO_REG(GPIO_OUTPUT_VAL) & ~(1 << SPI_TDO)  );

		GPIO_REG(GPIO_OUTPUT_VAL) |= (1 << SPI_TCK); volatile int w1=0; while(w1<5) w1++;
		GPIO_REG(GPIO_OUTPUT_VAL) ^= (1 << SPI_TCK); volatile int w2=0; while(w2<5) w2++;
		bit = ( GPIO_REG(GPIO_INPUT_VAL) >> SPI_TDI) & 1U;
		rx_data = ( bit==1 ? rx_data |  (1 << i) : rx_data & ~(1 << i) );

	}

	return rx_data;
}

#define CMD_DUMMY 0xFFFFFF
#define CMD_STOP  0x000000

static volatile uint32_t usb_state = 0;
static volatile uint32_t man_cmd = CMD_STOP;
static volatile int LED = LED_RED;

uint64_t task0(); // OWI Sequence
uint64_t task1(); // Manual cmd stop
uint64_t task2(); // Keep alive
uint64_t task3(); // LED off

static struct {
	uint64_t (*task)(void);
	uint64_t timecmp;
} timer[] = {{task0, UINT64_MAX}, {task1, UINT64_MAX}, {task2, UINT64_MAX}, {task3, UINT64_MAX}};

void timer_set(const int i, const uint64_t timecmp){

	timer[i].timecmp = timecmp;

	uint64_t timecmp_min = UINT64_MAX;
	for (int i=0; i<sizeof(timer)/sizeof(timer[0]); i++)
		timecmp_min = timer[i].timecmp < timecmp_min ? timer[i].timecmp : timecmp_min;

	ECALL_WRTIMECMP(timecmp_min);

}
void timer_handler(const uint64_t time){

	for (int i=0; i<sizeof(timer)/sizeof(timer[0]); i++)
		if(time >= timer[i].timecmp){
			timer_set(i, timer[i].task());
			break;
		}
}

uint64_t task0(){ // OWI sequence

	uint64_t timecmp = UINT64_MAX;

	if (usb_state==0x12670000){

		if (owi_sequence_next()!=-1){
			spi_rw(owi_sequence_get_cmd());
			timecmp = ECALL_RDTIME() + owi_sequence_get_ms()*RTC_FREQ/1000;
		}

	}

	return timecmp;

}
uint64_t task1(){ // Manual cmd stop
	spi_rw(man_cmd = CMD_STOP);
	return UINT64_MAX;
}
uint64_t task2(){ // Keep alive 1sec

	// Send keep alive packet and check ret value
	volatile uint32_t rx_data = spi_rw(CMD_DUMMY);

    // Update USB state (0xFFFFFFFF no spi/usb adapter)
    if (rx_data != usb_state){
    	if (rx_data==0x12670000){
    		LED = LED_GRN;
    		ECALL_SEND(1, "USB ID 0x12670000");
    	} else if (usb_state==0x12670000){
    		LED = LED_RED;
    		ECALL_SEND(1, "USB DISCONNECT");
    		owi_sequence_stop();
    	}
    	usb_state=rx_data;
    }

	const uint64_t time = ECALL_RDTIME();

    // Turn on LED & start LED timer
    GPIO_REG(GPIO_OUTPUT_VAL) |= (1<<LED);
    timer_set(3, time + LED_TIME);

	return time + KEEP_ALIVE_TIME;
}
uint64_t task3(){ // LED off
	GPIO_REG(GPIO_OUTPUT_VAL) &= ~(1<<LED);
	return UINT64_MAX;
}

__attribute__(( interrupt(), aligned(4) )) void trap_handler(void){

	switch(ECALL_CSRR(CSR_MCAUSE)){
		case 0 : break; // Instruction address misaligned
		case 1 : break; // Instruction access fault
		case 3 : break; // Breakpoint
		case 4 : break; // Load address misaligned
		case 5 : break; // Load access fault
		case 6 : break; // Store/AMO address misaligned
		case 7 : break; // Store access fault
		case 8 : break; // Environment call from U-mode

		case 0x80000007 : timer_handler(ECALL_RDTIME()); break; // Muliplexed timer

	}

}

void msg_handler(const char *msg){

	if (strcmp("ping", msg)==0){
		ECALL_SEND(1, "pong");

	} else if (usb_state==0x12670000 && man_cmd==CMD_STOP){

		if (strcmp("stop", msg)==0) owi_sequence_stop_req();

		else if (!owi_sequence_is_running()){

			     if (strcmp("start", msg)==0) {owi_sequence_start(MAIN);   timer_set(0, 0);}
			else if (strcmp("fold",  msg)==0) {owi_sequence_start(FOLD);   timer_set(0, 0);}
			else if (strcmp("unfold",msg)==0) {owi_sequence_start(UNFOLD); timer_set(0, 0);}

			// Manual single-command adjustments
				 if (strcmp("q", msg)==0) man_cmd = 0x000001; // grip close
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
				timer_set(1, ECALL_RDTIME() + MAN_CMD_TIME);
			}

		}

	}

}

int main (void){

	//volatile int w=0; while(1){w++;}
	//while(1) ECALL_YIELD();
	//while(1) ECALL_WFI();

	GPIO_REG(GPIO_INPUT_EN)  |= (1 << SPI_TDI);
	GPIO_REG(GPIO_PULLUP_EN) |= (1 << SPI_TDI);
	GPIO_REG(GPIO_OUTPUT_EN) |= ((1 << SPI_TCK) | (1<< SPI_TDO) | (1 << LED_RED) | (1 << LED_GRN));
    GPIO_REG(GPIO_DRIVE)     |= ((1 << SPI_TCK) | (1<< SPI_TDO)) ;

	CSRW(mtvec, trap_handler);  	// register trap handler
	CSRS(mie, 1<<7); 				// enable timer interrupts
    CSRS(mstatus, 1<<3);			// enable global interrupts

    // Start task2: Hartbeat LED, USB status, Keep alive pkt
    timer_set(2, 0);

	while(1){

<<<<<<< HEAD
		int msg[4]={0,0,0,0};
=======
		// Message handler
		char msg[16]; if (ECALL_RECV(1, msg)) msg_handler(msg);
>>>>>>> refs/heads/dev/2.0.2

<<<<<<< HEAD
		if (ECALL_RECV(1, msg)) {

			if (msg[0] && !msg[1] && usb_state==0x12670000 && cmd_timer==0){

				uint8_t cmd[3] = {0x00, 0x00, 0x00};

				switch (msg[0]){
					case 'q' : cmd[0] = 0x01; break; // grip close
					case 'a' : cmd[0] = 0x02; break; // grip open
					case 'w' : cmd[0] = 0x04; break; // wrist up
					case 's' : cmd[0] = 0x08; break; // wrist down
					case 'e' : cmd[0] = 0x10; break; // elbow up
					case 'd' : cmd[0] = 0x20; break; // elbow down
					case 'r' : cmd[0] = 0x40; break; // shoulder up
					case 'f' : cmd[0] = 0x80; break; // shoulder down
					case 't' : cmd[1] = 0x01; break; // base clockwise
					case 'g' : cmd[1] = 0x02; break; // base counterclockwise
					case 'y' : cmd[2] = 0x01; break; // light on
					default  : break;
				}

				if ( cmd[0] + cmd[1] + cmd[2] != 0 ){
					rx_data = spi_rw(cmd);
					cmd_timer = SYS_TIME + CMD_TIME;
					ping_timer = SYS_TIME + PING_TIME;
				}
			}

			// Ping Pong & Change LED color
			if (msg[0]=='p' && msg[1]=='i' && msg[2]=='n' && msg[3]=='g') ECALL_SEND(1, msg);
			else if (msg[0]=='r' && msg[1]=='e' && msg[2]=='d')                LED = LED_RED;
			else if (msg[0]=='g' && msg[1]=='r' && msg[2]=='e' && msg[3]=='e') LED = LED_GREEN;
			else if (msg[0]=='b' && msg[1]=='l' && msg[2]=='u' && msg[3]=='e') LED = LED_BLUE;

		}

		// auto stop manual commands after CMD_TIME
	    if (cmd_timer >0 && SYS_TIME > cmd_timer){
	    	rx_data = spi_rw(CMD_STOP);
	    	cmd_timer=0;
	    	ping_timer = SYS_TIME + PING_TIME;
	    }

	    // Detect USB state every 1sec
	    if (SYS_TIME > ping_timer){
	    	rx_data = spi_rw(CMD_DUMMY);
	    	ping_timer = SYS_TIME + PING_TIME;
	    }

	    // Update USB state (0xFFFFFFFF no spi/usb adapter)
	    if (rx_data != usb_state){

	    	if (rx_data==0x12670000 && usb_state==0x0){
	    		LED = LED_GREEN;
	    		ECALL_SEND(1, ((int[]){1,0,0,0}));
	    	} else if (rx_data==0x0 && usb_state==0x12670000){
	    		LED = LED_RED;
	    		ECALL_SEND(1, ((int[]){2,0,0,0}));
	    		owi_task_stop_request();
	    	}

	    	usb_state=rx_data;
	    }

		// OWI sequence
	    if (usb_state==0x12670000){

	    	switch (msg[0]){
				case '<' : owi_task_fold(); break;
				case '>' : owi_task_unfold(); break;
				case '1' : owi_task_start_request(); break;
				case '0' : owi_task_stop_request(); break;
	    	}

			int32_t cmd = owi_task_run(SYS_TIME);
			if ( cmd != -1){
				rx_data = spi_rw((uint8_t[]){(uint8_t)cmd, (uint8_t)(cmd>>8), (uint8_t)(cmd>>16)});
				ping_timer = SYS_TIME + PING_TIME;
			}

	    }

        // LED blink
	    if (SYS_TIME > led_timer){

	    	if ( GPIO_REG(GPIO_OUTPUT_VAL) & (LED_RED | LED_GREEN | LED_BLUE) ) {
	    		// ON => OFF
	        	GPIO_REG(GPIO_OUTPUT_VAL) &= ~(LED_RED | LED_GREEN | LED_BLUE);
	    		led_timer = SYS_TIME + LED_OFF_TIME;

	    	} else {
	    		// OFF => ON
	        	GPIO_REG(GPIO_OUTPUT_VAL) &= ~(LED_RED | LED_GREEN | LED_BLUE);
	    		GPIO_REG(GPIO_OUTPUT_VAL) |= LED;
	    		led_timer = SYS_TIME + LED_ON_TIME;
	    	}

	    }

	    // Yield to other zones
		ECALL_YIELD();
=======
	    ECALL_WFI();
>>>>>>> refs/heads/dev/2.0.2

	}

}



