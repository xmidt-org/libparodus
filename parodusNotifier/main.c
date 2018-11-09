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
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include "../src/libparodus.h"
#include "parodus_notifier.h"

int main( int argc, char **argv)
{
    notifier_t *msg = NULL;
    msg = (notifier_t *) malloc(sizeof(notifier_t));
    memset(msg, 0, sizeof(notifier_t));

    static const struct option long_options[] = {
        {"service-name",                required_argument, 0, 'n'},
        {"source",                      required_argument, 0, 's'},
        {"destination",                 required_argument, 0, 'd'},
        {"payload",                     required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };

    int c;
    NotifPrint("Parsing command line arguments..\n");
    optind = 1;

    while(1)
    {
        /* getopt_long stores the option index here. */
        int option_index = 0;
        c = getopt_long (argc, argv, "n:s:d:p", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 'n':
                msg->service_name = strdup(optarg);
                NotifPrint("msg->service_name is %s\n",msg->service_name);
            break;

            case 's':
                msg->source = strdup(optarg);
                NotifPrint("source is %s\n",msg->source);
            break;

            case 'd':
                msg->destination = strdup(optarg);
                NotifPrint("destination is %s\n",msg->destination);
            break;

            case 'p':
                msg->payload = strdup(optarg);
                NotifPrint("payload is %s\n",msg->payload);
            break;

            case '?':
            /* getopt_long already printed an error message. */
            break;

            default:
                NotifError("Enter Valid commands..\n");
            return -1;
        }
    }

    NotifPrint("argc is :%d\n", argc);
    NotifPrint("optind is :%d\n", optind);

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        NotifPrint("non-option ARGV-elements: ");
        while (optind < argc)
            NotifPrint("%s ", argv[optind++]);
        putchar ('\n');
    }

    int ret = send_event_to_parodus(msg);
    if(ret == 0)
    {
        NotifInfo("Event sent successfully\n");
    }

    return ret;
}
