/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#ifndef _COMM_H_
#define _COMM_H_

#include <FreeRTOS.h>
#include <semphr.h>
#include <event_groups.h>
#include <libhexfive.h>

/** CHANNEL_SIZE MUST BE POWER OF TWO */
#define CHANNEL_SIZE    128

struct mz_channel {

    int zone;

    char in[CHANNEL_SIZE];
    int in_beg, in_end;
    SemaphoreHandle_t in_mutex;

    char out[CHANNEL_SIZE];
    int out_beg, out_end;
    SemaphoreHandle_t out_mutex;

    EventGroupHandle_t events;
};

void mz_channel_init(struct mz_channel *chan, int zone);

int mz_channel_write(struct mz_channel *chan, char *buf, int n);
int mz_channel_read(struct mz_channel *chan, char *buf, int n);

void mz_channel_update(struct mz_channel *chan);

#endif /* _COMM_H_ */