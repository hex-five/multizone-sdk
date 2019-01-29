/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <cli.h>
#include <robot.h>
#include <mzmsg.h>

#define PRINT_BUFFER_SIZE   128
static char print_buffer[PRINT_BUFFER_SIZE] = "";

static const char welcome_msg[] =
	"\e[2J\e[H" // clear terminal screen
	"======================================================================\r\n"
	"     	       Hex Five MultiZone(TM) Security v.0.1.1\r\n"
	"    Copyright (C) 2018 Hex Five Security Inc. All Rights Reserved\r\n"
	"======================================================================\r\n"
	" This version of MultiZone(TM) is meant for evaluation purposes only.\r\n"
	" As such, use of this software is governed by your Evaluation License.\r\n"
	" There may be other functional limitations as described in the\r\n"
	" evaluation kit documentation. The full version of the software does\r\n"
	" not have these restrictions.\r\n"
	"======================================================================\r\n"
;

static mzmsg_t zone2;

void restart(){
	ECALL_CSRC_MIE();
	mzmsg_reset(&zone2);
	asm ("j _start");
}

unsigned long handle_syncexception(unsigned long mcause, unsigned long mtval, unsigned long mepc){

	uint32_t rst = 0;

	switch(mcause){

        case 0x0: // Instruction address misaligned
            sprintf(print_buffer, "Instruction address misaligned : 0x%08x 0x%08x 0x%08x \r\n", mcause, mtval, mepc);
			rst = 1;
            break;
        case 0x1: // Instruction access fault
        	sprintf(print_buffer, "Instruction access fault : 0x%08x 0x%08x 0x%08x \r\n", mcause, mtval, mepc);
			rst = 1;
            break;
        case 0x2: // Illegal Instruction
            sprintf(print_buffer, "Illegal instruction : 0x%08x 0x%08x 0x%08x \r\n", mcause, mtval, mepc);
			rst = 1;
            break;
        case 0x4: // Load address misaligned
        	sprintf(print_buffer, "Load address misaligned : 0x%08x 0x%08x 0x%08x \r\n", mcause, mtval, mepc);   
            break; 
        case 0x5: // Load access fault
            sprintf(print_buffer, "Load access fault : 0x%08x 0x%08x 0x%08x \r\n", mcause, mtval, mepc);
            break;
        case 0x6: // Store/AMO address misaligned
            sprintf(print_buffer, "Store/AMO address misaligned : 0x%08x 0x%08x 0x%08x \r\n", mcause, mtval, mepc);
            break;
        case 0x7: // Store access fault
            sprintf(print_buffer, "Store access fault : 0x%08x 0x%08x 0x%08x \r\n", mcause, mtval, mepc);
            break;
        default:
			sprintf(print_buffer, "Unknown fault : 0x%08x 0x%08x 0x%08x \r\n", mcause, mtval, mepc);
			rst = 1;
            break;
    }

	mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
	
	if(rst){
		sprintf(print_buffer, "\r\nPress any key to restart");
		mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
		char c='\0'; while(mzmsg_read(&zone2, &c, 1) == 0);
		restart();
	}

	mepc += 4;
    return mepc;
}

