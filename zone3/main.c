/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <string.h>	// strcmp()
#include <stdio.h> // sprintf()

#include "platform.h"
#include "multizone.h"
#include "owi_sequence.h"

#define SPI_TDI 11 	// in
#define SPI_TCK 10	// out (master)
#define SPI_TDO  9  // out
#define SPI_SYN  8  // out - not used

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

	return rx_data;
}

__attribute__((interrupt())) void trap_handler(void){

	switch(ECALL_CSRR(CSR_MCAUSE)){
		case 0 : break; // Instruction address misaligned
		case 1 : break; // Instruction access fault
		case 3 : break; // Breakpoint
		case 4 : break; // Load address misaligned
		case 5 : break; // Load access fault
		case 6 : break; // Store/AMO address misaligned
		case 7 : break; // Store access fault
		case 8 : break; // Environment call from U-mode

		case 0x80000007 : {

			int delay_ms = 1000;

			// OWI Robot sequence
			const int owi_seq = owi_sequence_next();
			const int32_t owi_cmd = owi_sequence_get_cmd();
			const int owi_ms = owi_sequence_get_ms();

			if (owi_seq!=-1){

				char str[16+1]="\0";
				sprintf(str, "%2d 0x%05x %4d", owi_seq, owi_cmd, owi_ms);
				ECALL_SEND(1, str);

				delay_ms = owi_ms;

			}

			ECALL_SETTIMECMP(+delay_ms*RTC_FREQ/1000);

		} break;

	}

}


int main (void){

	//volatile int w=0; while(1){w++;}
	//while(1) ECALL_YIELD();
	//while(1) ECALL_WFI();

	GPIO_REG(GPIO_INPUT_EN)  |= (0x1 << SPI_TDI);
	GPIO_REG(GPIO_PULLUP_EN) |= (0x1 << SPI_TDI);
	GPIO_REG(GPIO_OUTPUT_EN) |= ((0x1 << SPI_TCK) | (0x1<< SPI_TDO) | (0x1 << LED_RED) | (0x1 << LED_GREEN));
    GPIO_REG(GPIO_DRIVE)     |= ((0x1 << SPI_TCK) | (0x1<< SPI_TDO)) ;

	#define CMD_STOP  ((uint8_t[]){0x00, 0x00, 0x00})
	#define CMD_DUMMY ((uint8_t[]){0xFF, 0xFF, 0xFF})
	#define CMD_TIME  RTC_FREQ*250/1000 // 250ms
	#define PING_TIME RTC_FREQ // 1000ms
	#define LED_ON_TIME  RTC_FREQ*20/1000 //  50ms
	#define LED_OFF_TIME RTC_FREQ 		  // 950ms

    uint64_t time, cmd_timer=0, ping_timer=0, led_timer=0;
	uint32_t rx_data = 0, usb_state = 0;
	int LED = LED_RED;

	// Set timer 1 sec
	ECALL_SETTIMECMP(+1*RTC_FREQ);
	CSRW(mtvec, trap_handler);  // register trap handler
	CSRS(mie, 1<<7); 			// enable timer interrupts
    CSRS(mstatus, 1<<3);		// enable global interrupts

	while(1){

		// Message handler
		char msg[16]; if (ECALL_RECV(1, msg)) {
			if (strcmp("ping", msg)==0) ECALL_SEND(1, "pong");
			else if (strcmp("start", msg)==0) owi_sequence_start(MAIN);
			else if (strcmp("fold", msg)==0) owi_sequence_start(FOLD);
			else if (strcmp("unfold", msg)==0) owi_sequence_start(UNFOLD);
			else if (strcmp("stop", msg)==0) owi_sequence_stop();
		}

	    ECALL_WFI();

	}

}



