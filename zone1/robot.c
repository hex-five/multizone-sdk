/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <robot.h>

#define SPI_TDI 11 	// in
#define SPI_TCK 10	// out (master)
#define SPI_TDO  9  // out
#define SPI_SYN  8  // out - not used

#define LED_RED   1
#define LED_GREEN 2

QueueHandle_t robot_queue;

typedef enum{
	START_REQUEST,
	STARTED,
	STOP_REQUEST,
	STOPPED,
	FOLD,
	UNFOLD
} state_enum;

typedef enum{
	STOP 				= 0x000000,
	GRIP_CLOSE			= 0x000001,
	GRIP_OPEN 			= 0x000002,
	WRIST_UP 			= 0x000004,
	WRIST_DOWN 			= 0x000008,
	ELBOW_UP   			= 0x000010,
	ELBOW_DOWN			= 0x000020,
	SHOULDER_UP   		= 0x000040,
	SHOULDER_DOWN		= 0x000080,
	BASE_CLOCKWISE 	  	= 0x000100,
	BASE_COUNTERCLOCK 	= 0x000200,
	LIGHT_ON  			= 0x010000,
	ARM_UP    = 0x000008 | 0x000010 | 0x000040, 		   // wrist down + elbow up   + shoulder up
	ARM_DOWN  = 0x000004 | 0x000020 | 0x000080 | 0x000100, // wrist up   + elbow down + shoulder down + base clockwise
} cmd;

struct sequence_step{
	uint32_t command;
	int duration_ms;
};

// R5 02-DEC-2018
#define T_STOP 	1000
#define T_GRIP 	1400
#define T_WRIST 2500
#define T_ARM  	1600
#define T_BASE	3000
static struct sequence_step main_sequence[] = {
	{ .command = STOP, 				.duration_ms = T_STOP },
	{ .command = GRIP_CLOSE, 		.duration_ms = T_GRIP },
	{ .command = WRIST_UP, 			.duration_ms = T_WRIST},
	{ .command = ARM_DOWN, 			.duration_ms = T_ARM  },
	{ .command = BASE_CLOCKWISE,	.duration_ms = T_BASE },
	{ .command = GRIP_OPEN, 		.duration_ms = T_GRIP },
	{ .command = STOP, 				.duration_ms = T_STOP },
	{ .command = GRIP_CLOSE, 		.duration_ms = T_GRIP },
	{ .command = BASE_COUNTERCLOCK, .duration_ms = T_BASE+T_ARM -200},
	{ .command = ARM_UP, 			.duration_ms = T_ARM  },
    { .command = SHOULDER_UP, 		.duration_ms =   +175 },
    { .command = ELBOW_UP, 			.duration_ms =   +120 },
	{ .command = WRIST_DOWN, 		.duration_ms = T_WRIST -260},
	{ .command = GRIP_OPEN, 		.duration_ms = T_GRIP  -200},
};

/*// R4 01-DEC-2018
#define T_STOP 	1000
#define T_GRIP 	1400
#define T_WRIST 2500
#define T_ARM  	1600
#define T_BASE	3000
static struct sequence_step main_sequence[] = {
	{ .command = STOP, 				.duration_ms = T_STOP },
	{ .command = GRIP_CLOSE, 		.duration_ms = T_GRIP },
	{ .command = WRIST_UP, 			.duration_ms = T_WRIST},
	{ .command = ARM_DOWN, 			.duration_ms = T_ARM  },
	{ .command = BASE_CLOCKWISE,	.duration_ms = T_BASE },
	{ .command = GRIP_OPEN, 		.duration_ms = T_GRIP },
	{ .command = STOP, 				.duration_ms = T_STOP },
	{ .command = GRIP_CLOSE, 		.duration_ms = T_GRIP },
	{ .command = BASE_COUNTERCLOCK, .duration_ms = T_BASE+T_ARM -330},
	{ .command = ARM_UP, 			.duration_ms = T_ARM  },
    { .command = SHOULDER_UP, 		.duration_ms =   +175 },
    { .command = ELBOW_UP, 			.duration_ms =   +100 },
	{ .command = WRIST_DOWN, 		.duration_ms = T_WRIST -200},
	{ .command = GRIP_OPEN, 		.duration_ms = T_GRIP  -200},
};*/

