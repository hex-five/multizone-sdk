/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <fcntl.h>	// open()
#include <unistd.h> // read() write()
#include <string.h>	// strxxx()
#include <stdio.h>	// printf()
#include <stdlib.h> // qsort() strtoul()

#include <platform.h>
#include <multizone.h>

__attribute__((interrupt())) void trap_handler(void){

	const unsigned long mcause = ECALL_CSRR(CSR_MCAUSE); //CSRR(mcause);//asm volatile("csrr %0, mcause" : "=r"(mcause));
	const unsigned long mepc   = ECALL_CSRR(CSR_MEPC);   //CSRR(mepc); 	//asm volatile("csrr %0, mepc"   : "=r"(mepc));
	const unsigned long mtval  = ECALL_CSRR(CSR_MTVAL);  //CSRR(mtval);	//asm volatile("csrr %0, mtval"  : "=r"(mtval));

	char c='\0';

	switch(mcause){

	case 0 : printf("Instruction address misaligned : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
 	 	 	 printf("Press any key to restart this zone \n");
			 while(read(0, &c, 1) ==0 ){;} asm ("j _start");
			 break;

	case 1 : printf("Instruction access fault : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
 	 	 	 printf("Press any key to restart this zone \n");
			 while(read(0, &c, 1) ==0 ){;} asm ("j _start");
			 break;

	case 2 : printf("Illegal instruction : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
 	 	 	 printf("Press any key to restart this zone \n");
			 while(read(0, &c, 1) ==0 ){;} asm ("j _start");
			 break;

	case 3 : printf("Breakpoint : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
	 	 	 printf("Press any key to restart this zone \n");
			 while(read(0, &c, 1) ==0 ){;} asm ("j _start");
			 break;

	case 4 : printf("Load address misaligned : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
	 	 	 asm volatile("csrw mepc, %0" : : "r"(mepc+4)); // skip
			 break;

	case 5 : printf("Load access fault : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
			 asm volatile("csrw mepc, %0" : : "r"(mepc+4)); // skip
			 break;

	case 6 : printf("Store/AMO address misaligned : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
	 	 	 asm volatile("csrw mepc, %0" : : "r"(mepc+4)); // skip
			 break;

	case 7 : printf("Store access fault : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
	 	 	 asm volatile("csrw mepc, %0" : : "r"(mepc+4)); // skip
			 break;

	case 8 : printf("Environment call from U-mode : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
	 	 	 asm volatile("csrw mepc, %0" : : "r"(mepc+4)); // skip
			 break;

	case 0x80000003 : printf("Machine software interrupt : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
					  printf("Press any key to restart this zone \n");
					  while(read(0, &c, 1) ==0 ){;} asm ("j _start");
					  break;

	case 0x80000007 : printf("Machine timer interrupt : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
					  printf("Press any key to restart this zone \n");
					  while(read(0, &c, 1) ==0 ){;} asm ("j _start");
					  break;

	case 0x80000011 : printf("Machine external interrupt : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
					  printf("Press any key to restart this zone \n");
					  while(read(0, &c, 1) ==0 ){;} asm ("j _start");
					  break;

	default: printf("Exception : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
			 printf("Press any key to restart this zone \n");
			 while(read(0, &c, 1) ==0 ){;} asm ("j _start");

	}

}

// ------------------------------------------------------------------------
void print_cpu_info(void) {
// ------------------------------------------------------------------------

	// misa
	const unsigned long misa = CSRR(misa);
	const int xlen = ((misa >> __riscv_xlen-2)&0b11)==1 ?  32 :
					 ((misa >> __riscv_xlen-2)&0b11)==2 ?  64 :
					 ((misa >> __riscv_xlen-2)&0b11)==1 ? 128 :
							 	 	 	 	 	 	 	 	0 ;
	char misa_str[26+1]="";
	for (int i=0, j=0; i<26; i++)
		if ( (misa & (1ul << i)) !=0){
			misa_str[j++]=(char)('A'+i); misa_str[j]='\0';
		}
	printf("Machine ISA   : 0x%08x RV%d %s \n", (int)misa, xlen, misa_str);

	// mvendid
	const unsigned long  mvendorid = CSRR(mvendorid);
	const char *mvendorid_str = (mvendorid==0x10e31913 ? "SiFive, Inc.\0" :
							     mvendorid==0x489      ? "SiFive, Inc.\0" :
							     mvendorid==0x57c      ? "Hex Five, Inc.\0" :
							    		 	 	 	 	 "\0");
	printf("Vendor        : 0x%08x %s \n", (int)mvendorid, mvendorid_str);

	// marchid
	const unsigned long  marchid = CSRR(marchid);
	const char *marchid_str = (mvendorid==0x489 && (int)misa==0x40101105    && marchid==0x80000002 ? "E21\0"  :
							   mvendorid==0x489 && (int)misa==0x40101105    && marchid==0x00000001 ? "E31\0"  :
						       mvendorid==0x489 && misa==0x8000000000101105 && marchid==0x00000001 ? "S51\0"  :
						       mvendorid==0x57c && (int)misa==0x40101105    && marchid==0x00000001 ? "X300\0" :
						       "\0");
	printf("Architecture  : 0x%08x %s \n", (int)marchid, marchid_str);

	// mimpid
	printf("Implementation: 0x%08x \n", CSRR(mimpid) );

	// mhartid
	printf("Hart id       : 0x%1x \n", CSRR(mhartid) );

	// CPU clk
	printf("CPU clock     : %d MHz \n", (int)(CPU_FREQ/1E+6) );

	// RTC clk
	printf("RTC clock     : %d KHz \n", (int)(RTC_FREQ/1E+3) );

}

// ------------------------------------------------------------------------
int cmpfunc(const void* a, const void* b){
// ------------------------------------------------------------------------
    const int ai = *(const int* )a;
    const int bi = *(const int* )b;
    return ai < bi ? -1 : ai > bi ? 1 : 0;
}

// ------------------------------------------------------------------------
void print_stats(void){
// ------------------------------------------------------------------------

	const int COUNT = 10+1; // odd values for median
	#define MHZ (CPU_FREQ/1000000)

	int cycles[COUNT], instrs[COUNT];

	for (int i=0; i<COUNT; i++){

		volatile unsigned long C1 = ECALL_CSRR(CSR_MCYCLE);
		volatile unsigned long I1 = ECALL_CSRR(CSR_MINSTRET);
		ECALL_YIELD();
		volatile unsigned long I2 = ECALL_CSRR(CSR_MINSTRET);
		volatile unsigned long C2 = ECALL_CSRR(CSR_MCYCLE);

		cycles[i] = C2-C1; instrs[i] = I2-I1;

	}

	int max_cycle = 0;
	for (int i=0; i<COUNT; i++)	max_cycle = (cycles[i] > max_cycle ? cycles[i] : max_cycle);
	char str[16]; sprintf(str, "%lu", max_cycle); const int col_len = strlen(str);

	for (int i=0; i<COUNT; i++)
		printf("%*d instr %*d cycles %*d us \n", col_len, instrs[i], col_len, cycles[i], col_len-2, cycles[i]/MHZ);

	qsort(cycles, COUNT, sizeof(int), cmpfunc);
	qsort(instrs, COUNT, sizeof(int), cmpfunc);

	printf("------------------------------------------------\n");
	int min = instrs[0], med = instrs[COUNT/2], max = instrs[COUNT-1];
	printf("instrs  min/med/max = %d/%d/%d \n", min, med, max);
		min = cycles[0], med = cycles[COUNT/2], max = cycles[COUNT-1];
	printf("cycles  min/med/max = %d/%d/%d \n", min, med, max);
	printf("time    min/med/max = %d/%d/%d us \n", min/MHZ, med/MHZ, max/MHZ);

	// mhpmcounters might not be implemented
	volatile unsigned long ctxsw_cycle = ECALL_CSRR(CSR_MHPMCOUNTER3);
	volatile unsigned long ctxsw_instr = ECALL_CSRR(CSR_MHPMCOUNTER4);
	if (ctxsw_instr>0 && cycles>0){
		printf("\n");
		printf("ctx sw instr  = %lu \n", ctxsw_instr);
		printf("ctx sw cycles = %lu \n", ctxsw_cycle);
		printf("ctx sw time   = %d us \n", (int)(ctxsw_cycle/MHZ));
	}

}

// ------------------------------------------------------------------------
void print_pmp(void){
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

		char type[5+1]="";

		if ( (cfg & (TOR | NA4 | NAPOT)) == TOR){
			start = pmpaddr[i-1]<<2;
			end =  (pmpaddr[i]<<2) -1;
			strcpy(type, "TOR");

		} else if ( (cfg & (TOR | NA4 | NAPOT)) == NA4){
			start = pmpaddr[i]<<2;
			end =  start+4 -1;
			strcpy(type, "NA4");

		} else if ( (cfg & (TOR | NA4 | NAPOT)) == NAPOT){
			for (int j=0; j<__riscv_xlen; j++){
				if ( ((pmpaddr[i] >> j) & 0x1) == 0){
					const uint64_t size = 1 << (3+j);
					start = (pmpaddr[i] >>j) <<(j+2);
					end = start + size -1;
					strcpy(type, "NAPOT");
					break;
				}
			}

		} else break;

#if __riscv_xlen==32
		printf("0x%08x 0x%08x %s %s \n", (unsigned long)start, (unsigned int)end, rwx, type);
#else
		printf("0x%08" PRIX64 " 0x%08" PRIX64 " %s %s \n", start, end, rwx, type);
#endif

	}

}

// ------------------------------------------------------------------------
int readline(char *cmd_line) {
// ------------------------------------------------------------------------

	#define CMD_LINE_SIZE 32

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
		int msg[4]={0,0,0,0};

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

	CSRW(mtvec, trap_handler); // register trap handler

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
	int msg[4]={0,0,0,0};

	while(1){

		write(1, "\n\rZ1 > ", 7);

		readline(cmd_line);

		write(1, "\n", 1);

		char * tk1 = strtok (cmd_line, " ");
		char * tk2 = strtok (NULL, " ");
		char * tk3 = strtok (NULL, " ");

		if (tk1 == NULL) tk1 = "help";

		// --------------------------------------------------------------------
		if (strcmp(tk1, "load")==0){
		// --------------------------------------------------------------------
			if (tk2 != NULL){
				uint8_t data = 0x00;
				const unsigned long addr = strtoull(tk2, NULL, 16);
				asm ("lbu %0, (%1)" : "+r"(data) : "r"(addr));
				printf("0x%08x : 0x%02x \n", (unsigned int)addr, data);
			} else printf("Syntax: load address \n");

		// --------------------------------------------------------------------
		} else if (strcmp(tk1, "store")==0){
		// --------------------------------------------------------------------
			if (tk2 != NULL && tk3 != NULL){
				const uint32_t data = (uint32_t)strtoul(tk3, NULL, 16);
				const unsigned long addr = strtoull(tk2, NULL, 16);

				if ( strlen(tk3) <=2 )
					asm ( "sb %0, (%1)" : : "r"(data), "r"(addr));
				else if ( strlen(tk3) <=4 )
					asm ( "sh %0, (%1)" : : "r"(data), "r"(addr));
				else
					asm ( "sw %0, (%1)" : : "r"(data), "r"(addr));

				printf("0x%08x : 0x%02x \n", (unsigned int)addr, (unsigned int)data);
			} else printf("Syntax: store address data \n");

		// --------------------------------------------------------------------
		} else if (strcmp(tk1, "exec")==0){
		// --------------------------------------------------------------------
			if (tk2 != NULL){
				const unsigned long addr = strtoull(tk2, NULL, 16);
				asm ( "jr (%0)" : : "r"(addr));
			} else printf("Syntax: exec address \n");

		// --------------------------------------------------------------------
		} else if (strcmp(tk1, "send")==0){
		// --------------------------------------------------------------------
			if (tk2 != NULL && tk2[0]>='1' && tk2[0]<='4' && tk3 != NULL){
				for (int i=0; i<4; i++)
					msg[i] = i<strlen(tk3) ? (unsigned int)*(tk3+i) : 0x0;
				if (!ECALL_SEND( tk2[0]-'0', msg) )
					printf("Error: Inbox full.\n");
			} else printf("Syntax: send {1|2|3|4} message \n");

		// --------------------------------------------------------------------
		} else if (strcmp(tk1, "recv")==0){
		// --------------------------------------------------------------------
			if (tk2 != NULL && tk2[0]>='1' && tk2[0]<='4'){
				if (ECALL_RECV(tk2[0]-'0', msg))
					printf("msg : 0x%08x 0x%08x 0x%08x 0x%08x \n", msg[0], msg[1], msg[2], msg[3]);
				else
					printf("Error: Inbox empty.\n");
			} else printf("Syntax: recv {1|2|3|4} \n");

		// --------------------------------------------------------------------
		} else if (strcmp(tk1, "yield")==0){
		// --------------------------------------------------------------------
			volatile unsigned long I1 = ECALL_CSRR(CSR_MINSTRET);
			volatile unsigned long C1 = ECALL_CSRR(CSR_MCYCLE);
			ECALL_YIELD();
			volatile unsigned long C2 = ECALL_CSRR(CSR_MCYCLE);
			volatile unsigned long I2 = ECALL_CSRR(CSR_MINSTRET);
			const int TC = (C2-C1)/(CPU_FREQ/1000000);
			printf( (TC>0 ? "yield : elapsed time %dus \n" : "yield : n/a \n"), TC);

/*		// --------------------------------------------------------------------
		} else if (strcmp(tk1, "timer")==0){
		// --------------------------------------------------------------------
			if (tk2 != NULL){
				const uint64_t ms = abs(strtoull(tk2, NULL, 10));
				const uint64_t T0 = ECALL_CSRR_MTIME();
				const uint64_t T1 = T0 + ms*RTC_FREQ/1000;
				ECALL_CSRW_MTIMECMP(T1);
				printf("timer set T0=%lu, T1=%lu \n", (unsigned long)(T0*1000/RTC_FREQ),
													  (unsigned long)(T1*1000/RTC_FREQ) );
			} else printf("Syntax: timer ms \n");*/

		// --------------------------------------------------------------------
		} else if (strcmp(tk1, "stats")==0)	print_stats();
		// --------------------------------------------------------------------

		// --------------------------------------------------------------------
		else if (strcmp(tk1, "restart")==0) asm ("j _start");
		// --------------------------------------------------------------------

		// --------------------------------------------------------------------
		else if (strcmp(tk1, "pmp")==0) print_pmp();
		// --------------------------------------------------------------------

		// --------------------------------------------------------------------
		else if (strcmp(tk1, "ebreak")==0) asm("ebreak");
		// --------------------------------------------------------------------

		// --------------------------------------------------------------------
		else if (strcmp(tk1, "rdtime")==0){
		// --------------------------------------------------------------------
			uint64_t timecmp = ECALL_RDTIME();
			printf("0x%08x_%08x \n", (uint32_t)(timecmp>>32), (uint32_t)timecmp);
		}

		// --------------------------------------------------------------------
		else if (strcmp(tk1, "rdtimecmp")==0){
		// --------------------------------------------------------------------
			uint64_t timecmp = ECALL_RDTIMECMP();
			printf("0x%08x_%08x \n", (uint32_t)(timecmp>>32), (uint32_t)timecmp);
		}

		// --------------------------------------------------------------------
		else if (strcmp(tk1, "wrtimecmp")==0){
		// --------------------------------------------------------------------
			uint64_t timecmp = strtoull("0x1234567890abcdef", NULL, 16);
			ECALL_WRTIMECMP(timecmp);
		}

		// --------------------------------------------------------------------
		else if (strcmp(tk1, "trap")==0) asm ("rdtime x0"); // csrrs x0, time, 0 => M trap 0x2
		// --------------------------------------------------------------------

		else printf("Commands: yield send recv pmp load store exec stats rdtime timer restart \n");

	}

}

/*
    printf("\n");
    printf("# CSRR(mscratch) 0x%08x \n", CSRR(mscratch));

    printf("# CSRW(mscratch) 0x12345678 \n"); CSRW(mscratch, 0x12345678);
    printf("# CSRR(mscratch) 0x%08x \n", CSRR(mscratch));

    printf("# CSRW(mscratch) 0x1f \n"); CSRW(mscratch, 31);
    printf("# CSRR(mscratch) 0x%08x \n", CSRR(mscratch));

    printf("# CSRRW(mscratch, 0xff) 0x%08x \n", CSRRW(mscratch, 0xFF));
    printf("# CSRR(mscratch) 0x%08x \n", CSRR(mscratch));

    printf("# CSRRC(mscratch, 0x0) 0x%08x \n", CSRRC(mscratch, 0x0));
    printf("# CSRR(mscratch) 0x%08x \n", CSRR(mscratch));

    printf("# CSRRS(mscratch, 0x0) 0x%08x \n", CSRRS(mscratch, 0x0));
    printf("# CSRR(mscratch) 0x%08x \n", CSRR(mscratch));

 */
