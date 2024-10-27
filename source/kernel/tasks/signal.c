// ========================================================
// signal.c - Process signal driver
// ========================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

// Signal implementation: https://github.com/klange/toaruos/blob/591a60939ecce85d80416e2aec7b0a7c701e4f7a/kernel/sys/signal.c

#include <kernel/process.h>
#include <kernel/regs.h>
#include <kernel/serial.h>
#include <kernel/signal.h>
#include <kernel/syscall.h>
#include <libk_reduced/errno.h>
#include <libk_reduced/signal.h>
#include <libk_reduced/signal_defs.h>
#include <libk_reduced/spinlock.h>

static spinlock_t* sig_lock;

#define SIG_DISP_Ign  0
#define SIG_DISP_Term 1
#define SIG_DISP_Core 2
#define SIG_DISP_Stop 3
#define SIG_DISP_Cont 4

// How signals should be handled
#pragma GCC diagnostic ignored "-Woverride-init"
static char sig_defaults[] = {
    0,
    [SIGHUP] = SIG_DISP_Term,
    [SIGINT] = SIG_DISP_Term,
    [SIGQUIT] = SIG_DISP_Core,
    [SIGILL] = SIG_DISP_Core,
    [SIGTRAP] = SIG_DISP_Core,
    [SIGABRT] = SIG_DISP_Core,
    [SIGBUS] = SIG_DISP_Core,
    [SIGFPE] = SIG_DISP_Core,
    [SIGKILL] = SIG_DISP_Term,
    [SIGBUS] = SIG_DISP_Core,
    [SIGSEGV] = SIG_DISP_Core,
    [SIGSYS] = SIG_DISP_Core,
    [SIGPIPE] = SIG_DISP_Term,
    [SIGALRM] = SIG_DISP_Term,
    [SIGTERM] = SIG_DISP_Term,
    [SIGUSR1] = SIG_DISP_Term,
    [SIGUSR2] = SIG_DISP_Term,
    [SIGCHLD] = SIG_DISP_Ign,
    [SIGPWR] = SIG_DISP_Ign,
    [SIGWINCH] = SIG_DISP_Ign,
    [SIGTTOU] = SIG_DISP_Ign,
    [SIGPOLL] = SIG_DISP_Ign,
    [SIGSTOP] = SIG_DISP_Stop,
    [SIGTSTP] = SIG_DISP_Stop,
    [SIGCONT] = SIG_DISP_Cont,
    [SIGTTIN] = SIG_DISP_Stop,
    [SIGTTOUT] = SIG_DISP_Stop,
    [SIGTTOU] = SIG_DISP_Stop,
    [SIGVTALRM] = SIG_DISP_Term,
    [SIGPROF] = SIG_DISP_Term,
    [SIGXCPU] = SIG_DISP_Core,
    [SIGXFSZ] = SIG_DISP_Core,
};

#define shift_signal(signum) (1ULL << signum)

// signal_maybeRestartSyscall(registers_t *r, int signum) - If a system call returned -ERESTARTSYS
static void signal_maybeRestartSyscall(registers_t* r, int signum) {
    if (signum < 0 || signum >= NUMSIGNALS) {
        serialPrintf("signal: Invalid signal number %i\n", signum);
        return;
    }

    if (currentProcess->interrupted_syscall && (int)r->eax == -ERESTARTSYS) {
        if (sig_defaults[signum] == SIG_DISP_Cont || (currentProcess->signals[signum].flags & SA_RESTART)) {
            r->eax = currentProcess->interrupted_syscall;
            currentProcess->interrupted_syscall = 0;
            syscallHandler(r);
        } else {
            currentProcess->interrupted_syscall = 0;
            r->eax = -EINTR;
        }
    }
}

#define PENDING                                                                                                        \
    (currentProcess->pending_signals                                                                                   \
     & ((~currentProcess->blocked_signals) | shift_signal(SIGSTOP) | shift_signal(SIGKILL)))

#define PUSH(stack, type, item)                                                                                        \
    do {                                                                                                               \
        stack -= sizeof(type);                                                                                         \
        *((volatile type*)stack) = item;                                                                               \
    } while (0)

#define POP(stack, type, item)                                                                                         \
    do {                                                                                                               \
        item = *((volatile type*)stack);                                                                               \
        stack += sizeof(type);                                                                                         \
    } while (0)

