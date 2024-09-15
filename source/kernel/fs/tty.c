/**
 * @file source/kernel/fs/tty.c
 * @brief reduceOS PTY (psuedo-teletype) and TTY filesystem driver.
 * 
 * This file will handle creating primary and slave PTY/TTY devices.
 * This page is a great resource for understanding what all of that means: http://www.linusakesson.net/programming/tty/index.php
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include <kernel/hashmap.h>
#include <kernel/mem.h>
#include <kernel/process.h>
#include <kernel/ringbuffer.h>
#include <kernel/signal.h>
#include <kernel/syscall.h>
#include <kernel/ttydev.h>
#include <kernel/vfs.h>
#include <libk_reduced/errno.h>
#include <libk_reduced/stdio.h>
#include <libk_reduced/termios.h>

/*
    First, what is a PTY/TTY?
    The difference between a PTY and a TTY isn't much:
    A PTY is a psuedo-teletype, which acts like a terminal but sends its output to another process.
    A TTY is an actual terminal
    
    The TTY is similar to the parent. Here's a good analogy by caf on StackOverflow:
    "For example, when you ssh in to a machine and run ls, the ls command is sending its output to a pseudo-terminal, 
    the other side of which is attached to the SSH daemon."

    PTY devices will handle serving signals based on keystrokes, handling child process input, etc.
    These devices are created on VFS nodes (described later), and serve as the main terminal "emulators"
*/

// PTY devices themselves are created in the /device/pts directory.
// TTY devices are different and instead rely on creating character devices in /device/ttyX, where X is the character number.
// /dev/tty0 is reserved for the kernel to output debug messages on.
// /dev/tty is a symlink to the current process' PTY
// ioctl() can be usesd to determine if something is a TTY, and readlink witll search the current process's fds and find one.

// Filesystem nodes
static fsNode_t* pty_dir;
static fsNode_t* tty_dev;

// Other variables
static hashmap_t* pty_hashmap; // Hashmap of all PTY devices
static int pty_idx = 0;        // The current index of the PTY

// Helper macros
#define IFLAG(flag) (pty->tios.c_iflag & flag)
#define OFLAG(flag) (pty->tios.c_oflag & flag)
#define CFLAG(flag) (pty->tios.c_cflag & flag)
#define LFLAG(flag) (pty->tios.c_lflag & flag)

#define GETCC(flag) (pty->tios.c_cc[flag])
#define ISCC(flag)  (c == pty->tios.c_cc[flag])

#define ISCTRL(c)   (c < ' ' || c == 0x7F)

#define MIN(a, b)   ((a > b) ? b : a)

// Function prototypes
static void tty_writeOutput(pty_t* pty, char c);

/**
 * @brief Check if something is a tty based off of ioctl call
 * This function's implementation is clever, and I have to quote that it is ToaruOS.
 * Using ioctl(), we can check if something is a tty. 
 * 
 * @returns 1 if the device is a tty, 0 if it is not.
 */
int isatty(fsNode_t* node) {
    if (!node || !node->ioctl) return 0;
    return ioctlFilesystem(node, IOCTLDTYPE, NULL) == IOCTL_DTYPE_TTY;
}

/**
 * @brief PTY ringbuffer write out method
 */
static void pty_ringbuffer_write_output(pty_t* pty, uint8_t c) { ringbuffer_write(pty->out, 1, &c); }

/**
 * @brief PTY ringbuffer write in method
 */
static void pty_ringbuffer_write_in(pty_t* pty, uint8_t c) { ringbuffer_write(pty->in, 1, &c); }

/**
 * @brief TTY backspace
 * @warning Recursion
 */
