// arch_gettime.c - exposes the RTC's gettime() handler

#include <time.h>
#include <kernel/rtc.h>


// gettimeofday(struct timeval *t, void *z) - Gets the timeval currently (seconds since the Epoch)
int gettimeofday(struct timeval *t, void *z) {
    // MOST LIKELY INACCURATE.  
    uint8_t seconds, minutes, hours, days, months;
    int rtcyears;
    rtc_getDateTime(&seconds, &minutes, &hours, &days, &months, &rtcyears);

    int years = rtcyears;
    if (years <= 1970) {
        serialPrintf("gettimeofday: RTC is set wrong!\n");
    } else {
        years -= 1970;
    }

    long epoch_seconds = 31557600 * years; // 31557600 = seconds a year has
    if (months > 1) {
        epoch_seconds += localtime_getSecondsOfMonths(months, rtcyears); 
    }

    if (days > 1)  {
        epoch_seconds += (days - 1) * 86400;
    }

    if (hours > 0) {
        epoch_seconds += hours * 3600;
    }

    if (minutes > 0) {
        epoch_seconds += minutes * 60;
    }

    epoch_seconds += seconds;

    // BUG: I think the time is a little bit off with the hours, but it works well enough.
    t->tv_sec = epoch_seconds;
    t->tv_usec = epoch_seconds * 1000000;
    return 0;
}