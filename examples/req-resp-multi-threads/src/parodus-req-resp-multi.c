#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <getopt.h>

#include <cimplog.h>
#include <cJSON.h>
#include <libparodus.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define CONTENT_TYPE_JSON     "application/json"
#define PARODUS_URL      "tcp://127.0.0.1:6666"
#define CLIENT_URL       "tcp://127.0.0.1:6667"
#define MAX_PARAMETERNAME_LEN 512
#define URL_SIZE              64
#define LOGGING_MODULE        "parodus-req-resp-multi"
#define OPTIONS_STRING_FORMAT "p:c:n:d:"
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
    {"max_parallel_threads", required_argument, 0, 'n'},
    {"delay_opt", required_argument, 0, 'd'},
    {0,0,0,0}
};
static char *parodus_url = NULL;
static char *client_url = NULL;
static unsigned int max_parallel_threads = 1;
static int delay_opt = 'a';

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void sig_handler(int sig);
static void send_notification(char const *buff);
static void connect_parodus(void);
static void parodus_receive_wait(void);
static int get_options( int argc, char **argv);

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

    connect_parodus ();
    send_notification("Hello Parodus");
    parodus_receive_wait ();
    
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
				max_parallel_threads = parse_num_arg (optarg, "max-parallel-threads");
				if (max_parallel_threads == (unsigned int) -1)
				  return -1;
				if (max_parallel_threads < 1)
				  max_parallel_threads = 1;
				if (max_parallel_threads > 10) {
				  debug_info ("Using 10 threads\n");
				  max_parallel_threads = 10;
				}
				break;
			case 'd':
				// Delay options:
				// MAX_DELAY is 1048575 usecs
				// option 'a':  all threads delay random (0 .. MAX_DELAY)
				// option 'n':	no delays
				// option '1':	thread 1 delays random (0 .. MAX_DELAY)
				// option '2':	all threads except thread 1 delay random
				if (strcmp (optarg, "a") == 0)
				  delay_opt = 'a';
				else if (strcmp (optarg, "n") == 0)
				  delay_opt = 'n';
				else if (strcmp (optarg, "1") == 0)
				  delay_opt = '1';
				else if (strcmp (optarg, "2") == 0)
				  delay_opt = '2';
				else {
					debug_info ("Invalid delay option\n");
					return -1;
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

// waits from 0 to 0x7FFFF (1048575) usecs
long wait_random (void)
{
  long usecs = random() >> 11;
  struct timeval timeout;
  if (usecs < 1000000l) {
    timeout.tv_sec = 0;
    timeout.tv_usec = usecs;
  } else {
	timeout.tv_sec = 1;
	timeout.tv_usec = usecs - 1000000l;
  }
  select (0, NULL, NULL, NULL, &timeout);
  return usecs;
}

static void send_notification(char const* buff)
{
    wrp_msg_t *notif_wrp_msg = NULL;
    int retry_count = 0;
    int sendStatus = -1;
    int backoffRetryTime = 0;
    int c = 2;
    char source[] = "mac:PCApplication";
    cJSON *notifyPayload = cJSON_CreateObject();
    char *stringifiedNotifyPayload;

    /* Create JSON payload */
    cJSON_AddStringToObject(notifyPayload,"device_id", source);
    cJSON_AddStringToObject(notifyPayload,"iot", buff);
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
}

static void connect_parodus()
{
    int backoffRetryTime = 0;
    int backoff_max_time = 5;
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

    libpd_cfg_t cfg = { .service_name = "iot",
                        .receive = true, 
                        .keepalive_timeout_secs = 64,
                        .parodus_url = parodus_url,
                        .client_url = client_url
                      };
                     
    debug_info("Configurations => service_name : %s parodus_url : %s client_url : %s\n", cfg.service_name, cfg.parodus_url, cfg.client_url );
    (void)retval;
    
    while( 1 ) {
        if( backoffRetryTime < max_retry_sleep ) {
            backoffRetryTime = (int) pow(2, c) -1;
        }
        debug_print("New backoffRetryTime value calculated as %d seconds\n", backoffRetryTime);
        int ret = libparodus_init (&hpd_instance, &cfg);
        if( ret ==0 ) {
            debug_info("Init for parodus Success..!!\n");
            break;
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
}

static void processRequest(char *reqPayload, char *transactionId, char **resPayload)
{
	char *new_payload = strdup(reqPayload);
	size_t payload_size = strlen(new_payload);
	int i;
	
	for (i=0; i<payload_size; i++)
	   new_payload[i] = tolower(new_payload[i]);
	*resPayload = new_payload;
}

static bool should_delay (unsigned n)
{
	if (delay_opt == 'a')
	  return true;
	if (delay_opt == '1')
	  return (n == 1);
	if (delay_opt == '2')
	  return (n != 1);
	// delay_opt is 'n'
	return false;
}

static void parodus_receive(unsigned n)
{
        int rtn;
        wrp_msg_t *wrp_msg;
        wrp_msg_t *res_wrp_msg ;

/*
        struct timespec start,end,*startPtr,*endPtr;
        startPtr = &start;
        endPtr = &end;
*/
        char *contentType = NULL;
        char *sourceService, *sourceApplication =NULL;
        char *status=NULL;

        rtn = libparodus_receive (hpd_instance, &wrp_msg, 2000);
        if (rtn == 1)
        {
                return;
        }

        if (rtn != 0)
        {
                debug_error ("Libparodus failed to recieve message: '%s'\n",libparodus_strerror(rtn));
                sleep(5);
                return;
        }

        if(wrp_msg != NULL)
        {
			long delay = 0;
			
            if (wrp_msg->msg_type == WRP_MSG_TYPE__REQ)
            {
                    res_wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
                    

                    if(res_wrp_msg != NULL)
                    {
						memset(res_wrp_msg, 0, sizeof(wrp_msg_t));
                        // getCurrentTime(startPtr);
						if (should_delay (n))
						  delay = wait_random ();
						else
						  delay = 0;
                        processRequest((char *)wrp_msg->u.req.payload, wrp_msg->u.req.transaction_uuid, ((char **)(&(res_wrp_msg->u.req.payload))));

                        
                        if(res_wrp_msg->u.req.payload !=NULL)
                        {   
                                debug_print("Response payload is %s\n",(char *)(res_wrp_msg->u.req.payload));
                                res_wrp_msg->u.req.payload_size = strlen(res_wrp_msg->u.req.payload);
                        }
                        res_wrp_msg->msg_type = wrp_msg->msg_type;
			if(wrp_msg->u.req.dest != NULL)
			{
				res_wrp_msg->u.req.source = strdup(wrp_msg->u.req.dest);
			}
			if(wrp_msg->u.req.source != NULL)
			{
				res_wrp_msg->u.req.dest = strdup(wrp_msg->u.req.source);
			}
			if(wrp_msg->u.req.transaction_uuid != NULL)
			{
				res_wrp_msg->u.req.transaction_uuid = strdup(wrp_msg->u.req.transaction_uuid);
			}
                        contentType = strdup(CONTENT_TYPE_JSON);
                        if(contentType != NULL)
                        {
                            res_wrp_msg->u.req.content_type = contentType;
                        }
                        int sendStatus = libparodus_send(hpd_instance, res_wrp_msg);
                        debug_print("sendStatus is %d\n",sendStatus);
                        if(sendStatus == 0)
                        {
                                debug_info
                                ("Sent msg to parodus from thread %u after delay %ld\n", n, delay);
                        }
                        else
                        {
                                debug_error("Failed to send message: '%s'\n",libparodus_strerror(sendStatus));
                        }
                        // getCurrentTime(endPtr);
                        // WalInfo("Elapsed time : %ld ms\n", timeValDiff(startPtr, endPtr));
                        wrp_free_struct (res_wrp_msg);
                    }
		    wrp_free_struct (wrp_msg);
            }
        }
}

void *parallelProcessTask(void *id)
{
	unsigned n = (unsigned) (uintptr_t) id;
	
	debug_info ("Starting thread %u\n", n);
	if(n < max_parallel_threads)
	{
		debug_print("Detaching parallelProcess thread\n");
        	pthread_detach(pthread_self());
	}
        while( true )
        {
                parodus_receive(n);
        }
        return NULL;
}

static void initParallelProcess()
{
        int err = 0, i = 0;
        pthread_t threadId[max_parallel_threads-1];
        debug_print("============ initParallelProcess ==============\n");
        for(i=0; i<max_parallel_threads-1; i++)
        {
                err = pthread_create(&threadId[i], NULL, parallelProcessTask, 
                  (void *)(uintptr_t)(i+1));
                if (err != 0)
                {
                        debug_error("Error creating parallelProcessTask thread %d :[%s]\n", i, strerror(err));
                }
                else
                {
                        debug_print("parallelProcessTask thread %d created Successfully\n",i);
                }
        }
}

static void parodus_receive_wait()
{
        if(max_parallel_threads > 1)
        {
                debug_info("Parallel request processing is enabled\n");
        }
        initParallelProcess();
        parallelProcessTask((void *) (uintptr_t)max_parallel_threads);
        libparodus_shutdown(&hpd_instance);
        debug_print ("End of parodus_upstream\n");
}

