/* Test Suite: ESP UART
 * Tests ESP8266 UART communication with AT commands
 * Inlined from nextp8-bsp/src/esp.c (adapted for self-contained testing)
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "nextp8.h"
#include "funcval.h"
#include "mmio.h"

/* ESP_CTRL register bits */
#define ESP_CTRL_DATA_READY  0x01  /* Read: data ready */
#define ESP_CTRL_READY       0x02  /* Read: ready */
#define ESP_CTRL_WRITE_STROBE 0x01 /* Write: write strobe */
#define ESP_CTRL_READ_STROBE  0x02 /* Write: read strobe */

#define AT_TIMEOUT_US 5000000  /* 5 seconds */
#define AT_RESPONSE_BUF_SIZE 512

/* Write a single byte to ESP8266 UART */
static int esp_write_byte(unsigned char byte)
{
    unsigned timeout = 10000;

    /* Write data */
    MMIO_REG8(_ESP_DATA) = byte;

    /* Wait for ready (bit 2) */
    while (!(MMIO_REG8(_ESP_CTRL) & ESP_CTRL_READY)) {
        if (--timeout == 0) {
            test_puts("  WRITE_BYTE timeout");
            return -1;
        }
    }

    /* Pulse write strobe (bit 0) */
    MMIO_REG8(_ESP_CTRL) = ESP_CTRL_WRITE_STROBE;
    MMIO_REG8(_ESP_CTRL) = ESP_CTRL_WRITE_STROBE;
    MMIO_REG8(_ESP_CTRL) = ESP_CTRL_WRITE_STROBE;
    MMIO_REG8(_ESP_CTRL) = 0;

    return 0;
}

/* Read a single byte from ESP8266 UART with timeout */
static int esp_read_byte(unsigned char *byte, unsigned timeout_us)
{
    uint64_t start_time = MMIO_REG64(_UTIMER_1MHZ);

    /* Wait for data ready (bit 1) */
    while (1) {
        if (MMIO_REG8(_ESP_CTRL) & ESP_CTRL_DATA_READY) {
            *byte = MMIO_REG8(_ESP_DATA);

            /* Pulse read strobe (bit 1) */
            MMIO_REG8(_ESP_CTRL) = ESP_CTRL_READ_STROBE;
            MMIO_REG8(_ESP_CTRL) = ESP_CTRL_READ_STROBE;
            MMIO_REG8(_ESP_CTRL) = ESP_CTRL_READ_STROBE;
            MMIO_REG8(_ESP_CTRL) = 0;

            return 0;
        }

        uint64_t elapsed = MMIO_REG64(_UTIMER_1MHZ) - start_time;
        if (elapsed >= timeout_us) {
            test_puts("  READ_BYTE timeout");
            return -1;
        }
    }
}

/* Write a string to ESP8266 */
static int esp_write_string(const char *str)
{
    while (*str) {
        if (esp_write_byte(*str++) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Read a line from ESP8266 (terminated by \r\n) */
static int esp_read_line(char *buffer, size_t buf_size, unsigned timeout_us)
{
    size_t pos = 0;
    unsigned char ch;
    uint64_t start_time = MMIO_REG64(_UTIMER_1MHZ);

    while (pos < buf_size - 1) {
        uint64_t elapsed = MMIO_REG64(_UTIMER_1MHZ) - start_time;
        if (elapsed >= timeout_us) {
            buffer[pos] = '\0';
            test_puts("  READ_LINE timeout");
            return -1;
        }

        if (esp_read_byte(&ch, timeout_us - (unsigned)elapsed) < 0) {
            buffer[pos] = '\0';
            return -1;
        }

        if (ch == '\n') {
            /* Strip trailing \r if present */
            if (pos > 0 && buffer[pos - 1] == '\r') {
                buffer[pos - 1] = '\0';
            } else {
                buffer[pos] = '\0';
            }
            return pos;
        }

        buffer[pos++] = ch;
    }

    buffer[buf_size - 1] = '\0';
    return -1;
}

/* Send AT command and wait for expected response */
static int esp_send_at_command(const char *cmd, const char *expected_response, unsigned timeout_us)
{
    char response[AT_RESPONSE_BUF_SIZE];

    /* Send command */
    if (esp_write_string(cmd) < 0) {
        test_puts("  AT command write failed");
        return -1;
    }
    if (esp_write_string("\r\n") < 0) {
        test_puts("  AT command CRLF failed");
        return -1;
    }

    /* Read response lines until we find the expected response or timeout */
    uint64_t start_time = MMIO_REG64(_UTIMER_1MHZ);

    while (1) {
        uint64_t elapsed = MMIO_REG64(_UTIMER_1MHZ) - start_time;
        if (elapsed >= timeout_us) {
            test_puts("  AT command timeout");
            return -1;
        }

        if (esp_read_line(response, sizeof(response), timeout_us - elapsed) < 0) {
            return -1;
        }

        /* Check for expected response */
        if (expected_response && strstr(response, expected_response) != NULL) {
            return 0;
        }

        /* Check for error */
        if (strcmp(response, "ERROR") == 0 || strcmp(response, "FAIL") == 0) {
            test_puts("  AT command error: ");
            test_puts(response);
            test_print_crlf();
            return -1;
        }
    }
}

/* Test 1: Send AT and check for OK */
static int test_at_command(void)
{
    test_puts("  Sending AT command... ");

    int result = esp_send_at_command("AT", "OK", AT_TIMEOUT_US);


}

/* Test 2: Disable echo with ATE0 */
static int test_disable_echo(void)
{
    test_puts("  Disabling echo (ATE0)... ");

    int result = esp_send_at_command("ATE0", "OK", AT_TIMEOUT_US);

    return (result != 0) ? TEST_FAIL : TEST_PASS;
}

/* Test 3: Read AT response line by line */
static int test_read_response(void)
{
    test_puts("  Reading AT response line-by-line... ");

    char buffer[AT_RESPONSE_BUF_SIZE];

    /* Send AT command */
    if (esp_write_string("AT") < 0) {
        test_puts("FAIL (write)");
        test_print_crlf();
        return TEST_FAIL;
    }
    if (esp_write_string("\r\n") < 0) {
        test_puts("FAIL (CRLF)");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Read first response line */
    int result = esp_read_line(buffer, sizeof(buffer), AT_TIMEOUT_US);
    if (result < 0) {
        test_puts("FAIL (read timeout)");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_print_crlf();
    test_puts("  First line: '");
    test_puts(buffer);
    test_puts("'");
    test_print_crlf();

    /* Read second response line (should be OK or empty) */
    result = esp_read_line(buffer, sizeof(buffer), AT_TIMEOUT_US);
    if (result < 0) {
        test_puts("  (Timeout on second line)");
        test_print_crlf();
        test_puts("PASS");
        test_print_crlf();
        return TEST_PASS;
    }

    test_puts("  Second line: '");
    test_puts(buffer);
    test_puts("'");
    test_print_crlf();

    return TEST_PASS;
}

/* Test suite array */
TEST_SUITE(09_uart_esp,
           at_command,
           disable_echo,
           read_response);
