/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_surface_sdl.h                                   */
/* Description:   SDL surface backend                                */
/*                                                                   */
/*********************************************************************/

#ifndef __AV_BITMAPA_SDL_H
#define __AV_BITMAPA_SDL_H

#include <av_oop.h>
#include <SDL.h>

typedef struct _bitmap_sdl_ctx_t
{
	SDL_Surface *surface;
} bitmap_sdl_ctx_t, *bitmap_sdl_ctx_p;

static const char* bitmap_context = "bitmap_sdl_ctx_p";
#define O_bitmap_context(o) ((bitmap_sdl_ctx_p)O_attr(o, bitmap_context))

//AV_API av_result_t av_surface_sdl_register_oop(av_oop_p);

#endif /* __AV_BITMAPA_SDL_H */