// ------------------------------------------------------------------------
void print_cpu_info(void) {
// ------------------------------------------------------------------------

	// misa
	const uint64_t misa = ECALL_CSRR_MISA();

	const int xlen = ((misa >> __riscv_xlen-2)&0b11)==1 ?  32 :
					 ((misa >> __riscv_xlen-2)&0b11)==2 ?  64 :
					 ((misa >> __riscv_xlen-2)&0b11)==1 ? 128 : 0;

	char misa_str[26+1]="";
	for (int i=0, j=0; i<26; i++)
		if ( (misa & (1ul << i)) !=0){
			misa_str[j++]=(char)('A'+i); misa_str[j]='\0';
		}

	sprintf(print_buffer, "Machine ISA   : 0x%08x RV%d %s \r\n", (int)misa, xlen, misa_str);
	mzmsg_write(&zone2, print_buffer, strlen(print_buffer));

	// mvendorid
	const uint64_t mvendorid = ECALL_CSRR_MVENDID();
	char *mvendorid_str = (mvendorid==0x10e31913 ? "SiFive, Inc.\0" : "Unknown\0");
	sprintf(print_buffer, "Vendor        : 0x%08x %s \r\n", (int)mvendorid, mvendorid_str);
	mzmsg_write(&zone2, print_buffer, strlen(print_buffer));

	// marchid
	const uint64_t marchid = ECALL_CSRR_MARCHID();
	sprintf(print_buffer, "Architecture  : 0x%08x \r\n", (int)marchid );
	mzmsg_write(&zone2, print_buffer, strlen(print_buffer));

	// mimpid
	const uint64_t mimpid = ECALL_CSRR_MIMPID();
	sprintf(print_buffer, "Implementation: 0x%08x \r\n", (int)mimpid );
	mzmsg_write(&zone2, print_buffer, strlen(print_buffer));

	// mhartid
	const uint64_t mhartid = ECALL_CSRR_MHARTID();
	sprintf(print_buffer, "Hart ID       : 0x%08x \r\n", (int)mhartid );
	mzmsg_write(&zone2, print_buffer, strlen(print_buffer));

	// CPU Clock
	const int cpu_clk = round(CPU_FREQ/1E+6);
	sprintf(print_buffer, "CPU clock     : %d MHz \r\n", cpu_clk );
	mzmsg_write(&zone2, print_buffer, strlen(print_buffer));

} // print_cpu_info()

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
		for (int i=0; i<COUNT; i++){
			sprintf(print_buffer, "%*d cycles in %*d us \r\n", max_col, cycles[i], max_col-2, (int)(cycles[i]*1000/MHZ));
			mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
		}

		qsort(cycles, COUNT, sizeof(int), cmpfunc);

		sprintf(print_buffer, "------------------------------------------------\r\n");
		mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
		int min = cycles[0], med = cycles[COUNT/2], max = cycles[COUNT-1];
		sprintf(print_buffer, "cycles  min/med/max = %d/%d/%d \r\n", min, med, max);
		mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
		sprintf(print_buffer, "time    min/med/max = %d/%d/%d us \r\n", (int)min*1000/MHZ, (int)med*1000/MHZ, (int)max*1000/MHZ);
		mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
	}

	if (ctxsw_instr[0]>0 && cycles[0]>0){

		qsort(ctxsw_cycle, COUNT, sizeof(int), cmpfunc);
		qsort(ctxsw_instr, COUNT, sizeof(int), cmpfunc);

		sprintf(print_buffer, "\r\n");
		mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
		int min = ctxsw_instr[0], med = ctxsw_instr[COUNT/2], max = ctxsw_instr[COUNT-1];
		sprintf(print_buffer, "ctx sw instr  min/med/max = %d/%d/%d \r\n", min, med, max);
		mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
		min = ctxsw_cycle[0], med = ctxsw_cycle[COUNT/2], max = ctxsw_cycle[COUNT-1];
		sprintf(print_buffer, "ctx sw cycles min/med/max = %d/%d/%d \r\n", min, med, max);
		mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
		sprintf(print_buffer, "ctx sw time   min/med/max = %d/%d/%d us \r\n", (int)min*1000/MHZ, (int)med*1000/MHZ, (int)max*1000/MHZ);
		mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
	} else if (ctxsw_instr[0]>0 && cycles[0]==0){

		sprintf(print_buffer, "ctx sw instr  = %d \r\n", ctxsw_instr[0]);
		mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
		sprintf(print_buffer, "ctx sw cycles = %d \r\n", ctxsw_cycle[0]);
		mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
		sprintf(print_buffer, "ctx sw time   = %d us \r\n", (int)ctxsw_cycle[0]*1000/MHZ);
		mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
	}

	if (cycles[0]==0 && ctxsw_instr[0]==0){
		sprintf(print_buffer, "stats : n/a \r\n");
		mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
	}

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
		sprintf(print_buffer, "0x%08x 0x%08x %s \r\n", (unsigned int)start, (unsigned int)end, rwx);
#else
		sprintf(print_buffer, "0x%08" PRIX64 " 0x%08" PRIX64 " %s \r\n", start, end, rwx);
#endif
        mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
	}

} // print_pmpcfg()


#define CMD_LINE_SIZE 128
static char history[CMD_LINE_SIZE+1]="";
// ------------------------------------------------------------------------
 int readline(char *cmd_line) {

	int p=0;
	char c='\0';
	int esc=0;
	cmd_line[0] = '\0';


	while(c!='\r'){

		if ( mzmsg_read(&zone2, &c, 1) >0 ) {

			if (c=='\e'){
				esc=1;

			} else if (esc==1 && c=='['){
				esc=2;

			} else if (esc==2 && c=='3'){
				esc=3;

			} else if (esc==3 && c=='~'){ // del key
				for (int i=p; i<strlen(cmd_line); i++) cmd_line[i]=cmd_line[i+1];
				esc=0;

			} else if (esc==2 && c=='C'){ // right arrow
				esc=0;
				if (p < strlen(cmd_line)){
					p++;
					mzmsg_write(&zone2, "\e[C", 3);
				}

			} else if (esc==2 && c=='D'){ // left arrow
				esc=0;
				if (p>0){
					p--;
					mzmsg_write(&zone2, "\e[D", 3);
				}

			} else if (esc==2 && c=='A'){ // up arrow
				esc=0;
				if (strlen(history)>0){
					p=strlen(history);
					strcpy(cmd_line, history);
					mzmsg_write(&zone2, "\e[2K", 4); // 2K clear entire line - cur pos dosn't change
					mzmsg_write(&zone2, "\rZ1 > ", 6);
					mzmsg_write(&zone2, &cmd_line[0], strlen(cmd_line));
				}

			} else if (esc==2 && c=='B'){ // down arrow
				esc=0;

			} else if (c == 0x7f){ // backspace
				if(p > 0){
					p--;
					for (int i=p; i<strlen(cmd_line); i++) cmd_line[i]=cmd_line[i+1];

					mzmsg_write(&zone2, "\b \b", 3);
				}

			} else if (c>=' ' && c<='~' && p < CMD_LINE_SIZE && esc==0){
				for (int i = CMD_LINE_SIZE-1; i > p; i--) cmd_line[i]=cmd_line[i-1]; // make room for 1 ch
				cmd_line[p]=c;
                p++;
                mzmsg_write(&zone2, &c, 1);
			} else{
				esc=0;
            }
		}

		ECALL_YIELD();

	} // while(1)

	for (int i = CMD_LINE_SIZE-1; i > 0; i--)
		if (cmd_line[i]==' ') cmd_line[i]='\0';	else break;

	if (strlen(cmd_line)>0)
		strcpy(history, cmd_line);

	return strlen(cmd_line);

} // readline()

