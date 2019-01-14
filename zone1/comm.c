/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <comm.h>

#define ACK    0
#define IND    1
#define CTL    2
#define DAT    3

#define CTL_ACK    (1 << 0)
#define CTL_DAT    (1 << 1)

#define EVT_WR  (1 << 0)
#define EVT_RD  (1 << 1)

void mz_channel_init(struct mz_channel *chan, int zone){

    if(chan != NULL){
        chan->zone = zone;

        chan->in_beg = 0;
        chan->in_end = 0;
        chan->out_beg = 0;
        chan->out_end = 0;

        chan->out_mutex = xSemaphoreCreateMutex();
        chan->in_mutex = xSemaphoreCreateMutex();
        chan->events = xEventGroupCreate();
    }

}

int mz_channel_write(struct mz_channel *chan, char *buf, int n){

    int count = 0;

    if(chan == NULL || buf == NULL || n < 0)
        return -1;

    xSemaphoreTake(chan->out_mutex, portMAX_DELAY);

    while(count != n){
        if(chan->out_beg != ((chan->out_end + 1) & (CHANNEL_SIZE-1))){
            chan->out[chan->out_end] = *buf;
            chan->out_end = (chan->out_end + 1) & (CHANNEL_SIZE-1);
            buf++;
            count++;
        } else {
            xEventGroupClearBits(chan->events, EVT_WR);
            xEventGroupWaitBits(chan->events, EVT_WR, pdFALSE, pdFALSE, portMAX_DELAY );
        }
    }

    xSemaphoreGive(chan->out_mutex);

    return count;
}

int mz_channel_read(struct mz_channel *chan, char *buf, int n){

    int count = 0;

    if(chan == NULL || buf == NULL || n < 0)
        return -1;

    xSemaphoreTake(chan->in_mutex, portMAX_DELAY);

    while(count != n){
        if(chan->in_beg != chan->in_end) {
            *buf = chan->in[chan->in_beg];
            chan->in_beg = (chan->in_beg + 1) & (CHANNEL_SIZE-1);
            buf++;
            count++;
        } else {
             xEventGroupClearBits(chan->events, EVT_RD);
             xEventGroupWaitBits(chan->events, EVT_RD, pdFALSE, pdFALSE, portMAX_DELAY );
        }
    }

    xSemaphoreGive(chan->in_mutex);

    return count;
}

void mz_channel_update(struct mz_channel *chan){

    static int ack_pending = 0;
    static int ack_index = 0;
    static int flush = 0;
    static int msg_out[4] = {-1,0,0,0};

    int msg[4] = {0,0,0,0};

    ECALL_RECV(chan->zone, (void*)msg);

    if((msg[CTL] & CTL_DAT) != 0){
        if(msg[IND] == (msg_out[ACK] + 1)){
            chan->in[chan->in_end] = msg[DAT];
            chan->in_end = (chan->in_end + 1) & (CHANNEL_SIZE-1);

            msg_out[CTL] |= CTL_ACK;
            msg_out[ACK] = msg[IND];
            flush = 1;

            xEventGroupSetBits(chan->events, EVT_RD);
        }
    }

    if((msg[CTL] & CTL_ACK != 0) & ack_pending){
        if(msg[ACK] == ack_index){
            chan->out_beg = (chan->out_beg + 1) & (CHANNEL_SIZE-1);
            ack_index++;

            xEventGroupSetBits(chan->events, EVT_WR);
        }

        ack_pending = 0;
    }

    if(!ack_pending){
        if(chan->out_beg != chan->out_end){
            msg_out[CTL] |= CTL_DAT;
            msg_out[IND] = ack_index;
            msg_out[DAT] = chan->out[chan->out_beg];
            flush = 1;
            ack_pending = 1;
        }
    }

    if(flush != 0){
        flush = 0;
        ECALL_SEND(chan->zone, (void*)msg_out);
    }

}
