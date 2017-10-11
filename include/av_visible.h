/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_visible.h                                       */
/*                                                                   */
/*********************************************************************/

/*! \file av_visible.h
*   \brief Base visible
*/

#ifndef __AV_VISIBLE_H
#define __AV_VISIBLE_H

#include <av_oop.h>

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
struct _av_system_t;

/*!
* \brief visible interface
*
*/
struct _av_visible_t;

typedef void(*on_draw_t)(struct _av_visible_t* self, av_graphics_p graphics);
typedef void(*on_destroy_t)(struct _av_visible_t* self);

typedef struct _av_visible_t
{
	av_window_t window;

	struct _av_system_t* system;
	av_surface_p surface;
	av_bool_t is_owner_draw;

	av_result_t (*draw)   (struct _av_visible_t* self);
	void (*render)        (struct _av_visible_t* self, av_rect_p src_rect, av_rect_p dst_rect);
	void (*on_tick)       (struct _av_visible_t* self);
	void (*on_draw)       (struct _av_visible_t* self, av_graphics_p graphics);
	void (*on_destroy)    (struct _av_visible_t* self);
	void (*set_surface)   (struct _av_visible_t* self, av_surface_p surface);
	av_result_t           (*create_child) (struct _av_visible_t* self, const char* classname, struct _av_visible_t **pvisible);

} av_visible_t, *av_visible_p;

/*!
* \brief Registers sprite class into OOP
* \return av_result_t
*         - AV_OK on success
*         - != AV_OK on error
*/
AV_API av_result_t av_visible_register_oop(av_oop_p);

#ifdef __cplusplus
}
#endif

#endif /* __AV_SPRITE_H */
