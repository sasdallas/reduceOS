// ======================================================================
// process.c - Handles scheduling and creating processes/threads
// ======================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

// Scheduler implementation: https://github.com/klange/toaruos/blob/master/kernel/sys/process.c

#include <kernel/clock.h>
#include <kernel/elf.h>
#include <kernel/process.h> // Main header file
#include <kernel/signal.h>
#include <libk_reduced/stdio.h>

// Because of the way the spinlocks work, the volatile property is discarded.
// This is bad, and has been logged as a change to be made.
// For now, I'm focused on making reduceOS warning-free.
// Is this cheating? Yes.
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"

#pragma GCC diagnostic ignored "-Wunused-value" // envp will break this code.

/************************ VARIABLES ************************/

process_t* currentProcess;  // The current process running.
process_t* previousProcess; // The previous process
tree_t* process_tree;       // Stores the parent-child process relationships
list_t* process_list;       // Stores all existing processes
list_t* process_queue;      // Scheduler ready queue
list_t* sleep_queue;        // Ordered list of processes waiting to be awoken by timeouts
list_t* reap_queue;         // Processes that couldn't be cleaned up and need to be deleted.

process_t* idleTask; // The kernel's idling task

// Spinlocks
static spinlock_t tree_lock = SPINLOCK_RELEASED;
static spinlock_t process_queue_lock = SPINLOCK_RELEASED;
static spinlock_t wait_lock_tmp = SPINLOCK_RELEASED;
static spinlock_t sleep_lock = SPINLOCK_RELEASED;
static spinlock_t reap_lock = SPINLOCK_RELEASED;
static spinlock_t switch_lock = SPINLOCK_RELEASED; // Controls who gets to switch page directories

/************************* TIMING **************************/

// updateProcessTimes() - Updates the total time and the system time when switching to a new thread or exiting the current one.
void updateProcessTimes() {
    uint64_t pTime = clock_getTimer();
    if (currentProcess->time_in && currentProcess->time_in < pTime) {
        currentProcess->time_total += pTime - currentProcess->time_in;
    }

    currentProcess->time_in = 0;

    if (currentProcess->time_switch && currentProcess->time_switch < pTime) {
        currentProcess->time_sys += pTime - currentProcess->time_switch;
    }
    currentProcess->time_switch = 0;
}

// updateProcessTimesOnExit() - Add time spent in kernel to time_sys when returning to userspace
void updateProcessTimesOnExit() {
    uint64_t pTime = clock_getTimer();
    if (currentProcess->time_switch && currentProcess->time_switch < pTime) {
        currentProcess->time_sys += pTime - currentProcess->time_switch;
    }
    currentProcess->time_switch = 0;
}

/*********************** SWITCHING ************************/

// process_switchContext(thread_t *thread) - Performs a context switch
void process_switchContext(thread_t* thread) { start_process(thread->context.sp, thread->context.ip); }

// process_switchNext() - Restore the context of the next available process's kernel thread
void process_switchNext() {
    previousProcess = currentProcess;
    updateProcessTimes();

    // Get the next avialable process, discarding anything marked as finished
    do { currentProcess = process_getNextReadyProcess(); } while (currentProcess->flags & PROCESS_FLAG_FINISHED);

    currentProcess->time_in = clock_getTimer();
    currentProcess->time_switch = currentProcess->time_in;

    // Restore paging and task switch context
    spinlock_lock(&switch_lock);
    vmm_switchDirectory(currentProcess->thread.page_directory);
    spinlock_release(&switch_lock);

    setKernelStack(currentProcess->image.stack);

    if (currentProcess->flags & PROCESS_FLAG_FINISHED) {
        panic("scheduler", "process_switchNext", "Process is marked finished, we should not have this process");
    }

    // Mark the process as running or started
    __sync_or_and_fetch(&currentProcess->flags, PROCESS_FLAG_STARTED);

    asm volatile("" ::: "memory");

    load_context(&currentProcess->thread.context);
}

// process_switchTask(uint8_t reschedule) - Yields the current process and allows the next to run
void process_switchTask(uint8_t reschedule) {
    if (!currentProcess) return; // Scheduler disabled

    if (currentProcess == idleTask) {
        panic("scheduler", "process_switchTask",
              "Context switch from idleTask triggered from somewhere other than pre-emption source.");
    }

    // If a process got to switchTask but was not marked as running, it must be exiting.
    if (!(currentProcess->flags & PROCESS_FLAG_RUNNING) || (currentProcess == idleTask)) {
        process_switchNext();
        return;
    }

    // Save the FPU registers (TODO: move this to fpu.c)
    asm volatile("fxsave (%0)" ::"r"(&currentProcess->thread.fp_regs));

    if (save_context(&currentProcess->thread.context) == 1) {
        // Returning from a task switch.
        asm volatile("fxrstor (%0)" ::"r"(&currentProcess->thread.fp_regs));
        return;
    }

    // If this is a normal yield, we nede to reschedule.
    if (reschedule) { makeProcessReady(currentProcess); }

    // switch_next() will not return
    process_switchNext();
}

// scheduler_init() - Initialize the scheduler datastructures
void scheduler_init() {
    process_tree = tree_create();
    process_list = list_create();
    process_queue = list_create();
    sleep_queue = list_create();
    reap_queue = list_create();
}

// isValidProcess(process_t *process) - Determines if a process is alive and valid
int isValidProcess(process_t* process) {
    foreach (lnode, process_list) {
        if (lnode->value == process) return 1;
    }

    return 0;
}

// getNextPID() - Allocate a process identifier (TODO: bad)
pid_t getNextPID() {
    static pid_t _next_pid = 2;
    return __sync_fetch_and_add(&_next_pid, 1);
}

// kidle() - Kernel idle task
static void kidle() {
    while (1) {
        asm volatile("sti\n"
                     "hlt\n"
                     "cli\n");
        process_switchNext();
    }
}

