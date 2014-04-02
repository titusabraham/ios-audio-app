#include "timesec.h"

// microseconds to milliseconds
unsigned long usec_to_msec(unsigned long usec) {
    return usec/1000;
}

// microseconds to nanoseconds
unsigned long usec_to_nsec(unsigned long usec) {
    return usec*1000;
}

// milliseconds to microseconds
unsigned long msec_to_usec(unsigned long msec) {
    return msec*1000;
}

// milliseconds to nanoseconds
unsigned long msec_to_nsec(unsigned long msec) {
    return msec*1000000;
}

// nanoseconds to microseconds
unsigned long nsec_to_usec(unsigned long nsec) {
    return nsec/1000;
}

// nanoseconds to milliseconds
unsigned long nsec_to_msec(unsigned long nsec) {
    return nsec/1000000;
}

// seconds to milliseconds
unsigned long sec_to_msec(time_t sec) {
    return sec*1000;
}

// seconds to microseconds
unsigned long sec_to_usec(time_t sec) {
    return sec*1000000;
}

// seconds to nanoseconds
unsigned long sec_to_nsec(time_t sec) {
    return sec*1000000000;
}

// timespec + seconds
void ts_add_sec(struct timespec* ts, time_t sec) {
    ts->tv_sec += sec;
}

// timespec + nanoseconds
void ts_add_nsec(struct timespec* ts, unsigned long nsec) {
    ts->tv_nsec += nsec;
    int sec = ts->tv_nsec / 1000000000;
    if (sec > 0) {
	ts->tv_sec += sec;
	ts->tv_nsec -= sec*1000000000;
    }
}

// timespec + milliseconds
void ts_add_msec(struct timespec* ts, unsigned long msec) {
    ts_add_nsec(ts, msec_to_nsec(msec));
}

// timespec + microseconds
void ts_add_usec(struct timespec* ts, unsigned long usec) {
    ts_add_nsec(ts, usec_to_nsec(usec));
}

// timeval in milliseconds
unsigned long ts_in_msec(struct timespec* ts) {
    return sec_to_msec(ts->tv_sec) + nsec_to_msec(ts->tv_nsec);
}

// timeval + seconds
void tv_add_sec(struct timeval* tv, time_t sec) {
    tv->tv_sec += sec;
}

// timeval + microseconds
void tv_add_usec(struct timeval* tv, unsigned long usec) {
    tv->tv_usec += usec;
    int sec = tv->tv_usec / 1000000;
    if (sec > 0) {
	tv->tv_sec += sec;
	tv->tv_usec -= sec*1000000;
    }
}

// timeval + milliseconds
void tv_add_msec(struct timeval* tv, unsigned long msec) {
    tv_add_usec(tv, msec_to_usec(msec));
}

// timeval + nanoseconds
void tv_add_nsec(struct timeval* tv, unsigned long nsec) {
    tv_add_usec(tv, nsec_to_usec(nsec));
}

// timeval in milliseconds
unsigned long tv_in_msec(struct timeval* tv) {
    return sec_to_msec(tv->tv_sec) + usec_to_msec(tv->tv_usec);
}

// current time in milliseconds
unsigned long time_in_millis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv_in_msec(&tv);
}

#ifdef UNIT_TEST
int main(int argc, char **argv) {
    struct timespec ts;
    struct timespec ts_tmp;

    struct timeval tv;
    struct timeval tv_tmp;

    clock_gettime(CLOCK_REALTIME, &ts);
    gettimeofday(&tv, NULL);

    printf("timespec\n");
    printf("  %ld\n", ts_in_msec(&ts));
    printf("  %ld %ld\n", ts.tv_sec, ts.tv_nsec);

    ts_tmp.tv_sec = ts.tv_sec;
    ts_tmp.tv_nsec = ts.tv_nsec;
    ts_add_msec(&ts_tmp, 200);
    printf("  %ld %ld\n", ts_tmp.tv_sec, ts_tmp.tv_nsec);

    ts_tmp.tv_sec = ts.tv_sec;
    ts_tmp.tv_nsec = ts.tv_nsec;
    ts_add_usec(&ts_tmp, 200000);
    printf("  %ld %ld\n", ts_tmp.tv_sec, ts_tmp.tv_nsec);

    ts_tmp.tv_sec = ts.tv_sec;
    ts_tmp.tv_nsec = ts.tv_nsec;
    ts_add_nsec(&ts_tmp, 200000000);
    printf("  %ld %ld\n", ts_tmp.tv_sec, ts_tmp.tv_nsec);

    printf("timeval\n");
    printf("  %ld\n", tv_in_msec(&tv));
    printf("  %ld %ld\n", tv.tv_sec, tv.tv_usec);

    tv_tmp.tv_sec = tv.tv_sec;
    tv_tmp.tv_usec = tv.tv_usec;
    tv_add_msec(&tv_tmp, 200);
    printf("  %ld %ld\n", tv_tmp.tv_sec, tv_tmp.tv_usec);

    tv_tmp.tv_sec = tv.tv_sec;
    tv_tmp.tv_usec = tv.tv_usec;
    tv_add_usec(&tv_tmp, 200000);
    printf("  %ld %ld\n", tv_tmp.tv_sec, tv_tmp.tv_usec);

    tv_tmp.tv_sec = tv.tv_sec;
    tv_tmp.tv_usec = tv.tv_usec;
    tv_add_nsec(&tv_tmp, 200000000);
    printf("  %ld %ld\n", tv_tmp.tv_sec, tv_tmp.tv_usec);
}
#endif // UNIT_TEST
