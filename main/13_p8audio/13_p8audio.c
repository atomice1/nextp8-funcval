/* Test Suite: PICO-8 Audio Subsystem
 * Functional validation of SFX and MUSIC APIs via MMIO
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "nextp8.h"
#include "funcval.h"
#include "funcval_tb.h"
#include "mmio.h"

#define SFX_BYTES 68
#define MUSIC_BYTES 4
#define SFX_COUNT 64
#define MUSIC_COUNT 64

#define SFX_CMD_FLAG 0x8000
#define SFX_IDX_STOP 0x3f
#define SFX_IDX_RELEASE 0x3e

#define CH_AUTO 0x7
#define CH_STOP_SFX_ANY 0x6

#define NOTE_DEFAULT_WAVE 3
#define NOTE_DEFAULT_VOL 7

static uint8_t sfx_mem[SFX_COUNT * SFX_BYTES] __attribute__((aligned(2)));
static uint8_t music_mem[MUSIC_COUNT * MUSIC_BYTES] __attribute__((aligned(2)));

static int p8audio_inited = 0;

extern bool platform_has_testbench;

static inline uint16_t stat_sfx(int ch)
{
    switch (ch) {
    case 0:
        return MMIO_REG16(_P8AUDIO_STAT46);
    case 1:
        return MMIO_REG16(_P8AUDIO_STAT47);
    case 2:
        return MMIO_REG16(_P8AUDIO_STAT48);
    case 3:
    default:
        return MMIO_REG16(_P8AUDIO_STAT49);
    }
}

static inline uint16_t stat_note(int ch)
{
    switch (ch) {
    case 0:
        return MMIO_REG16(_P8AUDIO_STAT50);
    case 1:
        return MMIO_REG16(_P8AUDIO_STAT51);
    case 2:
        return MMIO_REG16(_P8AUDIO_STAT52);
    case 3:
    default:
        return MMIO_REG16(_P8AUDIO_STAT53);
    }
}

static inline uint16_t stat_music_pattern(void)
{
    return MMIO_REG16(_P8AUDIO_STAT54);
}

static inline uint16_t stat_music_playing(void)
{
    return MMIO_REG16(_P8AUDIO_STAT57);
}

static int16_t read_pcm_left(void)
{
    return (int16_t)MMIO_REG16(FUNCVAL_PCM_L);
}

static int pcm_avg_abs(int samples)
{
    int i;
    int32_t sum = 0;

    if (!platform_has_testbench) {
        return -1;
    }

    for (i = 0; i < samples; i++) {
        int16_t s = read_pcm_left();
        int32_t a = (s < 0) ? -s : s;
        sum += a;
        usleep(1000000 / 22050); /* Sleep for ~1 sample period at 22050 Hz */
    }

    return (int)(sum / samples);
}

static void record_start(void)
{
    if (platform_has_testbench) {
        MMIO_REG8(FUNCVAL_WAV_REC) = 1;
        usleep(10);
    }
}

static void record_stop(void)
{
    if (platform_has_testbench) {
        MMIO_REG8(FUNCVAL_WAV_REC) = 0;
        usleep(10);
    }
}

static void p8audio_init(void);
static void stop_all(void);

static void p8audio_record_setup(void)
{
    p8audio_init();
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0000;
    record_start();
    stop_all();
}

static void p8audio_record_cleanup(void)
{
    stop_all();
    record_stop();
}


static void set_p8audio_bases(void)
{
    uintptr_t sfx_base = (uintptr_t)sfx_mem;
    uintptr_t music_base = (uintptr_t)music_mem;

    MMIO_REG16(_P8AUDIO_SFX_BASE_HI) = (uint16_t)(sfx_base >> 16);
    MMIO_REG16(_P8AUDIO_SFX_BASE_LO) = (uint16_t)(sfx_base & 0xffff);
    MMIO_REG16(_P8AUDIO_MUSIC_BASE_HI) = (uint16_t)(music_base >> 16);
    MMIO_REG16(_P8AUDIO_MUSIC_BASE_LO) = (uint16_t)(music_base & 0xffff);
}

static void write_note(int sfx, int note_idx, uint8_t pitch, uint8_t wave, uint8_t vol, uint8_t effect)
{
    uint16_t note_word = (uint16_t)(((effect & 0x7) << 12) |
                                    ((vol & 0x7) << 9) |
                                    ((wave & 0x7) << 6) |
                                    (pitch & 0x3f));
    size_t base = (size_t)(sfx * SFX_BYTES + note_idx * 2);
    sfx_mem[base + 0] = (uint8_t)(note_word & 0xff);
    sfx_mem[base + 1] = (uint8_t)((note_word >> 8) & 0xff);
}

