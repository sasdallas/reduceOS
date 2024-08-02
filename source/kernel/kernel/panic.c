// =====================================================================
// panic.c - Kernel panic
// This file handles the kernel panicking and exiting
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.


#include "include/panic.h" // Main panic include file

typedef struct {
    struct stack_frame* ebp;
    uint32_t eip;
} stack_frame;

// This static function performes a stack trace to get the callers of panic.
static void stackTrace(uint32_t maximumFrames) {
    stack_frame *stk;
    asm volatile ("movl %%ebp, %0" :: "r"(stk));
    printf("\nStack trace:\n");
    serialPrintf("\nSTACK TRACE (EBP based):\n");
    for (uint32_t frame = 0; stk && frame < maximumFrames; frame++) {
        printf("0x%x\n", stk->eip);
        stk = stk->ebp;
    }
}

// Panic - halts system and prints an error message
void *panic(char *caller, char *code, char *reason) {
    serialPrintf("===========================================================\n");
    serialPrintf("panic() called! FATAL ERROR!\n");
    serialPrintf("*** [%s] %s: %s\n", caller, code, reason);
    serialPrintf("panic type: non-registers, called by external function.\n");



    clearScreen(COLOR_WHITE, COLOR_RED);
    updateTerminalColor_gfx(COLOR_BLACK, COLOR_LIGHT_GRAY); // Update terminal color

    printf("reduceOS v%s %s - Kernel Panic", VERSION, CODENAME);
    if (terminalMode == 0) {
        for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v1.0  - Kernel Panic") - strlen(CODENAME)); i++) printf(" "); // vga only because idc enough to do PSF_WIDTH
    } else {
        printf("                                                                                          \n"); // cry about it neat freaks
    }

    // updateBottomText("A fatal error occurred!");

    updateTerminalColor_gfx(COLOR_WHITE, COLOR_RED);

    printf("reduceOS encountered a fatal error and needs to shutdown.\n");
    printf("The error cause will be printed below. If you start an issue on GitHub, please include the following text.\n");
    printf("Apologies for any inconveniences caused by this error.\n");
    printf("\n");
    printf("The error encountered was:\n");
    printf("*** [%s] %s: %s \n", caller, code, reason);
    printf("\nStack dump:\n\n");
    
    
    
    
    // TODO: Add debug symbols into the initrd so we get function names.
    // stackTrace(5); // Get a stack trace.



    asm volatile ("hlt");
    for (;;);
}


void *panicReg(char *caller, char *code, char *reason, registers_t *reg) {
    serialPrintf("===========================================================\n");
    serialPrintf("panic() called! FATAL ERROR!\n");
    serialPrintf("*** ISR threw exception: %s\n", reason);
    serialPrintf("panic type: registers, %s.\n", code);

    clearScreen(COLOR_WHITE, COLOR_RED);
    updateTerminalColor_gfx(COLOR_BLACK, COLOR_LIGHT_GRAY); // Update terminal color

    printf("reduceOS v%s %s - Kernel Panic", VERSION, CODENAME);
    if (terminalMode == 0) {
        for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v1.0  - Kernel Panic") - strlen(CODENAME)); i++) printf(" "); // vga only because idc enough to do PSF_WIDTH
    } else {
        printf("                                                                                          \n"); // cry about it neat freaks
    }

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

    //stackTrace(5);

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
    // Get the flags
    int present = !(reg->err_code & 0x1); // Page not present?
    int rw = reg->err_code & 0x2; // Was this a write operation?
    int user = reg->err_code & 0x4; // Were we in user-mode?
    int reserved = reg->err_code & 0x8; // Were the reserved bits of page entry overwritten?

    clearScreen(COLOR_WHITE, COLOR_RED);
    updateTerminalColor_gfx(COLOR_BLACK, COLOR_LIGHT_GRAY); // Update terminal color

    printf("reduceOS v%s %s - Kernel Panic", VERSION, CODENAME);
    if (terminalMode == 0) {
        for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v1.0  - Kernel Panic") - strlen(CODENAME)); i++) printf(" "); // vga only because idc enough to do PSF_WIDTH
    } else {
        printf("                                                                                          \n"); // cry about it neat freaks
    }

    // updateBottomText("A fatal error occurred!");

    updateTerminalColor_gfx(COLOR_WHITE, COLOR_RED);

    printf("reduceOS encountered a fatal error and needs to shutdown.\n");
    printf("The error cause will be printed below. If you start an issue on GitHub, please include the following text.\n");
    printf("Apologies for any inconveniences caused by this error.\n");
    printf("\n");
    printf("The error encountered was:\n");
    printf("*** Page fault at address 0x%x\n", faultAddress);
    printf("*** Flags: %s %s %s %s\n", (present) ? "present" : "\0", (rw) ? "read-only" : "\0", (user) ? "user-mode" : "\0", (reserved) ? "reserved" : "\0");
    printf("\nStack dump:\n\n");
    
    printf("Error Code: %d\n", reg->err_code);
    printf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", reg->eax, reg->ebx, reg->ecx, reg->edx);
    printf("edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x\n", reg->edi, reg->esi, reg->ebp, reg->esp);
    printf("eip=0x%x, cs=0x%x, ss=0x%x, eflags=0x%x, useresp=0x%x\n", reg->eip, reg->ss, reg->eflags, reg->useresp);

    serialPrintf("*** Flags: %s%s%s%s\n", (present) ? "present " : "\0", (rw) ? "read-only " : "\0", (user) ? "user-mode " : "\0", (reserved) ? "reserved " : "\0");


    asm volatile ("hlt");
    for (;;);
}