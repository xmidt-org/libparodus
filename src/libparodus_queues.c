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

#define DBG_QMEM 1

#if DBG_QMEM
#include <assert.h>
#endif

typedef struct queue {
	const char *queue_name;
	unsigned max_msgs;
	int msg_count;
	pthread_mutex_t mutex;
	pthread_cond_t not_empty_cond;
	pthread_cond_t not_full_cond;
	void **msg_array;
	int head_index;
	int tail_index;
#if DBG_QMEM
    unsigned saved_max_msgs;
    void **saved_msg_array;
#endif
} queue_t;

#if DBG_QMEM
#define check_max_msgs(q) assert (q->max_msgs == q->saved_max_msgs)
#define check_msg_array(q) assert (q->msg_array == q->saved_msg_array)
#define validate_head_index(q) assert ( \
  (q->head_index >= -1) && (q->head_index < (int)q->max_msgs) \
  )
#define validate_tail_index(q) assert ( \
  (q->tail_index >= -1) && (q->tail_index < (int)q->max_msgs) \
  )
#define CHECK_QMEM(q) {\
  check_max_msgs(q); check_msg_array(q); validate_head_index(q); validate_tail_index(q); \
 }
#else
#define check_max_msgs(q)
#define check_msg_array(q)
#define validate_head_index(q)
#define validate_tail_index(q)
#define CHECK_QMEM(q)
#endif

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

	err = pthread_cond_init (&newq->not_empty_cond, NULL);
	if (err != 0) {
		*exterr = err;
		libpd_log_err (LEVEL_ERROR, err, ("Error creating not_empty_cond for queue %s\n",
			queue_name));
		pthread_mutex_destroy (&newq->mutex);
		free (newq);
		return LIBPD_QERR_CREATE_NECOND;
	}

	err = pthread_cond_init (&newq->not_full_cond, NULL);
	if (err != 0) {
		*exterr = err;
		libpd_log_err (LEVEL_ERROR, err, ("Error creating not_full_cond for queue %s\n",
			queue_name));
		pthread_mutex_destroy (&newq->mutex);
		pthread_cond_destroy (&newq->not_empty_cond);
		free (newq);
		return LIBPD_QERR_CREATE_NFCOND;
	}

	newq->msg_array = malloc (array_size);
	if (NULL == newq->msg_array) {
		libpd_log (LEVEL_ERROR, ("Unable to allocate memory(2) for queue %s\n",
			queue_name));
		pthread_mutex_destroy (&newq->mutex);
		pthread_cond_destroy (&newq->not_empty_cond);
		pthread_cond_destroy (&newq->not_full_cond);
		free (newq);
		return LIBPD_QERR_CREATE_ALLOC_2;
	}
#if DBG_QMEM
    newq->saved_max_msgs = newq->max_msgs;
    newq->saved_msg_array = newq->msg_array;
#endif
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
	pthread_cond_destroy (&q->not_empty_cond);
	pthread_cond_destroy (&q->not_full_cond);
	pthread_mutex_unlock (&q->mutex);
	pthread_mutex_destroy (&q->mutex);
	free (q);
	*mq = NULL;
	return 0;
}

int libpd_qsend (libpd_mq_t mq, void *msg, unsigned timeout_ms, int *exterr)
{
	queue_t *q = (queue_t*) mq;
	struct timespec ts;
	int rtn;

	*exterr = 0;
	if (NULL == mq)
		return LIBPD_QERR_SEND_NULL;
	CHECK_QMEM(q);
	pthread_mutex_lock (&q->mutex);
	while (true) {
		if (enqueue_msg (q, msg))
			break;
		rtn = get_expire_time (timeout_ms, &ts);
		if (rtn != 0) {
			*exterr = rtn;
			libpd_log_err (LEVEL_ERROR, rtn, 
				("gettimeofday error waiting to send queue\n"));
			pthread_mutex_unlock (&q->mutex);
			return LIBPD_QERR_SEND_EXPTIME;
		}
		CHECK_QMEM(q);
		rtn = pthread_cond_timedwait (&q->not_full_cond, &q->mutex, &ts);
		if (rtn != 0) {
			if (rtn == ETIMEDOUT) {
				pthread_mutex_unlock (&q->mutex);
				return 1;
			}
			*exterr = rtn;
			libpd_log_err (LEVEL_ERROR, rtn, 
				("pthread_cond_timedwait error waiting for not_full_cond\n"));
			pthread_mutex_unlock (&q->mutex);
			return LIBPD_QERR_SEND_CONDWAIT;
		}
	}
	if (q->msg_count == 1)
		pthread_cond_signal (&q->not_empty_cond);
	pthread_mutex_unlock (&q->mutex);
	return 0;
}

int libpd_qreceive (libpd_mq_t mq, void **msg, unsigned timeout_ms, int *exterr)
{
	queue_t *q = (queue_t*) mq;
	struct timespec ts;
	void *msg__;
	int rtn;

	*exterr = 0;
	if (NULL == mq)
		return LIBPD_QERR_RCV_NULL;
	CHECK_QMEM(q);
	pthread_mutex_lock (&q->mutex);
	while (true) {
		msg__ = dequeue_msg (q);
		if (NULL != msg__)
			break;
		rtn = get_expire_time (timeout_ms, &ts);
		if (rtn != 0) {
			*exterr = rtn;
			libpd_log_err (LEVEL_ERROR, rtn, 
				("gettimeofday error waiting to receive on queue\n"));
			pthread_mutex_unlock (&q->mutex);
			return LIBPD_QERR_RCV_EXPTIME;
		}
		CHECK_QMEM(q);
		rtn = pthread_cond_timedwait (&q->not_empty_cond, &q->mutex, &ts);
		if (rtn != 0) {
			if (rtn == ETIMEDOUT) {
				pthread_mutex_unlock (&q->mutex);
				return 1;
			}
			*exterr = rtn;
			libpd_log_err (LEVEL_ERROR, rtn, 
				("pthread_cond_timedwait error waiting for not_empty_cond\n"));
			pthread_mutex_unlock (&q->mutex);
			return LIBPD_QERR_RCV_CONDWAIT;
		}
	}
	*msg = msg__;
	if ((q->msg_count+1) == (int)q->max_msgs)
		pthread_cond_signal (&q->not_full_cond);
	pthread_mutex_unlock (&q->mutex);
	return 0;
}

