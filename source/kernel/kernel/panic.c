// =====================================================================
// panic.c - Kernel panic
// This file handles the kernel panicking and exiting
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.


/*
    And the prize for number one most used file in reduceOS goes to...
*/


#include <kernel/panic.h> // Main panic include file
#include <kernel/ksym.h>
#include <kernel/keyboard.h>
#include <kernel/terminal.h>
#include <kernel/process.h>
#include <libk_reduced/stdio.h>

#include <libk_reduced/time.h>

extern uint32_t text_start;
extern uint32_t bss_end;
extern multiboot_info *globalInfo;
extern char ch;


// This function will dump physical memory to disk
void panic_dumpPMM() {
    // Do not dump PMM if we're using a RELEASE build.
    if (!strcmp(BUILD_CONFIGURATION, "RELEASE")) {
        printf("\nThis is a RELEASE build of reduceOS - therefore, to retain the security of your disks, memory dumps have been disabled.\n");
        return;
    }


    // BUG: We can't dump memory unless the system has around 512MB of RAM, else EXT2 driver hits an OOM.
    // I'm not entirely sure whether this is because the system is scuffed and not designed for this, or what.
    // But, if I'm being honest, I don't care.
    // This whole dumpPMM function is pretty much COMPLETELY scuffed because if the EXT2 driver crashes (due to an OOM or even a divbyzero bye bye data)
    // Therefore, RELEASE configurations of reduceOS have this turned off.
    if (pmm_getPhysicalMemorySize() < 510000) {
        printf("\nCannot dump physical memory: Your system does not have enough RAM to write the data correctly.\n");
        printf("If this is an emulator such as QEMU, you can dump the contents of RAM using the inbuilt debugger.\n");
        return;
    }

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

        time(&rawtime);

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
        uint32_t addr = (uint32_t)&text_start; // Better dumps
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
                memcpy(pbuffer, (void*)addr, PAGE_SIZE);
                dumpfile->write(dumpfile, (i+j)*PAGE_SIZE, PAGE_SIZE, pbuffer);
                addr += PAGE_SIZE;
            }

            serialPrintf("dumpPMM: %i%% completed.\n", (pages / i));

            printf(".");
        }

        dumpfile->close(dumpfile);
        printf("\nDump finished.\n");

    } else {
        printf("\nCannot dump physical memory to initial ramdisk.\n");
    }

}

