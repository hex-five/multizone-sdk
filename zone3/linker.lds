/* Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved */

OUTPUT_ARCH( "riscv" )

ENTRY( _start )

MEMORY {
  flash (rxai!w) : ORIGIN = flash + 0x9000, LENGTH = 4K
  itim  (rxai!w) : ORIGIN = itim  + 0x9000, LENGTH = 4K
  dtim  (wxa!ri) : ORIGIN = dtim  + 0x3000, LENGTH = 4K
}

SECTIONS {
  __stack_size = DEFINED(__stack_size) ? __stack_size : 2K;

  .init           : ALIGN(8)  {
    KEEP (*(SORT_NONE(.init)))
  } >itim AT>flash 
  
  .text   :  ALIGN(8) { 
    *(.text.unlikely .text.unlikely.*)
    *(.text.startup .text.startup.*)
    *(.text .text.*)
    *(.gnu.linkonce.t.*) 
  } >itim AT>flash 

  .fini           :  ALIGN(8) {
    KEEP (*(SORT_NONE(.fini)))
  } >itim AT>flash

  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);

  .rodata         :   ALIGN(8) {
    *(.rdata)
    *(.rodata .rodata.*)
    *(.gnu.linkonce.r.*)
  } >itim AT>flash

  .preinit_array  :  ALIGN(8)  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >itim AT>flash

  .init_array     :  ALIGN(8)  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >itim AT>flash

  .fini_array     : ALIGN(8)   {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >itim AT>flash  

  .ctors          : ALIGN(8)   {
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  } >itim AT>flash 

  .dtors          : ALIGN(8)   {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  } >itim AT>flash 

  .lalign         :   ALIGN(8) {
    PROVIDE( _data_lma = . );
  } >itim AT>flash  

  .dalign         :   ALIGN(8) {
    PROVIDE( _data = . );
  } >dtim AT>flash 

  .data          :  ALIGN(8) {
    *(.data .data.*)
    *(.gnu.linkonce.d.*)
    . = ALIGN(8);
    PROVIDE( __global_pointer$ = . + 0x800 );
    *(.sdata* .sdata.*)
    *(.gnu.linkonce.s.*)
    . = ALIGN(8);
    *(.srodata.cst16)
    *(.srodata.cst8)
    *(.srodata.cst4)
    *(.srodata.cst2)
    *(.srodata .srodata.*)
  } >dtim AT>flash

  . = ALIGN(8);
  PROVIDE( _edata = . );
  PROVIDE( edata = . );

  PROVIDE( _fbss = . );
  PROVIDE( __bss_start = . );
  .bss            :   {
    *(.sbss*)
    *(.gnu.linkonce.sb.*)
    *(.bss .bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN(8);
  } >dtim AT>dtim 

  . = ALIGN(8);
  PROVIDE( _end = . );
  PROVIDE( end = . );

  .stack ORIGIN(dtim) + LENGTH(dtim) - __stack_size :   {
    PROVIDE( _heap_end = . );
    . = __stack_size;
    PROVIDE( _sp = . );
  } >dtim AT>dtim 

}