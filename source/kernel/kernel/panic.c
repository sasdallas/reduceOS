// =====================================================================
// panic.c - Kernel panic
// This file handles the kernel panicking and exiting
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.


#include <kernel/panic.h> // Main panic include file
#include <kernel/ksym.h>
#include <time.h>

extern uint32_t text_start;
extern uint32_t bss_end;
extern multiboot_info *globalInfo;
extern char ch;


// This static function will dump physical memory to disk
static void dumpPMM() {
    // Check if we have a way to dump memory to drive.
    if (strcmp(fs_root->name, "initrd") && fs_root->create != NULL) {
        // We are not mounted to initial ramdisk, try to dump.
        printf("\nA filesystem driver of name '%s' is mounted to root.\n", fs_root->name);
        
        // Check if it's our stable EXT2 driver. If not, prompt the user
        // (shouldn't be possible)
        if (strcmp(fs_root->name, "EXT2 driver")) {
            printf("WARNING: A non-ext2 driver was mounted to root.\nThis should not be possible but it is likely the code was not updated.\n");
            printf("Physical memory dumping disabled.\n"); // amazing solution
            return;
        }

        printf("Creating dump file...");

        // Create the dump file using timestamps.
        char buffer[1024];
        time_t rawtime;
        struct tm *t;

        time(&rawtime);
        t = localtime(&rawtime);

        sprintf(buffer, "crashdump-%i.log", rawtime);

        fs_root->create(fs_root, buffer, 0);
        fsNode_t *dumpfile = fs_root->finddir(fs_root, buffer);
        
        // Let's dump memory to it
        printf("DONE\n");

        int memory = pmm_getPhysicalMemorySize() * 1024;
        int pages = memory / PAGE_SIZE;
        if (pages > 200) pages = 200;   // Limit else EXT2 driver will crash due to OOM, which is touchy in this environment 
        int pages_per_loop = pages / 10;

        printf("Physical memory dump started (%i pages total, . = %i pages).\n", pages, pages_per_loop);
        printf("Cancellation is not possible.\n"); // yeahhh keyboard driver sucks

        uint8_t pbuffer[PAGE_SIZE];
        uint32_t addr = &text_start; // Better dumps
        for (int i = 0; i < pages; i += pages_per_loop) {
            
            pagedirectory_t *pageDirectory = vmm_getCurrentDirectory();
            pde_t *entry = &pageDirectory->entries[PAGEDIR_INDEX((uint32_t)addr)];
            if (!entry) {
                printf("Error when trying to dump page %i (get page table error)\n", i);
                break;
            }

            if ((*entry & PTE_PRESENT) != PTE_PRESENT) {
                // Skip me
                serialPrintf("dumpPMM: Skipping page %i\n", i);
                continue;
            }
            
            // Let's dump this entry. We'll have to iterate through all pages per dot
            for (int j = 0; j < pages_per_loop; j++) {
                memset(pbuffer, 0, PAGE_SIZE);
                memcpy(pbuffer, addr, PAGE_SIZE);
                dumpfile->write(dumpfile, (i+j)*PAGE_SIZE, PAGE_SIZE, pbuffer);
                addr += PAGE_SIZE;
            }

            printf(".");
        }

        dumpfile->close(dumpfile);
        printf("\nDump finished.\n");

    } else {
        printf("\nCannot dump physical memory to initial ramdisk.\n");
    }

}

// This static function performes a stack trace to get the callers of panic.
static void stackTrace(uint32_t maximumFrames) {
    stack_frame *stk;
    asm volatile ("movl %%ebp, %0" : "=r"(stk));
    
    printf("\nStack trace:\n");
    serialPrintf("\nSTACK TRACE (EBP based):\n");
    
    for (uint32_t frame = 0; stk && frame < maximumFrames; frame++) {
        // Make sure the code is in kernelspace, else the symbol finder might crash
        if (stk->eip < &text_start || stk->eip > &bss_end) {
            printf("Frame %i: IP 0x%x (outside of kspace)\n", frame, stk->eip);
            serialPrintf("FRAME %i: IP 0x%x (outside of kspace)\n", frame, stk->eip);
            stk = stk->eip;
            continue;
        }

        // Try to get the symbol
        ksym_symbol_t *sym = kmalloc(sizeof(ksym_symbol_t));
        int err = ksym_find_best_symbol(stk->eip, sym);
        if (err == -1) {
            printf("Frame %i: 0x%x (exception occurred before ksym_init)\n", frame, stk->eip);
            serialPrintf("FRAME %i: IP 0x%x (exception before alloc init/ksym_init)\n", frame, stk->eip);
        } else if (err == -2) {
            printf("Frame %i: 0x%x (no debug symbols loaded)\n", frame, stk->eip);
            serialPrintf("FRAME %i: IP 0x%x (no debug symbols loaded)\n", frame, stk->eip);
        } else if (err == 0) {
            printf("Frame %i: 0x%x (%s+0x%x)\n", frame, stk->eip, sym->symname, stk->eip - (long)sym->address);
            serialPrintf("FRAME %i: IP 0x%x (%s+0x%x - base func addr 0x%x)\n", frame, stk->eip, sym->symname, stk->eip - (long)sym->address, (long)sym->address);
        } else {
            printf("Frame %i: 0x%x (unknown error when getting symbols)\n", frame, stk->eip);
            serialPrintf("FRAME %i: IP 0x%x (err = %i, unknown)\n", frame, stk->eip, err);
        }

        kfree(sym);

        stk = stk->ebp;
    }
}


