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

/*	Event Demo	*/

#include <ll/i386/stdlib.h>
#include <ll/i386/cons.h>
#include <ll/i386/error.h>
#include <ll/i386/mem.h>
#include <ll/sys/ll/ll-func.h>
#include <ll/sys/ll/time.h>
#include <ll/sys/ll/event.h>

#define T  1000

DWORD t1[10], t2[10];
int id[10];
int canEnd;

void evtHandler(void *p)
{
  int i;
  struct timespec t;
  static int n = 0;

  i = *(int *)p;

  t1[++n] = ll_gettime(TIME_NEW, &t);
  t2[n] = TIMESPEC2USEC(&t);
  id[n] = i;

  if (n == 9) {
    canEnd = 1;
    /*message("\t\tCan End\n");*/
  }
}

int main (int argc, char *argv[])
{
  DWORD sp1, sp2;
  struct ll_initparms parms;
  void *mbi;
  struct timespec time1, time2, time3, time4, time5;
  struct timespec time6, time7, time8, time9;
  int par[10];
  int i, secs;
  DWORD oldt, t;

  sp1 = get_sp();
  cli();

#ifdef PERIODIC
  parms.mode = LL_PERIODIC;
  parms.tick = T;
#else
  parms.mode = LL_ONESHOT;
#endif

  mbi = ll_init();
  event_init(&parms);

  if (mbi == NULL) {
    message("Error in LowLevel initialization code...\n");
    sti();
    l1_exit(-1);
  }
  sti();

  message("LowLevel started...\n");

/*  cli(); */
  NULL_TIMESPEC(&time1);
  NULL_TIMESPEC(&time2);
  NULL_TIMESPEC(&time3);
  NULL_TIMESPEC(&time4);
  NULL_TIMESPEC(&time5);
  NULL_TIMESPEC(&time6);
  NULL_TIMESPEC(&time7);
  NULL_TIMESPEC(&time8);
  NULL_TIMESPEC(&time9);

  ADDNANO2TIMESPEC(1000000, &time1);        /* Time1: 1 ms */
  ADDNANO2TIMESPEC(5000000, &time2);        /* Time2: 5 ms */
  ADDNANO2TIMESPEC(10000000, &time3);        /* Time 3: 10 ms */
  ADDNANO2TIMESPEC(3000000, &time4);        /* Time 4: 3 ms */
  ADDNANO2TIMESPEC(7500000, &time5);        /* Time 5: 7.5 ms */
  ADDNANO2TIMESPEC(7000000, &time6);        /* Time 6: 7 ms */
  ADDNANO2TIMESPEC(500000000, &time7);
  ADDNANO2TIMESPEC(500000000, &time7);
  ADDNANO2TIMESPEC(500000000, &time7);
  ADDNANO2TIMESPEC(500000000, &time7);
  ADDNANO2TIMESPEC(500000000, &time7);
  ADDNANO2TIMESPEC(500000000, &time7);        /* Time 7: 6*500 ms = 3 Sec*/
  ADDNANO2TIMESPEC(51700000, &time8);        /* Time 8: 51.7 ms */
  ADDNANO2TIMESPEC(51500000, &time9);        /* Time 9: 51.5 ms */

cli();
t = ll_gettime(TIME_NEW, NULL);
sti();
  
  for (i = 0; i < 10; i++) {
    par[i] = i + 1;
  }
  canEnd = 0;

  cli();

  event_post(time1, evtHandler, &(par[0]));
  event_post(time2, evtHandler, &(par[1]));
  event_post(time3, evtHandler, &(par[2]));
  event_post(time4, evtHandler, &(par[3]));
  event_post(time5, evtHandler, &(par[4]));
  i = event_post(time6, evtHandler, &(par[5]));
  event_post(time7, evtHandler, &(par[6]));
  event_post(time8, evtHandler, &(par[7]));
  event_post(time9, evtHandler, &(par[8]));

  event_delete(i);
  event_post(time5, evtHandler, &(par[5]));
    
  message("Now time is %lu\n", t);
  
  secs = 0;
  oldt = 0;
  canEnd = 0;
  while((canEnd == 0) && (secs < 6)) {
    cli();
    t = ll_gettime(TIME_NEW, NULL);
    sti();
    if (t < oldt) {
      error("Time goes back???\n");
      message("ARGGGGG! %lu %lu\n", t, oldt);
      ll_abort(100);
    }
    oldt = t;
    if ((t  / 1000000) > secs) {
      secs++;
      message("           %d     %lu\n", secs, t);
    }
  }
  cli();
  message("Can End detected\n");
  ll_end();
  sp2 = get_sp();
  for (i = 1; i < 10; i++) {
    message("Event %d called at time %lu    ==    %lu\n", id[i],
	    t1[i], t2[i]);
  }
  message("End reached!\n");
  message("Actual stack : %lx - ", sp2);
  message("Begin stack : %lx\n", sp1);
  message("Check if same : %s\n",sp1 == sp2 ? "Ok :-)" : "No :-(");

  return 1;
}
