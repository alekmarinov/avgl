/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_video_dfb.c                                     */
/* Description:   DirectFB video backend                             */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_DIRECTFB
/* DirectFB video backend implementation */

#include <malloc.h>
#include "av_video_dfb.h"

#define CONTEXT "video_dfb_ctx"
#define O_context(o) O_attr(o, CONTEXT)

/* export video_dfb registration method */
AV_API av_result_t av_video_dfb_register_torba(void);

/* import dfb error processor */
av_result_t av_dfb_error_process(DFBResult, const char*, const char*, int);

/* import video_surface_dfb registration method and constructor */
av_result_t av_video_surface_dfb_register_torba(void);
av_result_t av_video_surface_dfb_constructor(av_object_p object);

#define av_dfb_error_check(funcname, rc) av_dfb_error_process(rc, funcname, __FILE__, __LINE__)

/* Cleans up DFB objects */
static void av_video_dfb_clean_up(av_video_dfb_p self)
{
	IDirectFBSurface* primary_surface = (IDirectFBSurface*)O_context(self);

	if (((av_video_p)self)->backbuffer)
		O_destroy(((av_video_p)self)->backbuffer);

	if (primary_surface)
		primary_surface->Release(primary_surface);
	O_set_attr(self, CONTEXT, AV_NULL);

	if (self->primary_layer)
		self->primary_layer->Release(self->primary_layer);
	self->primary_layer = AV_NULL;

	if (self->dfb)
		self->dfb->Release(self->dfb);
	self->dfb = AV_NULL;
}

static av_result_t av_video_dfb_set_size(av_surface_p pvideo, int width, int height)
{
	av_video_p self = (av_video_p)pvideo;
	av_video_config_t video_config;
	video_config.flags = AV_VIDEO_CONFIG_SIZE;
	video_config.width = width;
	video_config.height = height;
	return self->set_configuration(self, &video_config);
}

static av_result_t av_video_dfb_get_size(av_surface_p pvideo, int* pwidth, int* pheight)
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
static av_bool_t av_video_dfb_compare_configuration(av_video_config_flags_t flags, av_video_config_p config1, av_video_config_p config2)
{
	if (flags & AV_VIDEO_CONFIG_MODE)
		if (config1->mode != config2->mode)
			return AV_FALSE;

	if (flags & AV_VIDEO_CONFIG_SIZE)
		if (config1->width != config2->width || config1->height != config2->height)
			return AV_FALSE;

	return AV_TRUE;
}

