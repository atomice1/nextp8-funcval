/* UART Functions for nextp8 Functional Validation Library
 *
 * Copyright (C) 2026 Chris January
 */

#include <stddef.h>
#include <stdint.h>
#include "nextp8.h"
#include "funcval.h"

/* UART constants */
#define UART_CTRL_WRITE_STROBE  0x01
#define UART_STATUS_READY       0x02
#define UART_STATUS_WRITE_ACK   0x08
#define UART_BAUD_115200        0x005F  /* 95 = 115200 baud at ~10.955 MHz clk_sys */

/* Inline register access */
#define MMIO_REG8(addr)  (*(volatile uint8_t *)(addr))
#define MMIO_REG16(addr) (*(volatile uint16_t *)(addr))

/* uart_init - Initialize UART to 115200 baud */
void uart_init(void)
{
    MMIO_REG16(_UART_BAUD_DIV) = UART_BAUD_115200;
}

/* uart_write_byte - Write a single byte to UART */
void uart_write_byte(uint8_t byte)
{
    /* Wait for UART ready */
    while ((MMIO_REG8(_UART_CTRL) & UART_STATUS_READY) == 0);

    /* Write data */
    MMIO_REG8(_UART_DATA) = byte;

    /* Strobe write */
    MMIO_REG8(_UART_CTRL) = UART_CTRL_WRITE_STROBE;

    /* Wait for write acknowledgment */
    while ((MMIO_REG8(_UART_CTRL) & UART_STATUS_WRITE_ACK) == 0);

    /* Clear strobe */
    MMIO_REG8(_UART_CTRL) = 0x00;
}

/* uart_puts - Write null-terminated string to UART */
void uart_puts(const char *str)
{
    while (*str) {
        uart_write_byte(*str++);
    }
}

/* uart_putchar - Write single character (convenience wrapper) */
void uart_putchar(char c)
{
    uart_write_byte(c);
}

/* uart_print_hex_nibble - Print single hex nibble */
static void uart_print_hex_nibble(uint8_t nibble)
{
    nibble &= 0x0F;
    if (nibble < 10)
        uart_write_byte('0' + nibble);
    else
        uart_write_byte('A' + (nibble - 10));
}

/* uart_print_hex_byte - Print 8-bit value as hex (2 digits) */
void uart_print_hex_byte(uint8_t value)
{
    uart_print_hex_nibble(value >> 4);
    uart_print_hex_nibble(value);
}

/* uart_print_hex_word - Print 16-bit value as hex (4 digits) */
void uart_print_hex_word(uint16_t value)
{
    for (int i = 3; i >= 0; i--) {
        uart_print_hex_nibble(value >> (i * 4));
    }
}

/* uart_print_hex_long - Print 32-bit value as hex (8 digits) */
void uart_print_hex_long(uint32_t value)
{
    for (int i = 7; i >= 0; i--) {
        uart_print_hex_nibble(value >> (i * 4));
    }
}

/* uart_print_dec - Print 32-bit value as decimal */
void uart_print_dec(uint32_t value)
{
    char buffer[12];  /* Max uint32_t is 4294967295 (10 digits) + \0 */
    int pos = 0;

    /* Handle zero specially */
    if (value == 0) {
        uart_write_byte('0');
        return;
    }

    /* Convert to decimal (reverse order) */
    while (value > 0) {
        buffer[pos++] = '0' + (value % 10);
        value /= 10;
    }

    /* Print in correct order */
    for (int i = pos - 1; i >= 0; i--) {
        uart_write_byte(buffer[i]);
    }
}

/* uart_print_crlf - Print carriage return and line feed */
void uart_print_crlf(void)
{
    uart_write_byte(0x0D);  /* CR */
    uart_write_byte(0x0A);  /* LF */
}
