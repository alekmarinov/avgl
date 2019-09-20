/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_log.h                                           */
/*                                                                   */
/*********************************************************************/

/*! \file av_log.h
*   \brief av_log definition
*/

#ifndef __AV_LOG_H
#define __AV_LOG_H

#include <av.h>
#include <av_oop.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! Logging verbosity types */
typedef enum 
{
	/*! Silent logging. */
	LOG_VERBOSITY_SILENT = 0,
	
	/*! Log important messages like fatal errors notifications */
	LOG_VERBOSITY_ERROR,

	/*! Log warning messages. Used for non fatal errors */
	LOG_VERBOSITY_WARN,
	
	/*!
	* Log informative messages. Used for following the application flow.
	* It shouldn't be very verbose.
	*/
	LOG_VERBOSITY_INFO,

	/*! Log debug messages. Very verbose. */
	LOG_VERBOSITY_DEBUG,

	/*! The number of all verbosity levels */
	LOG_VERBOSITIES_COUNT

} av_log_verbosity_t;

/*!
* A prototype for custom loggers. In the last call \c message is NULL 
* indicating to the logger to free its used memory
*/
typedef av_result_t (*av_log_method_t)(/*! custom parameter */ void*, /*! message */ const char*);

/*! 
* \brief logger type
*/
typedef struct av_log
{
	/*! Parent object */
	av_service_t service;

	/*!
	* \brief Adds new console logger
	* \param self is a reference to this object
	* \param verbosity logging level
	* \param name identifying the added console logger
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*add_console_logger)(struct av_log* self,
									  av_log_verbosity_t verbosity,
									  const char* name);

	/*!
	* \brief Adds new file logger
	* \param self is a reference to this object
	* \param verbosity logging level
	* \param name identifying the added file logger
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*add_file_logger)   (struct av_log* self,
									  av_log_verbosity_t verbosity,
									  const char* name,
									  const char* filename);

	/*!
	* \brief Adds new custom logger
	* \param self is a reference to this object
	* \param verbosity logging level
	* \param name identifying the added custom logger
	* \param logmethod is a callback function with prototype \c av_log_method_t
	* \param param is a custom parameter to the \c logmethod 
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*add_custom_logger) (struct av_log* self,
									  av_log_verbosity_t verbosity,
									  const char* name,
									  av_log_method_t ,
								      void *param
	);

	/*!
	* \brief Sets a verbosity level to a logger given by name
	* \param self is a reference to this object
	* \param name of the logger or null to set the same verbosity on all loggers
	* \param verbosity is the new verbosity logging level
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EFOUND if the given logger name is not registered
	*         - AV_EARG if the given logger name is NULL
	*					or the verbosity is not any defined by \c av_log_verbosity_t enum
	*/
	av_result_t (*set_verbosity)     (struct av_log* self,
									  const char* name,
									  av_log_verbosity_t verbosity);

	/*!
	* \brief Gets the verbosity from a logger given by name
	* \param self is a reference to this object
	* \param name of the logger used when the logger has been added
	* \param pverbosity points the verbosity result
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EFOUND if the given logger name is not registered
	*         - AV_EARG if the given logger name is NULL
	*/
	av_result_t (*get_verbosity)     (struct av_log* self,
									  const char* name,
									  av_log_verbosity_t* pverbosity);

	/*!
	* \brief Sends a log \c message to the loggers having a verbosity
	*        higher or equal to the given \c verbosity
	* \param self is a reference to this object
	* \param verbosity is the verbosity level of the logged message
	* \param message to be logged
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EARG if the verbosity is not any defined by \c av_log_verbosity_t enum
	*         - error code returned by any of the logger's logging method
	*/
	av_result_t (*log)               (struct av_log* self,
									  av_log_verbosity_t verbosity,
									  const char* message);

	/*!
	* \brief Logs with \c LOG_VERBOSITY_ERROR verbosity
	* \param self is a reference to this object
	* \param format is a string format
	* \return av_result_t
	*         - AV_OK on success
	*         - error code returned by any of the logger's logging method
	*/
	av_result_t (*error)             (struct av_log* self,
									  const char* format, ...);

	/*!
	* \brief Logs with \c LOG_VERBOSITY_WARN verbosity
	* \param self is a reference to this object
	* \param format is a string format
	* \return av_result_t
	*         - AV_OK on success
	*         - error code returned by any of the logger's logging method
	*/
	av_result_t (*warn)             (struct av_log* self, /*! format */ const char*, /*! arguments */ ...);

	/*!
	* \brief Logs with \c LOG_VERBOSITY_INFO verbosity
	* \param self is a reference to this object
	* \param format is a string format
	* \return av_result_t
	*         - AV_OK on success
	*         - error code returned by any of the logger's logging method
	*/
	av_result_t (*info)             (struct av_log* self, /*! format */ const char*, /*! arguments */ ...);

	/*!
	* \brief Logs with \c LOG_VERBOSITY_DEBUG verbosity
	* \param self is a reference to this object
	* \param format is a string format
	* \return av_result_t
	*         - AV_OK on success
	*         - error code returned by any of the logger's logging method
	*/
	av_result_t (*debug)             (struct av_log* self, /*! format */ const char*, /*! arguments */ ...);

} av_log_t, *av_log_p;

/*!
* \brief Registers log class into oop container
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_log_register_oop(av_oop_p);

#ifdef __cplusplus
}
#endif

#endif
