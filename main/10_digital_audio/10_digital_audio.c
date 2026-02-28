/* Test Suite: Digital Audio (DA) Controller
 * Tests MMIO registers for digital audio PWM output
 * Can play C major scale at different sample rates
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "nextp8.h"
#include "funcval.h"
#include "mmio.h"

/* DA controller constants */
#define DA_CLOCK_FREQ 65000000  /* 65 MHz */

/* DA_CONTROL bits */
#define DA_CTRL_START  0x0001  /* bit 0: start (1) / stop (0) */
#define DA_CTRL_MONO   0x0100  /* bit 8: mono (1) / stereo (0) */

/* C major scale frequencies (Hz) - Middle C octave to one octave higher */
static const unsigned int scale_freqs[] = {
    262,   /* C4 */
    294,   /* D4 */
    330,   /* E4 */
    349,   /* F4 */
    392,   /* G4 */
    440,   /* A4 */
    494,   /* B4 */
    523,   /* C5 */
};
#define SCALE_LENGTH (sizeof(scale_freqs) / sizeof(scale_freqs[0]))

/* Helper: Generate triangle wave sample at phase (0-65535) */
static int16_t triangle_sample(uint16_t phase)
{
    /* Use phase as 0-65535 for full cycle, convert to signed amplitude */
    if (phase < 32768) {
        /* Rising slope: 0 to 32767 */
        return (int16_t)((phase << 1) - 32768);
    } else {
        /* Falling slope: 32767 to 0 */
        return (int16_t)(65536 - (phase << 1));
    }
}

/* Helper: Fill DA memory with C major scale at given sample rate */
static int fill_scale_buffer(unsigned sample_rate, int ascending_then_descending)
{
    volatile int16_t *da_mem = (volatile int16_t *)_DA_MEMORY_BASE;
    unsigned notes_to_play[16];
    unsigned note_count = 0;
    unsigned i, j;

    /* Build note sequence: ascending scale */
    for (i = 0; i < SCALE_LENGTH; i++) {
        notes_to_play[note_count++] = scale_freqs[i];
    }

    /* Then descending (skip highest C, go back down) */
    if (ascending_then_descending) {
        for (i = SCALE_LENGTH - 1; i > 0; i--) {
            notes_to_play[note_count++] = scale_freqs[i - 1];
        }
    }

    /* Generate samples: 1/4 second per note */
    unsigned samples_per_note = (sample_rate + 2) / 4;  /* sample_rate / 4, rounded */
    unsigned total_samples = samples_per_note * note_count;

    if (total_samples * 2 > _DA_MEMORY_SIZE) {
        test_puts("  ERROR: Scale buffer too large for DA memory");
        test_print_crlf();
        return -1;
    }

    unsigned sample_idx = 0;
    for (i = 0; i < note_count; i++) {
        unsigned note_freq = notes_to_play[i];
        unsigned samples_per_cycle = (sample_rate + note_freq / 2) / note_freq;

        if (samples_per_cycle == 0) samples_per_cycle = 1;

        for (j = 0; j < samples_per_note; j++) {
            if (sample_idx >= total_samples) break;

            uint16_t phase = ((j % samples_per_cycle) * 65536) / samples_per_cycle;
            int16_t sample = triangle_sample(phase);

            /* Scale amplitude to avoid clipping, and add DC bias */
            sample = ((sample >> 1) + 16384);  /* Shift right to reduce amplitude, add bias */

            da_mem[sample_idx++] = sample;
        }
    }

    return total_samples;
}

/* Helper: Fill DA memory with stereo panning scale */
static int fill_scale_buffer_stereo(unsigned sample_rate)
{
    volatile int16_t *da_mem = (volatile int16_t *)_DA_MEMORY_BASE;
    unsigned notes_to_play[16];
    unsigned note_count = 0;
    unsigned i, j;

    /* Build note sequence: ascending scale */
    for (i = 0; i < SCALE_LENGTH; i++) {
        notes_to_play[note_count++] = scale_freqs[i];
    }

    /* Then descending (skip highest C) */
    for (i = SCALE_LENGTH - 1; i > 0; i--) {
        notes_to_play[note_count++] = scale_freqs[i - 1];
    }

    /* Generate samples: 1/4 second per note */
    unsigned samples_per_note = (sample_rate + 2) / 4;
    unsigned total_samples = samples_per_note * note_count;

    if (total_samples * 2 > _DA_MEMORY_SIZE) {
        test_puts("  ERROR: Scale buffer too large for DA memory");
        test_print_crlf();
        return -1;
    }

    unsigned sample_idx = 0;
    for (i = 0; i < note_count; i++) {
        unsigned note_freq = notes_to_play[i];
        unsigned samples_per_cycle = (sample_rate + note_freq / 2) / note_freq;

        if (samples_per_cycle == 0) samples_per_cycle = 1;

        /* Calculate pan position (0-256): 0=full left, 256=full right */
        unsigned pan;
        if (i < SCALE_LENGTH) {
            /* Ascending: pan left to right */
            pan = (i * 256) / (SCALE_LENGTH - 1);
        } else {
            /* Descending: pan right to left */
            unsigned desc_idx = i - SCALE_LENGTH;
            pan = 256 - ((desc_idx * 256) / (note_count - SCALE_LENGTH - 1));
        }

        for (j = 0; j < samples_per_note; j++) {
            if (sample_idx >= total_samples) break;

            uint16_t phase = ((j % samples_per_cycle) * 65536) / samples_per_cycle;
            int16_t sample = triangle_sample(phase);

            /* Scale amplitude and convert to 8-bit (0-255) */
            int sample_8bit = ((sample >> 8) + 128);  /* Convert -32768..32767 to 0..255 */

            /* Apply panning: left channel gets (256-pan), right gets pan */
            int left = (sample_8bit * (256 - pan)) >> 8;
            int right = (sample_8bit * pan) >> 8;

            /* Stereo format: LSB 8 bits = left, MSB 8 bits = right */
            da_mem[sample_idx++] = (int16_t)((right << 8) | (left & 0xFF));
        }
    }

    return total_samples;
}

