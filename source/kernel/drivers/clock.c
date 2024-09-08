// ==========================================================================================
// clock.c - Real-time clock driver (note: this handles TSC + the overall clock handler)
// ==========================================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/clock.h> // Main header file
#include <kernel/process.h>
#include <libk_reduced/time.h>


// Note: Clock implementation ties in with the ToaruOS-sourced process scheduler.

uint64_t boottime = 0; // Calculated on startup
uint64_t tsc_baseline = 0; // Accumulated time on the TSC when it was timed (baseline)
uint64_t tsc_mhz = 0x1337; // Calculated on startup to determine TSC difference.

static spinlock_t *timeset_lock; // Lock for setting time
static spinlock_t *clock_lock; // Not very useful right now.


clock_callback_func callbacks[MAX_CLOCK_FUNCTIONS];
static int callback_index = 0;



// (static) yearsToSeconds(int years) - Unix millisecond conversion
static uint64_t yearsToSeconds(int years) {
    uint64_t days = 0;
    if (years < 2000) years += 2000; // Probably bad, what if their clock isn't set? Is that even a problem? I don't know lmao

    while (years > 1969) {
        days += 365;
        if (localtime_isYearLeap(years)) days++;
        years--;
    }
    return days * 86400;
}

// (static) monthsToSeconds(int months, int years) - Returns the amount of seconds in a month
static uint64_t monthsToSeconds(int months, int years) {
    if (years < 2000) years += 2000;

    long days = 0;
    for (int i = 1; i < months; i++) {
        // index starting from 1, scaring all C programmers
        days += localtime_getDaysInMonth(i, years);
    }

    return days * 86400;
}


// clock_convertCMOSToUnix() - Converts the CMOS to a Unix timestamp (TODO: This should be done in cmos.c)
uint64_t clock_convertCMOSToUnix() {
    uint16_t values[128]; // stack allocation (gross)
    uint16_t old_values[128];

    while (cmos_isUpdateInProgress());
    cmos_dump(values);

    do {
        memcpy(old_values, values, 128);
        while (cmos_isUpdateInProgress());
        cmos_dump(values);
    } while ((old_values[CMOS_SECOND] != values[CMOS_SECOND]) ||
            (old_values[CMOS_MINUTE] != values[CMOS_MINUTE]) ||
            (old_values[CMOS_HOUR] != values[CMOS_HOUR]) ||
            (old_values[CMOS_DAY] != values[CMOS_DAY]) || 
            (old_values[CMOS_MONTH] != values[CMOS_MONTH]) ||
            (old_values[CMOS_YEAR] != values[CMOS_YEAR]));
    

    // It is important to note that this code is very similar to the RTC code in our driver.
    // But I don't know about the reliability of that driver, and I would rather not risk it and spend hours debugging.
    // Marked as TODO.

    uint64_t time = yearsToSeconds(from_bcd(values[CMOS_YEAR]) - 1) +
                    monthsToSeconds(from_bcd(values[CMOS_MONTH]) - 1,
                    from_bcd(values[CMOS_YEAR])) +
                    (from_bcd(values[CMOS_DAY]) - 1) * 86400 +
                    (from_bcd(values[CMOS_HOUR])) * 3600 +
                    (from_bcd(values[CMOS_MINUTE])) * 60 +
                    from_bcd(values[CMOS_SECOND]) + 0;

    return time;
}

// clock_readTSC() - Reads the CPU timestamp counter
static uint64_t clock_readTSC() {
    // Important to note that this value is pretty useless without the MHz.
    uint32_t lo, hi;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi)); // rdtsc outputs lo in EAX and hi in EDX
    return ((uint64_t)hi << 32UL) | (uint64_t)lo;
}

// clock_getTimer() - Exposed interface that is used by other applications and is customizable.
uint64_t clock_getTimer() {
    // We'll use TSC
    return clock_readTSC();
}


// clock_getTSCSpeed() - Returns the calculated MHz (is it MHz?) of the TSC
size_t clock_getTSCSpeed() {
    return tsc_mhz;
}

