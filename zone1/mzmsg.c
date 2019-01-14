/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <mzmsg.h>

#define ACK    0
#define IND    1
#define CTL    2
#define DAT    3

#define CTL_ACK    (1 << 0)
#define CTL_DAT    (1 << 1)

int msg_out[4] = {-1,0,0,0};
int msg_in[4] = {0,-1,0,0};

static void mzmsg_update(){
    
    int msg[4] = {0,0,0,0};

    ECALL_RECV(2, (void*)msg);

    // if(msg[CTL] != 0)
    //     printf("Z1> RECV ind: %d; ack: %d\n", msg[IND], msg[ACK]);

    if(msg[CTL] & CTL_ACK){
        msg_in[ACK] = msg[ACK];
        msg_in[CTL] |= CTL_ACK;
    }

    if(msg[CTL] & CTL_DAT){
        msg_in[IND] = msg[IND];
        msg_in[DAT] = msg[DAT];
        msg_in[CTL] |= CTL_DAT;
    }

}

static void mzmsg_flush(){
    // printf("Z1> SEND ind: %d; ack: %d; dat: 0x%x\n", 
    //     msg_out[IND], msg_out[ACK], msg_out[DAT]);
    ECALL_SEND(2, (void*)msg_out);
}

int mzmsg_read(char *buf, size_t len){

    static int last_seen_ind = -1;
    size_t count = 0;

    while(count < len){

        if(msg_in[IND] == last_seen_ind)
            mzmsg_update();

        if(msg_in[IND] == last_seen_ind){
            msg_in[DAT] = 0;
            msg_in[CTL] &= ~CTL_DAT;
        }

        if((msg_in[CTL] & CTL_DAT) != 0){
            *buf++ = (char)msg_in[DAT];
            count++;
            last_seen_ind = msg_in[IND];
            msg_out[ACK] = msg_in[IND];
            msg_out[CTL] |= CTL_ACK;

            mzmsg_flush();
        } else {
            break;
        }
    }

    return count;
}

int mzmsg_write(char *buf, size_t len){

    static int ack_pending = 0;
    static int ack_index = 0;
    size_t count = 0;

    while(count < len){

        if(ack_pending){
            if(!(msg_in[CTL] & CTL_ACK) || (msg_in[ACK] != ack_index))
                mzmsg_update();

            if((msg_in[CTL] & CTL_ACK) && (msg_in[ACK] == ack_index)){
                ack_pending = 0;
                ack_index++;
            } else {
               ECALL_YIELD();
               //mzmsg_flush();
            }
        }

        if(!ack_pending){
            msg_out[CTL] |= CTL_DAT;
            msg_out[DAT] = *buf++;
            msg_out[IND] = ack_index;
            count++;

            ack_pending = 1;
            
            mzmsg_flush();
        } 

        for(int i = 0; i < 9999; i++);
    }

    return count;
}