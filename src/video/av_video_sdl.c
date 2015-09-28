/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_video_sdl.c                                     */
/* Description:   SDL video implementation                           */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_SDL
/* SDL video backend implementation */

#include <av_video.h>

#include "av_video_sdl.h"
#include "av_video_cursor_sdl.h"

#define O_context(o) O_attr(o, CONTEXT_SDL_SURFACE)

/* exported registration method */
AV_API av_result_t av_video_sdl_register_torba(void);

/* import sdl error processor */
av_result_t av_sdl_error_process(int, const char*, const char*, int);

/* import video_surface_sdl registration method */
av_result_t av_video_surface_sdl_register_torba(void);

/* import video_surface_sdl constructor */
av_result_t av_video_surface_sdl_constructor(av_object_p object);


#define av_sdl_error_check(funcname, rc) av_sdl_error_process(rc, funcname, __FILE__, __LINE__)

/* Cleans up SDL objects */
static void av_video_sdl_clean_up(av_video_sdl_p self)
{
	SDL_Surface *primary_surface = (SDL_Surface *)O_context(self);

	if (primary_surface)
		SDL_FreeSurface(primary_surface);
	O_set_attr(self, CONTEXT_SDL_SURFACE, AV_NULL);
}

static av_result_t av_video_sdl_set_size(av_surface_p pvideo, int width, int height)
{
	av_video_p self = (av_video_p)pvideo;
	av_video_config_t video_config;
	video_config.flags = AV_VIDEO_CONFIG_SIZE;
	video_config.width = width;
	video_config.height = height;
	return self->set_configuration(self, &video_config);
}

static av_result_t av_video_sdl_get_size(av_surface_p pvideo, int* pwidth, int* pheight)
{
	av_result_t rc;
	av_video_config_t video_config;
	av_video_p self = (av_video_p)pvideo;
	video_config.flags = AV_VIDEO_CONFIG_SIZE;
	if (AV_OK != (rc = self->get_configuration(self, &video_config)))
		return rc;
	*pwidth = video_config.width;
	*pheight = video_config.height;
	return AV_OK;
}

/* compares two video configurations, and returns AV_TRUE if they are equal, AV_FALSE otherwise */
static av_bool_t av_video_sdl_compare_configuration(av_video_config_flags_t flags, av_video_config_p config1, av_video_config_p config2)
{
	if (flags & AV_VIDEO_CONFIG_MODE)
		if (config1->mode != config2->mode)
			return AV_FALSE;

	if (flags & AV_VIDEO_CONFIG_SIZE)
		if (config1->width != config2->width || config1->height != config2->height)
			return AV_FALSE;

	return AV_TRUE;
}

static av_result_t av_video_sdl_enum_video_modes(av_video_p pvideo, video_mode_callback_t vmcbk)
{
	AV_UNUSED(pvideo);
	AV_UNUSED(vmcbk);
/*	av_video_sdl_p self = (av_video_sdl_p)pvideo;
	SDL_PixelFormat format;
	format.palette = 0;
	BitsPerPixel = 16;
	BytesPerPixel = 2;
	Uint8  Rloss;
	Uint8  Gloss;
	Uint8  Bloss;
	Uint8  Aloss;
	Uint8  Rshift;
	Uint8  Gshift;
	Uint8  Bshift;
	Uint8  Ashift;
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;
	Uint32 colorkey;
	Uint8  alpha;
*/
	return AV_OK;
}