static void init_sfx_slot(int sfx, uint8_t pitch, uint8_t wave, uint8_t vol,
                          uint8_t speed, uint8_t loop_start, uint8_t loop_end)
{
    int i;
    size_t base = (size_t)(sfx * SFX_BYTES);
    for (i = 0; i < 32; i++) {
        write_note(sfx, i, pitch, wave, vol, 0);
    }
    sfx_mem[base + 64] = 0x00;
    sfx_mem[base + 65] = speed;
    sfx_mem[base + 66] = (uint8_t)(loop_start & 0x3f);
    sfx_mem[base + 67] = (uint8_t)(loop_end & 0x3f);
}

static void init_music_patterns(void)
{
    size_t base0 = 0 * MUSIC_BYTES;
    size_t base1 = 1 * MUSIC_BYTES;
    size_t base2 = 2 * MUSIC_BYTES;

    music_mem[base0 + 0] = 0x80 | 0x00; /* loop start, ch0: sfx0 */
    music_mem[base0 + 1] = 0x01;        /* ch1: sfx1 */
    music_mem[base0 + 2] = 0x02;        /* ch2: sfx2 */
    music_mem[base0 + 3] = 0x40;        /* ch3: continue */

    music_mem[base1 + 0] = 0x00;        /* ch0: sfx0 */
    music_mem[base1 + 1] = 0x80 | 0x01; /* loop end, ch1: sfx1 */
    music_mem[base1 + 2] = 0x02;        /* ch2: sfx2 */
    music_mem[base1 + 3] = 0x40;        /* ch3: continue */

    music_mem[base2 + 0] = 0x0b;        /* ch0: sfx11 */
    music_mem[base2 + 1] = 0x0c;        /* ch1: sfx12 */
    music_mem[base2 + 2] = 0x0d;        /* ch2: sfx13 */
    music_mem[base2 + 3] = 0x40;        /* ch3: continue */
}

static void init_audio_memory(void)
{
    size_t i;
    for (i = 0; i < sizeof(sfx_mem); i++) {
        sfx_mem[i] = 0x00;
    }
    for (i = 0; i < sizeof(music_mem); i++) {
        music_mem[i] = 0x00;
    }

    init_sfx_slot(0, 24, NOTE_DEFAULT_WAVE, NOTE_DEFAULT_VOL, 2, 0, 0); /* C4 */
    init_sfx_slot(1, 28, NOTE_DEFAULT_WAVE, NOTE_DEFAULT_VOL, 2, 0, 0); /* E4 */
    init_sfx_slot(2, 31, NOTE_DEFAULT_WAVE, NOTE_DEFAULT_VOL, 2, 0, 0); /* G4 */
    init_sfx_slot(3, 35, NOTE_DEFAULT_WAVE, NOTE_DEFAULT_VOL, 2, 0, 0); /* B4 */
    init_sfx_slot(4, 36, 2, NOTE_DEFAULT_VOL, 1, 0, 0);                /* C5 */
    init_sfx_slot(5, 24, NOTE_DEFAULT_WAVE, NOTE_DEFAULT_VOL, 2, 0, 1); /* looping */
    init_sfx_slot(6, 24, NOTE_DEFAULT_WAVE, 2, 2, 0, 0);                /* mix: C4 */
    init_sfx_slot(7, 24, NOTE_DEFAULT_WAVE, 2, 2, 0, 0);                /* mix: C4 */
    init_sfx_slot(8, 24, NOTE_DEFAULT_WAVE, 2, 2, 0, 0);                /* mix: C4 */
    init_sfx_slot(9, 24, 7, 2, 2, 0, 0);                                /* mix: C4 */
    init_sfx_slot(11, 24, NOTE_DEFAULT_WAVE, NOTE_DEFAULT_VOL, 2, 0, 31);/* loop: C4 */
    init_sfx_slot(12, 28, NOTE_DEFAULT_WAVE, NOTE_DEFAULT_VOL, 8, 0, 0); /* slow: E4 */
    init_sfx_slot(13, 31, NOTE_DEFAULT_WAVE, NOTE_DEFAULT_VOL, 4, 0, 0); /* fast: G4 */

    init_music_patterns();
}

