/* $Header: /usr/home/yav/lupe/RCS/timer.c,v 1.3 1998/08/26 07:37:01 yav Exp $
 * Lupe Time control routine
 * written by yav <yav@mte.biglobe.ne.jp>
 */

char rcsid_timer[] = "$Id: timer.c,v 1.3 1998/08/26 07:37:01 yav Exp $";

#include <X11/Xos.h>
#include <stdio.h>
#include "config.h"

#include "timer.h"


long diff_time(p1, p0)
     struct TMV *p1;
     struct TMV *p0;
{
  struct TMV tm;
  
  tm.tv_sec = p1->tv_sec - p0->tv_sec;
  tm.tv_usec = p1->tv_usec - p0->tv_usec;
  if (tm.tv_usec < 0) {
    tm.tv_usec += 1000000;
    tm.tv_sec--;
  }
  if (tm.tv_sec > 86400L)	/* 24 hour */
    tm.tv_sec = 86400L;
  return tm.tv_sec * 1000 + tm.tv_usec / 1000;
}

void add_time(p, n)
     struct TMV *p;
     long n;			/* milliseconds */
{
  p->tv_sec += n / 1000;
  p->tv_usec += (n % 1000) * 1000;
  if (p->tv_usec < 0) {
    p->tv_usec += 1000000;
    p->tv_sec--;
  } else if (p->tv_usec >= 1000000) {
    p->tv_usec -= 1000000;
    p->tv_sec++;
  }
}

/* End of file */
