/*
 * M68000 Assembly Definitions and Macros
 * Common register names and utility macros for M68K assembly
 */

#ifndef M68K_DEFS_H
#define M68K_DEFS_H

/* Immediate value prefix - uses token pasting to create # prefix */
#ifndef __IMMEDIATE_PREFIX__
#define __IMMEDIATE_PREFIX__ #
#endif

/* ANSI concatenation macros */
#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b

/* use the right prefix for immediate values */
#define IMM(x) CONCAT1 (__IMMEDIATE_PREFIX__,x)

/* Register prefix (% for GNU AS) */
#ifndef __REGISTER_PREFIX__
#define __REGISTER_PREFIX__ %
#endif

#define REG(x) CONCAT1 (__REGISTER_PREFIX__,x)

/* Data Register Aliases */
#define d0  REG(d0)
#define d1  REG(d1)
#define d2  REG(d2)
#define d3  REG(d3)
#define d4  REG(d4)
#define d5  REG(d5)
#define d6  REG(d6)
#define d7  REG(d7)

/* Address Register Aliases */
#define a0  REG(a0)
#define a1  REG(a1)
#define a2  REG(a2)
#define a3  REG(a3)
#define a4  REG(a4)
#define a5  REG(a5)
#define a6  REG(a6)
#define a7  REG(a7)
#define sp  REG(sp)
#define fp  REG(fp)
#define pc  REG(pc)
#define sr  REG(sr)

/* M68K assembler recognizes register names natively (d0-d7, a0-a7, sp, pc)
 * No need for register prefix macros
 */

/* Status Register Bits */
#define SR_C    0x0001  /* Carry */
#define SR_V    0x0002  /* Overflow */
#define SR_Z    0x0004  /* Zero */
#define SR_N    0x0008  /* Negative */
#define SR_X    0x0010  /* Extend */
#define SR_IPM  0x0700  /* Interrupt Priority Mask */
#define SR_S    0x2000  /* Supervisor */
#define SR_T    0x8000  /* Trace */

/* Condition Code Masks */
#define CC_CARRY    0x01
#define CC_OVERFLOW 0x02
#define CC_ZERO     0x04
#define CC_NEGATIVE 0x08
#define CC_EXTEND   0x10

/* Common Constants */
#define NULL        0x00000000
#define TRUE        1
#define FALSE       0

/* Memory Sizes */
#define KB          * 1024
#define MB          * 1024 * 1024

#endif /* M68K_DEFS_H */
