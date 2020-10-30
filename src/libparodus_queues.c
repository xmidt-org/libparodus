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

#include "libparodus_queues.h"
#include "libparodus_time.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include "libparodus_log.h"

typedef struct queue {
	const char *queue_name;
	unsigned max_msgs;
	int msg_count;
	pthread_mutex_t mutex;
	void **msg_array;
	int head_index;
	int tail_index;
} queue_t;

int libpd_qcreate (libpd_mq_t *mq, const char *queue_name, 
	unsigned max_msgs, int *exterr)
{
	int err;
	unsigned array_size;
	queue_t *newq;

	*exterr = 0;
	*mq = NULL;
	if (max_msgs < 2) {
		libpd_log (LEVEL_ERROR, 
			("Error creating queue %s: max_msgs(%u) should be at least 2\n",
			queue_name, max_msgs));
		return LIBPD_QERR_CREATE_INVAL_SZ;
	}
		
	array_size = max_msgs * sizeof(void*);
	newq = (queue_t*) malloc (sizeof(queue_t));

	if (NULL == newq) {
		libpd_log (LEVEL_ERROR, ("Unable to allocate memory(1) for queue %s\n",
			queue_name));
		return LIBPD_QERR_CREATE_ALLOC_1;
	}

	newq->queue_name = queue_name;
	newq->max_msgs = max_msgs;
	newq->msg_count = 0;
	newq->head_index = -1;
	newq->tail_index = -1;

	err = pthread_mutex_init (&newq->mutex, NULL);
	if (err != 0) {
		*exterr = err;
		libpd_log_err (LEVEL_ERROR, err, ("Error creating mutex for queue %s\n",
			queue_name));
		free (newq);
		return LIBPD_QERR_CREATE_MUTEX;
	}

	newq->msg_array = malloc (array_size);
	if (NULL == newq->msg_array) {
		libpd_log (LEVEL_ERROR, ("Unable to allocate memory(2) for queue %s\n",
			queue_name));
		pthread_mutex_destroy (&newq->mutex);
		free (newq);
		return LIBPD_QERR_CREATE_ALLOC_2;
	}

	*mq = (libpd_mq_t) newq;
	return 0;
}



static bool enqueue_msg (queue_t *q, void *msg)
{
	if (q->msg_count == 0) {
		q->msg_array[0] = msg;
		q->head_index = 0;
		q->tail_index = 0;
		q->msg_count = 1;
		return true;
	}
	if (q->msg_count >= (int)q->max_msgs)
		return false;
	q->tail_index += 1;
	if (q->tail_index >= (int)q->max_msgs)
		q->tail_index = 0;
	q->msg_array[q->tail_index] = msg;
	q->msg_count += 1;
	return true;
}

static void *dequeue_msg (queue_t *q)
{
	void *msg;
	if (q->msg_count <= 0)
		return NULL;
	msg = q->msg_array[q->head_index];
	q->head_index += 1;
	if (q->head_index >= (int)q->max_msgs)
		q->head_index = 0;
	q->msg_count -= 1;
	return msg;
}

int libpd_qdestroy (libpd_mq_t *mq, free_msg_func_t *free_msg_func)
{
	queue_t *q = (queue_t*) *mq;
	void *msg;
	if (NULL == *mq)
		return 0;
	pthread_mutex_lock (&q->mutex);
	if (NULL != free_msg_func) {
		msg = dequeue_msg (q);
		while (NULL != msg) {
			(*free_msg_func) (msg);
			msg = dequeue_msg (q);
		}
	}
	free (q->msg_array);
	pthread_mutex_unlock (&q->mutex);
	pthread_mutex_destroy (&q->mutex);
	free (q);
	*mq = NULL;
	return 0;
}

void backoff (unsigned *current_wait)
{
  unsigned current = *current_wait;
  if (current == 0)
    current = 10;
  else if (current == 10)
    current = 20;
  else if (current == 20)
    current = 50;
  else if (current == 50)
    current = 100;
  else if (current == 100)
    current = 200;
  else
    current = 500;
  delay_ms (current);
  *current_wait = current;
}

int libpd_qsend (libpd_mq_t mq, void *msg, unsigned timeout_ms, int *exterr)
{
	queue_t *q = (queue_t*) mq;
	unsigned current_wait = 0;
        unsigned total_wait = 0;

	*exterr = 0;
	if (NULL == mq)
		return LIBPD_QERR_SEND_NULL;
	while (true) {
		pthread_mutex_lock (&q->mutex);
		if (enqueue_msg (q, msg))
			break;
		pthread_mutex_unlock (&q->mutex);
		if (total_wait >= timeout_ms)
			return 1;
		backoff (&current_wait);
		total_wait += current_wait;
	}
	pthread_mutex_unlock (&q->mutex);
	return 0;
}

int libpd_qreceive (libpd_mq_t mq, void **msg, unsigned timeout_ms, int *exterr)
{
	queue_t *q = (queue_t*) mq;
	unsigned current_wait = 0;
        unsigned total_wait = 0;
	void *msg__;

	*exterr = 0;
	if (NULL == mq)
		return LIBPD_QERR_RCV_NULL;
	while (true) {
		pthread_mutex_lock (&q->mutex);
		msg__ = dequeue_msg (q);
		pthread_mutex_unlock (&q->mutex);
		if (NULL != msg__)
			break;
		if (total_wait >= timeout_ms)
			return 1;
		backoff (&current_wait);
		total_wait += current_wait;
	}
	*msg = msg__;
	return 0;
}

