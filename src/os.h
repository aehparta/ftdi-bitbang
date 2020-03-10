/*
 * Operating system specific routines.
 */


#ifndef OS_H
#define OS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef long double os_time_t;

/**
 * Get absolute monotonic time with one second resolution.
 * Zero is some unknown time in the past, usually system startup.
 * @return time_t (uint32_t usually)
 */
time_t os_timei(void);

/**
 * Get absolute monotonic time with system specific resolution as floating point number.
 * Zero is some unknown time in the past, usually system startup.
 * @return os_time_t (long double or double usually)
 */
os_time_t os_timef(void);

/**
 * Sleep with multiple of seconds, resolution depends on system.
 * @param t seconds
 */
void os_sleepi(time_t t);

/**
 * Sleep specified amount of time. Resolution depends on system.
 * @param t seconds as floating point number
 */
void os_sleepf(os_time_t t);


#ifdef __cplusplus
}
#endif

#endif /* OS_H */