static void tty_backspace(pty_t* pty, int doerase, int howfar) {
    if (howfar == 0) return;

    if (pty->canon_buflen) {
        // Check how far to backspace. We do this because of control characters of width 2.
        int backspace = 1;
        if (ISCTRL(pty->canon_buffer[pty->canon_buflen])) {
            // Control character, we need to delete two.
            backspace = 2;
        }

        pty->canon_buflen--;
        pty->canon_buffer[pty->canon_buflen] = 0; // CTRL character is just of width 1 in the buffer.

        // If we actually want to erase the characters and echo is set, then write the erase sequence.
        if (LFLAG(ECHO) && doerase) {
            for (int i = 0; i < backspace; i++) {
                tty_writeOutput(pty, '\010'); // Go back
                tty_writeOutput(pty, ' ');    // Write space
                tty_writeOutput(pty, '\010'); // Go back again
            }
        }
    }

    tty_backspace(pty, doerase, howfar--);
}

/**
 * @brief PTY slave write method
 */
uint32_t pty_slave_write(fsNode_t* slave, off_t offset, uint32_t size, uint8_t* buffer) {
    // This is just a complicated write to the PTY output buffer.
    pty_t* pty = (pty_t*)slave->impl_struct;
    if (!pty) return 0;

    // If TOSTOP is specified, we should check whether a background process is trying to write to the terminal.
    if (LFLAG(TOSTOP)) {
        // Check whether the current process is the job of the foreground process, and whther it's part of the session..
        if (currentProcess->job != pty->fg_proc && pty->fg_proc && currentProcess->session == pty->ct_proc) {
            // However, it's possible that the process is blocking the signal.
            if (!(currentProcess->blocked_signals & (1ULL << SIGTTOU)) && !(currentProcess->signals[SIGTTOU].handler)) {
                // Send the signal to the group.
                group_send_signal(currentProcess->group, SIGTTOU, 1);
                return -ERESTARTSYS;
            }
        }
    }

    // Just write to the output buffer
    for (size_t i = 0; i < size; i++) { tty_writeOutput(pty, buffer[i]); }

    return size; // Not good?
}

/**
 * @brief Write out to the output buffer
 */
static void tty_writeOutput(pty_t* pty, char c) {
    // Some programs just want the input raw, then OPOST won't be specified.
    if (!pty->write_out) return;
    if (!OFLAG(OPOST)) {
        // Just write out the character.
        pty->write_out(pty, c);
        return;
    }

    // OLCUC: Map lowercase characters to uppercase on output
    if (OFLAG(OLCUC)) { c = toupper(c); }

    // ONLCR: Map NL to CRNL on output.
    if (OFLAG(ONLCR) && c == '\n') { pty->write_out(pty, '\r'); }

    // ONOCR: Don't output CR at column 0.
    if (OFLAG(ONOCR)) {
        // Unimplemented
    }

    pty->write_out(pty, c);
}

/**
 * @brief Write out to the input buffer.
 */
