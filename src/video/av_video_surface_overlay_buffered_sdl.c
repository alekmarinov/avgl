/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_video_surface_overlay_buffered_sdl.c            */
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

#define av_sdl_error_check(funcname, rc) av_sdl_error_process(rc, funcname, __FILE__, __LINE__)

/*! Blit overlay to parent surface */
static av_result_t av_video_overlay_sdl_blit(struct av_video_overlay* self, av_rect_p dstrect)
{
	av_video_overlay_sdl_p ctx = O_context_overlay(self);
	if (ctx->mtx)
	{
		ctx->mtx->lock(ctx->mtx);
		if (ctx->overlay)
		{
			if (ctx->surface)
			{
				SDL_Rect rect;
				if (dstrect)
				{
					rect.x = dstrect->x;
					rect.y = dstrect->y;
					rect.w = dstrect->w;
					rect.h = dstrect->h;
				}
				else
				{
					rect.x = rect.y = 0;
					rect.w = ctx->surface->w; rect.h = ctx->surface->h;
				}

				/* Blit overlay to backbuffer */
				SDL_DisplayYUVOverlay(ctx->overlay, &rect);
			}
		}
		ctx->mtx->unlock(ctx->mtx);
	}
	return AV_OK;
}

/*! Blit overlay's backbuffer to parent surface
	If dstrect size != back surface size the blit is stretched
*/
static av_result_t av_video_overlay_sdl_blit_back(struct av_video_overlay* self, av_rect_p srcrect, av_rect_p dstrect)
{
	av_video_overlay_sdl_p ctx = O_context_overlay(self);
	if (ctx->mtx)
	{
		ctx->mtx->lock(ctx->mtx);
		if (ctx->surface)
		{
			SDL_Rect rect;
			av_video_surface_p dstsurface;
			if (srcrect)
			{
				rect.x = srcrect->x;
				rect.y = srcrect->y;
				rect.w = srcrect->w;
				rect.h = srcrect->h;
			}
			else
			{
				rect.x = rect.y = 0;
				rect.w = ctx->surface->w; rect.h = ctx->surface->h;
			}
			if (AV_OK == self->video->get_backbuffer(self->video, &dstsurface))
			{
				SDL_Surface* dstbuffer = O_context(dstsurface);
				if (dstbuffer)
				{
					SDL_Rect drect;
					drect.x = dstrect->x; drect.y = dstrect->y;
					drect.w = dstrect->w; drect.h = dstrect->h;
					/* Blit backbuffer to screen */
					SDL_UnlockSurface(ctx->surface);
					SDL_BlitSurface(ctx->surface, &rect, dstbuffer, &drect);
					SDL_LockSurface(ctx->surface);
					/* swap destination size - just for fun ... */
					av_rect_copy(&ctx->dstrect, dstrect);
				}
			}
		}
		ctx->mtx->unlock(ctx->mtx);
	}
	return AV_OK;
}

/*! Blit to overlay's backbuffer */
static av_result_t av_video_overlay_sdl_blit_back_to(struct av_video_overlay* self, av_rect_p dstrect, void* rgba_data, av_rect_p srcrect)
{
#if 0
	av_video_overlay_sdl_p ctx = O_context_overlay(self);
	if (ctx->mtx)
	{
		if (ctx->surface)
		{
			SDL_Rect sr;
			SDL_Rect dr;
			SDL_Surface srcbuffer;
			ctx->mtx->lock(ctx->mtx);

			srcbuffer.flags = SDL_SWSURFACE;
			srcbuffer.format = ctx->surface->format;
			srcbuffer.w = srcrect->w;
			srcbuffer.h = srcrect->h;
			srcbuffer.pitch = srcrect->w * 4; /* FIXME: clean up hardcoded value */
			srcbuffer.pixels = rgba_data;
			srcbuffer.clip_rect.x = sr.x = srcrect->x;
			srcbuffer.clip_rect.y = sr.y = srcrect->y;
			srcbuffer.clip_rect.w = sr.w = srcrect->w;
			srcbuffer.clip_rect.h = sr.h = srcrect->h;
			srcbuffer.refcount = 0;

			dr.x = dstrect->x;
			dr.y = dstrect->y;
			dr.w = dstrect->w;
			dr.h = dstrect->h;

/* 			SDL_BlitSurface(&srcbuffer, &sr, ctx->surface, &dr);*/

			ctx->mtx->unlock(ctx->mtx);
		}
	}
#else
	AV_UNUSED(self);
	AV_UNUSED(dstrect);
	AV_UNUSED(srcrect);
	AV_UNUSED(rgba_data);
#endif
	return AV_OK;
}

