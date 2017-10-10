/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_visible.c                                       */
/* Description:   Base visible class                                 */
/*                                                                   */
/*********************************************************************/

#include <avgl.h>
#include <av_debug.h>

av_result_t av_window_set_rect(av_window_p self, av_rect_p newrect);
av_result_t av_system_create_visible(av_system_p system, av_visible_p parent, av_rect_p rect, on_draw_t drawer, av_surface_p surface, av_visible_p *pvisible);

static av_result_t av_visible_set_rect(struct av_window* pwindow, av_rect_p rect)
{
	av_result_t rc;
	av_window_p window = (av_window_p)pwindow;
	av_visible_p self = (av_visible_p)window;
	av_system_p system = (av_system_p)self->system;
	av_rect_t old_rect;
	av_rect_t new_rect;
	av_list_p inv_list;

	window->get_absolute_rect(window, &old_rect);
	av_window_set_rect(window, rect);
	window->get_absolute_rect(window, &new_rect);

	if (av_rect_compare(&old_rect, &new_rect))
		return AV_OK;

	if (self->surface && (old_rect.w != rect->w || old_rect.h != rect->h))
	{
		av_system_p system = (av_system_p)self->system;
		int sx = system->display->display_config.scale_x;
		int sy = system->display->display_config.scale_y;
		if (AV_OK != (rc = self->surface->set_size(self->surface, rect->w * sx, rect->h * sy)))
			return rc;

		self->draw(self);
	}

	system->invalidate_rect(system, &new_rect);
	if (AV_OK == av_rect_substract(&old_rect, &new_rect, &inv_list))
	{
		system->invalidate_rects(system, inv_list);
	}

	return AV_OK;
}

static void av_visible_set_surface(struct _av_visible_t* visible, av_surface_p surface)
{
	av_visible_p self = (av_visible_p)visible;
	av_window_p window = (av_window_p)visible;
	av_system_p system = (av_system_p)self->system;
	av_rect_t rect;
	av_dbg("av_visible_set_surface: %p -> %p\n", self, surface);

	if (self->surface)
	{
		O_destroy(self->surface);
		self->surface = AV_NULL;
	}

	/* change visible rect according to the new surface */
	window->get_rect(window, &rect);
	surface->get_size(surface, &rect.w, &rect.h);
	window->set_rect(window, &rect);
	self->surface = surface;
	window->get_absolute_rect(window, &rect);
	system->invalidate_rect(system, &rect);
}

static av_result_t av_visible_draw(struct _av_visible_t* visible)
{
	av_graphics_surface_p graphics_surace;
	av_pixel_p pixels;
	int pitch;
	av_result_t rc;
	av_visible_p self = (av_visible_p)visible;
	av_system_p system = (av_system_p)self->system;
	av_rect_t rect;
	int sx = system->display->display_config.scale_x;
	int sy = system->display->display_config.scale_y;

	((av_window_p)self)->get_rect((av_window_p)self, &rect);
	if (AV_OK != (rc = self->surface->lock(self->surface, &pixels, &pitch)))
		return rc;

	system->graphics->create_surface_from_data(system->graphics, rect.w * sx, rect.h * sy, pixels, pitch, &graphics_surace);
	system->graphics->begin(system->graphics, graphics_surace);
	system->graphics->scale_x = sx;
	system->graphics->scale_y = sy;
	self->on_draw(self, system->graphics);
	system->graphics->end(system->graphics);
	self->surface->unlock(self->surface);
	O_destroy(graphics_surace);
	return AV_OK;
}

av_result_t  av_visible_create_child(struct _av_visible_t* _self, av_rect_p rect, on_draw_t on_draw, av_surface_p surface, struct _av_visible_t **pvisible)
{
	av_visible_p self = (av_visible_p)_self;
	return av_system_create_visible((av_system_p)self->system, self, rect, on_draw, surface, pvisible);
}

static void av_visible_destructor(struct _av_object_t* pvisible)
{
	av_visible_p self = (av_visible_p)pvisible;
	if (self->surface)
		O_destroy(self->surface);

	if (self->on_destroy)
		self->on_destroy(self);
}

static av_result_t av_visible_constructor(av_object_p object)
{
	av_visible_p self = (av_visible_p)object;
	((av_window_p)object)->set_rect = av_visible_set_rect;
	self->draw = av_visible_draw;
	self->set_surface = av_visible_set_surface;
	self->create_child = av_visible_create_child;
	return AV_OK;
}

/* Registers visible class into oop */
av_result_t av_visible_register_oop(av_oop_p oop)
{
	av_result_t rc;

	av_window_register_oop(oop);

	if (AV_OK != (rc = oop->define_class(oop, "visible", "window",
		sizeof(av_visible_t),
		av_visible_constructor,
		av_visible_destructor)))
	{
		return rc;
	}

	return AV_OK;
}