// process_releaseDirectory(thread_t *thread) - Release a process's paging data
void process_releaseDirectory(thread_t* thread) {
    serialPrintf("Releasing process directory for thread 0x%x\n", thread);
    spinlock_lock(thread->pd_lock);
    thread->refcount--;
    if (thread->refcount < 1) {
        pmm_freeBlock(thread->page_directory);
    } else {
        spinlock_release(thread->pd_lock);
    }
}

// spawn_kidle(int bsp) - Spawns the kernel's idle process
process_t* spawn_kidle(int bsp) {
    process_t* idle = kcalloc(1, sizeof(process_t));
    idle->id = -1;
    idle->flags = PROCESS_FLAG_IS_TASKLET | PROCESS_FLAG_STARTED | PROCESS_FLAG_RUNNING;

    // Create a stack for the image and map it to kernel (non-user)
    idle->image.stack = (uintptr_t)kmalloc(KSTACK_SIZE) + KSTACK_SIZE; // WARNING: BUG HERE - THIS IS NOT ALIGNED
    vmm_mapPhysicalAddress(vmm_getCurrentDirectory(), idle->image.stack,
                           (uint32_t)vmm_getPhysicalAddress(vmm_getCurrentDirectory(), idle->image.stack), PTE_PRESENT);

    // Setup thread context
    idle->thread.context.ip = (uint32_t)&kidle;
    idle->thread.context.sp = idle->image.stack;
    idle->thread.context.bp = idle->image.stack;

    // Setup the queue & other lists
    idle->waitQueue = list_create();
    idle->shm_mappings = list_create();
    gettimeofday(&idle->start, NULL);

    // Setup the page directory and clone it from kspace
    idle->thread.page_directory = cloneDirectory(vmm_getCurrentDirectory());
    idle->thread.refcount = 1;
    idle->thread.pd_lock = spinlock_init(); // bug?

    // Name and description
    idle->name = kmalloc(strlen("idle"));
    strcpy(idle->name, "idle");

    idle->description = kmalloc(strlen("Kernel idle process"));
    strcpy(idle->description, "Kernel idle process");

    return idle;
}

process_t* spawn_init() {
    process_t* init = kcalloc(1, sizeof(process_t));
    tree_set_root(process_tree, (void*)init);

    // Setup process
    init->tree_entry = process_tree->root;
    init->id = 1;
    init->group = 0;
    init->job = 1;
    init->session = 1;
    init->name = kmalloc(strlen("init"));
    strcpy(init->name, "init");
    init->cmdline = NULL;
    init->status = 0;

    init->description = kmalloc(strlen("initial process"));
    strcpy(init->description, "initial process");

    // Setup working directory values
    init->wd_node = kmalloc(sizeof(fsNode_t));
    memcpy(init->wd_node, fs_root, sizeof(fsNode_t));
    init->wd_name = kmalloc(2);
    strcpy(init->wd_name, "/");

    // Setup image stack and other values
    init->image.entrypoint = 0;
    init->image.heap = 0;
    init->image.heap_start = 0;
    init->image.heap_end = 0;
    init->image.stack = (uintptr_t)kmalloc(KSTACK_SIZE) + KSTACK_SIZE;
    vmm_mapPhysicalAddress(vmm_getCurrentDirectory(), init->image.stack,
                           (uint32_t)vmm_getPhysicalAddress(vmm_getCurrentDirectory(), init->image.stack), PTE_PRESENT);

    // Setup flags
    init->flags = PROCESS_FLAG_STARTED | PROCESS_FLAG_RUNNING;

    // Setup queues
    init->waitQueue = list_create();
    init->shm_mappings = list_create();

    // Setup scheduler & time values
    init->schedulerNode.prev = NULL;
    init->schedulerNode.next = NULL;
    init->schedulerNode.value = init;

    init->sleepNode.prev = NULL;
    init->sleepNode.next = NULL;
    init->sleepNode.value = init;

    init->timedSleepNode = NULL;

    // Setup the file descriptors
    init->file_descs = kmalloc(sizeof(fd_table_t));
    init->file_descs->length = 0;
    init->file_descs->max_fds = 4;
    init->file_descs->nodes = kmalloc(init->file_descs->max_fds * sizeof(fsNode_t*));
    init->file_descs->modes = kmalloc(init->file_descs->max_fds * sizeof(int));
    init->file_descs->fd_offsets = kmalloc(init->file_descs->max_fds * sizeof(uint64_t));
    init->file_descs->fd_lock = spinlock_init();

    // Setup the page directory
    init->thread.page_directory = vmm_getCurrentDirectory();
    init->thread.refcount = 1;
    init->thread.pd_lock = spinlock_init();

    list_insert(process_list, (void*)init);
    return init;
}

