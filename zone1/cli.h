/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#ifndef CLI_H
#define CLI_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h> // round()

#include <platform.h>
#include <libhexfive.h>

void cliTask( void *pvParameters);
void cli_exceptions(unsigned long mcause, unsigned long mtval, unsigned long mepc);

#endif /* CLI_H */