/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_video_surface_dfb.c                             */
/* Description:   DirectFB video surface implementation              */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_DIRECTFB

#include <malloc.h>
#include "av_video_dfb.h"

#define O_context(o) O_attr(o, CONTEXT_DFB_SURFACE)

/* imported prototypes */
av_result_t av_dfb_error_process(DFBResult, const char*, const char*, int);

/* exported prototypes */
av_result_t av_video_surface_dfb_register_torba(void);
av_result_t av_video_surface_dfb_constructor(av_object_p object);

#define av_dfb_error_check(funcname, rc) av_dfb_error_process(rc, funcname, __FILE__, __LINE__)

typedef struct _av_video_surface_dfb_t
{
	/*! parent object */
	av_video_surface_t video_surface;

} av_video_surface_dfb_t, *av_video_surface_dfb_p;

static av_result_t av_video_surface_dfb_create_overlay(av_video_surface_p psurface, av_video_overlay_p* ppoverlay)
{
	/* FIXME: implement it */
	AV_UNUSED(psurface);
	AV_UNUSED(ppoverlay);
	return AV_ESUPPORTED;
}

static av_result_t av_video_surface_dfb_create_overlay_buffered(av_video_surface_p psurface, av_video_overlay_p* ppoverlay)
{
	/* FIXME: implement it */
	AV_UNUSED(psurface);
	AV_UNUSED(ppoverlay);
	return AV_ESUPPORTED;
}

static av_result_t av_surface_dfb_set_size(av_surface_p psurface, int width, int height)
{
	av_result_t rc;
	av_video_surface_dfb_p self = (av_video_surface_dfb_p)psurface;
	DFBSurfaceDescription dsc;
	IDirectFBSurface *surface;
	av_video_dfb_p video_dfb = (av_video_dfb_p)((av_video_surface_p)self)->video;
	av_assert(video_dfb, "owner video is not defined for the surface");

	/* creates 32bit DirectFB surface */
	dsc.flags       = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
	dsc.caps        = DSCAPS_SYSTEMONLY;
	dsc.width       = width;
	dsc.height      = height;
	dsc.pixelformat = DSPF_RGB32;

	if (AV_OK != (rc = av_dfb_error_check("CreateSurface", video_dfb->dfb->CreateSurface(video_dfb->dfb, &dsc, &surface))))
		return rc;

	if (O_context(self))
	{
		/* destroys the previously created surface */
		IDirectFBSurface *oldsurface = (IDirectFBSurface *)O_context(self);
		oldsurface->Release(oldsurface);
	}

	O_set_attr(self, CONTEXT_DFB_SURFACE, surface);

	return AV_OK;
}

static av_result_t av_surface_dfb_get_size(av_surface_p psurface, int* pwidth, int* pheight)
{
	av_video_surface_dfb_p self = (av_video_surface_dfb_p)psurface;
	IDirectFBSurface* surface = (IDirectFBSurface*)O_context(self);
	if (surface)
	{
		return av_dfb_error_check("GetSize", surface->GetSize(surface, pwidth, pheight));
	}
	else
	{
		*pwidth  = *pheight = 0;
	}
	return AV_OK;
}

static av_result_t av_surface_dfb_lock(av_surface_p psurface, av_surface_lock_flags_t lockflags,
												av_pixel_p* ppixels, int* ppitch)
{
	DFBSurfaceLockFlags dfblockflags = DSLF_READ;
	av_video_surface_dfb_p self = (av_video_surface_dfb_p)psurface;
	IDirectFBSurface *surface = (IDirectFBSurface *)O_context(self);
	av_assert(surface, "Attempt to Lock uninitialized surface");
	if (lockflags == AV_SURFACE_LOCK_WRITE) dfblockflags |= DSLF_WRITE;
	return av_dfb_error_check("Lock", surface->Lock(surface, dfblockflags, (void**)ppixels, ppitch));
}

static av_result_t av_surface_dfb_unlock(av_surface_p psurface)
{
	av_result_t rc;
	av_video_surface_dfb_p self = (av_video_surface_dfb_p)psurface;
	IDirectFBSurface *surface = (IDirectFBSurface *)O_context(self);
	av_assert(surface, "Attempt to UnLock uninitialized surface");
	if (AV_OK != (rc = av_dfb_error_check("UnLock", surface->Unlock(surface))))
		return rc;

	return AV_OK;
}

static av_result_t av_video_surface_dfb_lock(av_video_surface_p psurface, av_surface_lock_flags_t lockflags,
											 av_pixel_p* ppixels, int* ppitch)
{
	av_surface_p self = (av_surface_p)psurface;
	return self->lock(self, lockflags, ppixels, ppitch);
}

static av_result_t av_video_surface_dfb_unlock(av_video_surface_p psurface)
{
	av_surface_p self = (av_surface_p)psurface;
	return self->unlock(self);
}

static av_result_t av_video_surface_dfb_set_clip(av_surface_p psurface, av_rect_p cliprect)
{
	av_video_surface_dfb_p self = (av_video_surface_dfb_p)psurface;
	IDirectFBSurface *surface = (IDirectFBSurface *)O_context(self);
	av_assert(surface, "Attempt to SetClip uninitialized surface");

	/* FIXME: convert av_rect_p to DFBRegion */
	return av_dfb_error_check("SetClip", surface->SetClip(surface, (const DFBRegion *)cliprect));
}