// spawn_process(volatile process_t *parent, int flags) - Spawn a process
process_t* spawn_process(volatile process_t* parent, int flags) {
    process_t* proc = kcalloc(1, sizeof(process_t));

    // Setup values to be like the parents'
    proc->id = getNextPID();
    proc->group = proc->id;
    proc->job = parent->job;
    proc->session = parent->session;

    // Setup name & description
    proc->name = kmalloc(strlen(parent->name));
    strcpy(proc->name, parent->name);
    proc->description = NULL;

    // Setup thread context
    proc->thread.context.sp = 0;
    proc->thread.context.bp = 0;
    proc->thread.context.ip = 0;
    memcpy((void*)proc->thread.fp_regs, (void*)parent->thread.fp_regs, 512);

    // Entry is only stored for reference
    proc->image.entrypoint = parent->image.entrypoint;
    proc->image.heap = parent->image.heap;
    proc->image.heap_end = parent->image.heap_end;
    proc->image.heap_start = parent->image.heap_start;
    proc->image.stack = (uintptr_t)kmalloc(KSTACK_SIZE) + KSTACK_SIZE;
    vmm_mapPhysicalAddress(vmm_getCurrentDirectory(), proc->image.stack,
                           (uint32_t)vmm_getPhysicalAddress(vmm_getCurrentDirectory(), proc->image.stack), PTE_PRESENT);
    proc->image.shm_heap = NULL; // Unused

    proc->wd_node = kmalloc(sizeof(fsNode_t));
    memcpy(proc->wd_node, parent->wd_node, sizeof(fsNode_t));

    proc->wd_name = kmalloc(strlen(parent->wd_name));
    strcpy(proc->wd_name, parent->wd_name);

    // Setup wait queue & shared memory mappings
    proc->waitQueue = list_create();
    proc->shm_mappings = list_create();

    // Setup scheduler and sleep nodes
    proc->schedulerNode.value = proc;
    proc->sleepNode.value = proc;

    // Setup time
    gettimeofday(&proc->start, NULL);

    // Check if the kernel wants to reuse file descriptors
    if (flags & PROCESS_FLAG_REUSE_FDS) {
        // Reuse file descriptors from parent process
        spinlock_lock(parent->file_descs->fd_lock);
        proc->file_descs = parent->file_descs;
        spinlock_release(parent->file_descs->fd_lock);
    } else {
        // Allocate memory and clone over some values
        proc->file_descs = kmalloc(sizeof(fd_table_t));
        proc->file_descs->fd_lock = spinlock_init();
        spinlock_lock(parent->file_descs->fd_lock);
        proc->file_descs->length = parent->file_descs->length;
        proc->file_descs->max_fds = parent->file_descs->max_fds;

        // Allocate memory for lists
        proc->file_descs->nodes = kmalloc(proc->file_descs->max_fds * sizeof(fsNode_t*));
        proc->file_descs->modes = kmalloc(proc->file_descs->max_fds * sizeof(int));
        proc->file_descs->fd_offsets = kmalloc(proc->file_descs->max_fds * sizeof(uint64_t));

        // Copy over the file descriptors
        for (uint32_t i = 0; i < proc->file_descs->length; i++) {
            proc->file_descs->nodes[i] = kmalloc(sizeof(fsNode_t));
            memcpy(proc->file_descs->nodes[i], parent->file_descs->nodes[i], sizeof(fsNode_t));
            proc->file_descs->modes[i] = parent->file_descs->modes[i];
            proc->file_descs->fd_offsets[i] = parent->file_descs->fd_offsets[i];
        }

        spinlock_release(parent->file_descs->fd_lock);
    }

    // Setup tree node and insert it
    tree_node_t* entry = tree_node_create(proc);
    proc->tree_entry = entry;

    spinlock_lock(&tree_lock);
    tree_node_insert_child_node(process_tree, parent->tree_entry, entry);
    list_insert(process_list, (void*)proc);
    spinlock_release(&tree_lock);
    return proc;
}

// process_reap(process_t *proc) - Frees & releases the process
void process_reap(process_t* proc) {
    // Remap the stack bottom to be writable
    vmm_mapPhysicalAddress(vmm_getCurrentDirectory(), proc->image.stack,
                           (uint32_t)vmm_getPhysicalAddress(vmm_getCurrentDirectory(), proc->image.stack),
                           PTE_PRESENT | PTE_WRITABLE);

    // Free the stack
    kfree((void*)(proc->image.stack - KSTACK_SIZE));
    process_releaseDirectory(&proc->thread);

    kfree(proc->name);
    if (proc->description) kfree(proc->description);
    kfree(proc);
}

// (static) process_isOwned(process_t *proc) - Returns whether the process is a previous/current process
static int process_isOwned(process_t* proc) {
    if (currentProcess == proc || previousProcess == proc) return 1;
    return 0;
}

// process_reapLater(process_t *proc) - Marks the process to be deleted later
void process_reapLater(process_t* proc) {
    spinlock_lock(&reap_lock);

    while (reap_queue->head) {
        // Check if anything can be deleted from the reap queue while we're here
        process_t* proc = reap_queue->head->value;
        if (!process_isOwned(proc)) {
            kfree(list_dequeue(reap_queue));
            process_reap(proc);
        } else {
            break;
        }
    }

    list_insert(reap_queue, proc);
    spinlock_release(&reap_lock);
}

// process_delete(process_t *proc) - Remove a process from the valid process list
void process_delete(process_t* proc) {
    ASSERT((proc != currentProcess), "process_delete", "Attempted to delete current process");

    tree_node_t* entry = proc->tree_entry;
    if (!entry) {
        serialPrintf("process_delete: Tried to delete process but the object is corrupt (could not get tree entry).\n");
        return;
    }

    if (process_tree->root == entry) {
        // stupid users!!
        serialPrintf("process_delete: Tried to delete the init process - blocked.\n");
        return;
    }

    // Remove it from the tree
    spinlock_lock(&tree_lock);
    int has_children = entry->children->length;
    tree_remove_reparent_root(process_tree, entry);
    list_delete(process_list, list_find(process_list, proc));
    spinlock_release(&tree_lock);

    if (has_children) {
        // Wakeup init process
        process_t* init = process_tree->root->value;
        wakeup_queue(init->waitQueue);
    }

    proc->tree_entry = NULL;
    kfree(proc->shm_mappings);

    // Check if the process is in use
    if (process_isOwned(proc)) {
        process_reapLater(proc);
        return;
    }

    process_reap(proc);
}

// makeProcessReady(volatile process_t *proc) - Place an available process in the ready queue
void makeProcessReady(volatile process_t* proc) {
    if (proc->sleepNode.owner != NULL) {
        serialPrintf("a mimir\n");
        if (proc->sleepNode.owner == sleep_queue) {
            // The sleep queue is a little speical
            if (proc->timedSleepNode) {
                list_delete(sleep_queue, proc->timedSleepNode);
                proc->sleepNode.owner = NULL;
                kfree(proc->timedSleepNode->value);
            }
        } else {
            // Blocked on a smeaphore that we can interrupt
            __sync_or_and_fetch(&proc->flags, PROCESS_FLAG_SLEEPINT);
            list_delete((list_t*)proc->sleepNode.owner, (node_t*)&proc->sleepNode);
        }
    }

    spinlock_lock(&process_queue_lock);
    if (proc->schedulerNode.owner) {
        // Only one ready queue - process was already ready??
        spinlock_release(&process_queue_lock);
        return;
    }

    list_append(process_queue, (node_t*)&proc->schedulerNode);

    spinlock_release(&process_queue_lock);
}

