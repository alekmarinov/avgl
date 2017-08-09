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

typedef struct _system_ctx_t
{
	/*! Root visible */
	av_visible_p root;

	/*! List of invalid rects */
	av_list_p invalid_rects;
} system_ctx_t, *system_ctx_p;

static const char* context = "system_ctx_p";
#define O_context(o) (system_ctx_p)O_attr(o, context)

static void av_visible_on_invalidate(av_window_p visible, av_rect_p rect)
{
	system_ctx_p ctx = O_context(visible);
	av_rect_p prect;

	if (rect->w && rect->h)
	{
		av_rect_t crect;
		((av_window_p)ctx->root)->get_rect((av_window_p)ctx->root, &crect);
		/*av_rect_copy(&crect, rect);*/
		if (av_rect_intersect(rect, &crect, &crect))
		{
			for (ctx->invalid_rects->first(ctx->invalid_rects);
				ctx->invalid_rects->has_more(ctx->invalid_rects);
				ctx->invalid_rects->next(ctx->invalid_rects))
			{
				av_rect_t dummy_rect;
				prect = (av_rect_p)ctx->invalid_rects->get(ctx->invalid_rects);
				if (av_rect_intersect(&crect, prect, &dummy_rect))
				{
					av_rect_extend(prect, &crect);
					return;
				}
			}
			prect = (av_rect_p)av_malloc(sizeof(av_rect_t));
			av_rect_copy(prect, rect);
			ctx->invalid_rects->push_last(ctx->invalid_rects, prect);
		}
	}
}

static void av_visible_destructor(void* pvisible)
{
	av_visible_p self = (av_visible_p)pvisible;
	system_ctx_p system_ctx = O_context(self->system);

	if (self == system_ctx->root)
		system_ctx->root = AV_NULL;
}

static av_result_t av_visible_constructor(av_object_p object)
{
	av_visible_p self = (av_visible_p)object;
	av_window_p window = (av_window_p)object;

	return AV_OK;
}

static av_bool_t av_system_step(av_system_p self)
{
	av_event_t event;
	system_ctx_p ctx = O_context(self);

	if (self->input->poll_event(self->input, &event))
	{
		return AV_EVENT_QUIT != event.type;
	}
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

	visible->system = (struct av_system_t*)self;
	((av_window_p)visible)->on_invalidate = av_visible_on_invalidate;

	if (!ctx->root)
		ctx->root = visible;

	if (!parent)
		parent = ctx->root;

	if (AV_OK != (rc = ((av_window_p)visible)->set_parent((av_window_p)visible, (av_window_p)parent)))
	{
		O_destroy(visible);
		return rc;
	}
	((av_window_p)visible)->set_rect((av_window_p)visible, &arect);
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
