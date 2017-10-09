/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_system.c                                        */
/* Description:   Abstract system class                              */
/*                                                                   */
/*********************************************************************/

#include <avgl.h>
#include <av_debug.h>

av_result_t av_window_set_rect(av_window_p self, av_rect_p newrect);

typedef struct _system_ctx_t
{
	/*! Root visible */
	av_visible_p root;

	/*! List of invalid rects */
	av_list_p invalid_rects;

	/*! Hovered windows */
	av_list_p hover_windows;

	/*! Captured window */
	av_window_p capture;

	/*! Focus window */
	av_window_p focus;
} system_ctx_t, *system_ctx_p;

static const char* context = "system_ctx_p";
#define O_context(o) (system_ctx_p)O_attr(o, context)

typedef struct _hover_info_t
{
	av_window_p window;
	unsigned long hover_time;
	int mouse_x;
	int mouse_y;
	av_bool_t hovered;
} hover_info_t, *hover_info_p;

static void av_visible_on_invalidate(av_window_p window, av_rect_p rect)
{
}

static av_result_t av_visible_set_rect(struct av_window* pwindow, av_rect_p rect)
{
	av_result_t rc;
	av_window_p window = (av_window_p)pwindow;
	av_visible_p self = (av_visible_p)window;
	system_ctx_p system_ctx = O_context(self->system);
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


	system_ctx->invalid_rects->push_last(system_ctx->invalid_rects, av_rect_clone(&new_rect));
	if (AV_OK == av_rect_substract(&old_rect, &new_rect, &inv_list))
	{
		system_ctx->invalid_rects->push_all(system_ctx->invalid_rects, inv_list);
	}

	return AV_OK;
}

static void av_visible_on_draw(struct _av_visible_t* self, av_graphics_p graphics)
{
	av_rect_t rect;
	system_ctx_p system_ctx = O_context(self->system);
	av_display_config_t display_config;
	((av_system_p)self->system)->display->get_configuration(((av_system_p)self->system)->display, &display_config);

	((av_window_p)self)->get_rect((av_window_p)self, &rect);
	rect.x = rect.y = 0;
	graphics->set_color_rgba(graphics, 0, 0, 0, 1);
	graphics->rectangle(graphics, &rect);
	graphics->fill(graphics, AV_FALSE);
	if (system_ctx->root == self && (display_config.scale_x != 1 || display_config.scale_y != 1))
	{
		// draw grid
		int x, y;
		graphics->set_color_rgba(graphics, 1, 1, 1, 1);
		for (x = 0; x < display_config.width; x++)
		{
			graphics->move_to(graphics, x, 0);
			graphics->line_to(graphics, x, display_config.height);
		}
		for (y = 0; y < display_config.height; y++)
		{
			graphics->move_to(graphics, 0, y);
			graphics->line_to(graphics, display_config.width, y);
		}
		graphics->set_line_width(graphics, 1);
		graphics->stroke(graphics, AV_FALSE);
	}
}