// process_getNextReadyProcess() - Pops the next available process from the queue
volatile process_t* process_getNextReadyProcess() {
    spinlock_lock(&process_queue_lock);

    if (!process_queue->head) {
        if (process_queue->length) {
            panic("scheduler", "get_next_ready", "Process queue has length but the head is NULL");
        }

        spinlock_release(&process_queue_lock);
        return idleTask; // No processes to run
    }

    node_t* np = list_dequeue(process_queue);

    volatile process_t* next = np->value;

    spinlock_release(&process_queue_lock);

    if (!(next->flags & PROCESS_FLAG_FINISHED)) { __sync_or_and_fetch(&next->flags, PROCESS_FLAG_RUNNING); }

    return next;
}

// wakeup_queue(list_t *queue) - Signal a semaphore
int wakeup_queue(list_t* queue) {
    int awoken_processes = 0;
    spinlock_lock(&wait_lock_tmp);
    while (queue->length > 0) {
        node_t* node = list_pop(queue);
        spinlock_release(&wait_lock_tmp);
        if (!(((process_t*)node->value)->flags & PROCESS_FLAG_FINISHED)) { makeProcessReady(node->value); }
        spinlock_lock(&wait_lock_tmp);
        awoken_processes++;
    }

    spinlock_release(&wait_lock_tmp);
    return awoken_processes;
}

/* TODO: Implement wakeup_queue_interrupted */

// wakeup_queue_one(list_t *queue) - Only doing one iteration/pass
int wakeup_queue_one(list_t* queue) {
    int awoken_processes = 0;
    spinlock_lock(&wait_lock_tmp);

    if (queue->length > 0) {
        node_t* node = list_pop(queue);
        spinlock_release(&wait_lock_tmp);
        if (!(((process_t*)node->value)->flags & PROCESS_FLAG_FINISHED)) { makeProcessReady(node->value); }
        spinlock_lock(&wait_lock_tmp);
        awoken_processes++;
    }

    spinlock_release(&wait_lock_tmp);
    return awoken_processes;
}

// sleep_on(list_t *queue) - Wait for a binary semaphore
int sleep_on(list_t* queue) {
    if (currentProcess->sleepNode.owner) {
        process_switchTask(0);
        return 0;
    }

    __sync_and_and_fetch(&currentProcess->flags, ~(PROCESS_FLAG_SLEEPINT));
    spinlock_lock(&wait_lock_tmp);
    list_append(queue, (node_t*)&currentProcess->sleepNode);
    spinlock_release(&wait_lock_tmp);
    process_switchTask(0);
    return !!(currentProcess->flags & PROCESS_FLAG_SLEEPINT);
}

// sleep_on_unlocking(list_t *queue, spinlock_t *release) - Wait for a binary semaphore on unlocking
int sleep_on_unlocking(list_t* queue, spinlock_t* release) {
    __sync_and_and_fetch(&currentProcess->flags, ~(PROCESS_FLAG_SLEEPINT));
    spinlock_lock(&wait_lock_tmp);
    list_append(queue, (node_t*)&currentProcess->sleepNode);
    spinlock_release(&wait_lock_tmp);

    spinlock_release(release);

    process_switchTask(0);
    return !!(currentProcess->flags & PROCESS_FLAG_SLEEPINT);
}

// process_isReady(process_t *proc) - Indicates whether a process is ready to be run but not currently running
int process_isReady(process_t* proc) {
    return (proc->schedulerNode.owner != NULL && !(proc->flags & PROCESS_FLAG_RUNNING));
}

// Function prototype
int process_alert_node_locked(process_t* process, void* value);

// wakeup_sleepers(unsigned long seconds, unsigned long subseconds) - Reschedule all processes whose timed waits have expired
void wakeup_sleepers(unsigned long seconds, unsigned long subseconds) {
    if (!currentProcess) return; // Process scheduler not online
    spinlock_lock(&sleep_lock);
    if (sleep_queue->length) {
        sleeper_t* proc = ((sleeper_t*)sleep_queue->head->value);
        while (proc && (proc->end_tick < seconds || (proc->end_tick == seconds && proc->end_subtick <= subseconds))) {
            // Timeouts have expired, mark the processes as ready and clear their sleep nodes.
            if (proc->is_fswait) {
                proc->is_fswait = -1;
                process_alert_node_locked(proc->process, proc);
            } else {
                process_t* process = proc->process;
                process->sleepNode.owner = NULL;
                process->timedSleepNode = NULL;
                if (!process_isReady(process)) { makeProcessReady(process); }
            }

            kfree(proc);
            kfree(list_dequeue(sleep_queue));
            if (sleep_queue->length) {
                proc = ((sleeper_t*)sleep_queue->head->value); // Not done yet
            } else {
                break; // We're done
            }
        }
    }

    // All done, release the spinlock.
    spinlock_release(&sleep_lock);
}

// sleep_until(process_t *process, unsigned long seconds, unsigned long subseconds) - Suspend process until the given time
void sleep_until(process_t* process, unsigned long seconds, unsigned long subseconds) {
    spinlock_lock(&sleep_lock);
    if (currentProcess->sleepNode.owner) {
        // We're already sleeping. Sweet dreams!
        spinlock_release(&sleep_lock);
        return;
    }

    process->sleepNode.owner = sleep_queue;

    node_t* before = NULL;
    foreach (node, sleep_queue) {
        sleeper_t* candidate = ((sleeper_t*)node->value);
        if (!candidate) {
            serialPrintf("sleep_until: Null candidate\n");
            continue;
        }

        if (candidate->end_tick > seconds || (candidate->end_tick == seconds && candidate->end_subtick > subseconds)) {
            break;
        }
        before = node;
    }

    // Create a sleeper_t object, append it to the list, and then return.
    sleeper_t* proc = kmalloc(sizeof(sleeper_t));
    proc->process = process;
    proc->end_tick = seconds;
    proc->end_subtick = subseconds;
    proc->is_fswait = 0;
    list_insert_after(sleep_queue, before, proc);
    process->timedSleepNode = list_find(sleep_queue, proc);
    spinlock_release(&sleep_lock);
}

