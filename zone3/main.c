/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <platform.h>
#include <libhexfive.h>

int main (void){

	//volatile int w=0; while(1){w++;}
	//while(1) ECALL_YIELD();

	while(1){
		ECALL_YIELD();
	}

} // main




