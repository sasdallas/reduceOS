// process.h - header file for the task scheduler/switcher

#ifndef PROCESS_H
#define PROCESS_H

/* INCLUDES */
#include <stdint.h>
#include <time.h>
#include <spinlock.h>
#include <stdatomic.h>
#include <kernel/vmm.h>
#include <kernel/vfs.h>
#include <kernel/regs.h>
#include <kernel/list.h>
#include <kernel/tree.h>

/************ DEFINITIONS ***********/

#define MAX_THREADS             5       // Maximum threads a process can have
#define PROCESS_INVALID_PID     -1      // Default invalid process ID
#define KSTACK_SIZE             0x9000  // Size of the kernel's  stack


// Process state bitflags
#define PROCESS_FLAG_IS_TASKLET 0x01
#define PROCESS_FLAG_FINISHED   0x02
#define PROCESS_FLAG_STARTED    0x04
#define PROCESS_FLAG_RUNNING    0x08
#define PROCESS_FLAG_SLEEPINT   0x10
#define PROCESS_FLAG_SUSPEND    0x020

// Wait options (should be in libc_reduced)
#define WNOHANG   0x0001
#define WUNTRACED 0x0002
#define WSTOPPED  0x0004
#define WNOKERN   0x0010


/************* TYPEDEFS *************/

// Variable types
typedef int pid_t;                  // Process ID



struct thread; // Prototype

typedef struct {
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t edi;
    uint32_t esi;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t flags;
    // TODO: Add more
} trapFrame_t;


typedef struct {
    uint32_t sp;                    // Stack
    uint32_t bp;                    
    uint32_t tls_base;              
    uint32_t ip;                    // Entrypoint
    void* saved[6];
} thread_context_t;

/*
struct process;
typedef struct thread {
    struct process *parent;         // Parent process of a thread
    void *initial_stack;            // Initial process stack
    void *stack_limit;              // Stack limit
    void *kernel_stack;             // Thread's kernel stack
    uint32_t priority;              // Priority of the thread
    uint32_t state;            // Thread state
    trapFrame_t frame;              // Trap frame
    uint32_t imageBase;             // Base of the executable
    uint32_t imageSize;             // Size of the image
} thread_t;
*/

typedef struct _thread {
    thread_context_t context;                               // Thread context
    uint8_t fp_regs[512] __attribute__((aligned(16)));      // FPU registers
    pagedirectory_t *page_directory;                        // Thread page directory
    int refcount;                                           // PD refcount
    atomic_flag *pd_lock;                                   // PD lock
} thread_t;

// The image structure defines where the image/ELF is located, its size, its heap, its userstack, ...
typedef struct _image {
    uintptr_t entrypoint;           // Entrypoint of the image
    uintptr_t stack;                // Stack allocated for the image
    uintptr_t userstack;            // Usermode stack allocated for the image
    uintptr_t shm_heap;             // Shared memory heap (unused)
    uintptr_t heap;                 // Heap of the image (unused)
    atomic_flag spinlock;           // Spinlock
} image_t;



// This is the main process structure - all processes require this.
typedef struct process {
    pid_t id;                       // ID of the process
    pid_t group;                    // Thread group
    pid_t job;                      // tty job
    pid_t session;                  // tty session
    int status;                     // Status code
    unsigned int flags;             // State of the process - finished, running, started, ...
    int owner;                      // Unused

    // String variables
    char *name;                     // Process name
    char *description;              // Process description
    char **cmdline;                 // Command line used to call the process

    // Context variables
    registers_t *syscall_registers; // Registers for the process's system calls

    // Scheduler variables
    tree_node_t *tree_entry;
    list_t *waitQueue;
    list_t *shm_mappings;           // Unused
    list_t *nodeWaits;

    node_t schedulerNode;
    node_t sleepNode;
    node_t *timedSleepNode;
    node_t *timeoutNode;
    atomic_flag sched_lock;
    atomic_flag wait_lock;

    // Working directory
    char *wd_name;
    fsNode_t *wd_node;

    // Thread & image
    thread_t thread;
    image_t image;


    // Time values
    struct timeval start;
    int awoken_index;
    int fs_wait;

    uint64_t time_prev;             // User time from previous update of usage[]
    uint64_t time_total;            // User time
    uint64_t time_sys;              // System time
    uint64_t time_in;               // PIT ticks from when the process last entered running state
    uint64_t time_switch;           // PIT ticks from when the process last started doing system things
    uint64_t time_children;         // Sum of user times from waited-for children
    uint64_t time_sys_children;     // Sum of sys times from waited-for children
    uint16_t usage[4];              // 4 permille samples over some period (4Hz)

    // Syscall variables
    long interrupted_syscall;       // Syscall restarting
} process_t;

// a mimir process
typedef struct {
    uint64_t end_tick;
    uint64_t end_subtick;
    process_t *process;
    int is_fswait;
} sleeper_t;

/************ FUNCTIONS ************/

// External functions
extern void start_process(uint32_t stack, uint32_t entry);
extern void restore_kernel_selectors();
extern void enter_tasklet();
extern int save_context(thread_context_t *context);
extern void restore_context(thread_context_t *context);

extern process_t *currentProcess;

// Other functions
volatile process_t *process_getNextReadyProcess();
pagedirectory_t *cloneKernelSpace2(pagedirectory_t *in);


#endif