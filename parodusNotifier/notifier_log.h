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
#ifndef  _NOTIFIER_LOG_H
#define  _NOTIFIER_LOG_H

extern int debugEnable;

#ifdef NOTIFIER_LIB
#define LOGGING_MODULE                    "PARODUSNOTIFIER"
#define NotifError(...)                   cimplog_error(LOGGING_MODULE, __VA_ARGS__)
#define NotifInfo(...)                    cimplog_info(LOGGING_MODULE, __VA_ARGS__)
#define NotifPrint(...)                   cimplog_debug(LOGGING_MODULE, __VA_ARGS__)
#else
#define  NotifPrint(...)            \
{                                   \
    if(debugEnable)                 \
    {                               \
        printf(__VA_ARGS__);        \
    }                               \
}

#define  NotifInfo(...)             \
{                                   \
    if(debugEnable)                 \
    {                               \
        printf(__VA_ARGS__);        \
    }                               \
}

#define  NotifError(...)            \
{                                   \
    if(debugEnable)                 \
    {                               \
        printf(__VA_ARGS__);        \
    }                               \
}
#endif

#endif
