/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_surface_sdl.h                                   */
/* Description:   SDL surface backend                                */
/*                                                                   */
/*********************************************************************/

#ifndef __AV_SURFACE_SDL_H
#define __AV_SURFACE_SDL_H

#include "av_display_sdl.h"

typedef struct _surface_sdl_ctx_t
{
	SDL_Texture *texture;
	av_display_sdl_p display;
} surface_sdl_ctx_t, *surface_sdl_ctx_p;

static const char* surface_context = "surface_sdl_ctx_p";
#define O_surface_context(o) ((surface_sdl_ctx_p)O_attr(o, surface_context))

AV_API av_result_t av_surface_sdl_register_oop(av_oop_p);

#endif /* __AV_DISPLAY_SDL_H */