/*
*	Sets video configuration options.
*	After setting configuration the \c video_config parameter is filled
*	with the new successfully applied settings
*/
static av_result_t av_video_sdl_set_configuration(av_video_p pvideo, av_video_config_p new_video_config)
{
	int bpp;
	av_video_sdl_p self = (av_video_sdl_p)pvideo;
	Uint32 sdlflags = SDL_SURFACE_TYPE | /*SDL_ASYNCBLIT |*/ SDL_HWACCEL;
	SDL_Surface *surface;
	av_video_config_t video_config;

	if (new_video_config->flags & AV_VIDEO_CONFIG_BPP)
	{
		/* changing bpp is not allowed */
		return AV_EPERM;
	}

	sdlflags |= (is_sdl_double_buffer()? SDL_DOUBLEBUF : 0);

	/* gets previous video mode settings */
	video_config.flags = new_video_config->flags;
	pvideo->get_configuration(pvideo, &video_config);

	if (av_video_sdl_compare_configuration(video_config.flags, &video_config, new_video_config))
		return AV_OK; /* nothing to configure */

	video_config.flags |= AV_VIDEO_CONFIG_MODE | AV_VIDEO_CONFIG_SIZE;
	pvideo->get_configuration(pvideo, &video_config);

	/* apply only the changed mode settings */

	/* video mode */
	if (new_video_config->flags & AV_VIDEO_CONFIG_MODE)
		video_config.mode = new_video_config->mode;

	/* video resolution */
	if (new_video_config->flags & AV_VIDEO_CONFIG_SIZE)
	{
		video_config.width = new_video_config->width;
		video_config.height = new_video_config->height;
	}

	av_video_sdl_clean_up(self);

	if (video_config.mode & AV_VIDEO_MODE_FULLSCREEN)
	{
		sdlflags |= SDL_FULLSCREEN;
	}

	if (0 == (bpp = SDL_VideoModeOK(video_config.width, video_config.height, SDL_SURFACE_BPP, sdlflags)))
	{
		av_video_sdl_clean_up(self);
		return AV_ESUPPORTED;
	}

	surface = SDL_SetVideoMode(video_config.width, video_config.height, bpp, sdlflags);
	if (!surface)
	{
		av_video_sdl_clean_up(self);
		return AV_ESUPPORTED;
	}
	O_set_attr(self, CONTEXT_SDL_SURFACE, surface);

	av_video_cursor_sdl_init();
	pvideo->set_cursor_shape(pvideo, AV_VIDEO_CURSOR_DEFAULT);

	return AV_OK;
}

/* Gets video configuration options */
static av_result_t av_video_sdl_get_configuration(av_video_p pvideo, av_video_config_p video_config)
{
	av_video_sdl_p self = (av_video_sdl_p)pvideo;
	SDL_Surface *primary_surface = (SDL_Surface *)O_context(self);

	if (primary_surface)
	{
		if (video_config->flags & AV_VIDEO_CONFIG_SIZE)
		{
			video_config->width = primary_surface->w;
			video_config->height = primary_surface->h;
		}

		if (video_config->flags & AV_VIDEO_CONFIG_BPP)
		{
			video_config->bpp = primary_surface->format->BitsPerPixel;
		}

		if (video_config->flags & AV_VIDEO_CONFIG_MODE)
		{
			if (primary_surface->flags & SDL_FULLSCREEN)
			{
				video_config->mode = AV_VIDEO_MODE_FULLSCREEN;
			}
			else
			{
				video_config->mode = AV_VIDEO_MODE_WINDOWED;
			}
		}
	}
	else
	{
		if (video_config->flags & AV_VIDEO_CONFIG_SIZE)
		{
			video_config->width = 0;
			video_config->height = 0;
		}

		if (video_config->flags & AV_VIDEO_CONFIG_BPP)
		{
			video_config->bpp = 0;
		}

		if (video_config->flags & AV_VIDEO_CONFIG_MODE)
		{
			video_config->mode = AV_VIDEO_MODE_WINDOWED;
		}
	}
	return AV_OK;
}

static av_result_t av_video_sdl_create_surface(av_video_p self, int width, int height, av_video_surface_p* ppsurface)
{
	av_result_t rc;
	av_video_surface_p surface;
	if (AV_OK != (rc = av_torb_create_object("video_surface_sdl", (av_object_p*)&surface)))
	{
		return rc;
	}

	/* set_size will expect a reference to the video owner object */
	surface->video = self;
	if (0<width && 0<height)
	{
		if (AV_OK != (rc = ((av_surface_p)surface)->set_size((av_surface_p)surface, width, height)))
		{
			O_destroy(surface);
			return rc;
		}
	}
	*ppsurface = surface;
	return AV_OK;
}