/* // R3 28-NOV-2018
#define T_STOP 	 500
#define T_GRIP 	1500
#define T_WRIST 2500
#define T_ARM  	2000-200
#define T_BASE	3000
static struct sequence_step main_sequence[] = {
	{ .command = STOP, 				.duration_ms = T_STOP },
	{ .command = GRIP_CLOSE, 		.duration_ms = T_GRIP },
	{ .command = WRIST_UP, 			.duration_ms = T_WRIST},
	{ .command = ARM_DOWN, 			.duration_ms = T_ARM  },
	{ .command = BASE_CLOCKWISE,	.duration_ms = T_BASE },
	{ .command = GRIP_OPEN, 		.duration_ms = T_GRIP },
	{ .command = STOP, 				.duration_ms = T_STOP },
	{ .command = GRIP_CLOSE, 		.duration_ms = T_GRIP },
	{ .command = BASE_COUNTERCLOCK, .duration_ms = T_BASE+T_ARM -150},
	{ .command = ARM_UP, 			.duration_ms = T_ARM  },
    { .command = SHOULDER_UP, 		.duration_ms =   +200 },
    { .command = ELBOW_UP, 			.duration_ms =   +100 },
	{ .command = WRIST_DOWN, 		.duration_ms = T_WRIST -500},
	{ .command = GRIP_OPEN, 		.duration_ms = T_GRIP  -200},
}; */

/* // R1 27-NOV-2018
#define T_STOP 	 500
#define T_GRIP 	1500
#define T_WRIST 1700
#define T_ARM  	2000
#define T_BASE	3000
static struct sequence_step main_sequence[] = {
	{ .command = STOP, 				.duration_ms = T_STOP },
	{ .command = GRIP_CLOSE, 		.duration_ms = T_GRIP },
	{ .command = WRIST_UP, 			.duration_ms = T_WRIST},
	{ .command = ARM_DOWN, 			.duration_ms = T_ARM  },
	{ .command = BASE_CLOCKWISE,	.duration_ms = T_BASE },
	{ .command = GRIP_OPEN, 		.duration_ms = T_GRIP },
	{ .command = STOP, 				.duration_ms = T_STOP },
	{ .command = GRIP_CLOSE, 		.duration_ms = T_GRIP },
	{ .command = BASE_COUNTERCLOCK, .duration_ms = T_BASE+T_ARM -675 }, // -700 -825 -750
	{ .command = ARM_UP, 			.duration_ms = T_ARM  },
    { .command = SHOULDER_UP, 		.duration_ms =   +140 }, // +250
    { .command = ELBOW_UP, 			.duration_ms =    +50 }, // +75 +50 +100
	{ .command = WRIST_DOWN, 		.duration_ms = T_WRIST -200}, // -150
	{ .command = GRIP_OPEN, 		.duration_ms = T_GRIP },
};*/

#define T_FOLD 4500
#define T_FOLD_SHOULDER 800
static struct sequence_step fold_sequence[] = {
	{ .command = STOP, .duration_ms = 0},
	{ .command = GRIP_OPEN, .duration_ms = 100},
	{ .command = ELBOW_DOWN | SHOULDER_UP | WRIST_DOWN, .duration_ms = T_FOLD},
	{ .command = ELBOW_DOWN, .duration_ms = 200},
	{ .command = SHOULDER_UP, .duration_ms = T_FOLD_SHOULDER},
};
static struct sequence_step unfold_sequence[] = {
	{ .command = STOP, .duration_ms = 0},
	{ .command = ELBOW_UP, .duration_ms = 200},
	{ .command = SHOULDER_DOWN, .duration_ms = T_FOLD_SHOULDER + 50},
	{ .command = ELBOW_UP | SHOULDER_DOWN | WRIST_UP, .duration_ms = T_FOLD},
};

static state_enum state = STOPPED;
static int step = -1;
static uint64_t timer = 0;

void owi_task_start_request(void){if (state==STOPPED) state=START_REQUEST;}
void owi_task_stop_request(void){if (state==STARTED) state=STOP_REQUEST;}
void owi_task_fold(void){if (state==STOPPED) state=FOLD; step=0; timer=0;}
void owi_task_unfold(void){if (state==STOPPED) state=UNFOLD; step=0; timer=0;}

