/* $Header: /usr/home/yav/lupe/RCS/timer.h,v 1.2 1998/08/17 05:13:54 yav Exp $
 * Time control routine header
 * written by yav <UHD98984@biglobe.ne.jp>
 *
 * HAVE_GETTIMEOFDAY	- use gettimeofday system call
 * HAVE_USLEEP		- use usleep library
 */


#ifdef HAVE_GETTIMEOFDAY
#define TMV timeval
#else
struct TMV {
  long tv_sec;			/* seconds */
  long tv_usec;			/* microseconds */
};
#endif

/* sleep_time(n) macro sleep n milliseconds */
#ifdef HAVE_USLEEP
#define sleep_time(n) usleep((n)*1000L)	/* to microseconds */
#else
#define sleep_time(n) sleep(((n)+999)/1000) /* to seconds */
#endif

#ifdef HAVE_GETTIMEOFDAY
#define current_time(p) gettimeofday((p),NULL)
#else
#define current_time(p) ((p)->tv_usec = 0, time(&(p)->tv_sec))
#endif

/* timer.c function prototypes */
long diff_time();
void add_time();

/* End of file */
