#ifndef SOUND_H
#define SOUND_H

#include "types.h"
#include <stdint.h>
// Play a sound at a certain frequency
void playSound(uint8_t nFreq);
// Stop all sounds
void stopSound();
// Beep
void beep();

#endif
