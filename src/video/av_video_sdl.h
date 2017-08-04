/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_video_sdl.h                                     */
/* Description:   SDL video backend                                  */
/*                                                                   */
/*********************************************************************/

#ifndef __AV_VIDEO_SDL_H
#define __AV_VIDEO_SDL_H

#include <av_video.h>
#include <av_system.h>
#include <av_thread.h>

#include <SDL.h>
#include <SDL_syswm.h>

#define SDL_MEM_SURFACE_BPP 32
#define SDL_MEM_SURFACE_MASK_ALPHA 0xff000000
#define SDL_MEM_SURFACE_MASK_RED   0x00ff0000
#define SDL_MEM_SURFACE_MASK_GREEN 0x0000ff00
#define SDL_MEM_SURFACE_MASK_BLUE  0x000000ff

#define CONTEXT_SDL_SURFACE "video_surface_sdl_ctx"
#define CONTEXT_SDL_OVERLAY "video_overlay_sdl_ctx"

typedef struct av_video_sdl
{
	/* Parent object */
	av_video_t video;
} av_video_sdl_t, *av_video_sdl_p;

typedef struct av_video_surface_sdl
{
	/*! parent object */
	av_video_surface_t video_surface;

} av_video_surface_sdl_t, *av_video_surface_sdl_p;

typedef struct av_video_overlay_sdl
{
	/*! pointer to backbuffer surface */
	SDL_Surface* surface;

	/*! the overlay himself */
	SDL_Overlay* overlay;

	/*! the actual size */
	int w;
	int h;

	/*! destination pos and size
	 * \brief set with blit_back
	 */
	av_rect_t dstrect;

	/*! overlay mutex */
	av_mutex_p mtx;

} av_video_overlay_sdl_t, *av_video_overlay_sdl_p;

av_result_t av_video_surface_sdl_create_overlay(av_video_surface_p psurface, av_video_overlay_p* ppoverlay);
av_result_t av_video_surface_sdl_create_overlay_buffered(av_video_surface_p psurface, av_video_overlay_p* ppoverlay);

#endif /* __AV_VIDEO_SDL_H */
