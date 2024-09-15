// signal_defs.h - Provides definitions for the signals

#ifndef SIGNAL_DEFS_H
#define SIGNAL_DEFS_H

#define NUMSIGNALS 32

#define SIGHUP     1  // Hang up controlling process
#define SIGINT     2  // Interrupt from keyboard
#define SIGQUIT    3  // Quit from keyboard
#define SIGILL     4  // Illegal instruction
#define SIGTRAP    5  // Breakpoint for debugging
#define SIGABRT    6  // Abnormal termination
#define SIGIOT     6  // ^^^^
#define SIGBUS     7  // Bus error
#define SIGFPE     8  // Floating-point exception
#define SIGKILL    9  // Forced process termination
#define SIGUSR1    10 // Available to processes
#define SIGSEGV    11 // Invalid memory reference
#define SIGUSR2    12 // Available to processes
#define SIGPIPE    13 // Write to pipe with no readers
#define SIGALRM    14 // Wakey wakey
#define SIGTERM    15 // Process termination
#define SIGSTKFLT  16 // Coprocessor stack error
#define SIGCHLD    17 // Child process stopped/terminated
#define SIGCONT    18 // Continue
#define SIGSTOP    19 // Stop right there, criminal scum! (Ctrl-Z pause)
#define SIGTSTP    20 // Stop process from tty
#define SIGTTIN    21 // Background process requires input
#define SIGTTOUT   22 // Background process requires output
#define SIGTTOU    23 // Urgent condition on socket
#define SIGXCPU    24 // CPU time limit exceeded
#define SIGXFSZ    25 // File size limit exceeded
#define SIGVTALRM  26 // Virtual timer clock
#define SIGPROF    27 // Profile timer clock
#define SIGWINCH   28 // Window resizing
#define SIGIO      29 // I/O now possible
#define SIGPOLL    29 // ^^^^
#define SIGPWR     30 // Power supply failure
#define SIGSYS     31 // Bad system call
#define SIGUNUSED  31 // ^^^^

#endif