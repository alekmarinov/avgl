/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_xembed_surface.c                                */
/*                                                                   */
/*********************************************************************/

#include <malloc.h>
#include "av_xembed_surface.h"
#include <av_xembed.h>

#define CONTEXT "xembed_surface_ctx"
#define O_context(o) O_attr(o, CONTEXT)

typedef struct av_xembed_surface_ctx
{
	av_xembed_p xembed;

} av_xembed_surface_ctx_t, *av_xembed_surface_ctx_p;

static av_result_t av_xembed_surface_set_size(av_surface_p self, int width, int height)
{
	AV_UNUSED(self);
	AV_UNUSED(width);
	AV_UNUSED(height);
	return AV_OK;
}

static av_result_t av_xembed_surface_get_size(av_surface_p self,
											  int* pwidth, int* pheight)
{
	av_result_t rc;
	av_xembed_surface_ctx_p ctx = (av_xembed_surface_ctx_p)O_context(self);
	av_xembed_video_t xembed_info;
	if (AV_OK != (rc = ctx->xembed->get_video_info(ctx->xembed, &xembed_info)))
		return rc;
	*pwidth = xembed_info.xres;
	*pheight = xembed_info.yres;
	return AV_OK;
}

static av_result_t av_xembed_surface_lock(av_surface_p self,
										  av_surface_lock_flags_t lockflags,
										  av_pixel_p* ppixels,
										  int* ppitch)
{
	av_result_t rc;
	av_xembed_surface_ctx_p ctx = (av_xembed_surface_ctx_p)O_context(self);
	av_xembed_video_t xembed_info;

	if (lockflags == AV_SURFACE_LOCK_WRITE)
		return AV_EPERM;  /* write permission is denied */
	else
	if (lockflags != AV_SURFACE_LOCK_READ)
		return AV_EARG;   /* invalid surface locking flag */

	if (AV_OK != (rc = ctx->xembed->get_video_info(ctx->xembed, &xembed_info)))
		return rc;

	*ppixels = (av_pixel_p)xembed_info.vmem;
	*ppitch = (xembed_info.xres * xembed_info.bpp) >> 3;
	return AV_OK;
}

static av_result_t av_xembed_surface_unlock(av_surface_p self)
{
	AV_UNUSED(self);
	return AV_OK;
}

static av_result_t av_xembed_surface_constructor(av_object_p pobject)
{
	av_result_t rc;
	av_xembed_surface_p self = (av_xembed_surface_p)pobject;
	av_xembed_surface_ctx_p ctx = (av_xembed_surface_ctx_p)malloc(sizeof(av_xembed_surface_ctx_t));

	if (AV_OK != (rc = av_torb_service_addref("xembed", (av_service_p*)&ctx->xembed)))
	{
		free(ctx);
		return rc;
	}

	O_set_attr(self, CONTEXT, ctx);
	((av_surface_p)self)->set_size = av_xembed_surface_set_size;
	((av_surface_p)self)->get_size = av_xembed_surface_get_size;
	((av_surface_p)self)->lock     = av_xembed_surface_lock;
	((av_surface_p)self)->unlock   = av_xembed_surface_unlock;

	return AV_OK;
}

static void av_xembed_surface_destructor(void *pobject)
{
	av_xembed_surface_ctx_p ctx = (av_xembed_surface_ctx_p)O_context(pobject);
	av_torb_service_release("xembed");
	free(ctx);
}

av_result_t av_xembed_surface_register_class( void )
{
	av_result_t rc;
	
	if (AV_OK != (rc = av_torb_register_class("xembed_surface", "surface",
											  sizeof(av_xembed_surface_t),
											  av_xembed_surface_constructor,
											  av_xembed_surface_destructor)))
		return rc;

	return AV_OK;
}

