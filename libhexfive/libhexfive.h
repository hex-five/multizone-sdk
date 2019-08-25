/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

#ifndef LIBHEXFIVE_H_
#define LIBHEXFIVE_H_

#define ECALL_YIELD() asm volatile ("li a0, 0; ecall" : : : "a0")

int ECALL_SEND(int, void *);
int ECALL_RECV(int, void *);

#define ECALL_CSRR(csr) ({ unsigned long rd; \
  asm volatile ("li a0, 3; mv a1, %1; ecall; mv %0, a0" : "=r"(rd) : "r"(csr) : "a0", "a1"); \
  rd; })

#define CSR_MISA 			0
#define CSR_MVENDORID 		1
#define CSR_MARCHID 		2
#define CSR_MIMPID 			3
#define CSR_MHARTID 		4

#define CSR_MCYCLE 			5
#define CSR_MINSTRET 		6
#define CSR_MHPMCOUNTER3 	7
#define CSR_MHPMCOUNTER4 	8

#define CSR_MCYCLEH 		9
#define CSR_MINSTRETH 		10
#define CSR_MHPMCOUNTER3H 	11
#define CSR_MHPMCOUNTER4H 	12

// ----- Privileged Pseudoinstructions  ------

#define CSRW(csr, rs) ({ \
  if (__builtin_constant_p(rs) && (unsigned long)(rs) < 32) \
	asm volatile ("csrw " #csr ", %0" :: "k"(rs)); \
  else \
	asm volatile ("csrw " #csr ", %0" :: "r"(rs)); \
  })

#define CSRRW(csr, rs) ({ unsigned long rd; \
  if (__builtin_constant_p(rs) && (unsigned long)(rs) < 32) \
    asm volatile ("csrrw %0, " #csr ", %1" : "=r"(rd) : "k"(rs)); \
  else \
    asm volatile ("csrrw %0, " #csr ", %1" : "=r"(rd) : "r"(rs)); \
  rd; })

#define CSRR(csr) ({ unsigned long rd; \
  asm volatile ("csrr %0, " #csr : "=r"(rd)); \
  rd; })

#define CSRRS(csr, rs) ({ unsigned long rd; \
  if (__builtin_constant_p(rs) && (unsigned long)(rs) < 32) \
    asm volatile ("csrrs %0, " #csr ", %1" : "=r"(rd) : "k"(rs)); \
  else \
    asm volatile ("csrrs %0, " #csr ", %1" : "=r"(rd) : "r"(rs)); \
  rd; })

#define CSRRC(csr, rs) ({ unsigned long rd; \
  if (__builtin_constant_p(rs) && (unsigned long)(rs) < 32) \
    asm volatile ("csrrc %0, " #csr ", %1" : "=r"(rd) : "k"(rs)); \
  else \
    asm volatile ("csrrc %0, " #csr ", %1" : "=r"(rd) : "r"(rs)); \
  rd; })


#endif /* LIBHEXFIVE_H_ */

