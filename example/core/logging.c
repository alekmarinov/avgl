/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      logging.c                                          */
/* Description:   Demonstrate TORBA service usage and logging        */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <av_torb.h>
#include <av_log.h>

/* entry point */
int main( void )
{
	/* Initialize TORBA */
	if (AV_OK == av_torb_init())
	{
		av_log_p log;

		/* Get reference to service log */
		if (AV_OK == av_torb_service_addref("log", (av_service_p*)&log))
		{
			/* Use service object */
			log->info(log, "simple log message");
			av_torb_service_release("log");
		}

		/* Deinitialize TORBA */
		av_torb_done();
	}

	return 0;
}
