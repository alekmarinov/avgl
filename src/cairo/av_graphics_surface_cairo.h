/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_graphics_surface_cairo.h                        */
/* Description:   Cairo graphics surface interface                   */
/*                                                                   */
/*********************************************************************/


#ifndef __AV_GRAPHICS_SURFACE_CAIRO_H
#define __AV_GRAPHICS_SURFACE_CAIRO_H

#include <av_graphics.h>
#include <av_oop.h>

av_result_t av_graphics_surface_cairo_register_oop(av_oop_p);
av_result_t av_graphics_surface_cairo_init_class(void*, const char*);

typedef struct av_graphics_surface_cairo
{
	/*! parent object */
	av_graphics_surface_t graphics_surface;

	/*! wrapped surface */
	av_surface_p wrap_surface;

	/*! locking flags */
	av_surface_lock_flags_t lockflags;

	/*! wrapped surface pixels */
	av_pixel_p pixels;

	/*! wrapped surface pitch */
	int pitch;

} av_graphics_surface_cairo_t, *av_graphics_surface_cairo_p;

#define CONTEXT_GRAPHICS_SURFACE "graphics_surface_cairo_ctx"
#define CONTEXT_GRAPHICS_PATTERN "graphics_pattern_cairo_ctx"

#endif /* __AV_GRAPHICS_SURFACE_CAIRO_H */
