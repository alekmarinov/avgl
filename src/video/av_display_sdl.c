/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_display_sdl.c                                     */
/* Description:   SDL display implementation                           */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_SDL
/* SDL display backend implementation */

#include <av_display.h>
#include "av_display_sdl.h"
#include "av_display_cursor_sdl.h"
#include <SDL_mouse.h>

static av_result_t av_display_sdl_set_size(av_surface_p pdisplay, int width, int height)
{
	av_result_t rc;
	av_display_p self = (av_display_p)pdisplay;
	av_display_config_t display_config;
	if (AV_OK != (rc = self->get_configuration(self, &display_config)))
		return rc;
	display_config.width = width;
	display_config.height = height;
	return self->set_configuration(self, &display_config);
}

static av_result_t av_display_sdl_get_size(av_surface_p pdisplay, int* pwidth, int* pheight)
{
	av_result_t rc;
	av_display_config_t display_config;
	av_display_p self = (av_display_p)pdisplay;
	if (AV_OK != (rc = self->get_configuration(self, &display_config)))
		return rc;
	*pwidth = display_config.width;
	*pheight = display_config.height;
	return AV_OK;
}

/* compares two display configurations, and returns AV_TRUE if they are equal, AV_FALSE otherwise */
static av_bool_t av_display_config_compare(av_display_config_p config1, av_display_config_p config2)
{
	return config1->mode == config2->mode && config1->width == config2->width && config1->height == config2->height;
}

static av_result_t av_display_sdl_enum_display_modes(av_display_p pdisplay, display_mode_callback_t vmcbk)
{
	AV_UNUSED(pdisplay);
	AV_UNUSED(vmcbk);
	return AV_ESUPPORTED;
}

/*
*	Sets display configuration options.
*	After setting configuration the \c display_config parameter is filled
*	with the new successfully applied settings
*/
static av_result_t av_display_sdl_set_configuration(av_display_p pdisplay, av_display_config_p new_display_config)
{
	av_result_t rc;
	av_display_sdl_p self = (av_display_sdl_p)pdisplay;
	Uint32 sdlflags = 0;
	av_display_config_t display_config;

	if (AV_OK != (rc = pdisplay->get_configuration(pdisplay, &display_config)))
		return rc;

	if (av_display_config_compare(&display_config, new_display_config))
		return AV_OK; /* nothing to configure */

	/* display mode */
	display_config.mode = new_display_config->mode;

	if (display_config.mode & AV_DISPLAY_MODE_FULLSCREEN)
		sdlflags |= SDL_WINDOW_FULLSCREEN;

	/* display resolution */
	display_config.width = new_display_config->width;
	display_config.height = new_display_config->height;

	if (!self->window)
	{
		//extern DECLSPEC int SDLCALL SDL_CreateWindowAndRenderer(int width, int height, Uint32 window_flags,SDL_Window **window, SDL_Renderer **renderer);

		if (0 > SDL_CreateWindowAndRenderer(display_config.width, display_config.height, sdlflags, &self->window, &self->renderer))
		{
			// FIXME: provide logging
			printf("%s\n", SDL_GetError());
		}
	}

	return AV_OK;
}

/* Gets display configuration options */
static av_result_t av_display_sdl_get_configuration(av_display_p pdisplay, av_display_config_p display_config)
{
	av_display_sdl_p self = (av_display_sdl_p)pdisplay;
	if (self->window)
	{
		Uint32 flags = SDL_GetWindowFlags(self->window);
		SDL_GetWindowSize(self->window, &display_config->width, &display_config->height);
		display_config->mode = 0;
		if (flags & SDL_WINDOW_FULLSCREEN)
			display_config->mode |= AV_DISPLAY_MODE_FULLSCREEN;
	}
	else
	{
		display_config->width = 0;
		display_config->height = 0;
		display_config->mode = 0;
	}
	return AV_OK;
}