static void p8audio_init(void)
{
    if (p8audio_inited) {
        return;
    }

    init_audio_memory();
    set_p8audio_bases();
    MMIO_REG16(_P8AUDIO_CTRL) = 0x0001; /* run */
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0000;
    MMIO_REG16(_P8AUDIO_MUSIC_FADE) = 0x0000;
    MMIO_REG8(_P8AUDIO_HWFX40) = 0x00;
    MMIO_REG8(_P8AUDIO_HWFX41) = 0x00;
    MMIO_REG8(_P8AUDIO_HWFX42) = 0x00;
    MMIO_REG8(_P8AUDIO_HWFX43) = 0x00;
    MMIO_REG16(_P8AUDIO_CTRL) = 0x0001;

    p8audio_inited = 1;
}

static void sfx_cmd(int idx, int ch, int offset)
{
    uint16_t idx_f = (idx < 0) ? (uint16_t)(idx == -2 ? SFX_IDX_RELEASE : SFX_IDX_STOP) : (uint16_t)(idx & 0x3f);
    uint16_t ch_f;
    if (ch < 0) {
        ch_f = (ch == -2) ? CH_STOP_SFX_ANY : CH_AUTO;
    } else {
        ch_f = (uint16_t)(ch & 0x7);
    }
    MMIO_REG16(_P8AUDIO_SFX_CMD) = (uint16_t)(SFX_CMD_FLAG | (ch_f << 12) | ((offset & 0x3f) << 6) | idx_f);
}

static void music_cmd(int pat, int fade_len, int mask)
{
    uint16_t pat_f = (pat < 0) ? 0x3f : (uint16_t)(pat & 0x3f);
    uint16_t mask_f = (uint16_t)(mask & 0xf);
    MMIO_REG16(_P8AUDIO_MUSIC_FADE) = (uint16_t)fade_len;
    MMIO_REG16(_P8AUDIO_MUSIC_CMD) = (uint16_t)((pat_f << 7) | (mask_f << 3));
}

static void stop_all(void)
{
    sfx_cmd(-1, -1, 0);
    music_cmd(-1, 0x0000, 0);
    usleep(5000);
}

static bool wait_for_sfx_on_channel(int ch, uint8_t sfx, uint32_t timeout_us)
{
    uint64_t start = timer_get_us();
    while ((timer_get_us() - start) < timeout_us) {
        uint16_t val = stat_sfx(ch);
        if (val != 0xffff && (uint8_t)(val & 0x3f) == sfx) {
            return true;
        }
    }
    return false;
}

static bool wait_for_sfx_any(uint8_t sfx, int *ch_out, uint32_t timeout_us)
{
    uint64_t start = timer_get_us();
    while ((timer_get_us() - start) < timeout_us) {
        int ch;
        for (ch = 0; ch < 4; ch++) {
            uint16_t val = stat_sfx(ch);
            if (val != 0xffff && (uint8_t)(val & 0x3f) == sfx) {
                if (ch_out) {
                    *ch_out = ch;
                }
                return true;
            }
        }
    }
    return false;
}

static bool wait_for_idle(int ch, uint32_t timeout_us)
{
    uint64_t start = timer_get_us();
    while ((timer_get_us() - start) < timeout_us) {
        if (stat_sfx(ch) == 0xffff) {
            return true;
        }
    }
    return false;
}

static bool wait_for_release(int ch, uint32_t timeout_us)
{
    uint64_t start = timer_get_us();
    while ((timer_get_us() - start) < timeout_us) {
        uint16_t sfx_val = stat_sfx(ch);
        uint16_t note_val = stat_note(ch);
        if (sfx_val == 0xffff || (note_val != 0xffff && (uint8_t)(note_val & 0x3f) >= 32)) {
            return true;
        }
    }
    return false;
}

static bool ensure_idle_for(int ch, uint32_t duration_us)
{
    uint64_t start = timer_get_us();
    while ((timer_get_us() - start) < duration_us) {
        if (stat_sfx(ch) != 0xffff) {
            return false;
        }
        usleep(1000);
    }
    return true;
}

static bool wait_for_music_pattern(uint8_t pat, uint32_t timeout_us)
{
    uint64_t start = timer_get_us();
    while ((timer_get_us() - start) < timeout_us) {
        uint16_t val = stat_music_pattern();
        if ((uint8_t)(val & 0x3f) == pat) {
            return true;
        }
    }
    return false;
}

