/* Copyright(C) 2019 Hex Five Security, Inc. - All Rights Reserved */

/* ITIM:  8 KiB @0x0800_0000 don't use to leave 2-ways for instr cache  */
/* DTIM: 64 KiB @0x8000_0000                                            */

flash = 0x40400000; /* lower 8K reserved for MultiZone code in flash                */
itim  = 0x40400000; 
dtim  = 0x80000000; /* lower 8K reserved for MultiZone code & data in ram           */
                    /* upper 40K @_6000 available for loading zone code in ram (optional) */

