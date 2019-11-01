/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <fcntl.h>	// open()
#include <unistd.h> // read() write()
#include <string.h>	// strxxx()
#include <stdio.h>	// printf() sprintf()
#include <stdlib.h> // qsort() strtoul()

#include <platform.h>
#include <plic_driver.h>
#include <multizone.h>

#define BUFFER_SIZE 16
static struct{
	char data[BUFFER_SIZE];
	volatile int p0; // read
	volatile int p1; // write
} buffer;

static char inputline[32+1]="";

plic_instance_t g_plic;

__attribute__((interrupt())) void trap_handler(void){

	const unsigned long mcause = ECALL_CSRR(CSR_MCAUSE);
	const unsigned long mepc   = ECALL_CSRR(CSR_MEPC);
	const unsigned long mtval  = ECALL_CSRR(CSR_MTVAL);

	switch(mcause){

	case 0 : printf("Instruction address missaligned : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
			 break;

	case 1 : printf("Instruction access fault : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
			 break;

	case 2 : printf("Illegal instruction : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
			 break;

	case 3 : printf("Breakpoint : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
			 break;

	case 4 : printf("Load address missaligned : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
	 	 	 CSRW(mepc, mepc+4); // skip
	 	 	 return;

	case 5 : printf("Load access fault : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
 	 	 	 CSRW(mepc, mepc+4); // skip
	 	 	 return;

	case 6 : printf("Store/AMO address missaligned : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
 	 	 	 CSRW(mepc, mepc+4); // skip
	 	 	 return;

	case 7 : printf("Store access fault : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
 	 	 	 CSRW(mepc, mepc+4); // skip
	 	 	 return;

	case 8 : printf("Environment call from U-mode : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
			 break;

	case 9 : printf("Environment call from S-mode : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
			 break;

	case 11: printf("Environment call from M-mode : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
			 break;

	case 0x80000003: printf("Machine software interrupt : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
					 break;

	case 0x80000007: // Machine timer interrupt
					 write(1, "\e7\e[2K", 6);   // save curs pos & clear entire line
					 printf("\rMachine timer interrupt : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
					 write(1, "\nZ1 > %s", 6); write(1, inputline, strlen(inputline));
					 write(1, "\e8\e[2B", 6);   // restore curs pos & curs down 2x

					 ECALL_WRTIMECMP((uint64_t)-1); // reset mip.7
					 return;

	case 0x8000000B: // Machine external interrupt
					 ;const plic_source int_num  = PLIC_claim_interrupt(&g_plic); // claim
						 if (buffer.p0==buffer.p1) {buffer.p0=0; buffer.p1=0;}
						 read(0, &buffer.data[buffer.p1++], 1);
						 if (buffer.p1> BUFFER_SIZE-1) buffer.p1 = BUFFER_SIZE-1;
					 PLIC_complete_interrupt(&g_plic, int_num); // complete
					 return;

	default : printf("Exception : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);

	}

	printf("Press any key to restart \n");
	char c='\0'; while(read(0, &c, 1) ==0 ){;} asm ("j _start"); // blocking loop

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

    const int ai = *(const int* )a;
    const int bi = *(const int* )b;
    return ai < bi ? -1 : ai > bi ? 1 : 0;
}

// ------------------------------------------------------------------------
void print_stats(void){

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

	printf("-----------------------------------------\n");
	int min = instrs[0], med = instrs[COUNT/2], max = instrs[COUNT-1];
	printf("instrs min/med/max = %d/%d/%d \n", min, med, max);
		min = cycles[0], med = cycles[COUNT/2], max = cycles[COUNT-1];
	printf("cycles min/med/max = %d/%d/%d \n", min, med, max);
	printf("time   min/med/max = %d/%d/%d us \n", min/MHZ, med/MHZ, max/MHZ);

	// Kernel stats - may not be available (#ifdef STATS)
	const unsigned long count 	  = ECALL_CSRR(CSR_MHPMCOUNTER21);
	const unsigned long cycle_min = ECALL_CSRR(CSR_MHPMCOUNTER22);
	const unsigned long cycle_avg = ECALL_CSRR(CSR_MHPMCOUNTER23)/count;
	const unsigned long cycle_max = ECALL_CSRR(CSR_MHPMCOUNTER24);
	const unsigned long instr_min = ECALL_CSRR(CSR_MHPMCOUNTER25);
	const unsigned long instr_avg = ECALL_CSRR(CSR_MHPMCOUNTER26)/count;
	const unsigned long instr_max = ECALL_CSRR(CSR_MHPMCOUNTER27);

	if (count>0){
		printf("\n");
		printf("Kernel\n");
		printf("-----------------------------------------\n");
		printf("instrs min/avg/max = %lu/%lu/%lu \n", instr_min, instr_avg, instr_max);
		printf("cycles min/avg/max = %lu/%lu/%lu \n", cycle_min, cycle_avg, cycle_max);
		printf("time   min/avg/max = %lu/%lu/%lu \n", cycle_min/MHZ, cycle_avg/MHZ, cycle_max/MHZ);

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
void msg_handler() {

	// Message handler
	for (int zone=2; zone<=4; zone++){

		char msg[16];

		if (ECALL_RECV(zone, msg)) {

			if (strcmp("ping", msg) == 0)
				ECALL_SEND(zone, "pong");

			else {
				write(1, "\e7\e[2K", 6);   // save curs pos & clear entire line
				printf("\rZ%d > %.16s\n", zone, msg);
				write(1, "\nZ1 > %s", 6); write(1, inputline, strlen(inputline));
				write(1, "\e8\e[2B", 6);   // restore curs pos & curs down 2x
			}
		}

	}

}

// ------------------------------------------------------------------------
void cmd_handler(){

	char * tk1 = strtok (inputline, " ");
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
			char msg[16]; strncpy(msg, tk3, 16);
			if (!ECALL_SEND( tk2[0]-'0', msg) )
				printf("Error: Inbox full.\n");
		} else printf("Syntax: send {1|2|3|4} message \n");

	// --------------------------------------------------------------------
	} else if (strcmp(tk1, "recv")==0){
	// --------------------------------------------------------------------
		if (tk2 != NULL && tk2[0]>='1' && tk2[0]<='4'){
			char msg[16];
			if (ECALL_RECV(tk2[0]-'0', msg))
				printf("msg : %.16s\n", msg);
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

	// --------------------------------------------------------------------
	} else if (strcmp(tk1, "timer")==0){
	// --------------------------------------------------------------------
		if (tk2 != NULL){
			const uint64_t ms = abs(strtoull(tk2, NULL, 10));
			const uint64_t T0 = ECALL_RDTIME();
			const uint64_t T1 = T0 + ms*RTC_FREQ/1000;
			ECALL_WRTIMECMP(T1);
			printf("timer set T0=%lu, T1=%lu \n", (unsigned long)(T0*1000/RTC_FREQ),
												  (unsigned long)(T1*1000/RTC_FREQ) );
		} else printf("Syntax: timer ms \n");

	// --------------------------------------------------------------------
	} else if (strcmp(tk1, "stats")==0)	print_stats();
	// --------------------------------------------------------------------

	// --------------------------------------------------------------------
	else if (strcmp(tk1, "restart")==0) asm ("j _start");
	// --------------------------------------------------------------------

	// --------------------------------------------------------------------
	else if (strcmp(tk1, "pmp")==0) print_pmp();
	// --------------------------------------------------------------------

	else printf("Commands: yield send recv pmp load store exec stats timer restart \n");

}

// ------------------------------------------------------------------------
int readline() {
// ------------------------------------------------------------------------

	static int p=0;
	static int esc=0;
	static char history[8][sizeof(inputline)]={"","","","","","","",""}; static int h=-1;

	while ( buffer.p1>buffer.p0 ) {

		const char c = buffer.data[buffer.p0++];

		if (c=='\e'){
			esc=1;

		} else if (esc==1 && c=='['){
			esc=2;

		} else if (esc==2 && c=='3'){
			esc=3;

		} else if (esc==3 && c=='~'){ // del key
			for (int i=p; i<strlen(inputline); i++) inputline[i]=inputline[i+1];
			write(1, "\e7", 2); // save curs pos
			write(1, "\e[K", 3); // clear line from curs pos
			write(1, &inputline[p], strlen(inputline)-p);
			write(1, "\e8", 2); // restore curs pos
			esc=0;

		} else if (esc==2 && c=='C'){ // right arrow
			esc=0;
			if (p < strlen(inputline)){
				p++;
				write(1, "\e[C", 3);
			}

		} else if (esc==2 && c=='D'){ // left arrow
			esc=0;
			if (p>0){
				p--;
				write(1, "\e[D", 3);
			}

		} else if (esc==2 && c=='A'){ // up arrow (history)
			esc=0;
			if (h<8-1 && strlen(history[h+1])>0){
				h++;
				strcpy(inputline, history[h]);
				write(1, "\e[2K", 4); // 2K clear entire line - cur pos dosn't change
				write(1, "\rZ1 > ", 6);
				write(1, inputline, strlen(inputline));
				p=strlen(inputline);

			}

		} else if (esc==2 && c=='B'){ // down arrow (history)
			esc=0;
			if (h>0 && strlen(history[h-1])>0){
				h--;
				strcpy(inputline, history[h]);
				write(1, "\e[2K", 4); // 2K clear entire line - cur pos dosn't change
				write(1, "\rZ1 > ", 6);
				write(1, inputline, strlen(inputline));
				p=strlen(inputline);
			}

		} else if ((c=='\b' || c=='\x7f') && p>0 && esc==0){ // backspace
			p--;
			for (int i=p; i<strlen(inputline); i++) inputline[i]=inputline[i+1];
			write(1, "\e[D", 3);
			write(1, "\e7", 2);
			write(1, "\e[K", 3);
			write(1, &inputline[p], strlen(inputline)-p);
			write(1, "\e8", 2);

		} else if (c>=' ' && c<='~' && p < sizeof(inputline)-1 && esc==0){
			for (int i = sizeof(inputline)-1-1; i > p; i--) inputline[i]=inputline[i-1]; // make room for 1 ch
			inputline[p]=c;
			write(1, "\e7", 2); // save curs pos
			write(1, "\e[K", 3); // clear line from curs pos
			write(1, &inputline[p], strlen(inputline)-p); p++;
			write(1, "\e8", 2); // restore curs pos
			write(1, "\e[C", 3); // move curs right 1 pos

		} else if (c=='\r') {
			p=0; esc=0;
			write(1, "\n", 1);
			for (int i = sizeof(inputline)-1; i > 0; i--) if (inputline[i]==' ') inputline[i]='\0'; else break;

			if (strlen(inputline)>0 && strcmp(inputline, history[0])!=0){
				for (int i = 8-1; i > 0; i--) strcpy(history[i], history[i-1]);
				strcpy(history[0], inputline);
			}
			h = -1;

			return 1;

		} else esc=0;

	}

	return 0;

}

// ------------------------------------------------------------------------
int main (void) {

	//volatile int w=0; while(1){w++;}
	//while(1) ECALL_YIELD();
	//while(1) ECALL_WFI();

	// Enable PLIC irq 17 (UART)
	#define plic_irq_num 17
	PLIC_init(&g_plic, PLIC_BASE, PLIC_NUM_INTERRUPTS, PLIC_NUM_PRIORITIES);
	PLIC_enable_interrupt (&g_plic, plic_irq_num);
	PLIC_set_priority(&g_plic, plic_irq_num, 1);

	CSRW(mtvec, trap_handler);  // register trap handler
	CSRS(mie, 1<<7);			// enable timer
	CSRS(mie, 1<<11); 			// enable external interrupts (PLIC)
    CSRS(mstatus, 1<<3);		// enable global interrupts (PLIC, TMR)

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

	write(1, "\n\rZ1 > ", 7);

    while(1){

		if (readline()){
			cmd_handler(); //printf("%s\n", inputline);
			write(1, "\n\rZ1 > ", 7);
			inputline[0]='\0';
		}

		msg_handler();

		ECALL_WFI();

	}

} // main()


