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
#include "../src/libparodus_log.h"
#include "parodus_notifier.h"

#define PARODUS_URL_DEFAULT      "tcp://127.0.0.1:6666"
#define CONTENT_TYPE_JSON       "application/json"
#define DEVICE_PROPS_FILE  "/etc/device.properties"

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
        NotifPrint("Failed to open device.properties file:%s\n", DEVICE_PROPS_FILE);
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

int send_event_to_parodus(notifier_t *msg)
{
    char *parodus_url = NULL;
    libpd_instance_t client_instance;
    int ret = -1;
    wrp_msg_t *notif_wrp_msg = NULL;

    get_parodus_url(&parodus_url);
    NotifPrint("parodus_url is %s\n", parodus_url);
    if(parodus_url != NULL && msg->service_name != NULL )
    {
        NotifPrint("service_name is %s\n", msg->service_name);
        libpd_cfg_t cfg = {.service_name = msg->service_name,
						.receive = false, .keepalive_timeout_secs = 0,
						.parodus_url = parodus_url,
						.client_url = NULL
					   };

         ret = libparodus_init (&client_instance, &cfg);
         if(ret == 0)
         {
            NotifInfo("Init for parodus Success..!!\n");
            if(msg->source != NULL && msg->destination != NULL && msg->payload != NULL)
            {
                NotifPrint("source: %s destination: %s payload: %s\n",msg->source, msg->destination, msg->payload);
                notif_wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
                if(notif_wrp_msg != NULL)
                {
                    memset(notif_wrp_msg, 0, sizeof(wrp_msg_t));
                    notif_wrp_msg ->msg_type = WRP_MSG_TYPE__EVENT;
                    notif_wrp_msg ->u.event.source = msg->source;
                    notif_wrp_msg ->u.event.dest = msg->destination;
                    notif_wrp_msg->u.event.content_type = strdup(CONTENT_TYPE_JSON);
                    notif_wrp_msg ->u.event.payload = (void *)(msg->payload);
                    notif_wrp_msg ->u.event.payload_size = strlen(notif_wrp_msg ->u.event.payload);
                    ret = libparodus_send(client_instance, notif_wrp_msg );
                    if(ret == 0)
                    {
                        NotifInfo("Notification successfully sent to parodus\n");
                    }
                    wrp_free_struct (notif_wrp_msg);
                }
            }
         }
    }
    return ret;
}

const char *rdk_logger_module_fetch(void)
{
    return "LOG.RDK.PARODUSNOTIFIER";
}
