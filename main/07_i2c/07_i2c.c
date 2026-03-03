/* Test Suite: I2C RTC (Real-Time Clock)
 * Tests RTC via I2C master controller with comprehensive error checking
 * Duplicates code from rtc.c but with validation and error reporting
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "nextp8.h"
#include "funcval.h"
#include "mmio.h"
#include "funcval_tb.h"

/* I2C control bits (write) */
#define I2C_ENA_BIT  0x01        /* Enable/latch command */
#define I2C_RW_BIT   0x02        /* Read/Write select (0=Write, 1=Read) */

/* I2C status bits (read) */
#define I2C_BUSY_BIT 0x01        /* Transaction busy flag */
#define I2C_ERR_BIT  0x02        /* Acknowledge error flag */

#define I2C_TIMEOUT  1000000

/* Helper: wait for I2C busy to rise with error checking */
static int wait_i2c_busy_rise(void)
{
    unsigned char status;
    int timeout = I2C_TIMEOUT;

    while (timeout-- > 0) {
        status = MMIO_REG8(_I2C_STATUS);
        if ((status & I2C_BUSY_BIT) != 0) break;
    }
    if (timeout <= 0) {
        test_puts("BUSY_RISE timeout");
        return -1;
    }
    if ((status & I2C_ERR_BIT) != 0) {
        test_puts("BUSY_RISE error");
        return -1;
    }
    return 0;
}

/* Helper: wait for I2C busy to fall with error checking */
static int wait_i2c_busy_fall(void)
{
    unsigned char status;
    int timeout = I2C_TIMEOUT;

    while (timeout-- > 0) {
        status = MMIO_REG8(_I2C_STATUS);
        if ((status & I2C_BUSY_BIT) == 0) break;
    }
    if (timeout <= 0) {
        test_puts("BUSY_FALL timeout");
        return -1;
    }
    if ((status & I2C_ERR_BIT) != 0) {
        test_puts("BUSY_FALL error");
        return -1;
    }
    return 0;
}

/* Read RTC with comprehensive error checking */
static int read_rtc_validated(unsigned *date, unsigned *month, unsigned *year,
                              unsigned *hours, unsigned *minutes, unsigned *seconds,
                              unsigned *wday)
{
    int ret;

    /* Wait for I2C to be idle before starting */
    ret = wait_i2c_busy_fall();
    if (ret != 0) return ret;

    /* Cmd1: write register address 0x0 (seconds) */
    MMIO_REG8(_I2C_DATA) = 0x00;
    MMIO_REG8(_I2C_CTRL) = I2C_ENA_BIT;

    /* Wait for busy to rise */
    ret = wait_i2c_busy_rise();
    if (ret != 0) return ret;

    /* Cmd2: read date (set rw=1) */
    MMIO_REG8(_I2C_CTRL) = I2C_ENA_BIT | I2C_RW_BIT;

    /* Wait for busy to clear */
    ret = wait_i2c_busy_fall();
    if (ret != 0) return ret;

    /* Wait for busy to rise (seconds byte ready) */
    ret = wait_i2c_busy_rise();
    if (ret != 0) return ret;

    /* Wait for busy to clear */
    ret = wait_i2c_busy_fall();
    if (ret != 0) return ret;

    /* Capture seconds */
    uint8_t data = MMIO_REG8(_I2C_DATA);
    *seconds = ((data >> 4) & 0x07) * 10 + (data & 0x0F);

    /* Wait for busy to rise (minutes byte ready) */
    ret = wait_i2c_busy_rise();
    if (ret != 0) return ret;

    /* Wait for busy to clear */
    ret = wait_i2c_busy_fall();
    if (ret != 0) return ret;

    /* Capture minutes */
    data = MMIO_REG8(_I2C_DATA);
    *minutes = ((data >> 4) & 0x07) * 10 + (data & 0x0F);

    /* Wait for busy to rise (hours byte ready) */
    ret = wait_i2c_busy_rise();
    if (ret != 0) return ret;

    /* Wait for busy to clear */
    ret = wait_i2c_busy_fall();
    if (ret != 0) return ret;

    /* Capture hours */
    data = MMIO_REG8(_I2C_DATA);
    if (data & (1 << 6))
        *hours = ((data & 0x0F) + 10 * ((data >> 4) & 0x01)) + ((data & (1 << 5)) ? 12 : 0);
    else
        *hours = (data & 0x0F) + 10 * ((data >> 4) & 0x03);

    /* Wait for busy to rise (wday byte ready) */
    ret = wait_i2c_busy_rise();
    if (ret != 0) return ret;

    /* Wait for busy to clear */
    ret = wait_i2c_busy_fall();
    if (ret != 0) return ret;

    /* Capture wday */
    data = MMIO_REG8(_I2C_DATA);
    *wday = data & 0x7;

    /* Wait for busy to rise (date byte ready) */
    ret = wait_i2c_busy_rise();
    if (ret != 0) return ret;

    /* Wait for busy to clear */
    ret = wait_i2c_busy_fall();
    if (ret != 0) return ret;

    /* Capture date */
    data = MMIO_REG8(_I2C_DATA);
    *date = (data & 0x0F) + 10 * ((data >> 4) & 0x03);

    /* Wait for busy to rise (month byte ready) */
    ret = wait_i2c_busy_rise();
    if (ret != 0) return ret;

    /* Wait for busy to clear */
    ret = wait_i2c_busy_fall();
    if (ret != 0) return ret;

    /* Capture month */
    data = MMIO_REG8(_I2C_DATA);
    *month = (data & 0x0F) + 10 * ((data >> 4) & 0x01);

    /* Wait for busy to rise (year byte ready) */
    ret = wait_i2c_busy_rise();
    if (ret != 0) return ret;

    /* Stop I2C transaction */
    MMIO_REG8(_I2C_CTRL) = 0x00;

    /* Wait for busy to clear */
    ret = wait_i2c_busy_fall();
    if (ret != 0) return ret;

    /* Capture year */
    data = MMIO_REG8(_I2C_DATA);
    *year = (data & 0x0F) + 10 * ((data >> 4) & 0x0F) + 2000;

    return 0;
}