// This function performes a stack trace to get the callers of panic.
#pragma GCC diagnostic ignored "-Wuninitialized" // Stack frame is technically uninitialized as NULL but stk itself is never used, only EBP
void panic_stackTrace(uint32_t maximumFrames, registers_t *reg) {
    stack_frame *stk;
    stk->ebp = (stack_frame*)reg->ebp;
    stk->eip = reg->eip;
    
    printf("\nStack trace:\n");
    serialPrintf("\nSTACK TRACE (EBP based):\n");
    
    for (uint32_t frame = 0; stk && frame < maximumFrames; frame++) {

        if (!stk->eip) {
            printf("Frame %i: EIP unknown\n", frame);
            continue;
        }

        // Make sure the code is in kernelspace, else the symbol finder might crash
        if (stk->eip && (stk->eip < (uint32_t)&text_start || stk->eip > (uint32_t)&bss_end)) {
            printf("Frame %i: 0x%x (outside of kspace)\n", frame, stk->eip);
            serialPrintf("FRAME %i: IP 0x%x (outside of kspace)\n", frame, stk->eip);
            stk = stk->ebp;
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
    for (uint32_t i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v  - Kernel Panic") - strlen(CODENAME) - strlen(VERSION)); i++) printf(" ");
    
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


void panic_dumpStack(registers_t *r) {

    if (r != NULL) {
        registers_t *reg = r; // Dirty hack to make my life easier
        printf("Error Code: %d\n", reg->err_code);
        printf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", reg->eax, reg->ebx, reg->ecx, reg->edx);
        printf("edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x\n", reg->edi, reg->esi, reg->ebp, reg->esp);
        printf("eip=0x%x, cs=0x%x, ss=0x%x, eflags=0x%x, useresp=0x%x\n", reg->eip, reg->ss, reg->eflags, reg->useresp);

        serialPrintf("\nerr_code %d\n", reg->err_code);
        serialPrintf("REGISTER DUMP:\n");
        serialPrintf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", reg->eax, reg->ebx, reg->ecx, reg->edx);
        serialPrintf("edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x\n", reg->edi, reg->esi, reg->ebp, reg->esp);
        serialPrintf("eip=0x%x, cs=0x%x, ss=0x%x, eflags=0x%x, useresp=0x%x\n", reg->eip, reg->ss, reg->eflags, reg->useresp);
        return;
    }

    registers_t *reg = (registers_t*)((uint8_t*)&end); // this is bad, need to use stack
    memset(reg, 0, sizeof(registers_t));

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

}

// Panic Prepare - prepares the system for a kernel panic, useful for applications that need control over panic's output
void panic_prepare() {
    serialPrintf("===========================================================\n");
    serialPrintf("A fatal error in reduceOS has occurred.\n");

    setKBHandler(false);
    clearScreen(COLOR_WHITE, COLOR_RED);


    terminalUpdateTopBarKernel("Kernel Panic");

    updateTerminalColor_gfx(COLOR_WHITE, COLOR_RED);

    printf("reduceOS encountered a fatal error and needs to shutdown.\n");
    printf("The error cause will be printed below. If you start an issue on GitHub, please include the following text.\n");
    printf("Apologies for any inconveniences caused by this error.\n");
    printf("\n");
}  

// Panic - halts system and prints an error message
void panic(char *caller, char *code, char *reason) {
    serialPrintf("===========================================================\n");
    serialPrintf("panic() called! FATAL ERROR!\n");
    serialPrintf("*** [%s] %s: %s\n", caller, code, reason);
    serialPrintf("panic type: kernel panic\n");


    setKBHandler(false);
    clearScreen(COLOR_WHITE, COLOR_RED);

    // Make sure we're not in VGA text mode
    if (terminalMode == 0) {
        vgaTextMode_Panic(caller, code, reason, NULL, NULL);
    }

    terminalUpdateTopBarKernel("Kernel Panic");

    // updateBottomText("A fatal error occurred!");

    updateTerminalColor_gfx(COLOR_WHITE, COLOR_RED);

    printf("reduceOS encountered a fatal error and needs to shutdown.\n");
    printf("The error cause will be printed below. If you start an issue on GitHub, please include the following text.\n");
    printf("Apologies for any inconveniences caused by this error.\n");
    printf("\n");
    printf("The error encountered was:\n");
    printf("*** [%s] %s: %s \n", caller, code, reason);
    
    


    // Dump the stack and perform a stack trace
    panic_dumpStack(NULL);

    // Perform a stack trace
    registers_t *reg = (registers_t*)((uint8_t*)&end); // stealing from the stack dumper LMAO
    asm volatile ("mov %%ebp, %0" :: "r"(reg->ebp));
    reg->eip = read_eip();
    panic_stackTrace(7, reg);

    // Dump memory
    serialPrintf("\n==== DEBUG INFORMATION FROM PMM DUMP ====\n");
    panic_dumpPMM();
    serialPrintf("\n==== END DEBUG INFORMATION OF PMM DUMP ====\n");


    printf("\nThe system has been halted. Attach debugger now to view context.\n");

    asm volatile ("hlt");
    for (;;);
}


void panicReg(char *caller, char *code, char *reason, registers_t *reg) {
    serialPrintf("===========================================================\n");
    serialPrintf("panic() called! FATAL ERROR!\n");
    serialPrintf("*** ISR threw exception: %s\n", reason);
    serialPrintf("panic type: %s.\n", code);


    setKBHandler(false);
    clearScreen(COLOR_WHITE, COLOR_RED);
    
    terminalUpdateTopBarKernel("Kernel Panic");
    // updateBottomText("A fatal error occurred!"); - Function unused

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
    panic_stackTrace(7, reg);


    // Dump memory
    serialPrintf("\n==== DEBUG INFORMATION FROM PMM DUMP ====\n");
    panic_dumpPMM();
    serialPrintf("\n==== END DEBUG INFORMATION OF PMM DUMP ====\n");

    printf("\nThe system has been halted. Attach debugger now to view context.\n");

    asm volatile ("hlt");
    for (;;);
}



// Oh no! We encountered a page fault!
void pageFault(registers_t *reg) {
    // Get the faulting address from CR2
    uint32_t faultAddress;
    asm volatile("mov %%cr2, %0" : "=r" (faultAddress));

    // See syscall.c for an explanation on how this works
    // We'll map more stack and quietly return
    if (reg->cs != 0x08 && (currentProcess->image.heap) && (faultAddress >= currentProcess->image.heap_start) && (faultAddress < currentProcess->image.heap_end)) {
        volatile process_t *volatile proc = currentProcess;

        if (proc->group != 0) {
            proc = process_from_pid(proc->group);
        }

        if (!proc) goto continue_fault;

        spinlock_lock(&proc->image.spinlock);
        for (uintptr_t i = faultAddress; i < proc->image.userstack; i += 0x1000) {
            vmm_mapPhysicalAddress(proc->thread.page_directory, faultAddress, i, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
        }

        proc->image.userstack = faultAddress;

        spinlock_release(&proc->image.spinlock);

        // Literally just say screw the kernel and go back
        return;
    }

continue_fault:
    

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


    // Make sure we're not in VGA text mode
    if (terminalMode == 0) {
        vgaTextMode_Panic(NULL, NULL, NULL, reg, faultAddress);
    }

    terminalUpdateTopBarKernel("Kernel Panic");

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
    serialPrintf("eip=0x%x, cs=0x%x, ss=0x%x, eflags=0x%x, useresp=0x%x\n", reg->eip, reg->cs, reg->ss, reg->eflags, reg->useresp);


    // DUMP PMM FIRST
    // This is because page faults will sometimes hang stackTrace
    serialPrintf("\n==== DEBUG INFORMATION FROM PMM DUMP ====\n");
    panic_dumpPMM();
    serialPrintf("\n==== END DEBUG INFORMATION OF PMM DUMP ====\n");

    // Perform stack trace (note: this might hang) 
    panic_stackTrace(7, reg);

    printf("\nThe system has been halted. Attach debugger now to view context.\n");

    
    asm volatile ("hlt");
    for (;;);
}
