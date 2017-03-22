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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include "libparodus.h"
#include "libparodus_private.h"
#include "libparodus_time.h"
#include <pthread.h>
#include "libparodus_queues.h"

//#define PARODUS_SERVICE_REQUIRES_REGISTRATION 1

#define PARODUS_SERVICE_URL "tcp://127.0.0.1:6666"
//#define PARODUS_URL "ipc:///tmp/parodus_server.ipc"

#define PARODUS_CLIENT_URL "tcp://127.0.0.1:6667"
//#define CLIENT_URL "ipc:///tmp/parodus_client.ipc"

#define DEFAULT_KEEPALIVE_TIMEOUT_SECS 65

#define URL_SIZE 32
#define QNAME_SIZE 50

typedef struct {
	int run_state;
	int err;		// err of last api call
	int exterr; // exterr of last api call
	const char *parodus_url;
	const char *client_url;
	int keep_alive_count;
	int reconnect_count;
	libpd_cfg_t cfg;
	bool connect_on_every_send; // always false, currently
	int rcv_sock;
	int stop_rcv_sock;
	int send_sock;
	char wrp_queue_name[QNAME_SIZE];
	libpd_mq_t wrp_queue;
	pthread_t wrp_receiver_tid;
	pthread_mutex_t send_mutex;
	bool auth_received;
} __instance_t;

#define SOCK_SEND_TIMEOUT_MS 2000

#define MAX_RECONNECT_RETRY_DELAY_SECS 63

#define END_MSG "---END-PARODUS---\n"
static const char *end_msg = END_MSG;

static char *closed_msg = "---CLOSED---\n";

typedef struct {
	int len;
	char *msg;
} raw_msg_t;

#define WRP_QUEUE_SEND_TIMEOUT_MS	2000
#define WRP_QNAME_HDR "/LIBPD_WRP_QUEUE"
#define WRP_QUEUE_SIZE 50

const char *wrp_qname_hdr = WRP_QNAME_HDR;

int flush_wrp_queue (libpd_mq_t wrp_queue, uint32_t delay_ms);
static int wrp_sock_send (__instance_t *inst, wrp_msg_t *msg);
static void *wrp_receiver_thread (void *arg);
static void libparodus_shutdown__ (__instance_t *inst);

#define RUN_STATE_RUNNING		1234
#define RUN_STATE_DONE			-1234

int __libparodus_err (libpd_instance_t instance, int *exterr)
{
	__instance_t *inst = (__instance_t*) instance;
	int err = 0;
	int oserr = 0;
	if (NULL != inst) {
		err = inst->err;
		oserr = inst->exterr;
	}
	if (NULL != exterr)
		*exterr = oserr;
	return err;
}

static void getParodusUrl(__instance_t *inst)
{
	inst->parodus_url = inst->cfg.parodus_url;
	inst->client_url = inst->cfg.client_url;
	if (NULL == inst->parodus_url)
		inst->parodus_url = PARODUS_SERVICE_URL;
	if (NULL == inst->client_url)
		inst->client_url = PARODUS_CLIENT_URL;
  libpd_log (LEVEL_INFO, ("LIBPARODUS: parodus url is  %s\n", inst->parodus_url));
  libpd_log (LEVEL_INFO, ("LIBPARODUS: client url is  %s\n", inst->client_url));
}

static __instance_t *make_new_instance (libpd_cfg_t *cfg)
{
	size_t qname_len;
	__instance_t *inst = (__instance_t*) malloc (sizeof (__instance_t));
	if (NULL == inst)
		return NULL;
	qname_len = strlen(wrp_qname_hdr) + strlen(cfg->service_name) + 1;
	if (qname_len >= QNAME_SIZE)
		return NULL;
	memset ((void*) inst, 0, sizeof(__instance_t));
	pthread_mutex_init (&inst->send_mutex, NULL);
	//inst->cfg = *cfg;
	memcpy (&inst->cfg, cfg, sizeof(libpd_cfg_t));
	getParodusUrl (inst);
	sprintf (inst->wrp_queue_name, "%s.%s", wrp_qname_hdr, cfg->service_name);
	return inst;
}

static void destroy_instance (libpd_instance_t *instance)
{
	__instance_t *inst;
	if (NULL != instance) {
		inst = (__instance_t *) *instance;
		if (NULL != inst) {
			pthread_mutex_destroy (&inst->send_mutex);
			free (inst);
			*instance = NULL;
		}
	}
}

