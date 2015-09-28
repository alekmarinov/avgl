/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_audio_dfb.c                                     */
/* Description:   DirectFB audio class                               */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_DFB

#include <malloc.h>
#include <av_audio.h>
#include <av_media.h>

#define CONTEXT "audio_dfb_ctx"
#define O_context(o) O_attr(o, CONTEXT)

/* exported prototype */
av_result_t av_audio_dfb_register_torba(void);

typedef struct
{
	av_bool_t paused;
} av_audio_dfb_ctx_t, *av_audio_dfb_ctx_p;

static av_result_t av_audio_dfb_set_media(struct av_audio* self, struct av_media* media)
{
	AV_UNUSED(self);
	AV_UNUSED(media);
	return AV_ESUPPORTED;
}

static av_result_t av_audio_dfb_set_user_callback(struct av_audio* self, av_audio_callback_t cb, void* cb_data)
{
	AV_UNUSED(self);
	AV_UNUSED(cb);
	AV_UNUSED(cb_data);
	return AV_ESUPPORTED;
}

/* TODO: to be implemented */
static av_result_t av_audio_dfb_init(av_audio_p self)
{
	AV_UNUSED(self);
	return AV_ESUPPORTED;
}

/* TODO: to be implemented */
static av_result_t av_audio_dfb_write(av_audio_p self, av_frame_audio_p frame_audio)
{
	AV_UNUSED(self);
	AV_UNUSED(frame_audio);
	return AV_ESUPPORTED;
}

/* TODO: to be implemented */
static av_result_t av_audio_dfb_pause(av_audio_p self)
{
	av_audio_dfb_ctx_p ctx = (av_audio_dfb_ctx_p)O_context(self);
	ctx->paused = !ctx->paused;
	return AV_ESUPPORTED;
}

static av_bool_t av_audio_dfb_is_paused(av_audio_p self)
{
	av_audio_dfb_ctx_p ctx = (av_audio_dfb_ctx_p)O_context(self);
	return ctx->paused;
}

static av_result_t av_audio_dfb_stop(av_audio_p self)
{
	AV_UNUSED(self);
	return AV_ESUPPORTED;
}

/*	Initializes memory given by the input pointer with the audio class information */
static av_result_t av_audio_dfb_constructor(av_object_p object)
{
	av_audio_p self = (av_audio_p)object;
	av_audio_dfb_ctx_p ctx = (av_audio_dfb_ctx_p)malloc(sizeof(av_audio_dfb_ctx_t));

	if (!ctx)
	{
		return AV_EMEM;
	}

	ctx->paused = AV_TRUE;

	O_set_attr(self, CONTEXT, ctx);
	/* NOTE: inherited self->get_samplerate    = av_audio_get_samplerate; */
	/* NOTE: inherited self->get_channels      = av_audio_get_channels; */
	/* NOTE: inherited self->get_format        = av_audio_get_format; */
	self->set_media             = av_audio_dfb_set_media;
	self->set_user_callback     = av_audio_dfb_set_user_callback;
	self->init                  = av_audio_dfb_init;
	self->write                 = av_audio_dfb_write;
	self->pause                 = av_audio_dfb_pause;
	self->is_paused             = av_audio_dfb_is_paused;
	self->stop                  = av_audio_dfb_stop;

	return AV_OK;
}

/*	Registers dfb audio class into TORBA class repository */
av_result_t av_audio_dfb_register_torba(void)
{
	return av_torb_register_class("audio_dfb", "audio", sizeof(av_audio_t), av_audio_dfb_constructor, AV_NULL);
}

#endif /* WITH_SYSTEM_DFB */
