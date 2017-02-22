/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

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


int libpd_qcreate (libpd_mq_t *mq, const char *queue_name, 
	unsigned max_msgs, int *exterr);

typedef void free_msg_func_t (void *msg);

int libpd_qdestroy (libpd_mq_t *mq, free_msg_func_t *free_msg_func);

// returns 0 on success, 1 on timeout, or other error
int libpd_qsend (libpd_mq_t mq, void *msg, unsigned timeout_ms, int *exterr);

// returns 0 on success, 1 on timeout, or other error
int libpd_qreceive (libpd_mq_t mq, void **msg, unsigned timeout_ms, int *exterr);

#endif
