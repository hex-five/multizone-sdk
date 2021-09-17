/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

flash = 0x40400000;
itim  = 0x01800000; /* 16K */
dtim  = 0x80000200; /* 64K */

/* Note: dtim start value increased by +512B to offest the larger rv64 kernel
 without changing dtim offests in the zones' linker sripts */ 