/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#ifndef MZMSG_H
#define MZMSG_H

#include <stddef.h>

typedef struct {
    int zone;
    int out[4];
    int in[4];
    int ack_pending;
    int ack_index;
    int last_index;
} mzmsg_t;

void mzmsg_init(mzmsg_t *mzmsg, int zone);
void mzmsg_reset(mzmsg_t *mzmsg);
int mzmsg_read(mzmsg_t *mzmsg, char *buf, size_t len);
int mzmsg_write(mzmsg_t *mzmsg, char *buf, size_t len);

#endif /* MZMSG_H */