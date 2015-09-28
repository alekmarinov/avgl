/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_xembed_window.h                                 */
/*                                                                   */
/*********************************************************************/

#ifndef __AV_XEMBED_WINDOW_H
#define __AV_XEMBED_WINDOW_H

#include <av_window.h>
#include <av_torb.h>
#include <av_rect.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct av_xembed_window
{
	av_object_t object;

	av_result_t (*open)(struct av_xembed_window* self, av_window_p parent, av_rect_p rect);
	av_result_t (*show)(struct av_xembed_window* self);
	void (*set_scale_start)(struct av_xembed_window* self, double scale_start);
	void (*scale)(struct av_xembed_window* self, double scale, int offsetx, int offsety);
	void (*hide)(struct av_xembed_window* self);
	void (*idle)(struct av_xembed_window* self);
	av_result_t (*screenshot)(struct av_xembed_window* self, const char* filename);
	double (*get_scale)(struct av_xembed_window* self);
	void (*get_offsets)(struct av_xembed_window* self, int* poffsetx, int* poffsety);
	void (*get_window)(struct av_xembed_window* self, av_window_p* ppwindow);
	void (*on_mouse_event)(struct av_xembed_window* self, const char* eventname, int x, int y);
	void (*set_mouse_visible)(struct av_xembed_window* self, av_bool_t mouse_visible);

	void (*send_mouse_button)(struct av_xembed_window* self,
							  av_event_mouse_button_t button,
							  av_event_button_status_t status);
	void (*send_mouse_move)  (struct av_xembed_window* self, int x, int y);
	void (*set_mouse_capture)(struct av_xembed_window* self, av_bool_t capture);
	void (*set_cursor)(struct av_xembed_window* self, av_video_cursor_shape_t cursor);
	void (*set_cursor_visible)(struct av_xembed_window* self, av_bool_t cursor_visible);
	void (*set_xcursor_visible)(struct av_xembed_window* self, av_bool_t cursor_visible);
	void (*focus)(struct av_xembed_window* self);
	av_bool_t (*is_focus)(struct av_xembed_window* self);

	/*!
	 * \brief Sends key event to X server
	 *
	 * \param self is a reference to this object
	 * \param key which key
	 * \param key modifiers
	 * \param status is a key status (\sa av_event_button_status_t)
	 */
	void (*send_key)         (struct av_xembed_window* self,
							  av_key_t key,
							  av_keymod_t modifiers,
							  av_event_button_status_t status);

} av_xembed_window_t, *av_xembed_window_p;

av_result_t av_xembed_window_register_torba(void);

#ifdef __cplusplus
}
#endif

#endif /* __AV_XEMBED_WINDOW_H */
