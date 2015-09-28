/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      avplayer.c                                         */
/* Description:   Demonstrate player usage                           */
/*                                                                   */
/*********************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <av_system.h>
#include <av_sound.h>
#include <av_log.h>

av_log_p _log = 0;

int main(int nargs, char* argv[])
{
	av_result_t rc;
	av_system_p sys;
	av_sound_p sound;
	av_timer_p timer;
	struct av_sound_handle* sound_click;

	if (nargs < 2)
	{
		fprintf(stderr, "Usage: %s <sound_file.wav>\n",argv[0]);
		return 1;
	}

	if (AV_OK != (rc = av_torb_init()))
		return rc;

	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&_log)))
		return rc;

	/* get reference to system service */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
	{
		av_torb_service_release("log");
		av_torb_done();
		return rc;
	}

	sys->get_timer(sys, &timer);

	/* create sound object */
	if (AV_OK != (rc = av_torb_create_object("sound", (av_object_p*)&sound)))
	{
		_log->error(_log, "Error %d: sound create failed", rc);
		goto err_exit;
	}

	if (AV_OK != (rc = sound->open(sound,argv[1],&sound_click)))
	{
		_log->error(_log, "Error %d: sound open failed", rc);
		goto err_exit;
	}

	sound->play(sound,sound_click);

	while (sound->is_playing(sound,sound_click))
		timer->sleep_ms(10);

	sound->close(sound,sound_click);

	O_destroy(sound);

err_exit:
	av_torb_service_release("system");
	av_torb_service_release("log");
	av_torb_done();


	return 0;
}
