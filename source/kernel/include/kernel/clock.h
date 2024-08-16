// clock.h - The header file for the kernel's clock driver

#ifndef CLOCK_H
#define CLOCK_H

// Includes
#include <stdint.h>
#include <time.h>
#include <spinlock.h>
#include <stdatomic.h>
#include <kernel/rtc.h> // Real-time clock driver
#include <kernel/pit.h> // PIT driver
#include <kernel/isr.h> // Interrupt Service Registers
#include <kernel/cmos.h> // CMOS driver

// Definitions
#define SUBSECONDS_PER_SECOND 1000000

// Macros
#define from_bcd(n) ((n / 16) * 10 + (n & 0xF)) // who the hell thought it would be a good idea to make CMOS use BCD
                                                // BCD is terrible


// Functions
uint64_t clock_getTimer(); // Reads + returns the TSC
size_t clock_getTSCSpeed(); // Returns the calculated TSC speed
void clock_init(); // Initialize the clock
int clock_gettimeofday(struct timeval *t, void *z); // Do not call. Use gettimeofday()
int clock_settimeofday(struct timeval *t, void *z);
void clock_relative(unsigned long seconds, unsigned long subseconds, unsigned long *out_seconds, unsigned long *out_subseconds); // Calculate a time in the future
void clock_update(); // Update the clock



#endif