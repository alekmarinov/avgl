/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_timer_sdl.c                                     */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_SDL

#include <avgl.h>
#include <SDL.h>

av_result_t av_timer_register_oop(av_oop_p);

#define CONTEXT "timer_sdl_ctx"
#define O_context(o) O_attr(o, CONTEXT)

/* SDL timer context */
typedef struct av_timer_ctx
{
	av_list_p timers;
	int id_gen;
} av_timer_ctx_t, *av_timer_ctx_p;

/* timer callback info */
typedef struct av_timer_info
{
	/* timer object */
	av_timer_p timer;

	/* Desired delay in milliseconds */
	unsigned long delay_ms;

	/* User specifed data passed to callback */
	void* arg;

	/* On timeout callback function */
	av_timer_callback_p timer_callback;

	/* timer id*/
	int timer_id;

	/* SDL timer handle */
	SDL_TimerID sdl_timer_id;

} av_timer_info_t, *av_timer_info_p;

static Uint32 av_timer_sdl_callback(Uint32 interval, void *arg)
{
	av_timer_info_p tinfo = (av_timer_info_p)arg;

	if (tinfo->timer_callback(tinfo->arg))
	{
		return interval;
	}
	tinfo->timer->remove_timer(tinfo->timer, tinfo->timer_id);
	return 0; /* stop timer */
}

static av_result_t av_timer_sdl_add_timer(av_timer_p self,
										  av_timer_callback_p callback,
										  unsigned long delay_ms,
										  void* arg,
										  int* ptimer_id)
{
	av_result_t rc;
	av_timer_ctx_p ctx = (av_timer_ctx_p)O_context(self);
	av_timer_info_p tinfo = (av_timer_info_p)malloc(sizeof(av_timer_info_t));
	if (!tinfo) return AV_EMEM;

	AV_UNUSED(self);

	tinfo->timer                 = self;
	tinfo->delay_ms              = delay_ms;
	tinfo->arg                   = arg;
	tinfo->timer_callback        = callback;
	tinfo->sdl_timer_id          = SDL_AddTimer(delay_ms, av_timer_sdl_callback, (void*)tinfo);
	tinfo->timer_id              = ctx->id_gen++;
	if (AV_OK != (rc = ctx->timers->push_last(ctx->timers, tinfo)))
	{
		av_free(tinfo);
		return rc;
	}

	if (ptimer_id)
		*ptimer_id = tinfo->timer_id;
	return AV_OK;
}

static av_result_t av_timer_sdl_remove_timer(av_timer_p self, int timer_id)
{
	av_timer_ctx_p ctx = (av_timer_ctx_p)O_context(self);
	for (ctx->timers->first(ctx->timers); ctx->timers->has_more(ctx->timers); ctx->timers->next(ctx->timers))
	{
		av_timer_info_p timer_info = (av_timer_info_p)ctx->timers->get(ctx->timers);
		if (timer_id == timer_info->timer_id)
		{
			SDL_RemoveTimer(timer_info->sdl_timer_id);
			av_free(timer_info);
			ctx->timers->remove(ctx->timers);
			return AV_OK;
		}
	}
	return AV_EFOUND;
}

/* Sleeps for timeout given in seconds */
static void av_timer_sdl_sleep(unsigned long s)
{
	SDL_Delay(s * 1000);
}

/* Sleeps for timeout given in milliseconds */
static void av_timer_sdl_sleep_ms(unsigned long mills)
{
	SDL_Delay(mills);
}

/* destructor */
static void av_timer_sdl_destructor(void* pobject)
{
	av_timer_ctx_p ctx = (av_timer_ctx_p)O_context(pobject);
	for (ctx->timers->first(ctx->timers); ctx->timers->has_more(ctx->timers); ctx->timers->next(ctx->timers))
	{
		av_timer_info_p timer_info = (av_timer_info_p)ctx->timers->get(ctx->timers);
		SDL_RemoveTimer(timer_info->sdl_timer_id);
		av_free(timer_info);
	}
	ctx->timers->destroy(ctx->timers);
	av_free(ctx);
}

/* Returns current time since computer started in ms */
static unsigned long av_timer_sdl_now(void)
{
	return SDL_GetTicks();
}

/* constructor */
static av_result_t av_timer_sdl_constructor(av_object_p pobject)
{
	av_result_t rc;
	av_timer_p self = (av_timer_p)pobject;
	av_timer_ctx_p ctx = (av_timer_ctx_p)malloc(sizeof(av_timer_ctx_t));
	if (!ctx)
		return AV_EMEM;

	if (AV_OK != (rc = av_list_create(&ctx->timers)))
	{
		av_free(ctx);
		return rc;
	}
	ctx->id_gen = 1;
	O_set_attr(self, CONTEXT, ctx);
	self->add_timer    = av_timer_sdl_add_timer;
	self->remove_timer = av_timer_sdl_remove_timer;
	self->sleep        = av_timer_sdl_sleep;
	self->sleep_ms     = av_timer_sdl_sleep_ms;
	self->now          = av_timer_sdl_now;

	return AV_OK;
}

/* Registers SDL timer class into TORBA class repository */
av_result_t av_timer_sdl_register_oop(av_oop_p oop)
{
	av_timer_p timer;
	av_result_t rc;
	if (AV_OK != (rc = av_timer_register_oop(oop)))
		return rc;

	if (AV_OK != (rc = oop->define_class(oop, "timer_sdl", "timer", sizeof(av_timer_t),
		av_timer_sdl_constructor, av_timer_sdl_destructor)))
		return rc;


	if (AV_OK != (rc = oop->new(oop, "timer_sdl", (av_object_p*)&timer)))
		return rc;

	return oop->register_service(oop, "timer", (av_service_p)timer);
}

#endif /* WITH_SYSTEM_SDL */
