// ======================================================
// processor.c - Handles all CPU related functions
// ======================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/processor.h> // Main header file
#include <kernel/fpu.h>
#include <libk_reduced/stdio.h>


// Variables
static cpuInfo_t processor_data; // TODO: Seems like not ht ebest way to do things

// Function prototypes
void cpuCheckSSE();

// cpuInit() - Initializes the CPU with ISR, IDT, and GDT
void cpuInit() {
    // Load GDT and IDT (IDT method sets up ISR as well as PIC)
    gdtInit();
    idtInit();
	
    serialPrintf("[i386]: GDT/IDT installed\n");

    // Enable interrupts.
    enableHardwareInterrupts();
    serialPrintf("[i386]: Hardware interrupts enabled.\n");

	
	// We want a fast boot so we'll only detect CPU frequency when needed.
	processor_data.frequency = 0;

	// Now grab some vendor data.
	uint32_t eaxvar; // Unused
	memset(processor_data.vendor, 0, sizeof(processor_data.vendor));
	__cpuid(0, &eaxvar, (uint32_t*)&(processor_data.vendor[0]), (uint32_t*)&(processor_data.vendor[8]), (uint32_t*)&(processor_data.vendor[4]));	
	
	int edx = 0;

	// Also, get CPU long mode support.
    asm volatile ("movl $0x80000001, %%eax\n"
                "cpuid\n" : "=d"(edx) :: "eax", "ebx", "ecx");


	processor_data.long_mode_capable = edx & (1 << 29) ? 1 : 0;

	// SSE Support Checking
	cpuCheckSSE();

	// Initialize the FPU
	int fpu = fpu_init();
	processor_data.fpuEnabled = (fpu == 0) ? 1 : 0;


	// Print a summary
	// Now print a little summary
	serialPrintf("======== CPU Data Collection Summary ========\n");
	serialPrintf("- CPU VENDOR ID: %s\n", processor_data.vendor);
	serialPrintf("- Long Mode (x64) support: %i\n", processor_data.long_mode_capable);
	serialPrintf("- FPU support: %s\n\n", (fpu == 0) ? "YES": "NO");

	serialPrintf("== SSE Data Collection Summary ==\n");
	serialPrintf("SSE support: %s\n", processor_data.sse_support ? "YES" : "NO");
	serialPrintf("SSE2 support: %s\n", processor_data.sse2_support ? "YES" : "NO");
	serialPrintf("SSE3 support: %s\n", processor_data.sse3_support ? "YES" : "NO");
	serialPrintf("SSSE3 support: %s\n", processor_data.ssse3_support ? "YES" : "NO");
	serialPrintf("SSE4 support: %s\n", processor_data.sse4_support ? "YES" : "NO");
	serialPrintf("SSE5 support data summary:\n");
	serialPrintf("\tXOP support: %s\n", processor_data.sse5_Data.XOP_support ? "YES" : "NO");
	serialPrintf("\tFMA4 support: %s\n", processor_data.sse5_Data.FMA4_support ? "YES" : "NO");
	serialPrintf("\tCVT16 support: %s\n", processor_data.sse5_Data.CVT16_support ? "YES" : "NO");
	serialPrintf("\tAVX support: %s\n", processor_data.sse5_Data.AVX_support ? "YES" : "NO");
	serialPrintf("\tXSAVE support: %s\n", processor_data.sse5_Data.XSAVE_support ? "YES" : "NO");
	serialPrintf("\tAVX2 support: %s\n", processor_data.sse5_Data.AVX2_support ? "YES" : "NO");
	serialPrintf("== End SSE Data Collection Summary ==\n");
	serialPrintf("======== End CPU Data Collection Summary ========\n");

	// Done!
	

	printf("CPU initialization completed.\n");
	serialPrintf("CPU initialization completed\n");


	return;
	
}


// cpuCheckSSE() - Runs all the SSE checks and assigns correct values
void cpuCheckSSE() {
	/* 
		In case you don't know what SSE is, it stands for Streaming SIMD Extensions
	 	They are a special set of instructions known as single instruction, multiple data
		These instructions can give a very high increase in data throughput 
		As well as that, they add a few XMM registers (and even some 128-bit ones!) which can load 16 bytes from memory or store 16 bytes into memory with a single instruction
	*/

	uint32_t eax; // Unused
	uint32_t edx; // SSE and SSE2 bits found here
	uint32_t ecx; // SSE3, SSE4, and most of SSE5 are found here 
	uint32_t ebx; // AVX2 support checking

	__cpuid(1, &eax, &ebx, &ecx, &edx);

	processor_data.sse_support = edx & (1 << 25); // SSE support (bit 25 of EDX)
	processor_data.sse2_support = edx & (1 << 26); // SSE2 support (bit 26 of EDX)
	processor_data.sse3_support = ecx & (1 << 0); // SSE3 support (bit 0 of ECX)
	processor_data.ssse3_support = ecx & (1 << 9); // SSSE3 support (Supplemntal Streaming SIMD Extensions, bit 9 of ECX)
	

	// ok so basically I don't actually know if it changes whether SSE4.1, SSE4.2, or SSE4A is on or off
	// soo only enable SSE4 support if all 3 are supported
	processor_data.sse4_support = 0;

	if ((ecx & (1 << 19)) && (ecx & (1 << 20)) && (ecx & (1 << 6))) {
		processor_data.sse4_support = 1;
	}

	// SSE5 time, this is more complex, because it's split up
	// I created a type called sseData_t to help out

	processor_data.sse5_Data.XOP_support = ecx & (1 << 11); // XOP support
	processor_data.sse5_Data.FMA4_support = ecx & (1 << 16); // FMA4 support
	processor_data.sse5_Data.CVT16_support = ecx & (1 << 29); // CVT16 support
	processor_data.sse5_Data.AVX_support = ecx & (1 << 28); // AVX support
	processor_data.sse5_Data.XSAVE_support = ecx & (1 << 26); // XSAVE support
	processor_data.sse5_Data.AVX2_support = ecx & (1 << 5); // AVX2 support (potentially wrong page for CPUID?)

	// Hopefully done (praying I don't have to debug but some SSE stuff doesn't seem to work)


	
}


// detectCPUFrequency() - Detect the CPU frequency (buggy and takes time)
uint32_t detectCPUFrequency() {
	uint64_t start, end, diff;
	uint64_t ticks, old;

	if (processor_data.frequency > 0)
		return processor_data.frequency;

	old = pitGetTickCount();

	// Wait for the next time slice.
	while((ticks = pitGetTickCount()) - old == 0) continue;

    asm volatile ("rdtsc" : "=A"(start));
	// Wait a second to determine frequency.
	while(pitGetTickCount() - ticks < 1000) continue;
	asm volatile ("rdtsc" : "=A"(end));

	diff = end > start ? end - start : start - end;
	processor_data.frequency = (uint32_t) (diff / (uint64_t) 1000000);

	return processor_data.frequency;
}


// getCPUFrequency() - Returns the CPU frequency and gets it if it is not present.
uint32_t getCPUFrequency() {	
	if (processor_data.frequency > 0)
		return processor_data.frequency;

	return detectCPUFrequency();
}



// getCPUVendorData() - Returns CPU vendor data
char *getCPUVendorData() {
	return processor_data.vendor;
}

bool isCPULongModeCapable() {
	return processor_data.long_mode_capable ? true : false;
}

cpuInfo_t getCPUProcessorData() {
	return processor_data;
}
