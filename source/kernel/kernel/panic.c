// =====================================================================
// panic.c - Kernel panic
// This file handles the kernel panicking and exiting
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.


#include <kernel/panic.h> // Main panic include file

typedef struct {
    struct stack_frame* ebp;
    uint32_t eip;
} stack_frame;

// This static function performes a stack trace to get the callers of panic.
static void stackTrace(uint32_t maximumFrames) {
    stack_frame *stk;
    asm volatile ("movl %%ebp, %0" : "=r"(stk));
    printf("\nStack trace:\n");
    serialPrintf("\nSTACK TRACE (EBP based):\n");
    for (uint32_t frame = 0; stk && frame < maximumFrames; frame++) {
        printf("Frame %i: 0x%x\n", frame, stk->eip);
        serialPrintf("FRAME %i: 0x%x\n", frame, stk->eip);
        stk = stk->ebp;
    }
}

// Panic - halts system and prints an error message
void *panic(char *caller, char *code, char *reason) {
    serialPrintf("===========================================================\n");
    serialPrintf("panic() called! FATAL ERROR!\n");
    serialPrintf("*** [%s] %s: %s\n", caller, code, reason);
    serialPrintf("panic type: kernel panic\n");


    setKBHandler(false);
    clearScreen(COLOR_WHITE, COLOR_RED);
    updateTerminalColor_gfx(COLOR_BLACK, COLOR_LIGHT_GRAY); // Update terminal color

    // Make things look nicer
    terminalSetUpdateScreen(false);

    printf("reduceOS v%s %s - Kernel Panic", VERSION, CODENAME);
    if (terminalMode == 0) {
        for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v  - Kernel Panic") - strlen(CODENAME) - strlen(VERSION)); i++) printf(" "); // vga only because idc enough to do PSF_WIDTH
    } else {
        for (int i = 0; i < ((modeWidth - ((strlen("reduceOS v  - Kernel Panic") + strlen(CODENAME) + strlen(VERSION)) * psfGetFontWidth()))); i += psfGetFontWidth()) printf(" ");
    }
    
    terminalSetUpdateScreen(true);
    terminalUpdateScreen();

    // updateBottomText("A fatal error occurred!");

    updateTerminalColor_gfx(COLOR_WHITE, COLOR_RED);

    printf("reduceOS encountered a fatal error and needs to shutdown.\n");
    printf("The error cause will be printed below. If you start an issue on GitHub, please include the following text.\n");
    printf("Apologies for any inconveniences caused by this error.\n");
    printf("\n");
    printf("The error encountered was:\n");
    printf("*** [%s] %s: %s \n", caller, code, reason);
    
    
    registers_t *reg;
    
    // Funny
    asm volatile ("mov %%eax, %0" : "=r"(reg->eax));
    asm volatile ("mov %%ebx, %0" : "=r"(reg->ebx));
    asm volatile ("mov %%ecx, %0" : "=r"(reg->ecx));
    asm volatile ("mov %%edx, %0" : "=r"(reg->edx));
    asm volatile ("mov %%esi, %0" : "=r"(reg->esi));
    asm volatile ("mov %%esp, %0" : "=r"(reg->esp));
    asm volatile ("mov %%ebp, %0" : "=r"(reg->ebp));
    asm volatile ("mov %%edi, %0" : "=r"(reg->edi));
    asm volatile ("mov %%ds, %0" : "=r"(reg->ds));
    asm volatile ("mov %%cs, %0" : "=r"(reg->cs));
    asm volatile ("mov %%ss, %0" : "=r"(reg->ss));

    printf("\nStack dump:\n\n");
    printf("No error code was set (kernel panic).\n");
    printf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", reg->eax, reg->ebx, reg->ecx, reg->edx);
    printf("edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x\n", reg->edi, reg->esi, reg->ebp, reg->esp);
    printf("cs=0x%x, ss=0x%x\n", reg->cs, reg->ss);

    serialPrintf("STACK DUMP:\n");
    serialPrintf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", reg->eax, reg->ebx, reg->ecx, reg->edx);
    serialPrintf("edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x\n", reg->edi, reg->esi, reg->ebp, reg->esp);
    serialPrintf("cs=0x%x, ss=0x%x\n", reg->cs, reg->ss);


    // TODO: Add debug symbols into the initrd so we get function names.
    stackTrace(5); // Get a stack trace.

    asm volatile ("hlt");
    for (;;);
}


