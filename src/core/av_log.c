/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_log.c                                           */
/* Description:   Logging                                            */
/*                                                                   */
/*********************************************************************/

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <av_log.h>
#include <av_thread.h>
#include <time.h>

#ifdef _WIN32
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#endif

#define MAX_LOG_MESSAGE_SIZE 1024

#ifdef AV_MT
av_mutex_p mtx_logfile = 0;
av_mutex_p mtx_logconsole = 0;
#endif

#define CONTEXT "log_ctx"
#define O_context(o) O_attr(o, CONTEXT)

/*
* av_logger_t is a storage for arbitrary loggers
*/
typedef struct _av_logger_t
{
	void* param;                  /* custom parameter */
	char  name[MAX_NAME_SIZE];    /* logger name */
	av_log_verbosity_t verbosity; /* logging verbosity type */
	struct _av_logger_t* next;    /* the next logger in the list of loggers */

	/* Specific logging method */
	av_result_t (*log)                 (/* custom parameter */ void*, /* message */ const char*);

} av_logger_t, *av_logger_p;


/* logging methods */
static av_result_t av_log_method_console(void* param, const char* message)
{
	AV_UNUSED(param);
	if (message)
		printf("%s\n", message);

	return AV_OK;
}

static av_result_t av_log_method_file(void* param, const char* message)
{
	FILE* file = (FILE*)param;
	av_assert(file, "log file is not initialized");
	if (message)
	{
		int len = strlen(message);
		if (len != fprintf(file, "%s\n", message))
		{
			return AV_EWRITE;
		}
	}
	else
		fclose(file);

	return AV_OK;
}

/* log class implementation */

static av_result_t av_log_get_logger(av_log_p self,
									 const char* name,
									 av_logger_p *pplogger)
{
	av_logger_p alogger = (av_logger_p)O_context(self);
	while (alogger)
	{
		if ( 0 == strcmp(name, alogger->name) )
		{
			*pplogger = alogger;
			return AV_OK;
		}
		alogger = alogger->next;
	}
	return AV_EFOUND;
}

static av_result_t av_log_add_console_logger(av_log_p self,
											 av_log_verbosity_t verbosity,
											 const char* name)
{
	return self->add_custom_logger(self, verbosity, name, av_log_method_console, 0);
}

static av_result_t av_log_add_file_logger(av_log_p self,
										  av_log_verbosity_t verbosity,
										  const char* name,
										  const char* filename)
{
	return self->add_custom_logger(self, verbosity, name, av_log_method_file, (void*)fopen(filename, "w"));
}

static av_result_t av_log_add_custom_logger(av_log_p self,
											av_log_verbosity_t verbosity,
											const char* name,
											av_log_method_t logmethod,
											void *param)
{
	av_logger_p logger;
	av_assert(verbosity < LOG_VERBOSITIES_COUNT, "Invalid verbosity number");
	av_assert(name, "name can't be NULL");
	av_assert(logmethod, "logmethod  can't be NULL");

	if (!(verbosity < LOG_VERBOSITIES_COUNT) || !name || !logmethod)
		return AV_EARG;

	logger = (av_logger_p)malloc(sizeof(av_logger_t));
	if (!logger)
		return AV_EMEM;
	
	strncpy(logger->name, name, MAX_NAME_SIZE);
	logger->verbosity     = verbosity;
	logger->log           = logmethod;
	logger->param         = param;
	logger->next          = (av_logger_p)O_context(self);
	O_set_attr(self, CONTEXT, logger);
	return AV_OK;
}

static av_result_t av_log_set_verbosity(av_log_p self,
										const char* name,
										av_log_verbosity_t verbosity)
{
	av_result_t res;
	av_logger_p logger;
	av_assert(verbosity < LOG_VERBOSITIES_COUNT, "Invalid verbosity number");
	av_assert(name, "name can't be NULL");

	if (!(verbosity < LOG_VERBOSITIES_COUNT) || !name)
		return AV_EARG;
	
	res = av_log_get_logger(self, name, &logger);
	if (AV_OK == res)
	{
		logger->verbosity = verbosity;
	}
	return res;
}

static av_result_t av_log_get_verbosity(av_log_p self, 
										const char* name, 
										av_log_verbosity_t *verbosity)
{
	av_result_t res;
	av_logger_p logger;
	av_assert(name, "name can't be NULL");

	res = av_log_get_logger(self, name, &logger);
	if (AV_OK == res)
	{
		*verbosity = logger->verbosity;
	}
	return res;
}

