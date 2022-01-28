// Sound.c - Sound driver
#include "io_ports.h" // outportb and other methods
#include "timer.h" // PIT timer
#include "types.h" // Types
#include <stdint.h>
// Written for PC speaker
// NOTE: Able to run on real hardware AND QEMU if you add -soundhw pcspk on the options


void playSound(uint32_t nFreq) {
    uint32_t Div;
    uint8_t tmp;

    // Change PIT to desired frequency
    Div = 1193180 / nFreq;
    outportb(0x43, 0xb6);
    outportb(0x42, (uint8_t) (Div) );
    outportb(0x42, (uint8_t) (Div >> 8));


    // Play the sound
    tmp = inportb(0x61);
    if (tmp != (tmp | 3)) {
        outportb(0x61, tmp | 3);
    }

}

// Stop playing sound
void stopSound() {
    uint8_t tmp = inportb(0x61) & 0xF3;
    outportb(0x61, tmp);
}

// Make the speaker beep
void beep() {
    playSound(1000);
    sleep(10);
    stopSound();
}
