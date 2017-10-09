/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_display.c                                        */
/* Description:   Abstract display class                              */
/*                                                                   */
/*********************************************************************/

#include <av_display.h>

static av_result_t av_display_enum_display_modes(av_display_p self, display_mode_callback_t vmcbk)
{
	AV_UNUSED(self);
	AV_UNUSED(vmcbk);
	return AV_ESUPPORTED;
}

static av_result_t av_display_set_configuration(av_display_p self, av_display_config_p display_config)
{
	AV_UNUSED(self);
	AV_UNUSED(display_config);

	return AV_ESUPPORTED;
}

static av_result_t av_display_get_configuration(av_display_p self, av_display_config_p display_config)
{
	AV_UNUSED(self);
	AV_UNUSED(display_config);
	return AV_ESUPPORTED;
}

/*
static av_result_t av_display_create_surface(av_display_p self, int width, int height, av_display_surface_p* ppdisplaysurface)
{
	AV_UNUSED(self);
	AV_UNUSED(width);
	AV_UNUSED(height);
	AV_UNUSED(ppdisplaysurface);
	return AV_ESUPPORTED;
}

static av_result_t av_display_create_surface_from(av_display_p self, void* ptr, int width, int height,
												int pitch, av_display_surface_p* ppdisplaysurface)
{
	AV_UNUSED(self);
	AV_UNUSED(ptr);
	AV_UNUSED(width);
	AV_UNUSED(height);
	AV_UNUSED(pitch);
	AV_UNUSED(ppdisplaysurface);
	return AV_ESUPPORTED;
}
*/

static void av_display_set_capture(av_display_p self, av_bool_t iscaptured)
{
	AV_UNUSED(self);
	AV_UNUSED(iscaptured);
}

static void av_display_set_cursor_visible(av_display_p self, av_bool_t visible)
{
	AV_UNUSED(self);
	AV_UNUSED(visible);
}

static av_result_t av_display_set_cursor_shape(av_display_p self, av_display_cursor_shape_t shape)
{
	AV_UNUSED(self);
	AV_UNUSED(shape);
	return AV_ESUPPORTED;
}

static void av_display_set_mouse_position(av_display_p self, int mx, int my)
{
	AV_UNUSED(self);
	AV_UNUSED(mx);
	AV_UNUSED(my);
}

static void av_display_get_mouse_position(av_display_p self, int* pmx, int* pmy)
{
	AV_UNUSED(self);
	AV_UNUSED(pmx);
	AV_UNUSED(pmy);
}

static av_result_t av_display_create_bitmap(struct av_display* self, av_bitmap_p* bitmap)
{
	av_oop_p oop = O_oop(self);
	return oop->new(oop, "bitmap", (av_object_p*)bitmap);
}

static av_result_t av_display_create_surface(struct av_display* self, av_surface_p* surface)
{
	av_oop_p oop = O_oop(self);
	return oop->new(oop, "surface", (av_object_p*)surface);
}

static void av_display_render(struct av_display* self)
{
	AV_UNUSED(self);
}

/* Initializes memory given by the input pointer with the display's class information */
static av_result_t av_display_constructor(av_object_p object)
{
	av_display_p self           = (av_display_p)object;
	self->display_config.scale_x = self->display_config.scale_y = 1;
	self->enum_display_modes    = av_display_enum_display_modes;
	self->set_configuration     = av_display_set_configuration;
	self->get_configuration     = av_display_get_configuration;
	self->create_bitmap         = av_display_create_bitmap;
	self->create_surface        = av_display_create_surface;
	self->set_capture           = av_display_set_capture;
	self->set_cursor_visible    = av_display_set_cursor_visible;
	self->set_cursor_shape      = av_display_set_cursor_shape;
	self->set_mouse_position    = av_display_set_mouse_position;
	self->get_mouse_position    = av_display_get_mouse_position;
	self->render                = av_display_render;
	return AV_OK;
}

/* Registers display class into TORBA class repository */
av_result_t av_display_register_oop(av_oop_p oop)
{
	av_result_t rc;

	if (AV_OK != (rc = oop->define_class(oop, "display", "service", sizeof(av_display_t),
											  av_display_constructor, AV_NULL)))
		return rc;
	return AV_OK;
}
