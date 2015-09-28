/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      system.c                                           */
/* Description:   Demonstrate system usage                           */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <av_system.h>

/* entry point */
int main( void )
{
	av_result_t rc;
	av_system_p sys;
	av_video_p video;
	av_input_p input;
	av_video_config_t video_config;
	av_event_t event;

	/* Initialize TORBA */
	if (AV_OK != (rc = av_torb_init()))
		return rc;

	/* get reference to system service */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
	{
		av_torb_done();
		return rc; /* exit cleanly with error status */
	}

	/* setup video settings */
	video_config.flags  = AV_VIDEO_CONFIG_MODE | AV_VIDEO_CONFIG_SIZE;
	video_config.mode   = AV_VIDEO_MODE_WINDOWED;
	video_config.width  = 800;
	video_config.height = 600;

	/* get SDL video & input */
	sys->get_video(sys, &video);
	sys->get_input(sys, &input);

	/* apply SDL video settings */
	video->set_configuration(video, &video_config);
	event.type = 0;
	do
	{
		while (!(input->poll_event(input, &event)));
	} while (event.type != AV_EVENT_MOUSE_BUTTON || event.button_status != AV_BUTTON_PRESSED);
	av_torb_service_release("system");

	/* Deinitialize TORBA */
	av_torb_done();
	return 0;
}
