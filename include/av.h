/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av.h                                               */
/*                                                                   */
/*********************************************************************/

/*! \file av.h
*   \brief A header with common definitions used by AVGL library
*/

#ifndef __AV_H
#define __AV_H

#ifdef __cplusplus
extern "C" {
#endif

/*!
* Defines API exportation type
*/
#ifndef AV_API
#  ifdef _WIN32
#    ifndef AVGL_STATIC_LIB
#      ifdef AVGL_EXPORTS
#        define AV_API __declspec (dllexport)
#      else
#        define AV_API __declspec (dllimport)
#      endif
#    else
#      define AV_API
#    endif
#  else
#    define AV_API extern
#  endif
#endif

/*! \def av_assert
*   \brief Defines portable assert function
*   \param exp is an expression assummed to evaluate as non NULL value
*   \param msg is a message accompagning the assert exception
*/

/*! \def AV_DEBUG
*   \brief Defines debug compilation mode.
*          Without defining this macro the build is considered as a production ready,
*          av_assert macros are defined empty and the macro NDEBUG is defined
*/

#define AV_DEBUG

#ifdef AV_DEBUG
#  include <stdio.h>
#  include <stdlib.h>
#  define av_assert(exp, msg) if (!(exp)) \
  { fprintf(stderr, "%s:%d assertion %s FAILED - %s\n", __FILE__, __LINE__, #exp, msg); exit(1); }
#else
#  define av_assert(exp, msg)
#  ifndef NDEBUG
#    define NDEBUG
#  endif
#endif

/*!
* \brief Enable multithreading support.
*
* When the macro is not defined the thread module is undefined except 
* av_mutex and av_rwmutex classes which are defined but empty
* (e.g. do nothing when locked/unlocked)
*/
#define AV_MT

/*!
* Declares variable as unused
*/
#define AV_UNUSED(v) v=v

/*!
* Makes long from two short values 
*/
#define AV_MAKE_LONG(l, h) (unsigned long)(((l) & 0xFFFF)|(unsigned long)((h)<<16))

/*!	
* Returns the low short value from a long 
*/
#define AV_WORD_LO(l)        (short)(((l) & 0xFFFF))

/*!	
* Returns the high short value from a long 
*/
#define AV_WORD_HI(l)        (short)(((l) >> 16))

/*!	
* Returns the greater between two values 
*/
#define AV_MAX(a, b)         ((a)>(b)?(a):(b))

/*!	
* Returns the lower between two values 
*/
#define AV_MIN(a, b)         ((a)>(b)?(b):(a))

/*!	
* Checks if the first parameter is inclusively located in between 2nd and 3rd parameter
*/
#define AV_IN_RANGE(v, l, r)    ((v)>=(l) && (v)<=(r))

/*!	
* Checks if value matchs all bits of the given mask
*/
#define AV_MASK_ENABLED(v, m) ((m) == ((v) & (m)))

/*!	
* Checks if value matchs at least one bit of the given mask
*/
#define AV_MASK_ANY(v, m) (0 != ((v) & (m)))

/*!	
* Checks if value doesn't match any bits of the given mask
*/
#define AV_MASK_DISABLED(v, m) (0 == ((v) & (m)))

/*!	
* Execution status
*/
typedef enum 
{
	/* Common AVGL statuses */

	/*! Successful completion */
	/* 0 */ AV_OK = 0,

	/*! Not enough memory */
	/* 1 */ AV_EMEM,

	/*! Invalid argument */
	/* 2 */ AV_EARG,

	/*! Something not found or not occurred */
	/* 3 */ AV_EFOUND,

	/*! Something is not supported */
	/* 4 */ AV_ESUPPORTED,

	/*! Error reading */
	/* 5 */ AV_EREAD,

	/*! Error writing */
	/* 6 */ AV_EWRITE,

	/*! Error seeking */
	/* 7 */ AV_ESEEK,

	/*! General I/O error */
	/* 8 */ AV_EIO,

	/*! End of file reached */
	/* 9 */ AV_EEOF,

	/*! General failure (caused by external libraries) */
	/* 10 */ AV_EGENERAL,

	/* Thread statuses */

	/*! busy during operation */
	/* 11 */ AV_EBUSY,

	/*! dead lock */
	/* 12 */ AV_EDEADLK,

	/*! permission denied */
	/* 13 */ AV_EPERM,
	
	/*! out of resources */
	/* 14 */ AV_EAGAIN,

	/*! timed out */
	/* 15 */ AV_ETIMEDOUT,
	
	/*! interrupted error */
	/* 16 */ AV_EINTERRUPT,
	
	/*! no such process */
	/* 17 */ AV_ENOSUCHPROCESS,

	/* Graphics statuses */

	/*! restore without matching save */
	/* 18 */ AV_ERESTORE,

	/*! No current point defined */
	/* 19 */ AV_ECURRENT_POINT,

	/*! Invalid matrix */
	/* 20 */ AV_EMATRIX,

	/*! Target surface has been finished */
	/* 21 */ AV_ESURFACE_FINISHED,
	
	/*! The surface type is not appropriate for the operation */
	/* 22 */ AV_ESURFACE,

	/*! The pattern type is not appropriate for the operation */
	/* 23 */ AV_EPATTERN,

	/* Video statuses */

	/*! Not enough video memory */
	/* 24 */ AV_EVMEM,

	/*! The surface is already locked */
	/* 25 */ AV_ELOCKED,

	/*! A limit has been exceeded */
	/* 26 */ AV_ELIMIT,

	/*! The requested object is suspended */
	/* 27 */ AV_ESUSPENDED,

	/*! Initialization error (caused by external libraries) */
	/* 28 */ AV_EINIT,

	/*! Error in script */
	/* 29 */ AV_ESCRIPT,

	/*! Invalid operation state */
	/* 30 */ AV_ESTATE
} av_result_t;

/*!	
* Boolean type
*/
typedef enum 
{
	AV_FALSE = 0,
	AV_TRUE = 1
} av_bool_t;

/*!	
* Value for no value
*/
#define AV_NULL 0

#define AV_MAXINT 2147483647

/*!	
* Maximum size for strings used to store logger names, font names and declarations like `itallic', `bold' etc
*/
#define MAX_NAME_SIZE 50

/*!	\def MAX_FILENAME_SIZE
*   \brief Defines the maximum allowed filename size
*/
#ifdef PATH_MAX
#define MAX_FILENAME_SIZE PATH_MAX
#else
#ifdef MAXPATHLEN
#define MAX_FILENAME_SIZE MAXPATHLEN
#else
#define MAX_FILENAME_SIZE 512
#endif
#endif

#ifdef M_PI
#define AV_PI M_PI
#else
#define AV_PI 3.14159265358979323846
#endif

#ifdef __cplusplus
}
#endif

#endif
