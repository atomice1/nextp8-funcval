/* Timer and Delay Functions for nextp8 Functional Validation Library
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "nextp8.h"
#include "mmio.h"
#include "funcval.h"

/* timer_get_us - Read 64-bit microsecond timer
 * Returns current timer value (1 MHz, 64-bit counter)
 */
uint64_t timer_get_us(void)
{
    return MMIO_REG64(_UTIMER_1MHZ);
}

/* timer_diff_us - Calculate difference between two timer values
 * Returns (end_time - start_time) as 64-bit value
 */
uint64_t timer_diff_us(uint64_t start, uint64_t end)
{
    return end - start;
}

/* usleep - Delay for specified microseconds
 * Uses 1 MHz timer for accurate delay
 */
void usleep(uint32_t microseconds)
{
    uint64_t start, target, current;

    start = timer_get_us();
    target = start + microseconds;

    do {
        current = timer_get_us();
    } while (current < target);
}

/* delay_10us - Delay approximately 10 microseconds
 * Approximate delay using tight loop (less accurate than usleep)
 */
void delay_10us(void)
{
    /* At~33MHz, ~330 cycles for 10us */
    /* Each loop iteration is ~3-4 cycles, so ~80-100 iterations */
    for (volatile int i = 0; i < 80; i++)
        ;
}
