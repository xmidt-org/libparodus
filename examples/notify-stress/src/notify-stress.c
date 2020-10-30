#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <getopt.h>

#include <libparodus.h>
#include <cimplog.h>
#include <cJSON.h>

/*----------------------------------------------------------------------------
 * This is a test application to stress parodus by sending lots of notifications.
 * options:
 *  -p <parodus_url>
 *  -c <client_url>
 *  -n <num_msgs>
 * 
 *   num_msgs defaults to 1
 *   when specified as 0, sends messages continuously
 * ---------------------------------------------------------------------------*/
 
 
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define CONTENT_TYPE_JSON     "application/json"
#define CLIENT_URL            "tcp://127.0.0.1:6668"
#define PARODUS_URL           "tcp://127.0.0.1:6666"
#define MAX_PARAMETERNAME_LEN 512
#define URL_SIZE              64
#define LOGGING_MODULE        "notify-stress"
#define OPTIONS_STRING_FORMAT "p:c:n:"
#define debug_error(...)      cimplog_error(LOGGING_MODULE, __VA_ARGS__)
#define debug_info(...)       cimplog_info(LOGGING_MODULE, __VA_ARGS__)
#define debug_print(...)      cimplog_debug(LOGGING_MODULE, __VA_ARGS__)

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static libpd_instance_t hpd_instance;
static struct option command_line_options[] = {
    {"parodus_url", optional_argument, 0, 'p'},
    {"client_url", optional_argument, 0, 'c'},
    {"num_msgs", required_argument, 0, 'n'},
    {0,0,0,0}
};
static char *parodus_url = NULL;
static char *client_url = NULL;
static unsigned num_msgs = 1;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void sig_handler(int sig);
static int libpd_client_mgr(void);
static int send_notification(char const *buff);
static int connect_parodus(void);
static void* parodus_receive_wait(void *vp);
static void start_parodus_receive_thread(void);
static int get_options( int argc, char **argv);
static void wait_random (void);
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char **argv)
{
	debug_info ("RAND_MAX is %ld (0x%lx)\n", RAND_MAX, RAND_MAX);
	srandom (getpid());

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGBUS, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGHUP, sig_handler);
    signal(SIGALRM, sig_handler);

	if (get_options (argc, argv) != 0)
	  return 4;

    if (libpd_client_mgr() == 0) {
      int i=0;
	  int sendStatus = 0;
	  while (sendStatus == 0) {
        sendStatus = send_notification("Hello Parodus");
        i++;
        if (num_msgs != 0)
          if (i >= num_msgs)
            break;
        if ((i&0x1FF) == 0)
          wait_random ();
      }
    }
    
    if( NULL != parodus_url ) {
        free(parodus_url);
    }
    if( NULL != client_url ) {
        free(client_url);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
// waits from 0 to 0x7FFFF (1048575) usecs
// 1 2 secs
static void wait_random (void)
{
  long usecs = random() >> 11;
  struct timeval timeout;
  if (usecs < 1000000l) {
    timeout.tv_sec = 1;
    timeout.tv_usec = usecs;
  } else {
	timeout.tv_sec = 2;
	timeout.tv_usec = usecs - 1000000l;
  }
  debug_info ("wait %ld secs %ld usecs\n", timeout.tv_sec, timeout.tv_usec);
  select (0, NULL, NULL, NULL, &timeout);
}

unsigned int parse_num_arg (const char *arg, const char *arg_name)
{
	unsigned int result = 0;
	int i;
	char c;
	
	if (arg[0] == '\0') {
		debug_error ("Empty %s argument\n", arg_name);
		return (unsigned int) -1;
	}
	for (i=0; '\0' != (c=arg[i]); i++)
	{
		if ((c<'0') || (c>'9')) {
			debug_error ("Non-numeric %s argument\n", arg_name);
			return (unsigned int) -1;
		}
		result = (result*10) + c - '0';
	}
	return result;
}


static int get_options( int argc, char **argv)
{
    int item;
    int options_index = 0;

    while( -1 != (item = getopt_long(argc, argv, OPTIONS_STRING_FORMAT,
                         command_line_options, &options_index)) )
    {
        switch( item ) {
            case 'p':
                parodus_url = strdup(optarg);
                break;
            case 'c':
                client_url = strdup(optarg);
                break;
            case 'n':
				num_msgs = parse_num_arg (optarg, "num_msgs");
				if (num_msgs == (unsigned int) -1)
				  return -1;
				if (num_msgs > 1000000) {
				  debug_info ("Using 1000000 msgs\n");
				  num_msgs = 1000000;
				}
				break;
            default:
                break;
        }    
    }
    return 0;
}


static void sig_handler(int sig)
{
    if( sig == SIGINT ) {
        signal(SIGINT, sig_handler); /* reset it to this function */
        debug_info("SIGINT received!\n");
        exit(0);
    } else if( sig == SIGUSR1 ) {
        signal(SIGUSR1, sig_handler); /* reset it to this function */
        debug_info("SIGUSR1 received!\n");
    } else if( sig == SIGUSR2 ) {
        debug_info("SIGUSR2 received!\n");
    } else if( sig == SIGCHLD ) {
        signal(SIGCHLD, sig_handler); /* reset it to this function */
        debug_info("SIGHLD received!\n");
    } else if( sig == SIGPIPE ) {
        signal(SIGPIPE, sig_handler); /* reset it to this function */
        debug_info("SIGPIPE received!\n");
    } else if( sig == SIGALRM )	{
        signal(SIGALRM, sig_handler); /* reset it to this function */
        debug_info("SIGALRM received!\n");
    } else {
        debug_info("Signal %d received!\n", sig);
        exit(0);
    }
}

static int libpd_client_mgr()
{
   int status;
   debug_print("Connect parodus, etc. \n");
   status = connect_parodus();
   if (status == 0)
     start_parodus_receive_thread();
   return status;
}

static int send_notification(char const* buff)
{
    wrp_msg_t *notif_wrp_msg = NULL;
    int retry_count = 0;
    int sendStatus = -1;
    int backoffRetryTime = 0;
    int c = 2;
    int i;
    char source[] = "mac:PCApplication";
    cJSON *notifyPayload = cJSON_CreateObject();
    char *stringifiedNotifyPayload;
    char msg[4096];

    /* Create JSON payload */
    for(i=0; i<8; i++)
      msg[i] = '-';
    for (i=8; i<4088; i++)
      msg[i] = 'X';
    for (i=4088; i<4096; i++)
      msg[i] = '#';
    msg[4095] = 0;
    cJSON_AddStringToObject(notifyPayload,"device_id", source);
    cJSON_AddStringToObject(notifyPayload,"iot", buff);
    cJSON_AddStringToObject(notifyPayload,"reboot-reason", msg);
   
    stringifiedNotifyPayload = cJSON_PrintUnformatted(notifyPayload);

    /* Create WRP message to send Parodus */
    notif_wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
    memset(notif_wrp_msg, 0, sizeof(wrp_msg_t));
    notif_wrp_msg ->msg_type = WRP_MSG_TYPE__EVENT;
    notif_wrp_msg ->u.event.source = source;
    notif_wrp_msg ->u.event.dest = "event:IOT_NOTIFICATION";
    notif_wrp_msg->u.event.content_type = CONTENT_TYPE_JSON;
    notif_wrp_msg ->u.event.payload = stringifiedNotifyPayload;
    notif_wrp_msg ->u.event.payload_size = strlen(stringifiedNotifyPayload);

    debug_print("buf: %s\n",buff);
    debug_info("Notification payload %s\n",stringifiedNotifyPayload);
    debug_print("source: %s\n",notif_wrp_msg ->u.event.source);
    debug_print("destination: %s\n", notif_wrp_msg ->u.event.dest);
    debug_print("content_type is %s\n",notif_wrp_msg->u.event.content_type);

    /* Send message to Parodus */
    while( retry_count <= 3 ) {
        backoffRetryTime = (int) pow(2, c) -1;
        sendStatus = libparodus_send(hpd_instance, notif_wrp_msg );
        if(sendStatus == 0)        {
            retry_count = 0;
            debug_info("Notification successfully sent to parodus\n");
            break;
        } else {
            debug_error("Failed to send Notification: '%s', retrying ....\n",libparodus_strerror(sendStatus));
            debug_print("sendNotification backoffRetryTime %d seconds\n", backoffRetryTime);
            sleep(backoffRetryTime);
            c++;
            retry_count++;
         }
   }
   debug_print("sendStatus is %d\n",sendStatus);
   free (notif_wrp_msg );
   free(stringifiedNotifyPayload);
   cJSON_Delete(notifyPayload);
   return sendStatus;
}

static int connect_parodus()
{
    int backoffRetryTime = 0;
    int backoff_max_time = 5;
    int i;
    int max_retry_sleep;
    int c = 2;   //Retry Backoff count shall start at c=2 & calculate 2^c - 1.
    int retval = -1;
    max_retry_sleep = (int) pow(2, backoff_max_time) -1;
    debug_info("max_retry_sleep is %d\n", max_retry_sleep );
    
    if( NULL == parodus_url ) {
        parodus_url = strdup(PARODUS_URL);
    }
    if( NULL == client_url ) {
        client_url = strdup(CLIENT_URL);
    }

    libpd_cfg_t cfg = { .service_name = "hello-parodus",
                        .receive = true, 
                        .keepalive_timeout_secs = 64,
                        .parodus_url = parodus_url,
                        .client_url = client_url
                      };
                     
    debug_info("Configurations => service_name : %s parodus_url : %s client_url : %s\n", cfg.service_name, cfg.parodus_url, cfg.client_url );
    (void)retval;
    
    for (i=0; i<2; i++)
    {
        if( backoffRetryTime < max_retry_sleep ) {
            backoffRetryTime = (int) pow(2, c) -1;
        }
        debug_print("New backoffRetryTime value calculated as %d seconds\n", backoffRetryTime);
        int ret = libparodus_init (&hpd_instance, &cfg);
        if( ret ==0 ) {
            debug_info("Init for parodus Success..!!\n");
            return 0;
        } else {
            debug_error("Init for parodus (url %s) failed: '%s'\n", cfg.parodus_url, libparodus_strerror(ret));
            sleep(backoffRetryTime);
            c++;
         
	    if( backoffRetryTime == max_retry_sleep ) {
		c = 2;
		backoffRetryTime = 0;
		debug_print("backoffRetryTime reached max value, reseting to initial value\n");
	    }
        }
	retval = libparodus_shutdown(&hpd_instance);
    }
    return -1;
}

static void* parodus_receive_wait(void *vp)
{
    int rtn;
    wrp_msg_t *wrp_msg;

    debug_print("parodus_receive_wait\n");
    while( 1 ) {
        rtn = libparodus_receive(hpd_instance, &wrp_msg, 2000);
        debug_print("    rtn = %d\n", rtn);
        if( 0 == rtn ) {
            debug_info("Got something from parodus.\n");
        } else if( 1 == rtn || 2 == rtn ) {
            debug_info("Timed out or message closed.\n");
            continue;
        } else {
            debug_info("Libparodus failed to receive message: '%s'\n",libparodus_strerror(rtn));
        }
        if( NULL != wrp_msg ) {
            free(wrp_msg);
        }
        sleep(5);
    }
    libparodus_shutdown(&hpd_instance);
    debug_print("End of parodus_upstream\n");
    return 0;
}

static void start_parodus_receive_thread()
{
    int err = 0;
    pthread_t threadId;
    err = pthread_create(&threadId, NULL, parodus_receive_wait, NULL);
    if( err != 0 ) {
        debug_error("Error creating thread :[%s]\n", strerror(err));
        exit(1);
    } else {
        debug_print("Parodus Receive wait thread created Successfully %d\n", (int ) threadId);
    }    
}
