/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_surface_sdl.c                                   */
/* Description:   SDL surface implementation of bitmap class         */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_SDL

#include <av_surface.h>
#include <av_stdc.h>
#include "av_surface_sdl.h"
#include "av_core_sdl.h"
#include "av_bitmap_sdl.h"
#include "av_debug.h"
#include <SDL.h>

/* set surface width and height */
static av_result_t av_surface_sdl_set_size(av_surface_p self, int width, int height)
{
	surface_sdl_ctx_p ctx = O_surface_context(self);
	if (ctx->texture)
		SDL_DestroyTexture(ctx->texture);

	ctx->texture = SDL_CreateTexture(ctx->display->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (!ctx->texture)
		return AV_EMEM;

	return AV_OK;
}

/* get bitmap width and height */
static av_result_t av_surface_sdl_get_size(av_surface_p self, int* pwidth, int* pheight)
{
	surface_sdl_ctx_p ctx = O_surface_context(self);
	if (SDL_QueryTexture)
	{
		int dummy;
		SDL_QueryTexture(ctx->texture, &dummy, &dummy, pwidth, pheight);
		return AV_OK;
	}
	return AV_ESTATE;
}

static av_result_t av_surface_sdl_lock(av_surface_p self, av_pixel_p* ppixels, int* ppitch)
{
	surface_sdl_ctx_p ctx = O_surface_context(self);
	if (0 != SDL_LockTexture(ctx->texture, NULL, ppixels, ppitch))
		return AV_ESTATE;

	return AV_OK;
}

static void av_surface_sdl_unlock(av_surface_p self)
{
	surface_sdl_ctx_p ctx = O_surface_context(self);
	SDL_UnlockTexture(ctx->texture);
}

static av_result_t av_surface_sdl_render(av_surface_p self, av_rect_p src_rect, av_rect_p dst_rect)
{
	surface_sdl_ctx_p ctx = O_surface_context(self);
	if (0 != SDL_RenderCopy(ctx->display->renderer, ctx->texture, (SDL_Rect*)src_rect, (SDL_Rect*)dst_rect))
	{
		// FIXME: Verify SDL error
		return AV_EGENERAL;
	}
//	av_dbg("SDL_RenderCopy s:(%d %d %d %d) d:(%d %d %d %d)\n", src_rect->x, src_rect->y, src_rect->w, src_rect->h, dst_rect->x, dst_rect->y, dst_rect->w, dst_rect->h);
	return AV_OK;
}

static void av_surface_sdl_destructor(av_object_p object)
{
	surface_sdl_ctx_p ctx = O_surface_context(object);
	free(ctx);
}

static av_result_t av_surface_sdl_set_bitmap(av_surface_p self, av_bitmap_p bitmap)
{
	surface_sdl_ctx_p ctx = O_surface_context(self);
	bitmap_sdl_ctx_p bmpctx = O_bitmap_context(bitmap);
	SDL_Texture* new_texture = SDL_CreateTextureFromSurface(ctx->display->renderer, bmpctx->surface);
	if (!new_texture)
		return av_sdl_error_check("SDL_CreateTextureFromSurface", -1);
	if (ctx->texture)
		SDL_DestroyTexture(ctx->texture);
	ctx->texture = new_texture;
	return AV_OK;
}

/* Initializes memory given by the input pointer with the bitmap's class information */
static av_result_t av_surface_sdl_constructor(av_object_p object)
{
	av_surface_p self = (av_surface_p)object;
	av_oop_p oop = O_oop(object);
	surface_sdl_ctx_p ctx = (surface_sdl_ctx_p)av_calloc(1, sizeof(surface_sdl_ctx_t));
	O_set_attr(object, surface_context, ctx);

	self->lock        = av_surface_sdl_lock;
	self->unlock      = av_surface_sdl_unlock;
	self->set_size    = av_surface_sdl_set_size;
	self->get_size    = av_surface_sdl_get_size;
	self->set_bitmap  = av_surface_sdl_set_bitmap;
	self->render      = av_surface_sdl_render;
	return AV_OK;
}

/*	Registers surface class into OOP class repository */
av_result_t av_surface_sdl_register_oop(av_oop_p oop)
{
	return oop->define_class(oop, "surface_sdl", "surface", sizeof(av_surface_t), av_surface_sdl_constructor, av_surface_sdl_destructor);
}

#endif /* WITH_SYSTEM_SDL */
