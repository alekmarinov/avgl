/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_xembed_app.h                                    */
/*                                                                   */
/*********************************************************************/

/* DEPRICATED MODULE */

#ifndef __AV_XEMBED_APP_H
#define __AV_XEMBED_APP_H

#include <sys/types.h>
#include <av_torb.h>
#include <av_window.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief xembed configuration type
*
*/
typedef struct av_xembed_app_config
{
	/*! the window to be used for X application display */
	av_window_p window;

	/*! X application pid */
	pid_t pid;
} av_xembed_app_config_t, *av_xembed_app_config_p;

/*!
* \brief Controls and display xembeded application
*
*/
typedef struct av_xembed_app
{
	/*! Parent class object */
	av_object_t object;

	/*!
	* \brief Sets xembed app configuration
	*
	* \param self is a reference to this object
	* \param config new configuration
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on error
	*/
	av_result_t (*set_configuration)(struct av_xembed_app* self,
									 av_xembed_app_config_p config);

	/*!
	* \brief Gets xembed app configuration
	*
	* \param self is a reference to this object
	* \param config result with the current configuration
	*/
	void        (*get_configuration)(struct av_xembed_app* self,
									 av_xembed_app_config_p config);
} av_xembed_app_t, *av_xembed_app_p;

AV_API av_result_t av_xembed_app_register_class( void );

#ifdef __cplusplus
}
#endif

#endif /* __AV_XEMBED_H */
