#ifndef  _LIBPARODUS_TIME_H
#define  _LIBPARODUS_TIME_H

#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Get absolute expiration timespec, given delay in ms
 *
 * @param timestamp 20 byte buffer (19 byte time + ending null)
 * @param ms  delay in msecs
 * @param ts  expiration time
 * @return 0 on success, valid errno otherwise.
 */
int get_expire_time (uint32_t ms, struct timespec *ts);

#endif

