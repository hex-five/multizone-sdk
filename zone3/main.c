/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <stdint.h>
#include <stdbool.h>

#include <platform.h>
#include <libhexfive.h>
#include "owi_task.h"

#define SPI_TDI 11 	// in
#define SPI_TCK 10	// out (master)
#define SPI_TDO  9  // out
#define SPI_SYN  8  // out - not used

#define LED_RED   1
#define LED_GREEN 2

uint8_t CRC8(uint8_t bytes[]){

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

uint32_t spi_rw(uint8_t cmd[]){

	uint32_t rx_data = 0;

	const uint32_t tx_data = ((uint8_t)cmd[0] << 24) |  ((uint8_t)cmd[1] << 16) | ((uint8_t)cmd[2] << 8) | CRC8(cmd);

	for (int i=32-1, bit; i>=0; i--){

		bit = (tx_data >> i) & 1U;
		GPIO_REG(GPIO_OUTPUT_VAL) = (bit==1 ? GPIO_REG(GPIO_OUTPUT_VAL) | (0x1 << SPI_TDO) :
											  GPIO_REG(GPIO_OUTPUT_VAL) & ~(0x1 << SPI_TDO)  );

		GPIO_REG(GPIO_OUTPUT_VAL) |= (0x1 << SPI_TCK); volatile int w1=0; while(w1<5) w1++;
		GPIO_REG(GPIO_OUTPUT_VAL) ^= (0x1 << SPI_TCK); volatile int w2=0; while(w2<5) w2++;
		bit = ( GPIO_REG(GPIO_INPUT_VAL) >> SPI_TDI) & 1U;
		rx_data = ( bit==1 ? rx_data |  (0x1 << i) : rx_data & ~(0x1 << i) );

	}

	GPIO_REG(GPIO_OUTPUT_VAL) |=  (0x1 << (rx_data==0x12670000 ? LED_GREEN : LED_RED));
	volatile int w3=0; while(w3<25000) w3++;
	GPIO_REG(GPIO_OUTPUT_VAL) ^=  (0x1 << (rx_data==0x12670000 ? LED_GREEN : LED_RED));

	return rx_data;
}

int main (void){

	//volatile int w=0; while(1){w++;}
	//while(1) ECALL_YIELD();

	GPIO_REG(GPIO_INPUT_EN)  |= (0x1 << SPI_TDI);
	GPIO_REG(GPIO_PULLUP_EN) |= (0x1 << SPI_TDI);
	GPIO_REG(GPIO_OUTPUT_EN) |= ((0x1 << SPI_TCK) | (0x1<< SPI_TDO) | (0x1 << LED_RED) | (0x1 << LED_GREEN));
    GPIO_REG(GPIO_DRIVE)     |= ((0x1 << SPI_TCK) | (0x1<< SPI_TDO)) ;

	#define CMD_STOP  ((uint8_t[]){0x00, 0x00, 0x00})
	#define CMD_DUMMY ((uint8_t[]){0xFF, 0xFF, 0xFF})
	#define CMD_TIME  RTC_FREQ*250/1000 // 250ms
	#define PING_TIME RTC_FREQ // 1000ms
	#define SYS_TIME  RTC_REG(RTC_MTIME)

	uint64_t cmd_timer=0, ping_timer=0;
	uint32_t rx_data = 0, usb_state = 0;

	while(1){

		int msg[4]={0,0,0,0}; ECALL_RECV(4, msg);

		if (msg[0]>1 && usb_state==0x12670000 && cmd_timer==0){

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

		// auto stop manual commands after CMD_TIME
	    if (cmd_timer >0 && SYS_TIME > cmd_timer){
	    	rx_data = spi_rw(CMD_STOP);
	    	cmd_timer=0;
	    	ping_timer = SYS_TIME + PING_TIME;
	    }

	    // detect USB state every 1s
	    if (SYS_TIME > ping_timer){
	    	rx_data = spi_rw(CMD_DUMMY);
	    	ping_timer = SYS_TIME + PING_TIME;
	    }

	    // update USB state & trigger event for Zone1
	    if (rx_data != usb_state){
	    	if (rx_data==0x12670000)
	    		ECALL_SEND(4, ((int[]){1,0,0,0}));
	    	else if (rx_data==0x00000000){
	    		ECALL_SEND(4, ((int[]){2,0,0,0}));
	    		owi_task_stop_request();
	    	}
	    	usb_state=rx_data;
	    }

		// OWI sequence
	    if (msg[0]=='<' && usb_state==0x12670000) owi_task_fold();
	    if (msg[0]=='>' && usb_state==0x12670000) owi_task_unfold();
		if (msg[0]=='1' && usb_state==0x12670000) owi_task_start_request();
		if (msg[0]=='0' || usb_state!=0x12670000) owi_task_stop_request();
		int32_t cmd;
		if ( usb_state==0x12670000 && (cmd = owi_task_run(SYS_TIME)) != -1){
			rx_data = spi_rw((uint8_t[]){(uint8_t)cmd, (uint8_t)(cmd>>8), (uint8_t)(cmd>>16)});
			ping_timer = SYS_TIME + PING_TIME;
		}

		ECALL_YIELD();

	}

} // main