bool is_auth_received (libpd_instance_t instance)
{
	__instance_t *inst = (__instance_t *) instance;
	return inst->auth_received;
}

#define SHUTDOWN_SOCKET(sock) \
	if ((sock) != -1) \
		nn_shutdown ((sock), 0); \
	(sock) = 0;

void shutdown_socket (int *sock)
{
	if (*sock >= 0) {
		nn_shutdown (*sock, 0);
		nn_close (*sock);
	}
	*sock = -1;
}

typedef enum {
	/** 
	 * @brief Error on connect_receiver
	 * null url given
	 */
	CONN_RCV_ERR_NULL_URL = -1,
	/** 
	 * @brief Error on connect_receiver
	 * error creating socket
	 */
	CONN_RCV_ERR_CREATE = -0x40,
	/** 
	 * @brief Error on connect_receiver
	 * error setting socket option
	 */
	CONN_RCV_ERR_SETOPT = -0x80,
	/** 
	 * @brief Error on connect_receiver
	 * error binding to socket
	 */
	CONN_RCV_ERR_BIND = -0xC0
} conn_rcv_error_t;

/**
 * Open receive socket and bind to it.
 */
int connect_receiver (const char *rcv_url, int keepalive_timeout_secs, int *exterr)
{
	int rcv_timeout;
	int sock;

	*exterr = 0;
	if (NULL == rcv_url) {
		return CONN_RCV_ERR_NULL_URL;
	}
  sock = nn_socket (AF_SP, NN_PULL);
	if (sock < 0) {
		*exterr = errno;
		libpd_log_err (LEVEL_ERROR, errno, ("Unable to create rcv socket %s\n", rcv_url));
 		return CONN_RCV_ERR_CREATE;
	}
	if (keepalive_timeout_secs > 0) { 
		rcv_timeout = keepalive_timeout_secs * 1000;
		if (nn_setsockopt (sock, NN_SOL_SOCKET, NN_RCVTIMEO, 
					&rcv_timeout, sizeof (rcv_timeout)) < 0) {
			*exterr = errno;
			libpd_log_err (LEVEL_ERROR, errno, ("Unable to set socket timeout: %s\n", rcv_url));
			shutdown_socket (&sock);
 			return CONN_RCV_ERR_SETOPT;
		}
	}
  if (nn_bind (sock, rcv_url) < 0) {
		*exterr = errno;
		libpd_log_err (LEVEL_ERROR, errno, ("Unable to bind to receive socket %s\n", rcv_url));
		shutdown_socket (&sock);
		return CONN_RCV_ERR_BIND;
	}
	return sock;
}

typedef enum {
	/** 
	 * @brief Error on connect_sender
	 * null url specified
	 */
	CONN_SEND_ERR_NULL = -0x01,
	/** 
	 * @brief Error on connect_sender
	 * error creating socket
	 */
	CONN_SEND_ERR_CREATE = -0x40,
	/** 
	 * @brief Error on connect_sender
	 * error setting socket option
	 */
	CONN_SEND_ERR_SETOPT = -0x80,
	/** 
	 * @brief Error on connect_sender
	 * error connecting to socket
	 */
	CONN_SEND_ERR_CONN = -0xC0
} conn_send_error_t;

/**
 * Open send socket and connect to it.
 */
int connect_sender (const char *send_url, int *exterr)
{
	int sock;
	int send_timeout = SOCK_SEND_TIMEOUT_MS;

	*exterr = 0;
	if (NULL == send_url) {
		return CONN_SEND_ERR_NULL;
	}
  sock = nn_socket (AF_SP, NN_PUSH);
	if (sock < 0) {
		*exterr = errno;
		libpd_log_err (LEVEL_ERROR, errno, ("Unable to create send socket: %s\n", send_url));
 		return CONN_SEND_ERR_CREATE;
	}
	if (nn_setsockopt (sock, NN_SOL_SOCKET, NN_SNDTIMEO, 
				&send_timeout, sizeof (send_timeout)) < 0) {
		*exterr = errno;
		libpd_log_err (LEVEL_ERROR, errno, ("Unable to set socket timeout: %s\n", send_url));
		shutdown_socket (&sock);
 		return CONN_SEND_ERR_SETOPT;
	}
  if (nn_connect (sock, send_url) < 0) {
		*exterr = errno;
		libpd_log_err (LEVEL_ERROR, errno, ("Unable to connect to send socket %s\n",
			send_url));
		shutdown_socket (&sock);
		return CONN_SEND_ERR_CONN;
	}
	return sock;
}

