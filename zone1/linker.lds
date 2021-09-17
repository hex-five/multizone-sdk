/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

OUTPUT_ARCH( "riscv" )

ENTRY( _start )

MEMORY {
  flash (rxai!w) : ORIGIN = flash + 0x8000, LENGTH = 32K
  prog  (rxai!w) : ORIGIN = flash + 0x8000, LENGTH = 32K
  ram   (wa!xri) : ORIGIN = dtim  + 0x3000, LENGTH =  4K
}

SECTIONS {

  __stack_size = DEFINED(__stack_size) ? __stack_size : 1K;
  PROVIDE(__stack_size = __stack_size);


  /** Init / Fini related code | run once - loaded in slower memory **/

  .init : ALIGN(64 / 8)
  {
    KEEP (*(SORT_NONE(.init)))
  } >prog AT>flash 

  .fini : ALIGN(64 / 8)
  {
    KEEP (*(SORT_NONE(.fini)))
  } >prog AT>flash 

  .preinit_array : ALIGN(64 / 8) 
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >prog AT>flash

  .init_array : ALIGN(64 / 8) 
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >prog AT>flash

  .fini_array : ALIGN(64 / 8)
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >prog AT>flash 

  .ctors : ALIGN(64 / 8)
  {
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  } >prog AT>flash 

  .dtors : ALIGN(64 / 8)
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  } >prog AT>flash 


  /** Text section | program - can be loaded to faster memory **/
  
  .text : ALIGN(64 / 8) 
  { 
    *(.text.unlikely .text.*_unlikely .text.unlikely.*)
    *(.text.exit .text.exit.*)
    *(.text.startup .text.startup.*)
    *(.text.hot .text.hot.*)
    *(SORT(.text.sorted.*))
    *(.text .stub .text.* .gnu.linkonce.t.*)
    /* .gnu.warning sections are handled specially by elf.em.  */
    *(.gnu.warning)
  } >prog AT>flash 

  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);

  .rodata : ALIGN(64 / 8)
  {
    *(.rdata)
    *(.rodata .rodata.*)
    *(.gnu.linkonce.r.*)
  } >prog AT>flash 


  /* Data section - copied from rom to ram */

  .lalign : ALIGN(64 / 8) 
  {
    PROVIDE( _data_lma = . );
  } >prog AT>flash  

  .data : ALIGN(64 / 8)    
  {
    __DATA_BEGIN__ = .;
    _data = .;
    *(.data .data.* .gnu.linkonce.d.*)
  } >ram AT>flash

  .sdata : ALIGN(64 / 8)    
  {
    PROVIDE( __global_pointer$ = . + 0x800 );
    __SDATA_BEGIN__ = .;
    *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata .srodata.*)
    *(.sdata .sdata.* .sdata* .gnu.linkonce.s.*)
  } >ram AT>flash

  _edata = .; 
  PROVIDE (edata = .);

  .bss (NOLOAD): ALIGN(64 / 8)  
  {
    __bss_start = .;
    *(.sbss .sbss.* .gnu.linkonce.sb.*)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN(. != 0 ? 64 / 8 : 1);
  } >ram AT>ram 

  . = ALIGN(64 / 8);
  . = SEGMENT_START("ldata-segment", .);
  . = ALIGN(64 / 8);
  __bss_end = .;
  __BSS_END__ = .;


  /* Stack definition - change size at top of this script */

  .stack (NOLOAD): ALIGN(16)  
  {
    . += __stack_size;
    PROVIDE( _sp = . );
  } >ram AT>ram 


  /* Heap definition - calculated to use the remaining memory */

  .heap (NOLOAD): ALIGN(64 / 8)  
  {
    PROVIDE( _end = . );
    PROVIDE( __end = . );
    PROVIDE( __heap_start = . );
    . += LENGTH(ram) - ( . - ORIGIN(ram));
    PROVIDE( _heap_end = . );
    PROVIDE( __heap_end = . );
  } >ram AT>ram 

}