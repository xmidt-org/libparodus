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
#include "../src/libparodus.h"
#include "parodus_notifier.h"
#include "notifier_log.h"
#include <time.h>
#include <cimplog/cimplog.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define PARODUS_URL_DEFAULT      "tcp://127.0.0.1:6666"
#define DEVICE_PROPS_FILE  "/etc/device.properties"

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void get_parodus_url(char **url);
static void getCurrentTime(struct timespec *timer);
static long timeValDiff(struct timespec *starttime, struct timespec *finishtime);
static int validate_notifier_struct(notifier_t *msg);
static libpd_instance_t client_instance;
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int send_event_to_parodus(notifier_t *msg)
{
    char *parodus_url = NULL;
    int ret = -1;
    wrp_msg_t *notif_wrp_msg = NULL;
    struct timespec start,end,*startPtr,*endPtr;
    startPtr = &start;
    endPtr = &end;
    static int init_done = 0;

    getCurrentTime(startPtr);

    ret = validate_notifier_struct(msg);
    if(ret == SUCCESS)
    {
        NotifPrint("service_name: %s source: %s destination: %s payload: %s content_type: %s\n",msg->service_name, msg->source, msg->destination, msg->payload, msg->content_type);
        if( !init_done )
        {
            get_parodus_url(&parodus_url);
            libpd_cfg_t cfg = {.service_name = msg->service_name,
                            .receive = false,
                            .keepalive_timeout_secs = 0,
                            .parodus_url = parodus_url,
                            .client_url = NULL
                            };

            ret = libparodus_init (&client_instance, &cfg);
            if(ret != 0)
            {
                exit(0);
            }
            NotifPrint("Init for parodus Success..!!\n");
            init_done = 1;
        }

        notif_wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
        if(notif_wrp_msg != NULL)
        {
            memset(notif_wrp_msg, 0, sizeof(wrp_msg_t));
            notif_wrp_msg ->msg_type = WRP_MSG_TYPE__EVENT;
            notif_wrp_msg ->u.event.source = strdup(msg->source);
            notif_wrp_msg ->u.event.dest = strdup(msg->destination);
            notif_wrp_msg->u.event.content_type = strdup(msg->content_type);
            notif_wrp_msg ->u.event.payload = (void *)(strdup(msg->payload));
            notif_wrp_msg ->u.event.payload_size = strlen(notif_wrp_msg ->u.event.payload);
            ret = libparodus_send(client_instance, notif_wrp_msg );
            if(ret == 0)
            {
                NotifPrint("Notification successfully sent to parodus\n");
            }
            wrp_free_struct (notif_wrp_msg);
        }

        getCurrentTime(endPtr);
        NotifPrint("Elapsed time : %ld ms\n", timeValDiff(startPtr, endPtr));

    }
    return ret;
}

void free_notifier_struct(notifier_t *msg)
{
    if(msg != NULL)
    {
        if(msg->source != NULL)
        {
            free(msg->source);
            msg->source = NULL;
        }

        if(msg->destination != NULL)
        {
            free(msg->destination);
            msg->destination = NULL;
        }

        if(msg->service_name != NULL)
        {
            free(msg->service_name);
            msg->service_name = NULL;
        }

        if(msg->content_type != NULL)
        {
            free(msg->content_type);
            msg->content_type = NULL;
        }

        if(msg->payload != NULL)
        {
            free(msg->payload);
            msg->payload = NULL;
        }
        free(msg);
        msg = NULL;
    }
}

void get_status_message(notifier_status_t status, char **message)
{
    if(status == SUCCESS)
    {
        *message = strdup("Event send success");
    }
    else if(status == MESSAGE_IS_NULL)
    {
        *message = strdup("message is NULL");
    }
    else if(status == SERVICE_NAME_IS_NULL)
    {
        *message = strdup("service_name is required");
    }
    else if(status == SOURCE_IS_NULL)
    {
        *message = strdup("source is required");
    }
    else if(status == DESTINATION_IS_NULL)
    {
        *message = strdup("destination is required");
    }
    else if(status == PAYLOAD_IS_NULL)
    {
        *message = strdup("payload is required");
    }
    else if(status == CONTENT_TYPE_IS_NULL)
    {
        *message = strdup("content_type is required");
    }
    else
    {
        *message = strdup(libparodus_strerror((libpd_error_t)status));
    }
}

const char *rdk_logger_module_fetch(void)
{
    return "LOG.RDK.PARODUSNOTIFIER";
}

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/
static void get_parodus_url(char **url)
{
    FILE *fp = fopen(DEVICE_PROPS_FILE, "r");

    if( NULL != fp ) {
        char str[255] = {'\0'};
        while( fscanf(fp,"%s", str) != EOF) {
            char *value = NULL;
            if(( value = strstr(str, "PARODUS_URL=")))
            {
                value = value + strlen("PARODUS_URL=");
                *url = strdup(value);
                NotifPrint("parodus url is %s\n", *url);
            }
        }
    } else {
        NotifError("Failed to open device.properties file:%s\n", DEVICE_PROPS_FILE);
		NotifPrint("Adding default value for parodus_url\n");
		*url = strdup(PARODUS_URL_DEFAULT);
    }
    fclose(fp);

    if( NULL == *url ) {
        NotifPrint("parodus url is not present in device.properties file, adding default parodus_url\n");
		*url = strdup(PARODUS_URL_DEFAULT);
    }
    NotifPrint("parodus url formed is %s\n", *url);
}

static void getCurrentTime(struct timespec *timer)
{
	clock_gettime(CLOCK_REALTIME, timer);
}

static long timeValDiff(struct timespec *starttime, struct timespec *finishtime)
{
	long msec;
	msec=(finishtime->tv_sec-starttime->tv_sec)*1000;
	msec+=(finishtime->tv_nsec-starttime->tv_nsec)/1000000;
	return msec;
}

static int validate_notifier_struct(notifier_t *msg)
{
    if(msg != NULL)
    {
        if(msg->service_name == NULL || strlen(msg->service_name) <= 0 )
        {
            return SERVICE_NAME_IS_NULL;
        }

        if(msg->source == NULL || strlen(msg->source) <= 0 )
        {
            return SOURCE_IS_NULL;
        }

        if(msg->destination == NULL || strlen(msg->destination) <= 0 )
        {
            return DESTINATION_IS_NULL;
        }

        if(msg->payload == NULL || strlen(msg->payload) <= 0 )
        {
            return PAYLOAD_IS_NULL;
        }

        if(msg->content_type == NULL || strlen(msg->content_type) <= 0 )
        {
            return CONTENT_TYPE_IS_NULL;
        }
    }
    else
    {
        return MESSAGE_IS_NULL;
    }

    return SUCCESS;
}