// process_compare(void *proc_v, void *pid_v) - Compare a process and a PID
uint8_t process_compare(void* proc_v, void* pid_v) {
    pid_t pid = (*(pid_t*)pid_v);
    process_t* proc = (process_t*)proc_v;
    return (uint8_t)(proc->id == pid);
}

// process_from_pid(pid_t pid) - Gets a process by its PID
process_t* process_from_pid(pid_t pid) {
    if (pid < 0) return NULL; // users

    spinlock_lock(&tree_lock);
    tree_node_t* entry = tree_find(process_tree, &pid, process_compare);
    spinlock_release(&tree_lock);
    if (entry) return (process_t*)entry->value;
    return NULL;
}

// tasking_start() - Creates the tasks for the kernel
void tasking_start() {
    currentProcess = spawn_init();
    idleTask = spawn_kidle(1);
}

static int wait_candidate(volatile process_t* parent, int pid, int options, volatile process_t* proc) {
    if (!proc) return 0;

    if (options & WNOKERN) {
        // Skip kernel processes
        if (proc->flags & PROCESS_FLAG_IS_TASKLET) return 0;
    }

    if (pid < -1) {
        if (proc->job == -pid || proc->id == -pid) return 1;
    } else if (pid == 0) {
        // Check if it matches our GID
        if (proc->job == parent->id) return 1;
    } else if (pid > 0) {
        if (proc->id == pid) return 1;
    } else {
        return 1;
    }

    return 0;
}

// waitpid(int pid, int *status, int options) - Waits for a process to finish/suspend
int waitpid(int pid, int* status, int options) {
    volatile process_t* volatile proc = (process_t*)currentProcess;
    serialPrintf("waitpid: Call received.\n");

    do {
        serialPrintf("waitpid: Looping...\n");
        volatile process_t* candidate = NULL;
        int has_children = 0;
        int is_parent = 0;

        spinlock_lock(&proc->wait_lock);

        // Find out if there is anything to reap
        foreach (node, proc->tree_entry->children) {
            if (!node->value) continue;
            volatile process_t* volatile child = ((tree_node_t*)node->value)->value;

            if (wait_candidate(proc, pid, options, child)) {
                has_children = 1;
                is_parent = 1;
                if (child->flags & PROCESS_FLAG_FINISHED) {
                    candidate = child;
                    break;
                }

                if ((child->flags & PROCESS_FLAG_SUSPEND) && ((child->status & 0xFF) == 0x7F)) {
                    int reason = (child->status >> 16) & 0xFF;
                    if ((options & WSTOPPED) || (reason == 0xFF && (options & WUNTRACED))) {
                        candidate = child;
                        break;
                    }
                }
            }
        }

        if (!has_children) {
            serialPrintf("waitpid: no children found\n");
            return -1;
        }

        if (candidate) {
            spinlock_release(&proc->wait_lock);
            serialPrintf("waitpid: Candidate '%s' found.\n", candidate->name);
            if (status) *status = candidate->status;

            candidate->status &= ~0xFF;
            int pid = candidate->id;
            if (is_parent && (candidate->flags & PROCESS_FLAG_FINISHED)) {
                while (*((volatile int*)&candidate->flags) & PROCESS_FLAG_RUNNING);
                proc->time_children += candidate->time_children + candidate->time_total;
                proc->time_sys_children += candidate->time_sys_children + candidate->time_sys;
                process_delete((process_t*)candidate);
            }
            return pid;
        } else {
            if (options & WNOHANG) {
                spinlock_release(&proc->wait_lock);
                return 0;
            }

            serialPrintf("No candidate was found.\n");

            // Wait
            if (sleep_on_unlocking(proc->waitQueue, &proc->wait_lock) != 0) { return -2; }
        }
    } while (1);
}

// process_timeout_sleep(process_t *process, int timeout) - Put a process to sleep
int process_timeout_sleep(process_t* process, int timeout) {
    // Calculate the time to sleep
    unsigned long s, ss;
    rtc_getDateTime((uint8_t*)&s, NULL, NULL, NULL, NULL, NULL);
    ss = s * 1000;
    s += timeout;
    ss += timeout * 1000;

    // Find the process
    node_t* before = NULL;
    foreach (node, sleep_queue) {
        sleeper_t* candidate = ((sleeper_t*)node->value);
        if (candidate->end_tick > s || (candidate->end_tick == s && candidate->end_subtick > ss)) { break; }
        before = node;
    }

    sleeper_t* proc = kmalloc(sizeof(sleeper_t));
    proc->process = process;
    proc->end_tick = s;
    proc->end_subtick = ss;
    proc->is_fswait = 1;
    list_insert(process->nodeWaits, (void*)proc);
    list_insert_after(sleep_queue, before, proc);
    process->timeoutNode = list_find(sleep_queue, proc);

    return 0;
}

// process_awaken_from_fswait(process_t *process, int index) - Awaken from process_timeout_sleep
int process_awaken_from_fswait(process_t* process, int index) {
    spinlock_lock(&sleep_lock);
    process->awoken_index = index;
    list_free(process->nodeWaits);
    kfree(process->nodeWaits);
    process->nodeWaits = NULL;

    if (process->timeoutNode && process->timeoutNode->owner == sleep_queue) {
        sleeper_t* proc = process->timeoutNode->value;
        if (proc->is_fswait != -1) {
            list_delete(sleep_queue, process->timeoutNode);
            kfree(process->timeoutNode->value);
            kfree(process->timeoutNode);
        }
    }

    process->timeoutNode = NULL;

    makeProcessReady(process);
    spinlock_release(&process->sched_lock);
    return 0;
}

