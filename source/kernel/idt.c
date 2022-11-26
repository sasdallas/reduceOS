// =====================================================================
// idt.c - Interrupt Descriptor Table
// This file handles setting up the Interrupt Descriptor Table (IDT)
// =====================================================================

#include "include/idt.h" // IDT header file

typedef struct {
    uint16_t limit; // size of the IDT
    uint32_t base_addr; // base address of the IDT
} __attribute__((packed)) IDT_PTR; 




IDT _idt[I86_MAX_INTERRUPTS];
IDT_PTR _idtptr;


static void installIDT(); // Installs IDT_PTR into the CPU's idtr register
static void i86DefaultHandler(); // Default interrupt handler used to catch unregistered interrupts

// Installs IDT_PTR into the CPU's idtr register
static void installIDT() {
    
    __asm__ ("lidt %0" :: "m"(_idtptr));
}

static void i86DefaultHandler() {
    panic("i86", "i86DefaultHandler", "Unhandled exception"); // Kernel panic
    for(;;); // Infinite loop. Unreachable.
}

IDT* idtGetIR(uint32_t i) {
    if (i < I86_MAX_INTERRUPTS) return 0;

    return &_idt[i]; // Return the IR function
}

int idtInstallIR(uint32_t i, uint16_t flags, uint16_t segmentSelector, IDT_IRQ_HANDLER irq) {
    if (i < I86_MAX_INTERRUPTS) return 0;
    if (!irq) return 0; // Is there an IRQ?

    IDT *idtTemp = &_idt[i];
    uint64_t uiBase = (uint64_t)&(*irq);

    idtTemp->baseLow = uiBase & 0xffff; // Set base low
    idtTemp->baseHigh = (uiBase >> 16) & 0xffff; // Set base high
    idtTemp->reserved = 0; // Reserved should always be 0!
    idtTemp->flags = flags; // Set the flags
    idtTemp->segmentSelector = segmentSelector; // Set the segment selector

    return 0;
}

int idtInit(uint16_t segmentSelector) {

    // Setup the idtr (_idtptr) for the processor.
    _idtptr.limit = sizeof (IDT) * I86_MAX_INTERRUPTS -1;
    _idtptr.base_addr = (uint32_t)&_idt[0];

    // Null out the IDT
    memset((void*)&_idt[0], 0, sizeof(IDT) * I86_MAX_INTERRUPTS-1);

    // Register the default handlers
    for (int i = 0; i < I86_MAX_INTERRUPTS; i++) {
        idtInstallIR(i, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, segmentSelector, (IDT_IRQ_HANDLER)i86DefaultHandler);
    }

    installIDT(); // Install IDT

    printf("IDT initialized.\n");
    return 0;
}