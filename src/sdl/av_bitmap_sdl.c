/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_bitmap_sdl.c                                    */
/* Description:   SDL surface implementation of bitmap class         */
/*                                                                   */
/*********************************************************************/

#include <av_bitmap.h>
#include "av_bitmap_sdl.h"
#include "av_core_sdl.h"

/* SDL interprets each pixel as a 32-bit number, so our masks must depend
on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#  define RMASK 0xff000000
#  define GMASK 0x00ff000
#  define BMASK 0x0000ff00
#  define AMASK 0x000000ff
#else
#  define RMASK 0x000000ff
#  define GMASK 0x0000ff00
#  define BMASK 0x00ff0000
#  define AMASK 0xff000000
#endif

/* set bitmap width and height */
static av_result_t av_bitmap_sdl_set_size(struct _av_bitmap_t* self, int width, int height)
{
	bitmap_sdl_ctx_p ctx = O_bitmap_context(self);
	if (ctx->surface)
		SDL_FreeSurface(ctx->surface);
	ctx->surface = SDL_CreateRGBSurface(0, width, height, 32,
		RMASK, GMASK, BMASK, AMASK);

	return AV_OK;
}

/* get bitmap width and height */
static av_result_t av_bitmap_sdl_get_size(struct _av_bitmap_t* self, int* pwidth, int* pheight)
{
	bitmap_sdl_ctx_p ctx = O_bitmap_context(self);
	if (ctx->surface)
	{
		*pwidth = ctx->surface->w;
		*pheight = ctx->surface->h;
		return AV_OK;
	}
	return AV_ESTATE;
}

static av_result_t av_bitmap_sdl_load(struct _av_bitmap_t* object, const char* filename)
{
	bitmap_sdl_ctx_p ctx = O_bitmap_context(object);
	if (ctx->surface)
		SDL_FreeSurface(ctx->surface);
	ctx->surface = SDL_LoadBMP(filename);
	if (!ctx->surface)
		return av_sdl_error_check("SDL_LoadBMP", -1);
	return AV_OK;
}

static void av_bitmap_sdl_destructor(av_object_p object)
{
	bitmap_sdl_ctx_p ctx = O_bitmap_context(object);
	if (ctx->surface)
		SDL_FreeSurface(ctx->surface);
}

/* Initializes memory given by the input pointer with the bitmap's class information */
static av_result_t av_bitmap_sdl_constructor(av_object_p object)
{
	av_bitmap_p self = (av_bitmap_p)object;
	self->set_size    = av_bitmap_sdl_set_size;
	self->get_size    = av_bitmap_sdl_get_size;
	self->load        = av_bitmap_sdl_load;
	return AV_OK;
}

/*	Registers bitmap class into OOP class repository */
av_result_t av_bitmap_sdl_register_oop(av_oop_p oop)
{
	return oop->define_class(oop, "bitmap_sdl", "bitmap", sizeof(av_bitmap_t), av_bitmap_sdl_constructor, av_bitmap_sdl_destructor);
}