// process_awaken_signal(process_t *process) - too lazy
void process_awaken_signal(process_t* process) {
    spinlock_lock(&sleep_lock);
    spinlock_lock(&process->sched_lock);

    if (process->nodeWaits) {
        process_awaken_from_fswait(process, -4);
    } else {
        spinlock_release(&process->sched_lock);
    }

    spinlock_release(&sleep_lock);
}

int process_alert_node_locked(process_t* process, void* value) {
    if (!isValidProcess(process)) {
        serialPrintf("process_alert_node_locked: process pid=%d %s attempted to alert invalid process %#zx\n",
                     currentProcess->id, currentProcess->name, (uintptr_t)process);
        return 0;
    }

    spinlock_lock(&process->sched_lock);

    if (!process->nodeWaits) {
        spinlock_release(&process->sched_lock);
        return 0;
    }

    int index = 0;
    foreach (node, process->nodeWaits) {
        if (value == node->value) { return process_awaken_from_fswait(process, index); }

        index++;
    }

    spinlock_release(&process->sched_lock);
    return -1;
}

// process_alert_node(process_t *process, void *value) - Alert the process
int process_alert_node(process_t* process, void* value) {
    spinlock_lock(&sleep_lock);
    int result = process_alert_node_locked(process, value);
    spinlock_release(&sleep_lock);
    return result;
}

// process_get_parent(process_t *process) - Returns the parent of a process
process_t* process_get_parent(process_t* process) {
    process_t* result = NULL;
    spinlock_lock(&tree_lock);

    tree_node_t* entry = process->tree_entry;
    if (entry->parent)
        result = entry->parent->value;
    else
        serialPrintf("process_get_parent: No parent for this process was found.\n");

    spinlock_release(&tree_lock);
    return result;
}

// task_exit(int retval) - Exit a task
void task_exit(int retval) {
    currentProcess->status = retval;

    // Free whatever we can
    list_free(currentProcess->waitQueue);
    kfree(currentProcess->waitQueue);
    kfree(currentProcess->wd_name);
    if (currentProcess->nodeWaits) {
        list_free(currentProcess->nodeWaits);
        kfree(currentProcess->nodeWaits);
        currentProcess->nodeWaits = NULL;
    }

    if (currentProcess->file_descs) {
        spinlock_lock(currentProcess->file_descs->fd_lock);
        currentProcess->file_descs->references--;

        if (currentProcess->file_descs->references == 0) {
            // No child processes currently hold a refernece
            for (uint32_t fd = 0; fd < currentProcess->file_descs->length; fd++) {
                if (currentProcess->file_descs->nodes[fd]) {
                    closeFilesystem(currentProcess->file_descs->nodes[fd]);
                    // VFS currently doesn't take care of freeing
                    kfree(currentProcess->file_descs->nodes[fd]);
                }

                kfree(currentProcess->file_descs->nodes);
                kfree(currentProcess->file_descs->fd_offsets);
                kfree(currentProcess->file_descs->modes);
                kfree(currentProcess->file_descs);
                currentProcess->file_descs = NULL;
            }
        } else {
            // One does, dang..
            spinlock_release(currentProcess->file_descs->fd_lock);
        }
    }

    updateProcessTimes();

    process_t* parent = process_get_parent((process_t*)currentProcess);
    __sync_or_and_fetch(&currentProcess->flags, PROCESS_FLAG_FINISHED);

    // Before we switch to the next process we need to check if we just killed the init process.
    // Shoot first, ask questions later.
    if (currentProcess->id == 1) {
        // Uhh, did we just kill the init process?
        //panic("reduceOS", "kernel", "A process critical to the system (the init process) has terminated unexpectedly.");
    }

    if (parent && !(parent->flags & PROCESS_FLAG_FINISHED)) {
        spinlock_lock(&parent->wait_lock);
        serialPrintf("task_exit: Sending SIGCHLD...\n");
        send_signal(parent->group, SIGCHLD, 1);
        wakeup_queue(parent->waitQueue);
        spinlock_release(&parent->wait_lock);
    }

    process_switchNext();
}

#define PUSH(stack, type, item)                                                                                        \
    stack -= sizeof(type);                                                                                             \
    *((type*)stack) = item

// fork() - Fork the current process and creates a child process. Will return the PID of the child to the parent, and 0 to the child.
pid_t fork() {

    uint32_t *sp, bp;
    process_t* parent = currentProcess;

    // Clone the page directory and set it up
    pagedirectory_t* directory = cloneDirectory(parent->thread.page_directory);

    // Create a new process and setup its thread page directory
    process_t* new_proc = spawn_process(parent, 0);
    new_proc->thread.page_directory = directory;
    new_proc->thread.refcount = 1;
    new_proc->thread.pd_lock = spinlock_init();

    memcpy(new_proc->signals, parent->signals, sizeof(struct signal_config) * (NUMSIGNALS - 1));
    new_proc->blocked_signals = parent->blocked_signals;

    // Create system call registers
    registers_t r;
    memcpy(&r, parent->syscall_registers, sizeof(registers_t));

    // Setup SP and BP
    sp = &(new_proc->image.stack);
    bp = (uint32_t)sp;

    // Setup return value in syscall registers
    r.eax = 0;

    // Push it onto the stack, manually because PUSH() doesn't seem to work right now.
    sp -= sizeof(registers_t); // Stack grows downwards
    memcpy(sp, &r, sizeof(registers_t));

    // Setup registers and context
    new_proc->syscall_registers = (void*)sp;
    new_proc->thread.context.sp = (uint32_t)sp;
    new_proc->thread.context.bp = bp;
    new_proc->thread.context.tls_base = parent->thread.context.tls_base;

    new_proc->thread.context.ip = (uint32_t)&resume_usermode;

    // Save the context to the parent, but ONLY copy the saved part of the context over.
    save_context(&parent->thread.context);
    memcpy(new_proc->thread.context.saved, parent->thread.context.saved, sizeof(parent->thread.context.saved));

    if (parent->flags & PROCESS_FLAG_IS_TASKLET) new_proc->flags |= PROCESS_FLAG_IS_TASKLET;
    makeProcessReady(new_proc);

    return new_proc->id;
}