/*! Set overlay width and height */
static av_result_t av_video_overlay_sdl_set_size_format(struct av_video_overlay* self, int width, int height,
														 av_video_overlay_format_t format)
{
	av_video_overlay_sdl_p ctx = O_context_overlay(self);
	if (ctx->mtx)
	{
		ctx->mtx->lock(ctx->mtx);
		if (ctx->overlay)
		{
			if (ctx->w==width && ctx->h==height && ctx->overlay->format==format)
			{
				ctx->mtx->unlock(ctx->mtx);
				return AV_OK;
			}
			SDL_UnlockYUVOverlay(ctx->overlay);
			SDL_FreeYUVOverlay(ctx->overlay);
			ctx->overlay = AV_NULL;
			ctx->w = 0;
			ctx->h = 0;
		}

		if (!ctx->surface)
		{
			if (AV_NULL == (ctx->surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
																width, height, SDL_SURFACE_BPP,
																SDL_SURFACE_MASK_RED, SDL_SURFACE_MASK_GREEN,
																SDL_SURFACE_MASK_BLUE, 0)))
			{
				ctx->mtx->unlock(ctx->mtx);
				return AV_EMEM;
			}
			SDL_LockSurface(ctx->surface);
		}

		ctx->w = width;
		ctx->h = height;
		width = (width+3)&(~3);
		height = (height+3)&(~3);
		if (AV_NULL == (ctx->overlay = SDL_CreateYUVOverlay(width, height, format, ctx->surface)))
		{
			ctx->mtx->unlock(ctx->mtx);
			return AV_EMEM;
		}
		SDL_LockYUVOverlay(ctx->overlay);
		ctx->mtx->unlock(ctx->mtx);
	}
	return AV_OK;
}

/*! Set overlay's backbuffer width and height */
static av_result_t av_video_overlay_sdl_set_size_back(struct av_video_overlay* self, int back_width, int back_height)
{
	av_video_overlay_sdl_p ctx = O_context_overlay(self);

	if (ctx->mtx)
	{
		ctx->mtx->lock(ctx->mtx);
		if (ctx->surface)
		{
			if (ctx->surface->w==back_width && ctx->surface->h==back_height)
			{
				ctx->mtx->unlock(ctx->mtx);
				return AV_OK;
			}
			if (ctx->overlay)
			{
				SDL_UnlockYUVOverlay(ctx->overlay);
				SDL_FreeYUVOverlay(ctx->overlay);
				ctx->overlay = AV_NULL;
				ctx->w = 0;
				ctx->h = 0;
			}
			SDL_UnlockSurface(ctx->surface);
			SDL_FreeSurface(ctx->surface);
			ctx->surface = AV_NULL;
		}

		if (AV_NULL == (ctx->surface = SDL_CreateRGBSurface(SDL_SURFACE_TYPE,
															back_width, back_height, SDL_SURFACE_BPP,
															SDL_SURFACE_MASK_RED, SDL_SURFACE_MASK_GREEN,
															SDL_SURFACE_MASK_BLUE, 0)))
		{
			ctx->mtx->unlock(ctx->mtx);
			return AV_EMEM;
		}
		SDL_LockSurface(ctx->surface);
		ctx->mtx->unlock(ctx->mtx);
	}
	return AV_OK;
}

/*! Get overlay width and height */
static av_result_t av_video_overlay_sdl_get_size_format(struct av_video_overlay* self, int* pwidth, int* pheight,
														int* pback_width, int* pback_height, av_video_overlay_format_p pformat)
{
	av_video_overlay_sdl_p ctx = O_context_overlay(self);

	if (ctx->overlay)
	{
		*pwidth = ctx->overlay->w;
		*pheight = ctx->overlay->h;
		*pformat = ctx->overlay->format;
	}
	else
		*pwidth = *pheight = *pformat = 0;

	if (ctx->surface)
	{
		*pback_width = ctx->surface->w;
		*pback_height = ctx->surface->h;
	}
	else
		*pback_width = *pback_height = 0;

	return AV_OK;
}

/*! Validate overlay */
static av_bool_t av_video_overlay_sdl_validate(struct av_video_overlay* self)
{
	av_video_overlay_sdl_p ctx = O_context_overlay(self);
	return ((ctx->overlay)? AV_TRUE:AV_FALSE);
}

