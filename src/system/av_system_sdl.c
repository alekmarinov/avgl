/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_system_sdl.c                                    */
/* Description:   Defines class system sdl (av_system_sdl_t)         */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_SDL

#include <av.h>
#include <av_log.h>
#include <av_system.h>
#include <malloc.h>
#include <string.h>
#include <SDL.h>
#include <SDL_keyboard.h>

#include "av_system_sdl.h"

/* exported prototypes */
av_result_t av_sdl_error_process(int, const char*, const char*, int);
av_result_t av_system_sdl_register_torba(void);

/* imported prototypes */
av_result_t av_audio_sdl_register_torba(void);
av_result_t av_video_sdl_register_torba(void);
av_result_t av_input_sdl_register_torba(void);
av_result_t av_timer_sdl_register_torba(void);

static av_log_p _log = AV_NULL;

av_result_t av_sdl_error_process(int rc, const char* funcname, const char* srcfilename, int linenumber)
{
	av_result_t averr = AV_OK;
	if (rc < 0)
	{
		const char* errmsg = SDL_GetError();
		if (!strcmp(errmsg, "Out of memory"))
			averr = AV_EMEM;
		else if (!strcmp(errmsg, "Error reading from datastream"))
			averr = AV_EREAD;
		else if (!strcmp(errmsg, "Error writing to datastream"))
			averr = AV_EWRITE;
		else if (!strcmp(errmsg, "Error seeking in datastream"))
			averr = AV_ESEEK;

		if (averr != AV_OK)
		{
			if (_log) _log->error(_log, "%s returned error (%d) `%s' %s:%d", 
									funcname, rc, errmsg, srcfilename, linenumber);
		}
		else
		{
			if (_log) _log->error(_log, "%s returned unknown error with message `%s' %s:%d", 
									funcname, errmsg, srcfilename, linenumber);
		}
	}
	return averr;
}

#define av_sdl_error_check(funcname, rc) av_sdl_error_process(rc, funcname, __FILE__, __LINE__)

/* gets instance to SDL audio */
static av_result_t av_system_sdl_get_audio(av_system_p self, av_audio_p* ppaudio)
{
	av_result_t rc;
	if (AV_NULL == self->audio)
		if (AV_OK != (rc = av_torb_create_object("audio_sdl", (av_object_p*)&(self->audio))))
			return rc;
	*ppaudio = self->audio;
	return AV_OK;
}

/* gets instance to SDL video */
static av_result_t av_system_sdl_get_video(av_system_p self, av_video_p* ppvideo)
{
	av_result_t rc;
	if (AV_NULL == self->video)
		if (AV_OK != (rc = av_torb_create_object("video_sdl", (av_object_p*)&(self->video))))
			return rc;
	*ppvideo = self->video;
	self->video->system = self;
	return AV_OK;
}

/* gets instance to SDL input */
static av_result_t av_system_sdl_get_input(av_system_p self, av_input_p* ppinput)
{
	av_result_t rc;
	if (AV_NULL == self->input)
	{
		/* Checks if video is already initialized. If not, produce error */
		if (AV_NULL == self->video)
		{
			_log->error(_log, "sdl video must be initialized before creating sdl input");
			return AV_EINIT;
		}
		/* Creates input object */
		if (AV_OK != (rc = av_torb_create_object("input_sdl", (av_object_p*)&(self->input))))
			return rc;
	}
	*ppinput = self->input;
	return AV_OK;
}

/* gets instance to SDL timer */
static av_result_t av_system_sdl_get_timer(av_system_p self, av_timer_p* pptimer)
{
	av_result_t rc;
	if (AV_NULL == self->timer)
		if (AV_OK != (rc = av_torb_create_object("timer_sdl", (av_object_p*)&(self->timer))))
			return rc;
	*pptimer = self->timer;
	return AV_OK;
}

static void av_system_sdl_destructor(void* psystemsdl)
{
	av_system_p self = (av_system_p)psystemsdl;
	if (self->audio) O_destroy(self->audio);
	if (self->timer) O_destroy(self->timer);
	if (self->input) O_destroy(self->input);
	if (self->video) O_destroy(self->video);

	/* Quit SDL */
	_log->debug(_log, "SDL: Quiting");
	SDL_Quit();

	/* release log service */
	av_torb_service_release("log");
}

/* Initializes memory given by the input pointer with the system sdl class information */
static av_result_t av_system_sdl_constructor(av_object_p object)
{
	int sdl_flags;
	av_result_t rc;
	av_system_p self = (av_system_p)object;

	self->get_audio = av_system_sdl_get_audio;
	self->get_video = av_system_sdl_get_video;
	self->get_input = av_system_sdl_get_input;
	self->get_timer = av_system_sdl_get_timer;

	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&_log)))
		return rc;

	/* Setup environment variables for proper SDL functionality */
	av_system_sdl_setup_param();

	/* Initializes SDL library without catching any fatal signals */
	sdl_flags = SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO;

	_log->debug(_log, "SDL: Initializing");
	if (AV_OK != (rc = av_sdl_error_check("SDL_Init", SDL_Init(sdl_flags))))
	{
		av_torb_service_release("log");
		return rc;
	}

	/* Enable unicode translation of keyboard input */
	_log->debug(_log, "SDL: Enable UNICODE keyboard input translation");
	SDL_EnableUNICODE(1);

	/* Enable key repetition */
	_log->debug(_log, "SDL: Enable key repetition delay:%d, interval:%d",
				SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	return AV_OK;
}

/* Registers system sdl class into TORBA class repository */
av_result_t av_system_sdl_register_torba(void)
{
	av_result_t rc;
	if (AV_OK != (rc = av_audio_sdl_register_torba()))
		return rc;
	if (AV_OK != (rc = av_video_sdl_register_torba()))
		return rc;
	if (AV_OK != (rc = av_input_sdl_register_torba()))
		return rc;
	if (AV_OK != (rc = av_timer_sdl_register_torba()))
		return rc;
	return av_torb_register_class("system_sdl", "system", sizeof(av_system_t), av_system_sdl_constructor, av_system_sdl_destructor);
}

#endif /* WITH_SYSTEM_SDL */