// clock_init() - Initializes the clock driver, calculating boot time, TSC rate, ...
void clock_init() {
    serialPrintf("clock: Initializing clock system...\n");

    boottime = clock_convertCMOSToUnix();

    uintptr_t end_lo, end_hi;
    uint32_t start_lo, start_hi;


    /*
    * I can't even begin to pretend I wrote this code. 
    * It interfaces with the PIT (one-shot mode) and does CPU/TSC calculation.
    * To do this, it modifies the wall clock configuration.
    * All credits to ToaruOS.
    * 
    * Note: It appears that this causes some issues on Bochs, such as incorrect calculation and ear destruction.
    * Who thought it would be a good idea to have the speaker on PIT channel 2?
    * Will investigate further
    */
    asm volatile (
        // Disable PIT gating on channel 2
        "inb $0x61, %%al\n"
        "andb $0xDD, %%al\n"
        "orb $0x01, %%al\n"
        "outb %%al, $0x61\n"

        // Configure channel 2 to one-shot, next two bytes are low/high
        "movb $0xB2, %%al\n" // 10110010
        "outb %%al, $0x43\n"

        // 0x__9b
        "movb $0x9B, %%al\n"
        "outb %%al, $0x42\n"
        "inb $0x60\n"

        // 0x2e__
        "movb $0x2E, %%al\n"
        "outb %%al, $0x42\n"

        // Re-enable
        "inb $0x61, %%al\n"
        "andb $0xDE, %%al\n"
        "outb %%al, $0x61\n"

        // Pulse high
        "orb $0x01, %%al\n"
        "outb %%al, $0x61\n"

        // Read TSC and store it
        "rdtsc\n"
        "movl %%eax, %2\n"
        "movl %%edx, %3\n"

        // QEMU and Virtualbox: This will flip low
        // Real hardware (+ VMWare): This will flip high
        "inb $0x61, %%al\n"
        "andb $0x20, %%al\n"
        "jz 2f\n"
    "1:\n"
        // Loop until output goes low
        "inb $0x61, %%al\n"
        "andb $0x20, %%al\n"
        "jnz 1b\n"
        "rdtsc\n"
        "jmp 3f\n"
    "2:\n"
        // Loop until the output goes high
        "inb $0x61, %%al\n"
        "andb $0x20, %%al\n"
        "jz 2b\n"
        "rdtsc\n"
    "3:\n"
        : "=a"(end_lo), "=d"(end_hi), "=r"(start_lo), "=r"(start_hi)
    );

    // Finalize by doing some bitmask calculation
    uint64_t end = ((uint64_t)(end_hi & 0xFFFFffff) << 32) | (end_lo & 0xFFFFffff);
    uint64_t start = ((uint64_t)(start_hi & 0xFFFFffff) << 32) | (start_lo & 0xFFFFffff);
    tsc_mhz = (end - start) / 10000;
    if (tsc_mhz == 0) {
        serialPrintf("clock_init: oh no\n");
        tsc_mhz = 2000;
    }

    tsc_baseline = start / tsc_mhz;

    serialPrintf("clock: TSC calculated speed is %lu MHz\n", tsc_mhz);
    serialPrintf("clock: Boot time is %lus (UNIX timestamp).\n", boottime);
    serialPrintf("clock: Initial TSC timestamp was %luus\n", tsc_baseline);

    // Initialize the spinlocks
    timeset_lock = spinlock_init();
    clock_lock = spinlock_init();
}


// (static) clock_updateTicks(uint64_t ticks, uint64_t *timer_ticks, uint64_t *timer_subticks) - Subdivides ticks into seconds in subticks
static void clock_updateTicks(uint64_t ticks, uint64_t *timer_ticks, uint64_t *timer_subticks) {
    *timer_subticks = ticks - tsc_baseline;
    *timer_ticks = *timer_subticks / SUBSECONDS_PER_SECOND;
    *timer_subticks = *timer_subticks % SUBSECONDS_PER_SECOND;
}

// clock_gettimeofday(struct timeval *t, void *z) - DO NOT CALL. USE GETTIMEOFDAY(), SINCE IT USES THIS.
int clock_gettimeofday(struct timeval *t, void *z) {
    uint64_t tsc = clock_readTSC();
    uint64_t timer_ticks, timer_subticks;
    clock_updateTicks(tsc / tsc_mhz, &timer_ticks, &timer_subticks);
    t->tv_sec = boottime + timer_ticks;
    t->tv_usec = timer_subticks;
    return 0;
}

// clock_settimeofday(struct timeval *t, void *z) - Same as above (do not call)
int clock_settimeofday(struct timeval *t, void *z) {
    if (!t) return -1;
    if (t->tv_usec > 1000000) return -1;

    spinlock_lock(timeset_lock);
    uint64_t clock_time = now();
    boottime += t->tv_sec - clock_time;
    spinlock_release(timeset_lock);

    return 0;
}


// clock_relative(unsigned long seconds, unsigned long subseconds, unsigned long *out_seconds, unsigned long *out_subseconds) - Calculate time in the future
void clock_relative(unsigned long seconds, unsigned long subseconds, unsigned long *out_seconds, unsigned long *out_subseconds) {
    if (!boottime) {
        // ???
        *out_seconds = 0;
        *out_subseconds = 0;
        return;
    }

    // Read the TSC and calculate the time
    uint64_t tsc = clock_readTSC();
    uint64_t timer_ticks, timer_subticks;
    clock_updateTicks(tsc / tsc_mhz, &timer_ticks, &timer_subticks);
    if (subseconds + timer_subticks >= SUBSECONDS_PER_SECOND) {
        *out_seconds = timer_ticks + seconds + (subseconds + timer_subticks) / SUBSECONDS_PER_SECOND;
        *out_subseconds = (subseconds + timer_subticks) % SUBSECONDS_PER_SECOND;
    } else {
        *out_seconds = timer_ticks + seconds;
        *out_subseconds = timer_subticks + subseconds;
    }
}

// clock_update() - Update the clock
void clock_update() {
    spinlock_lock(clock_lock);

    // Get the TSC timestamp in microseconds
    uint64_t clock_ticks = clock_readTSC() / tsc_mhz;

    // Convert it to seconds and subseconds
    uint64_t timer_ticks, timer_subticks;
    clock_updateTicks(clock_ticks, &timer_ticks, &timer_subticks);

    spinlock_release(clock_lock);

    // Let's go through each callback and call it
    for (int i = 0; i < callback_index; i++) {
        clock_callback_func func = callbacks[i];
        func(timer_ticks, timer_subticks);
    }

    // Wakeup sleeping processes
    wakeup_sleepers(timer_ticks, timer_subticks);
}

// clock_registerCallback(clock_callback_func func) - Register a clock callback
void clock_registerCallback(clock_callback_func func) {
    if (callback_index >= MAX_CLOCK_FUNCTIONS) {
        panic("clock", "callback", "Maximum amount of callback functions reached");
        __builtin_unreachable();
    }

    callbacks[callback_index] = func;
    callback_index++;
}

