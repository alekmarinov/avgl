/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_media.c                                         */
/*                                                                   */
/*********************************************************************/

#include <av_media.h>

static av_result_t av_media_open(av_media_p self, const char* url)
{
	AV_UNUSED(self);
	AV_UNUSED(url);
	return AV_ESUPPORTED;
}

static av_result_t av_media_get_audio_info(av_media_p self, av_audio_info_p audio_info)
{
	AV_UNUSED(self);
	AV_UNUSED(audio_info);
	return AV_ESUPPORTED;
}

static av_result_t av_media_get_video_info(av_media_p self, av_video_info_p video_info)
{
	AV_UNUSED(self);
	AV_UNUSED(video_info);
	return AV_ESUPPORTED;
}

static av_result_t av_media_read_packet(av_media_p self, av_packet_p* packet)
{
	AV_UNUSED(self);
	AV_UNUSED(packet);
	return AV_ESUPPORTED;
}

static av_result_t av_media_get_audio_decoder(av_media_p self, av_decoder_audio_p* ppaudiodecoder)
{
	AV_UNUSED(self);
	AV_UNUSED(ppaudiodecoder);
	return AV_ESUPPORTED;
}

static av_result_t av_media_get_video_decoder(av_media_p self, av_decoder_video_p* ppvideodecoder)
{
	AV_UNUSED(self);
	AV_UNUSED(ppvideodecoder);
	return AV_ESUPPORTED;
}

static void av_media_close(av_media_p self)
{
	AV_UNUSED(self);
}

static int64_t av_media_clock(struct av_media* self)
{
	AV_UNUSED(self);
	return (int64_t)0;
}

static av_result_t av_media_synchronize_video(struct av_media* self, av_frame_video_p pvideoframe)
{
	AV_UNUSED(self);
	AV_UNUSED(pvideoframe);
	return AV_ESUPPORTED;
}

static av_result_t av_media_synchronize_audio(struct av_media* self, av_frame_audio_p paudioframe)
{
	AV_UNUSED(self);
	AV_UNUSED(paudioframe);
	return AV_ESUPPORTED;
}

static av_result_t av_media_seek(struct av_media* self, double pos_seconds)
{
	AV_UNUSED(self);
	AV_UNUSED(pos_seconds);
	return AV_ESUPPORTED;
}

static av_result_t av_media_constructor(av_object_p pobject)
{
	av_media_p self         = (av_media_p)pobject;
	self->open              = av_media_open;
	self->get_audio_info    = av_media_get_audio_info;
	self->get_video_info    = av_media_get_video_info;
	self->read_packet       = av_media_read_packet;
	self->get_audio_decoder = av_media_get_audio_decoder;
	self->get_video_decoder = av_media_get_video_decoder;
	self->clock             = av_media_clock;
	self->close             = av_media_close;
	self->synchronize_video = av_media_synchronize_video;
	self->synchronize_audio = av_media_synchronize_audio;
	self->seek              = av_media_seek;

	return AV_OK;
}

av_result_t av_media_register_torba(void)
{
	av_result_t rc;

	if (AV_OK != (rc = av_torb_register_class("media",
											  AV_NULL,
											  sizeof(av_media_t),
											  av_media_constructor, AV_NULL)))
		return rc;

	return AV_OK;
}