static int create_thread (pthread_t *tid, void *(*thread_func) (void*),
	__instance_t *inst)
{
	int rtn = pthread_create (tid, NULL, thread_func, (void*) inst);
	if (rtn != 0) {
		libpd_log_err (LEVEL_ERROR, rtn, ("Unable to create thread\n"));
	}
	return rtn; 
}

static bool is_closed_msg (wrp_msg_t *msg)
{
	return (msg->msg_type == WRP_MSG_TYPE__REQ) &&
		(strcmp (msg->u.req.dest, closed_msg) == 0);
}

static void wrp_free (void *msg)
{
	wrp_msg_t *wrp_msg;
	if (NULL == msg)
		return;
	wrp_msg = (wrp_msg_t *) msg;
	if (is_closed_msg (wrp_msg))
		free (wrp_msg);
	else
		wrp_free_struct (wrp_msg);
}

typedef enum {
	/** 
	 * @brief Error on sock_send
	 * not all bytes sent
	 */
	SOCK_SEND_ERR_BYTE_CNT = -0x01,
	/** 
	 * @brief Error on sock_send
	 * nn_send error
	 */
	SOCK_SEND_ERR_NN = -0x40
} sock_send_error_t;

typedef enum {
	/** 
	 * @brief Error on wrp_sock_send
	 * convert to struct error
	 */
	WRP_SEND_ERR_CONVERT = -0x01,
	/** 
	 * @brief Error on wrp_sock_send
	 * connect sender error
	 * only applies if connect_on_every_send
	 */
	WRP_SEND_ERR_CONNECT = -0x200,
	/** 
	 * @brief Error on wrp_sock_send
	 * socket send error
	 */
	WRP_SEND_ERR_SOCK_SEND = -0x800,
	/** 
	 * @brief Error on sock_send
	 * not all bytes sent
	 */
	WRP_SEND_ERR_BYTE_CNT = -0x801,
	/** 
	 * @brief Error on sock_send
	 * nn_send error
	 */
	WRP_SEND_ERR_NN = -0x840
} wrp_sock_send_error_t;

static int send_registration_msg (__instance_t *inst)
{
	wrp_msg_t reg_msg;
	reg_msg.msg_type = WRP_MSG_TYPE__SVC_REGISTRATION;
	reg_msg.u.reg.service_name = (char *) inst->cfg.service_name;
	reg_msg.u.reg.url = (char *) inst->client_url;
	return wrp_sock_send (inst, &reg_msg);
}

static bool show_options (libpd_cfg_t *cfg)
{
	libpd_log (LEVEL_DEBUG, 
		("LIBPARODUS Options: Rcv: %d, KA Timeout: %d\n",
		cfg->receive, cfg->keepalive_timeout_secs));
	return cfg->receive;
}

static void abort_init (__instance_t *inst, const char *opt_str)
{
	int i;
	char c;

	for (i=0; (c=opt_str[i]) != 0; i++) {
		if (c == 'r')
			shutdown_socket (&inst->rcv_sock);
		else if (c == 'q')
			libpd_qdestroy (&inst->wrp_queue, &wrp_free);
		else if (c == 's')
			shutdown_socket(&inst->send_sock);
		else if (c == 't')
			shutdown_socket(&inst->stop_rcv_sock);
	}
  // instance will be destroyed in libparodus_shutdown
}

