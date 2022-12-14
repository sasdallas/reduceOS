// =====================================================================
// panic.c - Kernel panic
// This file handles the kernel panicking and exiting
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.


#include "include/panic.h" // Main panic include file

void *panic(char *caller, char *code, char *reason) {
    clearScreen(terminalColor);
    updateTerminalColor(vgaColorEntry(COLOR_BLACK, COLOR_LIGHT_GRAY)); // Update terminal color

    printf("reduceOS v1.0 (Development Build) - Kernel Panic");
    for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v1.0 (Development Build) - Kernel Panic")); i++) printf(" ");

    updateBottomText("A fatal error occurred!");

    printf("reduceOS encountered a fatal error and needs to shutdown.\n");
    printf("The error cause will be printed below. If you start an issue on GitHub, please include the following text.\n");
    printf("Apologies for any inconveniences caused by this error.\n");
    printf("\n");
    printf("The error encountered was:\n");
    printf("*** [%s] %s: %s \n", caller, code, reason);
    printf("\nStack dump:\n\n");
    
    uint32_t eax, ebx, ecx, edx;
    for (uint32_t i = 0; i < 4; i++) {
        __cpuid(i, &eax, &ebx, &ecx, &edx);
        printf("Type: 0x%x, EAX: 0x%x, EBX: 0x%x, ECX: 0x%x, EDX: 0x%x\n", i, eax, ebx, ecx, edx);
    }
    
    asm volatile ("hlt");
    for (;;);
}


void *panicReg(char *caller, char *code, char *reason, REGISTERS *reg) {
    clearScreen(terminalColor);
    updateTerminalColor(vgaColorEntry(COLOR_BLACK, COLOR_LIGHT_GRAY)); // Update terminal color

    printf("reduceOS v1.0 (Development Build) - Kernel Panic");
    for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v1.0 (Development Build) - Kernel Panic")); i++) printf(" ");

    updateBottomText("A fatal error occurred!");

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



    asm volatile ("hlt");
    for (;;);
}