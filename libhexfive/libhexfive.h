/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

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


#endif /* LIBHEXFIVE_H_ */


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
