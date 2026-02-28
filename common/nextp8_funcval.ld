/*
 * Linker Script for nextp8 Functional Validation Tests
 *
 * Does not require C library or BSP crt0-rom.o
 */

OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)
EXTERN(__interrupt_vector)

MEMORY
{
  rom (rx) : ORIGIN = 0M, LENGTH = 256K
  ram (rw) : ORIGIN = 256K, LENGTH = 785920
}

/* Place the stack at the end of memory, unless specified otherwise. */
PROVIDE (__stack = 1048064);

SECTIONS
{
  /* Interrupt Service Vector table - must be first */
  .isv :
  {
    CREATE_OBJECT_SYMBOLS
    *(.interrupt_vector)
  } >rom

  /* Code and read-only data - contiguous with ISV */
  .text :
  {
    *(.text._start)      /* Entry point first */
    *(.text .text.*)

    . = ALIGN(0x4);
    *(.rodata .rodata.*)

    . = ALIGN(4);
    _etext = .;
  } >rom

  /* Test suite entries - combined from multiple test.o objects */
  .test_suite_entry :
  {
    __test_suite_start = .;
    *(.test_suite_entry)
    __test_suite_end = .;
  } >rom

  /* Initialized data in RAM (loaded from ROM) */
  .data :
  {
    __data_load = LOADADDR (.data);
    __data_start = .;
    *(.data .data.*)
    . = ALIGN (4);
    _edata = .;
  } >ram AT>rom

  /* Uninitialized data in RAM */
  .bss :
  {
    __bss_start = . ;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN (8);
    _end = .;
    __end = _end;
  } >ram AT>rom

  /* Screen buffer section - placed after BSS */
  .screen_buffer :
  {
    *(.screen_buffer .screen_buffer.*)
    . = ALIGN (4);
  } >ram AT>rom

  /* Discard sections not needed for funcval tests */
  /DISCARD/ : {
    *(.comment)
    *(.note*)
    *(.debug*)
    *(.eh_frame)
    *(.got.plt)
    *(.got)
  }
}
