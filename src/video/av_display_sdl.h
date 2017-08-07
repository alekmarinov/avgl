/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_display_sdl.h                                   */
/* Description:   SDL display backend                                */
/*                                                                   */
/*********************************************************************/

#ifndef __AV_DISPLAY_SDL_H
#define __AV_DISPLAY_SDL_H

#include <av_display.h>

#include <SDL.h>

typedef struct av_display_sdl
{
	/* Parent object */
	av_display_t display;

	/* SDL window */
	SDL_Window* window;

	/* SDL renderer */
	SDL_Renderer* renderer;
} av_display_sdl_t, *av_display_sdl_p;

AV_API av_result_t av_display_sdl_register_oop(av_oop_p);

#endif /* __AV_DISPLAY_SDL_H */