static bool wait_for_note_at_least(int ch, uint8_t note, uint32_t timeout_us)
{
    uint64_t start = timer_get_us();
    while ((timer_get_us() - start) < timeout_us) {
        uint16_t val = stat_note(ch);
        if (val != 0xffff && (uint8_t)(val & 0x3f) >= note) {
            return true;
        }
    }
    return false;
}

static int test_p8audio_version(void)
{
    int result = TEST_PASS;
    p8audio_init();
    test_puts("Version: ");
    uart_print_hex_word(MMIO_REG16(_P8AUDIO_VERSION));
    test_print_crlf();
    return result;
}

static int test_sfx_auto_channel(void)
{
    int result = TEST_PASS;
    int ch = -1;
    sfx_cmd(0, -1, 0);
    if (!wait_for_sfx_any(0, &ch, 200000)) {
        test_puts("FAIL: sfx auto channel not active");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_sfx_explicit_queue(void)
{
    int result = TEST_PASS;
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0004;
    sfx_cmd(0, 0, 0);
    if (!wait_for_sfx_on_channel(0, 0, 200000)) {
        test_puts("FAIL: sfx0 did not start on ch0");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(1, 0, 0);
    if (!wait_for_sfx_on_channel(0, 1, 800000)) {
        test_puts("FAIL: queued sfx1 did not play on ch0");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0000;
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_sfx_stop_channel(void)
{
    int result = TEST_PASS;
    sfx_cmd(2, 1, 0);
    if (!wait_for_sfx_on_channel(1, 2, 200000)) {
        test_puts("FAIL: sfx2 did not start on ch1");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(-1, 1, 0);
    if (!wait_for_idle(1, 200000)) {
        test_puts("FAIL: sfx(-1,1) did not stop ch1");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_sfx_stop_by_index(void)
{
    int result = TEST_PASS;
    sfx_cmd(3, 0, 0);
    sfx_cmd(3, 1, 0);
    if (!wait_for_sfx_on_channel(0, 3, 200000) || !wait_for_sfx_on_channel(1, 3, 200000)) {
        test_puts("FAIL: sfx3 not active on ch0/ch1");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(3, -2, 0);
    if (!wait_for_idle(0, 200000) || !wait_for_idle(1, 200000)) {
        test_puts("FAIL: sfx(3,-2) did not stop both channels");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_sfx_release_loop(void)
{
    int result = TEST_PASS;
    uint16_t note_val;
    int last_note;
    int i;
    bool looped = false;
    sfx_cmd(5, 2, 0);
    if (!wait_for_sfx_on_channel(2, 5, 200000)) {
        test_puts("FAIL: sfx5 did not start on ch2");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    note_val = stat_note(2);
    last_note = (int)(note_val & 0x3f);
    for (i = 0; i < 500; i++) {
        usleep(1000);
        note_val = stat_note(2);
        if (note_val == 0xffff) {
            test_puts("FAIL: looping sfx5 stopped early");
            test_print_crlf();
            result = TEST_FAIL;
            return result;
        }
        if ((int)(note_val & 0x3f) <= last_note) {
            looped = true;
            break;
        }
        last_note = (int)(note_val & 0x3f);
    }
    if (!looped) {
        test_puts("FAIL: sfx5 did not loop within 500 ticks");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(-2, 2, 0);
    if (!wait_for_idle(2, 800000)) {
        test_puts("FAIL: sfx(-2,2) did not release loop");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_sfx_offset_length(void)
{
    int result = TEST_PASS;
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x000a;
    sfx_cmd(4, 3, 16);
    if (!wait_for_sfx_on_channel(3, 4, 200000)) {
        test_puts("FAIL: sfx4 did not start on ch3");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    if (!wait_for_note_at_least(3, 16, 200000)) {
        test_puts("FAIL: offset not honored");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    if (!wait_for_idle(3, 800000)) {
        test_puts("FAIL: length override not honored");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0000;
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_sfx_offset_length_edges(void)
{
    int result = TEST_PASS;
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0002;
    sfx_cmd(4, 0, 30);
    if (!wait_for_sfx_on_channel(0, 4, 200000)) {
        test_puts("FAIL: sfx4 did not start on ch0");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    if (!wait_for_note_at_least(0, 30, 200000)) {
        test_puts("FAIL: offset 30 not honored");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    if (!wait_for_idle(0, 400000)) {
        test_puts("FAIL: length=2 did not stop quickly");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0000;
    test_puts("PASS");
    test_print_crlf();
    return result;
}


static int test_sfx_loop_offset_wrap(void)
{
    int result = TEST_PASS;
    int i;
    size_t base = 10 * SFX_BYTES;

    for (i = 0; i < 16; i++) {
        write_note(10, i, (uint8_t)(i * 2), 0, NOTE_DEFAULT_VOL, 0);
    }
    sfx_mem[base + 64] = 0x00;
    sfx_mem[base + 65] = 0x04;
    sfx_mem[base + 66] = 0x00;
    sfx_mem[base + 67] = 0x0f;

    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0008;
    sfx_cmd(10, 0, 12);
    if (!wait_for_sfx_on_channel(0, 10, 200000)) {
        test_puts("FAIL: loop offset sfx did not start on ch0");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    for (i = 0; i < 2000; i++) {
        uint16_t note_val = stat_note(0);
        if (note_val != 0xffff && (uint8_t)(note_val & 0x3f) < 10) {
            MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0000;
            test_puts("PASS");
            test_print_crlf();
            return result;
        }
        usleep(1000);
    }

    test_puts("FAIL: loop offset sfx did not wrap around");
    test_print_crlf();
    result = TEST_FAIL;
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0000;
    return result;
}

static int test_sfx_stop_all(void)
{
    int result = TEST_PASS;
    sfx_cmd(0, 0, 0);
    sfx_cmd(1, 1, 0);
    sfx_cmd(2, 2, 0);
    if (!wait_for_sfx_on_channel(0, 0, 200000) ||
        !wait_for_sfx_on_channel(1, 1, 200000) ||
        !wait_for_sfx_on_channel(2, 2, 200000)) {
        test_puts("FAIL: setup for sfx(-1) did not start");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(-1, -1, 0);
    if (!wait_for_idle(0, 200000) || !wait_for_idle(1, 200000) || !wait_for_idle(2, 200000)) {
        test_puts("FAIL: sfx(-1) did not stop all channels");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_sfx_release_all(void)
{
    int result = TEST_PASS;
    sfx_cmd(5, 0, 0);
    sfx_cmd(5, 1, 0);
    if (!wait_for_sfx_on_channel(0, 5, 200000) || !wait_for_sfx_on_channel(1, 5, 200000)) {
        test_puts("FAIL: setup for sfx(-2) did not start");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(-2, -1, 0);
    if (!wait_for_idle(0, 800000) || !wait_for_idle(1, 800000)) {
        test_puts("FAIL: sfx(-2) did not release looping on all channels");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_multi_channel_play(void)
{
    int result = TEST_PASS;
    sfx_cmd(6, 0, 0);
    sfx_cmd(7, 1, 0);
    sfx_cmd(8, 2, 0);
    sfx_cmd(9, 3, 0);
    if (!wait_for_sfx_on_channel(0, 6, 200000) ||
        !wait_for_sfx_on_channel(1, 7, 200000) ||
        !wait_for_sfx_on_channel(2, 8, 200000) ||
        !wait_for_sfx_on_channel(3, 9, 200000)) {
        test_puts("FAIL: not all channels active");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_all_channels_busy_auto_queue(void)
{
    int result = TEST_PASS;
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0002;
    sfx_cmd(0, 0, 0);
    sfx_cmd(1, 1, 0);
    sfx_cmd(2, 2, 0);
    sfx_cmd(3, 3, 0);
    if (!wait_for_sfx_on_channel(0, 0, 200000) ||
        !wait_for_sfx_on_channel(1, 1, 200000) ||
        !wait_for_sfx_on_channel(2, 2, 200000) ||
        !wait_for_sfx_on_channel(3, 3, 200000)) {
        test_puts("FAIL: setup for all channels busy failed");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(4, -1, 0);
    if (!wait_for_sfx_on_channel(0, 4, 1200000)) {
        test_puts("FAIL: auto-queued sfx did not play on ch0");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0000;
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_busy_channel_queue(void)
{
    int result = TEST_PASS;
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0003;
    sfx_cmd(0, 1, 0);
    if (!wait_for_sfx_on_channel(1, 0, 200000)) {
        test_puts("FAIL: sfx0 did not start on ch1");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(1, 1, 0);
    if (!wait_for_sfx_on_channel(1, 1, 1200000)) {
        test_puts("FAIL: queued sfx1 did not play on busy ch1");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0000;
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_pcm_mixing(void)
{
    int result = TEST_PASS;
    int avg1, avg2, avg4;

    sfx_cmd(9, 0, 0);
    if (!wait_for_sfx_on_channel(0, 9, 200000)) {
        test_puts("FAIL: sfx9 did not start on ch0");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    avg1 = pcm_avg_abs(512);
    if (platform_has_testbench && avg1 < 4) {
        test_puts("FAIL: PCM is silent for single channel");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    sfx_cmd(9, 1, 0);
    if (!wait_for_sfx_on_channel(1, 9, 200000)) {
        test_puts("FAIL: sfx9 did not start on ch1");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    avg2 = pcm_avg_abs(512);
    if (platform_has_testbench && avg2 < (avg1 * 3) / 2) {
        test_puts("FAIL: PCM mix not ~2x for two channels");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    sfx_cmd(9, 2, 0);
    sfx_cmd(9, 3, 0);
    if (!wait_for_sfx_on_channel(2, 9, 200000) || !wait_for_sfx_on_channel(3, 9, 200000)) {
        test_puts("FAIL: sfx9 did not start on ch2/ch3");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    avg4 = pcm_avg_abs(512);
    if (platform_has_testbench && avg4 < (avg1 * 3)) {
        test_puts("FAIL: PCM mix not ~4x for four channels");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_sfx_over_music(void)
{
    int result = TEST_PASS;

    music_cmd(0, 0x0000, 0x7);
    if (!wait_for_sfx_on_channel(0, 0, 800000)) {
        test_puts("FAIL: music did not start on ch0");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(4, 0, 0);
    if (!wait_for_sfx_on_channel(0, 4, 200000)) {
        test_puts("FAIL: sfx4 did not override music on ch0");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    if (!stat_music_playing()) {
        test_puts("FAIL: music stopped when sfx started");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    MMIO_REG16(_P8AUDIO_SFX_LEN) = 0x0000;
    music_cmd(-1, 0x0000, 0);
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_sfx_stop_release_while_music(void)
{
    int result = TEST_PASS;

    music_cmd(0, 0x0000, 0x7);
    if (!wait_for_sfx_on_channel(1, 1, 800000)) {
        test_puts("FAIL: music did not start on ch1");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    sfx_cmd(5, 3, 0);
    if (!wait_for_sfx_on_channel(3, 5, 200000)) {
        test_puts("FAIL: sfx5 did not start on ch3");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(-1, 3, 0);
    if (!wait_for_idle(3, 200000)) {
        test_puts("FAIL: sfx(-1,3) did not stop ch3");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    sfx_cmd(5, 3, 0);
    if (!wait_for_sfx_on_channel(3, 5, 200000)) {
        test_puts("FAIL: sfx5 did not restart on ch3");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(-2, 3, 0);
    if (!wait_for_release(3, 800000)) {
        test_puts("FAIL: sfx(-2,3) did not release loop");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    if (!wait_for_music_pattern(1, 2000000)) {
        test_puts("FAIL: music stopped advancing during sfx stop/release");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    music_cmd(-1, 0x0000, 0);
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_music_basic(void)
{
    int result = TEST_PASS;

    music_cmd(0, 0x0000, 0x7);
    if (stat_music_pattern() != 0x0000) {
        test_puts("FAIL: music did not start at pattern 0");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    if (!wait_for_music_pattern(1, 2000000)) {
        test_puts("FAIL: music did not advance to pattern 1");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    music_cmd(-1, 0x0000, 0);
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_music_mask(void)
{
    int result = TEST_PASS;
    music_cmd(0, 0x0000, 0x0);
    if (!wait_for_music_pattern(0, 2000000)) {
        test_puts("FAIL: music did not advance with mask=0");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    if (!ensure_idle_for(0, 200000) || !ensure_idle_for(1, 200000) ||
        !ensure_idle_for(2, 200000) || !ensure_idle_for(3, 200000)) {
        test_puts("FAIL: channels not idle with mask=0");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    music_cmd(0, 0x0000, 0x5);
    if (!wait_for_sfx_on_channel(0, 0, 800000) || !wait_for_sfx_on_channel(2, 2, 800000)) {
        test_puts("FAIL: mask=0x5 did not start on ch0/ch2");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    if (!ensure_idle_for(1, 200000) || !ensure_idle_for(3, 200000)) {
        test_puts("FAIL: mask=0x5 did not mute ch1/ch3");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    music_cmd(-1, 0x0000, 0);
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_music_loop_fade(void)
{
    int result = TEST_PASS;
    music_cmd(0, 0x0000, 0x7);
    if (!wait_for_sfx_on_channel(0, 0, 800000) ||
        !wait_for_sfx_on_channel(1, 1, 800000) ||
        !wait_for_sfx_on_channel(2, 2, 800000)) {
        test_puts("FAIL: music did not start on ch0-2");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(3, -1, 0);
    if (!wait_for_sfx_on_channel(3, 3, 400000)) {
        test_puts("FAIL: auto sfx did not use ch3 with music mask");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    sfx_cmd(2, 0, 0);
    if (!wait_for_sfx_on_channel(0, 2, 400000)) {
        test_puts("FAIL: explicit sfx did not override music mask on ch0");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    if (!wait_for_music_pattern(1, 2000000) || !wait_for_music_pattern(0, 3000000)) {
        test_puts("FAIL: music loop did not advance as expected");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    music_cmd(-1, 0x0000, 0);
    usleep(50000);
    if (!wait_for_idle(0, 800000) || !wait_for_idle(1, 800000) || !wait_for_idle(2, 800000)) {
        test_puts("FAIL: music stop did not idle channels");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_music_fade_out_time(void)
{
    int result = TEST_PASS;
    uint32_t expected_samples = 6615;
    uint32_t expected_us = 300000;
    uint32_t tolerance_us = 100000;
    uint64_t start;
    uint64_t elapsed;

    music_cmd(0, (int)expected_samples, 0x7);
    if (!wait_for_music_pattern(0, 2000000)) {
        test_puts("FAIL: music did not start for fade timing test");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    music_cmd(-1, (int)expected_samples, 0);
    start = timer_get_us();
    while (stat_music_playing()) {
        if ((timer_get_us() - start) > 2000000) {
            break;
        }
    }
    elapsed = timer_get_us() - start;

    if (elapsed + tolerance_us < expected_us) {
        test_puts("FAIL: fade too fast");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }
    if (elapsed > (expected_us + tolerance_us)) {
        test_puts("FAIL: fade too slow");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    test_puts("PASS");
    test_print_crlf();
    return result;
}

static int test_music_advance_timing(void)
{
    int result = TEST_PASS;

    music_cmd(2, 0x0000, 0x7);
    if (!wait_for_sfx_on_channel(0, 11, 800000) ||
        !wait_for_sfx_on_channel(1, 12, 800000) ||
        !wait_for_sfx_on_channel(2, 13, 800000)) {
        test_puts("FAIL: music pattern 2 did not start on ch0-2");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    if (!wait_for_idle(2, 1200000)) {
        test_puts("FAIL: channel 2 (faster) did not finish");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    if ((stat_music_pattern() & 0x3f) != 2) {
        test_puts("FAIL: music advanced before leftmost non-looping channel finished");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    if (!wait_for_idle(1, 1200000)) {
        test_puts("FAIL: channel 1 (leftmost non-looping) did not finish");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    if ((stat_music_pattern() & 0x3f) == 2) {
        test_puts("FAIL: music did not advance after leftmost non-looping channel finished");
        test_print_crlf();
        result = TEST_FAIL;
        return result;
    }

    music_cmd(-1, 0x0000, 0);
    test_puts("PASS");
    test_print_crlf();
    return result;
}

TEST_SUITE(13_p8audio,
           p8audio_version,
           TEST_CASE_SETUP_CLEANUP(sfx_auto_channel, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(sfx_explicit_queue, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(sfx_stop_channel, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(sfx_stop_by_index, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(sfx_release_loop, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(sfx_offset_length, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(sfx_offset_length_edges, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(sfx_loop_offset_wrap, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(sfx_stop_all, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(sfx_release_all, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(multi_channel_play, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(all_channels_busy_auto_queue, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(busy_channel_queue, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(pcm_mixing, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(sfx_over_music, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(sfx_stop_release_while_music, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(music_basic, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(music_mask, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(music_loop_fade, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(music_fade_out_time, p8audio_record_setup, p8audio_record_cleanup),
           TEST_CASE_SETUP_CLEANUP(music_advance_timing, p8audio_record_setup, p8audio_record_cleanup));
