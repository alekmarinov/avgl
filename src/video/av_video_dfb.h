/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_video_dfb.h                                     */
/* Description:   DirectFB video backend                             */
/*                                                                   */
/*********************************************************************/

#ifndef __AV_VIDEO_DFB_H
#define __AV_VIDEO_DFB_H

#include <av_video.h>
#include <directfb.h>
#include <directfb/directfb_version.h>

#define MAKE_VERSION_NUM(x,y,z) (z + (y << 8) + (x << 16))
#define DIRECTFB_VERSION_NUM MAKE_VERSION_NUM(DIRECTFB_MAJOR_VERSION, DIRECTFB_MINOR_VERSION, DIRECTFB_MICRO_VERSION)
#define DIRECTFB_NEWER(x,y,z) (MAKE_VERSION_NUM(x,y,z) <= DIRECTFB_VERSION_NUM)
#define DIRECTFB_OLDER(x,y,z) (MAKE_VERSION_NUM(x,y,z) > DIRECTFB_VERSION_NUM)

typedef struct av_video_dfb
{
	/* Parent object */
	av_video_t video;

	/* DirectFB super interface */
	IDirectFB *dfb;

	/* DirectFB primary display layer */
	IDirectFBDisplayLayer *primary_layer;
} av_video_dfb_t, *av_video_dfb_p;

#define CONTEXT_DFB_SURFACE "video_surface_dfb_ctx"
#define CONTEXT_DFB_OVERLAY "video_overlay_dfb_ctx"

#endif /* __AV_VIDEO_DFB_H */
