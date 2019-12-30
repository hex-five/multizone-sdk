/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#ifndef OWI_SEQUENCE_H_
#define OWI_SEQUENCE_H_

#include <stdint.h>

typedef enum{
	MAIN,
	FOLD,
	UNFOLD,
} owi_sequence;

void owi_sequence_start(owi_sequence);
void owi_sequence_stop();
int owi_sequence_next();

int32_t owi_sequence_get_cmd();
int owi_sequence_get_ms();

#endif
