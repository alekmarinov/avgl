/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_scaler.c                                        */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_FFMPEG

#include <string.h>
#include <av_media.h>
#include <errno.h>
#include <malloc.h>

#ifdef USE_INCLUDE_FFMPEG
	#include <ffmpeg/avcodec.h>
	#include <ffmpeg/swscale.h>
#endif

#ifdef USE_INCLUDE_LIBAVFORMAT
	#include <libavcodec/avcodec.h>
	#include <libswscale/swscale.h>
#endif

#define CONTEXT "scaler_swscale_ctx"
#define O_context(o) O_attr(o, CONTEXT)

av_result_t av_scaler_swscale_register_torba(void);

typedef struct
{
	av_scale_info_t scale_info;
	struct SwsContext *sws;
} av_scaler_ctx_t, *av_scaler_ctx_p;

static int pix_fmt_cvt(int in_pix_fmt)
{
	switch (in_pix_fmt)
	{
		case AV_VIDEO_FORMAT_RGB24:
			return PIX_FMT_RGB24;
		break;
		case AV_VIDEO_FORMAT_RGB32:
 			return PIX_FMT_RGB32;
		break;
		case AV_VIDEO_FORMAT_YUV420P:
 			return PIX_FMT_YUV420P;
		break;
		default:
			break;
	}
	return in_pix_fmt;
}

static av_result_t av_scaler_swscale_set_configuration(av_scaler_p self, av_scale_info_p scale_info)
{
	av_scaler_ctx_p ctx = (av_scaler_ctx_p)O_context(self);
	int src_pix_fmt, dst_pix_fmt;
	if (ctx->sws)
	{
		if (!memcmp(&ctx->scale_info, scale_info, sizeof(av_scale_info_t)))
		{
			return AV_OK;
		}
		sws_freeContext(ctx->sws);
	}
	memcpy(&ctx->scale_info, scale_info, sizeof(av_scale_info_t));

	src_pix_fmt = pix_fmt_cvt(scale_info->src_format);
	dst_pix_fmt = pix_fmt_cvt(scale_info->dst_format);

	/*! FIXME: add scale algorithm to configuration (e.g. AV_SCALE_ALG_BILINEAR) */
	ctx->sws = sws_getContext(scale_info->src_width, scale_info->src_height, src_pix_fmt,
							  scale_info->dst_width, scale_info->dst_height, dst_pix_fmt,
							  SWS_CPU_CAPS_MMX|
							  SWS_FAST_BILINEAR /*SWS_GAUSS,SWS_SPLINE,SWS_BILINEAR,SWS_POINT*/,
							  0, 0, 0);
	return AV_OK;
}

static av_result_t av_scaler_swscale_get_configuration(av_scaler_p self, av_scale_info_p scale_info)
{
	av_scaler_ctx_p ctx = (av_scaler_ctx_p)O_context(self);
	memcpy(scale_info, &ctx->scale_info, sizeof(av_scale_info_t));
	return AV_OK;
}

static av_result_t av_scaler_swscale_scale(av_scaler_p self, av_frame_video_p frame, av_surface_p surface)
{
	av_scaler_ctx_p ctx = (av_scaler_ctx_p)O_context(self);
	AVFrame FrameRGB;
	av_result_t rc;
	av_pixel_p pixels;
	int linesize, width, height;

	if (AV_OK != (rc = surface->get_size(surface, &width, &height)))
	{
		return rc;
	}

	if (AV_OK != (rc = surface->lock(surface, AV_SURFACE_LOCK_WRITE, &pixels, &linesize)))
	{
		return rc;
	}

	avpicture_fill((AVPicture *)&FrameRGB, (void*)pixels, PIX_FMT_RGB32, width, height);
	sws_scale(ctx->sws, frame->data, frame->linesize, 0, frame->height, FrameRGB.data, FrameRGB.linesize);

	// /*
	// FIXME: Via ffmpeg, swscale or cairo unificate the meaning of alpha component
	// Cairo expect 255 for visibility, while avpicture_fill, sws_scale put zero
	// Or somehow avoid alpha interpretation from cairo graphics
	if (ctx->scale_info.dst_format == AV_VIDEO_FORMAT_RGB32)
	{
		int len = width*height;
		unsigned long* palpha = pixels;
		while (len-->0) *palpha++ |= 0xff000000U;
	}
	// end of FIXME
	// */
	surface->unlock(surface);
	return AV_OK;
}

static av_result_t av_scaler_swscale_scale_raw(av_scaler_p self, void* src, void* dst)
{
	av_scaler_ctx_p ctx = (av_scaler_ctx_p)O_context(self);
	AVPicture* PicSrc = (AVPicture*)src;
	AVPicture* PicDst = (AVPicture*)dst;
	av_assert(0 !=ctx->sws, "scaler context not initialized");
	sws_scale(ctx->sws, PicSrc->data, PicSrc->linesize, 0, ctx->scale_info.dst_height, PicDst->data, PicDst->linesize);
	// /*
	// FIXME: Via ffmpeg, swscale or cairo unificate the meaning of alpha component
	// Cairo expect 255 for visibility, while avpicture_fill, sws_scale put zero
	// Or somehow avoid alpha interpretation from cairo graphics
	if (ctx->scale_info.dst_format == AV_VIDEO_FORMAT_RGB32)
	{
		int len = ctx->scale_info.dst_width*ctx->scale_info.dst_height;
		unsigned long* palpha = (unsigned long*)PicDst->data[0];
		while (len-->0) *palpha++ |= 0xff000000U;
	}
	// end of FIXME
	// */
	return AV_OK;
}

static void av_scaler_swscale_destructor(void* pobject)
{
	av_scaler_p self    = (av_scaler_p)pobject;
	av_scaler_ctx_p ctx = (av_scaler_ctx_p)O_context(self);
	if (ctx->sws)
		sws_freeContext(ctx->sws);
	free(ctx);
}

static av_result_t av_scaler_swscale_constructor(av_object_p pobject)
{
	av_scaler_p self        = (av_scaler_p)pobject;
	av_scaler_ctx_p ctx     = (av_scaler_ctx_p)malloc(sizeof(av_scaler_ctx_t));
	if (!ctx)
		return AV_EMEM;

	memset(ctx, 0, sizeof(av_scaler_ctx_t));
	O_set_attr(self, CONTEXT, ctx);
	self->set_configuration = av_scaler_swscale_set_configuration;
	self->get_configuration = av_scaler_swscale_get_configuration;
	self->scale             = av_scaler_swscale_scale;
	self->scale_raw         = av_scaler_swscale_scale_raw;
	return AV_OK;
}

av_result_t av_scaler_swscale_register_torba(void)
{
	av_result_t rc;

	if (AV_OK != (rc = av_torb_register_class("scaler_swscale",
											  "scaler",
											  sizeof(av_scaler_t),
											  av_scaler_swscale_constructor, av_scaler_swscale_destructor)))
		return rc;

	return AV_OK;
}

#endif // WITH_FFMPEG
