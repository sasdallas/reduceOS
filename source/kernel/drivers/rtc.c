// =======================================================
// rtc.c - System real-time clock driver (like CMOS)
// =======================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/rtc.h" // Main header file


// It's important to note that this file doesn't go very in-depth in the RTC - it only reads time and date from it.
// All RTC ports/registers are defiend in rtc.h.

// Later, when ACPI is implemented, we can use a variable here to get the century.

// rtc_getUpdateInProgress() - Returns if the RTC is doing an update
int rtc_getUpdateInProgress() {
    outportb(CMOS_ADDRESS, 0x0A);
    return (inportb(CMOS_DATA) & 0x80);
}


// rtc_getRegister(int register) - Returns the value of an RTC register
uint8_t rtc_getRegister(int reg) {
    outportb(CMOS_ADDRESS, reg);
    return inportb(CMOS_DATA);
} 

// rtc_getDateTime(uint8_t *second, uint8_t *minute, uint8_t *hour, uint8_t *day, uint8_t *month, uint8_t *year) - Returns the date and time (in the pointers)
void rtc_getDateTime(uint8_t *second, uint8_t *minute, uint8_t *hour, uint8_t *day, uint8_t *month, int *year) {
    uint8_t lastSecond, lastMinute, lastHour, lastDay, lastMonth, lastYear, lastCentury, registerB;
    // Because of some weird pointer logic, we have to make more variables and then set the pointers to the variables.
    uint8_t seconds, minutes, hours, days, months, years;

    // First, make sure an update isn't in progress.
    while (rtc_getUpdateInProgress());

    // Get the registers.
    seconds = rtc_getRegister(RTC_SECOND_REGISTER);
    minutes = rtc_getRegister(RTC_MINUTE_REGISTER);
    hours = rtc_getRegister(RTC_HOUR_REGISTER);
    days = rtc_getRegister(RTC_DAY_REGISTER);
    months = rtc_getRegister(RTC_MONTH_REGISTER);
    years = rtc_getRegister(RTC_YEAR_REGISTER);

    // ACPI will come into play later.

    // Do a little workaround to get around RTC updates.
    do {
        lastSecond = seconds;
        lastMinute = minutes;
        lastHour = hours;
        lastDay = days;
        lastMonth = months;
        lastYear = years;

        // Get new values.
        while (rtc_getUpdateInProgress());
        seconds = rtc_getRegister(RTC_SECOND_REGISTER);
        minutes = rtc_getRegister(RTC_MINUTE_REGISTER);
        hours = rtc_getRegister(RTC_HOUR_REGISTER);
        days = rtc_getRegister(RTC_DAY_REGISTER);
        months = rtc_getRegister(RTC_MONTH_REGISTER);
        years = rtc_getRegister(RTC_YEAR_REGISTER);        
    } while ((lastSecond != seconds) || (lastMinute != minutes) || (lastHour != hours) ||
             (lastDay != days) || (lastMonth != months) || (lastYear != years));

    registerB = rtc_getRegister(0x08);

    // Now, convert BCD -> binary values (if necessary)
    if (!(registerB & 0x04)) {
        seconds = (seconds & 0x0F) + ((seconds / 16) * 10);
        minutes = (minutes & 0x0F) + ((minutes / 16) * 10);
        hours = ( (hours & 0x0F) + (((hours & 0x70) / 16) * 10)) | (hours & 0x80);
        days = (days & 0x0F) + ((days / 16) * 10);
        months = (months & 0x0F) + ((months / 16) * 10);
        years = (years & 0x0F) + ((years / 16) * 10);
    }

    // Convert 12 hour to 24 hour if necessary.
    if (!(registerB & 0x02) && (hours & 0x80)) {
        hours = ((hours & 0x7F) + 12) % 24;
    }

    // Calculate full 4-digit year - by making another variable cause conversions are WEIRD
    int actualYears = (int)years;
    actualYears += ((RTC_CURRENT_YEAR / 100) * 100);
    if (actualYears < RTC_CURRENT_YEAR) actualYears += 100;
    
    // Set the pointers.
    *second = seconds;
    *minute = minutes;
    *hour = hours;
    *day = days;
    *month = months;
    *year = actualYears;

}