// (ARCH-SPECIFIC) signal_handler(uintptr_t entrypoint, int signum, registers_t *r, uintptr_t stack) - The signal handler
void signal_handler(uintptr_t entrypoint, int signum, registers_t* r, uintptr_t stack) {
    registers_t ret;
    ret.cs = 0x28 | 0x03;
    ret.ss = 0x20 | 0x03;
    ret.eip = entrypoint;
    ret.eflags = (1 << 21) | (1 << 9);
    ret.esp = (r->esp - 128) & 0xFFFFFFF0;

    PUSH(ret.esp, registers_t, *r);
    PUSH(ret.esp, long, currentProcess->interrupted_syscall);

    currentProcess->interrupted_syscall = 0;
    PUSH(ret.esp, long, signum);
    PUSH(ret.esp, sigset_t, currentProcess->blocked_signals);

    struct signal_config* config = (struct signal_config*)&currentProcess->signals[signum];
    currentProcess->blocked_signals |= config->mask | (config->flags & SA_NODEFER ? 0 : (1UL << signum));

    /*asm volatile ("fxsave (%0)" :: "r"(&currentProcess->thread.fp_regs));
    for (int i = 0; i < 64; i++) {
        PUSH(ret.esp, uint64_t, currentProcess->thread.fp_regs[i]);
    }*/

    PUSH(ret.esp, uintptr_t, 0x516);

    updateProcessTimesOnExit();
    start_process(ret.esp, entrypoint);

    panic("signal", "signal_handler", "Failed to jump to signal handler");
}

// handle_signal(process_t *proc, int signum, registers_t *r) - Handle a signal received for a process
int handle_signal(process_t* proc, int signum, registers_t* r) {
    struct signal_config config = proc->signals[signum];

    // Check if the process is finished
    if (proc->flags & PROCESS_FLAG_FINISHED) return 1;

    if (signum == 0 || signum >= NUMSIGNALS) goto ignore_signal;

    if (!config.handler) {
        // No handler is configured, we're gonna have to do it ourselves
        char action = sig_defaults[signum];
        if (action == SIG_DISP_Term || action == SIG_DISP_Core) {
            // The action to take is to exit the task
            task_exit(((128 + signum) << 8) | signum);
            __builtin_unreachable();
        } else if (action == SIG_DISP_Stop) {
            __sync_or_and_fetch(&currentProcess->flags, PROCESS_FLAG_SUSPEND);
            currentProcess->status = 0x7F | (signum << 8) | 0xFF0000;
            process_t* parent = process_get_parent((process_t*)currentProcess);

            if (parent && !(parent->flags & PROCESS_FLAG_FINISHED)) { wakeup_queue(parent->waitQueue); }

            do { process_switchTask(0); } while (!PENDING);

            return 0; // Handle another
        } else if (action == SIG_DISP_Cont) {
            goto ignore_signal;
        }
        goto ignore_signal;
    }

    // If the value is 1 ignore the signal
    if (config.handler == 1) goto ignore_signal;

    if (config.flags & SA_RESETHAND) { proc->signals[signum].handler = 0; }

    uintptr_t stack;
    if (proc->syscall_registers->useresp < 0x10000100) {
        stack = proc->image.userstack;
    } else {
        stack = proc->syscall_registers->useresp;
    }

    serialPrintf("signal: Handling signal %i for process %d (%s) - handler 0x%x\n", signum, proc->id, proc->name,
                 config.handler);
    signal_handler(config.handler, signum, r, stack);

ignore_signal:
    serialPrintf("signal: Ignoring signal %i for process %d (%s)\n", signum, proc->id, proc->name);
    signal_maybeRestartSyscall(r, signum);
    return !PENDING;
}

// send_signal(pid_t process, int signal, int force_root) - Deliver a signal to another process
int send_signal(pid_t process, int signal, int force_root) {
    process_t* receiver = process_from_pid(process);

    if (!receiver) return -ESRCH;
    // TODO: Use force_root
    if (receiver->flags & PROCESS_FLAG_IS_TASKLET) return -EPERM;
    if (signal >= NUMSIGNALS || signal < 0) return -EINVAL;
    if (receiver->flags & PROCESS_FLAG_FINISHED) return -ESRCH;
    if (!signal) return 0;

    int awaited = receiver->awaited_signals & shift_signal(signal);
    int ignored = !receiver->signals[signal].handler && !sig_defaults[signal];
    int blocked = (receiver->blocked_signals & shift_signal(signal)) && signal != SIGKILL && signal != SIGSTOP;

    if (sig_defaults[signal] == SIG_DISP_Cont && (receiver->flags & PROCESS_FLAG_SUSPEND)) {
        __sync_and_and_fetch(&receiver->flags, ~(PROCESS_FLAG_SUSPEND));
        receiver->status = 0;
    }

    if (!awaited && !blocked && ignored) return 0;

    // Mark the signal for delivery
    spinlock_lock(sig_lock);
    receiver->pending_signals |= shift_signal(signal);
    spinlock_release(sig_lock);

    // If the signal is blocked and not being awaited, end.
    if (blocked && !awaited) return 0;

    // Inform any blocking events that the process has been interrupted.
    process_awaken_signal(receiver);

    if (receiver != currentProcess && !process_isReady(receiver)) { makeProcessReady(receiver); }

    serialPrintf("signal: Signal %i send to process %d (%s)\n", signal, process, receiver->name);

    

    return 0;
}

