#ifndef _timesec_h
#define _timesec_h

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned long usec_to_msec(unsigned long usec);
extern unsigned long usec_to_nsec(unsigned long usec);

extern unsigned long msec_to_usec(unsigned long msec);
extern unsigned long msec_to_nsec(unsigned long msec);

extern unsigned long nsec_to_usec(unsigned long nsec);
extern unsigned long nsec_to_msec(unsigned long nsec);

extern unsigned long sec_to_msec(time_t sec);
extern unsigned long sec_to_usec(time_t sec);
extern unsigned long sec_to_nsec(time_t sec);

extern void ts_add_sec(struct timespec* ts, time_t sec);
extern void ts_add_nsec(struct timespec* ts, unsigned long nsec);
extern void ts_add_msec(struct timespec* ts, unsigned long msec);
extern void ts_add_usec(struct timespec* ts, unsigned long usec);

extern void tv_add_sec(struct timeval* tv, time_t sec);
extern void tv_add_usec(struct timeval* tv, unsigned long usec);
extern void tv_add_msec(struct timeval* tv, unsigned long msec);
extern void tv_add_nsec(struct timeval* tv, unsigned long nsec);

extern unsigned long tv_in_msec(struct timeval* tv);
extern unsigned long time_in_millis();

#ifdef __cplusplus
}
#endif

#endif // _timesec_h
