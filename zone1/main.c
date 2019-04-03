/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h> // round()

#include <platform.h>
#include <libhexfive.h>

#define CMD_LINE_SIZE 32
#define MSG_SIZE 4

void trap_0x0_handler(void)__attribute__((interrupt("user")));
void trap_0x0_handler(void){

	int msg[MSG_SIZE]={0,0,0,0};
	ECALL_RECV(1, msg);
	printf("Instruction address misaligned : 0x%08x 0x%08x 0x%08x \n", msg[0], msg[1], msg[2]);

	printf("\nPress any key to restart");
	char c='\0'; while(read(0, &c, 1) ==0 ){;} asm ("j _start");

}

void trap_0x1_handler(void)__attribute__((interrupt("user")));
void trap_0x1_handler(void){

	int msg[MSG_SIZE]={0,0,0,0};
	ECALL_RECV(1, msg);
	printf("Instruction access fault : 0x%08x 0x%08x 0x%08x \n", msg[0], msg[1], msg[2]);
	
	printf("\nPress any key to restart");
	char c='\0'; while(read(0, &c, 1) ==0 ){;} asm ("j _start");

}

void trap_0x2_handler(void)__attribute__((interrupt("user")));
void trap_0x2_handler(void){

	int msg[MSG_SIZE]={0,0,0,0};
	ECALL_RECV(1, msg);
	printf("Illegal instruction : 0x%08x 0x%08x 0x%08x \n", msg[0], msg[1], msg[2]);

}

void trap_0x3_handler(void)__attribute__((interrupt("user")));
void trap_0x3_handler(void){

	const uint64_t T = ECALL_CSRR_MTIME();

	printf("\e7"); 		// save curs pos
	printf("\e[r"); 	// scroll all screen
	printf("\e[2M"); 	// scroll up up
	printf("\e8");   	// restore curs pos
	printf("\e[2A"); 	// curs up 2 lines
	printf("\e[2L"); 	// insert 2 lines

	printf("\rZ1 > timer expired : %lu", (unsigned long)(T*1000/RTC_FREQ));

	printf("\e8");   	// restore curs pos

}

void trap_0x4_handler(void)__attribute__((interrupt("user")));
void trap_0x4_handler(void){

	int msg[MSG_SIZE]={0,0,0,0};
	ECALL_RECV(1, msg);
	printf("Load address misaligned : 0x%08x 0x%08x 0x%08x \n", msg[0], msg[1], msg[2]);

}

void trap_0x5_handler(void)__attribute__((interrupt("user")));
void trap_0x5_handler(void){

	int msg[MSG_SIZE]={0,0,0,0};
	ECALL_RECV(1, msg);
	printf("Load access fault : 0x%08x 0x%08x 0x%08x \n", msg[0], msg[1], msg[2]);

}

void trap_0x6_handler(void)__attribute__((interrupt("user")));
void trap_0x6_handler(void){

	int msg[MSG_SIZE]={0,0,0,0};
	ECALL_RECV(1, msg);
	printf("Store/AMO address misaligned : 0x%08x 0x%08x 0x%08x \n", msg[0], msg[1], msg[2]);

}

void trap_0x7_handler(void)__attribute__((interrupt("user")));
void trap_0x7_handler(void){

	int msg[MSG_SIZE]={0,0,0,0};
	ECALL_RECV(1, msg);
	printf("Store access fault : 0x%08x 0x%08x 0x%08x \n", msg[0], msg[1], msg[2]);

}