// clone() - create a new thread/process (likely bugged and probably will not work)
pid_t clone(uintptr_t new_stack, uintptr_t thread_func, uintptr_t arg) {
    uintptr_t sp, bp;
    process_t* parent = (process_t*)currentProcess;
    process_t* new_proc = spawn_process(currentProcess, 1);

    // BUG: These are supposed to refer to each other, right?
    new_proc->thread.page_directory = currentProcess->thread.page_directory;
    new_proc->thread.refcount = currentProcess->thread.refcount;
    new_proc->thread.pd_lock = currentProcess->thread.pd_lock;

    memcpy(new_proc->signals, parent->signals, sizeof(struct signal_config) * (NUMSIGNALS - 1));
    new_proc->blocked_signals = parent->blocked_signals;

    spinlock_lock(new_proc->thread.pd_lock);
    new_proc->thread.refcount++;
    spinlock_release(new_proc->thread.pd_lock);

    // Setup system call registers and BP/SP
    registers_t r;
    memcpy(&r, parent->syscall_registers, sizeof(registers_t));
    sp = new_proc->image.stack;
    bp = sp;
    r.edi = arg; // Different calling conventions

    // Push them onto the stack
    PUSH(new_stack, uintptr_t, (uintptr_t)0);
    PUSH(sp, registers_t, r);

    // Setup ESP, EBP, and EIP
    new_proc->syscall_registers = (void*)sp;
    new_proc->syscall_registers->esp = new_stack;
    new_proc->syscall_registers->ebp = new_stack;
    new_proc->syscall_registers->eip = thread_func;

    new_proc->thread.context.sp = sp;
    new_proc->thread.context.bp = bp;
    new_proc->thread.context.tls_base = currentProcess->thread.context.tls_base;
    new_proc->thread.context.ip = (uintptr_t)&resume_usermode;

    if (parent->flags & PROCESS_FLAG_IS_TASKLET) new_proc->flags |= PROCESS_FLAG_IS_TASKLET;
    makeProcessReady(new_proc);

    enableHardwareInterrupts();

    return new_proc->id;
}

// spawn_worker_thread(void (*entrypoint)(void *argp), const char *name, void *argp) - Spawn a worker thread
process_t* spawn_worker_thread(void (*entrypoint)(void* argp), const char* name, void* argp) {
    process_t* proc = kcalloc(1, sizeof(process_t));

    // Setup basic fields
    proc->flags = PROCESS_FLAG_IS_TASKLET | PROCESS_FLAG_STARTED;
    proc->id = getNextPID();
    proc->group = proc->id;
    proc->name = kmalloc(strlen(name));
    strcpy(proc->name, name);
    proc->description = NULL;
    proc->cmdline = NULL;

    proc->job = proc->id;
    proc->session = proc->id;

    // Setup page directory
    proc->thread.page_directory = cloneDirectory(vmm_getCurrentDirectory()); // Is this possibly bugged?
    proc->thread.refcount = 1;
    proc->thread.pd_lock = spinlock_init();

    // Setup the image stack
    proc->image.stack = (uintptr_t)kmalloc(KSTACK_SIZE) + KSTACK_SIZE;
    PUSH(proc->image.stack, uintptr_t, (uintptr_t)entrypoint);
    PUSH(proc->image.stack, void*, argp);

    // Setup the context
    proc->thread.context.sp = proc->image.stack;
    proc->thread.context.bp = proc->image.stack;
    proc->thread.context.ip = (uintptr_t)&enter_tasklet;

    // Setup scheduler values
    proc->waitQueue = list_create();
    proc->shm_mappings = list_create(); // Unused

    proc->schedulerNode.value = proc;
    proc->sleepNode.value = proc;

    // Timing value
    gettimeofday(&proc->start, NULL);
    tree_node_t* entry = tree_node_create(proc);
    proc->tree_entry = entry;

    // Final initialization
    spinlock_lock(&tree_lock);
    tree_node_insert_child_node(process_tree, currentProcess->tree_entry, entry);
    list_insert(process_list, (void*)proc);
    spinlock_release(&tree_lock);

    makeProcessReady(proc);
    serialPrintf("spawn_worker_thread: Successfully spawned '%s'\n", name);
    return proc;
}

pagedirectory_t* cloneDirectory(pagedirectory_t* in) {
    pagedirectory_t* out = pmm_allocateBlock();
    memcpy(out, in, sizeof(pagedirectory_t));
    return out;
}

