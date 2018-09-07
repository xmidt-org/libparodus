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
#ifndef  _LIBPARODUS_H
#define  _LIBPARODUS_H

#include <wrp-c/wrp-c.h>
#include "libparodus_log.h"

/**
 * This module is linked with the client, and provides connectivity
 * to the parodus service.
 */ 

typedef struct {
	const char *service_name;
	bool receive;
	int  keepalive_timeout_secs; 
	const char *parodus_url;
	const char *client_url;
	unsigned test_flags;  // always 0 except when testing
} libpd_cfg_t;

typedef void *libpd_instance_t;


/** 
 * @brief libparodus error rtn codes
 * 
 */
typedef enum {
	/** 
	 * @brief Error on libparodus_init
	 * could not create new instance
	 */
	LIBPD_ERROR_INIT_INST = -101,
	/** 
	 * @brief Error on libparodus_init
	 * invalid cfg parameter
	 */
	LIBPD_ERROR_INIT_CFG = -102,
	/** 
	 * @brief Error on libparodus_init
	 * error connecting
	 */
	LIBPD_ERROR_INIT_CONNECT = -103,
	/** 
	 * @brief Error on libparodus_init
	 * error creating wrp receiver thread
	 */
	LIBPD_ERROR_INIT_RCV_THREAD = -104,
	/** 
	 * @brief Error on libparodus_init
	 * error creating wrp msg rcv queue
	 */
	LIBPD_ERROR_INIT_QUEUE = -105,
	/** 
	 * @brief Error on libparodus_init
	 * error sending registration msg
	 */
	LIBPD_ERROR_INIT_REGISTER = -106,
	/** 
	 * @brief Error on libparodus_receive
	 * null instance given
	 */
	LIBPD_ERROR_RCV_NULL_INST = -201,
	/** 
	 * @brief Error on libparodus_receive
	 * run state error
	 */
	LIBPD_ERROR_RCV_STATE = -202,
	/** 
	 * @brief Error on libparodus_receive
	 * not configured for receive
	 */
	LIBPD_ERROR_RCV_CFG = -203,
	/** 
	 * @brief Error on libparodus_receive
	 * receive error
	 */
	LIBPD_ERROR_RCV_RCV = -204,
	/** 
	 * @brief Error on libparodus_receive
	 * thread limit exceeded
	 */
	LIBPD_ERROR_RCV_THR_LIMIT = -205,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * null instance given
	 */
	LIBPD_ERROR_CLOSE_RCV_NULL_INST = -301,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * run state error
	 */
	LIBPD_ERROR_CLOSE_RCV_STATE = -302,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * not configured for receive
	 */
	LIBPD_ERROR_CLOSE_RCV_CFG = -303,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * queue send error, timed out
	 */
	LIBPD_ERROR_CLOSE_RCV_TIMEDOUT = -304,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * unable to send close recevier message
	 */
	LIBPD_ERROR_CLOSE_RCV_SEND = -305,
	/** 
	 * @brief Error on libparodus_close_receiver
	 * thread limit exceeded
	 */
	LIBPD_ERROR_CLOSE_RCV_THR_LIMIT = -306,
	/** 
	 * @brief Error on libparodus_send
	 * null instance given
	 */
	LIBPD_ERROR_SEND_NULL_INST = -401,
	/** 
	 * @brief Error on libparodus_send
	 * run state error
	 */
	LIBPD_ERROR_SEND_STATE = -402,
	/** 
	 * @brief Error on libparodus_send
	 * invalid WRP message
	 */
	LIBPD_ERROR_SEND_WRP_MSG = -403,
	/** 
	 * @brief Error on libparodus_send
	 * socket send error
	 */
	LIBPD_ERROR_SEND_SOCKET = -404,
	/** 
	 * @brief Error on libparodus_send
	 * thread limit exceeded
	 */
	LIBPD_ERROR_SEND_THR_LIMIT = -405
} libpd_error_t;

/**
 * Initialize the parodus wrp interface
 *
 * @param instance pointer to receive instance object that must be provided
 *   to all subsequent API calls.
 * @param cfg configuration information: service_name must be provided,
 * @return 0 on success, else:
 *		LIBPD_ERROR_INIT_INST = -101, could not create new instance
 *		LIBPD_ERROR_INIT_CFG = -102, invalid config parameter
 *		LIBPD_ERROR_INIT_CONNECT = -103, error connecting
 *		LIBPD_ERROR_INIT_RCV_THREAD = -104, error creating wrp receiver thread
 *		LIBPD_ERROR_INIT_QUEUE = -105, error creating wrp msg receive queue
 *		LIBPD_ERROR_INIT_REGISTER = -106, error sending registration msg
 *
 * @note libparodus_shutdown must be called even if there is an error
 * on libparodus_init   
 */
int libparodus_init (libpd_instance_t *instance, libpd_cfg_t *libpd_cfg);

/**
 *  Receives the next message in the queue that was sent to this service, waiting
 *  the prescribed number of milliseconds before returning.
 *
 *  @note msg will be set to NULL if no message is present during the time
 *  allotted.
 *
 *  @param instance instance object
 *  @param msg the pointer to receive the next msg struct
 *  @param ms the number of milliseconds to wait for the next message
 *
 *  @return 0 on success, 2 if closed msg received, 1 if timed out, else:
 *		LIBPD_ERROR_RCV_NULL_INST = -201, null instance given
 *		LIBPD_ERROR_RCV_STATE = -202, run state error, not running
 *		LIBPD_ERROR_RCV_CFG = -203, not configured for receive
 *		LIBPD_ERROR_RCV_RCV = -204, receive error
 *
 *  @note don't free the msg when return is 2. 
 */
int libparodus_receive (libpd_instance_t instance, wrp_msg_t **msg, uint32_t ms);

/**
 * Sends a close message to the receiver
 *
 *  @param instance instance object
 *  @return 0 on success,  else:
 *		LIBPD_ERROR_CLOSE_RCV_NULL_INST = -301, null instance given
 *		LIBPD_ERROR_CLOSE_RCV_STATE = -302, run state error, not running
 *		LIBPD_ERROR_CLOSE_RCV_CFG = -303, not configured for receive
 *		LIBPD_ERROR_CLOSE_RCV_TIMEDOUT = -304, timed out on queue send
 *		LIBPD_ERROR_CLOSE_RCV_SEND = -305, unable to send close receiver msg
 */
int libparodus_close_receiver (libpd_instance_t instance);

/**
 * Shut down the parodus wrp interface
 *
 * @param instance instance object
 * @return always 0
*/
int libparodus_shutdown (libpd_instance_t *instance);


/**
 * Send a wrp message to the parodus service
 *
 * @param instance instance object
 * @param msg wrp message to send
 *
 * @return 0 on success, else:
 *		LIBPD_ERROR_SEND_NULL_INST = -501, null instance given
 *		LIBPD_ERROR_SEND_STATE = -502, run state error, not running
 *		LIBPD_ERROR_SEND_WRP_MSG = -503, invalid wrp message
 *		LIBPD_ERROR_SEND_SOCKET = -504, socket send error
 */
int libparodus_send (libpd_instance_t instance, wrp_msg_t *msg);

/**
 * Return the string value of a libparodus error code
 *
 * @param err libparodus error code
 *
 * @return string value of the specified error code
 */
const char *libparodus_strerror (libpd_error_t err);

#endif
