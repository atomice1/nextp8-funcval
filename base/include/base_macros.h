/*
 * nextp8 Base Test Helper Macros
 * Assembly macros for standalone base tests
 *
 * These macros provide simple UART output and system control
 * without requiring the full libfuncval.a library.
 *
 * Clobbers:
 *   SET_POST: a0
 *   UART_INIT: a0
 *   UART_WRITE_BYTE: a0, d0, d1
 *   UART_PUTS: a1, d0, d1
 *   SHUTDOWN: a0, d0
 */

#ifndef BASE_MACROS_H
#define BASE_MACROS_H

#include <nextp8.h>

/* Set POST code (diagnostic output) */
#define SET_POST(code) \
	move.l  #_POST_CODE, a0; \
	move.b  code, (a0)

/* Initialize UART for 115200 baud */
#define UART_INIT() \
	move.l  #_UART_BAUD_DIV, a0; \
	move.w  #_UART_BAUD_115200, (a0)

/* Write a single byte to UART (blocking) */
#define UART_WRITE_BYTE(value) \
	move.b  value, d0; \
	move.l  #_UART_CTRL, a0; \
	1:  move.b  (a0), d1; \
	andi.b  #_UART_STATUS_READY, d1; \
	beq.s   1b; \
	move.l  #_UART_DATA, a0; \
	move.b  d0, (a0); \
	move.l  #_UART_CTRL, a0; \
	move.b  #_UART_CTRL_WRITE_STROBE, (a0); \
	2:  move.b  (a0), d1; \
	andi.b  #_UART_STATUS_WRITE_ACK, d1; \
	beq.s   2b; \
	move.b  #0x00, (a0)

/* Write a null-terminated string to UART */
#define UART_PUTS(str_reg) \
	move.l  str_reg, a1; \
	5:  move.b  (a1)+, d0; \
	beq.s   6f; \
	UART_WRITE_BYTE(d0); \
	bra.s   5b; \
	6:

/* Shutdown the system (write 0xFF to RESET_REQ) */
#define SHUTDOWN() \
	move.l  #_RESET_REQ, a0; \
	move.b  #0xFF, (a0); \
	4:  bra.s   4b

#endif /* BASE_MACROS_H */
