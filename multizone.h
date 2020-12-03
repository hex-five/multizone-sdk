/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#ifndef MULTIZONE_H
#define MULTIZONE_H


#include <stdint.h> // uint32_t uint64_t

/* MultiZone opcode */
#define mzone ".word 0x00300073;"

/* Thread Management */
#define MZONE_YIELD() ({ \
	const register volatile unsigned long a0  asm ("a0") = 0; \
	asm volatile (mzone : : "r"(a0)); \
})

#define MZONE_WFI() ({ \
	const register volatile unsigned long a0 asm ("a0") = 1; \
	asm volatile (mzone : : "r"(a0)); \
})

/* Secure Messaging */
#if __riscv_xlen==32

	#define MZONE_SEND(zone, msg) ({ \
		      register volatile uint32_t a0 asm ("a0") = 2; \
		const register volatile uint32_t a1 asm ("a1") = (const int)zone ; \
		const register volatile uint32_t a2 asm ("a2") = *((uint32_t *)(msg+0*4)) ; \
		const register volatile uint32_t a3 asm ("a3") = *((uint32_t *)(msg+1*4)) ; \
		const register volatile uint32_t a4 asm ("a4") = *((uint32_t *)(msg+2*4)) ; \
		const register volatile uint32_t a5 asm ("a5") = *((uint32_t *)(msg+3*4)) ; \
		asm volatile (mzone	: "+r"(a0) : "r"(a1),"r"(a2),"r"(a3),"r"(a4),"r"(a5)); \
		a0; \
	})

	#define MZONE_RECV(zone, msg) ({ \
		      register volatile uint32_t a0 asm ("a0") = 3; \
		const register volatile uint32_t a1 asm ("a1") = (const int)zone ; \
		      register volatile uint32_t a2 asm ("a2"); \
		      register volatile uint32_t a3 asm ("a3"); \
		      register volatile uint32_t a4 asm ("a4"); \
		      register volatile uint32_t a5 asm ("a5"); \
		asm volatile (mzone	: "+r"(a0), "=r"(a2),"=r"(a3),"=r"(a4),"=r"(a5) : "r"(a1)); \
		*((uint32_t *)(msg+0*4)) = a2; \
		*((uint32_t *)(msg+1*4)) = a3; \
		*((uint32_t *)(msg+2*4)) = a4; \
		*((uint32_t *)(msg+3*4)) = a5; \
		a0; \
	})

#else

	#define MZONE_SEND(zone, msg) ({ \
		      register volatile uint64_t a0 asm ("a0") = 2; \
		const register volatile uint64_t a1 asm ("a1") = (const int)zone ; \
		const register volatile uint64_t a2 asm ("a2") = *((uint64_t *)(msg+0*8)) ; \
		const register volatile uint64_t a3 asm ("a3") = *((uint64_t *)(msg+1*8)) ; \
		asm volatile (mzone	: "+r"(a0) : "r"(a1),"r"(a2),"r"(a3)); \
		a0; \
	})

	#define MZONE_RECV(zone, msg) ({ \
		      register volatile uint64_t a0 asm ("a0") = 3; \
		const register volatile uint64_t a1 asm ("a1") = (const int)zone ; \
		      register volatile uint64_t a2 asm ("a2"); \
		      register volatile uint64_t a3 asm ("a3"); \
		asm volatile (mzone	: "+r"(a0), "=r"(a2),"=r"(a3) : "r"(a1)); \
		*((uint64_t *)(msg+0*8)) = a2; \
		*((uint64_t *)(msg+1*8)) = a3; \
		a0; \
	})

#endif

/* RTC & Timer */
#if __riscv_xlen==32

	#define MZONE_RDTIME() ({ \
		register volatile uint32_t a0 asm ("a0") = 4; \
		register volatile uint32_t a1 asm ("a1"); \
		asm volatile (mzone : "+r"(a0), "=r"(a1)); \
		(uint64_t)a1<<32 | a0; \
	})

	#define MZONE_RDTIMECMP() ({ \
		register volatile uint32_t a0 asm ("a0") = 5; \
		register volatile uint32_t a1 asm ("a1"); \
		asm volatile (mzone : "+r"(a0), "=r"(a1)); \
		(uint64_t)a1<<32 | a0; \
	})

	#define MZONE_WRTIMECMP(val) ({ \
		const register volatile uint32_t a0 asm ("a0") = 6; \
		const register volatile uint32_t a1 asm ("a1") = (uint32_t)((uint64_t)val); \
		const register volatile uint32_t a2 asm ("a2") = (uint32_t)((uint64_t)val>>32); \
		asm volatile (mzone : : "r"(a0), "r"(a1), "r"(a2) ); \
	})

	#define MZONE_ADTIMECMP(val) ({ \
		const register volatile uint32_t a0 asm ("a0") = 7; \
			  register volatile uint32_t a1 asm ("a1") = (uint32_t)((uint64_t)val); \
		      register volatile uint32_t a2 asm ("a2") = (uint32_t)((uint64_t)val>>32); \
		asm volatile (mzone : "+r"(a1), "+r"(a2) : "r"(a0) ); \
	})

#else

	#define MZONE_RDTIME() ({ \
		register volatile uint64_t a0 asm ("a0") = 4; \
		asm volatile (mzone : "+r"(a0)); \
		a0; \
	})

	#define MZONE_RDTIMECMP() ({ \
		register volatile uint64_t a0 asm ("a0") = 5; \
		asm volatile (mzone : "+r"(a0)); \
		a0; \
	})

	#define MZONE_WRTIMECMP(val) ({ \
		const register volatile uint64_t a0 asm ("a0") = 6; \
		const register volatile uint64_t a1 asm ("a1") = (uint64_t)val; \
		asm volatile (mzone : : "r"(a0), "r"(a1) ); \
	})

	#define MZONE_ADTIMECMP(val) ({ \
		const register volatile uint64_t a0 asm ("a0") = 7; \
		      register volatile uint64_t a1 asm ("a1") = (uint64_t)val; \
		asm volatile (mzone : "+r"(a1) : "r"(a0) ); \
	})

#endif

/* Fast CSR read */
#define MZONE_CSRR(CSR_XXX) ({	\
		      register volatile unsigned long a0 asm ("a0") = 8; \
		const register volatile unsigned long a1 asm ("a1") = CSR_XXX; \
		asm volatile (mzone : "+r"(a0) : "r"(a1)); \
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

#define CSR_MHPMCOUNTER26 	21 // kernel irq lat cycle min
#define CSR_MHPMCOUNTER27 	22 // kernel irq lat cycle max
//                          23 //
#define CSR_MHPMCOUNTER28 	24 // kernel ctx sw instr min
#define CSR_MHPMCOUNTER29 	25 // kernel ctx sw instr max
#define CSR_MHPMCOUNTER30 	26 // kernel ctx sw cycle min
#define CSR_MHPMCOUNTER31 	27 // kernel ctx sw cycle max


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
