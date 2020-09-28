/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

/*
* Hardware setup / OEM low-level boot code
* Link to HEX format and pass to the multizone.jar via --boot option
* Runs in privileged machine mode "M" (after policy integrity check)
* Return execution to MultiZone via trap 0xb (ecall from M mode)
 */
int main (void){

	// Hardware setup code goes here
	asm ("nop;");

	// Return execution to MultiZone
	asm ("ecall;");

}
