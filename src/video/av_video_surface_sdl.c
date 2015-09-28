/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_video_surface_sdl.c                             */
/* Description:   SDL video surface implementation                   */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_SDL

#include <malloc.h>

#include <av.h>
#include <av_thread.h>
#include <av_surface.h>
#include "av_video_sdl.h"

#define O_context(o) O_attr(o, CONTEXT_SDL_SURFACE)
#define O_context_overlay(o) O_attr(o, CONTEXT_SDL_OVERLAY)

/* imported prototypes */
av_result_t av_sdl_error_process(int, const char*, const char*, int);

/* exported prototypes */
av_result_t av_video_surface_sdl_register_torba(void);
av_result_t av_video_surface_sdl_constructor(av_object_p object);

#define av_sdl_error_check(funcname, rc) av_sdl_error_process(rc, funcname, __FILE__, __LINE__)

static av_result_t av_surface_sdl_set_size(av_surface_p psurface, int width, int height)
{
	av_video_surface_sdl_p self = (av_video_surface_sdl_p)psurface;
	SDL_Surface *surface = O_context(self);

	/* checks if surface already created and dimensions are the same */
	if (surface && width == surface->w && height == surface->h) return AV_OK;

	/* creates 32bit SDL surface */
	if (AV_NULL == (surface = SDL_CreateRGBSurface(SDL_SURFACE_TYPE,
													width, height, 32,
													SDL_SURFACE_MASK_RED, SDL_SURFACE_MASK_GREEN,
													SDL_SURFACE_MASK_BLUE, SDL_SURFACE_MASK_ALPHA)))
	{
		return AV_EMEM;
	}
	SDL_LockSurface(surface);

	if (O_context(self))
	{
		/* destroys the previously created surface */
		SDL_UnlockSurface((SDL_Surface *)O_context(self));
		SDL_FreeSurface((SDL_Surface *)O_context(self));
	}
	O_set_attr(self, CONTEXT_SDL_SURFACE, surface);
	return AV_OK;
}

static av_result_t av_surface_sdl_get_size(av_surface_p psurface, int* pwidth, int* pheight)
{
	av_video_surface_sdl_p self = (av_video_surface_sdl_p)psurface;
	SDL_Surface *surface = (SDL_Surface *)O_context(self);
	if (surface)
	{
		*pwidth = surface->w;
		*pheight = surface->h;
	}
	else
	{
		*pwidth  = *pheight = 0;
	}
	return AV_OK;
}

static av_result_t av_surface_sdl_lock(av_surface_p psurface, av_surface_lock_flags_t lockflags,
											 av_pixel_p* ppixels, int* ppitch)
{
	av_video_surface_sdl_p self = (av_video_surface_sdl_p)psurface;
	SDL_Surface *surface = (SDL_Surface *)O_context(self);
	av_assert(surface, "Attempt to Lock uninitialized surface");
	AV_UNUSED(lockflags);
	*ppixels = (av_pixel_p)(surface->pixels);
	*ppitch = surface->pitch;
	return AV_OK;
}

static av_result_t av_surface_sdl_unlock(av_surface_p psurface)
{
	av_video_surface_sdl_p self = (av_video_surface_sdl_p)psurface;
	SDL_Surface *surface = (SDL_Surface *)O_context(self);
	av_assert(surface, "Attempt to Unlock uninitialized surface");
	return AV_OK;
}

static av_result_t av_video_surface_sdl_lock(av_video_surface_p psurface, av_surface_lock_flags_t lockflags,
											 av_pixel_p* ppixels, int* ppitch)
{
	av_surface_p self = (av_surface_p)psurface;
	return self->lock(self, lockflags, ppixels, ppitch);
}

static av_result_t av_video_surface_sdl_unlock(av_video_surface_p psurface)
{
	av_surface_p self = (av_surface_p)psurface;
	return self->unlock(self);
}

static av_result_t av_video_surface_sdl_set_clip(av_surface_p psurface, av_rect_p cliprect)
{
	SDL_Rect rect;
	av_video_surface_sdl_p self = (av_video_surface_sdl_p)psurface;
	SDL_Surface *surface = (SDL_Surface *)O_context(self);
	av_assert(surface, "Attempt to SetClip uninitialized surface");

	rect.x = cliprect->x;
	rect.y = cliprect->y;
	rect.w = cliprect->w;
	rect.h = cliprect->h;

	SDL_SetClipRect(surface, &rect);
	return AV_OK;
}

static av_result_t av_video_surface_sdl_get_clip(av_surface_p psurface, av_rect_p cliprect)
{
	SDL_Rect rect;
	av_video_surface_sdl_p self = (av_video_surface_sdl_p)psurface;
	SDL_Surface *surface = (SDL_Surface *)O_context(self);
	av_assert(surface, "Attempt to GetClip uninitialized surface");

	SDL_GetClipRect(surface, &rect);

	cliprect->x = rect.x;
	cliprect->y = rect.y;
	cliprect->w = rect.w;
	cliprect->h = rect.h;

	return AV_OK;
}

