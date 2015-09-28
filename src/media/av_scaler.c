/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_scaler.c                                        */
/*                                                                   */
/*********************************************************************/

#include <av_media.h>

static av_result_t av_scaler_set_configuration(av_scaler_p self, av_scale_info_p scale_info)
{
	AV_UNUSED(self);
	AV_UNUSED(scale_info);
	return AV_ESUPPORTED;
}

static av_result_t av_scaler_get_configuration(av_scaler_p self, av_scale_info_p scale_info)
{
	AV_UNUSED(self);
	AV_UNUSED(scale_info);
	return AV_ESUPPORTED;
}

static av_result_t av_scaler_scale(av_scaler_p self, av_frame_video_p frame, av_surface_p surface)
{
	AV_UNUSED(self);
	AV_UNUSED(frame);
	AV_UNUSED(surface);
	return AV_ESUPPORTED;
}

static av_result_t av_scaler_scale_raw(av_scaler_p self, void* src, void* dst)
{
	AV_UNUSED(self);
	AV_UNUSED(src);
	AV_UNUSED(dst);
	return AV_ESUPPORTED;
}

static av_result_t av_scaler_constructor(av_object_p pobject)
{
	av_scaler_p self        = (av_scaler_p)pobject;
	self->set_configuration = av_scaler_set_configuration;
	self->get_configuration = av_scaler_get_configuration;
	self->scale             = av_scaler_scale;
	self->scale_raw         = av_scaler_scale_raw;
	return AV_OK;
}

av_result_t av_scaler_register_torba(void)
{
	av_result_t rc;

	if (AV_OK != (rc = av_torb_register_class("scaler",
											  AV_NULL,
											  sizeof(av_scaler_t),
											  av_scaler_constructor, AV_NULL)))
		return rc;

	return AV_OK;
}
