// process.h - header file for the task scheduler/switcher

#ifndef PROCESS_H
#define PROCESS_H

/* INCLUDES */
#include <kernel/list.h>
#include <kernel/regs.h>
#include <kernel/tree.h>
#include <kernel/vfs.h>
#include <kernel/vmm.h>
#include <libk_reduced/signal.h>
#include <libk_reduced/signal_defs.h>
#include <libk_reduced/spinlock.h>
#include <libk_reduced/stdint.h>
#include <libk_reduced/time.h>

/************ DEFINITIONS ***********/

#define MAX_THREADS             5      // Maximum threads a process can have
#define PROCESS_INVALID_PID     -1     // Default invalid process ID
#define KSTACK_SIZE             0x9000 // Size of the kernel's  stack

// Process state bitflags (& flags for creation)
#define PROCESS_FLAG_REUSE_FDS  0x001 // Reuse the parent process' file descriptors
#define PROCESS_FLAG_IS_TASKLET 0x01  // Is the process a kernel tasklet?
#define PROCESS_FLAG_FINISHED   0x02  // Process finished
#define PROCESS_FLAG_STARTED    0x04  // Process started
#define PROCESS_FLAG_RUNNING    0x08  // Process running
#define PROCESS_FLAG_SLEEPINT   0x10  // Process interrupted for sleep
#define PROCESS_FLAG_SUSPEND    0x020 // Suspended process

// Wait options (should be in libc_reduced)
#define WNOHANG                 0x0001
#define WUNTRACED               0x0002
#define WSTOPPED                0x0004
#define WNOKERN                 0x0010

/************* TYPEDEFS *************/

// Variable types
typedef int pid_t; // Process ID

struct thread; // Prototype

typedef struct {
    uint32_t sp; // Stack
    uint32_t bp;
    uint32_t tls_base;
    uint32_t ip; // Entrypoint
    void* saved[6];
} thread_context_t;

// File descriptor
typedef struct _fd_table {
    fsNode_t** nodes;     // List of filesystem nodes used by the FDs
    uint64_t* fd_offsets; // Offsets of the FDs, used for writing/reading
    int* modes;           // For future use
    spinlock_t* fd_lock;

    size_t length;     // Amount of FDs actually in the table
    size_t max_fds;    // The length of the table
    size_t references; // References that child processes hold on the FDs
} fd_table_t;

// Thread
typedef struct _thread {
    thread_context_t context;                          // Thread context
    uint8_t fp_regs[512] __attribute__((aligned(16))); // FPU registers
    pagedirectory_t* page_directory;                   // Thread page directory
    int refcount;                                      // PD refcount
    spinlock_t* pd_lock;                               // PD lock
} thread_t;

// The image structure defines where the image/ELF is located, its size, its heap, its userstack, ...
typedef struct _image {
    uintptr_t entrypoint; // Entrypoint of the image
    uintptr_t stack;      // Stack allocated for the image
    uintptr_t userstack;  // Usermode stack allocated for the image
    uintptr_t shm_heap;   // Shared memory heap (unused)
    uintptr_t heap;       // Heap of the image, mapped using SBRK
    uintptr_t heap_start; // Start of the heap
    uintptr_t heap_end;   // End of the heap
    spinlock_t spinlock;  // Spinlock
} image_t;

// Signal configuration
struct signal_config {
    uintptr_t handler;
    sigset_t mask;
    int flags;
};

// This is the main process structure - all processes require this.
typedef struct process {
    pid_t id;           // ID of the process
    pid_t group;        // Thread group
    pid_t job;          // tty job
    pid_t session;      // tty session
    int status;         // Status code
    unsigned int flags; // State of the process - finished, running, started, ...
    int owner;          // Unused

    // String variables
    char* name;        // Process name
    char* description; // Process description
    char** cmdline;    // Command line used to call the process
    int isChild;

    // User and group IDs
    int user_id;
    int real_user;

    int user_group;
    int real_user_group;

    // Context variables
    registers_t* syscall_registers; // Registers for the process's system calls

    // Scheduler variables
    tree_node_t* tree_entry;
    list_t* waitQueue;
    list_t* shm_mappings; // Unused
    list_t* nodeWaits;

    node_t schedulerNode;
    node_t sleepNode;
    node_t* timedSleepNode;
    node_t* timeoutNode;
    spinlock_t sched_lock;
    spinlock_t wait_lock;

    // Working directory
    char* wd_name;
    fsNode_t* wd_node;

    // Thread & image
    thread_t thread;
    image_t image;

    // File descriptors
    fd_table_t* file_descs;

    // Signal values
    struct signal_config signals[NUMSIGNALS + 1];
    sigset_t blocked_signals;
    sigset_t pending_signals;
    sigset_t awaited_signals;

    // Time values
    struct timeval start;
    int awoken_index;
    int fs_wait;

    uint64_t time_prev;         // User time from previous update of usage[]
    uint64_t time_total;        // User time
    uint64_t time_sys;          // System time
    uint64_t time_in;           // PIT ticks from when the process last entered running state
    uint64_t time_switch;       // PIT ticks from when the process last started doing system things
    uint64_t time_children;     // Sum of user times from waited-for children
    uint64_t time_sys_children; // Sum of sys times from waited-for children
    uint16_t usage[4];          // 4 permille samples over some period (4Hz)

    // Syscall variables
    long interrupted_syscall; // Syscall restarting
} process_t;

// a mimir process
typedef struct {
    uint64_t end_tick;
    uint64_t end_subtick;
    process_t* process;
    int is_fswait;
} sleeper_t;

/************ FUNCTIONS ************/

// External functions
extern void start_process(uint32_t stack, uint32_t entry);
extern void restore_kernel_selectors();
extern void enter_tasklet();
extern int save_context(thread_context_t* context);
extern void load_context(thread_context_t* context);
extern uint32_t read_eip();
extern void resume_usermode();

extern process_t* currentProcess;

// Exposed functions
volatile process_t* process_getNextReadyProcess(); // Returns the next ready process
process_t* spawn_worker_thread(void (*entrypoint)(void* argp), const char* name, void* argp); // Spawn a worker thread
pagedirectory_t* cloneDirectory(pagedirectory_t* in); // Clone directory (NULL = current directory)
int createProcess(char* filepath, int argc, char* argv[], char* env[], int envc); // Creates a process from filepath
void task_exit(int retval);                                                       // Exit the current task
int process_alert_node(process_t* process, void* value);                          // Alert the process
void process_awaken_signal(process_t* process);
int process_timeout_sleep(process_t* process, int timeout); // Put a process to sleep
int waitpid(int pid, int* status, int options);             // Wait for a process to finish/suspend
void sleep_until(process_t* process, unsigned long seconds, unsigned long subseconds);
void wakeup_sleepers(unsigned long seconds, unsigned long subseconds);
process_t* spawn_process(volatile process_t* parent, int flags); // Spawn a process
int sleep_on_unlocking(list_t* queue, spinlock_t* release);      // Wait for a binary semaphore
void process_switchTask(uint8_t reschedule);
void tasking_start();
void scheduler_init();
pid_t fork();
void makeProcessReady(volatile process_t* proc);
int wakeup_queue(list_t* queue);
void updateProcessTimesOnExit();
unsigned long process_addfd(process_t* proc, fsNode_t* node);
long process_movefd(process_t* proc, long src, long dest);
process_t* process_get_parent(process_t* process);
process_t* process_from_pid(pid_t pid);
int process_isReady(process_t* proc);

#endif
