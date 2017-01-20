/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#include "libparodus_time.h"
#include "libparodus_log.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>


/**
 * struct tv has two components: tv_sec and tv_usec
 *
 * struct tm has the time broken into components, year, month, day, etc.
*/  


/**
 * Get current time
 *
 * @param tv	current time
 * @param split_time  time broken down
 * @return 0 on success, valid errno otherwise.
 */
int get_current_time (struct timeval *tv, struct tm *split_time)
{
	time_t secs;
	int err = gettimeofday (tv, NULL);
	if (err != 0) {
		libpd_log (LEVEL_NO_LOGGER, errno, "Error getting time of day: ");
		return err;
	}
	secs = (time_t) tv->tv_sec;
	localtime_r (&secs, split_time);
	return 0;
}

/**
 * Get the date out of a struct tm, 8 chars: YYYYMMDD
 *
 * @param split_time  time broken down
 * @param date 8 character buffer to receive the date.
*/ 
void extract_date (struct tm *split_time, char *date)
{
	sprintf (date, "%04d%02d%02d", 
		split_time->tm_year+1900, split_time->tm_mon+1, split_time->tm_mday);
}

/**
 * Get the current date, 8 chars: YYYYMMDD
 *
 * @param date 8 byte buffer where date is stored (YYYYMMDD)
 * @return 0 on success, valid errno otherwise.
*/ 
int get_current_date (char *date)
{
	int err;
	struct timeval tv;
	struct tm split_time;

	err = get_current_time (&tv, &split_time);
	if (err != 0)
		return err;
	extract_date (&split_time, date);
	return 0;
}


/**
 * Create a timestamp, given a struct tm and milliseconds,
 * in timestamp format: (YYYYMMDD-HHMMSS-MMM)
 *
 * @param split_time  time broken down
 * @param msecs       milliseconds
 * @param timestamp   buffer (TIMESTAMP_BUFLEN characters) to receive the timestamp
*/ 
void make_timestamp (struct tm *split_time, unsigned msecs, char *timestamp)
{
	sprintf (timestamp, "%04d%02d%02d-%02d%02d%02d-%03d", 
		split_time->tm_year+1900, split_time->tm_mon+1, split_time->tm_mday,
		split_time->tm_hour, split_time->tm_min, split_time->tm_sec,
		msecs);
}

/**
 * Create a timestamp, given a struct timeval,
 * in timestamp format: (YYYYMMDD-HHMMSS-MMM)
 * 
 * @param tv time in a struct timeval structure
 * @param timestamp   buffer (TIMESTAMP_BUFLEN characters) to receive the timestamp
*/ 
void make_tv_timestamp (struct timeval *tv, char *timestamp)
{
	struct tm split_time;
	time_t secs = (time_t) tv->tv_sec;
	localtime_r (&secs, &split_time);
	make_timestamp (&split_time, tv->tv_usec / 1000, timestamp);
}


/**
 * Create a timestamp from the current time, in timestamp format:
 * (YYYYMMDD-HHMMSS-MMM)
 *
 * @param timestamp 20 byte buffer (19 byte time + ending null)
 * @return 0 on success, valid errno otherwise.
*/ 
int make_current_timestamp (char *timestamp)
{
	int err;
	struct timeval tv;
	struct tm split_time;
  
	err = get_current_time (&tv, &split_time);
	if (err != 0)
		return err;
	make_timestamp (&split_time, tv.tv_usec / 1000, timestamp);
	return 0;
}

