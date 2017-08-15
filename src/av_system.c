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

av_result_t av_window_set_rect(av_window_p self, av_rect_p newrect);

typedef struct _system_ctx_t
{
	/*! Root visible */
	av_visible_p root;

	/*! List of invalid rects */
	av_list_p invalid_rects;
} system_ctx_t, *system_ctx_p;

static const char* context = "system_ctx_p";
#define O_context(o) (system_ctx_p)O_attr(o, context)

static void av_visible_on_invalidate(av_window_p window, av_rect_p rect)
{
}

static av_result_t av_visible_set_rect(struct av_window* window, av_rect_p rect)
{
	av_result_t rc;
	av_visible_p self = (av_visible_p)window;
	av_window_set_rect(window, rect);

	if (AV_OK != (rc = self->surface->set_size(self->surface, rect->w, rect->h)))
		return rc;

	self->draw(self);

	return AV_OK;
}

static void av_visible_on_draw(struct _av_visible_t* self, av_graphics_p graphics)
{
	av_rect_t rect;
	((av_window_p)self)->get_rect((av_window_p)self, &rect);
	rect.x = rect.y = 0;
	graphics->set_color_rgba(graphics, 0, 0, 0, 1);
	graphics->rectangle(graphics, &rect);
	graphics->fill(graphics, AV_FALSE);
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

	((av_window_p)self)->get_rect((av_window_p)self, &rect);
	if (AV_OK != (rc = self->surface->lock(self->surface, &pixels, &pitch)))
		return rc;

	system->graphics->create_surface_from_data(system->graphics, rect.w, rect.h, pixels, pitch, &graphics_surace);
	system->graphics->begin(system->graphics, graphics_surace);
	self->on_draw(self, system->graphics);
	system->graphics->end(system->graphics);
	self->surface->unlock(self->surface);
	O_destroy(graphics_surace);
	return AV_OK;
}

static void av_visible_destructor(void* pvisible)
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
	window->get_rect(window, &dst_rect);
	src_rect = dst_rect;
	src_rect.x = src_rect.y = 0;
	visible->surface->render(visible->surface, &src_rect, &dst_rect);
	children = window->get_children(window);
	for (children->first(children); children->has_more(children); children->next(children))
	{
		render_recurse(self, (av_visible_p)children->get(children));
	}
}

static av_bool_t av_system_step(av_system_p self)
{
	av_event_t event;
	system_ctx_p ctx = O_context(self);

	if (self->input->poll_event(self->input, &event))
	{
		return AV_EVENT_QUIT != event.type;
	}
	if (ctx->root)
		render_recurse(self, ctx->root);

	self->display->render(self->display);
	return AV_TRUE;
}

static void av_system_loop(av_system_p self)
{
	while (self->step(self))
		self->timer->sleep_ms(10);
}

static av_result_t av_create_visible(av_system_p self, av_visible_p parent, av_rect_p rect, av_visible_p *pvisible)
{
	system_ctx_p ctx = O_context(self);
	av_visible_p visible;
	av_oop_p oop = ((av_object_p)self)->classref->oop;
	av_rect_t arect;
	av_result_t rc;

	if (rect)
		av_rect_copy(&arect, rect);
	else
		av_rect_init(&arect, AV_DEFAULT, AV_DEFAULT, AV_DEFAULT, AV_DEFAULT);

	if (AV_OK != (rc = oop->new(oop, "visible", (av_object_p*)&visible)))
		return rc;

	visible->draw = av_visible_draw;
	visible->on_draw = av_visible_on_draw;

	visible->system = (struct av_system_t*)self;
	if (AV_OK != (rc = self->display->create_surface(self->display, &visible->surface)))
	{
		O_destroy(visible);
		return rc;
	}
	visible->surface->set_size(visible->surface, rect->w, rect->h);
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
	((av_window_p)visible)->set_rect((av_window_p)visible, &arect);
	if (pvisible)
		*pvisible = visible;
	return AV_OK;
}

static void av_system_destructor(void* psystem)
{
	av_system_p self = (av_system_p)psystem;
	system_ctx_p ctx = O_context(self);

	ctx->invalid_rects->remove_all(ctx->invalid_rects, av_free);
	ctx->invalid_rects->destroy(ctx->invalid_rects);

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
	self->create_visible    = av_create_visible;
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