static void tty_writeInput(pty_t* pty, char c) {
    // We need to first handle the character being verbatim.
    if (pty->next_is_verbatim) {
        pty->next_is_verbatim = 0; // Should only be set when in ICANON mode, so we can assume that.

        // Add the character to the canon buffer length.
        if (pty->canon_buflen < pty->canon_bufsize) {
            pty->canon_buffer[pty->canon_buflen] = c;
            pty->canon_buflen++;
        }

        // If it's set to ECHO, then send it to the process.
        if (LFLAG(ECHO)) {
            // If it's a control character, then write it as ^C
            if (ISCTRL(c)) {
                tty_writeOutput(pty, '^');
                tty_writeOutput(pty, ('@' + c) % 128);
            } else {
                tty_writeOutput(pty, c);
            }
        }

        return;
    }

    // Before we print anything to the screen, handle ISIG
    if (LFLAG(ISIG)) {
        int sig = -1;
        if (ISCC(VINTR)) sig = SIGINT;
        if (ISCC(VQUIT)) sig = SIGQUIT;
        if (ISCC(VSUSP)) sig = SIGTSTP;

        if (sig != -1) {
            // We received a character, we need to print it, clear the input buffer, and send the signal.
            if (LFLAG(ECHO)) {
                // Echo the character out
                tty_writeOutput(pty, '^');
                tty_writeOutput(pty, ('@' + c) % 128);
            }

            // Clear the input buffer by sneakily setting first character to 0, and changing length.
            pty->canon_buffer[0] = 0;
            pty->canon_buflen = 0;

            if (pty->fg_proc) group_send_signal(pty->fg_proc, sig, 1);

            return;
        }
    }

    // BRKINT, IGNBRK, PARMRK: Ignored because termios should maybe potentially handle this.
    // INPCK: Input parity checking. Unimplemented.

    // ISTRIP: Strip off the eighth bit.
    if (IFLAG(ISTRIP)) { c &= ~(1 << 7); }

    // IGNCR: Ignore carriage return on input.
    if (IFLAG(IGNCR) && c == '\r') { return; }

    // INLCR: Translate NL to CR.
    if (IFLAG(INLCR) && c == '\n') { c = '\r'; }

    // ICRNL: Translate CR to NL
    if (IFLAG(ICRNL) && c == '\r') { c = '\n'; }

    // IUCLC: Map uppercase characters to lowercase (not in POSIX)
    if (IFLAG(IUCLC)) { c = toupper(c); }

    // IXON: Enable XON/XOFF flow control on output. Unimplemented.

    // IXOFF: Enable XON/XOFF flow control on input. Unimplemented.

    // IXANY: (XSI) Typing any character will restart stopped output. Unimplemented.

    // IMAXBEL: ring ring (Unimplemented)

    // IUTF8: Input is UTF-8, CE to be correctly interpreted in cooked mode. Unimplemented

    // ICANON: Canonical mode.
    if (LFLAG(ICANON)) {
        // VLNEXT: Literal next, quotes the next input character.
        // IEXTEN: Enables special input processing. Recognizes special characters.
        if (ISCC(VLNEXT) && LFLAG(IEXTEN)) {
            // Deprived of meaning. Tell our next write that it is a verbatim character.
            pty->next_is_verbatim = 1;
            tty_writeOutput(pty, '^');
            tty_writeOutput(pty, '\010');
            return;
        }

        // VEOF: End-of-file character. Causes the pending tty buffer to be sent without waiting for EOL.
        if (ISCC(VEOF)) {
            // If there is anything in the buffer, dump it, else just do EOF.
            if (pty->canon_buflen) {
                char* current_char = pty->canon_buffer;
                while (pty->canon_buflen > 0) {
                    pty->write_in(pty, *current_char);
                    current_char++;
                    pty->canon_buflen--;
                }
            } else {
                // Signify ringbuffer EOF
                ringbuffer_eof(pty->in);
            }

            return;
        }

        // VEOL: End-of-line character. Normally interpreted as \n, but is also checked as VEOL
        if (c == '\n' || ISCC(VEOL)) {
            // If ECHONL is set, echo the newline character.
            if (LFLAG(ECHONL)) { tty_writeOutput(pty, c); }

            pty->canon_buffer[pty->canon_buflen - 1] = c;
            char* current_char = pty->canon_buffer;
            while (pty->canon_buflen > 0) {
                pty->write_in(pty, *current_char);
                current_char++;
                pty->canon_buflen--;
            }

            return;
        }

        // VERASE: Erase last character
        if (ISCC(VERASE)) {
            // Backspace the character, and print the erase character if ECHOE isn't specified.
            tty_backspace(pty, LFLAG(ECHOE), 1);
            if (LFLAG(ECHO) && !LFLAG(ECHOE)) {
                tty_writeOutput(pty, '^');
                tty_writeOutput(pty, ('@' + c) % 128);
            }
            return;
        }

        // VKILL: Erase input since last EOF/beginning of line.
        if (ISCC(VKILL)) {
            tty_backspace(pty, LFLAG(ECHOK), pty->canon_buflen);
            if (LFLAG(ECHO) && !LFLAG(ECHOK)) {
                tty_writeOutput(pty, '^');
                tty_writeOutput(pty, ('@' + c) % 128);
            }
        }

        // VREPRINT: Unhandled for now.

        // VWERASE: Word erase
        if (ISCC(VWERASE) && LFLAG(IEXTEN)) {
            // User may have put spaces after, account for that
            while (pty->canon_buflen && pty->canon_buffer[pty->canon_buflen - 1] == ' ') {
                tty_backspace(pty, LFLAG(ECHOE), 1);
            }

            // Now just erase the word
            while (pty->canon_buflen && pty->canon_buffer[pty->canon_buflen - 1] != ' ') {
                tty_backspace(pty, LFLAG(ECHOE), 1);
            }

            // If ECHO was specified and ECHOE was not, print out the character.
            if (LFLAG(ECHO) && !LFLAG(ECHOE)) {
                tty_writeOutput(pty, '^');
                tty_writeOutput(pty, ('@' + c) % 128);
            }
            return;
        }

        // Add the character to the buffer, and if ECHO is specified, print it out.
        // The buffer might not have been flushed yet, though. (TODO: Is discarding ok?)
        if (pty->canon_buflen < pty->canon_bufsize) {
            pty->canon_buffer[pty->canon_buflen] = c;
            pty->canon_buflen++;
        }

        // Echo the character if specified.
        if (LFLAG(ECHO)) {
            if (ISCTRL(c) && c != '\n') {
                // This \n thing is a hack, we shouldn't do it.
                tty_writeOutput(pty, '^');
                tty_writeOutput(pty, ('@' + c) % 128);
            } else {
                tty_writeOutput(pty, c);
            }
        }

        return;
    } else if (LFLAG(ECHO)) {
        tty_writeOutput(pty, c);
    }

    if (pty->write_in) pty->write_in(pty, c);
}