/* Test 1: Verify DA_ADDRESS advances at expected rate */
static int test_address_advance(void)
{
    test_puts("  Testing DA_ADDRESS behavior... ");
    test_print_crlf();

    /* Set period for 22.05 kHz */
    unsigned period = (DA_CLOCK_FREQ + 22050 / 2) / 22050;
    MMIO_REG16(_DA_PERIOD) = period;

    /* Fill buffer with simple pattern */
    volatile int16_t *da_mem = (volatile int16_t *)_DA_MEMORY_BASE;
    for (unsigned i = 0; i < 8192; i++) {
        da_mem[i] = 16384;  /* Mid-level sample */
    }

    /* Test 1a: Verify address does NOT advance when stopped (bit 0 = 0) */
    test_puts("    Checking address doesn't advance when stopped... ");
    MMIO_REG16(_DA_CONTROL) = 0x0000;  /* Ensure stopped */

    uint16_t addr_before = MMIO_REG16(_DA_ADDRESS);

    /* Wait a bit */
    for (volatile unsigned i = 0; i < 1000000; i++) {
        /* Busy wait */
    }

    uint16_t addr_after = MMIO_REG16(_DA_ADDRESS);

    if (addr_after != addr_before) {
        test_puts("FAIL (advanced while stopped)");
        test_print_crlf();
        return TEST_FAIL;
    }
    test_puts("OK");
    test_print_crlf();

    /* Test 1b: Verify address advances at correct rate when playing */
    test_puts("    Checking advance rate during playback... ");

    /* Start playback in mono mode */
    MMIO_REG16(_DA_CONTROL) = DA_CTRL_START | DA_CTRL_MONO;

    /* Read initial address */
    uint64_t start_time = MMIO_REG64(_UTIMER_1MHZ);
    uint16_t start_addr = MMIO_REG16(_DA_ADDRESS);

    /* Wait approximately 100ms */
    while ((MMIO_REG64(_UTIMER_1MHZ) - start_time) < 100000) {
        /* Busy wait */
    }

    /* Read final address */
    uint64_t end_time = MMIO_REG64(_UTIMER_1MHZ);
    uint16_t end_addr = MMIO_REG16(_DA_ADDRESS);

    /* Calculate expected and actual progression */
    uint64_t elapsed_us = end_time - start_time;
    unsigned expected_samples = (unsigned)((elapsed_us * 22050) / 1000000);
    unsigned actual_samples = (end_addr - start_addr) & 0xFFFF;

    test_puts("Expected ~");
    uart_print_hex_word(expected_samples);
    test_puts(", actual ");
    uart_print_hex_word(actual_samples);
    test_print_crlf();

    /* Check if within 10% tolerance */
    unsigned diff = (actual_samples > expected_samples) ?
                    (actual_samples - expected_samples) :
                    (expected_samples - actual_samples);

    if (diff > (expected_samples / 10)) {
        test_puts("FAIL (rate mismatch)");
        test_print_crlf();
        MMIO_REG16(_DA_CONTROL) = 0x0000;  /* Stop */
        return TEST_FAIL;
    }
    test_puts("    OK");
    test_print_crlf();

    /* Test 1c: Verify address stops advancing after stopping playback */
    test_puts("    Checking address stops after 1->0 transition... ");

    uint16_t addr_at_stop = MMIO_REG16(_DA_ADDRESS);

    /* Stop playback */
    MMIO_REG16(_DA_CONTROL) = 0x0000;

    /* Wait a bit */
    for (volatile unsigned i = 0; i < 1000000; i++) {
        /* Busy wait */
    }

    uint16_t addr_after_stop = MMIO_REG16(_DA_ADDRESS);

    if (addr_after_stop != addr_at_stop) {
        test_puts("FAIL (advanced after stop)");
        test_print_crlf();
        return TEST_FAIL;
    }
    test_puts("OK");
    test_print_crlf();

    /* Test 1d: Verify address wraps from 8191 -> 0 */
    test_puts("    Checking address wraps at 8191->0... ");

    /* Set address near wrap point by starting playback and waiting */
    MMIO_REG16(_DA_CONTROL) = DA_CTRL_START | DA_CTRL_MONO;

    /* Wait for address to approach 8191 (or wrap multiple times) */
    /* With 22.05kHz, 8192 samples = ~371ms. Wait longer to ensure wrap. */
    uint64_t wrap_start = MMIO_REG64(_UTIMER_1MHZ);
    while ((MMIO_REG64(_UTIMER_1MHZ) - wrap_start) < 500000) {
        /* Wait 500ms - should wrap at least once */
    }

    /* Now watch for a wrap in real-time */
    uint16_t prev_addr = MMIO_REG16(_DA_ADDRESS);
    int wrap_detected = 0;

    for (unsigned attempts = 0; attempts < 100000; attempts++) {
        uint16_t curr_addr = MMIO_REG16(_DA_ADDRESS);

        /* Check if we wrapped (address decreased) */
        if (curr_addr < prev_addr) {
            /* Verify we wrapped from near 8191 to near 0 */
            if (prev_addr >= 8100 && curr_addr <= 100) {
                wrap_detected = 1;
                test_puts("wrapped from ");
                uart_print_hex_word(prev_addr);
                test_puts(" to ");
                uart_print_hex_word(curr_addr);
                test_print_crlf();
                break;
            }
        }
        prev_addr = curr_addr;
    }

    /* Stop playback */
    MMIO_REG16(_DA_CONTROL) = 0x0000;

    if (!wrap_detected) {
        test_puts("FAIL (no wrap detected)");
        test_print_crlf();
        return TEST_FAIL;
    }
    test_puts("    OK");
    test_print_crlf();

    test_puts("  PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 2: Play C major scale at 22.05 kHz (mono) */
static int test_scale_22050hz(void)
{
    test_puts("  Playing C major scale at 22.05 kHz... ");

    /* Set baud rate divisor for 22.05 kHz */
    MMIO_REG16(_DA_PERIOD) = (DA_CLOCK_FREQ + 22050 / 2) / 22050;  /* ~2948 */

    /* Fill buffer with scale */
    int num_samples = fill_scale_buffer(22050, 1);
    if (num_samples < 0) {
        test_puts("FAIL");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Enable DA output in mono mode */
    MMIO_REG16(_DA_CONTROL) = DA_CTRL_START | DA_CTRL_MONO;

    /* Wait for playback (approximately 4 seconds for 8 notes * 0.5 seconds each) */
    for (volatile unsigned i = 0; i < 200000000; i++) {
        /* Busy wait ~4 seconds at 50M loops/sec */
    }

    /* Disable DA output */
    MMIO_REG16(_DA_CONTROL) = 0x0000;

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 3: Play C major scale at 5.5125 kHz (mono) */
static int test_scale_5512hz(void)
{
    test_puts("  Playing C major scale at 5.5125 kHz (mono)... ");

    /* Set period for 5.5125 kHz */
    unsigned period = (DA_CLOCK_FREQ + 5512 / 2) / 5512;  /* ~11792 */
    MMIO_REG16(_DA_PERIOD) = period;

    /* Fill buffer with scale */
    int num_samples = fill_scale_buffer(5512, 1);
    if (num_samples < 0) {
        test_puts("FAIL");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Enable DA output in mono mode */
    MMIO_REG16(_DA_CONTROL) = DA_CTRL_START | DA_CTRL_MONO;

    /* Wait for playback (approximately 4 seconds) */
    for (volatile unsigned i = 0; i < 200000000; i++) {
        /* Busy wait ~4 seconds */
    }

    /* Disable DA output */
    MMIO_REG16(_DA_CONTROL) = 0x0000;

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 4: Play C major scale with stereo panning */
static int test_stereo_panning(void)
{
    test_puts("  Playing scale with stereo panning... ");

    /* Set period for 22.05 kHz */
    MMIO_REG16(_DA_PERIOD) = (DA_CLOCK_FREQ + 22050 / 2) / 22050;

    /* Fill buffer with stereo panning scale */
    int num_samples = fill_scale_buffer_stereo(22050);
    if (num_samples < 0) {
        test_puts("FAIL");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Enable DA output in stereo mode (bit 8 = 0) */
    MMIO_REG16(_DA_CONTROL) = DA_CTRL_START;  /* Start without mono bit */

    /* Wait for playback (approximately 4 seconds) */
    for (volatile unsigned i = 0; i < 200000000; i++) {
        /* Busy wait ~4 seconds */
    }

    /* Disable DA output */
    MMIO_REG16(_DA_CONTROL) = 0x0000;

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test suite array */
TEST_SUITE(10_digital_audio,
           address_advance,
           scale_22050hz,
           scale_5512hz,
           stereo_panning);
