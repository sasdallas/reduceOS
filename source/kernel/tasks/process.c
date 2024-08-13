// ======================================================================
// process.c - Handles scheduling and creating processes/threads
// ======================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/process.h> // Main header file
#include <kernel/elf.h>

process_t *currentProcess;


process_t *process_getCurrentProcess() {
    return currentProcess;
}

typedef int func(int argc, char *argv[]);

// createProcess(char *filepath) - Creates a process from filepath, maps it into memory, and sets it up
int createProcess(char *filepath) {
    // First, get the file
    fsNode_t *file = open_file(filepath, 0);
    if (!file) {
        return -1; // Not found
    }

    char *buffer = kmalloc(file->length);
    int ret = file->read(file, 0, file->length, buffer);
    if (ret != file->length) {
        return -2; // Read error
    }

    // We basically have to take over ELF parsing.
    Elf32_Ehdr *ehdr = (Elf32_Ehdr*)buffer;
    if (elf_isCompatible(ehdr)) {
        kfree(buffer);
        return -3; // Not compatible
    }
    if (ehdr->e_type != ET_EXEC) {
        kfree(buffer);
        return -3; // Not compatible
    }

    pagedirectory_t *addressSpace = vmm_getCurrentDirectory();
    if (!addressSpace) {
        kfree(buffer);
        return -4; // Failed to get addre4ss space
    }

    // vmm_mapKernelSpace(addressSpace);

    // Create the process
    process_t *proc = process_getCurrentProcess();
    proc->pid = 1;
    proc->pageDirectory = addressSpace;
    proc->priority = 1;
    proc->state = ACTIVE;
    proc->threadCount = 1;

    // Create the main thread
    thread_t *mainThread = &proc->threads[0];
    mainThread->kernel_stack = 0;
    mainThread->parent = proc;
    mainThread->priority = 1;
    mainThread->state = ACTIVE;
    mainThread->initial_stack = 0;
    mainThread->stack_limit = (void*)((uint32_t)mainThread->initial_stack + 4096);
    memset(&mainThread->frame, 0, sizeof(trapFrame_t));
    mainThread->frame.eip = ehdr->e_entry;
    mainThread->frame.flags = 0x200;

    mainThread->imageBase = (int)ehdr;
    mainThread->imageSize = file->length;

    // Now, we should start parsing.
    uintptr_t usermodeBase = 0; // Should replace
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf32_Phdr *phdr = elf_getPHDR(ehdr, i);
        if (phdr->p_type == PT_LOAD) {
            // We have to load it into memory
            void *mem = pmm_allocateBlock();
            vmm_mapPhysicalAddress(addressSpace, phdr->p_vaddr, (uint32_t)mem, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
            memcpy(phdr->p_vaddr, buffer + phdr->p_offset, phdr->p_filesize);
        }

        if (phdr->p_vaddr + phdr->p_filesize + PAGE_SIZE > usermodeBase) usermodeBase = phdr->p_vaddr + phdr->p_filesize + PAGE_SIZE;
    }


    // Create a usermode stack
    serialPrintf("createProcess: Creating usermode stack at 0x%x...\n", usermodeBase);
    void *usermodeStack = usermodeBase;
    void *stackPhysical = pmm_allocateBlock(); // Allocate a physical block for the stack

    // Map the user process stack space
    vmm_mapPhysicalAddress(addressSpace, (uint32_t)usermodeStack, (uint32_t)stackPhysical, PTE_PRESENT|PTE_WRITABLE|PTE_USER);

    // Final initialization
    mainThread->initial_stack = usermodeStack;
    mainThread->frame.esp = (uint32_t)mainThread->initial_stack;
    mainThread->frame.ebp = mainThread->frame.esp;

    serialPrintf("createProcess: frame.esp = 0x%x\n", mainThread->frame.esp);


    // Return the process id
    return proc->pid;
}

// processInit() - Start up the process scheduler
void processInit() {
    // Right now, we should set up the kernel's process
    currentProcess = kmalloc(sizeof(process_t));
    currentProcess->pid = PROCESS_INVALID_PID;
}

// External functions
extern void start_process(uint32_t stack, uint32_t entry);
extern void restore_kernel_selectors();

// executeProcess() - Executes the current process
void executeProcess() {
    process_t *proc = 0;
    uint32_t entryPoint = 0;
    unsigned int procStack = 0;

    // Get the current runnig process
    proc = process_getCurrentProcess();
    if (proc->pid == PROCESS_INVALID_PID) {
        return; // No valid process
    }

    if (!proc->pageDirectory) {
        return; // Invalid address space
    }

    // Get the entrypoint & stack
    entryPoint = proc->threads[0].frame.eip;
    procStack = proc->threads[0].frame.esp;

    // Switch to process address space
    vmm_loadPDBR((physical_address)proc->pageDirectory);

    setKernelStack(2048);


    serialPrintf("entrypoint: 0x%x\nprocStack: 0x%x\n", entryPoint, procStack);
    start_process(procStack, entryPoint);

    // PROCESS SHOULD CALL _EXIT() WHEN FINISHED
}



void terminateProcess() {
    // Terminate the currently running process
    process_t *current = currentProcess;
    if (current->pid == PROCESS_INVALID_PID) return; 

    // Release the threads
    int i = 0;
    thread_t *pThread = &current->threads[i];

    // Get the physical address of the stack
    void *stackFrame = vmm_getPhysicalAddress(current->pageDirectory, (uint32_t)pThread->initial_stack);

    // Unmap it and release stack memory
    vmm_unmapPhysicalAddress(current->pageDirectory, (uint32_t)pThread->initial_stack);
    pmm_freeBlock(stackFrame);

    serialPrintf("-- Initial stack unmapped (0x%x)\n", pThread->frame.esp);

    // Find all segments that used PT_LOAD and unmap them
    Elf32_Ehdr *ehdr = (Elf32_Ehdr*)pThread->imageBase;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf32_Phdr *phdr = elf_getPHDR(ehdr, i);
        if (phdr->p_type == PT_LOAD) {
            // PT_LOAD segments are mapped to their respective vaddrs, we net to get the physical address
            uint32_t phys = vmm_getPhysicalAddress(current->pageDirectory, phdr->p_vaddr);

            serialPrintf("-- PT_LOAD segment vaddr 0x%x phys 0x%x unmapped\n", phdr->p_vaddr, phys);
            if (phys == 0x0) continue;

            // Unmap and release the page.
            vmm_unmapPhysicalAddress(current->pageDirectory, phys);
            pmm_freeBlock((void*)phys);
        }
    }


    // To prevent any page faults or GPFs we wait until after kernel selectors are restored to unmap imageBase

    // Restore the kernel selectors
    restore_kernel_selectors();

    serialPrintf("-- Restored kernel selectors\n");

    // Unmap the memory used by imageBase
    kfree(pThread->imageBase);

    serialPrintf("-- Freed memory\n");
    serialPrintf("-- Returning to kernel shell now...\n");
    kshell();
}