// createProcess(char *filepath, int argc, char *argv[], char *envl[], int envc) - Creates a process from filepath, maps it into memory, and sets it up
int createProcess(char* filepath, int argc, char* argv[], char* envl[], int envc) {
    // First, get the file
    fsNode_t* file = open_file(filepath, 0);
    if (!file) {
        return -1; // Not found
    }

    char* buffer = kmalloc(file->length);
    uint32_t ret = file->read(file, 0, file->length, (uint8_t*)buffer);
    if (ret != file->length) {
        return -2; // Read error
    }

    // We basically have to take over ELF parsing.
    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)buffer;
    if (elf_isCompatible(ehdr)) {
        kfree(buffer);
        return -3; // Not compatible
    }
    if (ehdr->e_type != ET_EXEC) {
        kfree(buffer);
        return -3; // Not compatible
    }

    spinlock_lock(&switch_lock); // We're going to be greedy and lock it until we're done.
    currentProcess->thread.page_directory = cloneDirectory(vmm_getCurrentDirectory());
    currentProcess->thread.refcount = 1;
    spinlock_release(currentProcess->thread.pd_lock);
    vmm_switchDirectory(currentProcess->thread.page_directory);
    spinlock_release(&switch_lock);

    pagedirectory_t* addressSpace = currentProcess->thread.page_directory;

    // Now, we should start parsing.
    uintptr_t heapBase = 0;
    uintptr_t execBase = -1;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf32_Phdr* phdr = elf_getPHDR(ehdr, i);
        if (phdr->p_type == PT_LOAD) {
            // We have to load it into memory
            uint32_t memory_blocks = (phdr->p_memsize + 4096)
                                     - ((phdr->p_memsize + 4096) % 4096); // Round up, else page fault will occur.
            void* mem = pmm_allocateBlocks(memory_blocks / 4096);
            for (uint32_t blk = 0; blk < memory_blocks; blk += 4096) {
                vmm_mapPhysicalAddress(addressSpace, phdr->p_vaddr + blk, (uint32_t)mem + blk,
                                       PTE_PRESENT | PTE_WRITABLE | PTE_USER);
            }
            memcpy((void*)phdr->p_vaddr, buffer + phdr->p_offset, phdr->p_filesize);
        }

        if (phdr->p_vaddr < execBase) execBase = phdr->p_vaddr;
        if (phdr->p_vaddr + phdr->p_memsize > heapBase) heapBase = phdr->p_vaddr + phdr->p_memsize;
    }

    // Create a usermode stack
    // We'll use 0xC0000000 as the usermode stack. That should be far enough into memory that it will be fine.
    // This allocation is done via PMM mapped memory.

    uintptr_t usermode_stack = 0xC0000000;
    for (uintptr_t i = usermode_stack - 512 * 0x400; i < usermode_stack; i += 0x1000) {
        void* block = kmalloc(4096);
        vmm_allocateRegionFlags((uintptr_t)block, i, 0x1000, 1, 1, 1);
    }

    serialPrintf("usermode stack mapped from 0x%x to 0x%x\n", usermode_stack - 512 * 0x400, usermode_stack);

    currentProcess->image.userstack = usermode_stack - 16 * 0x400;

    // Setup values
    currentProcess->image.heap = heapBase;
    currentProcess->image.heap_start = currentProcess->image.heap;
    currentProcess->image.heap_end = currentProcess->image.heap;
    currentProcess->image.entrypoint = ehdr->e_entry;

    // Setup the process' stack

    // First part is to push the actual strings onto the stack.
    char* argv_ptrs[argc];
    for (int i = 0; i < argc; i++) {
        int l = strlen(argv[i]);
        do {
            PUSH(currentProcess->image.userstack, char, (char)(argv[i][l]));
            l--;
        } while (l >= 0);
        argv_ptrs[i] = (char*)currentProcess->image.userstack;
    }

    // Now we'll do the same thing with environments

    char* env_ptrs[envc];

    int true_envc = 0;

    for (int i = 0; i < envc; i++) {
        if (envl[i])
            true_envc++;
        else
            break;
        int l = strlen(envl[i]);
        do {
            PUSH(currentProcess->image.userstack, char, (char)(envl[i][l]));
            l--;
        } while (l >= 0);
        env_ptrs[i] = (char*)currentProcess->image.userstack;
    }

    // Next, we can push pointers to those strings we just pushed before.
    PUSH(currentProcess->image.userstack, int, 0);
    for (int i = argc; i > 0; i--) { PUSH(currentProcess->image.userstack, char*, argv_ptrs[i - 1]); }

    // Same for envp
    PUSH(currentProcess->image.userstack, int, 0);
    for (int i = true_envc; i > 0; i--) { PUSH(currentProcess->image.userstack, char*, env_ptrs[i - 1]); }

    PUSH(currentProcess->image.userstack, int, argc);
    PUSH(currentProcess->image.userstack, int, envc);

    // It is time for your execution
    setKernelStack(currentProcess->image.stack);
    start_process((uint32_t)currentProcess->image.userstack, ehdr->e_entry);

    return 0;
}

// process_addfd(process_t *proc, fsNode_t *node) - Creates a new file descriptor and return its ID
unsigned long process_addfd(process_t* proc, fsNode_t* node) {
    spinlock_lock(proc->file_descs->fd_lock);

    // Fill in the gaps
    for (unsigned long i = 0; i < proc->file_descs->length; i++) {
        if (!proc->file_descs->nodes[i]) {
            // There's a free node, yoink it
            proc->file_descs->nodes[i] = node;
            proc->file_descs->modes[i] = 0;
            proc->file_descs->fd_offsets[i] = 0;
            spinlock_release(proc->file_descs->fd_lock);
            return i;
        }
    }

    // :( no gaps in the list
    // Expand, expand, expand...
    if (proc->file_descs->length == proc->file_descs->max_fds) {
        proc->file_descs->max_fds *= 2;
        proc->file_descs->nodes = krealloc(proc->file_descs->nodes, sizeof(fsNode_t*) * proc->file_descs->max_fds);
        proc->file_descs->modes = krealloc(proc->file_descs->modes, sizeof(int) * proc->file_descs->max_fds);
        proc->file_descs->fd_offsets = krealloc(proc->file_descs->fd_offsets,
                                                sizeof(uint64_t) * proc->file_descs->max_fds);
    }

    proc->file_descs->nodes[proc->file_descs->length] = node;
    proc->file_descs->modes[proc->file_descs->length] = 0;
    proc->file_descs->fd_offsets[proc->file_descs->length] = 0;
    proc->file_descs->length++;
    spinlock_release(proc->file_descs->fd_lock);
    return proc->file_descs->length - 1;
}

// process_movefd(process_t *proc, long src, long dest) - Move a file descriptor around
long process_movefd(process_t* proc, long src, long dest) {
    if ((size_t)src >= proc->file_descs->length || (dest != -1 && (size_t)dest >= proc->file_descs->length))
        return -1; // Invalid parameters

    if (dest == -1) {
        // If dest is specified as -1, we just need to make a new fd
        dest = process_addfd(proc, NULL);
    }
    if (proc->file_descs->nodes[dest] != proc->file_descs->nodes[src]) {
        // Clone them over
        closeFilesystem(proc->file_descs->nodes[dest]);
        proc->file_descs->nodes[dest] = proc->file_descs->nodes[src];
        proc->file_descs->modes[dest] = proc->file_descs->modes[src];
        proc->file_descs->fd_offsets[dest] = proc->file_descs->fd_offsets[src];
        openFilesystem(proc->file_descs->nodes[dest], 0, 0);
    }

    return dest;
}
