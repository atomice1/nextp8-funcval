/* Keyboard input functions for funcval library */

#include <stdbool.h>
#include <stdint.h>
#include "mmio.h"
#include "nextp8.h"

/* Clear latched key state for all keys */
void clear_latched_keys(void)
{
    for (int i=0;i<32;++i)
        MMIO_REG8(_KEYBOARD_MATRIX_LATCHED + i) = 0xff;
    MMIO_REG8(_JOYSTICK0_LATCHED) = 0xff;
    MMIO_REG8(_JOYSTICK1_LATCHED) = 0xff;
}

void wait_for_any_key(void)
{
    clear_latched_keys();
    bool key_pressed = false;
    do {
        for (int i=0;i<32;++i) {
            if (MMIO_REG8(_KEYBOARD_MATRIX_LATCHED + i) != 0)
                key_pressed = true;
        }
        if (MMIO_REG8(_JOYSTICK0_LATCHED) != 0)
            key_pressed = true;

        if (MMIO_REG8(_JOYSTICK1_LATCHED) != 0)
            key_pressed = true;
    } while (!key_pressed);
    clear_latched_keys();
}

/* Return if the key with the given scancode is currently pressed or not.
 */
static bool is_down(volatile uint8_t *base, unsigned index)
{
    return base[index >> 3] & (1 << (index & 0x7));
}

/* Clear latched key state for a specific key index */
static void clear_key(unsigned index)
{
    MMIO_REG8(_KEYBOARD_MATRIX_LATCHED + (index >> 3)) = (1 << (index & 0x7));
}

/* Wait for and read a keyboard key press
 */
uint8_t read_keyboard_scancode(void)
{
    uint8_t scancode = 0;
    volatile uint8_t *matrix = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;

    do {
        for (unsigned i=0;i<256;++i) {
            bool down = is_down(matrix, i);
            if (down) {
                scancode = i;
                clear_key(i);
                break;
            }
        }
    } while (scancode == 0);

    return scancode;
}

/* Key mapping table
   USB HID scancodes to ASCII
   Format: Each index represents a USB HID scancode, value is ASCII character or 0 if no mapping */
