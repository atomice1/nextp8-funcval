/* Watchdog Timer for nextp8 Functional Validation Library
 * Interrupt-based timeout mechanism using VBlank interrupts
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "nextp8.h"
#include "mmio.h"
#include "funcval.h"

/* Watchdog state variables */
uint32_t watchdog_timeout_us = 0;
uint32_t watchdog_start_time_hi = 0;
uint32_t watchdog_start_time_lo = 0;
void (*watchdog_handler)(void) = 0;
uint8_t watchdog_active = 0;

/* watchdog_start - Start or restart the watchdog timer
 * Records current time, enables VBlank interrupts, sets timeout handler
 *
 * Inputs:
 *   timeout_us = timeout in microseconds
 *   handler = pointer to timeout handler function (called as interrupt handler)
 */
void watchdog_start(uint32_t timeout_us, void (*handler)(void))
{
    /* Save configuration */
    watchdog_timeout_us = timeout_us;
    watchdog_handler = handler;

    /* Get current time */
    uint64_t current_time = timer_get_us();
    watchdog_start_time_hi = (uint32_t)(current_time >> 32);
    watchdog_start_time_lo = (uint32_t)(current_time & 0xFFFFFFFF);

    /* Mark watchdog as active */
    watchdog_active = 1;

    /* Enable VBlank interrupt in hardware */
    MMIO_REG8(_VBLANK_INTR_CTRL) = 0x01;  /* Bit 0 = enable interrupt */

    /* Enable interrupts in CPU (set interrupt mask to level 1) */
    /* This allows level 2+ interrupts (VBlank is level 2) */
    __asm__ volatile (
        "andi.w #0xF8FF, %%sr\n"   /* Clear interrupt mask */
        "ori.w  #0x0100, %%sr\n"   /* Set mask to level 1 */
        : : : "cc"
    );
}

/* watchdog_restart - Restart the watchdog with current timeout
 * Updates start time, keeps existing timeout and handler
 */
void watchdog_restart(void)
{
    /* Get current time */
    uint64_t current_time = timer_get_us();
    watchdog_start_time_hi = (uint32_t)(current_time >> 32);
    watchdog_start_time_lo = (uint32_t)(current_time & 0xFFFFFFFF);
}

/* watchdog_set_timeout - Update watchdog timeout while running
 * Changes timeout value without resetting start time
 */
void watchdog_set_timeout(uint32_t timeout_us)
{
    watchdog_timeout_us = timeout_us;
}

/* watchdog_stop - Stop the watchdog timer
 * Disables VBlank interrupts
 */
void watchdog_stop(void)
{
    /* Mark watchdog as inactive */
    watchdog_active = 0;

    /* Disable VBlank interrupt in hardware */
    MMIO_REG8(_VBLANK_INTR_CTRL) = 0x00;  /* Bit 0 = 0 to disable */
}

/* watchdog_vblank_handler - VBlank interrupt handler for watchdog
 * Should be installed as the VBlank (Level 2) interrupt handler
 * Checks elapsed time and calls timeout handler if needed
 *
 * This is installed in the interrupt vector table at vector 26 (Level 2 autovector)
 */
__attribute__((interrupt)) void watchdog_vblank_handler(void)
{
    /* Check if watchdog is active */
    if (!watchdog_active)
        goto done;

    /* Get current time */
    uint64_t current_time = timer_get_us();
    uint32_t current_hi = (uint32_t)(current_time >> 32);
    uint32_t current_lo = (uint32_t)(current_time & 0xFFFFFFFF);

    /* Calculate elapsed time: current - start */
    uint32_t elapsed_hi, elapsed_lo;

    /* Low: elapsed_low = current_low - start_low */
    elapsed_lo = current_lo - watchdog_start_time_lo;

    /* High: elapsed_high = current_high - start_high - borrow */
    elapsed_hi = current_hi - watchdog_start_time_hi;
    if (current_lo < watchdog_start_time_lo)
        elapsed_hi--;  /* Borrow from high word */

    /* Compare with timeout (only check low 32 bits for simplicity) */
    /* If high 32 bits of elapsed > 0, definitely timed out */
    if (elapsed_hi != 0 || elapsed_lo >= watchdog_timeout_us) {
        /* Timeout occurred - stop watchdog and call handler */
        watchdog_stop();

        /* Call timeout handler if set */
        if (watchdog_handler)
            watchdog_handler();
    }

done:
    /* Acknowledge VBlank interrupt */
    MMIO_REG8(_VBLANK_INTR_CTRL) = 0x01;  /* Write bit 0 to acknowledge */
}

/* watchdog_check - Manual check if watchdog has timed out
 * Can be called from main loop to check timeout without waiting for interrupt
 *
 * Returns: 0 if not timed out, 1 if timed out
 */
int watchdog_check(void)
{
    /* Check if watchdog is active */
    if (!watchdog_active)
        return 0;

    /* Get current time */
    uint64_t current_time = timer_get_us();
    uint32_t current_hi = (uint32_t)(current_time >> 32);
    uint32_t current_lo = (uint32_t)(current_time & 0xFFFFFFFF);

    /* Calculate elapsed time */
    uint32_t elapsed_hi = current_hi - watchdog_start_time_hi;
    uint32_t elapsed_lo = current_lo - watchdog_start_time_lo;
    if (current_lo < watchdog_start_time_lo)
        elapsed_hi--;  /* Borrow from high word */

    /* Compare with timeout */
    if (elapsed_hi != 0 || elapsed_lo >= watchdog_timeout_us)
        return 1;

    return 0;
}