static void av_log_format_message(char* buffer,
								  int maxbuflen,
								  av_log_verbosity_t verbosity,
								  const char* msg)
{
	/* current time given in seconds since 1st Jan 1970 */
	time_t now = time(NULL);

	/* verbosity names as seen in the log output */
	static const char *verbosity_names[LOG_VERBOSITIES_COUNT] = 
		{"silent", "error", "warn", "info", "debug"};

	/* gets the current date/time as string in format "Wed Jun 30 21:49:08 1993\n" */
	char* timebuf = strdup(asctime(localtime(&now)));
	int lastchr = strlen(timebuf)-1;
	/* removes the last character '\n' (newline) resulted by the asctime function */
	if (timebuf[lastchr] == '\n') /* doesn't heart to check if really the char \n is there */
	{
		timebuf[lastchr] = 0; /* removes the last */
	}

	av_assert(verbosity < LOG_VERBOSITIES_COUNT, "Invalid verbosity number");

	snprintf(buffer, maxbuflen, "%s <%s>: %s", timebuf, verbosity_names[verbosity], msg);
	free(timebuf);
}

static av_result_t av_log_log(av_log_p self, 
							  av_log_verbosity_t verbosity, 
							  const char* message)
{
	av_result_t rc = AV_OK;
	av_logger_p alogger = (av_logger_p)O_context(self);
	while (alogger)
	{
		if (verbosity <= alogger->verbosity)
		{
			av_result_t lrc;
			char formatedmsg[MAX_LOG_MESSAGE_SIZE];
			av_log_format_message(formatedmsg, MAX_LOG_MESSAGE_SIZE, verbosity, message);
			if (AV_OK != (lrc = alogger->log(alogger->param, formatedmsg)))
			{
				rc = lrc;
			}
		}
		alogger = alogger->next;
	}
	return rc;
}

#define LOG_METHOD_IMPL(verb)                           \
{                                                       \
	va_list args;                                       \
	char buffer[MAX_LOG_MESSAGE_SIZE];                  \
	va_start(args, fmt);                                \
	vsnprintf(buffer, MAX_LOG_MESSAGE_SIZE, fmt, args); \
	va_end(args);                                       \
	return self->log(self, verb, buffer);               \
}

static av_result_t av_log_error(av_log_p self, const char* fmt, ...)
	LOG_METHOD_IMPL(LOG_VERBOSITY_ERROR)
static av_result_t av_log_warn(av_log_p self, const char* fmt, ...)
	LOG_METHOD_IMPL(LOG_VERBOSITY_WARN)
static av_result_t av_log_info(av_log_p self, const char* fmt, ...)
	LOG_METHOD_IMPL(LOG_VERBOSITY_INFO)
static av_result_t av_log_debug(av_log_p self, const char* fmt, ...)
	LOG_METHOD_IMPL(LOG_VERBOSITY_DEBUG)

/*	Destroys log object */
static void av_log_destructor(void* self)
{
	av_logger_p alogger = (av_logger_p)O_context(self);
	av_logger_p plogger = 0;

	while (alogger)
	{
		/* notifies the logger to free its occuppied memory */
		alogger->log(alogger->param, 0);
		plogger = alogger;
		alogger = alogger->next;
		free(plogger);
	}
}

/*	Initializes memory given by the input pointer with the log class information */
static av_result_t av_log_constructor(av_object_p object)
{
	av_log_p self                = (av_log_p)object;
	self->add_console_logger     = av_log_add_console_logger;
	self->add_file_logger        = av_log_add_file_logger;
	self->add_custom_logger      = av_log_add_custom_logger;
	self->set_verbosity          = av_log_set_verbosity;
	self->get_verbosity          = av_log_get_verbosity;
	self->log                    = av_log_log;
	self->error                  = av_log_error;
	self->warn                   = av_log_warn;
	self->info                   = av_log_info;
	self->debug                  = av_log_debug;
	return AV_OK;
}

/* Registers log class into TORBA */
av_result_t av_log_register_torba(void)
{
	av_service_p log;
	av_result_t rc;

	if (AV_OK != (rc = av_torb_register_class("log", "service", sizeof(av_log_t), av_log_constructor, av_log_destructor)))
		return rc;

	if (AV_OK != (rc = av_torb_create_object("log", (av_object_p*)&log)))
		return rc;

	return av_torb_service_register("log", log);
}
