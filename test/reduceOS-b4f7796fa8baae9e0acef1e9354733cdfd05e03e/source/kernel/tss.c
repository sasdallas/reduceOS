// =================================================================
// tss.c - Handles managing the Task State Segment
// =================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/tss.h" // Task State Segment

tss_t kernel_tss;


// tssInit(uint32_t index, uint32_t kernel_ss, uint32_t kernel_esp) - Initializes the task state segment (this can be used for user mode later)
void tssInit(uint32_t idx, uint32_t kss, uint32_t kesp) {
    uint32_t base = (uint32_t)&kernel_tss;
    gdtSetGate(idx, base, base + sizeof(tss_t), 0xE9, 0);
    /* Kernel tss, access(E9 = 1 11 0 1 0 0 1)
        1   present
        11  ring 3
        0   should always be 1, why 0? may be this value doesn't matter at all
        1   code?
        0   can not be executed by ring lower or equal to DPL,
        0   not readable
        1   access bit, always 0, cpu set this to 1 when accessing this sector(why 0 now?)
    */
    memset(&kernel_tss, 0, sizeof(tss_t));
    kernel_tss.ss0 = kss;
    // Note that we usually set tss's esp to 0 when booting our os, however, we need to set it to the real esp when we've switched to usermode because
    // the CPU needs to know what esp to use when usermode app is calling a kernel function(aka system call), that's why we have a function below called tss_set_stack
    kernel_tss.esp0 = kesp;
    kernel_tss.cs = 0x0b;
    kernel_tss.ds = 0x13;
    kernel_tss.es = 0x13;
    kernel_tss.fs = 0x13;
    kernel_tss.gs = 0x13;
    kernel_tss.ss = 0x13;
    tssFlush();
}



extern task_t *currentTask;

// setKernelStack() - Set the kernel stack.
void setKernelStack() {
    task_t *current_task = currentTask;
    // 16384 is the kernel stack size.
    kernel_tss.esp0 = (size_t)current_task->stackStart + 16384-16;
}