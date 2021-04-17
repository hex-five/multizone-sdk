/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

OUTPUT_ARCH( "riscv" )

ENTRY( boot )

MEMORY {
  flash (rxai!w) : ORIGIN = flash + 0x1800, LENGTH = 2K
}

SECTIONS {
  .boot : { KEEP(*(.text.boot)); } >flash
}
