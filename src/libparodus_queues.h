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
#ifndef  _LIBPARODUS_QUEUES_H
#define  _LIBPARODUS_QUEUES_H

#include <errno.h>

typedef void *libpd_mq_t;

/** 
 * @brief liboarodus error rtn codes
 * 
 */
typedef enum {
	/** 
	 * @brief Error on libpd_qcreate
	 */
	LIBPD_QERR_CREATE = -0x1000,
	/** 
	 * @brief Error on libpd_qcreate
	 * invalid queue size
	 */
	LIBPD_QERR_CREATE_INVAL_SZ = -0x1001,
	/** 
	 * @brief Error on libpd_qcreate
	 * unable to allocate q struct
	 */
	LIBPD_QERR_CREATE_ALLOC_1 = -0x1002,
	/** 
	 * @brief Error on libpd_qcreate
	 * unable to allocate q msg array
	 */
	LIBPD_QERR_CREATE_ALLOC_2 = -0x1003,
	/** 
	 * @brief Error on libpd_qcreate
	 * unable to create mutex
	 */
	LIBPD_QERR_CREATE_MUTEX = -0x1040,
	/** 
	 * @brief Error on libpd_qcreate
	 * unable to create not_empty cond var
	 */
	LIBPD_QERR_CREATE_NECOND = -0x1080,
	/** 
	 * @brief Error on libpd_qcreate
	 * unable to create not_full cond var
	 */
	LIBPD_QERR_CREATE_NFCOND = -0x10C0,
	/** 
	 * @brief Error on libpd_qsend
	 */
	LIBPD_QERR_SEND = -0x2000,
	/** 
	 * @brief Error on libpd_qsend
	 * null queue id provided
	 */
	LIBPD_QERR_SEND_NULL = -0x2001,
	/** 
	 * @brief Error on libpd_qsend
	 * error on cond wait
	 */
	LIBPD_QERR_SEND_CONDWAIT = -0x2040,
	/** 
	 * @brief Error on libpd_qreceive
	 */
	LIBPD_QERR_RCV = -0x3000,
	/** 
	 * @brief Error on libpd_qreceive
	 * null queue id provided
	 */
	LIBPD_QERR_RCV_NULL = -0x3001,
	/** 
	 * @brief Error on libpd_qreceive
	 * error on cond wait
	 */
	LIBPD_QERR_RCV_CONDWAIT = -0x3040

} libpd_qerror_t;


/**
 * Create a queue
 *
 * @param mq pointer to receive queue object that must be provided
 *   to all subsequent API calls.
 * @param queue_name name of queue
 * @param max_msgs maximum number of messages queue can hold
 * @param exterr extra error info
 * @return 0 on success, valid libpd_qerror_t (LIBPD_QERR_CREATE_ ...)  otherwise. 
 */
int libpd_qcreate (libpd_mq_t *mq, const char *queue_name, 
	unsigned max_msgs, int *exterr);

typedef void free_msg_func_t (void *msg);

/**
 * Destroy queue
 *
 * @param mq pointer to queue object
 * @param free_msg_func pointer to function that frees the queue. can be 'free'
 * @return always returns 0
 */
int libpd_qdestroy (libpd_mq_t *mq, free_msg_func_t *free_msg_func);

/**
 * Send message on queue
 *
 * @param mq queue object
 * @param msg pointer to message to be sent
 * @param timeout_ms maximum wait time for message to be placed on the queue
 * @param exterr extra error info
 * @return 0 on success, valid libpd_qerror_t (LIBPD_QERR_SEND_ ...)  otherwise. 
 */
int libpd_qsend (libpd_mq_t mq, void *msg, unsigned timeout_ms, int *exterr);

/**
 * Receive message from queue
 *
 * @param mq queue object  
 * @param msg pointer to variable that will receive message pointer
 *    this message must be freed
 * @param timeout_ms maximum wait time for message to be placed on the queue
 * @param exterr extra error info
 * @return 0 on success, valid libpd_qerror_t (LIBPD_QERR_RCV_ ...)  otherwise. 
 */
int libpd_qreceive (libpd_mq_t mq, void **msg, unsigned timeout_ms, int *exterr);

#endif
