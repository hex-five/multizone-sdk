/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include <platform.h>

/* https://www.embecosm.com/appnotes/ean9/ean9-howto-newlib-1.0.html */

// ----------------------------------------------------------------------------
int _close(int file) {
// ----------------------------------------------------------------------------

	return -1;
}

// ----------------------------------------------------------------------------
int _fstat(int file, struct stat *st) {
// ----------------------------------------------------------------------------

	st->st_mode = S_IFCHR;
	return 0;
}

// ----------------------------------------------------------------------------
void * _sbrk(int incr) {
// ----------------------------------------------------------------------------

	extern char _end[];
	extern char _heap_end[];
	static char *_heap_ptr = _end;

	if ((_heap_ptr + incr < _end) || (_heap_ptr + incr > _heap_end))
		return  (void *) -1;

	_heap_ptr += incr;
	return _heap_ptr - incr;
}

// ----------------------------------------------------------------------------
int _isatty(int file) {
// ----------------------------------------------------------------------------

	return (file == STDIN_FILENO || file == STDOUT_FILENO || file == STDERR_FILENO) ? 1 : 0;

}

// ----------------------------------------------------------------------------
int _lseek(int file, off_t ptr, int dir) {
// ----------------------------------------------------------------------------

	return 0;
}

// ----------------------------------------------------------------------------
int _open(const char* name, int flags, int mode) {
// ----------------------------------------------------------------------------

	if (strcmp(name, "UART")==0){

		UART_REG(UART_DIV) = CPU_FREQ/(115200-1);
		UART_REG(UART_TXCTRL) = 0b01;
		UART_REG(UART_RXCTRL) = 0b01;
		UART_REG(UART_IE)     = 0b10; // RX irq

		return 0;

	}

	return -1;
}

// ----------------------------------------------------------------------------
int _read(int file, char *ptr, size_t len) {
// ----------------------------------------------------------------------------

	if (isatty(file)) {

		size_t count = 0;
		int rxfifo = -1;

		while( count<len && ((rxfifo = UART_REG(UART_RXFIFO)) >0) ){
			*ptr++ = (char)rxfifo;
			count++;
		}

		return count;
	}

	return -1;
}

// ----------------------------------------------------------------------------
void _putchar(const char c) {
// ----------------------------------------------------------------------------

    while (UART_REG(UART_TXFIFO) & 0x80000000){;}

    UART_REG(UART_TXFIFO) = c;

    if (c == '\n') {
        while (UART_REG(UART_TXFIFO) & 0x80000000){;}
        UART_REG(UART_TXFIFO) = '\r';
    }

}

// ----------------------------------------------------------------------------
size_t _write(int file, const void *ptr, size_t len) {
// ----------------------------------------------------------------------------

	if (isatty(file)) {

		const uint8_t * buff = (uint8_t *)ptr;

		for (size_t i = 0; i < len; i++) {

		    _putchar(buff[i]);

		}

		return len;

	}

	return -1;
}

// ----------------------------------------------------------------------------
int _kill (int  pid, int  sig) {
// ----------------------------------------------------------------------------

	return -1;

}

// ----------------------------------------------------------------------------
int _getpid () {
// ----------------------------------------------------------------------------

	return  1;

}

// open("UART", 0, 0); write(1, "Ok >\n", 5); char c='\0'; while(1) if ( read(0, &c, 1) >0 ) write(1, &c, 1);
