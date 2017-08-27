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
void av_graphics_cairo_register_oop(av_oop_p);

typedef struct _avgl_t
{
	av_result_t last_error;

	/* reference to oop container */
	av_oop_p oop;

	/* reference to log service */
	av_log_p log;

	/* reference to system service */
	av_system_p system;

} avgl_t;

static avgl_t avgl;

void avgl_loop()
{
	avgl.system->loop(avgl.system);
}

void avgl_step()
{
	avgl.system->step(avgl.system);
}

void avgl_destroy()
{
	for (avgl.oop->services->first(avgl.oop->services);
		avgl.oop->services->has_more(avgl.oop->services);
		avgl.oop->services->next(avgl.oop->services))
	{
		char* name = avgl.oop->services->get(avgl.oop->services);
		av_service_p service = avgl.oop->servicemap->get(avgl.oop->servicemap, name);
		if (service->refcnt > 1)
			avgl.log->warn(avgl.log, "Service %s have %d unreleased references", name, service->refcnt - 1);
	}

	O_release(avgl.system);
	O_release(avgl.log);
	avgl.oop->destroy(avgl.oop);
}

av_result_t avgl_last_error()
{
	return avgl.last_error;
}

av_bool_t avgl_create()
{
	av_result_t rc;
	av_rect_t winrect;

	if (AV_OK != (rc = av_oop_create(&avgl.oop)))
	{
		avgl.last_error = rc;
		return AV_FALSE;
	}

	/* Initialize logging */
	av_log_register_oop(avgl.oop);
	avgl.oop->service_ref(avgl.oop, "log", (av_service_p*)&avgl.log);
	avgl.log->add_console_logger(avgl.log, LOG_VERBOSITY_DEBUG, "console");
	avgl.log->info(avgl.log, "AVGL is initializing...");

	/* Initialize system */
	av_graphics_cairo_register_oop(avgl.oop);
	av_system_sdl_register_oop(avgl.oop);
	avgl.oop->service_ref(avgl.oop, "system", (av_service_p*)&avgl.system);

	av_display_config_t display_config;
	display_config.width = 1280;
	display_config.height = 768;
	display_config.mode = 0;

	avgl.system->display->set_configuration(avgl.system->display, &display_config);

	av_rect_init(&winrect, 0, 0, display_config.width, display_config.height);
	avgl.system->create_visible(avgl.system, AV_NULL, &winrect, AV_NULL);

	return AV_TRUE;
}

av_visible_p avgl_create_visible(av_visible_p parent, int x, int y, int w, int h, on_draw_t on_draw)
{
	av_result_t rc;
	av_rect_t rect;
	av_visible_p visible;
	av_rect_init(&rect, x, y, w, h);
	if (AV_OK != (rc = avgl.system->create_visible(avgl.system, parent, &rect, &visible)))
	{
		avgl.last_error = rc;
		return AV_NULL;
	}
	if (on_draw)
		visible->on_draw = on_draw;
	visible->draw(visible);
	return visible;
}

void avgl_capture_visible(av_visible_p visible)
{
	avgl.system->set_capture(avgl.system, visible);
}

unsigned long avgl_time_now()
{
	return avgl.system->timer->now();
}

void avgl_event_push(av_event_p event)
{
	avgl.system->input->push_event(avgl.system->input, event);
}

av_bool_t avgl_event_poll(av_event_p event)
{
	return avgl.system->input->poll_event(avgl.system->input, event);
}