int get_expire_time (uint32_t ms, struct timespec *ts)
{
	struct timeval tv;
	int err = gettimeofday (&tv, NULL);
	if (err != 0) {
		libpd_log (LEVEL_ERROR, errno, "Error getting time of day\n");
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

void delay_ms(unsigned msecs)
{
  struct timespec ts;
  ts.tv_sec = (time_t) (msecs / 1000);
  ts.tv_nsec = (long) (msecs % 1000) * 1000000L;
  nanosleep (&ts, NULL);
}

#ifdef TEST_SOCKET_TIMING

sst_totals_t sst_totals;
int sst_err;

void sst_init_totals (void)
{
	libpd_log (LEVEL_INFO, 0, "LIBPARODUS Init Socket Timing\n");
	sst_totals.count = 0;
	sst_totals.total_time.tv_sec = 0;
	sst_totals.total_time.tv_usec = 0;
	sst_totals.send_time.tv_sec = 0;
	sst_totals.send_time.tv_usec = 0;
	sst_err = 0;
}

void sub_time (struct timeval *a, struct timeval *b)
// result in b
{
	b->tv_sec = b->tv_sec - a->tv_sec;
	if (b->tv_usec < a->tv_usec) {
		b->tv_sec--;
		b->tv_usec += 1000000;
	}
	b->tv_usec -= a->tv_usec;
}

void add_time (struct timeval *a, struct timeval *b)
// add a to b
{
	b->tv_sec += a->tv_sec;
	b->tv_usec += a->tv_usec;
	if (b->tv_usec >= 1000000) {
		b->tv_sec++;
		b->tv_usec -= 1000000;
	}
}

void sst_start_total_timing (sst_times_t *times)
{
	int err = gettimeofday (&times->connect_time, NULL);
	if (err != 0)
		sst_err = err;
}

void sst_start_send_timing (sst_times_t *times)
{
	int err = gettimeofday (&times->send_time, NULL);
	if (err != 0)
		sst_err = err;
}

void sst_update_send_time (sst_times_t *times)
{
	struct timeval stop_time;
	int err = gettimeofday (&stop_time, NULL);
	if (err != 0) {
		sst_err = err;
		return;
	}
	if (sst_totals.total_time.tv_sec >= 1999)
		return;
	sub_time (&times->send_time, &stop_time);
	add_time (&stop_time, &sst_totals.send_time);
	//libpd_log (LEVEL_INFO, 0, "LIBPARODUS Diff Time (%lu:%lu), Send Time (%lu:%lu)\n",
	//	stop_time.tv_sec, stop_time.tv_usec,
	//	sst_totals.send_time.tv_sec, sst_totals.send_time.tv_usec);
}

void sst_update_total_time (sst_times_t *times)
{
	struct timeval stop_time;
	int err = gettimeofday (&stop_time, NULL);
	if (err != 0) {
		sst_err = err;
		return;
	}
	if (sst_totals.total_time.tv_sec >= 1999)
		return;
	sub_time (&times->connect_time, &stop_time);
	add_time (&stop_time, &sst_totals.total_time);
	sst_totals.count++;
}


int sst_display_totals (void)
{
	unsigned long con_send_discon_avg, send_avg, diff_avg;

	libpd_log (LEVEL_INFO, 0, "LIBPARODUS Socket Timing Totals\n");
	libpd_log (LEVEL_INFO, 0, " Count %u, Total Time (%lu:%lu), Send Time (%lu:%lu)\n",
		sst_totals.count, sst_totals.total_time.tv_sec, sst_totals.total_time.tv_usec,
		sst_totals.send_time.tv_sec, sst_totals.send_time.tv_usec);
	if (sst_totals.count == 0)
		return sst_err;
	con_send_discon_avg = ((sst_totals.total_time.tv_sec * 1000lu) +
		sst_totals.total_time.tv_usec) / sst_totals.count;
	send_avg = ((sst_totals.send_time.tv_sec * 1000lu) +
		sst_totals.send_time.tv_usec) / sst_totals.count;
	sub_time (&sst_totals.send_time, &sst_totals.total_time);
	diff_avg = ((sst_totals.total_time.tv_sec * 1000lu) +
		sst_totals.total_time.tv_usec) / sst_totals.count;

	libpd_log (LEVEL_INFO, 0, " ConSendDiscon Avg %lu, Send Avg %lu, Diff Avg %lu\n",
		con_send_discon_avg, send_avg, diff_avg);
	return sst_err;
}

#endif




