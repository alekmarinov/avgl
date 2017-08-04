/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      graphics.c                                         */
/* Description:   Demonstrate graphics usage                         */
/*                                                                   */
/*********************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <av_torb.h>
#include <av_log.h>
#include <av_system.h>
#include <av_graphics.h>

#define XRES 1024
#define YRES 768

#define SURFACE_WIDTH 300
#define SURFACE_HEIGHT 200

static const char* graphics_driver = "graphics_cairo";
static av_log_p _log = 0;

#define av_check(name, exit_point, result) \
if (AV_OK != (rc = result))\
{\
	if (_log) \
		_log->error(_log, "%s returned %d\n", name, result); \
	else \
		printf("%s returned %d\n", name, result); \
	goto exit_point;\
}

/* entry point */
int main( void )
{
	av_result_t rc = AV_OK;
	av_system_p sys;
	av_video_p video;
	av_graphics_p graphics;
	av_timer_p timer;
	av_graphics_surface_p graphics_surface;
	av_video_config_t video_config;
	av_surface_p video_surface;
	av_pixel_p pixels;
	av_rect_t rect;
	int pitch;

	video_config.mode = AV_VIDEO_MODE_WINDOWED;
	video_config.width = XRES;
	video_config.height = YRES;
	video_config.bpp = 0;

	av_check("av_torb_init", err_torba_init, av_torb_init())

	av_check("av_torb_service_addref", err_log_ref, av_torb_service_addref("log", (av_service_p*)&_log))
	_log->set_verbosity(_log, NULL, LOG_VERBOSITY_DEBUG);

	av_check("av_torb_service_addref",           err_system_register, av_torb_service_addref("system", (av_service_p*)&sys))
	av_check("system->get_video",                err_system_register, sys->get_video(sys, &video))
	av_check("system->get_timer",                err_system_register, sys->get_timer(sys, &timer))
	av_check("video->set_configuration",         err_video_mode,      video->set_configuration(video, &video_config))

	video_surface = (av_surface_p)video;

	/* clear video surface */
	av_check("video_surface->lock", err_video_mode, video_surface->lock(video_surface, AV_SURFACE_LOCK_WRITE, &pixels, &pitch))
	memset(pixels, 0x80, XRES*YRES*4);
	av_check("video_surface->unlock", err_video_mode, video_surface->unlock(video_surface))

	av_check("av_torb_create_object", err_video_mode, av_torb_create_object(graphics_driver, (av_object_p*)&graphics))

	av_check("graphics->wrap_surface", err_graphics, graphics->wrap_surface(graphics, video_surface, &graphics_surface))

	/* draw something */
	graphics->begin(graphics, graphics_surface);
	graphics->set_line_width(graphics, 10.0);
	graphics->move_to(graphics, 0, 0);
	graphics->line_to(graphics, XRES, YRES);
	graphics->stroke(graphics, 0);
	graphics->arc(graphics, XRES/2, YRES/2, AV_MIN(XRES,YRES)/2, 0, 2*AV_PI);
	av_rect_init(&rect, 0, 0, XRES, YRES);
	graphics->rectangle(graphics, &rect);
	graphics->move_to(graphics, 0, YRES/2);
	graphics->curve_to(graphics, XRES/3, 3*YRES/4, 3*XRES/4, 3*YRES/4, XRES, YRES/2);
	graphics->close_path(graphics);
	graphics->stroke(graphics, 0);
	graphics->end(graphics);
	video->update(video,AV_NULL);
	goto err_graphics_surface;

err_graphics_surface:
	/* destroying graphics surface flush the painting over the target surface */
	O_destroy(graphics_surface);
	timer->sleep(5);
err_graphics:
	O_destroy(graphics);
err_video_mode:
err_system_register:
	av_torb_service_release("system");
	av_torb_service_release("log");
err_log_ref:
	av_torb_done();
err_torba_init:
	return rc;
}
