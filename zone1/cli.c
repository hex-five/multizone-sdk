/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <cli.h>
#include <robot.h>

#define PRINT_BUFFER_SIZE   64
static char print_buffer[PRINT_BUFFER_SIZE] = "";
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
		sprintf(print_buffer,"\r0x%08x 0x%08x %s \n", (unsigned int)start, (unsigned int)end, rwx);
#else
		sprintf(print_buffer"\r0x%08" PRIX64 " 0x%08" PRIX64 " %s \n", start, end, rwx);
#endif
        mzmsg_write(print_buffer, strlen(print_buffer));
	}

} // print_pmpcfg()


	#define CMD_LINE_SIZE 32
static char history[CMD_LINE_SIZE+1]="";
// ------------------------------------------------------------------------
 int readline(char *cmd_line) {

	int p=0;
	char c='\0';
	int esc=0;
	cmd_line[0] = '\0';


	while(c!='\r'){

		if ( mzmsg_read( &c, 1) >0 ) {

			if (c=='\e'){
				esc=1;

			} else if (esc==1 && c=='['){
				esc=2;

			} else if (esc==2 && c=='3'){
				esc=3;

			} else if (esc==3 && c=='~'){ // del key
				for (int i=p; i<strlen(cmd_line); i++) cmd_line[i]=cmd_line[i+1];
				// mzmsg_write( "\e7", 2); // save curs pos
				// mzmsg_write( "\e[K", 3); // clear line from curs pos
				// mzmsg_write( &cmd_line[p], strlen(cmd_line)-p);
				// mzmsg_write( "\e8", 2); // restore curs pos
				esc=0;

			} else if (esc==2 && c=='C'){ // right arrow
				esc=0;
				if (p < strlen(cmd_line)){
					p++;
					mzmsg_write( "\e[C", 3);
				}

			} else if (esc==2 && c=='D'){ // left arrow
				esc=0;
				if (p>0){
					p--;
					mzmsg_write( "\e[D", 3);
				}

			} else if (esc==2 && c=='A'){ // up arrow
				esc=0;
				if (strlen(history)>0){
					p=strlen(history);
					strcpy(cmd_line, history);
					mzmsg_write( "\e[2K", 4); // 2K clear entire line - cur pos dosn't change
					mzmsg_write( "\rZ1 > ", 6);
					mzmsg_write( &cmd_line[0], strlen(cmd_line));
				}

			} else if (esc==2 && c=='B'){ // down arrow
				esc=0;

			} else if (c == 0x7f){ // backspace
				p--;
				for (int i=p; i<strlen(cmd_line); i++) cmd_line[i]=cmd_line[i+1];
                // mzmsg_write("\e[D", 3);
				// mzmsg_write("\e7", 2);
				// mzmsg_write("\e[K", 3);
				// mzmsg_write(&cmd_line[p], strlen(cmd_line)-p);
				// mzmsg_write("\e8", 2);
                mzmsg_write("\b \b", 3);

			} else if (c>=' ' && c<='~' && p < CMD_LINE_SIZE && esc==0){
				for (int i = CMD_LINE_SIZE-1; i > p; i--) cmd_line[i]=cmd_line[i-1]; // make room for 1 ch
				cmd_line[p]=c;
                // mzmsg_write("\e7", 2); // save curs pos
				// mzmsg_write("\e[K", 3); // clear line from curs pos
				// mzmsg_write(&cmd_line[p], strlen(cmd_line)-p); p++;
				// mzmsg_write("\e8", 2); // restore curs pos
				// mzmsg_write("\e[C", 3); // move curs right 1 pos
                p++;
                mzmsg_write(&c, 1);
			} else{
 //               printf("Not recognized char 0x%x\n", c);
				esc=0;
            }
		}

		ECALL_YIELD();

	} // while(1)

	for (int i = CMD_LINE_SIZE-1; i > 0; i--)
		if (cmd_line[i]==' ') cmd_line[i]='\0';	else break;

	if (strlen(cmd_line)>0)
		strcpy(history, cmd_line);

//    printf("%s\n", cmd_line);
	return strlen(cmd_line);

} // readline()

void cliTask( void *pvParameters){

    char c = 0;

    mzmsg_write("\r\nFreeRTOS CLI\r\n",16);

    char cmd_line[CMD_LINE_SIZE+1]="";

    while(1){

	    mzmsg_write("\n\rZ1 > ", 7);
        readline(cmd_line);
        mzmsg_write("\n", 1);

		char * tk1 = strtok (cmd_line, " ");
		char * tk2 = strtok (NULL, " ");
		char * tk3 = strtok (NULL, " ");

		if (tk1 != NULL && strcmp(tk1, "pmp")==0){
            print_pmp_ranges();
        } else if(tk1 != NULL && strcmp(tk1, "robot")==0){
			char c = (char) *tk2;
			// mzmsg_write("\n\rZ1> robot command ", 20);
			// mzmsg_write(&c, 1);
			// mzmsg_write("\n",1);
			xQueueSend( robot_queue, &c, 0 );
		}

		ECALL_YIELD();
    }
}