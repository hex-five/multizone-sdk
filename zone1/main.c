/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <platform.h>
#include <libhexfive.h>

int main (void){

	//volatile int w=0; while(1){w++;}
	//while(1) ECALL_YIELD();

	while(1){

		int msg[4]={0,0,0,0};

		ECALL_RECV(4, msg);

		if (msg[0]) ECALL_SEND(4, msg);

		ECALL_YIELD();

	}

} // main
