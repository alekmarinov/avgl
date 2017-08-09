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

#include "av_core_sdl.h"
#include <avgl.h>
#include <SDL.h>
#include <SDL_keyboard.h>

/* imported prototypes */
av_result_t av_display_sdl_register_oop(av_oop_p);
av_result_t av_input_sdl_register_oop(av_oop_p);
av_result_t av_timer_sdl_register_oop(av_oop_p);
/*
av_result_t av_audio_sdl_register_oop(av_oop_p);
*/

static void av_system_sdl_destructor(void* psystemsdl)
{
	av_system_p self = (av_system_p)psystemsdl;

	/* Quit SDL */
	// _log->debug(_log, "SDL: Quiting");
	SDL_Quit();
}

/* Initializes memory given by the input pointer with the system sdl class information */
static av_result_t av_system_sdl_constructor(av_object_p object)
{
	int sdl_flags;
	av_result_t rc;
	av_system_p self = (av_system_p)object;

	/* Initializes SDL library without catching any fatal signals */
	sdl_flags = SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO;

	// _log->debug(_log, "SDL: Initializing");
	if (AV_OK != (rc = av_sdl_error_check("SDL_Init", SDL_Init(sdl_flags))))
		return rc;


	/* Enable unicode translation of keyboard input */
	// _log->debug(_log, "SDL: Enable UNICODE keyboard input translation");
	// SDL_EnableUNICODE(1);

	/* Enable key repetition */
	//_log->debug(_log, "SDL: Enable key repetition delay:%d, interval:%d",
	//			SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	//SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	return AV_OK;
}

/* Registers system sdl class into oop container */
av_result_t av_system_sdl_register_oop(av_oop_p oop)
{
	av_result_t rc;
	av_system_p system;
	if (AV_OK != (rc = av_system_register_oop(oop)))
		return rc;
	if (AV_OK != (rc = av_display_sdl_register_oop(oop)))
		return rc;
	if (AV_OK != (rc = av_input_sdl_register_oop(oop)))
		return rc;
	if (AV_OK != (rc = av_timer_sdl_register_oop(oop)))
		return rc;
	/*	if (AV_OK != (rc = av_audio_sdl_register_oop(oop)))
		return rc;*/
	if (AV_OK != (rc = oop->define_class(oop, "system_sdl", "system", sizeof(av_system_t), av_system_sdl_constructor, av_system_sdl_destructor)))
		return rc;

	if (AV_OK != (rc = oop->new(oop, "system_sdl", (av_object_p*)&system)))
		return rc;

	/* register system as service */
	return oop->register_service(oop, "system", (av_service_p)system);
}

#endif /* WITH_SYSTEM_SDL */
