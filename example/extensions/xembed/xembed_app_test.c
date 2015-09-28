/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      xembed_test.c                                      */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <av.h>
#include <av_torb.h>
#include <av_system.h>
#include <av_xembed.h>
#include <av_xembed_app.h>

#define X_APP_NAME "firefox"

#define xres 1024
#define yres 768

int main( void )
{
	av_result_t rc;
	av_xembed_p xembed;
	av_xembed_app_p app;
	av_system_p sys;
	av_window_p app_window;
	char* xappargs[2];
	char xappname[100];

	av_torb_init();
	av_xembed_register();

	if (AV_OK != (rc = av_torb_service_addref("xembed", (av_service_p*)&xembed)))
	{
		printf("unable to obtain reference to xembed service\n");
		return 1;
	}

	if (AV_OK != (rc = xembed->open(xembed, xres, yres)))
	{
		printf("xembed: open failed\n");
		return 1;
	}

	/* obtain reference to system service */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
	{
		printf("unable to obtain reference to system service\n");
		return 1;
	}

	/* create root window */
	if (AV_OK != (rc = sys->create_window(sys, AV_NULL, AV_NULL, &app_window)))
	{
		printf("system: create root window failed\n");
		return 1;
	}

	strcpy(xappname, X_APP_NAME);
	xappargs[0] = xappname;
	xappargs[1] = AV_NULL;

	/* create xembedded application */
	xembed->execute(xembed, xappargs, app_window, &app);

	sys->loop(sys);
	O_destroy(app);

	av_torb_service_release("xembed");
	av_torb_done();

	return 0;
}
