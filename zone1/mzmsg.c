/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <mzmsg.h>
#include <string.h>

#define ACK    0
#define IND    1
#define CTL    2
#define DAT    3

#define CTL_ACK    (1 << 0)
#define CTL_DAT    (1 << 1)
#define CTL_RST    (1 << 2)
#define CTL_PSH    (1 << 3)

void mzmsg_reset(mzmsg_t *mzmsg){

    int msg[4] = {0,0,CTL_RST,0};

    mzmsg->ack_pending = 0;
    mzmsg->ack_index = 0;
    mzmsg->last_index = -1;

    ECALL_SEND(mzmsg->zone, msg);
    do {
        ECALL_YIELD();
        ECALL_RECV(mzmsg->zone, msg);
    } while(!(msg[CTL] &= CTL_RST));
}

void mzmsg_init(mzmsg_t *mzmsg, int zone){
    mzmsg->zone = zone;
    memcpy(mzmsg->out, (int[]){-1,0,0,0}, 4*sizeof(int));
    memcpy(mzmsg->in, (int[]){0,-1,0,0}, 4*sizeof(int));
    mzmsg->ack_pending = -1;
    mzmsg->ack_index = 0;
    mzmsg->last_index = -1;
}

static void mzmsg_update(mzmsg_t *mzmsg){
    
    int msg[4] = {0,0,0,0};

    ECALL_RECV(mzmsg->zone, (void*)msg);

    if(msg[CTL] & CTL_RST){
        mzmsg->ack_pending = 0;
        mzmsg->ack_index = 0;
        mzmsg->last_index = -1;
        memcpy(mzmsg->out, (int[]){-1,0,0,0}, 4*sizeof(int));
        ECALL_SEND(1, (int[]){0,0,CTL_RST,0});
        return;
    }

    if(msg[CTL] & CTL_ACK){
        mzmsg->in[ACK] = msg[ACK];
        mzmsg->in[CTL] |= CTL_ACK;
    }

    if(msg[CTL] & CTL_DAT){
        mzmsg->in[IND] = msg[IND];
        mzmsg->in[DAT] = msg[DAT];
        mzmsg->in[CTL] |= CTL_DAT;
    }

}

static void mzmsg_flush(mzmsg_t *mzmsg){
    ECALL_SEND(mzmsg->zone, (void*)mzmsg->out);
}

int mzmsg_read(mzmsg_t *mzmsg, char *buf, size_t len){

    size_t count = 0;

    while(count < len){

        if(mzmsg->in[IND] == mzmsg->last_index)
            mzmsg_update(mzmsg);

        if(mzmsg->in[IND] == mzmsg->last_index){
            mzmsg->in[DAT] = 0;
            mzmsg->in[CTL] &= ~CTL_DAT;
        }

        if((mzmsg->in[CTL] & CTL_DAT) != 0){
            memcpy(buf, &mzmsg->in[DAT], 1);
            buf++;
            count++;
            mzmsg->last_index = mzmsg->in[IND];
            mzmsg->out[ACK] = mzmsg->in[IND];
            mzmsg->out[CTL] |= CTL_ACK;

            mzmsg_flush(mzmsg);
        } else {
            break;
        }
    }

    return count;
}

int mzmsg_write(mzmsg_t *mzmsg, char *buf, size_t len){

    static int ack_pending = 0;
    static int ack_index = 0;
    size_t count = 0;

    while(count < len){

        if(ack_pending){
            if(!(mzmsg->in[CTL] & CTL_ACK) || (mzmsg->in[ACK] != ack_index))
                mzmsg_update(mzmsg);

            if((mzmsg->in[CTL] & CTL_ACK) && (mzmsg->in[ACK] == ack_index)){
                ack_pending = 0;
                ack_index++;
            } else {
               ECALL_YIELD();
            }
        }

        if(!ack_pending){
            int written;
            mzmsg->out[CTL] |= CTL_DAT;
            memset(&mzmsg->out[DAT], 0, 4);
            if (len - count > 4) {
                written = 4;
            } else {
                written = len - count;
            }
            memcpy(&mzmsg->out[DAT], buf, written);
            mzmsg->out[IND] = ack_index;
            count += written;
            buf += written;
            if (count == len)
                mzmsg->out[CTL] |= CTL_PSH;

            ack_pending = 1;
            
            mzmsg_flush(mzmsg);

            mzmsg->out[CTL] &= ~CTL_PSH;
        } 
        
    }

    return count;
}