int libparodus_init (libpd_instance_t *instance, libpd_cfg_t *libpd_cfg)
{
	bool need_to_send_registration;
	int err;
	int oserr = 0;
	__instance_t *inst = make_new_instance (libpd_cfg);
#define SETERR(oserr,err_) \
	inst->err = err_; \
	inst->exterr = oserr; \
	errno = oserr
#define CONNECT_ERR(oserr) \
	(oserr == EINVAL) ? LIBPD_ERROR_INIT_CFG : LIBPD_ERROR_INIT_CONNECT

	if (NULL == inst) {
		libpd_log (LEVEL_ERROR, ("LIBPARODUS: unable to allocate new instance\n"));
		return LIBPD_ERROR_INIT_INST;
	}
	*instance = (libpd_instance_t) inst;

	show_options (libpd_cfg);
	if (inst->cfg.receive) {
		libpd_log (LEVEL_INFO, ("LIBPARODUS: connecting receiver to %s\n",  inst->client_url));
		err = connect_receiver (inst->client_url, inst->cfg.keepalive_timeout_secs, &oserr);
		if (err < 0) {
			SETERR(oserr, LIBPD_ERR_INIT_RCV + err); 
			return CONNECT_ERR (oserr);
		}
		inst->rcv_sock = err;
	}
	if (!inst->connect_on_every_send) {
		libpd_log (LEVEL_INFO, ("LIBPARODUS: connecting sender to %s\n", inst->parodus_url));
		err = connect_sender (inst->parodus_url, &oserr);
		if (err < 0) {
			abort_init (inst, "r");
			SETERR (oserr, LIBPD_ERR_INIT_SEND + err); 
			return CONNECT_ERR (oserr);
		}
		inst->send_sock = err;
	}
	if (inst->cfg.receive) {
		// We use the stop_rcv_sock to send a stop msg to our own receive socket.
		err = connect_sender (inst->client_url, &oserr);
		if (err < 0) {
			abort_init (inst, "rs");
			SETERR (oserr, LIBPD_ERR_INIT_TERMSOCK + err); 
			return CONNECT_ERR (oserr);
		}
		inst->stop_rcv_sock = err;
		libpd_log (LEVEL_INFO, ("LIBPARODUS: Opened sockets\n"));
		err = libpd_qcreate (&inst->wrp_queue, inst->wrp_queue_name, WRP_QUEUE_SIZE, &oserr);
		if (err != 0) {
			abort_init (inst, "rst");
			SETERR (oserr, LIBPD_ERR_INIT_QUEUE + err); 
			return LIBPD_ERROR_INIT_QUEUE;
		}
		libpd_log (LEVEL_INFO, ("LIBPARODUS: Created queues\n"));
		err = create_thread (&inst->wrp_receiver_tid, wrp_receiver_thread,
				inst);
		if (err != 0) {
			abort_init (inst, "rqst"); 
			SETERR (err, LIBPD_ERR_INIT_RCV_THREAD_PCR);
			return LIBPD_ERROR_INIT_RCV_THREAD;
		}
	}


#ifdef TEST_SOCKET_TIMING
	sst_init_totals ();
#endif

	inst->run_state = RUN_STATE_RUNNING;

#ifdef PARODUS_SERVICE_REQUIRES_REGISTRATION
	need_to_send_registration = true;
#else
	need_to_send_registration = inst->cfg.receive;
#endif

	if (!inst->cfg.receive) {
		libpd_log (LEVEL_DEBUG, ("LIBPARODUS: Init without receiver\n"));
	}

	if (need_to_send_registration) {
		libpd_log (LEVEL_INFO, ("LIBPARODUS: sending registration msg\n"));
		err = send_registration_msg (inst);
		if (err != 0) {
			libpd_log (LEVEL_ERROR, ("LIBPARODUS: error sending registration msg\n"));
			oserr = inst->exterr;
			libparodus_shutdown__ (inst);
			inst->err = LIBPD_ERR_INIT_REGISTER + err;
			errno = oserr;
			return LIBPD_ERROR_INIT_REGISTER;
		}
		libpd_log (LEVEL_DEBUG, ("LIBPARODUS: Sent registration message\n"));
	}
	return 0;
}

// When msg_len is given as -1, then msg is a null terminated string
static int sock_send (int sock, const char *msg, int msg_len, int *exterr)
{
  int bytes;
	*exterr = 0;
	if (msg_len < 0)
		msg_len = strlen (msg) + 1; // include terminating null
	bytes = nn_send (sock, msg, msg_len, 0);
  if (bytes < 0) {
		*exterr = errno; 
		libpd_log_err (LEVEL_ERROR, errno, ("Error sending msg\n"));
		return -0x40;
	}
  if (bytes != msg_len) {
		libpd_log (LEVEL_ERROR, ("Not all bytes sent, just %d\n", bytes));
		return -1;
	}
	return 0;
}

// returns 0 OK, 1 timedout, -1 error
static int sock_receive (int rcv_sock, raw_msg_t *msg, int *exterr)
{
	char *buf = NULL;
  msg->len = nn_recv (rcv_sock, &buf, NN_MSG, 0);

	*exterr = 0;
  if (msg->len < 0) {
		libpd_log_err (LEVEL_ERROR, errno, ("Error receiving msg\n"));
		if (errno == ETIMEDOUT)
			return 1;
		*exterr = errno; 
		return -1;
	}
	msg->msg = buf;
	return 0;
}

