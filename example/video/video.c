/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      video.c                                            */
/* Description:   Test and demonstrate video usage                   */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <av_torb.h>
#include <av_log.h>
#include <av_video.h>

#define sleep(x) usleep(1000*(x))

#define NMODES 3

static int xres[NMODES] = {800, 1024, 1280};
static int yres[NMODES] = {600,  768, 1024};

static av_log_p _log = 0;
static const char* video_driver = "video_sdl";

#define av_check(name, exit_point, result) \
if (AV_OK != (rc = result))\
{\
	printf("%s returned %d\n", name, result); \
	goto exit_point;\
}

static av_result_t probe_video_mode(av_video_p video, int w, int h, av_bool_t fullscreen)
{
	av_result_t rc;
	av_video_config_t video_config;
	_log->info(_log, "Probing %s video mode %dx%d", (fullscreen?"fullscreen":"windowed"), w, h);
	video_config.flags = AV_VIDEO_CONFIG_MODE | AV_VIDEO_CONFIG_SIZE;
	video_config.mode = fullscreen?AV_VIDEO_MODE_FULLSCREEN:AV_VIDEO_MODE_WINDOWED;
	video_config.width = w;
	video_config.height = h;

	if (AV_OK != (rc = video->set_configuration(video, &video_config)))
	{
		_log->error(_log, "Unable to set %s video mode %dx%d", (fullscreen?"fullscreen":"windowed"), w, h);
	}
	else
	{
		video_config.flags |= AV_VIDEO_CONFIG_BPP;
		video->get_configuration(video, &video_config);
		_log->info(_log, "Video mode set to %s %dx%dx%d", ((video_config.mode & AV_VIDEO_MODE_FULLSCREEN)?"fullscreen":"windowed"),
		video_config.width, video_config.height, video_config.bpp);
		sleep(1);
	}
	return rc;
}

static av_result_t switch_video_modes(av_video_p video)
{
	int i;

	for (i=0; i<NMODES; i++)
		probe_video_mode(video, xres[i], yres[i], AV_TRUE);

	for (i=0; i<NMODES; i++)
		probe_video_mode(video, xres[i], yres[i], AV_FALSE);

	return AV_OK;
}

/* entry point */
int main( void )
{
	av_result_t rc = AV_OK;
	av_video_p video;

	av_check("av_torb_init", err_torba_init, av_torb_init());
	av_check("av_torb_service_addref", err_log_ref, av_torb_service_addref("log", (av_service_p*)&_log));
	av_check("av_torb_create_object", err_video_register, av_torb_create_object(video_driver, (av_object_p*)&video));
	av_check("switch_video_modes", err_video_mode, switch_video_modes(video));

err_video_mode:
	O_destroy(video);
err_video_register:
	av_torb_service_release("log");
err_log_ref:
	av_torb_done();
err_torba_init:
	return rc;
}