static av_result_t av_video_dfb_set_configuration(av_video_p pvideo, av_video_config_p new_video_config)
{
	av_result_t rc;
	av_video_dfb_p self = (av_video_dfb_p)pvideo;
	IDirectFBDisplayLayer *layer;
	IDirectFBSurface *surface;
	IDirectFB *dfb;
	DFBDisplayLayerConfig primary_layer_config;
	DFBDisplayLayerCooperativeLevel cooperativelevel;
	av_video_config_t video_config;

	if (new_video_config->flags & AV_VIDEO_CONFIG_BPP)
	{
		/* changing bpp is not allowed */
		return AV_EPERM;
	}

	/* FIXME: Windowed mode is not supported for DirectFB */
	if (new_video_config->flags & AV_VIDEO_CONFIG_MODE)
	{
		/* HACK: assume always fullscreen */
// 		new_video_config->mode = AV_VIDEO_MODE_FULLSCREEN;
	}

	/* gets previous video mode settings */
	video_config.flags = new_video_config->flags;
	pvideo->get_configuration(pvideo, &video_config);
	if (av_video_dfb_compare_configuration(video_config.flags, &video_config, new_video_config))
		return AV_OK; /* nothing to configure */

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

	if ((AV_NULL == self->dfb) || (video_config.flags & AV_VIDEO_CONFIG_MODE))
	{
		/* unset previous video mode, cleans up all DirectFB related objects */
		av_video_dfb_clean_up(self);

		/* creates DirectFB super interface */
		if (AV_OK != (rc = av_dfb_error_check("DirectFBCreate", DirectFBCreate(&dfb))))
		{
			return rc;
		}
		self->dfb = dfb;

		/* gets primary display layer */
		if (AV_OK != (rc = av_dfb_error_check("IDirectFB.GetDisplayLayer",
											  dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &layer))))
		{
			/* clean up */
			av_video_dfb_clean_up(self);
			return rc;
		}
		self->primary_layer = layer;

		if (video_config.mode & AV_VIDEO_MODE_FULLSCREEN)
		{
			/* exclusive access, fullscreen/mode switching */
			cooperativelevel = DLSCL_EXCLUSIVE;
		}
		else
		{
			/* windowed mode */
			cooperativelevel = DLSCL_ADMINISTRATIVE;
		}

		/* sets DFB cooperation level (for fullscreen or windowed video mode ) */
		if (AV_OK != (rc = av_dfb_error_check("IDirectFB.SetCooperativeLevel",
											  dfb->SetCooperativeLevel(dfb, cooperativelevel))))
		{
			/* clean up */
			av_video_dfb_clean_up(self);
			return rc;
		}

		/* sets Primary Layer cooperation level (for fullscreen or windowed video mode ) */
		if (AV_OK != (rc = av_dfb_error_check("IDirectFBDisplayLayer.SetCooperativeLevel",
											  layer->SetCooperativeLevel(layer, cooperativelevel))))
		{
			/* clean up */
			av_video_dfb_clean_up(self);
			return rc;
		}
	}

	/* configures primary display layer */
	if ((video_config.flags & AV_VIDEO_CONFIG_MODE) && (video_config.mode == AV_VIDEO_MODE_FULLSCREEN))
	{
		primary_layer_config.flags        = DLCONF_PIXELFORMAT | DLCONF_BUFFERMODE;
		primary_layer_config.pixelformat  = DSPF_RGB32;
		primary_layer_config.buffermode   = DLBM_FRONTONLY;

		if (video_config.flags & AV_VIDEO_CONFIG_SIZE)
		{
			primary_layer_config.flags   |= DLCONF_WIDTH | DLCONF_HEIGHT;
			primary_layer_config.width    = video_config.width;
			primary_layer_config.height   = video_config.height;
		}

		/* apply new configuration */
		if (AV_OK != (rc = av_dfb_error_check("IDirectFBDisplayLayer.SetConfiguration",
											layer->SetConfiguration(layer, &primary_layer_config))))
		{
			/* clean up */
			av_video_dfb_clean_up(self);
			return rc;
		}

		if (AV_OK != (rc = av_dfb_error_check("IDirectFBDisplayLayer.GetSurface",
											  layer->GetSurface(layer, &surface))))
		{
			/* clean up */
			av_video_dfb_clean_up(self);
			return rc;
		}
	}
	else
	{
		/* DirectFB is not initialized, Video type or size is changed */
		DFBSurfaceDescription dsc;

		surface = O_context(self);
		if (surface)
			surface->Release(surface);

		if (video_config.flags & AV_VIDEO_CONFIG_SIZE)
		{
			if (AV_OK != (rc = av_dfb_error_check("IDirectFB.SetVideoMode",
												  dfb->SetVideoMode(dfb, video_config.width, video_config.height, 32))))
			{
				/* clean up */
				av_video_dfb_clean_up(self);
				return rc;
			}
		}

		dsc.flags = DSDESC_CAPS;
		dsc.caps  = DSCAPS_PRIMARY;
		if (AV_OK != (rc = av_dfb_error_check("IDirectFB.CreateSurface",
											  dfb->CreateSurface(dfb, &dsc, &surface))))
		{
			/* clean up */
			av_video_dfb_clean_up(self);
			return rc;
		}
	}

	layer->EnableCursor(layer, 1);

	/* return the new applied configuration */
	pvideo->get_configuration(pvideo, new_video_config);

	O_set_attr(self, CONTEXT, surface);

	return AV_OK;
}

