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
	if (AV_OK != (rc = av_torb_init())) {
		printf("av_torb_init failed\n");
		return rc;
	}

	/* get reference to system service */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
	{
		av_torb_done();
		printf("av_torb_service_addref failed\n");
		return rc; /* exit cleanly with error status */
	}

	/* setup video settings */
	video_config.mode   = AV_VIDEO_MODE_WINDOWED;
	video_config.width  = 800;
	video_config.height = 600;
	video_config.bpp = 0;

	/* get SDL video & input */
	sys->get_video(sys, &video);
	sys->get_input(sys, &input);

	/* apply SDL video settings */
	video->set_configuration(video, &video_config);
	event.type = 0;
	do
	{
		while (!(input->poll_event(input, &event)));

		printf("Event type = %d\n", event.type);
	} while (event.type != AV_EVENT_MOUSE_BUTTON || event.button_status != AV_BUTTON_PRESSED);
	av_torb_service_release("system");

	/* Deinitialize TORBA */
	av_torb_done();
	printf("Exiting...\n");
	return 0;
}
