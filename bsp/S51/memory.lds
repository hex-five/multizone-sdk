/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

flash = 0x40400000;
itim  = 0x08000000; /* 16K */
dtim  = 0x80000800; /* 64K */

/* Note:
 dtim start value increased +2K to make room for the larger 8K rv64 kernel
 while allowing for unchanged dtim/ram offsets in the zones linker sripts 
 that fit the smaller 6K rv32 kernel and the DTIM 16K size limit imposed
 by the FE310
*/ 