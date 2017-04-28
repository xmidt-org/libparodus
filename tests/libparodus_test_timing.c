 /**
  * Copyright 2016 Comcast Cable Communications Management, LLC
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
 */

#include "../src/libparodus_test_timing.h"
#include "../src/libparodus_log.h"

#ifdef TEST_SOCKET_TIMING

sst_totals_t sst_totals;
int sst_err;

void sst_init_totals (void)
{
	libpd_log (LEVEL_INFO, ("LIBPARODUS Init Socket Timing\n"));
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

	libpd_log (LEVEL_INFO, ("LIBPARODUS Socket Timing Totals\n"));
	libpd_log (LEVEL_INFO, (" Count %u, Total Time (%lu:%lu), Send Time (%lu:%lu)\n",
		sst_totals.count, sst_totals.total_time.tv_sec, sst_totals.total_time.tv_usec,
		sst_totals.send_time.tv_sec, sst_totals.send_time.tv_usec));
	if (sst_totals.count == 0)
		return sst_err;
	con_send_discon_avg = ((sst_totals.total_time.tv_sec * 1000lu) +
		sst_totals.total_time.tv_usec) / sst_totals.count;
	send_avg = ((sst_totals.send_time.tv_sec * 1000lu) +
		sst_totals.send_time.tv_usec) / sst_totals.count;
	sub_time (&sst_totals.send_time, &sst_totals.total_time);
	diff_avg = ((sst_totals.total_time.tv_sec * 1000lu) +
		sst_totals.total_time.tv_usec) / sst_totals.count;

	libpd_log (LEVEL_INFO, (" ConSendDiscon Avg %lu, Send Avg %lu, Diff Avg %lu\n",
		con_send_discon_avg, send_avg, diff_avg));
	return sst_err;
}

#endif

