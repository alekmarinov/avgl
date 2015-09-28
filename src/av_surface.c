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
static av_result_t av_surface_lock(av_surface_p self, av_surface_lock_flags_t lockflags,
									av_pixel_p* ppixels, int* ppitch)
{
	AV_UNUSED(self);
	AV_UNUSED(lockflags);
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

/* set clipping rectangle */
static av_result_t av_surface_set_clip(av_surface_p self, av_rect_p rect)
{
	AV_UNUSED(self);
	AV_UNUSED(rect);
	return AV_ESUPPORTED;
}

/* get clipping rectangle */
static av_result_t av_surface_get_clip(av_surface_p self, av_rect_p rect)
{
	AV_UNUSED(self);
	AV_UNUSED(rect);
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
	self->set_clip    = av_surface_set_clip;
	self->get_clip    = av_surface_get_clip;
	return AV_OK;
}

/*	Registers surface class into TORBA class repository */
av_result_t av_surface_register_torba(void)
{
	return av_torb_register_class("surface", AV_NULL, sizeof(av_surface_t), av_surface_constructor, AV_NULL);
}
