#include "libparodus_time.h"
#include "libparodus_log.h"
#include <stdio.h>
#include <errno.h>


int get_expire_time (uint32_t ms, struct timespec *ts)
{
	struct timeval tv;
	int err = gettimeofday (&tv, NULL);
	if (err != 0) {
        log_error("Error getting time of day %d\n", errno);
		return err;
	}
	tv.tv_sec += ms/1000;
	tv.tv_usec += (ms%1000) * 1000;
	if (tv.tv_usec >= 1000000) {
		tv.tv_sec += 1;
		tv.tv_usec -= 1000000;
	}

	ts->tv_sec = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000L;
	return 0;
}





