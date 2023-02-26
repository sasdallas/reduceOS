// tss.h - Task State Segment structure definitions

#ifndef TSS_H
#define TSS_H

// Includes
#include "include/libc/stdint.h" // Integer definitions
#include "include/tasking_t.h"
#include "include/tasking.h"
#include "include/gdt.h"


// Typedefs
// Note: This will need to be updated when we upgrade to reduceOS x86_64.
// However, by then we may have an entirely new TSS implementation, so, eh.

typedef struct {
    uint16_t backlink;
    uint32_t esp0;
    uint16_t ss0;
    uint32_t esp1;
    uint16_t ss1;
    uint16_t __ss1h;
	uint32_t esp2;
	uint16_t ss2;
    uint16_t __ss2h;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint16_t es, __esh;
	uint16_t cs, __csh;
	uint16_t ss, __ssh;
	uint16_t ds, __dsh;
	uint16_t fs, __fsh;
	uint16_t gs, __gsh;
	uint16_t ldt, __ldth;
	uint16_t trace, bitmap;
} __attribute__ ((packed)) tss_t;



// External functions
extern void tssFlush();

// Functions
void tssInit(uint32_t index, uint32_t kernel_ss, uint32_t kernel_esp);
void setKernelStack();

#endif
