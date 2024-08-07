// rtc.h - header file for the real-time clock driver

#ifndef RTC_H
#define RTC_H

// Includes
#include <stdint.h> // Integer definitions
#include <kernel/hal.h> // Hardware abstraction layer

// WARNING: The below is a user-created constant! This needs to change each year and cannot automatically update!
#define RTC_CURRENT_YEAR 2023 // wooo 2023

// The rest are just ports and stuff.
#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

#define RTC_SECOND_REGISTER 0x00
#define RTC_MINUTE_REGISTER 0x02
#define RTC_HOUR_REGISTER 0x04
#define RTC_DAY_REGISTER 0x07
#define RTC_MONTH_REGISTER 0x08
#define RTC_YEAR_REGISTER 0x09

// Functions
void rtc_getDateTime(uint8_t *second, uint8_t *minute, uint8_t *hour, uint8_t *day, uint8_t *month, int *year); // Returns the date and time (in the pointers)

#endif