/* Blit source surface to this video surface */
static av_result_t av_video_surface_sdl_blit(av_video_surface_p pdstsurface,
											 av_rect_p dstrect,
											 av_surface_p psrcsurface,
											 av_rect_p srcrect)
{
	av_video_surface_sdl_p self = (av_video_surface_sdl_p)pdstsurface;
	SDL_Surface *dstsurface = (SDL_Surface *)O_context(self);
	av_assert(dstsurface, "Attempt to Blit uninitialized surface");

	if (O_is_a(psrcsurface, "video_surface_sdl"))
	{
		SDL_Rect srect, drect;
		av_video_surface_sdl_p src = (av_video_surface_sdl_p)psrcsurface;
		SDL_Surface *srcsurface = (SDL_Surface *)O_context(src);
		av_assert(srcsurface, "Attempt to Blit from uninitialized surface");

		srect.x = srcrect->x;
		srect.y = srcrect->y;
		srect.w = srcrect->w;
		srect.h = srcrect->h;

		drect.x = dstrect->x;
		drect.y = dstrect->y;
		drect.w = dstrect->w;
		drect.h = dstrect->h;

		SDL_UnlockSurface(srcsurface);

		if ((dstrect->w == srcrect->w) && (dstrect->h == srcrect->h))
			SDL_BlitSurface(srcsurface, &srect, dstsurface, &drect);
		else
			SDL_SoftStretch(srcsurface, &srect, dstsurface, &drect);

		SDL_LockSurface(srcsurface);
	}
	else
	{
		/* FIXME: implement software blitting */
		return AV_ESUPPORTED;
	}

	return AV_OK;
}

static av_result_t av_video_surface_sdl_set_size(av_video_surface_p self, int width, int height)
{
	return av_surface_sdl_set_size((av_surface_p)self, width, height);
}

static av_result_t av_video_surface_sdl_get_size(av_video_surface_p self, int* pwidth, int* pheight)
{
	return av_surface_sdl_get_size((av_surface_p)self, pwidth, pheight);
}

static av_result_t av_video_surface_sdl_get_depth(struct av_video_surface* self, int* pdepth)
{
	SDL_Surface *surface = (SDL_Surface *)O_context(self);

	if (!surface)
		return AV_EFOUND;

	*pdepth = surface->w / surface->pitch;
	return AV_OK;
}

static av_result_t av_video_surface_sdl_fill_rect(av_video_surface_p self, av_rect_p prect,
												double r, double g, double b, double a)
{
	SDL_Rect rect;
	unsigned int color;
	SDL_Surface *surface = (SDL_Surface *)O_context(self);

	if (!surface)
		return AV_EFOUND;

	color = SDL_MapRGBA(surface->format, (unsigned char)(255.*r), (unsigned char)(255.*g),
						(unsigned char)(255.*b), (unsigned char)(255.*a));

	rect.x = prect->x; rect.y = prect->y;
	rect.w = prect->w; rect.h = prect->h;
	SDL_FillRect(surface, &rect, color);
	return AV_OK;
}

static av_result_t av_video_surface_sdl_fill_rect_rgba(av_video_surface_p self, av_rect_p prect,
												av_pixel_t rgba)
{
	SDL_Rect rect;
	SDL_Surface *surface = (SDL_Surface *)O_context(self);

	if (!surface)
		return AV_EFOUND;

	rect.x = prect->x; rect.y = prect->y;
	rect.w = prect->w; rect.h = prect->h;
	SDL_FillRect(surface, &rect, rgba);
	return AV_OK;
}

static void av_video_surface_sdl_destructor(void* psurface)
{
	av_video_surface_sdl_p self = (av_video_surface_sdl_p)psurface;
	SDL_Surface *surface = (SDL_Surface *)O_context(self);
	if (surface)
	{
		SDL_UnlockSurface(surface);
		SDL_FreeSurface(surface);
	}
}

av_result_t av_video_surface_sdl_constructor(av_object_p object)
{
	av_video_surface_sdl_p self = (av_video_surface_sdl_p)object;

	/* SDL surface is not created until set_size method invoked */
	O_set_attr(self, CONTEXT_SDL_SURFACE, AV_NULL);
	/* registers surface methods ... */
	((av_surface_p)self)->set_size                        = av_surface_sdl_set_size;
	((av_surface_p)self)->get_size                        = av_surface_sdl_get_size;
	((av_surface_p)self)->lock                            = av_surface_sdl_lock;
	((av_surface_p)self)->unlock                          = av_surface_sdl_unlock;
	((av_surface_p)self)->set_clip                        = av_video_surface_sdl_set_clip;
	((av_surface_p)self)->get_clip                        = av_video_surface_sdl_get_clip;
	((av_video_surface_p)self)->fill_rect                 = av_video_surface_sdl_fill_rect;
	((av_video_surface_p)self)->fill_rect_rgba            = av_video_surface_sdl_fill_rect_rgba;
	((av_video_surface_p)self)->blit                      = av_video_surface_sdl_blit;
	((av_video_surface_p)self)->create_overlay            = av_video_surface_sdl_create_overlay;
	((av_video_surface_p)self)->create_overlay_buffered   = av_video_surface_sdl_create_overlay_buffered;
	((av_video_surface_p)self)->set_size                  = av_video_surface_sdl_set_size;
	((av_video_surface_p)self)->get_size                  = av_video_surface_sdl_get_size;
	((av_video_surface_p)self)->get_depth                 = av_video_surface_sdl_get_depth;
	((av_video_surface_p)self)->lock                      = av_video_surface_sdl_lock;
	((av_video_surface_p)self)->unlock                    = av_video_surface_sdl_unlock;

	return AV_OK;
}

/* Registers SDL video surface class into TORBA class repository */
av_result_t av_video_surface_sdl_register_torba(void)
{
	return av_torb_register_class("video_surface_sdl", "video_surface", sizeof(av_video_surface_sdl_t),
								  av_video_surface_sdl_constructor, av_video_surface_sdl_destructor);
}

#endif /* WITH_SYSTEM_SDL */