/*
static av_result_t av_display_sdl_create_surface_from(av_display_p self, void* ptr, int width, int height,
													int pitch, av_display_surface_p* ppsurface)
{
	av_result_t rc;
	av_display_surface_p surface;
	SDL_Surface *sdl_surface;

	AV_UNUSED(self);

	if (AV_OK != (rc = av_torb_create_object("display_surface_sdl", (av_object_p*)&surface)))
	{
		return rc;
	}

	if (AV_NULL == (sdl_surface = SDL_CreateRGBSurfaceFrom(ptr, width, height, SDL_MEM_SURFACE_BPP, pitch,
														   SDL_MEM_SURFACE_MASK_RED, SDL_MEM_SURFACE_MASK_GREEN, SDL_MEM_SURFACE_MASK_BLUE, 0)))
	{
		O_destroy(surface);
		return AV_EMEM;
	}
	SDL_LockSurface(sdl_surface);

	O_set_attr(surface, CONTEXT_SDL_SURFACE, sdl_surface);
	*ppsurface = surface;
	return AV_OK;
}
*/

static void av_display_sdl_set_capture(av_display_p self, av_bool_t iscaptured)
{
	AV_UNUSED(self);
	SDL_CaptureMouse(iscaptured);
}

static void av_display_sdl_set_cursor_visible(av_display_p self, av_bool_t visible)
{
	AV_UNUSED(self);
	SDL_ShowCursor(visible);
}

static av_bool_t av_display_sdl_is_cursor_visible(av_display_p self)
{
	AV_UNUSED(self);
	return SDL_ShowCursor(-1);
}

static av_result_t av_display_sdl_set_cursor_shape(av_display_p self, av_display_cursor_shape_t shape)
{
	AV_UNUSED(self);
	// FIXME: return av_display_cursor_sdl_set_shape(shape);
	return AV_OK;
}

static void av_display_sdl_set_mouse_position(av_display_p self, int mx, int my)
{
	AV_UNUSED(self);
	SDL_WarpMouseInWindow(((av_display_sdl_p)self)->window, mx, my);
}

static void av_display_sdl_get_mouse_position(av_display_p self, int* pmx, int* pmy)
{
	AV_UNUSED(self);
	SDL_GetMouseState(pmx, pmy);
}

static void av_display_sdl_destructor(void* pdisplay)
{
	av_display_sdl_p self = (av_display_sdl_p)pdisplay;
	if (self->renderer)
		SDL_DestroyRenderer(self->renderer);
	if (self->window)
		SDL_DestroyWindow(self->window);
}

/* Initializes memory given by the input pointer with the display sdl class information */
static av_result_t av_display_sdl_constructor(av_object_p self)
{
	/* override display methods */
	((av_display_p)self)->enum_display_modes        = av_display_sdl_enum_display_modes;
	((av_display_p)self)->set_configuration         = av_display_sdl_set_configuration;
	((av_display_p)self)->get_configuration         = av_display_sdl_get_configuration;
//	((av_display_p)self)->create_surface            = av_display_sdl_create_surface;
//	((av_display_p)self)->create_surface_from       = av_display_sdl_create_surface_from;
	((av_display_p)self)->set_capture               = av_display_sdl_set_capture;
	((av_display_p)self)->set_cursor_visible        = av_display_sdl_set_cursor_visible;
	((av_display_p)self)->is_cursor_visible         = av_display_sdl_is_cursor_visible;
	((av_display_p)self)->set_cursor_shape          = av_display_sdl_set_cursor_shape;
	((av_display_p)self)->set_mouse_position        = av_display_sdl_set_mouse_position;
	((av_display_p)self)->get_mouse_position        = av_display_sdl_get_mouse_position;

	return AV_OK;
}

/* Registers SDL display class into TORBA class repository */
AV_API av_result_t av_display_sdl_register_oop(av_oop_p oop)
{
	av_result_t rc;
	av_display_sdl_p display;
	if (AV_OK != (rc = av_display_register_oop(oop)))
		return rc;

	if (AV_OK != (rc = oop->define_class(oop, "display_sdl", "display", sizeof(av_display_sdl_t), av_display_sdl_constructor, av_display_sdl_destructor)))
		return rc;

	if (AV_OK != (rc = oop->new(oop, "display_sdl", (av_object_p*)&display)))
		return rc;

	/* register display as service */
	return oop->register_service(oop, "display", (av_service_p)display);
}


#endif /* WITH_SYSTEM_SDL */
