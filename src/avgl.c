/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      avgl.c                                             */
/*                                                                   */
/*********************************************************************/

#include <avgl.h>

void av_system_sdl_register_oop(av_oop_p);

typedef struct _avgl_ctx_t
{
	/* reference to oop container */
	av_oop_p oop;

	/* reference to log service */
	av_log_p log;

	/* reference to system service */
	av_system_p system;

} avgl_ctx_t, *avgl_ctx_p;

static void avgl_loop(avgl_p self)
{
	avgl_ctx_p ctx = (avgl_ctx_p)self->ctx;

	ctx->system->loop(ctx->system);
}

static void avgl_destroy(avgl_p self)
{
	avgl_ctx_p ctx = (avgl_ctx_p)self->ctx;
	for (ctx->oop->services->first(ctx->oop->services);
		ctx->oop->services->has_more(ctx->oop->services);
		ctx->oop->services->next(ctx->oop->services))
	{
		char* name = ctx->oop->services->get(ctx->oop->services);
		av_service_p service = ctx->oop->servicemap->get(ctx->oop->servicemap, name);
		if (service->refcnt > 1)
		ctx->log->warn(ctx->log, "Service %s have %d unreleased references", name, service->refcnt - 1);
	}

	O_release(ctx->system);
	O_release(ctx->log);
	ctx->oop->destroy(ctx->oop);
	free(self->ctx);
	free(self);
}

av_result_t avgl_create(avgl_p* pself)
{
	av_result_t rc;
	avgl_p self = (avgl_p)av_calloc(1, sizeof(avgl_t));
	if (!self)
		return AV_EMEM;


	self->loop    = avgl_loop;
	self->destroy = avgl_destroy;

	self->ctx = (avgl_ctx_p)av_calloc(1, sizeof(avgl_ctx_t));
	if (!self->ctx)
	{
		av_free(self);
		return AV_EMEM;
	}
	avgl_ctx_p ctx = (avgl_ctx_p)self->ctx;

	if (AV_OK != (rc = av_oop_create(&ctx->oop)))
	{
		av_free(self->ctx);
		av_free(self);
	}

	/* Initialize logging */
	av_log_register_oop(ctx->oop);
	ctx->oop->service_ref(ctx->oop, "log", (av_service_p*)&ctx->log);
	ctx->log->add_console_logger(ctx->log, LOG_VERBOSITY_DEBUG, "console");
	ctx->log->info(ctx->log, "AVGL is initializing...");

	/* Initialize system */
	av_system_sdl_register_oop(ctx->oop);
	ctx->oop->service_ref(ctx->oop, "system", (av_service_p*)&ctx->system);

	av_display_config_t display_config;
	display_config.width = 1280;
	display_config.height = 768;
	display_config.mode = 0;

	ctx->system->display->set_configuration(ctx->system->display, &display_config);

	*pself = self;
	return AV_OK;
}
