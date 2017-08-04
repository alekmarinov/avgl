/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_video.c                                         */
/* Description:   Abstract video class                               */
/*                                                                   */
/*********************************************************************/

#include <av_torb.h>
#include <av_video.h>
#include <av_prefs.h>

/* Blit source surface to this video surface */
static av_result_t av_video_surface_blit(av_video_surface_p dst, av_rect_p dstrect, av_surface_p src, av_rect_p srcrect)
{
	AV_UNUSED(dst);
	AV_UNUSED(dstrect);
	AV_UNUSED(src);
	AV_UNUSED(srcrect);
	return AV_ESUPPORTED;
}

static av_result_t av_video_surface_create_overlay(av_video_surface_p psurface, av_video_overlay_p* ppoverlay)
{
	AV_UNUSED(psurface);
	AV_UNUSED(ppoverlay);
	return AV_ESUPPORTED;
}

static av_result_t av_video_surface_set_size(struct av_video_surface* self, int width, int height)
{
	av_surface_p surface = (av_surface_p)self;
	return surface->set_size(surface, width, height);
}

static av_result_t av_video_surface_get_size(struct av_video_surface* self, int* pwidth, int* pheight)
{
	av_surface_p surface = (av_surface_p)self;
	return surface->get_size(surface, pwidth, pheight);
}

/** Lock surface for direct memory access */
static av_result_t av_video_surface_lock(struct av_video_surface* self, av_surface_lock_flags_t lockflags,
											av_pixel_p* ppixels, int* ppitch)
{
	av_surface_p surface = (av_surface_p)self;
	return surface->lock(surface, lockflags, ppixels, ppitch);
}

/** Unlock surface for direct memory access */
static av_result_t av_video_surface_unlock(struct av_video_surface* self)
{
	av_surface_p surface = (av_surface_p)self;
	return surface->unlock(surface);
}

static av_result_t av_video_enum_video_modes(av_video_p self, video_mode_callback_t vmcbk)
{
	AV_UNUSED(self);
	AV_UNUSED(vmcbk);
	return AV_ESUPPORTED;
}

static av_result_t av_video_set_configuration(av_video_p self, av_video_config_p video_config)
{
	av_prefs_p prefs;
	AV_UNUSED(self);
	AV_UNUSED(video_config);

	return AV_ESUPPORTED;
}

static av_result_t av_video_get_configuration(av_video_p self, av_video_config_p video_config)
{
	AV_UNUSED(self);
	AV_UNUSED(video_config);
	return AV_ESUPPORTED;
}

static av_result_t av_video_create_surface(av_video_p self, int width, int height, av_video_surface_p* ppvideosurface)
{
	AV_UNUSED(self);
	AV_UNUSED(width);
	AV_UNUSED(height);
	AV_UNUSED(ppvideosurface);
	return AV_ESUPPORTED;
}

static av_result_t av_video_create_overlay(av_video_p self, av_video_overlay_p* ppoverlay)
{
	AV_UNUSED(self);
	AV_UNUSED(ppoverlay);
	return AV_ESUPPORTED;
}

static av_result_t av_video_create_surface_from(av_video_p self, void* ptr, int width, int height,
												int pitch, av_video_surface_p* ppvideosurface)
{
	AV_UNUSED(self);
	AV_UNUSED(ptr);
	AV_UNUSED(width);
	AV_UNUSED(height);
	AV_UNUSED(pitch);
	AV_UNUSED(ppvideosurface);
	return AV_ESUPPORTED;
}

static av_result_t av_video_get_backbuffer(av_video_p self, av_video_surface_p* ppvideosurface)
{
	AV_UNUSED(self);
	AV_UNUSED(ppvideosurface);
	return AV_ESUPPORTED;
}

static av_result_t av_video_update(av_video_p self, av_rect_p rect)
{
	AV_UNUSED(self);
	AV_UNUSED(rect);
	return AV_ESUPPORTED;
}

static void av_video_set_capture(av_video_p self, av_bool_t iscaptured)
{
	AV_UNUSED(self);
	AV_UNUSED(iscaptured);
}

static void av_video_set_cursor_visible(av_video_p self, av_bool_t visible)
{
	AV_UNUSED(self);
	AV_UNUSED(visible);
}

static av_result_t av_video_set_cursor_shape(av_video_p self, av_video_cursor_shape_t shape)
{
	AV_UNUSED(self);
	AV_UNUSED(shape);
	return AV_ESUPPORTED;
}

static void av_video_set_mouse_position(av_video_p self, int mx, int my)
{
	AV_UNUSED(self);
	AV_UNUSED(mx);
	AV_UNUSED(my);
}

static void av_video_get_mouse_position(av_video_p self, int* pmx, int* pmy)
{
	AV_UNUSED(self);
	AV_UNUSED(pmx);
	AV_UNUSED(pmy);
}

static av_result_t av_video_get_root_window(struct av_video* self, void** pdisplay, void** pwindow)
{
	AV_UNUSED(self);
	AV_UNUSED(pdisplay);
	AV_UNUSED(pwindow);
	return AV_ESUPPORTED;
}

static av_result_t av_video_get_color_key(struct av_video* self, av_pixel_t* pcolorkey)
{
	AV_UNUSED(self);
	AV_UNUSED(pcolorkey);
	return AV_ESUPPORTED;
}

/* Initializes memory given by the input pointer with the video surface class information */
static av_result_t av_video_surface_constructor(av_object_p object)
{
	av_video_surface_p self = (av_video_surface_p)object;
	self->video          = AV_NULL;
	self->blit           = av_video_surface_blit;
	self->create_overlay = av_video_surface_create_overlay;
	self->set_size       = av_video_surface_set_size;
	self->get_size       = av_video_surface_get_size;
	self->lock           = av_video_surface_lock;
	self->unlock         = av_video_surface_unlock;
	return AV_OK;
}

/* Initializes memory given by the input pointer with the video's class information */
static av_result_t av_video_constructor(av_object_p object)
{
	av_video_p self           = (av_video_p)object;
	self->backbuffer          = AV_NULL;
	self->enum_video_modes    = av_video_enum_video_modes;
	self->set_configuration   = av_video_set_configuration;
	self->get_configuration   = av_video_get_configuration;
	self->create_surface      = av_video_create_surface;
	self->create_overlay      = av_video_create_overlay;
	self->create_surface_from = av_video_create_surface_from;
	self->get_backbuffer      = av_video_get_backbuffer;
	self->update              = av_video_update;
	self->set_capture         = av_video_set_capture;
	self->set_cursor_visible  = av_video_set_cursor_visible;
	self->set_cursor_shape    = av_video_set_cursor_shape;
	self->set_mouse_position  = av_video_set_mouse_position;
	self->get_mouse_position  = av_video_get_mouse_position;
	self->get_root_window     = av_video_get_root_window;
	self->get_color_key       = av_video_get_color_key;
	return AV_OK;
}

/* Registers video class into TORBA class repository */
av_result_t av_video_register_torba(void)
{
	av_result_t rc;
	if (AV_OK != (rc = av_torb_register_class("surface", AV_NULL, sizeof(av_surface_t), AV_NULL, AV_NULL )))
		return rc;
	if (AV_OK != (rc = av_torb_register_class("video_surface", "surface", sizeof(av_video_surface_t),
											  av_video_surface_constructor, AV_NULL )))
		return rc;
	if (AV_OK != (rc = av_torb_register_class("video", "video_surface", sizeof(av_video_t),
											  av_video_constructor, AV_NULL)))
		return rc;
	return AV_OK;
}
