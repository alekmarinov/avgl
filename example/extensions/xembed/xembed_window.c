/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      xembed_window.c                                    */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <av.h>
#include <av_system.h>
#include <av_xembed.h>
#include <av_xembed_window.h>

av_result_t av_scaler_swscale_register_torba(void);

int main( void )
{
	av_result_t rc;
	av_system_p sys;
	av_window_p root;
	av_xembed_window_p xembed_window;
	av_rect_t win_rect = { 0, 0, 400, 300 };

	av_torb_init();

	av_xembed_register();
	av_xembed_window_register_torba();

	/* get reference to system service */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
	{
		av_torb_done();
		return rc; /* exit cleanly with error status */
	}

	/* create root window */
	if (AV_OK != (rc = sys->create_window(sys, AV_NULL, AV_NULL, &root)))
	{
		av_torb_service_release("system");
		av_torb_done();
		return rc; /* exit cleanly with error status */
	}

	/* create xembed window */
	if (AV_OK != (rc = av_torb_create_object("xembed_window", (av_object_p*)&xembed_window)))
	{
		av_torb_service_release("system");
		av_torb_done();
		return rc; /* exit cleanly with error status */
	}

	xembed_window->open(xembed_window, root, &win_rect);
	xembed_window->show(xembed_window);
	//system("sh ./xtest.sh");

	while (sys->step(sys))
	{
		xembed_window->idle(xembed_window);
	}

	O_destroy(xembed_window);
	av_torb_service_release("system");
	av_torb_done();
	return 0;
}
