/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/
#ifndef  _LIBPARODUS_LOG_H
#define  _LIBPARODUS_LOG_H

#include <stdarg.h>
#include <stdbool.h>

#define LEVEL_ERROR 0
#define LEVEL_INFO  1
#define LEVEL_DEBUG 2

// When not compiled with TEST_ENVIRONMENT, 
//		a log_handler function must be provided.
// When compiled with TEST_ENVIRONMENT,
//		a log handler function can, but need not be provided.
//#define TEST_ENVIRONMENT 1

#ifdef TEST_ENVIRONMENT
#define LEVEL_NO_LOGGER 99
#else
#define LEVEL_NO_LOGGER LEVEL_INFO
#endif

/**
 * @brief Handler used by svcagt_log_set_handler to receive all log
 * notifications produced by the library on this function.
 *
 * @param level The log level
 *
 * @param log_msg The actual log message reported.
 *
 */
typedef void (*parlibLogHandler) (int level, const char * log_msg);

/**
 * @brief Initialize the logger
 *
 * @param log_directory only used if TEST_ENVIRONMENT defined
 *
 * @param handler pointer to log handler function. may be null if TEST_ENVIRONMENT
 * defined.
 *
 */
int log_init (const char *log_directory, parlibLogHandler handler);

int log_shutdown (void);

//#define svcagt_log(level, message, ...)


void libpd_log (  int level, int os_errno, const char *fmt, ...);
 

#endif


