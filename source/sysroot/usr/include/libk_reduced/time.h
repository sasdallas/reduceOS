// time.h - what day is it? i forgot

#ifndef TIME_H
#define TIME_H

// Includes
#include <kernel/rtc.h>
#include <libk_reduced/stdint.h>

// Typedefs
typedef unsigned long long time_t;
typedef unsigned long long suseconds_t;


// This is a structure containing actual time data (does conform to C standard)
typedef struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;

    const char *_tm_zone_name;
    int _tm_zone_offset;
};

// Seconds/microseconds timeval
struct timeval {
    time_t tv_sec;          // Seconds
    suseconds_t tv_usec;    // Microseconds
};

struct timezone {
    int tz_minuteswest;     // Minutes west of Greenwich
    int tz_dsttime;         // Type of Daylight Savings Time correction
};


// Functions

size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
time_t mktime(struct tm *tm);
struct tm *localtime_r(const time_t *ptr, struct tm *timeValue);
struct tm *gmtime_r(const time_t *ptr, struct tm *timeValue);
struct tm *localtime(const time_t *ptr);
struct tm *gmtime(const time_t *ptr);
int gettimeofday(struct timeval *t, void *z);
time_t time(time_t *out);
double difftime(time_t a, time_t b);
char * asctime(const struct tm *tm);
bool localtime_isYearLeap(int year);
long localtime_getDaysInMonth(int month, int year);
uint64_t now();

#endif