void cliTask( void *pvParameters){

    char c = 0;
	mzmsg_init(&zone2, 2);

    mzmsg_write(&zone2, welcome_msg, sizeof(welcome_msg));
    print_cpu_info();
	mzmsg_write(&zone2, "\r\nFreeRTOS CLI\r\n",16);

    char cmd_line[CMD_LINE_SIZE+1]="";
	int msg[4]={0,0,0,0};

    while(1){

	    mzmsg_write(&zone2, "\r\nZ1 > ", 7);
        readline(cmd_line);
        mzmsg_write(&zone2, "\r\n", 2);

		char * tk1 = strtok (cmd_line, " ");
		char * tk2 = strtok (NULL, " ");
		char * tk3 = strtok (NULL, " ");

		if (tk1 != NULL && strcmp(tk1, "pmp")==0){
            print_pmp_ranges();
        } else if(tk1 != NULL && strcmp(tk1, "robot")==0){
			char c = (char) *tk2;
			xQueueSend( robot_queue, &c, 0 );
		} else if (tk1 != NULL && strcmp(tk1, "load")==0){
			if (tk2 != NULL){
				uint8_t data = 0x00;
				const uint64_t addr = strtoull(tk2, NULL, 16);
				asm ("lbu %0, (%1)" : "+r"(data) : "r"(addr));
				sprintf(print_buffer, "0x%08x : 0x%02x \r\n", (unsigned int)addr, data);
				mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
			} else {
				sprintf(print_buffer, "Syntax: load address \r\n");
				mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
			}
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

				sprintf(print_buffer, "0x%08x : 0x%02x \r\n", (unsigned int)addr, (unsigned int)data);
				mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
			} else { 
				sprintf(print_buffer, "Syntax: store address data \r\n");
				mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
			} 

		} else if (tk1 != NULL && strcmp(tk1, "send")==0){
			if (tk2 != NULL && tk2[0]>='1' && tk2[0]<='4' && tk3 != NULL){
				msg[0]=(unsigned int)*tk3; msg[1]=0; msg[2]=0; msg[3]=0;
				ECALL_SEND(tk2[0]-'0', msg);
			} else {
				sprintf(print_buffer, "Syntax: send {1|2|3|4} message \r\n");
				mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
			}
		} else if (tk1 != NULL && strcmp(tk1, "recv")==0){
			if (tk2 != NULL && tk2[0]>='1' && tk2[0]<='4'){
				ECALL_RECV(tk2[0]-'0', msg);
				sprintf(print_buffer, "msg : 0x%08x 0x%08x 0x%08x 0x%08x \r\n", msg[0], msg[1], msg[2], msg[3]);
				mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
			} else {
				sprintf(print_buffer, "Syntax: recv {1|2|3|4} \r\n");
				mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
			}
		} else if (tk1 != NULL && strcmp(tk1, "yield")==0){
			uint64_t C1 = ECALL_CSRR_MCYCLE();
			ECALL_YIELD();
			uint64_t C2 = ECALL_CSRR_MCYCLE();
			const int T = ((C2-C1)*1000000)/CPU_FREQ;
			sprintf(print_buffer, (T>0 ? "yield : elapsed time %dus \r\n" : "yield : n/a \r\n"), T);
			mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
		} else if (tk1 != NULL && strcmp(tk1, "stats")==0){
			print_stats();
		} else if (tk1 != NULL && strcmp(tk1, "restart")==0){
			restart();
		} else if (tk1 != NULL && strcmp(tk1, "exec")==0){
			if (tk2 != NULL){
				const uint64_t addr = strtoull(tk2, NULL, 16);
			    asm ( "jr (%0)" : : "r"(addr));
			} else {
				sprintf(print_buffer, "Syntax: exec address \n");
				mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
			}
		} else {
			sprintf(print_buffer,
				"Commands: load store exec send recv yield pmp robot stats restart\n");
			mzmsg_write(&zone2, print_buffer, strlen(print_buffer));
		}

		taskYIELD();
    }
}