// ------------------------------------------------------------------------
void print_cpu_info(void) {
// ------------------------------------------------------------------------

	// misa
	uint64_t misa = 0x0; asm volatile("csrr %0, misa" : "=r"(misa)); // trap & emulate example
	//const uint64_t misa = ECALL_CSRR_MISA();

	const int xlen = ((misa >> __riscv_xlen-2)&0b11)==1 ?  32 :
					 ((misa >> __riscv_xlen-2)&0b11)==2 ?  64 :
					 ((misa >> __riscv_xlen-2)&0b11)==1 ? 128 : 0;

	char misa_str[26+1]="";
	for (int i=0, j=0; i<26; i++)
		if ( (misa & (1ul << i)) !=0){
			misa_str[j++]=(char)('A'+i); misa_str[j]='\0';
		}

	printf("Machine ISA   : 0x%08x RV%d %s \n", (int)misa, xlen, misa_str);

	// mvendorid
	const uint64_t mvendorid = ECALL_CSRR_MVENDID();
	const char *mvendorid_str = (mvendorid==0x10e31913 ? "SiFive, Inc.\0" :
						         mvendorid==0x489      ? "SiFive, Inc.\0" :
								 mvendorid==0x57c      ? "Hex Five, Inc.\0" :
												         "\0");
	printf("Vendor        : 0x%08x %s \n", (int)mvendorid, mvendorid_str);

	// marchid
	const uint64_t marchid = ECALL_CSRR_MARCHID();
	const char *marchid_str = (mvendorid==0x489 && (int)misa==0x40101105    && marchid==0x1 ? "E31\0" :
						       mvendorid==0x489 && misa==0x8000000000101105 && marchid==0x1 ? "S51\0" :
						       mvendorid==0x57c && (int)misa==0x40101105    && marchid==0x1 ? "X300\0" :
								 	 	 	 	 	 	 	 	 	 	 	 	 	 		  "\0");
	printf("Architecture  : 0x%08x %s \n", (int)marchid, marchid_str);

	// mimpid
	const uint64_t mimpid = ECALL_CSRR_MIMPID();
	printf("Implementation: 0x%08x \n", (int)mimpid );

	// mhartid
	const uint64_t mhartid = ECALL_CSRR_MHARTID();
	printf("Hart ID       : 0x%08x \n", (int)mhartid );

	// CPU Clock
	const int cpu_clk = round(CPU_FREQ/1E+6);
	printf("CPU clock     : %d MHz \n", cpu_clk );

}

// ------------------------------------------------------------------------
int cmpfunc(const void* a , const void* b){
// ------------------------------------------------------------------------
    const int ai = *(const int* )a;
    const int bi = *(const int* )b;
    return ai < bi ? -1 : ai > bi ? 1 : 0;
}

// ------------------------------------------------------------------------
void print_stats(void){
// ------------------------------------------------------------------------

	const int COUNT = 10+1; // odd values for med
	const int MHZ = CPU_FREQ/1000;

	int cycles[COUNT], ctxsw_cycle[COUNT], ctxsw_instr[COUNT]; cycles[0]=0; ctxsw_instr[0]=0;

	for (int i=0, first=1; i<COUNT; i++){

		uint32_t r0, r1, r2;

		asm volatile (

			"   li %1, 150; li %2, 300000;"
			"   li a0, 6; ecall; mv %0, a0; "

			"0: li a0, 6; ecall;  "
			"   sub %0, a0, %0; bgeu %0, %1, 2f; "
			"   addi %2, %2, -1; beqz %2, 1f; "
			"	mv %0, a0; "
			"   j 0b; "

			"1: li %0, 0x0;"
			"2: "

			"li a0, 8; ecall; mv %1, a0; " // mhpmcounter3 (cycle)
			"li a0, 9; ecall; mv %2, a0; " // mhpmcounter4 (instr)

		: "=r"(r0), "=r"(r1), "=r"(r2) : : "a0", "a1" );

		cycles[i] 	   = (int)r0;
		ctxsw_cycle[i] = (int)r1;
		ctxsw_instr[i] = (int)r2;


		if (r0==0) break;

		if (first) {first=0; i--;} // ignore 1st reading (cache?)

	}

	if (cycles[0]>0){

		int max_cycle = 0; for (int i=0; i<COUNT; i++){max_cycle = cycles[i] > max_cycle ? cycles[i] : max_cycle;}
		char str[16]; sprintf(str, "%d", max_cycle); const int max_col = strlen(str);
		for (int i=0; i<COUNT; i++)
			printf("%*d cycles in %*d us \n", max_col, cycles[i], max_col-2, (int)(cycles[i]*1000/MHZ));

		qsort(cycles, COUNT, sizeof(int), cmpfunc);

		printf("------------------------------------------------\n");
		int min = cycles[0], med = cycles[COUNT/2], max = cycles[COUNT-1];
		printf("cycles  min/med/max = %d/%d/%d \n", min, med, max);
		printf("time    min/med/max = %d/%d/%d us \n", (int)min*1000/MHZ, (int)med*1000/MHZ, (int)max*1000/MHZ);

	}

	if (ctxsw_instr[0]>0 && cycles[0]>0){

		qsort(ctxsw_cycle, COUNT, sizeof(int), cmpfunc);
		qsort(ctxsw_instr, COUNT, sizeof(int), cmpfunc);

		printf("\n");
		int min = ctxsw_instr[0], med = ctxsw_instr[COUNT/2], max = ctxsw_instr[COUNT-1];
		printf("ctx sw instr  min/med/max = %d/%d/%d \n", min, med, max);
		min = ctxsw_cycle[0], med = ctxsw_cycle[COUNT/2], max = ctxsw_cycle[COUNT-1];
		printf("ctx sw cycles min/med/max = %d/%d/%d \n", min, med, max);
		printf("ctx sw time   min/med/max = %d/%d/%d us \n", (int)min*1000/MHZ, (int)med*1000/MHZ, (int)max*1000/MHZ);

	} else if (ctxsw_instr[0]>0 && cycles[0]==0){

		printf("ctx sw instr  = %d \n", ctxsw_instr[0]);
		printf("ctx sw cycles = %d \n", ctxsw_cycle[0]);
		printf("ctx sw time   = %d us \n", (int)ctxsw_cycle[0]*1000/MHZ);
	}

	if (cycles[0]==0 && ctxsw_instr[0]==0) printf("stats : n/a \n");

}

