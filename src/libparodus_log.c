/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#include "libparodus_log.h"
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "pthread.h"
#include "libparodus_time.h"


#define LEVEL_ERROR 0
#define LEVEL_INFO  1
#define LEVEL_DEBUG 2


#ifdef TEST_ENVIRONMENT
#define MAX_FILE_SIZE 75000
int current_level = LEVEL_DEBUG;
#else
#define MAX_FILE_SIZE 75000
int current_level = LEVEL_INFO;
#endif

#define MSG_BUF_SIZE 4096

/* log handling */
parlibLogHandler     log_handler =NULL;

pthread_mutex_t log_mutex=PTHREAD_MUTEX_INITIALIZER;

const char *level_names[3] = {"ERROR", "INFO ", "DEBUG"};

void libpd_log ( int level, int os_errno, const char *msg, ...);


#ifdef TEST_ENVIRONMENT
const char *test_log_date = NULL; // set this for testing

const char *libpd_log_dir = NULL;

int current_fd = -1;

int current_file_num = -1;

char current_file_date[8];

char current_file_name[128];

long current_file_size = 0;


static int my_current_date (char *date)
{
	if (test_log_date != NULL) {	
		if (strlen (test_log_date) == 8) {
			strncpy (date, test_log_date, 8);
			return 0;
		}
		test_log_date += 1;
	}
	return get_current_date (date);
}


static void make_file_name (void)
{
	sprintf (current_file_name, "%s/log%8.8s.%d", 
		libpd_log_dir, current_file_date, current_file_num);
}

bool log_level_is_debug (void)
{
	return (current_level == LEVEL_DEBUG);
}

int get_valid_file_num (const char *file_name, const char *date)
{
#define IS_DIGIT(c) (((c)>='0') && ((c)<='9'))
	char c;
	int i, val;
	if (strncmp (file_name, "log", 3) != 0)
		return -1;
	if (strncmp (file_name+3, date, 8) != 0)
		return -1;
	if (file_name[11] != '.')
		return -1;
	val = 0;
	if (file_name[12] == 0)
		return -1;
	for (i=12; (c=file_name[i]) != 0; i++) {
		if (!IS_DIGIT (c))
			return -1;
		val = (val*10) + (c-'0');
	}
	if (val == 0)
		return -1;
	return val;
}

int get_last_file_num_in_dir (const char *date, const char *log_dir)
{
	int file_num = -1;
	struct dirent *dent;
	DIR *dirp = opendir (log_dir);

	if (dirp == NULL) {
		libpd_log (LEVEL_NO_LOGGER, errno, "Could not open log directory %s\n", log_dir);
		return -1;
	}

	while (1) {
		int new_file_num = -1;

		dent = readdir (dirp);
		if (dent == NULL)
			break;
		if (dent->d_type != DT_REG)
			continue;
		new_file_num = get_valid_file_num (dent->d_name, date);
		if (new_file_num > file_num)
			file_num = new_file_num;
	}

	closedir (dirp);
	return file_num;
}

int get_last_file_num (const char *date)
{
	return get_last_file_num_in_dir (date, libpd_log_dir);
}

static void close_current_file (void)
{
	if (current_fd != -1) {
		close (current_fd);
		current_fd = -1;
	}
}

static int get_current_file_size (void)
{
	int err;
	struct stat file_stat;

	err = fstat (current_fd, &file_stat);
	if (err != 0) {
		libpd_log (LEVEL_NO_LOGGER, errno, "Could not stat file %s\n", current_file_name);
		close_current_file ();
		return -1;
	}
	current_file_size = (long) file_stat.st_size;
	return 0;
}

static int open_next_file (void);

static int open_file (bool init)
{
	int flags = O_WRONLY | O_CREAT | O_SYNC;

	if (init)
		flags |= O_APPEND;
	else
		flags |= O_TRUNC;

	make_file_name ();
	current_fd = open (current_file_name, flags, 0666);
	if (current_fd == -1) {
		libpd_log (LEVEL_NO_LOGGER, errno, "Unable to open log file %s\n", current_file_name);
		return -1;
	}
	current_file_size = 0;
	if (init) {
		if (get_current_file_size () != 0)
			return -1;
		if (current_file_size >= MAX_FILE_SIZE)
			return open_next_file ();
	}
	return 0;
}


static int open_next_file (void)
{
	int err;
	char current_date[8];

	close_current_file ();

	err = my_current_date (current_date);
	if (err != 0)
		return err;
	if (strncmp (current_date, current_file_date, 8) == 0) {
		current_file_num++;
	} else {
		strncpy (current_file_date, current_date, 8);
		current_file_num = 1;
	}

	return open_file (false);
}

int log_init (const char *log_directory, parlibLogHandler handler)
{
	int err;

	log_handler = handler;

	if (NULL != log_handler)
		return 0;

	if (NULL == log_directory) {
		libpd_log_dir = getenv ("LIBPARODUS_LOG_DIRECTORY");
		if (NULL != libpd_log_dir)
			if (libpd_log_dir[0] == '\0')
				libpd_log_dir = NULL;
	} else
		libpd_log_dir = log_directory;
	if (NULL == libpd_log_dir)
		return 0;
	err = mkdir (libpd_log_dir, 0777);
	if ((err == -1) && (errno != EEXIST)) {
		libpd_log (LEVEL_NO_LOGGER, errno, "Failed to create directory %s\n", log_directory);
		return -1;
	}
	err = my_current_date (current_file_date);
	if (err != 0)
		return err;
	libpd_log (LEVEL_NO_LOGGER, 0, "Current date in logger is %8.8s\n", current_file_date);
	current_file_num = get_last_file_num (current_file_date);
	if (current_file_num <= 0)
		current_file_num = 1;
	return open_file (true);
}

