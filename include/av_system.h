/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_system.h                                        */
/*                                                                   */
/*********************************************************************/

/*! \file av_system.h
*   \brief av_system definition
*/

#ifndef __AV_SYSTEM_H
#define __AV_SYSTEM_H

#include <av_audio.h>
#include <av_display.h>
#include <av_input.h>
#include <av_timer.h>
#include <av_window.h>
#include <av_bitmap.h>
#include <av_surface.h>
#include <av_visible.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief system interface
*
* OS abstraction layer dialing with system events,
* providing audio, video and graphics interfaces
*/
typedef struct _av_system_t
{
	/*! Parent class service */
	av_service_t service;

	/*! Audio object */
	av_audio_p audio;

	/*! Display object */
	av_display_p display;

	/*! Graphics object */
	av_graphics_p graphics;

	/*! Input object */
	av_input_p input;

	/*! Timer object */
	av_timer_p timer;

	/*!
	* \brief Process one system event
	* \param self is a reference to this object
	* \return - AV_FALSE is the last processed event has been AV_EVENT_QUIT
	*         - AV_TRUE otherwise
	*/
	av_bool_t (*step)             (struct _av_system_t* self);

	int (*flush_events)           (struct _av_system_t* self);

	/*!
	* \brief Main application loop
	* \param self is a reference to this object
	*/
	void (*loop)                  (struct _av_system_t* self);

	av_result_t(*invalidate_rect)  (struct _av_system_t* self, av_rect_p rect);
	av_result_t(*invalidate_rects) (struct _av_system_t* self, av_list_p rects);
	av_visible_p (*get_root_visible) (struct _av_system_t* self);
	void  (*set_root_visible) (struct _av_system_t* self, av_visible_p root);
	av_result_t (*create_bitmap) (struct _av_system_t* self, av_bitmap_p* pbitmap);

	/*!
	* \brief Captures window to receive all mouse event
	* \param self is a reference to this object
	* \param window to be captured or AV_NULL to uncapture
	*/
	void (*set_capture)           (struct _av_system_t* self, av_window_p window);

	av_result_t (*initialize)            (struct _av_system_t* self, av_display_config_p pdc);

} av_system_t, *av_system_p;

/*!
* \brief Registers system class into oop
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_system_register_oop(av_oop_p);

#ifdef __cplusplus
}
#endif

#endif /* __AV_SYSTEM_H */