static av_result_t av_video_sdl_create_overlay(av_video_p self, av_video_overlay_p* ppoverlay)
{
	return ((av_video_surface_p)self)->create_overlay((av_video_surface_p)self, ppoverlay);
}

static av_result_t av_video_sdl_create_overlay_buffered(av_video_p self, av_video_overlay_p* ppoverlay)
{
	return ((av_video_surface_p)self)->create_overlay_buffered((av_video_surface_p)self, ppoverlay);
}

static av_result_t av_video_sdl_create_surface_from(av_video_p self, void* ptr, int width, int height,
													int pitch, av_video_surface_p* ppsurface)
{
	av_result_t rc;
	av_video_surface_p surface;
	SDL_Surface *sdl_surface;

	AV_UNUSED(self);

	if (AV_OK != (rc = av_torb_create_object("video_surface_sdl", (av_object_p*)&surface)))
	{
		return rc;
	}

	/* creates 32bit SDL surface */
	if (AV_NULL == (sdl_surface = SDL_CreateRGBSurfaceFrom(ptr, width, height, SDL_SURFACE_BPP, pitch,
														   SDL_SURFACE_MASK_RED, SDL_SURFACE_MASK_GREEN,
														   SDL_SURFACE_MASK_BLUE, 0)))
	{
		O_destroy(surface);
		return AV_EMEM;
	}
	SDL_LockSurface(sdl_surface);

	O_set_attr(surface, CONTEXT_SDL_SURFACE, sdl_surface);
	*ppsurface = surface;
	return AV_OK;
}

static av_result_t av_video_sdl_get_backbuffer(av_video_p self, av_video_surface_p* ppsurface)
{
	*ppsurface = (av_video_surface_p)self;
	return AV_OK;
}

static av_result_t av_video_sdl_update(av_video_p self, av_rect_p rect)
{
	SDL_Surface *surface = (SDL_Surface *)O_context(self);
	av_assert(surface, "Video surface is not initialized properly");
	if (is_sdl_double_buffer())
	{
		SDL_Flip(surface);
	}
	else
	{
		if (rect)
		{
			int sw, sh;
			int x = rect->x, y = rect->y, w = rect->w, h = rect->h;
			((av_surface_p)self)->get_size((av_surface_p)self, &sw, &sh);
			if (x < 0)
			{
				w += x;
				x = 0;
			}

			if (y < 0)
			{
				h += y;
				y = 0;
			}

			if (x + w > sw)
				w = sw - x;

			if (y + h > sh)
				h = sh - y;

			SDL_UpdateRect(surface, x, y, w, h);
		}
		else
			SDL_UpdateRect(surface, 0, 0, 0, 0);
	}
	return AV_OK;
}

static void av_video_sdl_set_capture(av_video_p self, av_bool_t iscaptured)
{
	AV_UNUSED(self);
	if (iscaptured)
			SDL_WM_GrabInput(SDL_GRAB_ON);
	else
			SDL_WM_GrabInput(SDL_GRAB_OFF);
}

static void av_video_sdl_set_cursor_visible(av_video_p self, av_bool_t visible)
{
	AV_UNUSED(self);
	SDL_ShowCursor(visible);
}

static av_bool_t av_video_sdl_is_cursor_visible(av_video_p self)
{
	AV_UNUSED(self);
	return SDL_ShowCursor(-1);
}

static av_result_t av_video_sdl_set_cursor_shape(av_video_p self, av_video_cursor_shape_t shape)
{
	AV_UNUSED(self);
	return av_video_cursor_sdl_set_shape(shape);
}

static void av_video_sdl_set_mouse_position(av_video_p self, int mx, int my)
{
	AV_UNUSED(self);
	SDL_WarpMouse(mx, my);
}

