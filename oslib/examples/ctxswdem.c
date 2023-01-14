/* Project:     OSLib
 * Description: The OS Construction Kit
 * Date:                1.6.2000
 * Idea by:             Luca Abeni & Gerardo Lamastra
 *
 * OSLib is an SO project aimed at developing a common, easy-to-use
 * low-level infrastructure for developing OS kernels and Embedded
 * Applications; it partially derives from the HARTIK project but it
 * currently is independently developed.
 *
 * OSLib is distributed under GPL License, and some of its code has
 * been derived from the Linux kernel source; also some important
 * ideas come from studying the DJGPP go32 extender.
 *
 * We acknowledge the Linux Community, Free Software Foundation,
 * D.J. Delorie and all the other developers who believe in the
 * freedom of software and ideas.
 *
 * For legalese, check out the included GPL license.
 */

/*	Context Switch Demo	*/

#include <ll/i386/stdlib.h>
#include <ll/i386/tss-ctx.h>
#include <ll/i386/error.h>
#include <ll/sys/ll/ll-func.h>

#define STACK_SIZE      4096U           /* Stack size in bytes          */
#define USE_FPU         0x0001

WORD th1, thm;

#define T 1000

#if 1
#define TO     ll_context_to
#define FROM   ll_context_from 
#else
#define TO     ll_context_load
#define FROM   ll_context_save 
#endif

/* For stack allocation checking */
BYTE stack1[STACK_SIZE];
BYTE stack2[STACK_SIZE];

void thread1(void *px)
{
	message("Thread 1 running\n");
	message("Switching to main thread\n");
	TO(thm);
	message("Another time thread 1\n");
	message("Back to main\n");
	TO(thm);
	message("And now, finishing thread 1\n");
}

void killer(void)
{
	cli();
	message("Killer!!!\n");
	TO(thm);
}

int main (int argc, char *argv[])
{
    DWORD sp1, sp2;
    void *mbi;

    sp1 = get_sp();
    cli();

    mbi = ll_init();

    if (mbi == NULL) {
	    message("Error in LowLevel initialization code...\n");
	    sti();
	    l1_exit(-1);
    }
    sti();
    message("LowLevel started...\n");
    th1 = ll_context_create(thread1, &stack1[STACK_SIZE], NULL,killer, 0);
    thm = FROM();
    message("Thread 1 created\n");
    message("Switching to it...\n");
    TO(th1);
    message("Returned to main\n");
    message("And now, to thread 1 again!!!\n");
    TO(th1);
    message("Main another time...\n");
    TO(th1);
    message("OK, now I finish...\n");
    cli();
    ll_end();
    sp2 = get_sp();
    message("End reached!\n");
    message("Actual stack : %lx - ", sp2);
    message("Begin stack : %lx\n", sp1);
    message("Check if same : %s\n",sp1 == sp2 ? "Ok :-)" : "No :-(");
    return 1;
}
