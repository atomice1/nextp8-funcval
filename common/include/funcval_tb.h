/*
 * nextp8 FuncVal Testbench Definitions
 * Testbench-specific registers and memory regions
 *
 * Include this header in functional validation tests.
 * Provides testbench-specific definitions (simulation only).
 */

#ifndef FUNCVAL_TB_H
#define FUNCVAL_TB_H

/* FuncVal Testbench MMIO (Simulation Only) */
#define FUNCVAL_PERIPH_BASE     0x380000    /* Peripheral model control */
#define FUNCVAL_VGA_FB          0x390000    /* VGA framebuffer readback (171x128 downsampled) */
#define FUNCVAL_KB_SCANCODE     0x380001    /* Keyboard scancode input */
/* Mouse TB registers: write buttons, X, Y, Z in that order;
 * the write to Z latches all four values and triggers the PS/2 packet. */
#define FUNCVAL_MOUSE_BUTTONS   0x380020    /* Mouse button state (word write) */
#define FUNCVAL_MOUSE_X         0x380022    /* Mouse X movement (signed 9-bit, word write) */
#define FUNCVAL_MOUSE_Y         0x380024    /* Mouse Y movement (signed 9-bit, word write) */
#define FUNCVAL_MOUSE_Z         0x380026    /* Mouse Z scroll (signed 4-bit, word write, triggers update) */
#define FUNCVAL_SCREENSHOT      0x380041    /* Screenshot trigger */
#define FUNCVAL_TRACE           0x380043    /* Trace control register */
#define FUNCVAL_WAV_REC         0x380045    /* WAV recording control */
#define FUNCVAL_WAVE_CAP        0x380047    /* Wave capture control (write 1=start VCD, 0=stop) */
#define FUNCVAL_JOY0            0x380061    /* Joystick 0 state (bit 0=up, 1=down, 2=left, 3=right, 4=btn1, 5=btn2) */
#define FUNCVAL_JOY1            0x380063    /* Joystick 1 state (bit 0=up, 1=down, 2=left, 3=right, 4=btn1, 5=btn2) */
#define FUNCVAL_KB_MATRIX_BASE  0x380080    /* Built-in keyboard matrix  */
#define FUNCVAL_PCM_L           0x3800a0    /* PCM left channel */
#define FUNCVAL_PCM_R           0x3800a2    /* PCM right channel */

/* Per-model debug enable registers (write 1 to enable verbose $display, 0 to silence) */
#define FUNCVAL_MODEL_DEBUG_DS1307  0x3800B0    /* DS1307 I2C RTC device model */
#define FUNCVAL_MODEL_DEBUG_KB      0x3800B2    /* PS/2 keyboard device model */
#define FUNCVAL_MODEL_DEBUG_MOUSE   0x3800B4    /* PS/2 mouse device model */
#define FUNCVAL_MODEL_DEBUG_VGA     0x3800B6    /* VGA display model */
#define FUNCVAL_MODEL_DEBUG_SDSPI   0x3800B8    /* SD card SPI model */

/* Joystick bit definitions */
#define JOY_UP      0x01
#define JOY_DOWN    0x02
#define JOY_LEFT    0x04
#define JOY_RIGHT   0x08
#define JOY_BTN1    0x10
#define JOY_BTN2    0x20

#endif /* FUNCVAL_TB_H */