int32_t owi_task_run(const uint64_t time){

	int32_t cmd = -1;

	switch(state){

		case START_REQUEST :
			step = 0; timer = 0; state=STARTED;
		    break;

		case (STARTED) :
		case (STOP_REQUEST) :
			if (time > timer){
				step = (step+1) % (sizeof(main_sequence)/sizeof(main_sequence[0]));
				timer = time + main_sequence[step].duration_ms * RTC_FREQ/1000;
				cmd = main_sequence[step].command;
				cmd = (state==STOP_REQUEST ? cmd & ~(1UL<<16) : cmd | (1UL<<16));
			}
			if (state==STOP_REQUEST && step==0)	state = STOPPED;
			break;

		case (FOLD) :
			if (time > timer){
				step = (step+1) % (sizeof(fold_sequence)/sizeof(fold_sequence[0]));
				timer = time + fold_sequence[step].duration_ms * RTC_FREQ/1000;
				cmd = fold_sequence[step].command;
				if (step==0) state = STOPPED;
			} break;

		case (UNFOLD) :
			if (time > timer){
				step = (step+1) % (sizeof(unfold_sequence)/sizeof(unfold_sequence[0]));
				timer = time + unfold_sequence[step].duration_ms * RTC_FREQ/1000;
				cmd = unfold_sequence[step].command;
				if (step==0) state = STOPPED;
			} break;

		case STOPPED :
			break;

		default: break;

	}

	return cmd;

}

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
	vTaskDelay(60/portTICK_PERIOD_MS);
	GPIO_REG(GPIO_OUTPUT_VAL) ^=  (0x1 << (rx_data==0x12670000 ? LED_GREEN : LED_RED));

	return rx_data;
}

void robotTask( void *pvParameters )
{   


	GPIO_REG(GPIO_INPUT_EN)  |= (0x1 << SPI_TDI);
	GPIO_REG(GPIO_PULLUP_EN) |= (0x1 << SPI_TDI);
	GPIO_REG(GPIO_OUTPUT_EN) |= ((0x1 << SPI_TCK) | (0x1<< SPI_TDO) | (0x1 << LED_RED) | (0x1 << LED_GREEN));
	GPIO_REG(GPIO_DRIVE)     |= ((0x1 << SPI_TCK) | (0x1<< SPI_TDO)) ;

	#define CMD_STOP  ((uint8_t[]){0x00, 0x00, 0x00})
	#define CMD_DUMMY ((uint8_t[]){0xFF, 0xFF, 0xFF})
	#define CMD_TIME  RTC_FREQ*250/1000 // 250ms
	#define PING_TIME RTC_FREQ // 1000ms
	#define SYS_TIME  ECALL_CSRR_MTIME() //RTC_REG(RTC_MTIME)

	uint64_t cmd_timer=0, ping_timer=0;
	uint32_t rx_data = 0, usb_state = 0;

	while(1){

		//int msg[4]={0,0,0,0}; ECALL_RECV(4, msg);
		//if (msg[0]>1 && usb_state==0x12670000 && cmd_timer==0){
        char c = 0;

        if(xQueueReceive(robot_queue, &c, 0) == pdFALSE)
            c = 0;
            
        if( c > 1 && usb_state==0x12670000 && cmd_timer==0){

			uint8_t cmd[3] = {0x00, 0x00, 0x00};

			switch (c){
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
		if (c =='<' && usb_state==0x12670000) owi_task_fold();
		if (c =='>' && usb_state==0x12670000) owi_task_unfold();
		if (c =='1' && usb_state==0x12670000) owi_task_start_request();
		if (c =='0' || usb_state!=0x12670000) owi_task_stop_request();
		int32_t cmd;
		if ( usb_state==0x12670000 && (cmd = owi_task_run(SYS_TIME)) != -1){
			rx_data = spi_rw((uint8_t[]){(uint8_t)cmd, (uint8_t)(cmd>>8), (uint8_t)(cmd>>16)});
			ping_timer = SYS_TIME + PING_TIME;
		}
        
       taskYIELD();
	}
}