/* Gets video configuration options */
static av_result_t av_video_dfb_get_configuration(av_video_p pvideo, av_video_config_p video_config)
{
	av_result_t rc;
	av_video_dfb_p self = (av_video_dfb_p)pvideo;
	IDirectFBSurface* primary_surface = (IDirectFBSurface*)O_context(self);

	if (primary_surface)
	{
		if (video_config->flags & AV_VIDEO_CONFIG_SIZE)
		{
			if (AV_OK != (rc = av_dfb_error_check("IDirectFBSurface.GetSize",
												  primary_surface->GetSize(primary_surface, &(video_config->width), &(video_config->height)))))
			{
				return rc;
			}
		}

		if (video_config->flags & AV_VIDEO_CONFIG_BPP)
		{
			DFBSurfacePixelFormat pf;
			if (AV_OK != (rc = av_dfb_error_check("IDirectFBSurface.GetPixelFormat", primary_surface->GetPixelFormat(primary_surface, &pf))))
			{
				return rc;
			}

			video_config->bpp = 0;
			switch (pf)
			{
				case DSPF_ARGB1555:
				case DSPF_RGB16: video_config->bpp=16; break;
				case DSPF_RGB24: video_config->bpp=24; break;
				case DSPF_RGB32:
				case DSPF_ARGB:  video_config->bpp=32; break;
				default:
					/* DirectFB reported unhandled pixel format */
				break;
			}
		}

		if (video_config->flags & AV_VIDEO_CONFIG_MODE)
		{
			/*! FIXME: how to detect if DirectFB under fullscreen or windowed mode? */
			video_config->mode = AV_VIDEO_MODE_WINDOWED;
		}
	}
	else
	{
		/* video is not initialized */
		if (video_config->flags & AV_VIDEO_CONFIG_SIZE)
		{
			/* size is not set */
			video_config->width = video_config->height = 0;
		}
		if (video_config->flags & AV_VIDEO_CONFIG_BPP)
		{
			/* bpp is undefined */
			video_config->bpp = 0;
		}
		if (video_config->flags & AV_VIDEO_CONFIG_MODE)
		{
			/* mode is undefined */
			video_config->mode = 0;
		}
	}
	return AV_OK;
}

static av_result_t av_video_dfb_create_surface(av_video_p self, int width, int height, av_video_surface_p* ppsurface)
{
	av_result_t rc;
	av_video_surface_p surface;
	if (AV_OK != (rc = av_torb_create_object("video_surface_dfb", (av_object_p*)&surface)))
	{
		return rc;
	}
	/* set_size will expect a reference to the video owner object */
	surface->video = self;
	if (AV_OK != (rc = ((av_surface_p)surface)->set_size((av_surface_p)surface, width, height)))
	{
		O_destroy(surface);
		return rc;
	}
	*ppsurface = surface;
	return AV_OK;
}

static av_result_t av_video_dfb_create_overlay(av_video_p self, av_video_overlay_p* ppoverlay)
{
	av_result_t rc;
	av_video_surface_p backbuffer;
	if (AV_OK == (rc = self->get_backbuffer(self,&backbuffer)))
	{
		rc = backbuffer->create_overlay(backbuffer,ppoverlay);
	}
	return rc;
}

static av_result_t av_video_dfb_create_overlay_buffered(av_video_p self, av_video_overlay_p* ppoverlay)
{
	av_result_t rc;
	av_video_surface_p backbuffer;
	if (AV_OK == (rc = self->get_backbuffer(self,&backbuffer)))
	{
		rc = backbuffer->create_overlay_buffered(backbuffer,ppoverlay);
	}
	return rc;
}

static av_result_t av_video_dfb_create_surface_from(av_video_p self, void *ptr, int width, int height,
													int pitch, av_video_surface_p* ppsurface)
{
	av_result_t rc;
	av_video_surface_p surface;
	DFBSurfaceDescription dsc;
	IDirectFBSurface *dfb_surface;
	av_video_dfb_p video_dfb = (av_video_dfb_p)self;

	/* creates 32bit DirectFB surface */
	dsc.flags       = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_PREALLOCATED;
	dsc.caps        = DSCAPS_SYSTEMONLY;
	dsc.width       = width;
	dsc.height      = height;
	dsc.pixelformat = DSPF_RGB32;
	dsc.preallocated[0].data = ptr;
	dsc.preallocated[0].pitch = pitch;

	if (AV_OK != (rc = av_torb_create_object("video_surface_dfb", (av_object_p*)&surface)))
	{
		return rc;
	}

	if (AV_OK != (rc = av_dfb_error_check("CreateSurface",
										  video_dfb->dfb->CreateSurface(video_dfb->dfb, &dsc, &dfb_surface))))
	{
		O_destroy(surface);
		return rc;
	}

	O_set_attr(surface, CONTEXT_DFB_SURFACE, dfb_surface);
	*ppsurface = surface;
	return AV_OK;
}

static av_result_t av_video_dfb_get_backbuffer(av_video_p self, av_video_surface_p* ppsurface)
{
	if (AV_NULL == self->backbuffer)
	{
		int width, height;
		av_result_t rc;
		if (AV_OK != (rc = ((av_surface_p)self)->get_size((av_surface_p)self, &width, &height)))
			return rc;

		if (AV_OK != (rc = self->create_surface(self, width, height, &(self->backbuffer))))
			return rc;
	}
	*ppsurface = self->backbuffer;
	return AV_OK;
}