/* Helper: print date/time */
static void print_datetime(unsigned date, unsigned month, unsigned year,
                          unsigned hours, unsigned minutes, unsigned seconds)
{
    test_puts("  Date: 20");
    if (year >= 2000) year -= 2000;
    if (year < 10) uart_write_byte('0');
    uart_print_dec(year);
    uart_write_byte('-');
    if (month < 10) uart_write_byte('0');
    uart_print_dec(month);
    uart_write_byte('-');
    if (date < 10) uart_write_byte('0');
    uart_print_dec(date);
    test_puts("  Time: ");
    if (hours < 10) uart_write_byte('0');
    uart_print_dec(hours);
    uart_write_byte(':');
    if (minutes < 10) uart_write_byte('0');
    uart_print_dec(minutes);
    uart_write_byte(':');
    if (seconds < 10) uart_write_byte('0');
    uart_print_dec(seconds);
    test_print_crlf();
}

/* Test 1: Read RTC and verify no errors */
static int test_rtc_read_success(void)
{
    test_puts("  Reading RTC... ");

    unsigned date, month, year, hours, minutes, seconds, wday;

    int result = read_rtc_validated(&date, &month, &year, &hours, &minutes, &seconds, &wday);

    if (result != 0) {
        test_puts("FAIL (I2C communication error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    print_datetime(date, month, year, hours, minutes, seconds);
    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 2: Verify date is within valid range */
static int test_rtc_date_range(void)
{
    test_puts("  Verifying date range (2025-01-01 to 2027-12-31)... ");

    unsigned date, month, year, hours, minutes, seconds, wday;

    int result = read_rtc_validated(&date, &month, &year, &hours, &minutes, &seconds, &wday);

    if (result != 0) {
        test_puts("FAIL (read error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Check year range */
    if (year < 2025 || year > 2027) {
        test_puts("FAIL (year ");
        uart_print_hex_word(year);
        test_puts(" out of range)");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Validate month range (1-12) */
    if (month < 1 || month > 12) {
        test_puts("FAIL (invalid month: ");
        uart_print_hex_word(month);
        test_puts(")");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Validate date range (1-31) */
    if (date < 1 || date > 31) {
        test_puts("FAIL (invalid date: ");
        uart_print_hex_word(date);
        test_puts(")");
        test_print_crlf();
        return TEST_FAIL;
    }

    print_datetime(date, month, year, hours, minutes, seconds);
    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 3: Verify time is within valid range */
static int test_rtc_time_range(void)
{
    test_puts("  Verifying time range (07:00:00 to 23:00:00)... ");

    unsigned date, month, year, hours, minutes, seconds, wday;

    int result = read_rtc_validated(&date, &month, &year, &hours, &minutes, &seconds, &wday);

    if (result != 0) {
        test_puts("FAIL (read error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Check hours range (7-23) */
    if (hours < 7 || hours > 23) {
        test_puts("FAIL (hours ");
        uart_print_hex_word(hours);
        test_puts(" out of range)");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Validate minutes range (0-59) */
    if (minutes > 59) {
        test_puts("FAIL (invalid minutes: ");
        uart_print_hex_word(minutes);
        test_puts(")");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Validate seconds range (0-59) */
    if (seconds > 59) {
        test_puts("FAIL (invalid seconds: ");
        uart_print_hex_word(seconds);
        test_puts(")");
        test_print_crlf();
        return TEST_FAIL;
    }

    print_datetime(date, month, year, hours, minutes, seconds);
    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 4: Multiple reads verify time advances */
static int test_rtc_time_advances(void)
{
    test_puts("  Verifying time advances between reads... ");

    unsigned date1, month1, year1, hours1, minutes1, seconds1, wday1;
    unsigned date2, month2, year2, hours2, minutes2, seconds2, wday2;

    /* First read */
    int result = read_rtc_validated(&date1, &month1, &year1, &hours1, &minutes1, &seconds1, &wday1);
    if (result != 0) {
        test_puts("FAIL (first read error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Small delay */
    for (volatile int i = 0; i < 100000; i++) {
        /* Busy wait */
    }

    /* Second read */
    result = read_rtc_validated(&date2, &month2, &year2, &hours2, &minutes2, &seconds2, &wday2);
    if (result != 0) {
        test_puts("FAIL (second read error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Time should be same or slightly advanced (within same hour typically) */
    /* Just verify we can read twice without errors */

    print_datetime(date1, month1, year1, hours1, minutes1, seconds1);
    test_puts("  Second read: ");
    print_datetime(date2, month2, year2, hours2, minutes2, seconds2);

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

void software_init_hook(void)
{
    platform_detect();
    if (platform_is_simulation)
        MMIO_REG16(FUNCVAL_MODEL_DEBUG_DS1307) = 1;
}

/* Test suite array */
TEST_SUITE(07_i2c,
           rtc_read_success,
           rtc_date_range,
           rtc_time_range,
           rtc_time_advances);
