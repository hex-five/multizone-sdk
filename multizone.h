/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#ifndef MULTIZONE_H
#define MULTIZONE_H


#include <stdint.h> // uint32_t uint64_t

/* MultiZone opcode */
#define mzone ".word 0x00300073;"

/* Thread Management */
#define MZONE_YIELD() \
	asm volatile ("li a0, 0;" mzone : : : "a0")
#define MZONE_WFI() \
	asm volatile ("li a0, 1;" mzone : : : "a0")

/* Secure Messaging */
#if __riscv_xlen==32

	#define MZONE_SEND(zone, msg) ({ \
		register uint32_t a0 asm ("a0"); \
		asm volatile ( \
		" lw a2, 0*4+%1; " \
		" lw a3, 1*4+%1; " \
		" lw a4, 2*4+%1; " \
		" lw a5, 3*4+%1; " \
		" mv a1, %2; 	 " \
		" li a0, 2;  	 " \
		  mzone			   \
		: "=r"(a0) : "m"(*(const char (*)[16]) msg), "r"((const int)zone) : "a1","a2","a3","a4","a5"); \
		a0; \
	})
	#define MZONE_RECV(zone, msg) ({ \
		register uint32_t a0 asm ("a0"); \
		asm volatile ( \
		" mv a1, %2; 	 " \
		" li a0, 3;  	 " \
		  mzone        	   \
		" sw a2, 0*4+%1; " \
		" sw a3, 1*4+%1; " \
		" sw a4, 2*4+%1; " \
		" sw a5, 3*4+%1; " \
		: "=r"(a0), "=m"(*(const char (*)[16]) msg) : "r"((const int)zone) : "a1","a2","a3","a4","a5"); \
		a0; \
	})

#else

	/* Secure Messaging */
	#define MZONE_SEND(zone, msg) ({ \
		register uint32_t a0 asm ("a0"); \
		asm volatile ( \
		" ld a2, 0*8+%1; " \
		" ld a3, 1*8+%1; " \
		" mv a1, %2; 	 " \
		" li a0, 2;  	 " \
		  mzone			   \
		: "=r"(a0) : "m"(*(const char (*)[16]) msg), "r"((const int)zone) : "a1","a2","a3"); \
		a0; \
	})
	#define MZONE_RECV(zone, msg) ({ \
		register uint32_t a0 asm ("a0"); \
		asm volatile ( \
		" mv a1, %2; 	 " \
		" li a0, 3;  	 " \
		  mzone        	   \
		" sd a2, 0*8+%1; " \
		" sd a3, 1*8+%1; " \
		: "=r"(a0), "=m"(*(const char (*)[16]) msg) : "r"((const int)zone) : "a1","a2","a3"); \
		a0; \
	})

#endif

/* RTC & Timer */
#if __riscv_xlen==32

	#define MZONE_RDTIME() ({ \
		register uint32_t a0 asm ("a0"), a1 asm ("a1"); \
		asm volatile ("li a0, 4;" mzone : "=r"(a0), "=r"(a1)); \
		(uint64_t)a1<<32|a0; \
	})
	#define MZONE_WRTIME(val) ({ \
		asm volatile ( \
		"mv a1, %0; " \
		"mv a2, %1; " \
		"li a0, 5;  " \
		 mzone       \
		: : "r"((uint32_t)val), "r"((uint32_t)(val>>32)): "a0","a1","a2"); \
	})
	#define MZONE_RDTIMECMP() ({ \
		register uint32_t a0 asm ("a0"), a1 asm ("a1"); \
		asm volatile ("li a0, 6;" mzone : "=r"(a0), "=r"(a1)); \
		(uint64_t)a1<<32|a0; \
	})
	#define MZONE_WRTIMECMP(val) ({ \
		asm volatile ( \
		"mv a1, %0; " \
		"mv a2, %1; " \
		"li a0, 7;  " \
		 mzone       \
		: : "r"((uint32_t)val), "r"((uint32_t)(val>>32)): "a0","a1","a2"); \
	})
	#define MZONE_ADTIMECMP(val) ({ \
		asm volatile ( \
		"mv a1, %0; " \
		"mv a2, %1; " \
		"li a0, 8;  " \
		mzone        \
		: : "r"((uint32_t)val), "r"((uint32_t)(val>>32)): "a0","a1","a2"); \
	})

#else

	#define MZONE_RDTIME() ({ \
		register uint64_t a0 asm ("a0"); \
		asm volatile ("li a0, 4;" mzone : "=r"(a0)); \
		a0; \
	})