static av_result_t av_video_dfb_update(av_video_p self, av_rect_p rect)
{
	IDirectFBDisplayLayer *primary_layer;
	IDirectFBSurface *surface = (IDirectFBSurface *)O_context(self);
	IDirectFBSurface *backbuffer;
	av_rect_t update_rect;
	DFBRegion region;
	av_result_t rc;
	av_assert(surface, "Video surface is not initialized properly");
	av_assert(self->backbuffer, "Backbuffer is not initialized");
	backbuffer = (IDirectFBSurface *)O_context(self->backbuffer);
	primary_layer = ((av_video_dfb_p)self)->primary_layer;
	av_assert(primary_layer, "Primary layer can't be NULL");

	if (rect)
		av_rect_copy(&update_rect, rect);
	else
	{
		int w, h;
		((av_surface_p)self)->get_size((av_surface_p)self, &w, &h);
		av_rect_init(&update_rect, 0, 0, w, h);
	}

	region.x1 = update_rect.x;
	region.y1 = update_rect.y;
	region.x2 = update_rect.x + update_rect.w - 1;
	region.y2 = update_rect.y + update_rect.h - 1;

	primary_layer->EnableCursor(primary_layer, 0);
	if (AV_OK != (rc = av_dfb_error_check("Blit",
										  surface->Blit(surface, backbuffer, (DFBRectangle*)&update_rect, update_rect.x, update_rect.y))))
		return rc;

	if (AV_OK != (rc = av_dfb_error_check("Flip", surface->Flip(surface, &region, DSFLIP_BLIT))))
		return rc;
	primary_layer->EnableCursor(primary_layer, 1);

	return AV_OK;
}

static void av_video_dfb_set_cursor_visible(av_video_p self, av_bool_t visible)
{
	IDirectFBDisplayLayer *primary_layer = ((av_video_dfb_p)self)->primary_layer;
	primary_layer->EnableCursor(primary_layer, visible);
}

static av_result_t av_video_dfb_get_root_window(struct av_video* self, void** pdisplay, void** pwindow)
{
	AV_UNUSED(self);
	AV_UNUSED(pdisplay);
	AV_UNUSED(pwindow);
	return AV_ESUPPORTED;
}

static void av_video_dfb_destructor(void* pvideo)
{
	av_video_dfb_p self = (av_video_dfb_p)pvideo;
	/* clean up DirectFB objects */
	av_video_dfb_clean_up(self);
}

/* Initializes memory given by the input pointer with the video dfb class information */
static av_result_t av_video_dfb_constructor(av_object_p object)
{
	av_result_t rc;
	av_video_dfb_p self = (av_video_dfb_p)object;

	if (AV_OK != (rc = av_video_surface_dfb_constructor(object)))
		return rc;

	if (AV_OK != (rc = av_dfb_error_check("DirectFBInit", DirectFBInit(0, 0))))
		return rc;

	//O_context(self) = AV_NULL; /* context with the primary surface is not initialized until set_size */

	/* initializes object */
	self->dfb = AV_NULL;
	self->primary_layer = AV_NULL;

	/* override video methods */
	((av_surface_p)self)->set_size                   = av_video_dfb_set_size;
	((av_surface_p)self)->get_size                   = av_video_dfb_get_size;
	((av_video_p)self)->set_configuration            = av_video_dfb_set_configuration;
	((av_video_p)self)->get_configuration            = av_video_dfb_get_configuration;
	((av_video_p)self)->create_surface               = av_video_dfb_create_surface;
	((av_video_p)self)->create_overlay               = av_video_dfb_create_overlay;
	((av_video_p)self)->create_overlay_buffered      = av_video_dfb_create_overlay_buffered;
	((av_video_p)self)->create_surface_from          = av_video_dfb_create_surface_from;
	((av_video_p)self)->get_backbuffer               = av_video_dfb_get_backbuffer;
	((av_video_p)self)->update                       = av_video_dfb_update;
	((av_video_p)self)->set_cursor_visible           = av_video_dfb_set_cursor_visible;
	((av_video_p)self)->get_root_window              = av_video_dfb_get_root_window;
	/* initialize video surface objects */
	((av_video_surface_p)self)->video = (av_video_p)self;

	return AV_OK;
}

/* Registers DirectFB video class into TORBA class repository */
AV_API av_result_t av_video_dfb_register_torba(void)
{
	av_result_t rc;
	if (AV_OK != (rc = av_video_surface_dfb_register_torba()))
		return rc;
	return av_torb_register_class("video_dfb", "video", sizeof(av_video_dfb_t), av_video_dfb_constructor, av_video_dfb_destructor);
}

#endif /* WITH_SYSTEM_DIRECTFB */