/**
 * @brief PTY master write method
 */
uint32_t pty_master_write(fsNode_t* master, off_t offset, uint32_t size, uint8_t* buffer) {
    // Basically just hand it off to tty_writeInput
    pty_t* pty = (pty_t*)master->impl_struct;
    if (!pty) return 0;

    for (size_t i = 0; i < size; i++) { tty_writeInput(pty, buffer[i]); }

    return size; // This might not be a good idea.
}

/**
 * @brief PTY slave read method 
 */
uint32_t pty_slave_read(fsNode_t* slave, off_t offset, uint32_t size, uint8_t* buffer) {
    // Just a complicated read to the pty->out buffer.

    pty_t* pty = (pty_t*)slave->impl_struct;
    if (!pty) return 0;

    // Reading is different in that there isn't a flag to specify, we will just force it.
    if (pty->ct_proc == currentProcess->session && pty->fg_proc && currentProcess->job != pty->fg_proc) {
        // Send SIGTTIN if it's not being ignored
        if (!(currentProcess->blocked_signals & (1ULL << SIGTTOU)) && !(currentProcess->signals[SIGTTOU].handler)) {
            group_send_signal(currentProcess->group, SIGTTIN, 1);
            return -ERESTARTSYS;
        } else {
            return -EIO;
        }
    }

    // Canonical mode (read more at the man page for termios)
    if (LFLAG(ICANON)) {
        // Simple direct read
        return ringbuffer_read(pty->in, size, buffer);
    } else {
        // Noncanonical mode is more complicated.
        // Let's check some control flags.

        // There are four possible situations:
        // MIN == 0  TIME == 0 - Read returns immediately with either how many bytes are available, or how many specified.
        // MIN  > 0  TIME == 0 - Blocking read, reads until MIN bytes are available.
        // MIN == 0  TIME  > 0 - Read with timeout
        // MIN  > 0  TIME  > 0 - Read with interbyte timeout

        if (GETCC(VMIN) == 0 && GETCC(VTIME) == 0) {
            // Polling read
            return ringbuffer_read(pty->out, size, buffer);
        }

        // I think ringbuffer_read() will fix all of this - just read character by character.
        // THIS IS TEMPORARY!
        int c = 0;
        int vmin = MIN(GETCC(VMIN), size);
        while (c < vmin) {
            int ret = ringbuffer_read(pty->in, size - c, buffer + c);
            if (ret < 0) return c ? c : ret;
            c += ret;
        }

        return c;
    }
}

