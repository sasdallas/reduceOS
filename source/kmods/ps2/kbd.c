/**
 * @file source/kmods/ps2/kbd.c
 * @brief Keyboard section of the PS/2 driver.
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include <kernel/isr.h>
#include <kernel/keyboard.h>
#include <kernel/ringbuffer.h>
#include <kernel/signal.h>
#include "ps2.h"

// Making life so much easier for me. Instead of manually switching between the scancodes in a switch() statement, just match them to this! So much easier.
const char scancodeChars[] = {'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8',  '9',  '0', '-',  '=',  '\b',
                              '\t', 'q',  'w', 'e', 'r', 't', 'y', 'u', 'i', 'o',  'p',  '[', ']',  '\n', '\0',
                              'a',  's',  'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  '\'', '`', '\0', '\\', 'z',
                              'x',  'c',  'v', 'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0'};

extern char ch;

extern bool keyboard_enabled;
extern bool keyboard_printChars;

// keyboardHandler(registers_t *r) - The handler assigned by keyboardInitialize to IRQ 33. Handles all scancode stuff.
static void keyboardHandler(registers_t* r) {
    // No matter if the handler is enabled or not, we need to read from port 0x60 or the keyboard might stop responding.
    uint8_t scancode = inportb(0x60);

    if (!keyboard_enabled) { return; } // Do not continue if not enabled.

    if (scancode & 0x80) {
        // This signifies a key was released. We don't care UNLESS it's the shift key or control key. Then we care.
        if (scancode == 0xAA || scancode == 0xB6) { // Left or right shift key was released
            setKBShiftKey(false);
        }

        if (scancode == 0x9D) { setKBCtrl(false); }
        ch = '\0';
    } else {
        switch (scancode) {
            case SCANCODE_CAPSLOCK:
                ch = '\0';
                if (getKBCapsLock()) {
                    setKBCapsLock(false);
                } // Caps lock release event
                else {
                    setKBCapsLock(true);
                }
                break;

            case SCANCODE_ENTER:
                ch = '\n';
                break;

            case SCANCODE_LEFTSHIFT:
            case SCANCODE_RIGHTSHIFT:
                ch = '\0';
                setKBShiftKey(true);
                break;

            case SCANCODE_CTRL:
                ch = '\0';
                setKBCtrl(true);
                break;

            case SCANCODE_TAB:
                ch = '\t';
                break;

            case SCANCODE_LEFT:
            case SCANCODE_RIGHT:
                ch = '\0';
                break;

            case SCANCODE_SPACE:
                ch = ' ';
                break;

            case SCANCODE_BACKSPACE:
                ch = '\b'; // \b will tell terminalPutchar() to handle this.
                break;

            default:
                ch = scancodeChars[(int)scancode];

                if (getKBCapsLock()) {
                    if (getKBShift()) {
                        ch = keyboard_altChars(ch);
                    } // If SHIFT key is also pressed, do nothing except convert to alternate chars.
                    else {
                        ch = toupper(ch);
                    } // Else, convert to upper case.
                } else {
                    if (getKBShift()) {
                        if (isalpha(ch))
                            ch = toupper(ch);
                        else { ch = keyboard_altChars(ch); }
                    }
                }
        }
    }

    // keyboardGetChar() will handle getting the char and returning it (for the shell).
    // Before we return we need to report that the key was pressed.
    if (ch <= 0 || ch == '\0') {
        // Do nothing if ch is 0 or \n.
    } else {
        keyboardRegisterKeyPress(ch);
    }

    return;
}

void ps2_kbd_init() { isrRegisterInterruptHandler(33, keyboardHandler); }