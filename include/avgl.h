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
#include <av_bitmap.h>
#include <av_visible.h>
#include <av_sprite.h>
#include <av_stdc.h>

typedef void (*on_paint_t)(av_visible_p visible, av_graphics_p graphics);

AV_API av_visible_p avgl_create(av_display_config_p pdc);
AV_API void avgl_capture_visible(av_visible_p visible);
AV_API av_result_t avgl_last_error();
AV_API void avgl_loop();
AV_API av_bool_t avgl_step();
AV_API void avgl_destroy();
AV_API unsigned long avgl_time_now();
AV_API void avgl_event_push(av_event_p event);
AV_API av_bool_t avgl_event_poll(av_event_p event);
AV_API av_bitmap_p avgl_load_bitmap(const char* filename);
AV_API av_surface_p avgl_load_surface(const char* filename);

#endif /* __AVGL_H */
