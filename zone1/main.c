/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <fcntl.h>      // open()
#include <unistd.h>     // read() write()
#include <string.h>     // strxxx()
#include "printf.h"     //#include <stdio.h>    // printf() sprintf()
#include <stdlib.h>     // qsort() strtoul()
#include <limits.h>     // UINT_MAX ULONG_MAX

#include "platform.h"
#include "multizone.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

typedef enum {zone1=1, zone2, zone3, zone4} Zone;

#define BUFFER_SIZE 32
static volatile struct{
	char data[BUFFER_SIZE];
	int r; // read
	int w; // write
} buffer;
int buffer_empty(void){
	return (buffer.w==0);
}

static char inputline[BUFFER_SIZE+1]="";

static volatile char inbox[4][16] = { {'\0'}, {'\0'}, {'\0'}, {'\0'} };
int inbox_empty(void){
	return (inbox[0][0]=='\0' && inbox[1][0]=='\0' && inbox[2][0]=='\0' && inbox[3][0]=='\0');
}

// ------------------------------------------------------------------------
#ifdef E21
    static void (*trap_vect[173])(void) = {};
#else
    static void (*trap_vect[__riscv_xlen])(void) = {};
#endif
__attribute__((interrupt())) void trp_isr(void)  { // nmi traps (0)

	const unsigned long mcause = MZONE_CSRR(CSR_MCAUSE);
	const unsigned long mepc   = MZONE_CSRR(CSR_MEPC);
	const unsigned long mtval  = MZONE_CSRR(CSR_MTVAL);

    switch(mcause){

    case 0 : printf("Instruction address misaligned : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
             break;

    case 1 : printf("Instruction access fault : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
             break;

    case 2 : printf("Illegal instruction : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
             break;

    case 3 : printf("Breakpoint : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
             break;

    case 4 : printf("Load address misaligned : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
             CSRW(mepc, mepc + (((*(char *)mepc) & 0b11) == 0b11 ? 4 : 2)); // skip faulty instruction
             return;

    case 5 : printf("Load access fault : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
             CSRW(mepc, mepc + (((*(char *)mepc) & 0b11) == 0b11 ? 4 : 2)); // skip faulty instruction
             return;

    case 6 : printf("Store/AMO address missligned : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
             CSRW(mepc, mepc + (((*(char *)mepc) & 0b11) == 0b11 ? 4 : 2)); // skip faulty instruction
             return;

    case 7 : printf("Store access fault : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
             CSRW(mepc, mepc + (((*(char *)mepc) & 0b11) == 0b11 ? 4 : 2)); // skip faulty instruction
             return;

    case 8 : printf("Environment call from U-mode : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
             break;

    case 9 : printf("Environment call from S-mode : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
             break;

    case 11: printf("Environment call from M-mode : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
             break;

    default : printf("Exception : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);

    }

    printf("Press any key to restart \n");
    char c='\0'; while(read(0, &c, 1) == 0 ){;}
    asm ("j _start");

}
__attribute__((interrupt())) void msi_isr(void)  { // msip/inbox (3)

    for (Zone zone = zone1; zone <= zone4; zone++) {
        char msg[16];
        if (MZONE_RECV(zone, msg))
            memcpy((char*) &inbox[zone-1][0], msg, sizeof inbox[0]);
    }

}
__attribute__((interrupt())) void tmr_isr(void)  { // timer (7)

    const unsigned long mcause = MZONE_CSRR(CSR_MCAUSE);
    const unsigned long mepc   = MZONE_CSRR(CSR_MEPC);
    const unsigned long mtval  = MZONE_CSRR(CSR_MTVAL);

    write(1, "\e7\e[2K", 6);   	// save curs pos & clear entire line
    printf("\rTimer interrupt : 0x%08x 0x%08x 0x%08x \n", mcause, mepc, mtval);
    write(1, "\nZ1 > %s", 6); write(1, inputline, strlen(inputline));
    write(1, "\e8\e[2B", 6);   	// restore curs pos & curs down +2 lines
    CSRC(mie, 1<<7); 			// disable one-shot timer

}
__attribute__((interrupt())) void uart_isr(void) { // uart

#ifdef PLIC_BASE
    const uint32_t plic_int = PLIC_REG(PLIC_CLAIM); // PLIC claim
#endif
    char temp[8]; int count = read(0, &temp, sizeof temp);
    if (count > 0){
        //if(buffer.w==BUFFER_SIZE){write(1, "\n>>> BUFFER FULL !!!\n", 21); while(1);}
        count = MIN(count, BUFFER_SIZE - buffer.w);
        memcpy((char*) &buffer.data[buffer.w], temp, count);
        buffer.w += count;
    }
#ifdef PLIC_BASE
    PLIC_REG(PLIC_CLAIM) = plic_int; // PLIC complete
#endif

}
__attribute__((interrupt())) void dma_isr(void)  { // dma

#ifdef DMA_BASE
    write(1, "\e7\e[2K", 6);    // save curs pos & clear entire line
    printf("\rDMA transfer complete \n");
    printf("source : 0x%08x \n", DMA_REG(DMA_TR_SRC_OFF));
    printf("dest   : 0x%08x \n", DMA_REG(DMA_TR_DEST_OFF));
    printf("size   : 0x%08x \n", DMA_REG(DMA_TR_SIZE_OFF));
    write(1, "\e8\e[4B", 6);    // restore curs pos & curs down 4x

    // clear irq's by writing 1’s (R/W1C)
    DMA_REG(DMA_CH_STATUS_OFF) = (1<<16 | 1<<8 | 1<<0);
#endif

}

// ------------------------------------------------------------------------
void print_sys_info(void) {
// ------------------------------------------------------------------------

	// misa
	unsigned long misa;	asm volatile ("csrr %0, misa" : "=r"(misa) : );
//	const unsigned long misa = CSRR(misa);
	const int xlen = ((misa >> (__riscv_xlen-2))&0b11)==1 ?  32 :
					 ((misa >> (__riscv_xlen-2))&0b11)==2 ?  64 :
					 ((misa >> (__riscv_xlen-2))&0b11)==1 ? 128 :
							 	 	 	 	 	 	 	 	0 ;
	char misa_str[26+1]="";
	for (int i=0, j=0; i<26; i++)
		if ( (misa & (1ul << i)) !=0){
			misa_str[j++]=(char)('A'+i); misa_str[j]='\0';
		}
	printf("Machine ISA   : 0x%08x RV%d %s \n", (int)misa, xlen, misa_str);

	// mvendid
	const unsigned long  mvendorid = CSRR(mvendorid);
	const char *mvendorid_str = mvendorid==0x489      ? "SiFive, Inc." :
								mvendorid==0x31e      ? "Andes Technology" :
							    mvendorid==0x57c      ? "Hex Five, Inc." :
							                            "";
	printf("Vendor        : 0x%08x %s \n", (int)mvendorid, mvendorid_str);

	// marchid
	const unsigned long  marchid = CSRR(marchid);
	const char *marchid_str = (mvendorid==0x489 && marchid==0x80000002 ? "2-Series (E2, S2)" :
							   mvendorid==0x489 && marchid==0x00000001 ? "3/5-Series (E3, S5, U5)" :
						       mvendorid==0x57c && marchid==0x00000001 ? "X300" :
							   mvendorid==0x31e && marchid==0x80000022 ? "N22" :
							                                             "");
	printf("Architecture  : 0x%08x %s \n", (int)marchid, marchid_str);

	// mimpid
	printf("Implementation: 0x%08x \n", CSRR(mimpid) );

	// mhartid
	printf("Hart id       : 0x%1x \n", CSRR(mhartid) );

	// CPU clk
	printf("CPU clock     : %d MHz \n", (int)(CPU_FREQ/1E+6) );

	// RTC clk
	if (RTC_FREQ < 1E+6)
	printf("RTC clock     : %d KHz \n", (int)(RTC_FREQ/1E+3) );
	else
		printf("RTC clock     : %d MHz \n", (int)(RTC_FREQ/1E+6) );

	// Platform info

	printf(" \n");

#ifdef PLIC_BASE
	printf("PLIC @0x%08x \n", PLIC_BASE);
#endif
#ifdef CLIC_BASE
	printf("CLIC @0x%08x \n", CLIC_BASE);
#endif
#ifdef DMA_BASE
	printf("DMAC @0x%08x \n", DMA_BASE);
#endif
#ifdef UART_BASE
	printf("UART @0x%08x \n", UART_BASE);
#endif
#ifdef GPIO_BASE
	printf("GPIO @0x%08x \n", GPIO_BASE);
#endif

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
	const int COUNT = 11; // odd values for accurate median
	int cycles[COUNT], instrs[COUNT];

	for (int i=0; i<COUNT; i++){

		const unsigned long I1 = MZONE_CSRR(CSR_MINSTRET);
		const unsigned long C1 = MZONE_CSRR(CSR_MCYCLE);
		MZONE_YIELD();
		const unsigned long I2 = MZONE_CSRR(CSR_MINSTRET);
		const unsigned long C2 = MZONE_CSRR(CSR_MCYCLE);

		cycles[i] = C2-C1; instrs[i] = I2-I1;

	}

	int max_cycle = 0;
	for (int i=0; i<COUNT; i++)	max_cycle = (cycles[i] > max_cycle ? cycles[i] : max_cycle);
	char str[16]; sprintf(str, "%d", max_cycle); const int col_len = strlen(str);

	for (int i=0; i<COUNT; i++)
		printf("%*d instr %*d cycles %*d us \n", col_len, instrs[i], col_len, cycles[i], col_len, cycles[i]/MHZ);

	qsort(cycles, COUNT, sizeof(int), cmpfunc);
	qsort(instrs, COUNT, sizeof(int), cmpfunc);

	printf("-----------------------------------------\n");
    int min = instrs[0], med = instrs[COUNT/2], max = instrs[COUNT-1];
    printf("instrs min/med/max = %d/%d/%d \n", min, med, max);
	min = cycles[0], med = cycles[COUNT/2], max = cycles[COUNT-1];
	printf("cycles min/med/max = %d/%d/%d \n", min, med, max);
	printf("time   min/med/max = %d/%d/%d us \n", min/MHZ, med/MHZ, max/MHZ);

	// Kernel stats (#ifdef STATS)
	const unsigned irq_instr_min = (unsigned)(MZONE_CSRR(CSR_MHPMCOUNTER26) & 0xFFFF);
	const unsigned irq_instr_max = (unsigned)(MZONE_CSRR(CSR_MHPMCOUNTER26) >>16);
	const unsigned irq_cycle_min = (unsigned)(MZONE_CSRR(CSR_MHPMCOUNTER27) & 0xFFFF);
	const unsigned irq_cycle_max = (unsigned)(MZONE_CSRR(CSR_MHPMCOUNTER27) >>16);
	const unsigned long instr_min = MZONE_CSRR(CSR_MHPMCOUNTER28);
	const unsigned long instr_max = MZONE_CSRR(CSR_MHPMCOUNTER29);
	const unsigned long cycle_min = MZONE_CSRR(CSR_MHPMCOUNTER30);
	const unsigned long cycle_max = MZONE_CSRR(CSR_MHPMCOUNTER31); // <= resets kern stats

	if (instr_min>0){

		printf("\n");
		printf("Kernel time\n");
		printf("-----------------------------------------\n");
		printf("instrs min/max = %lu/%lu \n", instr_min, instr_max);
		printf("cycles min/max = %lu/%lu \n", cycle_min, cycle_max);
		printf("time   min/max = %lu/%lu us\n", cycle_min/MHZ, cycle_max/MHZ);

		printf("\n");
		printf("IRQ latency\n");
		printf("-----------------------------------------\n");
		printf("instrs min/max = %d/%d \n", irq_instr_min, irq_instr_max);
		printf("cycles min/max = %d/%d \n", irq_cycle_min, irq_cycle_max);
		printf("time   min/max = %d/%d us\n", irq_cycle_min/MHZ, irq_cycle_max/MHZ);

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

	unsigned long pmpaddr[8];
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

		unsigned long start=0, end=0;

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

		printf("0x%08x 0x%08x %s %s \n", (unsigned)start, (unsigned)end, rwx, type);

	}

}

// ------------------------------------------------------------------------
void msg_handler() {

    CSRC(mie, 1 << 3);

    for (Zone zone = zone1; zone <= zone4; zone++) {

        char * const msg = (char *)inbox[zone-1];

        if (*msg != '\0') {

            if (strcmp("ping", msg) == 0) {
                MZONE_SEND(zone, msg);

            } else {
                write(1, "\e7\e[2K", 6);   // save curs pos & clear entire line
                printf("\rZ%d > %.16s\n", zone, msg);
                write(1, "\nZ1 > ", 6);
                write(1, inputline, strlen(inputline));
                write(1, "\e8\e[2B", 6);   // restore curs pos & curs down 2x
            }

            *msg = '\0';

        }

    }

    CSRS(mie, 1 << 3);

}

// ------------------------------------------------------------------------
void cmd_handler(){

	char * tk[9]; tk[0] = strtok(inputline, " "); for (int i=1; i<9; i++) tk[i] = strtok(NULL, " ");

	if (tk[0] == NULL) tk[0] = "help";

	// --------------------------------------------------------------------
	if (strcmp(tk[0], "load")==0){
	// --------------------------------------------------------------------
		if (tk[1] != NULL){
			uint8_t data = 0x00;
			const unsigned long addr = strtoul(tk[1], NULL, 16);
			asm ("lbu %0, (%1)" : "+r"(data) : "r"(addr));
			printf("0x%08x : 0x%02x \n", (unsigned int)addr, data);
		} else printf("Syntax: load address \n");

	// --------------------------------------------------------------------
	} else if (strcmp(tk[0], "store")==0){
	// --------------------------------------------------------------------
		if (tk[1] != NULL && tk[2] != NULL){
			const uint32_t data = (uint32_t)strtoul(tk[2], NULL, 16);
			const unsigned long addr = strtoul(tk[1], NULL, 16);

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
			const unsigned long addr = strtoul(tk[1], NULL, 16);
			asm ( "jr (%0)" : : "r"(addr));
		} else printf("Syntax: exec address \n");

#ifdef DMA_BASE
	// --------------------------------------------------------------------
	} else if (strcmp(tk[0], "dma")==0){
	// --------------------------------------------------------------------
		if (tk[1] != NULL && tk[2] != NULL && tk[3] != NULL){
			DMA_REG(DMA_TR_SRC_OFF)  = strtoul(tk[1], NULL, 16);
			DMA_REG(DMA_TR_DEST_OFF) = strtoul(tk[2], NULL, 16);
			DMA_REG(DMA_TR_SIZE_OFF) = strtoul(tk[3], NULL, 16);
			DMA_REG(DMA_CH_CTRL_OFF) = 0b0001; // en irqs & start transfer
		} else printf("Syntax: dma source dest size \n");
#endif

	// --------------------------------------------------------------------
	} else if (strcmp(tk[0], "send")==0){
	// --------------------------------------------------------------------
		if (tk[1] != NULL && tk[1][0]>='1' && tk[1][0]<='4' && tk[2] != NULL){
			char msg[16]; strncpy(msg, tk[2], (sizeof msg)-1);
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
		const unsigned long C = C2-C1;
		const int T = C/(CPU_FREQ/1000000);
		printf( (C>0 ? "yield : elapsed cycles %d / time %dus \n" : "yield : n/a \n"), (int)C, T);

	// --------------------------------------------------------------------
	} else if (strcmp(tk[0], "timer")==0){
	// --------------------------------------------------------------------
		if (tk[1] != NULL){
			const uint64_t ms = strtoull(tk[1], NULL, 10);
			const uint64_t T0 = MZONE_RDTIME();
			const uint64_t T1 = T0 + ms*RTC_FREQ/1000;
			printf("timer set T0=%lu, T1=%lu \n", (unsigned long)(T0*1000/RTC_FREQ),
												  (unsigned long)(T1*1000/RTC_FREQ) );
			MZONE_WRTIMECMP(T1); CSRS(mie, 1<<7); // one-shot timer
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
		volatile unsigned long regval;
		unsigned long C0 = MZONE_CSRR(CSR_MCYCLE);
		  //asm volatile( "csrr %0, ustatus" : "=r"(regval) ); // illegal
		    asm volatile( "csrr %0, mip" : "=r"(regval) );
		  //regval = CSRR(mip);
		  //regval = MZONE_CSRR(CSR_MIP);
		unsigned long C1 = MZONE_CSRR(CSR_MCYCLE);
		printf( "0x%08x (%d cycles) \n", (unsigned)regval, (int)(C1-C0) );

	} else {
		printf("Commands: yield send recv pmp load store exec stats timer restart ");
#ifdef DMA_BASE
		printf("dma ");
#endif
		printf("\n");
	}

}

// ------------------------------------------------------------------------
int readline() {
// ------------------------------------------------------------------------

	static size_t p=0;
	static int esc=0;
	static char history[8][sizeof(inputline)]={"","","","","","","",""}; static int h=-1;

	int eol = 0; // end of line
	
		while ( !eol && buffer.w > buffer.r ) {

		CSRC(mstatus, 1<<3); // CSRC(mie, 1<<11); // PLIC_REG(PLIC_EN) &= ~(1 << PLIC_SRC_UART);
			const char c = buffer.data[buffer.r++];
			if (buffer.r >= buffer.w) {buffer.r = 0; buffer.w = 0;}
		CSRS(mstatus, 1<<3); // CSRS(mie, 1<<11); //PLIC_REG(PLIC_EN) |= 1 << PLIC_SRC_UART;

		if (c=='\e'){
			esc=1;

		} else if (esc==1 && c=='['){
			esc=2;

		} else if (esc==2 && c=='3'){
			esc=3;

		} else if (esc==3 && c=='~'){ // del key
			for (size_t i=p; i<strlen(inputline); i++) inputline[i]=inputline[i+1];
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
			for (size_t i=p; i<strlen(inputline); i++) inputline[i]=inputline[i+1];
			write(1, "\e[D", 3);
			write(1, "\e7", 2);
			write(1, "\e[K", 3);
			write(1, &inputline[p], strlen(inputline)-p);
			write(1, "\e8", 2);

		} else if (c>=' ' && c<='~' && p < sizeof(inputline)-1 && esc==0){
			for (size_t i = sizeof(inputline)-1-1; i > p; i--) inputline[i]=inputline[i-1]; // make room for 1 ch
			inputline[p]=c;
			write(1, "\e7", 2); // save curs pos
			write(1, "\e[K", 3); // clear line from curs pos
			write(1, &inputline[p], strlen(inputline)-p); p++;
			write(1, "\e8", 2); // restore curs pos
			write(1, "\e[C", 3); // move curs right 1 pos

		} else if (c=='\r') {
			p=0; esc=0; eol = 1;
			// trim
			while (inputline[strlen(inputline)-1]==' ') inputline[strlen(inputline)-1]='\0';
			while (inputline[0]==' ') for (size_t i = 0; i < strlen(inputline); i++) inputline[i]=inputline[i+1];
			// save line to history
			if (strlen(inputline)>0 && strcmp(inputline, history[0])!=0){
				for (int i = 8-1; i > 0; i--) strcpy(history[i], history[i-1]);
				strcpy(history[0], inputline);
			} h = -1;
			write(1, "\n", 1);

		} else
			esc=0;

	}

	return eol;

}

// ------------------------------------------------------------------------
int main (void) {

	//while(1) MZONE_WFI();
	//while(1) MZONE_YIELD();
	//while(1);

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

    print_sys_info();

    // setup vectored trap handler
    trap_vect[0] = trp_isr;
    trap_vect[3] = msi_isr;
    trap_vect[7] = tmr_isr;
    trap_vect[DMA_IRQ] = dma_isr;
    trap_vect[UART_IRQ] = uart_isr;

    // register trap vector
    CSRW(mtvec, trap_vect); CSRS(mtvec, 1);

    // enable interrupt sources
    CSRS(mie, 1<<3);
#ifdef PLIC_BASE
    CSRS(mie, 1L<<DMA_IRQ);
    CSRS(mie, 1<<11);
    PLIC_REG(PLIC_PRI + (PLIC_SRC_UART << PLIC_SHIFT_PER_SRC)) = 1;
    PLIC_REG(PLIC_EN) |= 1 << PLIC_SRC_UART;
#elif CLIC_BASE
    *(volatile uint8_t *)(CLIC_BASE + CLIC_INT_ENABLE + DMA_IRQ)  = 0x1;
    *(volatile uint8_t *)(CLIC_BASE + CLIC_INT_ENABLE + UART_IRQ) = 0x1;
#endif

    // enable global interrupt
    CSRS(mstatus, 1<<3);

	write(1, "\n\rZ1 > ", 7);

    while(1){

    	// UART RX event handler
		if (readline()){
			cmd_handler();
			write(1, "\n\rZ1 > ", 7);
			inputline[0]='\0';
		}

    	// Inbox event handler
		msg_handler();

		if (buffer_empty() && inbox_empty())
		    MZONE_WFI();
		else
			MZONE_YIELD();
	}

}


