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
#define DA_CLOCK_FREQ    65000000  /* 65 MHz */
#define DA_SAMPLE_COUNT  (_DA_MEMORY_SIZE / sizeof(int16_t))  /* 8192 samples */
#define DA_HALF          (DA_SAMPLE_COUNT / 2)               /* 4096 - half-buffer size */
#define DA_NOTE_MS       250                                 /* ms per note */
#define DA_GEN_MAX_NOTES 16

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

/* -----------------------------------------------------------------------
 * Streaming sample generator for double-buffer playback.
 *
 * Uses a continuous Q32 phase accumulator across note boundaries to avoid
 * the inter-note clicks that a per-note phase reset would cause.
 *
 * da_gen_fill_mono / da_gen_fill_stereo write exactly `n` samples into
 * `buf`, padding with silence once all notes are exhausted.
 * ----------------------------------------------------------------------- */
typedef struct {
    unsigned sample_rate;
    uint32_t phase_acc;
    uint32_t phase_inc;         /* current note: freq * 2^32 / sample_rate */
    unsigned freqs[DA_GEN_MAX_NOTES];
    unsigned note_count;        /* total notes in sequence */
    unsigned note_idx;          /* index of note currently playing */
    unsigned samples_per_note;
    unsigned samples_in_note;   /* samples generated for current note */
    unsigned total_remaining;   /* total samples left to generate (0 = exhausted) */
} da_gen_t;

static void da_gen_init(da_gen_t *g, unsigned sample_rate,
                        const unsigned *freqs, unsigned note_count)
{
    unsigned i;
    g->sample_rate      = sample_rate;
    g->phase_acc        = 0;
    g->note_count       = note_count;
    g->note_idx         = 0;
    g->samples_in_note  = 0;
    g->samples_per_note = (sample_rate * DA_NOTE_MS) / 1000;
    g->total_remaining  = g->samples_per_note * note_count;
    for (i = 0; i < note_count; i++)
        g->freqs[i] = freqs[i];
    /* Initial phase increment for first note */
    g->phase_inc = (uint32_t)(((uint64_t)freqs[0] << 32) / sample_rate);
}

static void da_gen_fill_mono(da_gen_t *g, volatile int16_t *buf, unsigned n)
{
    unsigned i;
    for (i = 0; i < n; i++) {
        if (g->total_remaining == 0) {
            buf[i] = 0;  /* silence padding */
            continue;
        }
        /* Advance to next note when current note is complete */
        if (g->samples_in_note >= g->samples_per_note) {
            g->note_idx++;
            g->samples_in_note = 0;
            g->phase_inc = (uint32_t)(((uint64_t)g->freqs[g->note_idx] << 32)
                                      / g->sample_rate);
        }
        uint16_t phase = (uint16_t)(g->phase_acc >> 16);
        int16_t s = triangle_sample(phase);
        s >>= 1;  /* half amplitude for headroom */
        buf[i] = s;
        g->phase_acc += g->phase_inc;
        g->samples_in_note++;
        g->total_remaining--;
    }
}

static void da_gen_fill_stereo(da_gen_t *g, volatile int16_t *buf, unsigned n)
{
    unsigned i;
    for (i = 0; i < n; i++) {
        if (g->total_remaining == 0) {
            /* Silence: both channels at midpoint */
            buf[i] = (int16_t)((128 << 8) | 128);
            continue;
        }
        /* Advance to next note when current note is complete */
        if (g->samples_in_note >= g->samples_per_note) {
            g->note_idx++;
            g->samples_in_note = 0;
            g->phase_inc = (uint32_t)(((uint64_t)g->freqs[g->note_idx] << 32)
                                      / g->sample_rate);
        }
        /* Pan: ascending notes sweep L→R, descending sweep R→L.
         * pan=0 → full left, pan=256 → full right. */
        unsigned ni = g->note_idx;
        unsigned pan;
        if (ni < SCALE_LENGTH) {
            pan = (ni * 256) / (SCALE_LENGTH - 1);
        } else {
            unsigned desc     = ni - SCALE_LENGTH;
            unsigned desc_tot = g->note_count - SCALE_LENGTH;
            pan = (desc_tot > 1)
                ? 256 - ((desc * 256) / (desc_tot - 1))
                : 128;
            if (pan > 256) pan = 256;
        }
        /* Generate waveform sample */
        uint16_t phase = (uint16_t)(g->phase_acc >> 16);
        int16_t raw = triangle_sample(phase);
        raw >>= 1;  /* half amplitude */
        g->phase_acc += g->phase_inc;
        g->samples_in_note++;
        g->total_remaining--;
        /* Convert signed deviation to unsigned 8-bit per channel.
         * dev=0 → both channels at midpoint (128 = silence in unsigned 8-bit). */
        int dev   = (int)raw >> 8;  /* signed -128..127 */
        int left  = ((dev * (256 - pan)) >> 8) + 128;
        int right = ((dev * pan)         >> 8) + 128;
        if (left  < 0) left  = 0; else if (left  > 255) left  = 255;
        if (right < 0) right = 0; else if (right > 255) right = 255;
        /* Hardware: da_data[7:0]=left, da_data[15:8]=right */
        buf[i] = (int16_t)((right << 8) | (left & 0xFF));
    }
}

