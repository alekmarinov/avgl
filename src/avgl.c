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

av_bool_t avgl_step()
{
	return avgl.system->step(avgl.system);
}

void avgl_destroy()
{
	av_visible_p main_visible;
	for (avgl.oop->services->first(avgl.oop->services);
		avgl.oop->services->has_more(avgl.oop->services);
		avgl.oop->services->next(avgl.oop->services))
	{
		char* name = avgl.oop->services->get(avgl.oop->services);
		av_service_p service = avgl.oop->servicemap->get(avgl.oop->servicemap, name);
		if (((av_object_p)service)->refcnt > 1)
			avgl.log->warn(avgl.log, "Service %s have %d unreleased references", name, ((av_object_p)service)->refcnt - 1);
	}

	main_visible = avgl.system->get_root_visible(avgl.system);
	if (main_visible)
		O_release(main_visible);
	O_release(avgl.system);
	O_release(avgl.log);
	avgl.oop->destroy(avgl.oop);
}

av_result_t avgl_last_error()
{
	return avgl.last_error;
}

av_visible_p avgl_create(av_display_config_p pdc)
{
	av_result_t rc;

	if (AV_OK != (rc = av_oop_create(&avgl.oop)))
	{
		avgl.last_error = rc;
		return AV_NULL;
	}

	/* Initialize logging */
	av_log_register_oop(avgl.oop);
	avgl.oop->get_service(avgl.oop, "log", (av_service_p*)&avgl.log);
	avgl.log->add_console_logger(avgl.log, LOG_VERBOSITY_DEBUG, "console");
	avgl.log->info(avgl.log, "AVGL is initializing...");

	/* Initialize system */
	av_graphics_cairo_register_oop(avgl.oop);
	av_system_sdl_register_oop(avgl.oop);
	avgl.oop->get_service(avgl.oop, "system", (av_service_p*)&avgl.system);

	av_sprite_register_oop(avgl.oop);

	av_display_config_t display_config;
	if (!pdc)
	{
		display_config.width = 1280;
		display_config.height = 1020;
		display_config.scale_x = 1;
		display_config.scale_y = 1;
		display_config.mode = 0;
	}
	else
	{
		display_config = *pdc;
	}
	if (AV_OK != (rc = avgl.system->initialize(avgl.system, &display_config)))
	{
		avgl.last_error = rc;
		return AV_NULL;
	}
	return avgl.system->get_root_visible(avgl.system);
}


void avgl_capture_visible(av_visible_p visible)
{
	avgl.system->set_capture(avgl.system, (av_window_p)visible);
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

av_bitmap_p avgl_load_bitmap(const char* filename)
{
	av_result_t rc;
	av_bitmap_p bitmap;

	if (AV_OK != (rc = avgl.system->create_bitmap(avgl.system, &bitmap)))
	{
		avgl.last_error = rc;
		return AV_NULL;
	}

	if (AV_OK != (rc = bitmap->load(bitmap, filename)))
	{
		O_release(bitmap);
		avgl.last_error = rc;
		return AV_NULL;
	}

	return bitmap;
}

av_surface_p avgl_load_surface(const char* filename)
{
	av_result_t rc;
	av_surface_p surface;
	av_bitmap_p bitmap = avgl_load_bitmap(filename);
	if (!bitmap)
		return AV_NULL;

	if (AV_OK != (rc = avgl.system->display->create_surface(avgl.system->display, &surface)))
	{
		O_release(bitmap);
		avgl.last_error = rc;
		return AV_NULL;
	}

	if (AV_OK != (rc = surface->set_bitmap(surface, bitmap)))
	{
		O_release(surface);
		O_release(bitmap);
		avgl.last_error = rc;
		return AV_NULL;
	}

	return surface;
}
