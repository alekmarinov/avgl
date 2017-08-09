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

#define O_context(o) O_attr(o, CONTEXT_GRAPHICS_SURFACE)

#ifdef WITH_SYSTEM_DIRECTFB
#error "DirectFB is not supported yet"
#endif

#ifdef WITH_SYSTEM_DIRECTFB
/* CAIRO_HAS_DIRECTFB_SURFACE can be defined by cairo.h */
#  ifdef CAIRO_HAS_DIRECTFB_SURFACE
#  include "../video/av_video_dfb.h"
cairo_public cairo_surface_t *cairo_directfb_surface_create(IDirectFB *, IDirectFBSurface *);
#  endif
#endif

static av_result_t av_graphics_surface_cairo_set_size(av_surface_p psurface, int width, int height)
{
	av_result_t rc;
	av_graphics_surface_cairo_p self = (av_graphics_surface_cairo_p)psurface;
	cairo_surface_t* cairo_surface = AV_NULL;

	if (AV_NULL == self->wrap_surface)
	{
		/* creates 32bit cairo image surface */
		cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
		if (!cairo_surface)
			return AV_EMEM;
	}
	else
	{
		/* creates 32bit cairo wrapped surface */
		av_surface_p wrap = self->wrap_surface;
		if (AV_OK != (rc = wrap->set_size(wrap, width, height)))
		{
			return rc;
		}

		/* Try to wrap the surface with cairo if supported */
#ifdef WITH_SYSTEM_DIRECTFB
#  ifdef CAIRO_HAS_DIRECTFB_SURFACE
		if (O_is_a(wrap, "video_surface"))
		{
			IDirectFB *dfb;
			IDirectFBSurface *dfbsurface;
			av_video_surface_p vsdfb = (av_video_surface_p)wrap;
			if (O_is_a(vsdfb->video, "video_dfb"))
			{
				dfbsurface = (IDirectFBSurface *)O_attr(vsdfb, CONTEXT_DFB_SURFACE);
				av_assert(dfbsurface && ((av_video_surface_p)vsdfb)->video, "wrapped surface is not initialized properly");
				dfb = ((av_video_dfb_p)(((av_video_surface_p)vsdfb)->video))->dfb;
				av_assert(dfb, "wrapped surface is not initialized properly");

				cairo_surface = cairo_directfb_surface_create(dfb, dfbsurface);
			}
		}
#  endif
#endif

		if (AV_NULL == cairo_surface)
		{
			/* the surface can't be managed by cairo, creating image surface over surface buffer */
			if (AV_OK != (rc = wrap->lock(wrap, AV_SURFACE_LOCK_READ, &(self->pixels), &(self->pitch))))
				return rc;

			cairo_surface = cairo_image_surface_create_for_data((unsigned char*)self->pixels, CAIRO_FORMAT_ARGB32, width, height, self->pitch);
			wrap->unlock(wrap);
			if (AV_NULL == cairo_surface)
			{
				return AV_EMEM;
			}
		}
	}

	if (O_context(self))
	{
		/* destroys the previously created surface */
		cairo_surface_t* oldsurface = (cairo_surface_t*)O_context(self);
		cairo_surface_destroy(oldsurface);
	}

	O_set_attr(self, CONTEXT_GRAPHICS_SURFACE, cairo_surface);

	return AV_OK;
}

static av_result_t av_graphics_surface_cairo_get_size(av_surface_p psurface, int* pwidth, int* pheight)
{
	av_graphics_surface_cairo_p self = (av_graphics_surface_cairo_p)psurface;
	cairo_surface_t* cairo_surface = (cairo_surface_t*)O_context(self);
	if (cairo_surface)
	{
		if (AV_NULL == self->wrap_surface)
		{
			*pwidth = cairo_image_surface_get_width(cairo_surface);
			*pheight = cairo_image_surface_get_height(cairo_surface);
		}
		else
		{
			return self->wrap_surface->get_size(self->wrap_surface, pwidth, pheight);
		}
	}
	else
	{
		*pwidth = *pheight = 0;
	}

	return AV_OK;
}

