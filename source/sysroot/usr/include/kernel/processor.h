// processor.h - Handles all CPU related functions

#ifndef PROCESSOR_H
#define PROCESSOR_H

// Includes
#include <stdint.h> // Integer declarations
#include <kernel/gdt.h> // Global descriptor table
#include <kernel/idt.h> // Interrupt descriptor table
#include <kernel/isr.h> // Interrupt service routines
#include <kernel/hal.h> // Hardware Abstraction Layer


// Typedefs

typedef struct {
    int XOP_support; // XOP support
    int FMA4_support; // FMA4 support
    int CVT16_support; // CVT16 support
    int AVX_support; // AVX support (Advanced Vector Extensions)
    int XSAVE_support; // XSAVE support (CPU extended power management support)
    int AVX2_support; // AVX2 support
    // TODO: AVX-512 support
} sseData_t;

typedef struct {
    char vendor[13]; // Vendor data
    int long_mode_capable; // Long mode capabilities
    uint32_t frequency; // Frequency (in Hz)

    // Streaming SIMD Extensions
    int sse_support; // SSE support
    int sse2_support; // SSE2 support
    int sse3_support; // SSE3 support 
    int ssse3_support; // SSSE3 Support (Supplemental Streaming SIMD Extensions)
    int sse4_support; // SSE4 support
    sseData_t sse5_Data; // SSE5 data

    // FPU support
    int fpuEnabled;
} cpuInfo_t;



// Functions
void cpuInit(); // Initializes the CPU with ISR, IDT, and GDT
uint32_t getCPUFrequency(); // Returns the CPU frequency (todo: create processor.c).
bool isCPULongModeCapable(); // Returns if the CPU is 64-bit capable
char *getCPUVendorData(); // Returns CPU vendor data
cpuInfo_t getCPUProcessorData();

#endif
