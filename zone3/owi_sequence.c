/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <stddef.h> // NULL
#include "owi_sequence.h"

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

// R4 09-MAR-2019
#define T_STOP 	1000
#define T_GRIP 	1400
#define T_WRIST 2600
#define T_ARM  	1800
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
	{ .command = BASE_COUNTERCLOCK, .duration_ms = T_BASE+T_ARM-250},
	{ .command = ARM_UP, 			.duration_ms = T_ARM  },
    { .command = SHOULDER_UP, 		.duration_ms = +175 },
    { .command = ELBOW_UP, 			.duration_ms = +100 },
	{ .command = WRIST_DOWN, 		.duration_ms = T_WRIST-250},
	{ .command = GRIP_OPEN, 		.duration_ms = T_GRIP},
};

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

static struct sequence_step *sequence = NULL;
static int stepIdx = 0;
static int size = 0;
static int stop = 1;

void owi_sequence_start(owi_sequence seq){

	if (sequence==NULL){

		switch(seq){

		case MAIN:
			sequence = main_sequence;
			size = sizeof(main_sequence) / sizeof(main_sequence[0]);
			stepIdx = 0;
			stop = 0;
			break;

		case FOLD:
			sequence = fold_sequence;
			size = sizeof(fold_sequence) / sizeof(fold_sequence[0]);
			stepIdx = 0;
			stop = 0;
			break;

		case UNFOLD:
			sequence = unfold_sequence;
			size = sizeof(unfold_sequence) / sizeof(unfold_sequence[0]);
			stepIdx = 0;
			stop = 0;
			break;

		}

	}

};

void owi_sequence_stop_req(){stop=1;}

void owi_sequence_stop(){
	sequence = NULL;
	stepIdx = 0;
	size = 0;
	stop = 1;
}

int owi_sequence_next(){

	if (stop && stepIdx==0) sequence=NULL;

	if (sequence==NULL) return -1;

	stepIdx = (stepIdx+1) % size;

	// Auto stop FOLD/UNFOLD
	if (sequence[stepIdx].duration_ms==0) stop=1;

	return stepIdx;
};

int32_t owi_sequence_get_cmd(){

	if (sequence==NULL) return 0;

	const int32_t cmd = sequence[stepIdx].command;

	return stop ? cmd & ~(1UL<<16) : cmd | (1UL<<16);

}
int owi_sequence_get_ms(){

	if (sequence==NULL) return 0;

	return sequence[stepIdx].duration_ms;
}

int owi_sequence_is_running(){

	return sequence!=NULL;

};