/*	#define MZONE_WRTIME(val) ({ \
		asm volatile ( \
		"mv a1, %0; " \
		"li a0, 5;  " \
		 mzone       \
		: : "r"((uint32_t)val), "r"((uint32_t)(val>>32)): "a0","a1","a2"); \
	}) */
	#define MZONE_RDTIMECMP() ({ \
		register uint64_t a0 asm ("a0"); \
		asm volatile ("li a0, 6;" mzone : "=r"(a0)); \
		a0; \
	})
	#define MZONE_WRTIMECMP(val) ({ \
		asm volatile ( \
		"mv a1, %0; " \
		"li a0, 7;  " \
		 mzone       \
		: : "r"(val) : "a0","a1"); \
	})
	#define MZONE_ADTIMECMP(val) ({ \
		asm volatile ( \
		"mv a1, %0; " \
		"li a0, 8;  " \
		mzone        \
		: : "r"(val): "a0","a1"); \
	})

#endif

/* Fast CSR read */
#define MZONE_CSRR(csr) ({	\
	register unsigned long a0 asm ("a0"); \
	asm volatile ("li a1, %1; li a0, 9;" mzone : "=r"(a0) : "I"(csr) : "a1"); \
	a0; \
})

#define CSR_MSTATUS			 0
#define CSR_MIE              1
#define CSR_MTVEC            2
#define CSR_MSCRATCH         3
#define CSR_MEPC             4
#define CSR_MCAUSE			 5
#define CSR_MTVAL			 6
#define CSR_MIP				 7

#define CSR_MISA			 8
#define CSR_MVENDORID		 9
#define CSR_MARCHID			10
#define CSR_MIMPID			11
#define CSR_MHARTID 	 	12

#define CSR_MCYCLE 		 	13
#define CSR_MINSTRET 	 	14
#define CSR_MHPMCOUNTER3 	15
#define CSR_MHPMCOUNTER4 	16

#if __riscv_xlen==32
	#define CSR_MCYCLEH 	  	17
	#define CSR_MINSTRETH 		18
	#define CSR_MHPMCOUNTER3H 	19
	#define CSR_MHPMCOUNTER4H 	20
#endif

#define CSR_MHPMCOUNTER26 	24 // kernel irq lat cycle min
#define CSR_MHPMCOUNTER27 	25 // kernel irq lat cycle max
#define CSR_MHPMCOUNTER28 	27 // kernel ctx sw instr min
#define CSR_MHPMCOUNTER29 	28 // kernel ctx sw instr max
#define CSR_MHPMCOUNTER30 	29 // kernel ctx sw cycle min
#define CSR_MHPMCOUNTER31 	30 // kernel ctx sw cycle max


/* Privileged Pseudoinstructions */
#define CSRR(csr) ({ \
	unsigned long rd; \
	asm volatile ("csrr %0, " #csr : "=r"(rd)); \
	rd; \
})
#define CSRW(csr, rs) ({ \
	if (__builtin_constant_p(rs) && (unsigned long)(rs) < 32) \
		asm volatile ("csrw " #csr ", %0" :: "K"(rs)); \
	else \
		asm volatile ("csrw " #csr ", %0" :: "r"(rs)); \
})
#define CSRRW(csr, rs) ({ \
	unsigned long rd; \
	if (__builtin_constant_p(rs) && (unsigned long)(rs) < 32) \
		asm volatile ("csrrwi %0, " #csr ", %1" : "=r"(rd) : "K"(rs)); \
	else \
		asm volatile ("csrrw %0, " #csr ", %1" : "=r"(rd) : "r"(rs)); \
	rd; \
})
#define CSRS(csr, rs) ({ \
	unsigned long rd; \
	if (__builtin_constant_p(rs) && (unsigned long)(rs) < 32) \
		asm volatile ("csrrsi %0, " #csr ", %1" : "=r"(rd) : "K"(rs)); \
	else \
		asm volatile ("csrrs %0, " #csr ", %1" : "=r"(rd) : "r"(rs)); \
	rd; \
})
#define CSRC(csr, rs) ({ \
	unsigned long rd; \
	if (__builtin_constant_p(rs) && (unsigned long)(rs) < 32) \
		asm volatile ("csrrci %0, " #csr ", %1" : "=r"(rd) : "K"(rs)); \
	else \
		asm volatile ("csrrc %0, " #csr ", %1" : "=r"(rd) : "r"(rs)); \
	rd; \
})


#endif /* MULTIZONE_H */