/**
 * @brief PTY master read method 
 */
uint32_t pty_master_read(fsNode_t* master, off_t offset, uint32_t size, uint8_t* buffer) {
    // This is just a read from the PTY out buffer, which is where slave processes write to.
    pty_t* pty = (pty_t*)master->impl_struct;
    if (!pty || !pty->out) return 0;
    return ringbuffer_read(pty->out, size, buffer);
}

/**
 * @brief PTY slave close method. Only here because of the hashmap
 */
void pty_slave_close(fsNode_t* node) {
    pty_t* pty = (pty_t*)node->impl_struct;
    if (pty && pty->name) { hashmap_remove(pty_hashmap, (char*)pty->name); }
}

/**
 * @brief ioctl method for the TTY device
 * 
 * Can return multiple bits of information on a TTY device
 * 
 */
int tty_ioctl(fsNode_t* node, unsigned long request, void* argp) {
    pty_t* pty = (pty_t*)node->impl_struct;
    if (!pty) return -EINVAL;

    switch (request) {
        // IOCTLDTYPE - Specific to reduceOS, identifies a TTY device
        case IOCTLDTYPE:
            return IOCTL_DTYPE_TTY;

        // IOCTLTTYNAME - Get the name of the TTY
        case IOCTLTTYNAME:
            if (!argp) return -EINVAL;
            syscall_validatePointer(argp, "tty_ioctl");
            pty->fill_name(pty, argp);
            return 0;

        // IOCTLTTYLOGIN - Set the UID of the current user
        case IOCTLTTYLOGIN:
            if (currentProcess->user_id != 0) return -EPERM; // Must be uid 0
            if (!argp) return -EINVAL;
            syscall_validatePointer(argp, "tty_ioctl");
            pty->slave->uid = *(int*)argp;
            pty->master->uid = *(int*)argp;
            return 0;

        // TIOCSWINSZ - Set window size
        case TIOCSWINSZ:
            if (!argp) return -EINVAL;
            syscall_validatePointer(argp, "tty_ioctl");
            memcpy(&pty->size, argp, sizeof(struct winsize));

            // If there is a foreground process, send the SIGWINCH signal
            if (pty->fg_proc) { group_send_signal(pty->fg_proc, SIGWINCH, 1); }

            return 0;

        // TIOCGWINSZ - Get window size
        case TIOCGWINSZ:
            if (!argp) return -EINVAL;
            syscall_validatePointer(argp, "tty_ioctl");
            memcpy(argp, &pty->size, sizeof(struct winsize));
            return 0;

        // TCGETS - Get the termios of the TTY
        case TCGETS:
            if (!argp) return -EINVAL;
            syscall_validatePointer(argp, "tty_ioctl");
            memcpy(argp, &pty->tios, sizeof(struct termios));
            return 0;

        // TIOCSPGRP - Set the process group
        case TIOCSPGRP:
            if (!argp) return -EINVAL;
            syscall_validatePointer(argp, "tty_ioctl");
            pty->fg_proc = *(pid_t*)argp;
            return 0;

        // TIOCGPGRP - Get the process group
        case TIOCGPGRP:
            if (!argp) return -EINVAL;
            syscall_validatePointer(argp, "tty_ioctl");
            *(pid_t*)argp = pty->fg_proc;
            return 0;

        // TIOCSCTTY - Set the controlling tty
        case TIOCSCTTY:
            // Are we already the controlling session?
            if (currentProcess->session == currentProcess->id && pty->ct_proc == currentProcess->session) { return 0; }

            // Are we the session leader?
            if (currentProcess->session != currentProcess->id) { return -EPERM; }

            // Is there already a session leader? If so, only root can force.
            if (pty->ct_proc && (!argp || (*(int*)argp != 1) || currentProcess->user_id != 0)) { return -EPERM; }

            pty->ct_proc = currentProcess->session;
            return 0;

        // TCSETS: Set the current termios setting
        case TCSETS:
            if (!argp) return -EINVAL;
            syscall_validatePointer(argp, "tty_ioctl");

            // If we're in canonical mode but trying not to set canonical mode, dump buffer.
            if (!(((struct termios*)argp)->c_lflag & ICANON) && LFLAG(ICANON)) {
                char* current_char = pty->canon_buffer;
                while (pty->canon_buflen > 0) {
                    pty->write_in(pty, *current_char);
                    current_char++;
                    pty->canon_buflen--;
                }
            }

            // Copy
            memcpy(&pty->tios, argp, sizeof(struct termios));
            return 0;

        // TCSETSW: Set the current termios setting and wait for buffer to drain
        case TCSETSW:
            // This is the exact same thing as TCSETS, TODO on waiting for output
            if (!argp) return -EINVAL;
            syscall_validatePointer(argp, "tty_ioctl");

            // If we're in canonical mode but trying not to set canonical mode, dump buffer.
            if (!(((struct termios*)argp)->c_lflag & ICANON) && LFLAG(ICANON)) {
                char* current_char = pty->canon_buffer;
                while (pty->canon_buflen > 0) {
                    pty->write_in(pty, *current_char);
                    current_char++;
                    pty->canon_buflen--;
                }
            }

            // Copy
            memcpy(&pty->tios, argp, sizeof(struct termios));
            return 0;

        // TCSETSF: Discard buffer
        case TCSETSF:
            if (!argp) return -EINVAL;
            syscall_validatePointer(argp, "tty_ioctl");

            pty->canon_buffer[0] = 0;
            pty->canon_buflen = 0;
            ringbuffer_discard(pty->in);

            memcpy(&pty->tios, argp, sizeof(struct termios));
            return 0;
        default:
            serialPrintf("tty_ioctl: Unknown ioctl call %i\n", request);
            return -EINVAL;
    }
}