static void libparodus_shutdown__ (__instance_t *inst)
{
	int rtn, exterr;
#ifdef TEST_SOCKET_TIMING
	sst_display_totals ();
#endif

	inst->run_state = RUN_STATE_DONE;
	libpd_log (LEVEL_INFO, ("LIBPARODUS: Shutting Down\n"));
	if (inst->cfg.receive) {
		sock_send (inst->stop_rcv_sock, end_msg, -1, &exterr);
	 	rtn = pthread_join (inst->wrp_receiver_tid, NULL);
		if (rtn != 0) {
			libpd_log_err (LEVEL_ERROR, rtn, ("Error terminating wrp receiver thread\n"));
		}
		shutdown_socket(&inst->rcv_sock);
		libpd_log (LEVEL_INFO, ("LIBPARODUS: Flushing wrp queue\n"));
		flush_wrp_queue (inst->wrp_queue, 5);
		libpd_qdestroy (&inst->wrp_queue, &wrp_free);
	}
	shutdown_socket(&inst->send_sock);
	if (inst->cfg.receive) {
		shutdown_socket(&inst->stop_rcv_sock);
	}
	inst->run_state = 0;
	inst->auth_received = false;
}

int libparodus_shutdown (libpd_instance_t *instance)
{
	__instance_t *inst;

	if (NULL == instance)
		return 0;
	inst = (__instance_t *) *instance;
	if (NULL == inst)
		return 0;

	if (RUN_STATE_RUNNING != inst->run_state) {
		libpd_log (LEVEL_DEBUG, ("LIBPARODUS: not running at shutdown\n"));
		destroy_instance (instance);
		return 0;
	}

	libparodus_shutdown__ (inst);
	destroy_instance (instance);
	return 0;
}

// returns 0 OK
//  1 timed out
static int timed_wrp_queue_receive (libpd_mq_t wrp_queue,	wrp_msg_t **msg, 
	unsigned timeout_ms, int *exterr)
{
	int rtn;
	void *raw_msg;

	rtn = libpd_qreceive (wrp_queue, &raw_msg, timeout_ms, exterr);
	if (rtn == 1) // timed out
		return 1;
	if (rtn != 0) {
		libpd_log (LEVEL_ERROR, ("Unable to receive on queue /WRP_QUEUE\n"));
		return rtn;
	}
	*msg = (wrp_msg_t *) raw_msg;
	libpd_log (LEVEL_DEBUG, ("LIBPARODUS: receive msg on WRP QUEUE\n"));
	return 0;
}

static wrp_msg_t *make_closed_msg (void)
{
	wrp_msg_t *msg = (wrp_msg_t *) malloc (sizeof (wrp_msg_t));
	if (NULL == msg)
		return NULL;
	msg->msg_type = WRP_MSG_TYPE__REQ;
	msg->u.req.transaction_uuid = closed_msg;
	msg->u.req.source = closed_msg;
	msg->u.req.dest = closed_msg;
	msg->u.req.payload = (void*) closed_msg;
	msg->u.req.payload_size = strlen(closed_msg);
	return msg;
}

// returns 0 OK
//  2 closed msg received
//  1 timed out
//  LIBPD_ERR_RCV_ ... on error
int libparodus_receive__ (libpd_mq_t wrp_queue, wrp_msg_t **msg, 
	uint32_t ms, int *exterr)
{
	int err;
	wrp_msg_t *msg__;

	err = timed_wrp_queue_receive (wrp_queue, msg, ms, exterr);
	if (err == 1) // timed out
		return 1;
	if (err != 0)
		return LIBPD_ERR_RCV_QUEUE + err;
	msg__ = *msg;
	if (msg__ == NULL) {
		libpd_log (LEVEL_DEBUG, ("LIBPARODOS: NULL msg from wrp queue\n"));
		return LIBPD_ERR_RCV_NULL_MSG;
	}
	libpd_log (LEVEL_DEBUG, ("LIBPARODUS: received msg type %d\n", msg__->msg_type));
	if (is_closed_msg (msg__)) {
		wrp_free (msg__);
		libpd_log (LEVEL_INFO, ("LIBPARODUS: closed msg received\n"));
		return 2;
	}
	return 0;
}