static char keytable[256] = {
    0,      // 0x00 NONE
    0,      // 0x01 ERR_OVF
    0,      // 0x02 MOD_LSHIFT
    0,      // 0x03
    0,      // 0x04 A
    0,      // 0x05 B
    0,      // 0x06 C
    0,      // 0x07 D
    0,      // 0x08 E
    0,      // 0x09 F
    0,      // 0x0a G
    0,      // 0x0b H
    0,      // 0x0c I
    0,      // 0x0d J
    0,      // 0x0e K
    0,      // 0x0f L
    0,      // 0x10 M
    0,      // 0x11 N
    0,      // 0x12 O
    0,      // 0x13 P
    0,      // 0x14 Q
    0,      // 0x15 R
    0,      // 0x16 S
    0,      // 0x17 T
    0,      // 0x18 U
    0,      // 0x19 V
    0,      // 0x1a W
    0,      // 0x1b X
    0,      // 0x1c Y
    0,      // 0x1d Z
    0,      // 0x1e 1
    0,      // 0x1f 2
    0,      // 0x20 3
    0,      // 0x21 4
    0,      // 0x22 5
    0,      // 0x23 6
    0,      // 0x24 7
    0,      // 0x25 8
    0,      // 0x26 9
    0,      // 0x27 0
    '\n',   // 0x28 ENTER
    '\b',   // 0x29 BACKSPACE
    '\t',   // 0x2a TAB
    ' ',    // 0x2b SPACE
    '-',    // 0x2c MINUS
    '=',    // 0x2d EQUAL
    '[',    // 0x2e LEFTBRACE
    ']',    // 0x2f RIGHTBRACE
    '\\',   // 0x30 BACKSLASH
    0,      // 0x31 HASHTILDE (non-US #)
    ';',    // 0x32 SEMICOLON
    '\'',   // 0x33 APOSTROPHE
    '`',    // 0x34 GRAVE
    ',',    // 0x35 COMMA
    '.',    // 0x36 DOT
    '/',    // 0x37 SLASH
    0,      // 0x38
    0,      // 0x39 CAPSLOCK
    0,      // 0x3a F1
    0,      // 0x3b F2
    0,      // 0x3c F3
    0,      // 0x3d F4
    0,      // 0x3e F5
    0,      // 0x3f F6
    0,      // 0x40 F7
    0,      // 0x41 F8
    0,      // 0x42 F9
    0,      // 0x43 F10
    0,      // 0x44 F11
    0,      // 0x45 F12
    0,      // 0x46 SYSRQ
    0,      // 0x47 SCROLLLOCK
    0,      // 0x48 PAUSE
    0,      // 0x49 INSERT
    0,      // 0x4a HOME
    0,      // 0x4b PAGEUP
    0,      // 0x4c DELETE
    0,      // 0x4d END
    0,      // 0x4e PAGEDOWN
    0,      // 0x4f RIGHT
    0,      // 0x50 LEFT
    0,      // 0x51 DOWN
    0,      // 0x52 UP
    0,      // 0x53 NUMLOCK
    0,      // 0x54 KPSLASH
    0,      // 0x55 KPASTERISK
    0,      // 0x56 KPMINUS
    0,      // 0x57 KPPLUS
    0,      // 0x58 KPENTER
    0,      // 0x59 KP1
    0,      // 0x5a KP2
    0,      // 0x5b KP3
    0,      // 0x5c KP4
    0,      // 0x5d KP5
    0,      // 0x5e KP6
    0,      // 0x5f KP7
    0,      // 0x60 KP8
    0,      // 0x61 KP9
    0,      // 0x62 KP0
    0,      // 0x63 KPDOT
    0,      // 0x64 102ND
    0,      // 0x65 COMPOSE
    0,      // 0x66 POWER
    0,      // 0x67 KPEQUAL
    0,      // 0x68 F13
    0,      // 0x69 F14
    0,      // 0x6a F15
    0,      // 0x6b F16
    0,      // 0x6c F17
    0,      // 0x6d F18
    0,      // 0x6e F19
    0,      // 0x6f F20
    0,      // 0x70 F21
    0,      // 0x71 F22
    0,      // 0x72 F23
    0,      // 0x73 F24
    0,      // 0x74 OPEN
    0,      // 0x75 HELP
    0,      // 0x76 PROPS
    0,      // 0x77 STOP
    0,      // 0x78 AGAIN
    0,      // 0x79 UNDO
    0,      // 0x7a CUT
    0,      // 0x7b COPY
    0,      // 0x7c PASTE
    0,      // 0x7d FIND
    0,      // 0x7e MUTE
    0,      // 0x7f VOLUMEDOWN
    0,      // 0x80 VOLUMEUP
    0,      // 0x81 KPCOMMA
    0,      // 0x82
    0,      // 0x83
    0,      // 0x84
    0,      // 0x85 KPRIGHTPAREN
    0,      // 0x86
    0,      // 0x87 RO
    0,      // 0x88 KATAKANAHIRAGANA
    0,      // 0x89 YEN
    0,      // 0x8a HENKAN
    0,      // 0x8b MUHENKAN
    0,      // 0x8c KPJPCOMMA
    0,      // 0x8d
    0,      // 0x8e
    0,      // 0x8f
    0,      // 0x90 HANGEUL
    0,      // 0x91 HANJA
    0,      // 0x92 KATAKANA
    0,      // 0x93 HIRAGANA
    0,      // 0x94 ZENKAKUHANKAKU
    0,      // 0x95
    0,      // 0x96
    0,      // 0x97
    0,      // 0x98
    0,      // 0x99
    0,      // 0x9a
    0,      // 0x9b
    0,      // 0x9c
    0,      // 0x9d
    0,      // 0x9e
    0,      // 0x9f
    0,      // 0xa0
    0,      // 0xa1
    0,      // 0xa2
    0,      // 0xa3
    0,      // 0xa4
    0,      // 0xa5
    0,      // 0xa6
    0,      // 0xa7
    0,      // 0xa8
    0,      // 0xa9
    0,      // 0xaa
    0,      // 0xab
    0,      // 0xac
    0,      // 0xad
    0,      // 0xae
    0,      // 0xaf
    0,      // 0xb0
    0,      // 0xb1
    0,      // 0xb2
    0,      // 0xb3
    0,      // 0xb4
    0,      // 0xb5
    0,      // 0xb6 KPLEFTPAREN
    0,      // 0xb7 KPRIGHTPAREN
    0,      // 0xb8
    0,      // 0xb9
    0,      // 0xba
    0,      // 0xbb
    0,      // 0xbc
    0,      // 0xbd
    0,      // 0xbe
    0,      // 0xbf
    0,      // 0xc0
    0,      // 0xc1
    0,      // 0xc2
    0,      // 0xc3
    0,      // 0xc4
    0,      // 0xc5
    0,      // 0xc6
    0,      // 0xc7
    0,      // 0xc8
    0,      // 0xc9
    0,      // 0xca
    0,      // 0xcb
    0,      // 0xcc
    0,      // 0xcd
    0,      // 0xce
    0,      // 0xcf
    0,      // 0xd0
    0,      // 0xd1
    0,      // 0xd2
    0,      // 0xd3
    0,      // 0xd4
    0,      // 0xd5
    0,      // 0xd6
    0,      // 0xd7
    0,      // 0xd8
    0,      // 0xd9
    0,      // 0xda
    0,      // 0xdb
    0,      // 0xdc
    0,      // 0xdd
    0,      // 0xde
    0,      // 0xdf
    0,      // 0xe0 LEFTCTRL
    0,      // 0xe1 LEFTSHIFT
    0,      // 0xe2 LEFTALT
    0,      // 0xe3 LEFTMETA
    0,      // 0xe4 RIGHTCTRL
    0,      // 0xe5 RIGHTSHIFT
    0,      // 0xe6 RIGHTALT
    0,      // 0xe7 RIGHTMETA
    0,      // 0xe8
    0,      // 0xe9
    0,      // 0xea
    0,      // 0xeb
    0,      // 0xec
    0,      // 0xed
    0,      // 0xee
    0,      // 0xef
    0,      // 0xf0
    0,      // 0xf1
    0,      // 0xf2
    0,      // 0xf3
    0,      // 0xf4
    0,      // 0xf5
    0,      // 0xf6
    0,      // 0xf7
    0,      // 0xf8
    0,      // 0xf9
    0,      // 0xfa
    0,      // 0xfb
    0,      // 0xfc
    0,      // 0xfd
    0,      // 0xfe
    0,      // 0xff
};

/* Wait for and read a keyboard character */