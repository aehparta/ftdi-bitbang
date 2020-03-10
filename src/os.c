/*
 * Operating system specific routines.
 */

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "os.h"


time_t os_timei(void)
{
	struct timespec tp;
	if (clock_gettime(CLOCK_MONOTONIC, &tp) == EINVAL) {
		fprintf(stderr, "clock_gettime() does not support CLOCK_MONOTONIC in this system");
	}
	return tp.tv_sec;
}

os_time_t os_timef(void)
{
	struct timespec tp;
	if (clock_gettime(CLOCK_MONOTONIC, &tp) == EINVAL) {
		fprintf(stderr, "clock_gettime() does not support CLOCK_MONOTONIC in this system");
	}
	return (os_time_t)((os_time_t)tp.tv_sec + (os_time_t)tp.tv_nsec / 1e9);
}

void os_sleepi(time_t t)
{
	os_sleepf((os_time_t)t);
}

void os_sleepf(os_time_t t)
{
	struct timespec tp;
	os_time_t integral;
	t += os_timef();
	tp.tv_nsec = (long)(modfl(t, &integral) * 1e9);
	tp.tv_sec = (time_t)integral;
	while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tp, NULL) == EINTR);
}