// returns 0 OK
//  2 closed msg received
//  1 timed out
// LIBPD_ERR_RCV_ ... on error
int libparodus_receive (libpd_instance_t instance, wrp_msg_t **msg, uint32_t ms)
{
	int rtn;
	__instance_t *inst = (__instance_t *) instance;

	if (NULL == inst) {
		libpd_log (LEVEL_ERROR, ("Null instance on libparodus_receive\n"));
		return LIBPD_ERROR_RCV_NULL_INST;
	}

	inst->err = 0;
	inst->exterr = 0;
	if (!inst->cfg.receive) {
		libpd_log (LEVEL_ERROR, ("No receive option on libparodus_receive\n"));
		inst->err = LIBPD_ERR_RCV_CFG;
		return LIBPD_ERROR_RCV_CFG;
	}
	if (RUN_STATE_RUNNING != inst->run_state) {
		libpd_log (LEVEL_ERROR, ("LIBPARODUS: not running at receive\n"));
		inst->err = LIBPD_ERR_RCV_STATE;
		return LIBPD_ERROR_RCV_STATE;
	}
	rtn = libparodus_receive__ (inst->wrp_queue, msg, ms, &inst->exterr);
	if (rtn >= 0)
		return rtn;
	inst->err = rtn;
	errno = inst->exterr;
	return LIBPD_ERROR_RCV_RCV;
}

int libparodus_close_receiver__ (libpd_mq_t wrp_queue, int *exterr)
{
	wrp_msg_t *closed_msg_ptr =	make_closed_msg ();
	int rtn = libpd_qsend (wrp_queue, (void *) closed_msg_ptr, 
				WRP_QUEUE_SEND_TIMEOUT_MS, exterr);
	if (rtn == 1) // timed out
		return 1;
	if (rtn != 0) {
		return LIBPD_ERR_CLOSE_RCV + rtn;
	}
	libpd_log (LEVEL_INFO, ("LIBPARODUS: Sent closed msg\n"));
	return 0;
}

int libparodus_close_receiver (libpd_instance_t instance)
{
	int rtn;
	__instance_t *inst = (__instance_t *) instance;

	if (NULL == inst) {
		libpd_log (LEVEL_ERROR, ("Null instance on libparodus_close_receiver\n"));
		return LIBPD_ERROR_CLOSE_RCV_NULL_INST;
	}
	inst->exterr = 0;
	if (!inst->cfg.receive) {
		libpd_log (LEVEL_ERROR, ("No receive option on libparodus_close_receiver\n"));
		inst->err = LIBPD_ERR_CLOSE_RCV_CFG;
		return LIBPD_ERROR_CLOSE_RCV_CFG;
	}
	if (RUN_STATE_RUNNING != inst->run_state) {
		libpd_log (LEVEL_ERROR, ("LIBPARODUS: not running at close receiver\n"));
		inst->err = LIBPD_ERR_CLOSE_RCV_STATE;
		return LIBPD_ERROR_CLOSE_RCV_STATE;
	}
	rtn = libparodus_close_receiver__ (inst->wrp_queue, &inst->exterr);
	if (rtn == 0)
		return 0;
	if (rtn == 1) {
		inst->err = LIBPD_ERR_CLOSE_RCV_TIMEDOUT;
		return LIBPD_ERROR_CLOSE_RCV_TIMEDOUT;
	}
	inst->err = rtn;
	return LIBPD_ERROR_CLOSE_RCV_SEND;
}

static int wrp_sock_send (__instance_t *inst,	wrp_msg_t *msg)
{
	int rtn;
	ssize_t msg_len;
	void *msg_bytes;
#ifdef TEST_SOCKET_TIMING
	sst_times_t sst_times;
#define SST(func) func
#else
#define SST(func)
#endif

	inst->exterr = 0;
	pthread_mutex_lock (&inst->send_mutex);
	msg_len = wrp_struct_to (msg, WRP_BYTES, &msg_bytes);
	if (msg_len < 1) {
		libpd_log (LEVEL_ERROR, ("LIBPARODUS: error converting WRP to bytes\n"));
		pthread_mutex_unlock (&inst->send_mutex);
		return -0x1001;
	}

	SST (sst_start_total_timing (&sst_times);)

	if (inst->connect_on_every_send) {
		rtn = connect_sender (inst->parodus_url, &inst->exterr);
		if (rtn < 0) {
			free (msg_bytes);
			pthread_mutex_unlock (&inst->send_mutex);
			return -0x1200 + rtn;
		}
		inst->send_sock = rtn;
	}

	SST (sst_start_send_timing (&sst_times);)
	rtn = sock_send (inst->send_sock, (const char *)msg_bytes, msg_len, &inst->exterr);
	SST (sst_update_send_time (&sst_times);)

	if (inst->connect_on_every_send) {
		shutdown_socket (&inst->send_sock);
	}
	SST (sst_update_total_time (&sst_times);)

	free (msg_bytes);
	pthread_mutex_unlock (&inst->send_mutex);
	if (rtn == 0)
		return 0;
	return -0x1800 + rtn;
}

