/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_sprite.c                                        */
/*                                                                   */
/*********************************************************************/ 

#include <av_sprite.h>
#include <av_stdc.h>
#include <av_debug.h>

typedef struct _sprite_ctx_t
{
	int frame_width;
	int frame_height;
	av_bool_t is_loop;
	int* sequence;
	int current_frame;
	int sequence_count;
	unsigned long duration;
	unsigned long seq_start_time;
} sprite_ctx_t, *sprite_ctx_p;

static const char* context_name = "sprite_ctx_p";
#define O_context(o) (sprite_ctx_p)O_attr(o, context_name)

void av_sprite_set_size(struct _av_sprite_t* _self, int frame_width, int frame_height)
{
	sprite_ctx_p ctx = O_context(_self);
	av_rect_t rect;
	ctx->frame_width = frame_width;
	ctx->frame_height = frame_height;

	((av_window_p)_self)->get_rect((av_window_p)_self, &rect);
	rect.w = frame_width;
	rect.h = frame_height;
	((av_window_p)_self)->set_rect((av_window_p)_self, &rect);
}

void av_sprite_get_size(struct _av_sprite_t* _self, int* frame_width, int* frame_height)
{
	sprite_ctx_p ctx = O_context(_self);
	*frame_width = ctx->frame_width;
	*frame_height = ctx->frame_height;
}

int av_sprite_get_frames_count(struct _av_sprite_t* _self)
{
	int swidth, sheight;
	sprite_ctx_p ctx = O_context(_self);
	av_surface_p surface = ((av_visible_p)_self)->surface;
	if (!surface)
		return 0;
	if (!ctx->frame_width || !ctx->frame_height)
		return 0;
	if (AV_OK != surface->get_size(surface, &swidth, &sheight))
		return 0;

	return (swidth / ctx->frame_width) * (sheight / ctx->frame_height);
}

void av_sprite_set_current_frame(struct _av_sprite_t* _self, int frame)
{
	sprite_ctx_p ctx = O_context(_self);
	ctx->current_frame = frame;
	((av_window_p)_self)->invalidate((av_window_p)_self);

	// av_dbg("av_sprite_set_current_frame: frame = %d\n", frame);
}

int av_sprite_get_current_frame(struct _av_sprite_t* _self)
{
	sprite_ctx_p ctx = O_context(_self);
	return ctx->current_frame;
}

void av_sprite_set_sequence(struct _av_sprite_t* _self, int* sequence, int count, unsigned long duration, av_bool_t is_loop)
{
	sprite_ctx_p ctx = O_context(_self);
	if (ctx->sequence)
		av_free(ctx->sequence);
	ctx->is_loop = is_loop;
	ctx->sequence = (int*)av_malloc(sizeof(int) * count);
	av_memcpy((unsigned char*)ctx->sequence, (unsigned char*)sequence, sizeof(int) * count);
	((av_sprite_p)_self)->set_current_frame((av_sprite_p)_self, 0);
	ctx->sequence_count = count;
	ctx->duration = duration;
	ctx->seq_start_time = ((av_visible_p)_self)->system->timer->now();
}

static void av_sprite_render(struct _av_visible_t* _self, av_rect_p src_rect, av_rect_p dst_rect)
{
	sprite_ctx_p ctx = O_context(_self);
	av_sprite_p self = (av_sprite_p)_self;
	av_visible_p visible = (av_visible_p)_self;
	int swidth, sheight;
	int frame = 0;
	int ofs_x, ofs_y;
	int row, col;
	av_rect_t frame_rect;
	int frames_per_row;
	int nframes = self->get_frames_count(self);
	if (nframes == 0)
		return;
	if (ctx->sequence_count > 0)
		frame = ctx->sequence[ctx->current_frame];
	if (AV_OK != visible->surface->get_size(visible->surface, &swidth, &sheight))
		return;
	frames_per_row = swidth / ctx->frame_width;
	row = frame / frames_per_row;
	col = frame % frames_per_row;
	ofs_x = ctx->frame_width * col;
	ofs_y = ctx->frame_height * row;
	av_rect_copy(&frame_rect, src_rect);
	frame_rect.x += ofs_x;
	frame_rect.y += ofs_y;
	visible->surface->render(visible->surface, &frame_rect, dst_rect);
}

static void av_sprite_on_tick(struct _av_visible_t* _self)
{
	sprite_ctx_p ctx = O_context(_self);
	unsigned long now = ((av_visible_p)_self)->system->timer->now();
	unsigned long passed = AV_MIN(ctx->duration, now - ctx->seq_start_time);
	int next_frame = (int)(ctx->sequence_count * (float)passed / ctx->duration);
	if (next_frame < ctx->sequence_count)
	{
		if (ctx->current_frame != next_frame)
			((av_sprite_p)_self)->set_current_frame((av_sprite_p)_self, next_frame);
	}
	else
	{
		if (ctx->is_loop)
		{
			ctx->seq_start_time = now;
		}
	}
}

static void av_sprite_destructor(void* _self)
{
	sprite_ctx_p ctx = O_context(_self);
	if (ctx->sequence) av_free(ctx->sequence);
	av_free(ctx);
}

/* constructor */
static av_result_t av_sprite_constructor(av_object_p pobject)
{
	av_sprite_p self = (av_sprite_p)pobject;
	sprite_ctx_p ctx = (sprite_ctx_p)av_calloc(1, sizeof(sprite_ctx_t));
	if (!ctx) return AV_EMEM;
	O_set_attr(self, context_name, ctx);

	((av_visible_p)self)->render = av_sprite_render;
	((av_visible_p)self)->on_tick = av_sprite_on_tick;
	self->set_size          = av_sprite_set_size;
	self->get_size          = av_sprite_get_size;
	self->get_frames_count  = av_sprite_get_frames_count;
	self->set_current_frame = av_sprite_set_current_frame;
	self->get_current_frame = av_sprite_get_current_frame;
	self->set_sequence = av_sprite_set_sequence;
	return AV_OK;
}

av_result_t av_sprite_register_oop(av_oop_p oop)
{
	return oop->define_class(oop, "sprite", "visible", sizeof(av_sprite_t), av_sprite_constructor, av_sprite_destructor);
}
