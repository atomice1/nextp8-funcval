/*
 * Minimal Linker Script for nextp8 Base Tests
 * M68000 bare-metal configuration
 *
 * Memory Layout:
 *   0x000000 - 0x0003FF: Exception vectors (1KB)
 *   0x004000 - 0x0FFFFF: Code and data (1008KB)
 *   0x100000:             Initial stack pointer (1MB)
 *
 * Usage: m68k-elf-ld -T nextp8_base.ld -o test.elf test.o
 */

OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)

MEMORY
{
    vectors (rx) : ORIGIN = 0x000000, LENGTH = 0x400     /* 1KB for exception vectors */
    rom (rx)     : ORIGIN = 0x004000, LENGTH = 0xFC000   /* ~1008KB for code */
    ram (rw)     : ORIGIN = 0x000400, LENGTH = 0x3FFC00  /* ~4MB RAM (after vectors) */
}

SECTIONS
{
    /* Exception vector table at address 0 */
    .vectors 0x0 : {
        KEEP(*(.vectors))
        . = ALIGN(4);
    } > vectors

    /* Code section starting at 0x4000 */
    .text 0x4000 : {
        _text_start = .;
        *(.text._start)      /* Entry point first */
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
        _text_end = .;
    } > rom

    /* Data section (if any) - placed in RAM to support .org directives */
    .data : {
        _data_start = .;
        *(.data .data.*)
        . = ALIGN(4);
        _data_end = .;
    } > ram

    /* BSS section (uninitialized data) at 0x8000 */
    .bss 0x8000 : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > ram

    /* Stack grows down from 1MB */
    _stack_top = 0x100000;

    /* Discard debug info to keep binary small */
    /DISCARD/ : {
        *(.comment)
        *(.note*)
    }
}
