/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_graphics_surface_cairo.c                        */
/* Description:   Cairo graphics surface implementation              */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_GRAPHICS_CAIRO

#include <malloc.h>
#include <cairo.h>
#include "av_graphics_surface_cairo.h"

#define O_context(o) ((cairo_surface_t*)O_attr(o, CONTEXT_GRAPHICS_SURFACE))

static av_result_t av_graphics_surface_cairo_set_size(av_surface_p self, int width, int height)
{
	cairo_surface_t* cairo_surface_old = O_context(self);
	cairo_surface_t* cairo_surface_new = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	if (!cairo_surface_new)
		return AV_EMEM;
	if (cairo_surface_old)
		cairo_surface_destroy(cairo_surface_old);
	O_set_attr(self, CONTEXT_GRAPHICS_SURFACE, cairo_surface_new);
	return AV_OK;
}

static av_result_t av_graphics_surface_cairo_get_size(av_surface_p self, int* pwidth, int* pheight)
{
	cairo_surface_t* cairo_surface = O_context(self);
	av_assert(cairo_surface, "cairo surface is not initialized properly");
	*pwidth = cairo_image_surface_get_width(cairo_surface);
	*pheight = cairo_image_surface_get_height(cairo_surface);
	return AV_OK;
}

static av_result_t av_graphics_surface_cairo_lock(av_surface_p self, av_pixel_p* ppixels, int* ppitch)
{
	cairo_surface_t* cairo_surface = O_context(self);
	av_assert(cairo_surface, "cairo surface is not initialized properly");
	*ppixels = (int*)cairo_image_surface_get_data(cairo_surface);
	*ppitch = cairo_image_surface_get_stride(cairo_surface);
	return AV_OK;
}

static av_result_t av_graphics_surface_cairo_unlock(av_surface_p psurface)
{
	AV_UNUSED(psurface);
	return AV_OK;
}

static av_result_t av_graphics_surface_cairo_create_pattern(av_graphics_surface_p self,
															av_graphics_pattern_p *ppattern)
{
	av_result_t rc;
	av_graphics_pattern_p pattern;
	av_oop_p oop = O_oop(self);
	cairo_surface_t* cairo_surface = O_context(self);
	cairo_pattern_t *p = cairo_pattern_create_for_surface(cairo_surface);
	if (!p)
		return AV_EMEM;

	if (AV_OK != (rc = oop->new(oop, "graphics_pattern_cairo", (av_object_p*)&pattern)))
	{
		cairo_pattern_destroy(p);
		return rc;
	}

	O_set_attr(pattern, CONTEXT_GRAPHICS_PATTERN, p);

	*ppattern = pattern;
	return AV_OK;
}

/* Writes surface to file */
static av_result_t av_graphics_surface_save(av_graphics_surface_p self,
	                                        const char* filename)
{
	cairo_surface_t* cairo_surface = O_context(self);
	AV_UNUSED(self);

	/* FIXME: What file format? */
	return av_cairo_error_check("cairo_surface_write_to_png", cairo_surface_write_to_png(cairo_surface, filename));
}

static void av_graphics_surface_cairo_destructor(av_object_p self)
{
	cairo_surface_t* cairo_surface = O_context(self);
	av_graphics_surface_p graphics_surface = (av_graphics_surface_p)self;
	if (graphics_surface->graphics)
		O_release(graphics_surface->graphics);

	if (cairo_surface)
		cairo_surface_destroy(cairo_surface);
}

static av_result_t av_graphics_surface_cairo_constructor(av_object_p object)
{
	av_surface_p self = (av_surface_p)object;

	self->set_size   = av_graphics_surface_cairo_set_size;
	self->get_size   = av_graphics_surface_cairo_get_size;
	self->lock       = av_graphics_surface_cairo_lock;
	self->unlock     = av_graphics_surface_cairo_unlock;
	((av_graphics_surface_p)self)->create_pattern = av_graphics_surface_cairo_create_pattern;
	((av_graphics_surface_p)self)->save = av_graphics_surface_save;
	return AV_OK;
}

/* Registers Cairo video surface class into OOP class repository */
av_result_t av_graphics_surface_cairo_register_oop(av_oop_p oop)
{
	av_surface_register_oop(oop);
	return oop->define_class(oop, "graphics_surface_cairo", "surface", // FIXME: surface or graphics_surface
								  sizeof(av_graphics_surface_t),
								  av_graphics_surface_cairo_constructor,
								  av_graphics_surface_cairo_destructor);
}

#endif /* WITH_GRAPHICS_CAIRO */