/**
 * @brief Readlink method for the TTY device.
 * 
 * This method will search the current process' file descriptors,
 * find a TTY, and return its name into buffer.
 * 
 * @todo What if there are multiple ttys per process? Is that even possible? Am I going insane?
 */
int tty_readlink(fsNode_t* node, char* buffer, size_t size) {
    // Iterate through the current process' file descriptors
    if (!currentProcess) return 0; // ioctl() relies on size

    fsNode_t* ttyFound = NULL;

    for (size_t i = 0; i < currentProcess->file_descs->length; i++) {
        // Check if the file descriptor is a tty
        if (isatty(currentProcess->file_descs->nodes[i])) {
            ttyFound = currentProcess->file_descs->nodes[i];
            break;
        }
    }

    if (ttyFound == NULL) return 0;

    // Let's copy the name to a buffer
    pty_t* pty = (pty_t*)ttyFound->impl_struct;

    char name_out[50]; // what if it's more than 50 characters?

    if (!pty || !pty->fill_name) {
        // Nothing. Let's send back /device/null.
        sprintf((char*)name_out, "/device/null");
    } else {
        // We have a fill_name method.
        pty->fill_name(pty, name_out);
    }

    // We still have to do some trickery with size, though.
    if (size < (size_t)strlen(name_out) + 1) {
        // Make sure to clear out the last character.
        memcpy(buffer, name_out, size);
        buffer[size - 1] = '\0';
        return size - 1;
    }

    // If not, they want more than what we should give them, update that.
    memcpy(buffer, name_out, strlen(name_out));
    return strlen(name_out);
}

/**
 * @brief Create a master PTY filesystem node
 */
