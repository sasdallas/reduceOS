#include <sys/signal.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/cdefs.h>
#include <stdint.h>

#include <errno.h>
#ifdef errno
#undef errno
extern int errno;
#endif

typedef void (*sighandler_t)(int); // TODO: Probably don't define this here...


/**
 * @brief Request changed treatment for a specific signal
 * 
 * @param signum The signal number to raise
 * @param handler The handler to use
 * 
 * 
 * Newlib will provide us a library to use that "emulates" system calls, but we need kernel integration.
 * The kernel needs to be able to send signals and handle them.
 * P.S. This is the first function in reduceOS to use this fancy style!
 * 
 * @return The previous handler or SIG_ERR
 * 
 */
sighandler_t signal(int signum, sighandler_t handler) {
    return SYSCALL2(sighandler_t, SYS_SIGNAL, signum, handler);
}

/**
 * @brief Send a signal to a process
 * 
 * @param pid The process to send the signal to
 * @param sig The signal to send to the process
 * 
 * @return 0 on success, -1 on failure.
 */
int kill(int pid, int sig) {
    int ret = SYSCALL2(int, SYS_KILL, pid, sig);
    if (ret != 0) {
        errno = ret*ret;
        return -1;
    }

    return 0;
}
