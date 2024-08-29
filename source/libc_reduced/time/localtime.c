// localtime.c - replacement for the libc file localtime.c

#include <libk_reduced/time.h>
#include <libk_reduced/stdbool.h>

/* HELPER FUNCTIONS */

// localtime_isYearLeap(int year) - Returns whether the year is a leap year
bool localtime_isYearLeap(int year) {
    return ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)));
}

// localtime_getDaysInMonth(int month, int year) - Technically the best way to do it
long localtime_getDaysInMonth(int month, int year) {
    switch (month) {
        case 1:
            return 31; // January
        case 2:
            return localtime_isYearLeap(year) ? 29 : 28; // February
        case 3:
            return 31; // March
        case 4:
            return 30; // April
        case 5:
            return 31; // May
        case 6:
            return 30; // June
        case 7:
            return 31; // July
        case 8:
            return 31; // August
        case 9:
            return 30; // September
        case 10:
            return 31; // October
        case 11:
            return 30; // November
        case 12:
            return 31; // December
    }

    return 0; // bruh
}

// localtime_getDayOfWeek(long seconds) - Returns the day of a week
int localtime_getDayOfWeek(long seconds) {
    long day = seconds / 86400; // 86400 seconds a day
    day += 4; // starts on thursday for some reason
    return day % 7;
}

// localtime_getSecondsOfMonths(int months, int year)
long localtime_getSecondsOfMonths(int months, int year) {
    long days = 0;
    for (int i = 1; i < months; i++) {
        // index starting from 1, scaring all C programmers
        days += localtime_getDaysInMonth(i, year);
    }

    return days * 86400;
}



// localtime_getSecondsOfYears(int years)
unsigned int localtime_getSecondsOfYears(int years) {
    unsigned int days = 0;
    while (years > 1969) {
        days += 365;
        if (localtime_isYearLeap(years)) days++;
        years--;
    }
    return days * 86400;
}

// localtime_fillTime(const time_t *time_ptr, struct tm *tm, const char *tzName, int tzOffset) - Creates a struct tm object
struct tm *localtime_fillTime(const time_t *time_ptr, struct tm *tm, const char *tzName, int tzOffset) {
    time_t timeValue = *time_ptr + tzOffset; // tzOffset should be 0 because we don't actually have timezone support yet
    tm->_tm_zone_name = tzName;
    tm->_tm_zone_offset = tzOffset;

    long seconds = timeValue < 0 ? -2208988800L : 0;
    long year_sec = 0;

    int startingyear = timeValue < 0 ? 1900 : 1970;

    for (int year = startingyear; year < 2100; year++) {
        long added = localtime_isYearLeap(year) ? 366 : 365;
        long secs = added * 86400;

        if (seconds + secs > timeValue) {
            tm->tm_year = year - 1900;
            year_sec = seconds;
            for (int month = 1; month <= 12; month++) {
                secs = localtime_getDaysInMonth(month, year) * 86400;
                if (seconds + secs > timeValue) {
                    tm->tm_mon = month - 1;
                    for (int day = 1; day <= localtime_getDaysInMonth(month, year); day++) {
                        secs = 60 * 60 * 24;
                        if (seconds  + secs > timeValue) {
                            tm->tm_mday = day;
                            for (int hour = 1; hour <= 24; hour++) {
                                secs = 60 * 60;
                                if (seconds + secs > timeValue) {
                                    long remaining = timeValue - seconds;
                                    tm->tm_hour = hour - 1;
                                    tm->tm_min = remaining / 60;
                                    tm->tm_sec = remaining % 60;
                                    tm->tm_wday = localtime_getDayOfWeek(timeValue);
                                    tm->tm_yday = (timeValue - year_sec) / 86400;
                                    tm->tm_isdst = 0;
                                    return tm;
                                } else {
                                    seconds += secs;
                                }
                            }
                            return NULL;
                        } else {
                            seconds += secs;
                        }
                    }
                    return NULL;
                } else {
                    seconds += secs;
                }
            }
            return NULL;
        } else {
            seconds += secs;
        }
    }
    return (void*)0;
}


/*
    TODO: Timezone support (who cares)
*/


static struct tm _timevalue;

/* EXPOSED FUNCTIONS */


// localtime_r(const time_t *ptr, struct tm *timeValue) - Thread-safe localtime()
struct tm *localtime_r(const time_t *ptr, struct tm *timeValue) {
    return localtime_fillTime(ptr, timeValue, "UTC", 0); // Default to UTC always
}

// gmtime_r(const time_t *ptr, struct tm *timeValue) - Thread-safe gmtime()
struct tm *gmtime_r(const time_t *ptr, struct tm *timeValue) {
    return localtime_fillTime(ptr, timeValue, "UTC", 0);
}

// localtime(const time_t *ptr) - Returns the localtime
struct tm *localtime(const time_t *ptr) {
    return localtime_fillTime(ptr, &_timevalue, "UTC", 0); // Default to UTC always
}

// gmtime(const time_t *ptr) - Returns the UTC time
struct tm *gmtime(const time_t *ptr) {
    return localtime_fillTime(ptr, &_timevalue, "UTC", 0);
}

// mktime(struct tm *tm) - Constructs a time_t object with the current time
time_t mktime(struct tm *tm) {
    return
        (localtime_getSecondsOfYears(tm->tm_year + 1899)) +
        (localtime_getSecondsOfMonths(tm->tm_mon + 1, tm->tm_year + 1900)) +
        (tm->tm_mday - 1) * 86400 +
        (tm->tm_hour) * 3600 +
        (tm->tm_min) * 60 + 
        (tm->tm_sec) - tm->_tm_zone_offset; 
}
