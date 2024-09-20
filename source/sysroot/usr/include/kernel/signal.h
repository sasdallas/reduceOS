#ifndef KERNEL_SIGNAL_H
#define KERNEL_SIGNAL_H

#include <kernel/process.h>
void signal_init();
int signal_await(sigset_t awaited, int *sig);
int group_send_signal(pid_t group, int signal, int force_root);
void restore_from_signal_handler(registers_t *r);
void process_check_signals(registers_t *r);
int send_signal(pid_t process, int signal, int force_root);
int handle_signal(process_t *proc, int signum, registers_t *r);
void restore_from_signal_handler(registers_t *r);
#endif