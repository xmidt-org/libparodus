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

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define CONTENT_TYPE_JSON     "application/json"
#define CLIENT_URL            "tcp://172.20.5.2:6668"
#define PARODUS_URL           "tcp://172.20.5.2:6666"
#define MAX_PARAMETERNAME_LEN 512
#define URL_SIZE              64
#define LOGGING_MODULE        "hello-parodus"
#define OPTIONS_STRING_FORMAT "p:c:"
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
    {0,0,0,0}
};
static char *parodus_url = NULL;
static char *client_url = NULL;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void sig_handler(int sig);
static void libpd_client_mgr(void);
static void send_notification(char const *buff);
static void connect_parodus(void);
static void* parodus_receive_wait(void *vp);
static void start_parodus_receive_thread(void);

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char **argv)
{
    int item;
    int options_index = 0;

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
            default:
                break;
        }    
    }

    libpd_client_mgr();
    send_notification("Hello Parodus");
    
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

static void libpd_client_mgr()
{
   debug_print("Connect parodus, etc. \n");
   connect_parodus();
   start_parodus_receive_thread();
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

    libpd_cfg_t cfg = { .service_name = "hello-parodus",
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