static void av_visible_set_surface(struct _av_visible_t* visible, av_surface_p surface)
{
	av_visible_p self = (av_visible_p)visible;
	av_dbg("av_visible_set_surface: %p -> %p\n", self, surface);
	self->surface = surface;
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

static void av_visible_destructor(struct _av_object_t* pvisible)
{
	av_visible_p self = (av_visible_p)pvisible;
	system_ctx_p system_ctx = O_context(self->system);

	if (self == system_ctx->root)
		system_ctx->root = AV_NULL;

	if (self->surface)
		O_destroy(self->surface);
}

static av_result_t av_visible_constructor(av_object_p object)
{
	((av_window_p)object)->set_rect = av_visible_set_rect;
	return AV_OK;
}

static void render_recurse(av_system_p self, av_visible_p visible)
{
	av_window_p window = (av_window_p)visible;
	av_rect_t dst_rect;
	av_rect_t src_rect;
	av_list_p children;
	av_display_config_t display_config;
	system_ctx_p ctx = O_context(self);
	av_list_p invrects = ctx->invalid_rects;

//	window->get_rect(window, &dst_rect);
//	src_rect = dst_rect;
//	src_rect.x = src_rect.y = 0;
	self->display->get_configuration(self->display, &display_config);

	for (invrects->first(invrects); invrects->has_more(invrects); invrects->next(invrects))
	{
		av_rect_p rect = invrects->get(invrects);
		av_rect_t winrect;
		av_rect_t irect;
		window->get_absolute_rect(window, &winrect);
		if (av_rect_intersect(rect, &winrect, &irect))
		{
			av_dbg("inv = %d %d %d %d, x=%d, y=%d\n", irect.x, irect.y, irect.w, irect.h, winrect.x, winrect.y);
			src_rect = irect;
			src_rect.x -= winrect.x;
			src_rect.y -= winrect.y;

			av_rect_scale(&src_rect, display_config.scale_x, display_config.scale_y);
			av_rect_scale(&irect, display_config.scale_x, display_config.scale_y);
			av_dbg("render_scaled %p: %d %d %d %d -> %d %d %d %d\n",
					visible->surface,
					src_rect.x, src_rect.y, src_rect.w, src_rect.h,
					irect.x, irect.y, irect.w, irect.h);

			visible->surface->render(visible->surface, &src_rect, &irect);
		}
	}
	children = window->get_children(window);
	for (children->first(children); children->has_more(children); children->next(children))
	{
		render_recurse(self, (av_visible_p)children->get(children));
	}
}

static av_bool_t bubble_event(av_window_p window, av_event_p event)
{
	/* bubbling event */
	while (window)
	{
		/* if event catch is final, then stop bubbling */
		if ((window->is_handle_events(window) && window->on_event && window->on_event(window, event)) ||
		   (!window->is_bubble_events(window))) return AV_TRUE;
		window = window->get_parent(window);
	}
	return AV_FALSE;
}

static av_window_p find_window_xy(av_window_p window, int x, int y)
{
	av_window_p result = AV_NULL;
	if (window->is_visible(window))
	{
		av_list_p children = window->get_children(window);
		if (window->point_inside(window, x, y))
			result = window;

		for (children->first(children); children->has_more(children); children->next(children))
		{
			av_window_p child = (av_window_p)children->get(children);
			if ((child = find_window_xy(child, x, y)))
			{
				result = child;
 			}
		}
	}
	return result;
}

static av_result_t add_hover_info(av_system_p system, av_window_p window, int x, int y, av_bool_t* is_added)
{
	av_result_t rc;
	hover_info_p hover_info;
	system_ctx_p ctx = O_context(system);

	if (!window->is_handle_events(window))
		return AV_FALSE;

	for (ctx->hover_windows->first(ctx->hover_windows);
		ctx->hover_windows->has_more(ctx->hover_windows);
		ctx->hover_windows->next(ctx->hover_windows))
	{
		hover_info = (hover_info_p)ctx->hover_windows->get(ctx->hover_windows);
		if (hover_info->window == window)
		{
			/* already added, do nothing */
			*is_added = AV_FALSE;
			return AV_OK;
		}
	}

	hover_info = (hover_info_p)av_malloc(sizeof(hover_info_t));
	if (!hover_info)
		return AV_EMEM;

	hover_info->window = window;
	hover_info->hover_time = system->timer->now();
	hover_info->mouse_x = x;
	hover_info->mouse_y = y;
	hover_info->hovered = AV_FALSE;
	if (AV_OK != (rc = ctx->hover_windows->push_last(ctx->hover_windows, hover_info)))
	{
		av_free(hover_info);
		return rc;
	}
	*is_added = AV_TRUE;
	return AV_OK;
}

static av_bool_t av_system_inject_event(av_system_p self, av_event_p event)
{
}

static av_bool_t av_system_step(av_system_p self)
{
	unsigned long now;
	av_event_t event;
	system_ctx_p ctx = O_context(self);
	av_list_p invrects = ctx->invalid_rects;

	now = self->timer->now();
	for (ctx->hover_windows->first(ctx->hover_windows);
		ctx->hover_windows->has_more(ctx->hover_windows);
		ctx->hover_windows->next(ctx->hover_windows))
	{
		hover_info_p hover_info = (hover_info_p)ctx->hover_windows->get(ctx->hover_windows);
		if (!hover_info->hovered && (now - hover_info->hover_time > hover_info->window->hover_delay))
		{
			if (hover_info->window->is_visible(hover_info->window))
			{
				event.type = AV_EVENT_MOUSE_HOVER;
				event.mouse_x = hover_info->mouse_x;
				event.mouse_y = hover_info->mouse_y;
				event.window = hover_info->window;
				bubble_event(hover_info->window, &event);
			}
			hover_info->hovered = AV_TRUE;
		}
	}

	if (!self->input->poll_event(self->input, &event))
	{
		self->timer->sleep_ms(10);
	}
	else
	{
		event.mouse_x /= self->display->display_config.scale_x;
		event.mouse_y /= self->display->display_config.scale_y;
		av_window_p target = AV_NULL;
		switch (event.type)
		{
			case AV_EVENT_MOUSE_BUTTON:
				av_assert(AV_MASK_ENABLED(event.flags, AV_EVENT_FLAGS_MOUSE_BUTTON), "invalid mouse event");
			case AV_EVENT_MOUSE_MOTION:
				av_assert(AV_MASK_ENABLED(event.flags, AV_EVENT_FLAGS_MOUSE_XY), "invalid mouse event");
			{
				if (ctx->capture)
					target = ctx->capture;
				else
				if (ctx->root)
				{
					av_event_type_t event_type = event.type;
					av_window_p hovered = AV_NULL;
					target = find_window_xy((av_window_p)ctx->root, event.mouse_x, event.mouse_y);
					/*if (event.type == AV_EVENT_MOUSE_MOTION)*/
					{
						if (target)
						{
							if (self->display->is_cursor_visible(self->display))
							{
								if (!target->cursor_visible)
									self->display->set_cursor_visible(self->display, AV_FALSE);
							}
							else
							{
								if (target->cursor_visible)
								{
									self->display->set_cursor_visible(self->display, AV_TRUE);
									self->display->set_mouse_position(self->display, event.mouse_x, event.mouse_y);
								}
							}
							self->display->set_cursor_shape(self->display, target->cursor);
						}
						for (ctx->hover_windows->first(ctx->hover_windows);
							ctx->hover_windows->has_more(ctx->hover_windows);
							ctx->hover_windows->next(ctx->hover_windows))
						{
							hover_info_p hover_info = (hover_info_p)ctx->hover_windows->get(ctx->hover_windows);
							if (hover_info->window == target)
								hovered = target;
							else
							if (!hover_info->window->point_inside(hover_info->window, event.mouse_x, event.mouse_y))
							{
								event.type = AV_EVENT_MOUSE_LEAVE;
								bubble_event(hover_info->window, &event);
								av_free(hover_info);
								ctx->hover_windows->remove(ctx->hover_windows);
							}
						}
						if (!hovered)
						{
							av_window_p target_parent = target;
							while (target_parent)
							{
								av_bool_t is_added;
								av_result_t rc;
								if (AV_OK != (rc = add_hover_info(self, target_parent, event.mouse_x, event.mouse_y, &is_added)))
								{
									// FIXME: log error
									return AV_FALSE;
								}
								else
								{
									if (is_added)
									{
										event.type = AV_EVENT_MOUSE_ENTER;
										bubble_event(target_parent, &event);
									}
								}
								target_parent = target_parent->get_parent(target_parent);
							}
						}
						/*target = AV_NULL; */ /* skip event bubble */
						event.type = event_type;
					}
				}
			}
			break;
			case AV_EVENT_KEYBOARD:
			{
				target = ctx->focus;
			}
			break;
//			case AV_EVENT_UPDATE:
//				if (event.flags != AV_EVENT_FLAGS_NONE)
//				{
//					/* FIXME: use event.data pointer or not */
//				}
//				self->update(self);
//				return AV_TRUE;
//			break;
//			case AV_EVENT_USER:
//				target = (av_window_p)event.window;
//			break;
			default:
				break;
		}

		if (target/* && (!self->modal || self->modal == target ||
					   target->is_parent(target, self->modal))*/)
		{
			event.window = target;
			bubble_event(target, &event);
		}
	}

	//	av_event_dbg(&event);


	// FIXME: Render only invalidated rects
	if (ctx->root)
		render_recurse(self, ctx->root);

	for (invrects->first(invrects); invrects->has_more(invrects); invrects->next(invrects))
	{
		av_rect_p rect = invrects->get(invrects);
		av_dbg("invrect = %d %d %d %d\n", rect->x, rect->y, rect->w, rect->h);
	}
	invrects->remove_all(invrects, av_free);

	self->display->render(self->display);
	return AV_EVENT_QUIT != event.type;
}

static void av_system_loop(av_system_p self)
{
	while (self->step(self))
		self->timer->sleep_ms(10);
}

static av_result_t av_system_create_visible(av_system_p self, av_visible_p parent, av_visible_p *pvisible)
{
	system_ctx_p ctx = O_context(self);
	av_visible_p visible;
	av_oop_p oop = ((av_object_p)self)->classref->oop;
	av_result_t rc;

	if (AV_OK != (rc = oop->new(oop, "visible", (av_object_p*)&visible)))
		return rc;

	visible->draw = av_visible_draw;
	visible->on_draw = av_visible_on_draw;
	visible->set_surface = av_visible_set_surface;
	visible->system = (struct av_system_t*)self;
	((av_window_p)visible)->on_invalidate = av_visible_on_invalidate;

	if (!ctx->root)
		ctx->root = visible;
	else if (!parent)
		parent = ctx->root;

	if (AV_OK != (rc = ((av_window_p)visible)->set_parent((av_window_p)visible, (av_window_p)parent)))
	{
		O_destroy(visible);
		return rc;
	}

	*pvisible = visible;
	return AV_OK;
}

static av_result_t av_system_create_visible_from_rect(av_system_p self, av_visible_p parent, av_rect_p rect, av_visible_p *pvisible)
{
	av_rect_t arect;
	av_display_config_t display_config;
	av_visible_p visible;
	av_result_t rc;

	self->display->get_configuration(self->display, &display_config);

	if (rect)
		av_rect_copy(&arect, rect);
	else
		av_rect_init(&arect, AV_DEFAULT, AV_DEFAULT, AV_DEFAULT, AV_DEFAULT);

	if (AV_OK != (rc = av_system_create_visible(self, parent, &visible)))
	{
		return rc;
	}

	if (AV_OK != (rc = self->display->create_surface(self->display, &visible->surface)))
	{
		O_destroy(visible);
		return rc;
	}
	((av_window_p)visible)->set_rect((av_window_p)visible, &arect);
	if (AV_OK != (rc = visible->surface->set_size(visible->surface, arect.w * display_config.scale_x, arect.h * display_config.scale_y)))
	{
		O_destroy(visible);
		return rc;
	}

	if (pvisible)
		*pvisible = visible;
	return AV_OK;
}


static av_result_t av_system_create_visible_from_surface(av_system_p self, av_visible_p parent, int x, int y, av_surface_p surface, av_visible_p *pvisible)
{
	av_rect_t rect;
	av_visible_p visible;
	av_result_t rc;
	surface->get_size(surface, &rect.w, &rect.h);
	rect.x = x;
	rect.y = y;

	if (AV_OK != (rc = av_system_create_visible(self, parent, &visible)))
	{
		return rc;
	}
	if (AV_OK != (rc = ((av_window_p)visible)->set_rect((av_window_p)visible, &rect)))
	{
		O_destroy(visible);
		return rc;
	}
	visible->surface = surface;
	if (pvisible)
		*pvisible = visible;
	return AV_OK;
}

static av_result_t av_system_create_bitmap(av_system_p self, av_bitmap_p* pbitmap)
{
	AV_UNUSED(self);
	AV_UNUSED(pbitmap);
	return AV_ESUPPORTED;
}

static void av_system_set_capture(av_system_p self, av_window_p window)
{
	system_ctx_p ctx = O_context(self);
	ctx->capture = window;
}

static void av_system_destructor(void* psystem)
{
	av_system_p self = (av_system_p)psystem;
	system_ctx_p ctx = O_context(self);

	ctx->invalid_rects->remove_all(ctx->invalid_rects, av_free);
	ctx->invalid_rects->destroy(ctx->invalid_rects);
	ctx->hover_windows->remove_all(ctx->hover_windows, av_free);
	ctx->hover_windows->destroy(ctx->hover_windows);

	if (ctx->root)
		O_destroy(ctx->root);

	if (self->audio) O_release(self->audio);
	if (self->timer) O_release(self->timer);
	if (self->input) O_release(self->input);
	if (self->display) O_release(self->display);
	if (self->graphics) O_release(self->graphics);
	free(ctx);
}

static av_result_t av_system_constructor(av_object_p object)
{
	av_result_t rc;
	av_oop_p oop;
	system_ctx_p ctx;
	av_system_p self        = (av_system_p)object;

	ctx = (system_ctx_p)av_calloc(1, sizeof(system_ctx_t));
	if (!ctx) return AV_EMEM;
	O_set_attr(self, context, ctx);

	if (AV_OK != (rc = av_list_create(&ctx->invalid_rects)))
		return rc;


	if (AV_OK != (rc = av_list_create(&ctx->hover_windows)))
		return rc;

	self->audio             = AV_NULL; // FIXME: 

	oop = object->classref->oop;
	
	if (AV_OK != (rc = oop->service_ref(oop, "display", (av_service_p *)&self->display)))
		return rc;

	if (AV_OK != (rc = oop->service_ref(oop, "graphics", (av_service_p *)&self->graphics)))
		return rc;

	if (AV_OK != (rc = oop->service_ref(oop, "input", (av_service_p *)&self->input)))
		return rc;

	if (AV_OK != (rc = oop->service_ref(oop, "timer", (av_service_p *)&self->timer)))
		return rc;

	self->step              = av_system_step;
	self->loop              = av_system_loop;
	self->create_visible    = av_system_create_visible_from_rect;
	self->create_visible_from_surface = av_system_create_visible_from_surface;
	self->create_bitmap     = av_system_create_bitmap;
	self->set_capture       = av_system_set_capture;
	return AV_OK;
}

/* Registers system class into oop */
av_result_t av_system_register_oop(av_oop_p oop)
{
	av_result_t rc;

	av_window_register_oop(oop);

	if (AV_OK != (rc = oop->define_class(oop, "visible", "window",
										 sizeof(av_visible_t),
										 av_visible_constructor,
										 av_visible_destructor)))
		return rc;

	if (AV_OK != (rc = av_display_register_oop(oop)))
		return rc;

	if (AV_OK != (rc = av_graphics_register_oop(oop)))
		return rc;

	if (AV_OK != (rc = av_input_register_oop(oop)))
		return rc;

	if (AV_OK != (rc = av_timer_register_oop(oop)))
		return rc;

	if (AV_OK != (rc = av_bitmap_register_oop(oop)))
		return rc;


	/*
	if (AV_OK != (rc = av_sound_register_oop(oop)))
		return rc;
	if (AV_OK != (rc = av_audio_register_oop(oop)))
		return rc;
	*/
	return oop->define_class(oop, "system", "service",
								  sizeof(av_system_t),
								  av_system_constructor, av_system_destructor);
}
