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
#ifndef  _LIBPARODUS_TIME_H
#define  _LIBPARODUS_TIME_H

#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

//#define TEST_SOCKET_TIMING 1

#define TIMESTAMP_LEN 19
#define TIMESTAMP_BUFLEN (TIMESTAMP_LEN+1)

#define NONE_TIME "00000000-000000-000"

/*
 * 	timestamps are stored as 19 byte character strings:
 *  YYYYMMDD-HHMMSS-mmm
 *  where the last 3 bytes are  milliseconds
*/

/**
 * Get current time
 *
 * @param tv	current time
 * @param split_time  time broken down
 * @return 0 on success, valid errno otherwise.
 */
int get_current_time (struct timeval *tv, struct tm *split_time);

/**
 * Extract date from struct tm 
 *
 * @param split_time  time broken down
 * @param date 8 byte buffer where date is stored (YYYYMMDD)
 * @return None
 */
void extract_date (struct tm *split_time, char *date);

/**
 * Get current date
 *
 * @param date 8 byte buffer where date is stored (YYYYMMDD)
 * @return 0 on success, valid errno otherwise.
 */
int get_current_date (char *date);

/**
 * Create a timestamp 
 *
 * @param split_time  time broken down
 * @param msecs milliseconds
 * @param timestamp 20 byte buffer (19 byte time + ending null)
 */
void make_timestamp (struct tm *split_time, unsigned msecs, char *timestamp);

/**
 * Create a timestamp for the current time
 *
 * @param timestamp 20 byte buffer (19 byte time + ending null)
 * @return 0 on success, valid errno otherwise.
 */
int make_current_timestamp (char *timestamp);


/**
 * Get absolute expiration timespec, given delay in ms
 *
 * @param timestamp 20 byte buffer (19 byte time + ending null)
 * @param ms  delay in msecs
 * @param ts  expiration time
 * @return 0 on success, valid errno otherwise.
 */
int get_expire_time (uint32_t ms, struct timespec *ts);

/**
 * Delay
 *
 * @param msecs  delay in msecs
 */
void delay_ms (unsigned msecs);

#ifdef TEST_SOCKET_TIMING

typedef struct {
	unsigned count;
	struct timeval total_time;
	struct timeval send_time;
} sst_totals_t;

typedef struct {
	struct timeval connect_time;
	struct timeval send_time;
} sst_times_t;

void sst_init_totals (void);
void sst_start_total_timing (sst_times_t *times);
void sst_start_send_timing (sst_times_t *times);
void sst_update_send_time (sst_times_t *times);
void sst_update_total_time (sst_times_t *times);
int sst_display_totals (void);

#endif

#endif

