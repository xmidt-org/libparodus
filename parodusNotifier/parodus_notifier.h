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
#include <cimplog/cimplog.h>

#define LOGGING_MODULE                    "PARODUSNOTIFIER"
#define NotifError(...)                   cimplog_error(LOGGING_MODULE, __VA_ARGS__)
#define NotifInfo(...)                    cimplog_info(LOGGING_MODULE, __VA_ARGS__)
#define NotifPrint(...)                   cimplog_debug(LOGGING_MODULE, __VA_ARGS__)

typedef struct {
	char *service_name;
	char *source;
	char *destination;
	char *payload;
} notifier_t;

int send_event_to_parodus(notifier_t *msg);

#endif