// ------------------------------------------------------------------------
void print_pmp_ranges(void){
// ------------------------------------------------------------------------


	#define TOR   0b00001000
	#define NA4   0b00010000
	#define NAPOT 0b00011000

	#define PMP_R 1<<0
	#define PMP_W 1<<1
	#define PMP_X 1<<2

	volatile uint64_t pmpcfg=0x0;
#if __riscv_xlen==32
	volatile uint32_t pmpcfg0; asm ( "csrr %0, pmpcfg0" : "=r"(pmpcfg0) );
	volatile uint32_t pmpcfg1; asm ( "csrr %0, pmpcfg1" : "=r"(pmpcfg1) );
	pmpcfg = pmpcfg1;
	pmpcfg <<= 32;
	pmpcfg |= pmpcfg0;
#else
	asm ( "csrr %0, pmpcfg0" : "=r"(pmpcfg) );
#endif

#if __riscv_xlen==32
	uint32_t pmpaddr[8];
#else
	uint64_t pmpaddr[8];
#endif
	asm ( "csrr %0, pmpaddr0" : "=r"(pmpaddr[0]) );
	asm ( "csrr %0, pmpaddr1" : "=r"(pmpaddr[1]) );
	asm ( "csrr %0, pmpaddr2" : "=r"(pmpaddr[2]) );
	asm ( "csrr %0, pmpaddr3" : "=r"(pmpaddr[3]) );
	asm ( "csrr %0, pmpaddr4" : "=r"(pmpaddr[4]) );
	asm ( "csrr %0, pmpaddr5" : "=r"(pmpaddr[5]) );
	asm ( "csrr %0, pmpaddr6" : "=r"(pmpaddr[6]) );
	asm ( "csrr %0, pmpaddr7" : "=r"(pmpaddr[7]) );

	for (int i=0; i<8; i++){

		const uint8_t cfg = (pmpcfg >> 8*i); if (cfg==0x0) continue;

		char rwx[3+1] = {cfg & PMP_R ? 'r':'-', cfg & PMP_W ? 'w':'-', cfg & PMP_X ? 'x':'-', '\0'};

		uint64_t start=0, end=0;

		if ( (cfg & (TOR | NA4 | NAPOT)) == TOR){
			start = pmpaddr[i-1]<<2;
			end =  (pmpaddr[i]<<2) -1;

		} else if ( (cfg & (TOR | NA4 | NAPOT)) == NA4){
			start = pmpaddr[i]<<2;
			end =  start+4 -1;

		} else if ( (cfg & (TOR | NA4 | NAPOT)) == NAPOT){
			for (int j=0; j<__riscv_xlen; j++){
				if ( ((pmpaddr[i] >> j) & 0x1) == 0){
					const uint64_t size = 1 << (3+j);
					start = (pmpaddr[i] >>j) <<(j+2);
					end = start + size -1;
					break;
				}
			}

		} else break;

#if __riscv_xlen==32
		printf("0x%08x 0x%08x %s \n", (unsigned int)start, (unsigned int)end, rwx);
#else
		printf("0x%08" PRIX64 " 0x%08" PRIX64 " %s \n", start, end, rwx);
#endif

	}

}