/* Forward declaration — defined after test_address_advance */
static int wait_for_audio(void);

/* Double-buffer playback loop.
 *
 * Fills both halves of DA memory before starting, then watches da_address:
 *   - When addr crosses DA_HALF (0→1 transition): refill lower half.
 *   - When addr wraps to 0    (1→0 transition): refill upper half.
 * Continues until the generator is exhausted, then calls wait_for_audio(). */
static int play_double_buffer(da_gen_t *g, int is_stereo)
{
    volatile int16_t *da_mem = (volatile int16_t *)_DA_MEMORY_BASE;
    unsigned ctrl = DA_CTRL_START | (is_stereo ? 0u : (unsigned)DA_CTRL_MONO);

    /* Initial fill: populate both halves before starting playback */
    if (is_stereo) {
        da_gen_fill_stereo(g, da_mem,           DA_HALF);
        da_gen_fill_stereo(g, da_mem + DA_HALF, DA_HALF);
    } else {
        da_gen_fill_mono(g, da_mem,           DA_HALF);
        da_gen_fill_mono(g, da_mem + DA_HALF, DA_HALF);
    }

    MMIO_REG16(_DA_CONTROL) = ctrl;

    /* Simulation has no real DA hardware; return immediately */
    if (platform_is_simulation)
        return TEST_PASS;

    /* Double-buffer loop: track which half the DA is currently playing.
     * Whenever it moves to the other half, refill the half it just left. */
    int active_half = 0;  /* half currently being played by DA */
    while (g->total_remaining > 0) {
        uint16_t addr     = MMIO_REG16(_DA_ADDRESS);
        int      curr_half = (addr >= DA_HALF) ? 1 : 0;
        if (curr_half != active_half) {
            /* DA moved to curr_half; refill the half it just finished */
            volatile int16_t *refill = da_mem + (unsigned)active_half * DA_HALF;
            if (is_stereo) da_gen_fill_stereo(g, refill, DA_HALF);
            else           da_gen_fill_mono  (g, refill, DA_HALF);
            active_half = curr_half;
        }
    }
    /* Generator exhausted. The last refilled half may still be playing.
     * Wait for DA to move to the other half so that half has started. */
    {
        uint64_t t = MMIO_REG64(_UTIMER_1MHZ) + 500000ULL;  /* 500 ms max */
        while (1) {
            uint16_t addr = MMIO_REG16(_DA_ADDRESS);
            if (((addr >= DA_HALF) ? 1 : 0) != active_half) break;
            if (MMIO_REG64(_UTIMER_1MHZ) > t) break;
        }
    }

    return wait_for_audio();
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

    /* Now watch for a wrap for up to 2 full buffer cycles (2 * ~371ms = ~742ms).
     * da_address only ever increments, so any decrease signals a wrap. */
    uint16_t prev_addr = MMIO_REG16(_DA_ADDRESS);
    int wrap_detected = 0;
    uint64_t poll_start = MMIO_REG64(_UTIMER_1MHZ);

    while ((MMIO_REG64(_UTIMER_1MHZ) - poll_start) < 750000) {
        uint16_t curr_addr = MMIO_REG16(_DA_ADDRESS);

        /* da_address only increments; any decrease is a wrap */
        if (curr_addr < prev_addr) {
            wrap_detected = 1;
            test_puts("wrapped from ");
            uart_print_hex_word(prev_addr);
            test_puts(" to ");
            uart_print_hex_word(curr_addr);
            test_print_crlf();
            break;
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

    return TEST_PASS;
}

/* Helper: stop DA playback - used as per-test cleanup */
static void cleanup_stop_playback(void)
{
    MMIO_REG16(_DA_CONTROL) = 0x0000;
}

/* Helper: wait for audio to finish or be evaluated.
 * Interactive: wait for Y (pass) or N (fail) keypress.
 * Model (non-interactive, non-simulation): play for 2 seconds.
 * Simulation: return immediately. */
static int wait_for_audio(void)
{
    if (platform_is_interactive) {
        test_puts("  Press Y to pass or N to fail: ");
        test_print_crlf();
        screen_flip();
        for (;;) {
            char c = read_keyboard_char();
            if (c == 'y' || c == 'Y' || c == '\r' || c == '\n')
                return TEST_PASS;
            if (c == 'n' || c == 'N' || c == 27 /* ESC */)
                return TEST_FAIL;
        }
    } else if (platform_is_simulation) {
        /* No wait in simulation */
        return TEST_PASS;
    } else {
        /* Hardware / model: play for 2 seconds */
        screen_flip();
        usleep(2000000);
        return TEST_PASS;
    }
}

/* Test 2: Play C major scale ascending+descending at 22.05 kHz (mono) */
static int test_scale_22050hz(void)
{
    unsigned notes[DA_GEN_MAX_NOTES];
    unsigned note_count = 0;
    unsigned i;
    da_gen_t gen;

    test_puts("  Playing C major scale (asc+desc) at 22.05 kHz... ");
    test_print_crlf();

    /* Build ascending then descending sequence */
    for (i = 0; i < SCALE_LENGTH; i++)
        notes[note_count++] = scale_freqs[i];
    for (i = SCALE_LENGTH - 1; i > 0; i--)
        notes[note_count++] = scale_freqs[i - 1];

    MMIO_REG16(_DA_PERIOD) = (DA_CLOCK_FREQ + 22050 / 2) / 22050;

    da_gen_init(&gen, 22050, notes, note_count);
    return play_double_buffer(&gen, 0 /* mono */);
}

/* Test 3: Play C major scale ascending+descending at 5.5125 kHz (mono) */
static int test_scale_5512hz(void)
{
    unsigned notes[DA_GEN_MAX_NOTES];
    unsigned note_count = 0;
    unsigned i;
    da_gen_t gen;

    test_puts("  Playing C major scale (asc+desc) at 5.5125 kHz (mono)... ");
    test_print_crlf();

    for (i = 0; i < SCALE_LENGTH; i++)
        notes[note_count++] = scale_freqs[i];
    for (i = SCALE_LENGTH - 1; i > 0; i--)
        notes[note_count++] = scale_freqs[i - 1];

    MMIO_REG16(_DA_PERIOD) = (DA_CLOCK_FREQ + 5512 / 2) / 5512;

    da_gen_init(&gen, 5512, notes, note_count);
    return play_double_buffer(&gen, 0 /* mono */);
}

/* Test 4: Play C major scale ascending+descending with stereo panning */
static int test_stereo_panning(void)
{
    unsigned notes[DA_GEN_MAX_NOTES];
    unsigned note_count = 0;
    unsigned i;
    da_gen_t gen;

    test_puts("  Playing scale with stereo panning... ");
    test_print_crlf();

    for (i = 0; i < SCALE_LENGTH; i++)
        notes[note_count++] = scale_freqs[i];
    for (i = SCALE_LENGTH - 1; i > 0; i--)
        notes[note_count++] = scale_freqs[i - 1];

    MMIO_REG16(_DA_PERIOD) = (DA_CLOCK_FREQ + 22050 / 2) / 22050;

    da_gen_init(&gen, 22050, notes, note_count);
    return play_double_buffer(&gen, 1 /* stereo */);
}

/* Test suite */
TEST_SUITE_SETUP_CLEANUP(10_digital_audio, NULL, NULL,
    TEST_CASE_SETUP_CLEANUP(address_advance, NULL, cleanup_stop_playback),
    TEST_CASE_SETUP_CLEANUP(scale_22050hz,   NULL, cleanup_stop_playback),
    TEST_CASE_SETUP_CLEANUP(scale_5512hz,    NULL, cleanup_stop_playback),
    TEST_CASE_SETUP_CLEANUP(stereo_panning,  NULL, cleanup_stop_playback));
