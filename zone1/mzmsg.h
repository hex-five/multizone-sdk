/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#ifndef MZSG_H
#define MZSG_H

#include <stddef.h>

int mzmsg_read(char *buf, size_t len);
int mzmsg_write(char *buf, size_t len);

#endif /* MZSG_H */