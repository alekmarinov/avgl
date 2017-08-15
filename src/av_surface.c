/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_surface.c                                       */
/* Description:   Abstract surface class                             */
/*                                                                   */
/*********************************************************************/

#include <av_surface.h>

/* set surface width and height */
static av_result_t av_surface_set_size(av_surface_p self, int width, int height)
{
	AV_UNUSED(self);
	AV_UNUSED(width);
	AV_UNUSED(height);
	return AV_ESUPPORTED;
}

/* get surface width and height */
static av_result_t av_surface_get_size(av_surface_p self, int* pwidth, int* pheight)
{
	AV_UNUSED(self);
	AV_UNUSED(pwidth);
	AV_UNUSED(pheight);
	return AV_ESUPPORTED;
}

/* lock surface memory */
static av_result_t av_surface_lock(av_surface_p self, av_pixel_p* ppixels, int* ppitch)
{
	AV_UNUSED(self);
	AV_UNUSED(ppixels);
	AV_UNUSED(ppitch);
	return AV_ESUPPORTED;
}

/* unlock surface memory */
static av_result_t av_surface_unlock(av_surface_p self)
{
	AV_UNUSED(self);
	return AV_ESUPPORTED;
}

static av_result_t av_surface_render(av_surface_p self, av_rect_p src_rect, av_rect_p dst_rect)
{
	AV_UNUSED(self);
	AV_UNUSED(src_rect);
	AV_UNUSED(dst_rect);
	return AV_ESUPPORTED;
}

static av_result_t av_surface_set_bitmap(av_surface_p self, av_bitmap_p bitmap)
{
	AV_UNUSED(self);
	AV_UNUSED(bitmap);
	return AV_ESUPPORTED;
}

/* Initializes memory given by the input pointer with the surface's class information */
static av_result_t av_surface_constructor(av_object_p object)
{
	av_surface_p self = (av_surface_p)object;
	self->set_size    = av_surface_set_size;
	self->get_size    = av_surface_get_size;
	self->lock        = av_surface_lock;
	self->unlock      = av_surface_unlock;
	self->set_bitmap  = av_surface_set_bitmap;
	self->render      = av_surface_render;
	return AV_OK;
}

/*	Registers surface class into OOP class repository */
av_result_t av_surface_register_oop(av_oop_p oop)
{
	return oop->define_class(oop, "surface", AV_NULL, sizeof(av_surface_t), av_surface_constructor, AV_NULL);
}
