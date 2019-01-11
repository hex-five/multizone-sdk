/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#ifndef OWI_TASK_H_
#define OWI_TASK_H_

#include <stdint.h>

void owi_task_start_request(void);
void owi_task_stop_request(void);
void owi_task_fold(void);
void owi_task_unfold(void);
int32_t owi_task_run(const uint64_t time);

#endif
