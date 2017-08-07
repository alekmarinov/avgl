/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      avgl.h                                             */
/*                                                                   */
/*********************************************************************/

/*! \file avgl.h
*   \brief A header including all AVGL header files
*/

#ifndef __AVGL_H
#define __AVGL_H

#include <av.h>
#include <av_audio.h>
#include <av_event.h>
#include <av_graphics.h>
#include <av_hash.h>
#include <av_input.h>
#include <av_keys.h>
#include <av_list.h>
#include <av_log.h>
#include <av_media.h>
#include <av_player.h>
#include <av_prefs.h>
#include <av_rect.h>
#include <av_scripting.h>
#include <av_surface.h>
#include <av_system.h>
#include <av_thread.h>
#include <av_timer.h>
#include <av_oop.h>
#include <av_tree.h>
#include <av_display.h>
#include <av_window.h>
#include <av_stdc.h>

typedef struct _avgl_t
{
	/*! implementation specific */
	void* ctx;

	/*!
	* \brief Main application loop
	* \param self is a reference to this object
	*/
	void(*loop)(void* self);

	/*!
	* \brief Default object destructor
	* \param self is a void* pointer to this object
	*/
	void(*destroy)(void* self);

} avgl_t, *avgl_p;

AV_API av_result_t avgl_create(avgl_p* pself);

#endif /* __AVGL_H */