// VGA Text Mode Panic - Halts system and prints out less helpful info
void vgaTextMode_Panic(char *caller, char *code, char *reason, registers_t *reg, uint32_t faultAddress) {
    serialPrintf("\nWARNING: Exception occurred within VGA text mode, before VBE initialization.\n");
    serialPrintf("As such, debug info will only be printed to console.\n");

    printf("reduceOS v%s %s - Kernel Panic", VERSION, CODENAME);
    for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v  - Kernel Panic") - strlen(CODENAME) - strlen(VERSION)); i++) printf(" ");
    
    updateTerminalColor_gfx(COLOR_WHITE, COLOR_RED); // When we're called this hasn't been done

    
    printf("reduceOS encountered a fatal error and needs to shutdown.\n");
    printf("The error cause will be printed below. If you start an issue on GitHub, please include the following text.\n");
    printf("Apologies for any inconveniences caused by this error.\n");
    printf("\n");
    printf("The error encountered was:\n");
    if (caller) printf("*** [%s] %s: %s \n", caller, code, reason);
    else {
        // Assume page fault, probably not the best idea.
        // Get the flags
        int present = !(reg->err_code & 0x1); // Page not present?
        int rw = reg->err_code & 0x2; // Was this a write operation?
        int user = reg->err_code & 0x4; // Were we in user-mode?
        int reserved = reg->err_code & 0x8; // Were the reserved bits of page entry overwritten?

        printf("*** Page fault at address 0x%x\n", faultAddress);
        printf("*** Flags: %s%s%s%s\n", (present) ? "present " : "\0", (rw) ? "read-only " : "\0", (user) ? "user-mode " : "\0 ", (reserved) ? "reserved " : "\0");
    }

    printf("\nThis error occurred before the graphics driver could initialize.\nAs such, all debug info will be printed to serial console.\n");

    if (reg) {
        serialPrintf("\nerr_code %d\n", reg->err_code);
        serialPrintf("REGISTER DUMP:\n");
        serialPrintf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", reg->eax, reg->ebx, reg->ecx, reg->edx);
        serialPrintf("edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x\n", reg->edi, reg->esi, reg->ebp, reg->esp);
        serialPrintf("eip=0x%x, cs=0x%x, ss=0x%x, eflags=0x%x, useresp=0x%x\n", reg->eip, reg->ss, reg->eflags, reg->useresp);
    }

    printf("The system has been halted. Attach debugger now to view context.\n");
    asm volatile ("hlt");
    for (;;);
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

    // Make sure we're not in VGA text mode
    if (terminalMode == 0) {
        vgaTextMode_Panic(caller, code, reason, NULL, NULL);
    }

    // Make things look nicer
    terminalSetUpdateScreen(false);

    printf("reduceOS v%s %s - Kernel Panic", VERSION, CODENAME);
    
    for (int i = 0; i < ((modeWidth - ((strlen("reduceOS v  - Kernel Panic") + strlen(CODENAME) + strlen(VERSION)) * psfGetFontWidth()))); i += psfGetFontWidth()) printf(" ");
    
    
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


    stackTrace(7); // Get a stack trace.

    // Dump memory
    serialPrintf("\n==== DEBUG INFORMATION FROM PMM DUMP ====\n");
    dumpPMM();
    serialPrintf("\n==== END DEBUG INFORMATION OF PMM DUMP ====\n");


    printf("\nThe system has been halted. Attach debugger now to view context.\n");

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
    stackTrace(7);


    // Dump memory
    serialPrintf("\n==== DEBUG INFORMATION FROM PMM DUMP ====\n");
    dumpPMM();
    serialPrintf("\n==== END DEBUG INFORMATION OF PMM DUMP ====\n");

    printf("\nThe system has been halted. Attach debugger now to view context.\n");

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

    // Make sure we're not in VGA text mode
    if (terminalMode == 0) {
        vgaTextMode_Panic(NULL, NULL, NULL, reg, faultAddress);
    }


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
    printf("eip=0x%x, cs=0x%x, ss=0x%x, eflags=0x%x, useresp=0x%x\n", reg->eip, reg->cs, reg->ss, reg->eflags, reg->useresp);

    serialPrintf("*** Flags: %s%s%s%s\n", (present) ? "present " : "\0", (rw) ? "read-only " : "\0", (user) ? "user-mode " : "\0", (reserved) ? "reserved " : "\0");
    
    
    serialPrintf("\nerr_code %d\n", reg->err_code);
    serialPrintf("REGISTER DUMP:\n");
    serialPrintf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", reg->eax, reg->ebx, reg->ecx, reg->edx);
    serialPrintf("edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x\n", reg->edi, reg->esi, reg->ebp, reg->esp);
    serialPrintf("eip=0x%x, cs=0x%x, ss=0x%x, eflags=0x%x, useresp=0x%x\n", reg->eip, reg->ss, reg->eflags, reg->useresp);


    // DUMP PMM FIRST
    // This is because page faults will sometimes hang stackTrace
    serialPrintf("\n==== DEBUG INFORMATION FROM PMM DUMP ====\n");
    dumpPMM();
    serialPrintf("\n==== END DEBUG INFORMATION OF PMM DUMP ====\n");

    // Perform stack trace (note: this might hang) 
    stackTrace(7);

    printf("\nThe system has been halted. Attach debugger now to view context.\n");

    
    asm volatile ("hlt");
    for (;;);
}