/*! Validate current sizes */
static av_bool_t av_video_overlay_sdl_validate_size(struct av_video_overlay* self, int width, int height)
{
	av_video_overlay_sdl_p ctx = O_context_overlay(self);
	return (ctx->overlay && ctx->w == width && ctx->h == height)? AV_TRUE:AV_FALSE;
}

/*! Locks overlay for direct memory access */
static av_result_t av_video_overlay_sdl_lock(struct av_video_overlay* self,
					     int* pplanes,
					     const av_pix_p** ppixels,
					     const short int** ppitch)
{
	av_video_overlay_sdl_p ctx = O_context_overlay(self);
	if (ctx->mtx)
	{
		ctx->mtx->lock(ctx->mtx);
		if (ctx->overlay)
		{
			*pplanes = ctx->overlay->planes;
			*ppixels = (const av_pix_p* )ctx->overlay->pixels;
			*ppitch = (const short int*)ctx->overlay->pitches;
			return AV_OK;
		}
	}
	return AV_EFOUND;
}

/*! Unlocks locked overlay */
static av_result_t av_video_overlay_sdl_unlock(struct av_video_overlay* self)
{
	av_video_overlay_sdl_p ctx = O_context_overlay(self);
	if (ctx->mtx)
	{
		if (ctx->overlay)
		{
			ctx->mtx->unlock(ctx->mtx);
			return AV_OK;
		}
		ctx->mtx->unlock(ctx->mtx);
	}
	return AV_EFOUND;
}

static av_result_t av_video_overlay_sdl_is_hardware(struct av_video_overlay* self, av_bool_t* hardware)
{
	av_video_overlay_sdl_p ctx = O_context_overlay(self);
	if (ctx)
	{
		if (ctx->overlay)
		{
			*hardware = (0 != ctx->overlay->hw_overlay)? AV_TRUE : AV_FALSE;
			return AV_OK;
		}
	}
	return AV_EFOUND;
}

static void av_video_overlay_sdl_destroy(void* object)
{
	av_video_overlay_p self = (av_video_overlay_p)object;
	av_video_overlay_sdl_p ctx = O_context_overlay(self);
	if (ctx)
	{
		if (ctx->overlay)
		{
			SDL_UnlockYUVOverlay(ctx->overlay);
			SDL_FreeYUVOverlay(ctx->overlay);
			ctx->overlay = AV_NULL;
		}
		if (ctx->surface)
		{
			SDL_UnlockSurface(ctx->surface);
			SDL_FreeSurface(ctx->surface);
			ctx->surface = AV_NULL;
		}
		if (ctx->mtx)
		{
			ctx->mtx->destroy(ctx->mtx);
			ctx->mtx = AV_NULL;
		}
		free(ctx);
		ctx = AV_NULL;
	}
}

av_result_t av_video_surface_sdl_create_overlay_buffered(av_video_surface_p psurface, av_video_overlay_p* ppoverlay)
{
	av_video_overlay_p self;
	av_video_overlay_sdl_p ctx;

	ctx = (av_video_overlay_sdl_p)malloc(sizeof(av_video_overlay_sdl_t));
	if (!ctx)
	{
		return AV_EMEM;
	}

	self = (av_video_overlay_p)malloc(sizeof(av_video_overlay_t));
	if (!self)
	{
		free(ctx);
		return AV_EMEM;
	}

	memcpy(self, psurface, sizeof(av_object_t));
	((av_object_p)self)->destroy = av_video_overlay_sdl_destroy;

	ctx->mtx = AV_NULL;
	av_mutex_create(&ctx->mtx);
	ctx->surface = AV_NULL;
	ctx->overlay = AV_NULL;
	ctx->w = 0;
	ctx->h = 0;
	O_set_attr(self, CONTEXT_SDL_OVERLAY, ctx);

	self->video            = psurface->video;
	self->blit             = av_video_overlay_sdl_blit;
	self->blit_back        = av_video_overlay_sdl_blit_back;
	self->blit_back_to     = av_video_overlay_sdl_blit_back_to;
	self->set_size_back    = av_video_overlay_sdl_set_size_back;
	self->set_size_format  = av_video_overlay_sdl_set_size_format;
	self->get_size_format  = av_video_overlay_sdl_get_size_format;
	self->validate         = av_video_overlay_sdl_validate;
	self->validate_size    = av_video_overlay_sdl_validate_size;
	self->lock             = av_video_overlay_sdl_lock;
	self->unlock           = av_video_overlay_sdl_unlock;
	self->is_hardware      = av_video_overlay_sdl_is_hardware;

	*ppoverlay = self;
	return AV_OK;
}

#endif /* WITH_SYSTEM_SDL */
