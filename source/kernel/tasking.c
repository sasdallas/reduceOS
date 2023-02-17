// ==========================================================
// tasking.c - Handles multitasking and more.
// ==========================================================
// Original implementation created by RWTH-OS in eduOS.

#include "include/tasking.h" // Main header file.

// Variables...
// Assume 16 for max tasks.
task_t taskTable[16] = { \
		[0]                 = {0, TASK_IDLE, NULL, NULL, 0, NULL, NULL}, \
		[1 ... 16-1] = {0, TASK_INVALID, NULL, NULL, 0, NULL, NULL}};

static spinlock_irqsave_t tableLock = SPINLOCK_IRQSAVE_INIT;
static readyQueues_t readyQueues = {taskTable+0, NULL, 0, 0, {[0 ... MAX_PRIORITY-2] = {NULL, NULL}}, SPINLOCK_IRQSAVE_INIT};
task_t *currentTask = taskTable+0; 

extern const void stack_bottom; // Stack, defined in boot.S




// Function prototypes
int createKernelTask(tid_t id, entryPoint_t ep, void *args, uint8_t priority); // Create a kernel task.
int task_blockTask(); // Block the current task.
int task_wakeupTask(tid_t id); // Wakeup a blocked task
int task_createDefaultFrame(task_t *task, entryPoint_t ep, void *arg); // Creates a default frame for a new task.
void task_leaveKernelTask(); // Leave the kernel task (all tasks call this on completion).
void task_abort(); // Abort the current task.
void initMultitasking(); // Initializes multitasking by setting up the task table and registering an idle task.
void task_finishTaskSwitch(); // Finish a task switch
size_t **task_scheduler(); // The task scheduler.
void task_reschedule(); // Reschedule a task.
task_t *task_getCurrentTask(); // Get the current task.
uint32_t task_getHighestPriority(); // Returns the highest priority
size_t *task_getCurrentStack(void); // Get the current stack of whatever task is currently running.

// Begin with some of the static functions, used with inline assembly.

// (static) task_registerTask(void) - Registers a task's TSS at GDT.
static int task_registerTask(void) {
    uint16_t sel = 5 << 3;
    asm volatile ("ltr %%ax" :: "a"(sel));
    return 0;
}


// (STATIC, NO RETURN) task_exit(int arg) - A method to lower code size. Always called on a task's exit method.
static void task_exit(int arg) {
    task_t* curr_task = currentTask;
    serialPrintf("Task %u terminated with return value %d\n", curr_task->id, arg);

    // Decrease number of active tasks.
    spinlock_irqsave_lock(&readyQueues.lock);
    readyQueues.numTasks--;
    spinlock_irqsave_unlock(&readyQueues.lock);

    curr_task->taskStatus = TASK_FINISHED;
    task_reschedule();

    panic("task_scheduler", "task_exit", "panic: scheduler found no valid task.");
}

// (static) task_createTask(tid_t *id, entryPoint_t ep, void *arg, uint8_t priority) - Create a task. Note that this is like kmalloc, so when we want to implement user mode tasks, we can.
static int task_createTask(tid_t *id, entryPoint_t ep, void *arg, uint8_t priority) {
    int ret = -12;
    uint32_t i;

    // Sanity checks first.
    if (!ep) return -22;
    if (priority == IDLE_PRIORITY) return -22;
    if (priority > MAX_PRIORITY) return -22;

    // Lock the spinlock.
    spinlock_irqsave_lock(&tableLock);

    // Create the task (16 is max tasks as of now)
    for (i = 0; i < 16; i++) {
        if (taskTable[i].taskStatus == TASK_INVALID) {
            taskTable[i].id = i;
            taskTable[i].taskStatus = TASK_READY;
            taskTable[i].lastStackPointer = NULL;
            taskTable[i].stackStart = createStack(i);
            taskTable[i].taskPriority = priority;
            if (id) *id = i;

            ret = task_createDefaultFrame(taskTable+i, ep, arg);

            // Add task in the readyqueues.
            spinlock_irqsave_lock(&readyQueues.lock);
            readyQueues.priorityBitmap |= (1 << priority);
            readyQueues.numTasks++;

            if (!readyQueues.queue[priority-1].first) {
                taskTable[i].next = taskTable[i].prev = NULL;
                readyQueues.queue[priority-1].first = taskTable+i;
                readyQueues.queue[priority-1].last = taskTable+i;
            } else {
                taskTable[i].prev = readyQueues.queue[priority-1].last;
                taskTable[i].next = NULL;
                readyQueues.queue[priority-1].last->next = taskTable+i;
                readyQueues.queue[priority-1].last = taskTable+i;
            }

            spinlock_irqsave_unlock(&readyQueues.lock);
            break;
        }
    }
    
    spinlock_irqsave_unlock(&tableLock);
    return ret;
}

// Now, moving on to the ACTUAL functions...

// createKernelTask(tid_t id, entryPoint_t ep, void *args, uint8_t priority) - Create a kernel task.
int createKernelTask(tid_t id, entryPoint_t ep, void *args, uint8_t priority) {
    if (priority > MAX_PRIORITY) priority = NORMAL_PRIORITY;

    return task_createTask(id, ep, args, priority);
}


