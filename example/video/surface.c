/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      surface.c                                          */
/* Description:   Demonstrate surface usage                          */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <av_torb.h>
#include <av_log.h>
#include <av_video.h>

#define XRES 1024
#define YRES 768

#define SURFACE_WIDTH 300
#define SURFACE_HEIGHT 200

static const char* video_driver = "video_sdl";
static av_log_p _log = 0;

#define av_check(name, exit_point, result) \
if (AV_OK != (rc = result))\
{\
	if (_log) \
		_log->error(_log, "%s returned %d", name, result); \
	else \
		printf("%s returned %d\n", name, result); \
	goto exit_point;\
}

/* entry point */
int main( void )
{
	av_result_t rc = AV_OK;
	av_video_p video;
	av_video_config_t video_config;
	av_surface_p surface;
	av_surface_p video_surface;
	av_pixel_p pixels;
	int pitch, i;
	av_rect_t src_rect;
	av_rect_t dst_rect;
	
	video_config.flags = AV_VIDEO_CONFIG_MODE | AV_VIDEO_CONFIG_SIZE;
	video_config.mode = AV_VIDEO_MODE_WINDOWED;
	video_config.width = XRES;
	video_config.height = YRES;

	av_check("av_torb_init", err_torba_init, av_torb_init())
	av_check("av_torb_service_addref", err_log_ref, av_torb_service_addref("log", (av_service_p*)&_log))
	av_check("av_torb_create_object", err_video_register, av_torb_create_object(video_driver, (av_object_p*)&video))
	av_check("video->set_configuration", err_video_mode, video->set_configuration(video, &video_config))

	video_surface = (av_surface_p)video;

	av_check("video_surface->lock", err_video_mode, video_surface->lock(video_surface, AV_SURFACE_LOCK_WRITE, &pixels, &pitch))
	memset(pixels, 0x80, XRES*YRES*4);
	av_check("video_surface->unlock", err_video_mode, video_surface->unlock(video_surface))

	av_check("video->create_surface", err_video_mode, video->create_surface(video, SURFACE_WIDTH, SURFACE_HEIGHT, (av_video_surface_p*)&surface))

	av_check("surface->lock", err_video_surface, surface->lock(surface, AV_SURFACE_LOCK_WRITE, &pixels, &pitch))
	memset(pixels, 0xFF, SURFACE_WIDTH*SURFACE_HEIGHT*4);
	av_check("surface->unlock", err_video_surface, surface->unlock(surface))

	av_rect_init(&src_rect, 0, 0, SURFACE_WIDTH, SURFACE_HEIGHT);
	av_rect_init(&dst_rect, 0, 0, SURFACE_WIDTH, SURFACE_HEIGHT);

	_log->info(_log, "blitting 10000 surfaces with size %dx%d...", SURFACE_WIDTH, SURFACE_HEIGHT);
	for (i=0; i<10000; i++)
	{
		dst_rect.x = rand()%(XRES - SURFACE_WIDTH);
		dst_rect.y = rand()%(YRES - SURFACE_HEIGHT);
		av_check("surface->blit", err_video_surface, ((av_video_surface_p)video_surface)->blit((av_video_surface_p)video_surface, &dst_rect, surface, &src_rect))
	}
	_log->info(_log, "done");

	goto err_video_surface;

err_video_surface:
	O_destroy(surface);
err_video_mode:
	O_destroy(video);
err_video_register:
	av_torb_service_release("log");
err_log_ref:
	av_torb_done();
err_torba_init:
	return rc;
}
