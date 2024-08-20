// =============================================================================
// fpu.c - Floating-Point Unit
// =============================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/fpu.h> // Main header file

// fpu_isSupportedCPUID() - Returns whether the FPU is enabled in CPUID.
bool fpu_isSupportedCPUID() {
    // NOTE: This test isn't definitive.
    // CR0 bits such as EM/ET will be set if an FPU is not meant to be used/was not found.
    
    uint32_t edx;
    __cpuid(1, NULL, NULL, NULL, &edx);

    // Bit 0 is FPU support
    if (edx & (1 << 0)) {
        return true;
    } else {
        return false;
    }
}

// (static) fpu_write(uint32_t word) - Writes a value to the FPU control word
static void fpu_write(uint32_t word) {
    asm volatile ("fldcw %0" :: "m"(word));
}


int fpu_init() {
    // Check if the system has an FPU
    if (!fpu_isSupportedCPUID) return -1;

    // Initialize the FPU

    // Clear the TS and EM bits in CR0
    uint32_t cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2);
    cr0 &= ~(1 << 3);

    unsigned short test_word = 0xFFFF; // Test word for the FPU

    asm volatile (";     \
        mov %1, %%cr0;   \
        fninit;          \
        fnstsw %0" : "=r"(test_word) : "r"(cr0));
    
    if (test_word == 0x0) {
        // Set up control words
        fpu_write(0x37A);   // Division by zero + operand exceptions
        serialPrintf("fpu_init: FPU initialized\n");
        return 0;
    } else { 
        serialPrintf("fpu_init: Could not initialize FPU.\n");
        return -1;
    }
}

// fpu_ftoa(char *buf, float f) - Convert a floating point value to a string (probably should not be here)
void fpu_ftoa(char *buf, float f) {
    uint32_t count = 1;
    // Assume the decimal count is 8

    char int_buf[16];
    char *p;
    memset(int_buf, 0, 16);

    // Add the integer part
    int x = (int)f;
    serialPrintf("int conv: %i\n", x);
    itoa(x, int_buf, 10);
    p = int_buf;
    while (*p != '\0') *buf++ = *p++;

    *buf++ = '.'; // Add the decimal point

    // Subtract the integer part from the float
    float decimal = f - x;
    if (decimal == 0) {
        *buf++ = '0';
    } else {
        while (decimal > 0) {
            uint32_t y = decimal * 10;
            *buf++ = y + '0';
            decimal = (decimal * 10) - y;
            count++;
            if (count == 8) break;
        }
    }

    *buf = '\0';
}