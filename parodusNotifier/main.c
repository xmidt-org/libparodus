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

static void print_usage();
int debugEnable = 0;

int main( int argc, char **argv)
{
    notifier_t *msg = NULL;
    msg = (notifier_t *) malloc(sizeof(notifier_t));
    memset(msg, 0, sizeof(notifier_t));
    const char *option_string = "n:s:d:p:c:D::";
    char *status = NULL;

    static const struct option long_options[] = {
        {"service-name",                required_argument, 0, 'n'},
        {"source",                      required_argument, 0, 's'},
        {"destination",                 required_argument, 0, 'd'},
        {"payload",                     required_argument, 0, 'p'},
        {"content-type",                required_argument, 0, 'c'},
        {"DEBUG",                       no_argument, 0, 'D'},
        {0, 0, 0, 0}
    };

    int c;
    optind = 1;

    while(1)
    {
        /* getopt_long stores the option index here. */
        int option_index = 0;
        c = getopt_long (argc, argv, option_string, long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 'n':
                msg->service_name = strdup(optarg);
            break;

            case 's':
                msg->source = strdup(optarg);
            break;

            case 'd':
                msg->destination = strdup(optarg);
            break;

            case 'p':
                msg->payload = strdup(optarg);
            break;

            case 'c':
                msg->content_type = strdup(optarg);
            break;

            case 'D':
                debugEnable = 1;
                printf("DEBUG mode is ON\n");
            break;

            case '?':
            /* getopt_long already printed an error message. */
            break;

            default:
                fprintf(stderr,"Enter valid arguments..\n");
                print_usage();
            return -1;
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        fprintf(stderr,"non-option ARGV-elements: ");
        while (optind < argc)
            fprintf(stderr,"%s ", argv[optind++]);
        putchar ('\n');
    }

    if(debugEnable)
    {
        printf("service_name: %s\nsource: %s\ndestination: %s\ncontent_type: %s\npayload: %s\n",msg->service_name, msg->source, msg->destination, msg->content_type, msg->payload);
    }

    int ret = send_event_to_parodus(msg);
    get_status_message(ret, &status);
    if(ret == 0)
    {
        fprintf(stdout,"%s\n",status);
    }
    else
    {
        fprintf(stderr,"%s\n",status);
        if(ret > 0)
        {
            print_usage();
        }
    }
    free(status);
    free_notifier_struct(msg);
    return ret;
}

static void print_usage()
{
    fprintf(stderr, "Usage:\nparodusNotifiercli --service-name=<value> --source=<value> --destination=<value> --payload=<value> --content-type=<value> [--DEBUG]\n\n" );
}
