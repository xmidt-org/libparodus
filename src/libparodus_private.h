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
#ifndef  _LIBPARODUS_PVT_H
#define  _LIBPARODUS_PVT_H

#include "libparodus.h"

/** 
 * @brief libparodus internal error rtn codes
 * 
 */
typedef enum {
	/** 
	 * @brief Error on libparodus_init
	 */
	LIBPD_ERR_INIT = -0x40000,
	/** 
	 * @brief Error on libparodus_init
	 * could not create new instance
	 */
	LIBPD_ERR_INIT_INST = -0x40001,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting receiver
	 */
	LIBPD_ERR_INIT_RCV = -0x42000,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting receiver
	 * null url specified
	 */
	LIBPD_ERR_INIT_RCV_NULL = -0x42001,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting receiver
	 * error creating socket
	 */
	LIBPD_ERR_INIT_RCV_CR = -0x42040,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting receiver
	 * error setting socket option
	 */
	LIBPD_ERR_INIT_RCV_OPT = -0x42080,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting receiver
	 * error binding to socket
	 */
	LIBPD_ERR_INIT_RCV_BIND = -0x420C0,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting sender
	 */
	LIBPD_ERR_INIT_SEND = -0x43000,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting sender
	 * null url specified
	 */
	LIBPD_ERR_INIT_SEND_NULL = -0x43001,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting sender
	 * error creating socket
	 */
	LIBPD_ERR_INIT_SEND_CREATE = -0x43040,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting sender
	 * error setting socket option
	 */
	LIBPD_ERR_INIT_SEND_SETOPT = -0x43080,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting sender
	 * error connecting to socket
	 */
	LIBPD_ERR_INIT_SEND_CONN = -0x430C0,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting termination socket
	 */
	LIBPD_ERR_INIT_TERMSOCK = -0x44000,
	/** 
	 * @brief Error on connect termination socket
	 * null url specified
	 */
	LIBPD_ERR_INIT_TERMSO_NULL = -0x44001,
	/** 
	 * @brief Error on connect termination socket
	 * error creating socket
	 */
	LIBPD_ERR_INIT_TERMSO_CREATE = -0x44040,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting termination socket
	 * error setting socket option
	 */
	LIBPD_ERR_INIT_TERMSO_SETOPT = -0x44080,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting termination socket
	 * error connecting to socket
	 */
	LIBPD_ERR_INIT_TERMSO_CONN = -0x440C0,
	/** 
	 * @brief Error on libparodus_init
	 * error creating wrp receiver thread
	 */
	LIBPD_ERR_INIT_RCV_THREAD = -0x45000,
	/** 
	 * @brief Error on libparodus_init
	 * error creating wrp receiver thread
	 * pthread_create error
	 */
	LIBPD_ERR_INIT_RCV_THREAD_PCR = -0x45040,
	/** 
	 * @brief Error on libparodus_init
	 * error creating wrp msg rcv queue
	 */
	LIBPD_ERR_INIT_QUEUE = -0x50000,
	/** 
	 * @brief Error on libparodus_init
	 * error creating rcv queue
	 */
	LIBPD_ERR_INIT_QCREATE = -0x51000,
	/** 
	 * @brief Error on libparodus_init
	 * invalid rcv queue size
	 */
	LIBPD_ERR_INIT_QCREATE_SZ = -0x51001,
	/** 
	 * @brief Error on libparodus init
	 * unable to allocate rcv q struct
	 */
	LIBPD_ERR_INIT_QCREATE_ALLOC_1 = -0x51002,
	/** 
	 * @brief Error on libparodus init
	 * unable to allocate rcv q msg array
	 */
	LIBPD_ERR_INIT_QCREATE_ALLOC_2 = -0x51003,
	/** 
	 * @brief Error on libparodus init
	 * unable to create mutex for rcv queue
	 */
	LIBPD_ERR_INIT_QCREATE_MUTEX = -0x51040,
	/** 
	 * @brief Error on libparodus init
	 * unable to create not_empty cond var for rcv queue
	 */
	LIBPD_ERR_INIT_QCREATE_NECOND = -0x51080,
	/** 
	 * @brief Error on libparodus init
	 * unable to create not_full cond var for rcv queue
	 */
	LIBPD_ERR_INIT_QCREATE_NFCOND = -0x510C0,
	/** 
	 * @brief Error on libparodus_init
	 * error sending registration msg
	 */
	LIBPD_ERR_INIT_REGISTER = -0x60000,
	/** 
	 * @brief Error on libparodus_init
	 * convert to struct error on send registration
	 */
	LIBPD_ERR_INIT_REG_CVT = -0x61001,
	/** 
	 * @brief Error on libparodus_init
	 * connect error on send registration
	 * only applies if connect_on_every_send
	 */
	LIBPD_ERR_INIT_REG_CONNECT = -0x61200,
	/** 
	 * @brief Error on libparodus_init
	 * connect error on send registration
	 * only applies if connect_on_every_send
	 * error creating socket
	 */
	LIBPD_ERR_INIT_REG_CONN_CR = -0x61240,
	/** 
	 * @brief Error on libparodus_init
	 * connect error on send registration
	 * only applies if connect_on_every_send
	 * error setting socket option
	 */
	LIBPD_ERR_INIT_REG_CONN_SETOPT = -0x61280,
	/** 
	 * @brief Error on libparodus_init
	 * connect error on send registration
	 * only applies if connect_on_every_send
	 * error connecting to socket
	 */
	LIBPD_ERR_INIT_REG_CONN_CONN = -0x612C0,
	/** 
	 * @brief Error on libparodus_init
	 * error on send registration
	 * socket send error
	 */
	LIBPD_ERR_INIT_REG_SEND = -0x61800,
	/** 
	 * @brief Error on libparodus_init
	 * error on send registration
	 * not all bytes sent
	 */
	LIBPD_ERR_INIT_REG_BYTE_CNT = -0x61801,
	/** 
	 * @brief Error on libparodus_init
	 * error on send registration
	 * nanomsg send error
	 */
	LIBPD_ERR_INIT_REG_SEND_NN = -0x61840,
	/** 
	 * @brief Error on libparodus_receive
	 */
	LIBPD_ERR_RCV = -0xA0000,
	/** 
	 * @brief Error on libparodus_receive
	 * null instance given
	 */
	LIBPD_ERR_RCV_NULL_INST = -0xA0001,
	/** 
	 * @brief Error on libparodus_receive
	 * run state error
	 */
	LIBPD_ERR_RCV_STATE = -0xA0002,
	/** 
	 * @brief Error on libparodus_receive
	 * not configured for receive
	 */
	LIBPD_ERR_RCV_CFG = -0xA0003,
	/** 
	 * @brief Error on libparodus_receive
	 * null msg received from wrp queue
	 */
	LIBPD_ERR_RCV_NULL_MSG = -0xA0004,
	/** 
	 * @brief Error on libparodus_receive
	 * wrp queue receive error
	 */
	LIBPD_ERR_RCV_QUEUE = -0xB0000,
	/** 
	 * @brief Error on libparodus_receive
	 * wrp queue receive error, libpd_qreceive
	 */
	LIBPD_ERR_RCV_QUEUE_RCV = -0xB3000,
	/** 
	 * @brief Error on libparodus receive
	 * internal error, null queue id
	 */
	LIBPD_ERR_RCV_QUEUE_NULL = -0xB3001,
	/** 
	 * @brief Error on libparodus receive
	 * error on queue cond wait var
	 */
	LIBPD_ERR_RCV_QUEUE_CONDWAIT = -0xB3040,
	/** 
	 * @brief Error on libparodus_close_receiver
	 */
	LIBPD_ERR_CLOSE_RCV = -0xE0000,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * null instance given
	 */
	LIBPD_ERR_CLOSE_RCV_NULL_INST = -0xE0001,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * run state error
	 */
	LIBPD_ERR_CLOSE_RCV_STATE = -0xE0002,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * not configured for receive
	 */
	LIBPD_ERR_CLOSE_RCV_CFG = -0xE0003,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * queue send error
	 */
	LIBPD_ERR_CLOSE_RCV_QSEND = -0xE2000,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * internal error, null queue id in queue send
	 */
	LIBPD_ERR_CLOSE_RCV_QSEND_NULL = -0xE2001,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * queue send error, timed out
	 */
	LIBPD_ERR_CLOSE_RCV_TIMEDOUT = -0xE2002,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * error on cond wait in queue send
	 */
	LIBPD_ERR_CLOSE_RCV_QSEND_CONDWAIT = -0xE2040,
	/** 
	 * @brief Error on libparodus_shutdown
	 */
	LIBPD_ERR_SHUTDOWN = -0x100000,
	/** 
	 * @brief Error on libparodus_shutdown
	 * run state error
	 */
	LIBPD_ERR_SHUT_STATE = -0x100001,
	/** 
	 * @brief Error on libparodus_send
	 */
	LIBPD_ERR_SEND = -0x140000,
	/** 
	 * @brief Error on libparodus_send
	 * null instance given
	 */
	LIBPD_ERR_SEND_NULL_INST = -0x140001,
	/** 
	 * @brief Error on libparodus_send
	 * run state error
	 */
	LIBPD_ERR_SEND_STATE = -0x140002,
	/** 
	 * @brief Error on libparodus_send
	 * convert to struct error
	 */
	LIBPD_ERR_SEND_CONVERT = -0x141001,
	/** 
	 * @brief Error on libparodus_send
	 * connect sender error
	 * only applies if connect_on_every_send
	 */
	LIBPD_ERR_SEND_CONNECT = -0x141200,
	/** 
	 * @brief Error on libparodus_init
	 * connect error on send
	 * only applies if connect_on_every_send
	 * error creating socket
	 */
	LIBPD_ERR_SEND_CONN_CR = -0x141240,
	/** 
	 * @brief Error on libparodus_init
	 * connect error on send
	 * only applies if connect_on_every_send
	 * error setting socket option
	 */
	LIBPD_ERR_SEND_CONN_SETOPT = -0x141280,
	/** 
	 * @brief Error on libparodus_init
	 * connect error on send
	 * only applies if connect_on_every_send
	 * error connecting to socket
	 */
	LIBPD_ERR_SEND_CONN_CONN = -0x1412C0,
	/** 
	 * @brief Error on libparodus_send
	 * socket send error
	 */
	LIBPD_ERR_SEND_SOCK_SEND = -0x141800,
	/** 
	 * @brief Error on libparodus_send
	 * not all bytes sent
	 */
	LIBPD_ERR_SEND_BYTE_CNT = -0x141801,
	/** 
	 * @brief Error on libparodus_send
	 * nanomsg send error
	 */
	LIBPD_ERR_SEND_NN = -0x141840,
} __libpd_err_t;


/**
 * Retrieve the err of last libparodus call
 *
 * @param exterr exterr of last libparodus call
 * @return err of last libparodus call
 */
int __libparodus_err (libpd_instance_t instance, int *exterr);


#endif