static fsNode_t* tty_createPTYMaster(pty_t* pty) {
    fsNode_t* out = kmalloc(sizeof(fsNode_t));

    out->name[0] = 0; // Apparently needed?
    sprintf(out->name, "Master PTY (%i)", pty->name);

    out->uid = currentProcess->user_id;
    out->gid = currentProcess->user_group;
    out->mask = 0666;
    out->flags = VFS_PIPE;

    // Methods
    out->open = NULL;
    out->close = NULL;
    out->read = pty_master_read;
    out->write = pty_master_write;
    out->ioctl = tty_ioctl;

    out->impl_struct = (uint32_t*)pty;
    return out;
}

/**
 * @brief Create a slave PTY filesystem node
 */
static fsNode_t* tty_createPTYSlave(pty_t* pty) {
    fsNode_t* out = kmalloc(sizeof(fsNode_t));
    out->impl_struct = (uint32_t*)pty;

    out->name[0] = 0; // Apparently needed?
    sprintf(out->name, "Slave PTY (%i)", pty->name);

    out->uid = currentProcess->user_id;
    out->gid = currentProcess->user_group;
    out->mask = 0620;
    out->flags = VFS_CHARDEVICE;

    // Methods
    out->open = NULL;
    out->close = pty_slave_close;
    out->read = pty_slave_read;
    out->write = pty_slave_write;
    out->ioctl = tty_ioctl;

    return out;
}

/**
 * @brief PTY finddir method for /device/pts
 */
static fsNode_t* pty_finddir(fsNode_t* node, char* name) {
    if (!name || strlen(name) < 1) return NULL;

    // Convert the name to an index
    intptr_t idx = 0;
    for (int i = 0; i < strlen(name); i++) {
        if (name[i] < '0' || name[i] > '9') return NULL; // (loud wrong buzzer)

        idx = idx * 10 + name[i] - '0'; // Weird math to convert to an integer of base-10
    }

    pty_t* pty = hashmap_get(pty_hashmap, (void*)idx);
    if (!pty) return NULL;
    return pty->slave;
}

/**
 * @brief PTY readdir method for /devicev/pts
 */
static struct dirent* pty_readdir(fsNode_t* node, unsigned long index) {
    // Make sure to handle . and ..
    if (index == 0) {
        struct dirent* out = kmalloc(sizeof(struct dirent));
        memset(out, 0, sizeof(struct dirent));
        out->ino = 0;
        strcpy(out->name, ".");
        return out;
    }

    if (index == 1) {
        struct dirent* out = kmalloc(sizeof(struct dirent));
        memset(out, 0, sizeof(struct dirent));
        out->ino = 1;
        strcpy(out->name, "..");
        return out;
    }

    index -= 2;

    // Let's try and find the pty to match
    pty_t* pty = NULL;
    list_t* values = hashmap_values(pty_hashmap);
    foreach (node, values) {
        if (index == 0) {
            pty = node->value;
            break;
        }

        index--;
    }

    list_free(values); // Remember to cleanup

    if (pty) {
        // Found it! Let's convert it to an inode
        struct dirent* out = kmalloc(sizeof(struct dirent));
        memset(out, 0, sizeof(struct dirent));
        out->ino = pty->name;
        sprintf(out->name, "%zd", pty->name);
        return out;
    } else {
        return NULL;
    }
}

/**
 * @brief Small method to fill in a TTY name, specified in a pty_t object.
 * @warning This is just for our driver. Other parts of the kernel will use another method.
 */
void tty_fillname(pty_t* pty, char* name) {
    if (!name) return;

    ((char*)name)[0] = 0; // Apparently, this is required.
    sprintf(name, "/device/pts/%zd", pty->name);
}

/**
 * @brief Create a PTY device
 * @param size The size of the PTY window. Taken as a "winsize" argument.
 * @returns A pty_t structure
 */
