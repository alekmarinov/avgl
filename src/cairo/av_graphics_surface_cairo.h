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

#define CONTEXT_GRAPHICS_SURFACE "graphics_surface_cairo_ctx"
#define CONTEXT_GRAPHICS_PATTERN "graphics_pattern_cairo_ctx"

#endif /* __AV_GRAPHICS_SURFACE_CAIRO_H */
