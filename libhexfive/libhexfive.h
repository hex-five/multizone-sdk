/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#include <unistd.h>

#ifndef LIBHEXFIVE_H_
#define LIBHEXFIVE_H_

void ECALL_YIELD();
int ECALL_SEND(int, void *);
int ECALL_RECV(int, void *);

// ----- Privileged Pseudoinstructions  ------

#define CSRW(csr, rs) ({ \
  if (__builtin_constant_p(rs) && (unsigned long)(rs) < 32) \
	asm volatile ("csrw " #csr ", %0" :: "i"(rs)); \
  else \
	asm volatile ("csrw " #csr ", %0" :: "r"(rs)); \
  })

#define CSRRW(csr, rs) ({ unsigned long rd; \
  if (__builtin_constant_p(rs) && (unsigned long)(rs) < 32) \
    asm volatile ("csrrw %0, " #csr ", %1" : "=r"(rd) : "i"(rs)); \
  else \
    asm volatile ("csrrw %0, " #csr ", %1" : "=r"(rd) : "r"(rs)); \
  rd; })

#define CSRR(csr) ({ unsigned long rd; \
  asm volatile ("csrr %0, " #csr : "=r"(rd)); \
  rd; })

#define CSRRS(csr, rs) ({ unsigned long rd; \
  if (__builtin_constant_p(rs) && (unsigned long)(rs) < 32) \
    asm volatile ("csrrs %0, " #csr ", %1" : "=r"(rd) : "i"(rs)); \
  else \
    asm volatile ("csrrs %0, " #csr ", %1" : "=r"(rd) : "r"(rs)); \
  rd; })

#define CSRRC(csr, rs) ({ unsigned long rd; \
  if (__builtin_constant_p(rs) && (unsigned long)(rs) < 32) \
    asm volatile ("csrrc %0, " #csr ", %1" : "=r"(rd) : "i"(rs)); \
  else \
    asm volatile ("csrrc %0, " #csr ", %1" : "=r"(rd) : "r"(rs)); \
  rd; })

// ----- TO BE REMOVED -------------

//void ECALL_TRP_VECT(int, void *);
//void ECALL_IRQ_VECT(int, void *);

uint64_t ECALL_CSRR_MTIME();
uint64_t ECALL_CSRR_MCYCLE();
uint64_t ECALL_CSRR_MINSTR();
uint64_t ECALL_CSRR_MHPMC3();
uint64_t ECALL_CSRR_MHPMC4();

void ECALL_CSRW_MTIMECMP(uint64_t);

#endif /* LIBHEXFIVE_H_ */


/* -------- CSRs ----------------------

#define CSR_MSTATUS 		0x300
#define CSR_MISA 			0x301
#define CSR_MEDELEG 		0x302
#define CSR_MIDELEG 		0x303
#define CSR_MIE 			0x304
#define CSR_MTVEC 			0x305

#define CSR_MSCRATCH 		0x340
#define CSR_MEPC 			0x341
#define CSR_MCAUSE 			0x342
#define CSR_MBADADDR 		0x343
#define CSR_MIP 			0x344

#define CSR_MCYCLE 			0xb00
#define CSR_MINSTRET 		0xb02
#define CSR_MHPMCOUNTER3 	0xb03
#define CSR_MHPMCOUNTER4 	0xb04

#define CSR_MVENDORID 		0xf11
#define CSR_MARCHID 		0xf12
#define CSR_MIMPID 			0xf13
#define CSR_MHARTID 		0xf14

#define CSR_MCYCLEH 		0xb80
#define CSR_MINSTRETH 		0xb82
#define CSR_MHPMCOUNTER3H 	0xb83
#define CSR_MHPMCOUNTER4H 	0xb84

#define CSR_SSTATUS 		0x100
#define CSR_SIE 			0x104
#define CSR_STVEC 			0x105
#define CSR_SSCRATCH 		0x140
#define CSR_SEPC 			0x141
#define CSR_SCAUSE 			0x142
#define CSR_SBADADDR 		0x143
#define CSR_SIP 			0x144
#define CSR_SPTBR 			0x180

#define CSR_CYCLE 			0xc00
#define CSR_TIME 			0xc01
#define CSR_INSTRET 		0xc02
#define CSR_HPMCOUNTER3 	0xc03
#define CSR_HPMCOUNTER4 	0xc04

#define CSR_CYCLEH 			0xc80
#define CSR_TIMEH 			0xc81
#define CSR_INSTRETH 		0xc82
#define CSR_HPMCOUNTER3H 	0xc83
#define CSR_HPMCOUNTER4H 	0xc84

*/

/*
#define read_csr(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define write_csr(reg, val) ({ \
  if (__builtin_constant_p(val) && (unsigned long)(val) < 32) \
    asm volatile ("csrw " #reg ", %0" :: "i"(val)); \
  else \
    asm volatile ("csrw " #reg ", %0" :: "r"(val)); })

#define swap_csr(reg, val) ({ unsigned long __tmp; \
  if (__builtin_constant_p(val) && (unsigned long)(val) < 32) \
    asm volatile ("csrrw %0, " #reg ", %1" : "=r"(__tmp) : "i"(val)); \
  else \
    asm volatile ("csrrw %0, " #reg ", %1" : "=r"(__tmp) : "r"(val)); \
  __tmp; })

#define set_csr(reg, bit) ({ unsigned long __tmp; \
  if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32) \
    asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit)); \
  else \
    asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit)); \
  __tmp; })

#define clear_csr(reg, bit) ({ unsigned long __tmp; \
  if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32) \
    asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit)); \
  else \
    asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit)); \
  __tmp; })

#define rdtime() read_csr(time)
#define rdcycle() read_csr(cycle)
#define rdinstret() read_csr(instret)
*/
