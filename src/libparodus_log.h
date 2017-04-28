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
#ifndef  _LIBPARODUS_LOG_H
#define  _LIBPARODUS_LOG_H

#include <stdbool.h>

#define LEVEL_ERROR 0
#define LEVEL_INFO  1
#define LEVEL_DEBUG 2

// if TEST_ENVIRONMENT is not defined, then the macros libpd_log and libpd_log_err
// generate nothing
//#define TEST_ENVIRONMENT 1

#ifndef TEST_ENVIRONMENT
#define libpd_log(level,msg)
#define libpd_log_err(level,errcode,msg)

#else
// TEST_ENVIRONMENT defined

#include <stdio.h>
#include <string.h>

// When TEST_ENVIRONMENT == 1, printf is used.
// If TEST_ENVIRONMENT > 1, then you need to provide
// external functions 'CheckLevel' and 'Printf'

#if TEST_ENVIRONMENT==1
#define Printf printf

#define output_level(level) \
  if ((level) == LEVEL_ERROR) \
    Printf ("Error: "); \
  else if ((level) == LEVEL_INFO) \
    Printf ("Info: "); \
  else \
    Printf ("Debug: ");

#define libpd_log(level,msg) \
  do { \
    output_level (level); \
    Printf msg; \
  } while (false)

#else
// TEST_ENVIRONMENT > 1

  extern bool CheckLevel (int level);
  extern int Printf (const char *format, ...);

#define libpd_log(level,msg) if (CheckLevel (level)) Printf msg
    
#endif

// Example:  libpd_log (LEVEL_ERROR, ("Unable to allocate new instance\n"));
// notice you need an extra set of parentheses

#define libpd_log_err(level,errcode,msg) \
  libpd_log (level, msg); \
  do { \
    char errbuf[100]; \
    Printf (" : %s\n", strerror_r (errcode, errbuf, 100)); \
  } while (false)

// Example: libpd_log_err (LEVEL_ERROR, errno, ("Unable to bind to receive_socket %s\n", rcv_url));
// notice you need an extra set of parentheses

#endif
 
 
#endif
  