// task_wakeupTask(tid_t id) - Wakeup a blocked task (block method below.)
int task_wakeupTask(tid_t id) {
    task_t *task;
    uint32_t priority;
    int ret = -22;
    uint8_t flags;
    
    
    // Disable nested IRQs
    size_t tmp;
    asm volatile ("pushf; cli; pop %0" : "=r"(tmp) :: "memory");
    if (tmp & (1 << 9)) flags = 1;
    else flags = 0;

    task = taskTable + id;
    priority = task->taskPriority;

    if (task->taskStatus == TASK_BLOCKED) {
        task->taskStatus = TASK_READY;
        ret = 0;

        spinlock_irqsave_lock(&readyQueues.lock);
        readyQueues.numTasks++;

        // Add tasks to the runqueue.
        if (!readyQueues.queue[priority-1].last) {
            readyQueues.queue[priority-1].last = readyQueues.queue[priority-1].first = task;
            task->next = task->prev = NULL;
            readyQueues.priorityBitmap |= (1 << priority);
        } else {
            task->prev = readyQueues.queue[priority-1].last;
            task->next = NULL;
            readyQueues.queue[priority-1].last->next = task;
            readyQueues.queue[priority-1].last = task;
        }

        spinlock_irqsave_unlock(&readyQueues.lock);
    }   

    // Enable nested IRQs
    if (flags) asm volatile ("sti" ::: "memory");

    // Return
    return ret;
}


// task_blockTask() - Block the current task.
int task_blockTask() {
    tid_t id;
    uint32_t priority;
    int ret = -22;
    uint8_t flags;

    // Disable nested IRQs
    size_t tmp;
    asm volatile ("pushf; cli; pop %0" : "=r"(tmp) :: "memory");
    if (tmp & (1 << 9)) flags = 1;
    else flags = 0;

    id = currentTask->id;
    priority = currentTask->taskPriority;

    if (taskTable[id].taskStatus == TASK_RUNNING) {
        taskTable[id].taskStatus = TASK_BLOCKED;
        ret = 0;

        spinlock_irqsave_lock(&readyQueues.lock);

        // Reduce number of ready tasks.
        readyQueues.numTasks--;

        // Remove task from queue.
        if (taskTable[id].prev) taskTable[id].prev->next = taskTable[id].next;
        if (taskTable[id].next) taskTable[id].next->prev = taskTable[id].prev;

        if (readyQueues.queue[priority-1].first == taskTable+id) readyQueues.queue[priority-1].first = taskTable[id].next;
        if (readyQueues.queue[priority-1].last == taskTable+id) {
            readyQueues.queue[priority-1].last = taskTable[id].prev;
            if (!readyQueues.queue[priority-1].last) readyQueues.queue[priority-1].last = readyQueues.queue[priority-1].first;
        }

        // No valid task in queue, update priority bitmap.
        if (!readyQueues.queue[priority-1].first) readyQueues.priorityBitmap &= ~(1 << priority);
        spinlock_irqsave_unlock(&readyQueues.lock);
    }

    // Enable nested IRQs
    if (flags) asm volatile ("sti" ::: "memory");

    // Return
    return ret;
} 



// task_createDefaultFrame(task_t *task, entryPoint_t ep, void *arg) - Creates a default frame for a new task.
int task_createDefaultFrame(task_t *task, entryPoint_t ep, void *arg) {
    size_t *stack;
    struct REGISTERS_MULTITASK *stackPtr;
    size_t stateSize;
    
    if (!task) return -22; // bruh
    if (!task->stackStart) return -22; // double bruh

    memset(task->stackStart, 0xCD, 16384); // 16384 - 16 KB worth of stack

    // We are setting up a stack, not a TSS (the stack which will be activated and popped off with an iret instruction)
    stack = (size_t*) (task->stackStart + 16384 - 16); // Stack is always 16-byte aligned.

    *stack-- = 0xDEADBEEF; // Debugging purposes.

    *stack-- = (size_t)arg; // The first function to be called's arguments.
    *stack = (size_t)task_leaveKernelTask; // Exit method.

    stateSize = sizeof(struct REGISTERS_MULTITASK) - 2*sizeof(size_t);
    stack = (size_t*)((size_t)stack - stateSize);

    stackPtr = (struct REGISTERS_MULTITASK*)stack;
    memset(stackPtr, 0x00, stateSize);
    stackPtr->esp = (size_t)stack + stateSize;

    // eduOS easter egg right here, too bad I'm not including it - i'll change it tho ;).
    stackPtr->int_no = 0x1337C0D3;
    stackPtr->err_code = 0x5A5DC0D3;

    // Instruction pointer should be set on the first function to be called.
    stackPtr->eip = (size_t)ep;
    stackPtr->cs = 0x08;
    stackPtr->ds = stackPtr->es = 0x10;
    stackPtr->eflags = 0x1202;

    // The creation of kernel tasks don't need the IOPL level, so useresp and ss isn't required.
    task->lastStackPointer = (size_t*)stack;

    return 0;
}