int libparodus_send__ (libpd_instance_t instance, wrp_msg_t *msg)
{
	int rtn = wrp_sock_send ((__instance_t *) instance, msg);
	if (rtn == 0)
		return 0;
	return LIBPD_ERR_SEND + rtn;
}

int libparodus_send (libpd_instance_t instance, wrp_msg_t *msg)
{
	int rtn;
	__instance_t *inst = (__instance_t *) instance;

	if (NULL == inst) {
		libpd_log (LEVEL_ERROR, ("Null instance on libparodus_send\n"));
		return LIBPD_ERROR_SEND_NULL_INST;
	}
	inst->exterr = 0;
	if (RUN_STATE_RUNNING != inst->run_state) {
		libpd_log (LEVEL_ERROR, ("LIBPARODUS: not running at send\n"));
		inst->err = LIBPD_ERR_SEND_STATE;
		return LIBPD_ERR_SEND_STATE;
	}
	rtn = libparodus_send__ (instance, msg);
	if (rtn == 0)
		return 0;
	inst->err = rtn;
	if (rtn == LIBPD_ERR_SEND_CONVERT)
		return LIBPD_ERROR_SEND_WRP_MSG;
	errno = inst->exterr;
  return LIBPD_ERROR_SEND_SOCKET;
}


static char *find_wrp_msg_dest (wrp_msg_t *wrp_msg)
{
	if (wrp_msg->msg_type == WRP_MSG_TYPE__REQ)
		return wrp_msg->u.req.dest;
	if (wrp_msg->msg_type == WRP_MSG_TYPE__EVENT)
		return wrp_msg->u.event.dest;
	if (wrp_msg->msg_type == WRP_MSG_TYPE__CREATE)
		return wrp_msg->u.crud.dest;
	if (wrp_msg->msg_type == WRP_MSG_TYPE__RETREIVE)
		return wrp_msg->u.crud.dest;
	if (wrp_msg->msg_type == WRP_MSG_TYPE__UPDATE)
		return wrp_msg->u.crud.dest;
	if (wrp_msg->msg_type == WRP_MSG_TYPE__DELETE)
		return wrp_msg->u.crud.dest;
	return NULL;
}

static void wrp_receiver_reconnect (__instance_t *inst)
{
	int p = 2;
	int retry_delay = 0;
	int exterr;

	while (true)
	{
		shutdown_socket (&inst->rcv_sock);
		if (retry_delay < MAX_RECONNECT_RETRY_DELAY_SECS) {
			p = p+p;
			retry_delay = p-1;
		}
		sleep (retry_delay);
		libpd_log (LEVEL_DEBUG, ("Retrying receiver connection\n"));
		inst->rcv_sock = connect_receiver 
			(inst->client_url, inst->cfg.keepalive_timeout_secs, &exterr);
		if (inst->rcv_sock < 0)
			continue;
		if (send_registration_msg (inst) != 0)
			continue;
		break;
	}
	inst->auth_received = false;
	inst->reconnect_count++;
	return;
}

