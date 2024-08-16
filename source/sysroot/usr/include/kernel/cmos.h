// cmos.h - header file for the CMOS driver

#ifndef CMOS_H
#define CMOS_H

// Includes
#include <stdint.h>
#include <kernel/hal.h>

// Definitions
#define CMOS_REGISTER_SECONDS   0x00
#define CMOS_REGISTER_MINUTES   0x02
#define CMOS_REGISTER_HOURS     0x04
#define CMOS_REGISTER_WEEKDAY   0x06
#define CMOS_REGISTER_DAY       0x07
#define CMOS_REGISTER_MONTH     0x08
#define CMOS_REGISTER_YEAR      0x09
#define CMOS_REGISTER_CENTURY   0x32    // Hit or miss

#define CMOS_REGISTER_STATUSA   0x0A
#define CMOS_REGISTER_STATUSB   0x0B

#define CMOS_ADDRESS            0x70
#define CMOS_DATA               0x71

// Enums
enum {
    CMOS_SECOND = 0,
    CMOS_MINUTE = 2,
    CMOS_HOUR = 4,
    CMOS_DAY = 7,
    CMOS_MONTH = 8,
    CMOS_YEAR = 9
};

// Functions
uint8_t cmos_readRegister(uint8_t cmosreg);
void cmos_writeRegister(uint8_t cmosreg, uint8_t value);
void cmos_dump(uint16_t *values);
int cmos_isUpdateInProgress();
#endif