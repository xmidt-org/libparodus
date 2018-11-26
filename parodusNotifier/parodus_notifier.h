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
#ifndef  _PARODUS_NOTIFIER_H
#define  _PARODUS_NOTIFIER_H

typedef struct {
	char *service_name;
	char *content_type;
	char *source;
	char *destination;
	char *payload;
} notifier_t;

typedef enum {
    SUCCESS = 0,
    MESSAGE_IS_NULL,
    SERVICE_NAME_IS_NULL,
    SOURCE_IS_NULL,
    DESTINATION_IS_NULL,
    PAYLOAD_IS_NULL,
    CONTENT_TYPE_IS_NULL
} notifier_status_t;

int send_event_to_parodus(notifier_t *msg);

void free_notifier_struct(notifier_t *msg);

void get_status_message(notifier_status_t status, char **message);

#endif