int log_shutdown (void)
{
	close_current_file ();
	return 0;
}

int output_log (int level, char *msg_buf, const char *fmt, va_list arg_ptr, int err_code)
{
	int err = 0;
	char timestamp[TIMESTAMP_BUFLEN];
	char errbuf[100];
	const char *err_msg;
	int header_len, error_len, buf_limit, nbytes;
	
	err = make_current_timestamp (timestamp);
	if (err != 0)
		timestamp[0] = '\0';

	header_len = sprintf (msg_buf, "[%s] %s: ", timestamp, level_names[level]);
	if (header_len < 0)
		return -1;

	buf_limit = MSG_BUF_SIZE - header_len;
	error_len = 0;
	if (err_code != 0) {
		err_msg = strerror_r (errno, errbuf, 100);
		error_len = strlen (err_msg)+3;
		buf_limit -= error_len;
	}

  nbytes = vsnprintf(msg_buf+header_len, buf_limit, fmt, arg_ptr);
	if (nbytes < 0)
		return -1;
	nbytes = strlen(msg_buf);
	if (err_code != 0) {
		sprintf (msg_buf+nbytes, ": %s\n", err_msg);
		nbytes += error_len; 
	}

	write (STDOUT_FILENO, msg_buf, nbytes);

	if (NULL == libpd_log_dir)
		return 0;

	pthread_mutex_lock (&log_mutex);
	
	if (current_fd != -1) {
		write (current_fd, msg_buf, nbytes);
		current_file_size += nbytes;
		if (current_file_size >= MAX_FILE_SIZE) {
			err = open_next_file ();
		}
	}
	pthread_mutex_unlock (&log_mutex);
	return err;
}


void libpd_log ( int level, int os_errno, const char *msg, ...)
{
	char *pTempChar = NULL;
	char errbuf[100];
	const char *err_msg;
	int error_len, buf_limit, nbytes;
	
	va_list arg_ptr; 

	if (level == LEVEL_NO_LOGGER) {
		if (NULL == log_handler) {
	    va_start(arg_ptr, msg);
	    vprintf(msg, arg_ptr);
	    va_end(arg_ptr);
			if (os_errno != 0)
				printf ("%s\n", strerror_r (os_errno, errbuf, 100));
		}
		return;
	}
	
	pTempChar = (char *)malloc(MSG_BUF_SIZE);
	if(pTempChar)
	{

		if (NULL == log_handler) {
			if ((level == LEVEL_ERROR) || (current_level >= level)) {
				va_start (arg_ptr, msg);
				output_log (level, pTempChar, msg, arg_ptr, os_errno);
				va_end (arg_ptr);
				free(pTempChar);
				pTempChar = NULL;
			}
			return;
		}

		buf_limit = MSG_BUF_SIZE;
		error_len = 0;
		if (os_errno != 0) {
			err_msg = strerror_r (os_errno, errbuf, 100);
			error_len = strlen (err_msg)+3;
			buf_limit -= error_len;
		}

		va_start(arg_ptr, msg); 

		nbytes = vsnprintf(pTempChar, buf_limit, msg, arg_ptr);
		if(nbytes < 0)
		{
			perror(pTempChar);
		}
		va_end(arg_ptr);

		if ((nbytes >= 0) && (os_errno != 0)) {
			sprintf (pTempChar+nbytes, ": %s\n", err_msg);
		}
		
		log_handler (level, pTempChar);
	
		if(pTempChar !=NULL)
		{
			free(pTempChar);
			pTempChar = NULL;
		}
			
	}
	return;	
}
#else

int log_init (const char *log_directory __attribute__ ((unused)), parlibLogHandler handler)
{
	log_handler = handler;

	return 0;
}

int log_shutdown (void)
{
	return 0;
}

void libpd_log ( int level, int os_errno, const char *msg, ...)
{
	char *pTempChar = NULL;
	char errbuf[100];
	const char *err_msg;
	int error_len, buf_limit, nbytes;
	
	va_list arg_ptr; 

	pTempChar = (char *)malloc(MSG_BUF_SIZE);
	if(pTempChar)
	{

		if (NULL == log_handler) {
			printf ("LIBPD: no log handler provided\n");
			return;
		}

		buf_limit = MSG_BUF_SIZE;
		error_len = 0;
		if (os_errno != 0) {
			err_msg = strerror_r (os_errno, errbuf, 100);
			error_len = strlen (err_msg)+3;
			buf_limit -= error_len;
		}

		va_start(arg_ptr, msg); 

		nbytes = vsnprintf(pTempChar, buf_limit, msg, arg_ptr);
		if(nbytes < 0)
		{
			perror(pTempChar);
		}
		va_end(arg_ptr);

		if ((nbytes >= 0) && (os_errno != 0)) {
			sprintf (pTempChar+nbytes, ": %s\n", err_msg);
		}
		
		log_handler (level, pTempChar);
	
		if(pTempChar !=NULL)
		{
			free(pTempChar);
			pTempChar = NULL;
		}
			
	}
	return;	
}

#endif

