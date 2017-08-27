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

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
struct av_system_t;



/*!
* \brief visible interface
*
*/
typedef struct _av_visible_t
{
	av_window_t window;

	struct av_system_t* system;

	av_surface_p surface;

	av_result_t (*draw)(struct _av_visible_t* self);

	void (*on_draw)(struct _av_visible_t* self, av_graphics_p graphics);

} av_visible_t, *av_visible_p;

typedef void (*on_draw_t)(av_visible_p self, av_graphics_p graphics);

/*!
* \brief system interface
*
* OS abstraction layer dialing with system events,
* providing audio, video and graphics interfaces
*/
typedef struct av_system
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
	av_bool_t (*step)             (struct av_system* self);

	int (*flush_events)           (struct av_system* self);

	/*!
	* \brief Main application loop
	* \param self is a reference to this object
	*/
	void (*loop)                  (struct av_system* self);

	av_result_t (*create_visible) (struct av_system* self, av_visible_p parent, av_rect_p rect, av_visible_p *pvisible);

	/*!
	* \brief Captures window to receive all mouse event
	* \param self is a reference to this object
	* \param window to be captured or AV_NULL to uncapture
	*/
	void (*set_capture)           (struct av_system* self, av_window_p window);

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