static av_result_t av_graphics_surface_cairo_lock(av_surface_p psurface, av_surface_lock_flags_t lockflags, av_pixel_p* ppixels, int* ppitch)
{
	av_graphics_surface_cairo_p self = (av_graphics_surface_cairo_p)psurface;
	cairo_surface_t* cairo_surface = (cairo_surface_t*)O_context(self);
	av_assert(cairo_surface, "cairo surface is not initialized properly");
	if (AV_NULL != self->wrap_surface)
	{
		return self->wrap_surface->lock(self->wrap_surface, lockflags, ppixels, ppitch);
	}
	else
	{
		*ppixels = (av_pixel_p)cairo_image_surface_get_data(cairo_surface);
		*ppitch  = cairo_image_surface_get_width(cairo_surface) << 2;
		av_assert(*ppixels && *ppitch, "cairo surface hasn't data buffer");
	}
	self->lockflags = lockflags;
	return AV_OK;
}

static av_result_t av_graphics_surface_cairo_unlock(av_surface_p psurface)
{
	av_graphics_surface_cairo_p self = (av_graphics_surface_cairo_p)psurface;
	cairo_surface_t* cairo_surface = (cairo_surface_t*)O_context(self);
	av_assert(cairo_surface, "cairo surface is not initialized properly");

	if (AV_NULL != self->wrap_surface)
	{
		self->wrap_surface->unlock(self->wrap_surface);
	}

	if (AV_SURFACE_LOCK_WRITE == self->lockflags)
	{
		/* if the surface has been locked for writing then mark it dirty */
		cairo_surface_mark_dirty(cairo_surface);
	}

	self->lockflags = AV_SURFACE_LOCK_NONE;
	return AV_OK;
}

static av_result_t av_graphics_surface_cairo_create_pattern(av_graphics_surface_p self,
															av_graphics_pattern_p *ppattern)
{
	av_result_t rc;
	av_graphics_pattern_p pattern;
	cairo_surface_t* cairo_surface = (cairo_surface_t*)O_context(self);
    cairo_pattern_t *p = cairo_pattern_create_for_surface(cairo_surface);
	av_oop_p oop = O_oop(self);
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

static void av_graphics_surface_cairo_destructor(void* psurface)
{
	av_graphics_surface_cairo_p self = (av_graphics_surface_cairo_p)psurface;
	cairo_surface_t* cairo_surface = (cairo_surface_t*)O_context(self);

	if (AV_NULL != self->wrap_surface)
	{
		self->wrap_surface->unlock(self->wrap_surface);
	}

	if (cairo_surface)
	{
		cairo_surface_destroy(cairo_surface);
	}
}

static av_result_t av_graphics_surface_cairo_constructor(av_object_p object)
{
	av_graphics_surface_cairo_p self = (av_graphics_surface_cairo_p)object;

	/* Cairo surface is not created until set_size method invoked */
	((av_graphics_surface_p)self)->graphics = AV_NULL;
	self->wrap_surface = AV_NULL;
	self->lockflags    = AV_SURFACE_LOCK_NONE;
	self->pixels       = AV_NULL;
	self->pitch        = 0;

	/* registers surface methods ... */
	((av_surface_p)self)->set_size   = av_graphics_surface_cairo_set_size;
	((av_surface_p)self)->get_size   = av_graphics_surface_cairo_get_size;
	((av_surface_p)self)->lock       = av_graphics_surface_cairo_lock;
	((av_surface_p)self)->unlock     = av_graphics_surface_cairo_unlock;
	((av_graphics_surface_p)self)->create_pattern = av_graphics_surface_cairo_create_pattern;
	return AV_OK;
}

/* Registers Cairo video surface class into OOP class repository */
av_result_t av_graphics_surface_cairo_register_oop(av_oop_p oop)
{
	return oop->define_class(oop, "graphics_surface_cairo", "graphics_surface",
								  sizeof(av_graphics_surface_cairo_t),
								  av_graphics_surface_cairo_constructor,
								  av_graphics_surface_cairo_destructor);
}

#endif /* WITH_GRAPHICS_CAIRO */