// process_check_signals(registers_t *r) - Examines the signal delivery queue and handles
void process_check_signals(registers_t* r) {

tryagain:
    spinlock_lock(sig_lock);
    if (currentProcess && !(currentProcess->flags & PROCESS_FLAG_FINISHED)) {
        sigset_t active_signals = PENDING;
        int signal = 0;

        while (active_signals && signal < NUMSIGNALS) {
            if (active_signals & 1) {
                currentProcess->pending_signals &= ~shift_signal(signal);
                spinlock_release(sig_lock);
                if (handle_signal((process_t*)currentProcess, signal, r)) return;
                goto tryagain;
            }
            active_signals >>= 1;
            signal++;
        }
    }

    spinlock_release(sig_lock);
}

// (ARCH-SPECIFIC) arch_return_from_signal_handler(registers_t *r) - Return from a signal handler
int arch_return_from_signal_handler(registers_t* r) {
    // Start by restoring the FPU values
    /*for (int i = 0; i < 64; i++) {
        POP(r->esp, uint64_t, currentProcess->thread.fp_regs[63-i]);
    }

    asm volatile ("fxrstor (%0)" :: "r"(&currentProcess->thread.fp_regs));*/

    // Next, restore signal information
    POP(r->esp, sigset_t, currentProcess->blocked_signals);
    long originalSignal;
    POP(r->esp, long, originalSignal);

    // Restore the system call
    POP(r->esp, long, currentProcess->interrupted_syscall);

    // Now do the registers
#define R(n) r->n = out.n

    registers_t out;
    POP(r->esp, registers_t, out);

    R(ds);
    R(edi);
    R(esi);
    R(ebp);
    R(esp);
    R(ebx);
    R(edx);
    R(ecx);
    R(eax);
    R(eip);
    R(esp);

    r->eflags = (out.eflags & 0xCD5) | (1 << 21) | (1 << 9) | ((r->eflags & (1 << 8)) ? (1 << 8) : 0);
    return originalSignal;
}

// restore_from_signal_handler(registers_t *r) - Restores the pre-signal context
void restore_from_signal_handler(registers_t* r) {
    int signum = arch_return_from_signal_handler(r);
    if (PENDING) { process_check_signals(r); }

    signal_maybeRestartSyscall(r, signum);
}

extern list_t* process_list;

// group_send_signal(pid_t group, int signal, int force_root) - Send a signal to multiple proceses
int group_send_signal(pid_t group, int signal, int force_root) {
    int kill_self = 0;
    int killed_something = 0;

    if (signal < 0) return 0;

    foreach (node, process_list) {
        process_t* proc = node->value;
        if (proc->group == proc->id && proc->job == group) {
            // Only do it to thread group leaders
            if (proc->group == currentProcess->group) {
                kill_self = 1;
            } else {
                if (send_signal(proc->group, signal, force_root) == 0) { killed_something = 1; }
            }
        }
    }

    if (kill_self) {
        if (send_signal(currentProcess->group, signal, force_root)) { killed_something = 1; }
    }

    return !!killed_something;
}

// signal_await(sigset_t awaited, int *sig) - Synchronously wait for specified signals to become pending
int signal_await(sigset_t awaited, int* sig) {
    do {
        sigset_t maybe = awaited & currentProcess->pending_signals;
        if (maybe) {
            int signal = 0;
            while (maybe && signal < NUMSIGNALS) {
                if (maybe & 1) {
                    spinlock_lock(sig_lock);
                    currentProcess->pending_signals &= ~shift_signal(signal);
                    *sig = signal;
                    spinlock_release(sig_lock);
                    return 0;
                }
                maybe >>= 1;
                signal++;
            }
        }

        // Set awaited signals
        currentProcess->awaited_signals = awaited;
        // Sleep
        process_switchTask(0);

        // Unset awaited signals
        currentProcess->awaited_signals = 0;
    } while (!PENDING);

    return -EINTR;
}

// signal_init() - Initialize the signals system
void signal_init() { sig_lock = spinlock_init(); }