pty_t* tty_createPTY(struct winsize size) {
    // Only use this method when current process is enabled.

    // Create the pty_t output object.
    pty_t* pty = kmalloc(sizeof(pty_t));

    // Setup some basic information
    pty->name = pty_idx;            // 'name' is not a string, but the index of the PTY
    pty->fill_name = &tty_fillname; // The fillname method

    // Setup the filesystem nodes
    pty->master = tty_createPTYMaster(pty);
    pty->slave = tty_createPTYSlave(pty);

    // Create the ringbuffers
    pty->in = ringbuffer_create(4096); // 4096 is like the magic number in this OS
    pty->out = ringbuffer_create(4096);

    // Setup the ringbuffer writing methods (why do we do this again?)
    pty->write_in = pty_ringbuffer_write_in;
    pty->write_out = pty_ringbuffer_write_output;

    // CT and FG processes do not exist yet, so set them to zero
    // TODO: Could this cause issues?
    pty->fg_proc = 0;
    pty->ct_proc = 0;

    // By default, we'll have this TTY use canonical mode.
    pty->canon_buffer = kmalloc(4096);
    pty->canon_bufsize = 4096;
    pty->canon_buflen = 0;

    // Default flags (sourced from ToaruOS)
    pty->tios.c_iflag = ICRNL | BRKINT; // Translate CR to newline on input and SIGINT on break.
    pty->tios.c_oflag = ONLCR | OPOST;  // Map NL to CRNL (\r\n)
    pty->tios.c_lflag = ECHO | ECHOE | ECHOK | ICANON | ISIG | IEXTEN;
    pty->tios.c_cflag = CREAD | CS8 | B38400;

    // Setting up control variables
    pty->tios.c_cc[VEOF] = 4;      // ^D - EOF
    pty->tios.c_cc[VEOL] = 0;      // Unset
    pty->tios.c_cc[VERASE] = 0x7F; // ^? - Erase character
    pty->tios.c_cc[VINTR] = 3;     // ^C - Interrupt
    pty->tios.c_cc[VKILL] = 21;    // ^U - Kill character
    pty->tios.c_cc[VMIN] = 1;      // Minimum number of characters for noncanonical read.
    pty->tios.c_cc[VQUIT] = 28;    // ^/ - Quit
    pty->tios.c_cc[VSTART] = 17;   // ^Q - Start
    pty->tios.c_cc[VSTOP] = 19;    // ^S - Stop
    pty->tios.c_cc[VSUSP] = 26;    // ^Z - Suspend
    pty->tios.c_cc[VTIME] = 0;     // Time for noncanonical read.
    pty->tios.c_cc[VLNEXT] = 22;   // ^V - Literal next
    pty->tios.c_cc[VWERASE] = 23;  // ^W - Word erase

    return pty;
}

/**
 * @brief TTY initialization function - creates the VFS nodes they need.
 */
void tty_init() {
    // Create the hashmap for storing PTY nodes
    pty_hashmap = hashmap_create(10);

    // Create and mount the /device/pts directory
    pty_dir = kmalloc(sizeof(fsNode_t));
    pty_dir->flags = VFS_DIRECTORY;
    strcpy(pty_dir->name, "PTY Directory");
    pty_dir->mask = 0555;
    pty_dir->uid = 0;
    pty_dir->gid = 0;
    pty_dir->finddir = pty_finddir;
    pty_dir->readdir = pty_readdir;
    vfsMount("/device/pts", pty_dir);

    // Create the /dev/tty device
    tty_dev = kmalloc(sizeof(fsNode_t));
    tty_dev->flags = VFS_FILE | VFS_SYMLINK; // todo: should we use chardevice?
    strcpy(tty_dev->name, "TTY Device");
    tty_dev->mask = 0777;
    tty_dev->uid = 0;
    tty_dev->gid = 0;
    tty_dev->readlink = tty_readlink;
    tty_dev->length = 1;
    vfsMount("/device/tty", tty_dev);

    // The kernel should handle this by creating its own node, such as a serial thread.
}