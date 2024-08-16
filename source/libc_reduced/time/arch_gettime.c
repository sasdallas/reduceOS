// arch_gettime.c - exposes the RTC's gettime() handler

#include <time.h>
#include <kernel/rtc.h>
#include <kernel/clock.h>

// gettimeofday(struct timeval *t, void *z) - Gets the timeval currently (seconds since the Epoch)
int gettimeofday(struct timeval *t, void *z) {
    // We just forward this request to the kernel's clock
    return clock_gettimeofday(t, z);
}

// now() - Returns the current time of day for things that want a Unix timestamp.
uint64_t now() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec;
}

// settimeofday(struct timeval *t, void *z) - Set the clock time
int settimeofday(struct timeval *t, void *z) {
    // Again, forward this to the kernel's clock
    return clock_settimeofday(t, z);
} 