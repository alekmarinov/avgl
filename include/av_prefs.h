/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_prefs.h                                         */
/*                                                                   */
/*********************************************************************/

/*! \file av_prefs.h
*   \brief av_prefs definition
*/

#ifndef __AV_PREFS_H
#define __AV_PREFS_H

#include <av_oop.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	AV_PREFS_VALUE_UNDEF,
	AV_PREFS_VALUE_NUMBER,
	AV_PREFS_VALUE_STRING
} av_prefs_value_type_t;

typedef struct av_prefs_value
{
	av_prefs_value_type_t value_type;
	char* name;
	int nvalue;
	char* svalue;
} av_prefs_value_t, *av_prefs_value_p;

typedef void (*av_prefs_observer_t)(void* param, const char* name, av_prefs_value_p value);

/*!
* \brief Preferences
*
* Defines preferences storage
*/
typedef struct av_prefs
{
	av_service_t service;

	av_result_t (*set_int)            (struct av_prefs* self,
									   const char* name,
									   int nvalue);

	av_result_t (*get_int)            (struct av_prefs* self,
									   const char* name,
									   int ndefault,
									   int* nvalue);

	av_result_t (*set_string)         (struct av_prefs* self,
									   const char* name,
									   const char* svalue);

	av_result_t (*get_string)         (struct av_prefs* self,
									   const char* name,
								       const char* sdefault,
									   const char** svalue);

	av_result_t (*register_observer)  (struct av_prefs* self,
									   const char* name,
									   av_prefs_observer_t observer,
									   void* param);

	av_result_t (*unregister_observer)(struct av_prefs* self,
									   av_prefs_observer_t observer);

	av_result_t (*load)               (struct av_prefs* self,
									   const char* filename);

	av_result_t (*save)               (struct av_prefs* self,
									   const char* filename);

	const char* (*get_prefs_file)     (struct av_prefs* self);
} av_prefs_t, *av_prefs_p;

/*!
* \brief Registers prefs class into TORBA
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_prefs_register_torba(void);

#ifdef __cplusplus
}
#endif

#endif /* __AV_PREFS_H */
