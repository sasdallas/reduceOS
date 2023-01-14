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

/*	Address Spaces test file			*/

#include <ll/i386/stdlib.h>
#include <ll/i386/error.h>
#include <ll/i386/mem.h>
#include <ll/i386/hw-arch.h>
#include <ll/i386/farptr.h>
#include <ll/sys/ll/ll-func.h>
#include <ll/sys/ll/aspace.h>
#include <ll/sys/ll/event.h>

#define T  1000
#define WAIT()  for (w = 0; w < 0x5FFFFF; w++)                                 

extern DWORD GDT_base;

BYTE space[2048];
BYTE stack1[1024];
WORD th1, th2, thm;

/* For now, this is not called */
void killer(void)
{
  cli();
  message("Killer!!!\n");
  ll_context_load(thm);
}

/* Used to switch back to main thread */
void evtHandler(void *v)
{
  ll_context_load(thm);
}

/* 
   Dummy thread: used only to test a context switch between two threads
   in the same AS
*/
void nullfun1(void *p)
{
  message("In thread 1\n");
  ll_context_load(thm);
}

/*
  And this is the thread that runs in the new AS... It cannot call
  any function (it is alone in its AS), so it simply plots a star
  on the screen and loops waiting for an event that switches to the
  main thread.
*/
void nullfun(void *p)
{
  /* Some asm to write on the screen (we cannot use libc... it is not
     linked in this AS */
  __asm__ __volatile__ ("movl $0xb8000, %ebp");
  __asm__ __volatile__ ("addl $150, %ebp");
  __asm__ __volatile__ ("movb $'*', %fs:0(%ebp)");
  __asm__ __volatile__ ("incl %ebp");
  __asm__ __volatile__ ("movb $6, %fs:0(%ebp)");

  /* And now, WAIT!!! */
  for(;;);
  halt();
}

int main (int argc, char *argv[])
{
  DWORD sp1, sp2;
  void *res;
  AS as, ds;
  int ok;
  char mystring[20];
  struct ll_initparms parms;
  struct timespec time;
  
  sp1 = get_sp();
  cli();
  if ((argc == 2) && (argv[1][0] == 'O')) {
    parms.mode = LL_ONESHOT;
  } else {
    parms.mode = LL_PERIODIC;
    parms.tick = 1000;
  }
  res = ll_init();
  event_init(&parms);

  if (res == NULL) {
    message("Error in LowLevel initialization code...\n");
    sti();
    l1_exit(-1);
  }
  NULL_TIMESPEC(&time);
  ADDNANO2TIMESPEC(500000000, &time);
  ADDNANO2TIMESPEC(500000000, &time);
  ADDNANO2TIMESPEC(500000000, &time);
  ADDNANO2TIMESPEC(500000000, &time);
  ADDNANO2TIMESPEC(500000000, &time);
  ADDNANO2TIMESPEC(500000000, &time);
  ADDNANO2TIMESPEC(500000000, &time);
  ADDNANO2TIMESPEC(500000000, &time);
  ADDNANO2TIMESPEC(500000000, &time);
  ADDNANO2TIMESPEC(500000000, &time);        /* Time1: 5 ms */
  event_post(time, evtHandler, NULL);
  sti();

  message("LowLevel started...\n");
    
  as_init();

  message("Creating an Address Space\n");
  as = as_create();
  message("Created: ID = %d %x\n", as, as);
  message("Binding array @ 0x%lx\n", (DWORD)space);
  ok = as_bind(as, (DWORD)space, 0, sizeof(space));

  if (ok < 0) {
    message("Error in as_bind!!!\n");
  } else {
    message("Bound\n");
  }
  _farpokeb(as, 0, 'H');
  _farpokeb(as, 1, 'e');
  _farpokeb(as, 2, 'l');
  _farpokeb(as, 3, 'l');
  _farpokeb(as, 4, 'o');
  _farpokeb(as, 5, 0);

  ds = get_ds();
  fmemcpy(ds, (DWORD)mystring, as, 0, 10);
  message("Memory space: %s\n", space);
  message("My String: %s\n", mystring);

  fmemcpy(as, 0, ds, (DWORD)nullfun, 1000);
  /* Create a thread (in our AS) and a task (in a different AS) */
  th2 = ll_context_create(nullfun1, &stack1[1024], NULL,killer, 0);
  th1 = ll_context_create(0, (void *)2048, NULL,killer, 0);
  ll_context_setspace(th1, as);
    	
  /* Save our context, in order to permit to switch back to main */
  thm = ll_context_save();
  message("I am thread %x\n", thm);
  message("Thread 1 created\n");
  message("Switching to it...\n");
  ll_context_load(th2);
  message("Back to Main\n");

  /* Plot something in a stupid way, just to do some work... */
  __asm__ __volatile__ ("movl $0x30, %eax");
  __asm__ __volatile__ ("movw %ax, %fs");
  __asm__ __volatile__ ("movl $0xb8000, %edi");
  __asm__ __volatile__ ("addl $152, %edi");
  __asm__ __volatile__ ("movb $'+', %ds:0(%edi)");
  __asm__ __volatile__ ("incl %edi");
  __asm__ __volatile__ ("movb $6, %ds:0(%edi)");
  
  /* And now, go to the new task!!! */
  ll_context_load(th1);

  message("Main task again...\n");

  __asm__ __volatile__ ("movl $0x30, %eax");
  __asm__ __volatile__ ("movw %ax, %fs");
  __asm__ __volatile__ ("movl $0xb8000, %edi");
  __asm__ __volatile__ ("addl $154, %edi");
  __asm__ __volatile__ ("movb $'-', %fs:0(%edi)");
  __asm__ __volatile__ ("incl %edi");
  __asm__ __volatile__ ("movb $6, %fs:0(%edi)");

  message("Current time: %ld (= %ld?)\n", ll_gettime(TIME_NEW, NULL),
	  ll_gettime(TIME_EXACT, NULL));
  cli();
  ll_end();
  sp2 = get_sp();
  message("End reached!\n");
  message("Actual stack : %lx - ", sp2);
  message("Begin stack : %lx\n", sp1);
  message("Check if same : %s\n",sp1 == sp2 ? "Ok :-)" : "No :-(");
  
  return 1;
}
