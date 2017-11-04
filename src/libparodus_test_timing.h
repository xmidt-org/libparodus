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
#ifndef  _LIBPARODUS_TEST_TIMING_H
#define  _LIBPARODUS_TEST_TIMING_H

#include <sys/time.h>

//#define TEST_SOCKET_TIMING 1

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