void *panicReg(char *caller, char *code, char *reason, registers_t *reg) {
    serialPrintf("===========================================================\n");
    serialPrintf("panic() called! FATAL ERROR!\n");
    serialPrintf("*** ISR threw exception: %s\n", reason);
    serialPrintf("panic type: %s.\n", code);


    setKBHandler(false);
    clearScreen(COLOR_WHITE, COLOR_RED);
    updateTerminalColor_gfx(COLOR_BLACK, COLOR_LIGHT_GRAY); // Update terminal color

    // Make things look nicer
    terminalSetUpdateScreen(false);

    printf("reduceOS v%s %s - Kernel Panic", VERSION, CODENAME);
    if (terminalMode == 0) {
        for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v  - Kernel Panic") - strlen(CODENAME) - strlen(VERSION)); i++) printf(" "); // vga only because idc enough to do PSF_WIDTH
    } else {
        for (int i = 0; i < ((modeWidth - ((strlen("reduceOS v  - Kernel Panic") + strlen(CODENAME) + strlen(VERSION)) * psfGetFontWidth()))); i += psfGetFontWidth()) printf(" ");
    }
    
    terminalSetUpdateScreen(true);
    terminalUpdateScreen();

    // updateBottomText("A fatal error occurred!");

    updateTerminalColor_gfx(COLOR_WHITE, COLOR_RED);

    printf("reduceOS encountered a fatal error and needs to shutdown.\n");
    printf("The error cause will be printed below. If you start an issue on GitHub, please include the following text.\n");
    printf("Apologies for any inconveniences caused by this error.\n");
    printf("\n");
    printf("The error encountered was:\n");
    printf("*** [%s] %s: %s \n", caller, code, reason);
    printf("\nStack dump:\n\n");
    
    printf("Error Code: %d\n", reg->err_code);
    printf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", reg->eax, reg->ebx, reg->ecx, reg->edx);
    printf("edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x\n", reg->edi, reg->esi, reg->ebp, reg->esp);
    printf("eip=0x%x, cs=0x%x, ss=0x%x, eflags=0x%x, useresp=0x%x\n", reg->eip, reg->ss, reg->eflags, reg->useresp);

    serialPrintf("\nerr_code %d\n", reg->err_code);
    serialPrintf("REGISTER DUMP:\n");
    serialPrintf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", reg->eax, reg->ebx, reg->ecx, reg->edx);
    serialPrintf("edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x\n", reg->edi, reg->esi, reg->ebp, reg->esp);
    serialPrintf("eip=0x%x, cs=0x%x, ss=0x%x, eflags=0x%x, useresp=0x%x\n", reg->eip, reg->ss, reg->eflags, reg->useresp);

    // Perform stack trace
    stackTrace(5);

    asm volatile ("hlt");
    for (;;);
}



// Oh no! We encountered a page fault!
void *pageFault(registers_t *reg) {
    // Get the faulting address from CR2
    uint32_t faultAddress;
    asm volatile("mov %%cr2, %0" : "=r" (faultAddress));

    serialPrintf("===========================================================\n");
    serialPrintf("panic() called! FATAL ERROR!\n");
    serialPrintf("*** Page fault at address 0x%x\n", faultAddress);
    
    setKBHandler(false);

    // Get the flags
    int present = !(reg->err_code & 0x1); // Page not present?
    int rw = reg->err_code & 0x2; // Was this a write operation?
    int user = reg->err_code & 0x4; // Were we in user-mode?
    int reserved = reg->err_code & 0x8; // Were the reserved bits of page entry overwritten?

    clearScreen(COLOR_WHITE, COLOR_RED);
    updateTerminalColor_gfx(COLOR_BLACK, COLOR_LIGHT_GRAY); // Update terminal color

    // Make things look nicer
    terminalSetUpdateScreen(false);

    printf("reduceOS v%s %s - Kernel Panic", VERSION, CODENAME);
    if (terminalMode == 0) {
        for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v  - Kernel Panic") - strlen(CODENAME) - strlen(VERSION)); i++) printf(" "); // vga only because idc enough to do PSF_WIDTH
    } else {
        for (int i = 0; i < ((modeWidth - ((strlen("reduceOS v  - Kernel Panic") + strlen(CODENAME) + strlen(VERSION)) * psfGetFontWidth()))); i += psfGetFontWidth()) printf(" ");
    }
    
    terminalSetUpdateScreen(true);
    terminalUpdateScreen();

    updateTerminalColor_gfx(COLOR_WHITE, COLOR_RED);

    printf("reduceOS encountered a fatal error and needs to shutdown.\n");
    printf("The error cause will be printed below. If you start an issue on GitHub, please include the following text.\n");
    printf("Apologies for any inconveniences caused by this error.\n");
    printf("\n");
    printf("The error encountered was:\n");
    printf("*** Page fault at address 0x%x\n", faultAddress);
    printf("*** Flags: %s%s%s%s\n", (present) ? "present " : "\0", (rw) ? "read-only " : "\0", (user) ? "user-mode " : "\0 ", (reserved) ? "reserved " : "\0");
    printf("\nStack dump:\n\n");
    
    printf("Error Code: %d\n", reg->err_code);
    printf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", reg->eax, reg->ebx, reg->ecx, reg->edx);
    printf("edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x\n", reg->edi, reg->esi, reg->ebp, reg->esp);
    printf("eip=0x%x, cs=0x%x, ss=0x%x, eflags=0x%x, useresp=0x%x\n", reg->eip, reg->ss, reg->eflags, reg->useresp);

    serialPrintf("*** Flags: %s%s%s%s\n", (present) ? "present " : "\0", (rw) ? "read-only " : "\0", (user) ? "user-mode " : "\0", (reserved) ? "reserved " : "\0");
    
    
    serialPrintf("\nerr_code %d\n", reg->err_code);
    serialPrintf("REGISTER DUMP:\n");
    serialPrintf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", reg->eax, reg->ebx, reg->ecx, reg->edx);
    serialPrintf("edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x\n", reg->edi, reg->esi, reg->ebp, reg->esp);
    serialPrintf("eip=0x%x, cs=0x%x, ss=0x%x, eflags=0x%x, useresp=0x%x\n", reg->eip, reg->ss, reg->eflags, reg->useresp);


    stackTrace(5);


    asm volatile ("hlt");
    for (;;);
}