static void *wrp_receiver_thread (void *arg)
{
	int rtn, msg_len;
	int exterr;
	raw_msg_t raw_msg;
	wrp_msg_t *wrp_msg;
	int end_msg_len = strlen(end_msg);
	__instance_t *inst = (__instance_t*) arg;
	char *msg_dest, *msg_service;

	libpd_log (LEVEL_INFO, ("LIBPARODUS: Starting wrp receiver thread\n"));
	while (1) {
		rtn = sock_receive (inst->rcv_sock, &raw_msg, &exterr);
		if (rtn != 0) {
			if (rtn == 1) { // timed out
				wrp_receiver_reconnect (inst);
				continue;
			}
			break;
		}
		if (raw_msg.len >= end_msg_len) {
			if (strncmp (raw_msg.msg, end_msg, end_msg_len) == 0) {
				nn_freemsg (raw_msg.msg);
				break;
			}
		}
		if (RUN_STATE_RUNNING != inst->run_state) {
			nn_freemsg (raw_msg.msg);
			continue;
		}
		libpd_log (LEVEL_DEBUG, ("LIBPARODUS: Converting bytes to WRP\n")); 
 		msg_len = (int) wrp_to_struct (raw_msg.msg, raw_msg.len, WRP_BYTES, &wrp_msg);
		nn_freemsg (raw_msg.msg);
		if (msg_len < 1) {
			libpd_log (LEVEL_ERROR, ("LIBPARODUS: error converting bytes to WRP\n"));
			continue;
		}
		if (wrp_msg->msg_type == WRP_MSG_TYPE__AUTH) {
			libpd_log (LEVEL_INFO, ("LIBPARODUS: AUTH msg received\n"));
			inst->auth_received = true;
			wrp_free_struct (wrp_msg);
			continue;
		}

		if (wrp_msg->msg_type == WRP_MSG_TYPE__SVC_ALIVE) {
			libpd_log (LEVEL_DEBUG, ("LIBPARODUS: received keep alive message\n"));
			inst->keep_alive_count++;
			wrp_free_struct (wrp_msg);
			continue;
		}

		// Pass thru REQ, EVENT, and CRUD if dest matches the selected service
		msg_dest = find_wrp_msg_dest (wrp_msg);
		if (NULL == msg_dest) {
			libpd_log (LEVEL_ERROR, ("LIBPARADOS: Unprocessed msg type %d received\n",
				wrp_msg->msg_type));
			wrp_free_struct (wrp_msg);
			continue;
		}
		msg_service = strrchr (msg_dest, '/');
		if (NULL == msg_service) {
			wrp_free_struct (wrp_msg);
			continue;
		}
		msg_service++;
		if (strcmp (msg_service, inst->cfg.service_name) != 0) {
			wrp_free_struct (wrp_msg);
			continue;
		}
		libpd_log (LEVEL_DEBUG, ("LIBPARODUS: received msg directed to service %s\n",
			inst->cfg.service_name));
		libpd_qsend (inst->wrp_queue, (void *) wrp_msg, WRP_QUEUE_SEND_TIMEOUT_MS, &exterr);
	}
	libpd_log (LEVEL_INFO, ("Ended wrp receiver thread\n"));
	return NULL;
}


int flush_wrp_queue (libpd_mq_t wrp_queue, uint32_t delay_ms)
{
	wrp_msg_t *wrp_msg = NULL;
	int count = 0;
	int err, exterr;

	while (1) {
		err = timed_wrp_queue_receive (wrp_queue, &wrp_msg, delay_ms, &exterr);
		if (err == 1)	// timed out
			break;
		if (err != 0)
			return err;
		count++;
		wrp_free (wrp_msg);
	}
	libpd_log (LEVEL_INFO, ("LIBPARODUS: flushed %d messages out of WRP Queue\n", 
		count));
	return count;
}

// Functions used by libpd_test.c

int test_create_wrp_queue (libpd_mq_t *wrp_queue, 
	const char *wrp_queue_name, int *exterr)
{
	return libpd_qcreate (wrp_queue, wrp_queue_name, 24, exterr);
}

void test_close_wrp_queue (libpd_mq_t *wrp_queue)
{
	libpd_qdestroy (wrp_queue, &wrp_free);
}

int test_send_wrp_queue_ok (libpd_mq_t wrp_queue, int *exterr)
{
	wrp_msg_t *reg_msg = (wrp_msg_t *) malloc (sizeof(wrp_msg_t));
	char *name;
	reg_msg->msg_type = WRP_MSG_TYPE__SVC_REGISTRATION;
	name = (char*) malloc (4);
	strcpy (name, "iot");
	reg_msg->u.reg.service_name = name;
	name = (char *) malloc (strlen(PARODUS_CLIENT_URL) + 1);
	strcpy (name, PARODUS_CLIENT_URL);
	reg_msg->u.reg.url = name;
	return libpd_qsend (wrp_queue, (void *) reg_msg, 
		WRP_QUEUE_SEND_TIMEOUT_MS, exterr);
}

int test_close_receiver (libpd_mq_t wrp_queue, int *exterr)
{
	return libparodus_close_receiver__ (wrp_queue, exterr);
}

void test_get_counts (libpd_instance_t instance, 
	int *keep_alive_count, int *reconnect_count)
{
	__instance_t *inst = (__instance_t *) instance;
	*keep_alive_count = inst->keep_alive_count;
	*reconnect_count = inst->reconnect_count;
}