static void av_video_sdl_get_mouse_position(av_video_p self, int* pmx, int* pmy)
{
	AV_UNUSED(self);
	SDL_GetMouseState(pmx, pmy);
}

static av_result_t av_video_sdl_get_root_window(struct av_video* self, void** pdisplay, void** pwindow)
{
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (SDL_GetWMInfo(&wmInfo))
	{
		/* FIXME: X11 depend code */
#ifdef WITH_X11
		*pdisplay = (void**)wmInfo.info.x11.display;
		*pwindow = (void**)wmInfo.info.x11.window;
#else
		/* assumming windows */
		*pdisplay = AV_NULL;
		*pwindow = (void**)wmInfo.window;
#endif
		return AV_OK;
	}
	AV_UNUSED(self);
	return AV_ESUPPORTED;
}

static av_result_t av_video_sdl_get_color_key(struct av_video* self, av_pixel_t* pcolorkey)
{
	AV_UNUSED(self);
	*pcolorkey = color_key();
	return AV_OK;
}

static void av_video_sdl_destructor(void* pvideo)
{
	av_video_sdl_p self = (av_video_sdl_p)pvideo;
	/* clean up SDL objects */
	av_video_sdl_clean_up(self);
	av_video_cursor_sdl_done();
}

/* Initializes memory given by the input pointer with the video sdl class information */
static av_result_t av_video_sdl_constructor(av_object_p object)
{
	av_result_t rc;
	av_video_sdl_p self = (av_video_sdl_p)object;

	if (AV_OK != (rc = av_video_surface_sdl_constructor(object)))
		return rc;
	/* context with the primary surface is not initialized until set_size */
	O_set_attr(self, CONTEXT_SDL_SURFACE, AV_NULL);
	/* override video methods */
	((av_surface_p)self)->set_size                = av_video_sdl_set_size;
	((av_surface_p)self)->get_size                = av_video_sdl_get_size;
	((av_video_p)self)->enum_video_modes          = av_video_sdl_enum_video_modes;
	((av_video_p)self)->set_configuration         = av_video_sdl_set_configuration;
	((av_video_p)self)->get_configuration         = av_video_sdl_get_configuration;
	((av_video_p)self)->create_surface            = av_video_sdl_create_surface;
	((av_video_p)self)->create_overlay            = av_video_sdl_create_overlay;
	((av_video_p)self)->create_overlay_buffered   = av_video_sdl_create_overlay_buffered;
	((av_video_p)self)->create_surface_from       = av_video_sdl_create_surface_from;
	((av_video_p)self)->get_backbuffer            = av_video_sdl_get_backbuffer;
	((av_video_p)self)->update                    = av_video_sdl_update;
	((av_video_p)self)->set_capture               = av_video_sdl_set_capture;
	((av_video_p)self)->set_cursor_visible        = av_video_sdl_set_cursor_visible;
	((av_video_p)self)->is_cursor_visible         = av_video_sdl_is_cursor_visible;
	((av_video_p)self)->set_cursor_shape          = av_video_sdl_set_cursor_shape;
	((av_video_p)self)->set_mouse_position        = av_video_sdl_set_mouse_position;
	((av_video_p)self)->get_mouse_position        = av_video_sdl_get_mouse_position;
	((av_video_p)self)->get_root_window           = av_video_sdl_get_root_window;
	((av_video_p)self)->get_color_key             = av_video_sdl_get_color_key;
	/* initialize video surface objects */
	((av_video_surface_p)self)->video             = (av_video_p)self;

	return AV_OK;
}

/* Registers SDL video class into TORBA class repository */
AV_API av_result_t av_video_sdl_register_torba(void)
{
	av_result_t rc;
	if (AV_OK != (rc = av_video_surface_sdl_register_torba()))
		return rc;
	return av_torb_register_class("video_sdl", "video", sizeof(av_video_sdl_t), av_video_sdl_constructor, av_video_sdl_destructor);
}

#endif /* WITH_SYSTEM_SDL */