// ------------------------------------------------------------------------
 int readline(char *cmd_line) {
// ------------------------------------------------------------------------
	int p=0;
	char c='\0';
	int esc=0;
	cmd_line[0] = '\0';
	static char history[CMD_LINE_SIZE+1]="";

	while(c!='\r'){

		if ( read(0, &c, 1) >0 ) {

			if (c=='\e'){
				esc=1;

			} else if (esc==1 && c=='['){
				esc=2;

			} else if (esc==2 && c=='3'){
				esc=3;

			} else if (esc==3 && c=='~'){ // del key
				for (int i=p; i<strlen(cmd_line); i++) cmd_line[i]=cmd_line[i+1];
				write(1, "\e7", 2); // save curs pos
				write(1, "\e[K", 3); // clear line from curs pos
				write(1, &cmd_line[p], strlen(cmd_line)-p);
				write(1, "\e8", 2); // restore curs pos
				esc=0;

			} else if (esc==2 && c=='C'){ // right arrow
				esc=0;
				if (p < strlen(cmd_line)){
					p++;
					write(1, "\e[C", 3);
				}

			} else if (esc==2 && c=='D'){ // left arrow
				esc=0;
				if (p>0){
					p--;
					write(1, "\e[D", 3);
				}

			} else if (esc==2 && c=='A'){ // up arrow
				esc=0;
				if (strlen(history)>0){
					p=strlen(history);
					strcpy(cmd_line, history);
					write(1, "\e[2K", 4); // 2K clear entire line - cur pos dosn't change
					write(1, "\rZ1 > ", 6);
					write(1, &cmd_line[0], strlen(cmd_line));
				}

			} else if (esc==2 && c=='B'){ // down arrow
				esc=0;

			} else if ((c=='\b' || c=='\x7f') && p>0 && esc==0){ // backspace
				p--;
				for (int i=p; i<strlen(cmd_line); i++) cmd_line[i]=cmd_line[i+1];
				write(1, "\e[D", 3);
				write(1, "\e7", 2);
				write(1, "\e[K", 3);
				write(1, &cmd_line[p], strlen(cmd_line)-p);
				write(1, "\e8", 2);

			} else if (c>=' ' && c<='~' && p < CMD_LINE_SIZE && esc==0){
				for (int i = CMD_LINE_SIZE-1; i > p; i--) cmd_line[i]=cmd_line[i-1]; // make room for 1 ch
				cmd_line[p]=c;
				write(1, "\e7", 2); // save curs pos
				write(1, "\e[K", 3); // clear line from curs pos
				write(1, &cmd_line[p], strlen(cmd_line)-p); p++;
				write(1, "\e8", 2); // restore curs pos
				write(1, "\e[C", 3); // move curs right 1 pos

			} else
				esc=0;
		}

		// poll & print incoming messages
		int msg[MSG_SIZE]={0,0,0,0};

		if (ECALL_RECV(3, msg)){

			write(1, "\e7", 2); // save curs pos
			write(1, "\e[2K", 4); // 2K clear entire line - cur pos dosn't change

			switch (msg[0]) {
			case 1   : write(1, "\rZ3 > USB DEVICE ATTACH VID=0x1267 PID=0x0000\r\n", 47); break;
			case 2   : write(1, "\rZ3 > USB DEVICE DETACH\r\n", 25); break;
			case 331 : write(1, "\rZ3 > CLINT IRQ 23 [BTN3]\r\n", 27); break;
			case 'p' : write(1, "\rZ3 > pong\r\n", 12); break;
			default  : write(1, "\rZ3 > ???\r\n", 11); break;
			}

			write(1, "\nZ1 > ", 6);
			write(1, &cmd_line[0], strlen(cmd_line));
			write(1, "\e8", 2);   // restore curs pos
			write(1, "\e[2B", 4); // curs down down
		}

		if (ECALL_RECV(2, msg)){

			write(1, "\e7", 2);   // save curs pos
			write(1, "\e[2K", 4); // 2K clear entire line - cur pos dosn't change

			switch (msg[0]) {
			case 201 : write(1, "\rZ2 > PLIC  IRQ 11 [BTN0]\r\n", 27); break;
			case 207 : write(1, "\rZ2 > CLINT IRQ 17 [BTN1]\r\n", 27); break;
			case 208 : write(1, "\rZ2 > CLINT IRQ 18 [BTN2]\r\n", 27); break;
			case 211 : write(1, "\rZ2 > CLINT IRQ 21 [BTN1]\r\n", 27); break;
			case 212 : write(1, "\rZ2 > CLINT IRQ 22 [BTN2]\r\n", 27); break;
			case 'p' : write(1, "\rZ2 > pong\r\n", 12); break;
			default  : write(1, "\rZ2 > ???\r\n", 11); break;
			}

			write(1, "\nZ1 > ", 6);
			write(1, &cmd_line[0], strlen(cmd_line));
			write(1, "\e8", 2);   // restore curs pos
			write(1, "\e[2B", 4); // curs down down
		}

		ECALL_YIELD();

	}

	for (int i = CMD_LINE_SIZE-1; i > 0; i--)
		if (cmd_line[i]==' ') cmd_line[i]='\0';	else break;

	if (strlen(cmd_line)>0)
		strcpy(history, cmd_line);

	return strlen(cmd_line);

}