// (NO RETURN) task_leaveKernelTask() - Leave the kernel task (all tasks call this on completion).
void task_leaveKernelTask() {
    int result = 0;
    result = 0; // todo: implement a task_getReturnValue
    task_exit(result);
}

// (NO RETURN) task_abort() - Abort the current task.
void task_abort() {
    task_exit(-1);
}




// initMultitasking() - Initializes multitasking by setting up the task table and registering an idle task.
void initMultitasking() {
    ASSERT(taskTable[0].taskStatus == TASK_IDLE, "initMultitasking", "Task 0 is not an idle task!");

    taskTable[0].taskPriority = IDLE_PRIORITY;
    taskTable[0].stackStart = (void*)&stack_bottom;
    
    return 0;
}

// task_finishTaskSwitch() - Finish a task switch
void task_finishTaskSwitch() {
    task_t *old;
    uint8_t priority;

    spinlock_irqsave_lock(&readyQueues.lock);

    if ((old = readyQueues.oldTask) != NULL) {
        if (old->taskStatus == TASK_INVALID) {
            old->stackStart = NULL;
            old->lastStackPointer = NULL;
            readyQueues.oldTask = NULL;
        } else {
            priority = old->taskPriority;
            if (!readyQueues.queue[priority-1].first) {
                old->next = old->prev = NULL;
                readyQueues.queue[priority-1].first = readyQueues.queue[priority-1].last = old;
            } else {
                old->next = NULL;
                old->prev = readyQueues.queue[priority-1].last;
                readyQueues.queue[priority-1].last->next = old;
                readyQueues.queue[priority-1].last = old;
            }
            readyQueues.oldTask = NULL;
            readyQueues.priorityBitmap |= (1 << priority);
        }
    }

    spinlock_irqsave_unlock(&readyQueues.lock);

    
}


// task_scheduler() - The task scheduler.
size_t **task_scheduler() {
    task_t *originalTask;
    uint32_t priority;

    originalTask = currentTask;

    spinlock_irqsave_lock(&readyQueues.lock);

    // TASK_FINISHED signifies that this task could be reused.
    if (currentTask->taskStatus == TASK_FINISHED) {
        currentTask->taskStatus = TASK_INVALID;
        readyQueues.oldTask = currentTask;
    } else readyQueues.oldTask = NULL; // Reset old task.


    priority = msb(readyQueues.priorityBitmap); // Get highest priority.
    if (priority > MAX_PRIORITY) {
        if ((currentTask->taskStatus == TASK_RUNNING) || (currentTask->taskStatus == TASK_IDLE)) {
            goto getTaskOut;
        }
        currentTask = readyQueues.idle;
    } else {
        // Does the current task have a higher priority? If so, don't switch task.
        if ((currentTask->taskPriority > priority) && (currentTask->taskStatus == TASK_RUNNING)) goto getTaskOut;
        if (currentTask->taskStatus == TASK_RUNNING) {
            currentTask->taskStatus = TASK_READY;
            readyQueues.oldTask = currentTask;
        }


        currentTask = readyQueues.queue[priority-1].first;
        if (currentTask->taskStatus == TASK_INVALID) {
            serialPrintf("task_scheduler: Got invalid task %d, orig task %d!\n");
            asm volatile ("hlt"); // Hold for now, maybe change this.
        }

        currentTask->taskStatus = TASK_RUNNING;

        // Remove new task from queue.
        readyQueues.queue[priority-1].first = currentTask->next;
        if (!currentTask->next) {
            readyQueues.queue[priority-1].last = NULL;
            readyQueues.priorityBitmap &= ~(1 << priority);
        }

        currentTask->next = currentTask->prev = NULL;
    }

getTaskOut:
    spinlock_irqsave_unlock(&readyQueues.lock);

    if (currentTask != originalTask) {
        serialPrintf("task_scheduler: Schedule from %u to %u with priority %u\n", originalTask->id, currentTask->id, (uint32_t)currentTask->taskPriority);
        return (size_t**) &(originalTask->lastStackPointer);
    }
    return 0;
}

// task_reschedule() - Reschedule a task.
void task_reschedule() {
    size_t **stack;
    uint8_t flags;
    
    // Disable nested IRQs
    size_t tmp;
    asm volatile ("pushf; cli; pop %0" : "=r"(tmp) :: "memory");
    if (tmp & (1 << 9)) flags = 1;
    else flags = 0;

    if ((stack = task_scheduler())) task_switchContext(stack);

    // Enable nested IRQs
    asm volatile ("sti" ::: "memory");
} 

// task_getCurrentTask() - Returns the current task.
task_t *task_getCurrentTask() { return currentTask; }

// task_getHighestPriority() - Returns the highest priority
uint32_t task_getHighestPriority() { return msb(readyQueues.priorityBitmap); }

// task_getCurrentStack(void) - Get the current stack of whatever task is currently running
size_t *task_getCurrentStack(void) {
    task_t *currentTask = currentTask;
    return currentTask->lastStackPointer;
}