static av_result_t av_video_surface_dfb_get_clip(av_surface_p psurface, av_rect_p cliprect)
{
#if DIRECTFB_NEWER(1,0,0)
	av_video_surface_dfb_p self = (av_video_surface_dfb_p)psurface;
	IDirectFBSurface *surface = (IDirectFBSurface *)O_context(self);
	av_assert(surface, "Attempt to GetClip from uninitialized surface");
	/* FIXME: convert av_rect_p to DFBRegion */
	return av_dfb_error_check("GetClip", surface->GetClip(surface, (DFBRegion *)cliprect));
#else
	AV_UNUSED(psurface);
	AV_UNUSED(cliprect);
	return AV_ESUPPORTED;
#endif
}

/* Blit source surface to this video surface */
static av_result_t av_video_surface_dfb_blit(av_video_surface_p pdstsurface, av_rect_p dstrect, av_surface_p psrcsurface, av_rect_p srcrect)
{
	av_result_t rc = AV_OK;
	av_video_surface_dfb_p dst = (av_video_surface_dfb_p)pdstsurface;
	IDirectFBSurface *dstsurface = (IDirectFBSurface *)O_context(dst);
	av_assert(dstsurface, "Attempt to Blit on uninitialized surface");
	if (O_is_a(psrcsurface, "video_surface_dfb"))
	{
		av_video_surface_dfb_p src = (av_video_surface_dfb_p)psrcsurface;
		IDirectFBSurface *srcsurface = (IDirectFBSurface *)O_context(src);
		DFBRectangle srect;
		av_assert(srcsurface, "Attempt to Blit from uninitialized surface");

		dstsurface->SetBlittingFlags(dstsurface, DSBLIT_NOFX);

		srect.x = srcrect->x;
		srect.y = srcrect->y;
		srect.w = srcrect->w;
		srect.h = srcrect->h;

		if ((dstrect->w == srcrect->w) && (dstrect->h == srcrect->h))
		{
			rc = av_dfb_error_check("Blit", dstsurface->Blit(dstsurface, srcsurface, &srect, dstrect->x, dstrect->y));
		}
		else
		{
			DFBRectangle drect;
			drect.x = dstrect->x;
			drect.y = dstrect->y;
			drect.w = dstrect->w;
			drect.h = dstrect->h;

			rc = av_dfb_error_check("StretchBlit", dstsurface->StretchBlit(dstsurface, srcsurface, &srect, &drect));
		}
	}
	else
	{
		/* FIXME: implement software blitting */
		rc = AV_ESUPPORTED;
	}

	if (AV_OK ==  rc)
		if (O_is_a(pdstsurface, "video"))
		{
			DFBRegion region;
			region.x1 = dstrect->x;
			region.y1 = dstrect->y;
			region.x2 = dstrect->x + dstrect->w - 1;
			region.y2 = dstrect->y + dstrect->h - 1;
			/* FIXME: Flipping should not be necessary when double buffering is switched off */
			rc = av_dfb_error_check("Flip", dstsurface->Flip(dstsurface, &region, DSFLIP_BLIT));
		}
	return rc;
}

static av_result_t av_video_surface_dfb_set_size(struct av_video_surface* self, int width, int height)
{
	return av_surface_dfb_set_size((av_surface_p)self, width, height);
}

static av_result_t av_video_surface_dfb_get_size(struct av_video_surface* self, int* pwidth, int* pheight)
{
	return av_surface_dfb_get_size((av_surface_p)self, pwidth, pheight);
}

static av_result_t av_video_surface_dfb_get_depth(struct av_video_surface* self, int* pdepth)
{
	/*FIXME*/
	AV_UNUSED(self);
	*pdepth = 32;
	return AV_OK;
}

static void av_video_surface_dfb_destructor(void* psurface)
{
	av_video_surface_dfb_p self = (av_video_surface_dfb_p)psurface;
	IDirectFBSurface *surface = (IDirectFBSurface *)O_context(self);
	if (surface)
	{
		surface->Release(surface);
	}
}

av_result_t av_video_surface_dfb_constructor(av_object_p object)
{
	av_video_surface_dfb_p self = (av_video_surface_dfb_p)object;

	/* DirectFB surface is not created until set_size method invoked */
	O_set_attr(self, CONTEXT_DFB_SURFACE, AV_NULL);
	/* registers surface methods ... */
	((av_surface_p)self)->set_size                      = av_surface_dfb_set_size;
	((av_surface_p)self)->get_size                      = av_surface_dfb_get_size;
	((av_surface_p)self)->lock                          = av_surface_dfb_lock;
	((av_surface_p)self)->unlock                        = av_surface_dfb_unlock;
	((av_surface_p)self)->set_clip                      = av_video_surface_dfb_set_clip;
	((av_surface_p)self)->get_clip                      = av_video_surface_dfb_get_clip;
	((av_video_surface_p)self)->blit                    = av_video_surface_dfb_blit;
	((av_video_surface_p)self)->create_overlay          = av_video_surface_dfb_create_overlay;
	((av_video_surface_p)self)->create_overlay_buffered = av_video_surface_dfb_create_overlay_buffered;
	((av_video_surface_p)self)->set_size                = av_video_surface_dfb_set_size;
	((av_video_surface_p)self)->get_size                = av_video_surface_dfb_get_size;
	((av_video_surface_p)self)->get_depth               = av_video_surface_dfb_get_depth;
	((av_video_surface_p)self)->lock                    = av_video_surface_dfb_lock;
	((av_video_surface_p)self)->unlock                  = av_video_surface_dfb_unlock;

	return AV_OK;
}

/* Registers DirectFB video surface class into TORBA class repository */
av_result_t av_video_surface_dfb_register_torba(void)
{
	return av_torb_register_class("video_surface_dfb", "video_surface", sizeof(av_video_surface_dfb_t),
								  av_video_surface_dfb_constructor, av_video_surface_dfb_destructor);
}

#endif /* WITH_SYSTEM_DIRECTFB */