// ------------------------------------------------------------------------
int main (void) {
// ------------------------------------------------------------------------

	//volatile int w=0; while(1){w++;}
	//while(1) ECALL_YIELD();

	ECALL_TRP_VECT(0x0, trap_0x0_handler); // 0x0 Instruction address misaligned
	ECALL_TRP_VECT(0x1, trap_0x1_handler); // 0x1 Instruction access fault
	ECALL_TRP_VECT(0x2, trap_0x2_handler); // 0x2 Illegal Instruction
	ECALL_TRP_VECT(0x3, trap_0x3_handler); // 0x3 Soft timer
    ECALL_TRP_VECT(0x4, trap_0x4_handler); // 0x4 Load address misaligned
    ECALL_TRP_VECT(0x5, trap_0x5_handler); // 0x5 Load access fault
    ECALL_TRP_VECT(0x6, trap_0x6_handler); // 0x6 Store/AMO address misaligned
	ECALL_TRP_VECT(0x7, trap_0x7_handler); // 0x7 Store access fault

	open("UART", 0, 0);

	printf("\e[2J\e[H"); // clear terminal screen
	printf("=====================================================================\n");
	printf("      	           Hex Five MultiZone(TM) Security                   \n");
	printf("    Copyright (C) 2018 Hex Five Security Inc. All Rights Reserved    \n");
	printf("=====================================================================\n");
	printf("This version of MultiZone(TM) is meant for evaluation purposes only. \n");
	printf("As such, use of this software is governed by your Evaluation License.\n");
	printf("There may be other functional limitations as described in the        \n");
	printf("evaluation kit documentation. The full version of the software does  \n");
	printf("not have these restrictions.                                         \n");
	printf("=====================================================================\n");

    print_cpu_info();

	char cmd_line[CMD_LINE_SIZE+1]="";
	int msg[MSG_SIZE]={0,0,0,0};

	while(1){

		write(1, "\n\rZ1 > ", 7);

		readline(cmd_line);

		write(1, "\n", 1);

		char * tk1 = strtok (cmd_line, " ");
		char * tk2 = strtok (NULL, " ");
		char * tk3 = strtok (NULL, " ");

		if (tk1 != NULL && strcmp(tk1, "load")==0){
			if (tk2 != NULL){
				uint8_t data = 0x00;
				const uint64_t addr = strtoull(tk2, NULL, 16);
				asm ("lbu %0, (%1)" : "+r"(data) : "r"(addr));
				printf("0x%08x : 0x%02x \n", (unsigned int)addr, data);
			} else printf("Syntax: load address \n");

		} else if (tk1 != NULL && strcmp(tk1, "store")==0){
			if (tk2 != NULL && tk3 != NULL){
				const uint32_t data = (uint32_t)strtoul(tk3, NULL, 16);
				const uint64_t addr = strtoull(tk2, NULL, 16);

				if ( strlen(tk3) <=2 )
					asm ( "sb %0, (%1)" : : "r"(data), "r"(addr));
				else if ( strlen(tk3) <=4 )
					asm ( "sh %0, (%1)" : : "r"(data), "r"(addr));
				else
					asm ( "sw %0, (%1)" : : "r"(data), "r"(addr));

				printf("0x%08x : 0x%02x \n", (unsigned int)addr, (unsigned int)data);
			} else printf("Syntax: store address data \n");

		} else if (tk1 != NULL && strcmp(tk1, "exec")==0){
			if (tk2 != NULL){
				const uint64_t addr = strtoull(tk2, NULL, 16);
			    asm ( "jr (%0)" : : "r"(addr));
		} else printf("Syntax: exec address \n");

		} else if (tk1 != NULL && strcmp(tk1, "send")==0){
			if (tk2 != NULL && tk2[0]>='1' && tk2[0]<='4' && tk3 != NULL){
				for (int i=0; i<MSG_SIZE; i++)
					msg[i] = i<strlen(tk3) ? (unsigned int)*(tk3+i) : 0x0;
				if (!ECALL_SEND(tk2[0]-'0', msg))
					printf("Error: Inbox full.\n");
			} else printf("Syntax: send {1|2|3|4} message \n");

		} else if (tk1 != NULL && strcmp(tk1, "recv")==0){
			if (tk2 != NULL && tk2[0]>='1' && tk2[0]<='4'){
				if (ECALL_RECV(tk2[0]-'0', msg))
					printf("msg : 0x%08x 0x%08x 0x%08x 0x%08x \n", msg[0], msg[1], msg[2], msg[3]);
				else
					printf("Error: Inbox empty.\n");
			} else printf("Syntax: recv {1|2|3|4} \n");

		} else if (tk1 != NULL && strcmp(tk1, "yield")==0){
			uint64_t C1 = ECALL_CSRR_MCYCLE();
			ECALL_YIELD();
			uint64_t C2 = ECALL_CSRR_MCYCLE();
			const int T = ((C2-C1)*1000000)/CPU_FREQ;
			printf( (T>0 ? "yield : elapsed time %dus \n" : "yield : n/a \n"), T);

		} else if (tk1 != NULL && strcmp(tk1, "stats")==0){
			print_stats();

		} else if (tk1 != NULL && strcmp(tk1, "restart")==0){
			asm ("j _start");

		} else if (tk1 != NULL && strcmp(tk1, "timer")==0){
			if (tk2 != NULL){
				const uint64_t ms = abs(strtoull(tk2, NULL, 10));
				const uint64_t T0 = ECALL_CSRR_MTIME();
				const uint64_t T1 = T0 + ms*RTC_FREQ/1000;
				ECALL_CSRW_MTIMECMP(T1);
				printf("timer set T0=%lu, T1=%lu \n", (unsigned long)(T0*1000/RTC_FREQ),
													  (unsigned long)(T1*1000/RTC_FREQ) );
			} else printf("Syntax: timer ms \n");

		} else if (tk1 != NULL && strcmp(tk1, "pmp")==0){
			print_pmp_ranges();

		} else
			printf("Commands: load store exec send recv yield pmp stats timer restart \n");

	}

}
