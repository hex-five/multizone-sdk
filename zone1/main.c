/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <fcntl.h>	// open()
#include <unistd.h> // read() write()
#include <string.h>	// strxxx()
#include <stdio.h>	// printf() sprintf()
#include <stdlib.h> // qsort() strtoul()
#include <limits.h> // UINT_MAX ULONG_MAX

#include "platform.h"
#include "multizone.h"

#define BUFFER_SIZE 16
static struct{
	char data[BUFFER_SIZE];
	volatile int p0; // read
	volatile int p1; // write
} buffer;

static char inputline[32+1]="";

__attribute__(( interrupt(), aligned(4) )) void trap_handler(void){

	const unsigned long mcause = MZONE_CSRR(CSR_MCAUSE);
	const unsigned long mepc   = MZONE_CSRR(CSR_MEPC);
	const unsigned long mtval  = MZONE_CSRR(CSR_MTVAL);

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
 	 	 	 CSRW(mepc, mepc + (*(char *)mepc & 0b11 ==0b11 ? 4 : 2) ); // skip
	 	 	 return;

	case 5 : printf("Load access fault : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
 	 	 	 CSRW(mepc, mepc+4); // skip
	 	 	 return;

	case 6 : printf("Store/AMO address missaligned : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
 	 	 	 CSRW(mepc, mepc + (*(char *)mepc & 0b11 ==0b11 ? 4 : 2) ); // skip
	 	 	 return;

	case 7 : printf("Store access fault : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
	 	 	 CSRW(mepc, mepc + (*(char *)mepc & 0b11 ==0b11 ? 4 : 2) ); // skip
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
					 write(1, "\e7\e[2K", 6);   	// save curs pos & clear entire line
					 printf("\rMachine timer interrupt : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
					 write(1, "\nZ1 > %s", 6); write(1, inputline, strlen(inputline));
					 write(1, "\e8\e[2B", 6);   	// restore curs pos & curs down 2x
					 MZONE_WRTIMECMP((uint64_t)-1); // reset mip.7
					 CSRC(mie, 1<<7); 				// disable mie.7
					 return;

	case 0x8000000B: // Machine external interrupt (PLIC)
					;const uint32_t plic_int = PLIC_REG(PLIC_CLAIM_OFFSET); // PLIC claim
					if (buffer.p0==buffer.p1) {buffer.p0=0; buffer.p1=0;}
					read(0, &buffer.data[buffer.p1++], 1);
					if (buffer.p1> BUFFER_SIZE-1) buffer.p1 = BUFFER_SIZE-1;
					PLIC_REG(PLIC_CLAIM_OFFSET) = plic_int; // PLIC complete
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

	#define MHZ (CPU_FREQ/1000000)
	const int COUNT = 10; // odd values for accurate median
	int cycles[COUNT], instrs[COUNT];

	// Kernel stats - read irq latency
	const unsigned long irq_instr = MZONE_CSRR(CSR_MHPMCOUNTER26);
	const unsigned long irq_cycle = MZONE_CSRR(CSR_MHPMCOUNTER27);
	MZONE_CSRR(CSR_MHPMCOUNTER31); // reset kernel stats

	for (int i=0; i<COUNT; i++){

		const unsigned long I1 = MZONE_CSRR(CSR_MINSTRET);
		const unsigned long C1 = MZONE_CSRR(CSR_MCYCLE);
		MZONE_YIELD();
		const unsigned long C2 = MZONE_CSRR(CSR_MCYCLE);
		const unsigned long I2 = MZONE_CSRR(CSR_MINSTRET);

		cycles[i] = C2>C1 ? C2-C1 : (2^32+C2)-C1;
		instrs[i] = I2>I1 ? I2-I1 : (2^32+I2)-I1;

	}

	// --------------------- Adjustments --------------------------
	MZONE_CSRR(CSR_MCYCLE); // prime cache for accurate reading
	const unsigned long ADJC1 = MZONE_CSRR(CSR_MCYCLE);
	const unsigned long ADJC2 = MZONE_CSRR(CSR_MCYCLE);
	const int ADJC = ADJC2-ADJC1; // ignore wrap around

	MZONE_CSRR(CSR_MCYCLE); // prime cache for accurate reading
	const unsigned long ADJI1 = MZONE_CSRR(CSR_MINSTRET);
								MZONE_CSRR(CSR_MCYCLE);
								MZONE_CSRR(CSR_MCYCLE);
	const unsigned long ADJI2 = MZONE_CSRR(CSR_MINSTRET);
	const int ADJI = ADJI2-ADJI1; // ignore wrap around

	for (int i=0; i<COUNT; i++)	{cycles[i] -= ADJC; instrs[i] -= ADJI;}
	// ------------------------------------------------------------

	int max_cycle = 0;
	for (int i=0; i<COUNT; i++)	max_cycle = (cycles[i] > max_cycle ? cycles[i] : max_cycle);
	char str[16]; sprintf(str, "%lu", max_cycle); const int col_len = strlen(str);

	for (int i=0; i<COUNT; i++)
		printf("%*d cycles %*d instr %*d us \n", col_len, cycles[i], col_len, instrs[i], col_len-1, cycles[i]/MHZ);

	qsort(cycles, COUNT, sizeof(int), cmpfunc);
	qsort(instrs, COUNT, sizeof(int), cmpfunc);

	printf("-----------------------------------------\n");
	int min = cycles[0], med = cycles[COUNT/2], max = cycles[COUNT-1];
	printf("cycles min/med/max = %d/%d/%d \n", min, med, max);
	    min = instrs[0], med = instrs[COUNT/2], max = instrs[COUNT-1];
	printf("instrs min/med/max = %d/%d/%d \n", min, med, max);
	printf("time   min/med/max = %d/%d/%d us \n", min/MHZ, med/MHZ, max/MHZ);

	// Kernel stats (#ifdef STATS)
	const unsigned long instr_min = MZONE_CSRR(CSR_MHPMCOUNTER28);
	const unsigned long instr_max = MZONE_CSRR(CSR_MHPMCOUNTER29);
	const unsigned long cycle_min = MZONE_CSRR(CSR_MHPMCOUNTER30);
	const unsigned long cycle_max = MZONE_CSRR(CSR_MHPMCOUNTER31); // reset
	if (instr_min>0){
		printf("\n");
		printf("Kernel\n");
		printf("-----------------------------------------\n");
		printf("cycles min/max = %lu/%lu \n", cycle_min, cycle_max);
		printf("instrs min/max = %lu/%lu \n", instr_min, instr_max);
		printf("time   min/max = %lu/%lu us\n", cycle_min/MHZ, cycle_max/MHZ);
		printf("-----------------------------------------\n");
		printf("irq lat cycles = %lu \n", irq_cycle);
		printf("irq lat instrs = %lu \n", irq_instr);
		printf("time           = %lu us\n", irq_cycle/MHZ);
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

	uint64_t pmpcfg=0x0;
#if __riscv_xlen==32
	uint32_t pmpcfg0; asm ( "csrr %0, pmpcfg0" : "=r"(pmpcfg0) );
	uint32_t pmpcfg1; asm ( "csrr %0, pmpcfg1" : "=r"(pmpcfg1) );
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

		if (MZONE_RECV(zone, msg)) {

			if (strcmp("ping", msg) == 0)
				MZONE_SEND(zone, "pong");

			else {
				write(1, "\e7\e[2K", 6);   // save curs pos & clear entire line
				printf("\rZ%d > %.16s\n", zone, msg);
				write(1, "\nZ1 > ", 6); write(1, inputline, strlen(inputline));
				write(1, "\e8\e[2B", 6);   // restore curs pos & curs down 2x
			}
		}

	}

}

// ------------------------------------------------------------------------
void cmd_handler(){

	const char * tk[4] = { strtok(inputline, " "), strtok(NULL, " "), strtok(NULL, " "), strtok(NULL, " ")};

	if (tk[0] == NULL) tk[0] = "help";

	// --------------------------------------------------------------------
	if (strcmp(tk[0], "load")==0){
	// --------------------------------------------------------------------
		if (tk[1] != NULL){
			uint8_t data = 0x00;
			const unsigned long addr = strtoull(tk[1], NULL, 16);
			asm ("lbu %0, (%1)" : "+r"(data) : "r"(addr));
			printf("0x%08x : 0x%02x \n", (unsigned int)addr, data);
		} else printf("Syntax: load address \n");

	// --------------------------------------------------------------------
	} else if (strcmp(tk[0], "store")==0){
	// --------------------------------------------------------------------
		if (tk[1] != NULL && tk[2] != NULL){
			const uint32_t data = (uint32_t)strtoul(tk[2], NULL, 16);
			const unsigned long addr = strtoull(tk[1], NULL, 16);

			if ( strlen(tk[2]) <=2 )
				asm ( "sb %0, (%1)" : : "r"(data), "r"(addr));
			else if ( strlen(tk[2]) <=4 )
				asm ( "sh %0, (%1)" : : "r"(data), "r"(addr));
			else
				asm ( "sw %0, (%1)" : : "r"(data), "r"(addr));

			printf("0x%08x : 0x%02x \n", (unsigned int)addr, (unsigned int)data);
		} else printf("Syntax: store address data \n");

	// --------------------------------------------------------------------
	} else if (strcmp(tk[0], "exec")==0){
	// --------------------------------------------------------------------
		if (tk[1] != NULL){
			const unsigned long addr = strtoull(tk[1], NULL, 16);
			asm ( "jr (%0)" : : "r"(addr));
		} else printf("Syntax: exec address \n");

	// --------------------------------------------------------------------
	} else if (strcmp(tk[0], "dma")==0){
	// --------------------------------------------------------------------
		if (tk[1] != NULL && tk[2] != NULL && tk[3] != NULL){
			DMA_REG(DMA_TR_SRC_OFF)  = strtoull(tk[1], NULL, 16);
			DMA_REG(DMA_TR_DEST_OFF) = strtoull(tk[2], NULL, 16);
			DMA_REG(DMA_TR_SIZE_OFF) = strtoull(tk[3], NULL, 16);
			DMA_REG(DMA_CH_CTRL_OFF) = 0x1; // start transfer
		} else printf("Syntax: dma source dest size \n");

	// --------------------------------------------------------------------
	} else if (strcmp(tk[0], "send")==0){
	// --------------------------------------------------------------------
		if (tk[1] != NULL && tk[1][0]>='1' && tk[1][0]<='4' && tk[2] != NULL){
			char msg[16]; strncpy(msg, tk[2], 16);
			if (!MZONE_SEND( tk[1][0]-'0', msg) )
				printf("Error: Inbox full.\n");
		} else printf("Syntax: send {1|2|3|4} message \n");

	// --------------------------------------------------------------------
	} else if (strcmp(tk[0], "recv")==0){
	// --------------------------------------------------------------------
		if (tk[1] != NULL && tk[1][0]>='1' && tk[1][0]<='4'){
			char msg[16];
			if (MZONE_RECV(tk[1][0]-'0', msg))
				printf("msg : %.16s\n", msg);
			else
				printf("Error: Inbox empty.\n");
		} else printf("Syntax: recv {1|2|3|4} \n");

	// --------------------------------------------------------------------
	} else if (strcmp(tk[0], "yield")==0){
	// --------------------------------------------------------------------
		const unsigned long C1 = MZONE_CSRR(CSR_MCYCLE);
		MZONE_YIELD();
		const unsigned long C2 = MZONE_CSRR(CSR_MCYCLE);
		const unsigned long C0 = MZONE_CSRR(CSR_MCYCLE)-MZONE_CSRR(CSR_MCYCLE);
		const unsigned long C = C2-C1+C0;
		const int T = C/(CPU_FREQ/1000000);
		printf( (C>0 ? "yield : elapsed cycles %d / time %dus \n" : "yield : n/a \n"), C, T);

	// --------------------------------------------------------------------
	} else if (strcmp(tk[0], "timer")==0){
	// --------------------------------------------------------------------
		if (tk[1] != NULL){
			const uint64_t ms = abs(strtoull(tk[1], NULL, 10));
			const uint64_t T0 = MZONE_RDTIME();
			const uint64_t T1 = T0 + ms*RTC_FREQ/1000;
			MZONE_WRTIMECMP(T1); CSRS(mie, 1<<7);
			printf("timer set T0=%lu, T1=%lu \n", (unsigned long)(T0*1000/RTC_FREQ),
												  (unsigned long)(T1*1000/RTC_FREQ) );
		} else printf("Syntax: timer ms \n");

	// --------------------------------------------------------------------
	} else if (strcmp(tk[0], "stats")==0)	print_stats();
	// --------------------------------------------------------------------

	// --------------------------------------------------------------------
	else if (strcmp(tk[0], "restart")==0) asm ("j _start");
	// --------------------------------------------------------------------

	// --------------------------------------------------------------------
	else if (strcmp(tk[0], "pmp")==0) print_pmp();
	// --------------------------------------------------------------------

	// --------------------------------------------------------------------
	else if (strcmp(tk[0], "ecall")==0) asm ("ecall"); // test
	// --------------------------------------------------------------------

	// --------------------------------------------------------------------
	else if (strcmp(tk[0], "csrr")==0){ // test
	// --------------------------------------------------------------------
		//unsigned long C0 = MZONE_CSRR(CSR_MCYCLE);
		unsigned long C1 = MZONE_CSRR(CSR_MCYCLE);
		volatile unsigned long misa; asm volatile( "csrr %0, misa" : "=r"(misa) );
		unsigned long C2 = MZONE_CSRR(CSR_MCYCLE);
		printf( "csrr: cycles %d \n", (int)(C2-C1)); //-(int)(C1-C0));

	} else printf("Commands: yield send recv pmp load store exec dma stats timer restart \n");

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

	CSRW(mtvec, trap_handler);  // register trap handler
	CSRS(mie, 1<<11); 			// enable external interrupts (PLIC)
    CSRS(mstatus, 1<<3);		// enable global interrupts (PLIC, TMR)

	// Enable UART RX IRQ (PLIC)
    PLIC_REG(PLIC_PRI_OFFSET + (PLIC_UART_RX_SOURCE << PLIC_PRI_SHIFT_PER_SOURCE)) = 0x1;
	PLIC_REG(PLIC_EN_OFFSET) |= 1 << (PLIC_UART_RX_SOURCE & 0x1F);

	open("UART", 0, 0);

	printf("\e[2J\e[H"); // clear terminal screen

	printf("=====================================================================\n");
	printf("      	             Hex Five MultiZone® Security                    \n");
	printf("    Copyright© 2020 Hex Five Security, Inc. - All Rights Reserved    \n");
	printf("=====================================================================\n");
	printf("This version of MultiZone® Security is meant for evaluation purposes \n");
	printf("only. As such, use of this software is governed by the Evaluation    \n");
	printf("License. There may be other functional limitations as described in   \n");
	printf("the evaluation SDK documentation. The commercial version of the      \n");
	printf("software does not have these restrictions.                           \n");
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

		MZONE_WFI();

	}

}


