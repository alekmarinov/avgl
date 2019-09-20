/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_audio.c                                         */
/* Description:   Abstract audio class                               */
/*                                                                   */
/*********************************************************************/

#ifdef _WIN32
#  define strcasecmp _stricmp
#else
#  include <strings.h>
#endif

#include <av_torb.h>
#include <av_audio.h>
#include <av_media.h>
#include <av_prefs.h>

/** \brief Bits per second of audio hardware output (default 16 bit signed) */
static av_audio_format_t av_audio_format = AV_AUDIO_SIGNED_16;

/** \brief Number of output channels (default 2 - stereo) */
static int av_audio_channels = 2;

/** \brief Sound samples frequency (default 22050) */
static int av_audio_samplerate = 22050;

static av_result_t av_audio_get_samplerate(av_audio_p self, int* samplerate)
{
	AV_UNUSED(self);
	*samplerate = av_audio_samplerate;
	return AV_OK;
}

static av_result_t av_audio_get_channels(av_audio_p self, int* channels)
{
	AV_UNUSED(self);
	*channels = av_audio_channels;
	return AV_OK;
}

static av_result_t av_audio_get_format(av_audio_p self, av_audio_format_p format)
{
	AV_UNUSED(self);
	*format = av_audio_format;
	return AV_OK;
}

static av_result_t av_audio_set_media(av_audio_p self, struct av_audio_handle* phandle, struct av_media* media)
{
	AV_UNUSED(self);
	AV_UNUSED(phandle);
	AV_UNUSED(media);
	return AV_ESUPPORTED;
}

static av_result_t av_audio_set_user_callback(struct av_audio* self, struct av_audio_handle* phandle, av_audio_callback_t cb, void* cb_data)
{
	AV_UNUSED(self);
	AV_UNUSED(phandle);
	AV_UNUSED(cb);
	AV_UNUSED(cb_data);
	return AV_ESUPPORTED;
}

static av_result_t av_audio_open(struct av_audio* self, int samplerate, int channels, int format, struct av_audio_handle** pphandle)
{
	AV_UNUSED(self);
	AV_UNUSED(samplerate);
	AV_UNUSED(channels);
	AV_UNUSED(format);
	AV_UNUSED(pphandle);
	return AV_ESUPPORTED;
}

static av_result_t av_audio_close(struct av_audio* self, struct av_audio_handle* phandle)
{
	AV_UNUSED(self);
	AV_UNUSED(phandle);
	return AV_ESUPPORTED;
}

static av_result_t av_audio_write(av_audio_p self, struct av_audio_handle* phandle, av_frame_audio_p frame_audio)
{
	AV_UNUSED(self);
	AV_UNUSED(phandle);
	AV_UNUSED(frame_audio);
	return AV_ESUPPORTED;
}

static av_result_t av_audio_play(av_audio_p self, struct av_audio_handle* phandle)
{
	AV_UNUSED(self);
	AV_UNUSED(phandle);
	return AV_ESUPPORTED;
}

static av_result_t av_audio_pause(av_audio_p self, struct av_audio_handle* phandle)
{
	AV_UNUSED(self);
	AV_UNUSED(phandle);
	return AV_ESUPPORTED;
}

static av_bool_t av_audio_is_paused(av_audio_p self, struct av_audio_handle* phandle)
{
	AV_UNUSED(self);
	AV_UNUSED(phandle);
	return AV_FALSE;
}

/* Initializes memory given by the input pointer with the audio's class information */
static av_result_t av_audio_constructor(av_object_p paudio)
{
	av_prefs_p prefs;

	av_audio_p self         = (av_audio_p)paudio;
	self->get_samplerate    = av_audio_get_samplerate;
	self->get_channels      = av_audio_get_channels;
	self->get_format        = av_audio_get_format;
	self->set_media         = av_audio_set_media;
	self->set_user_callback = av_audio_set_user_callback;
	self->open              = av_audio_open;
	self->close             = av_audio_close;
	self->write             = av_audio_write;
	self->play              = av_audio_play;
	self->pause             = av_audio_pause;
	self->is_paused         = av_audio_is_paused;

	if (AV_OK == av_torb_service_addref("prefs", (av_service_p*)&prefs))
	{
		const char* val;
		prefs->get_int(prefs, "system.audio.samplerate", 22050, &av_audio_samplerate);
		prefs->get_int(prefs, "system.audio.channels", 2, &av_audio_channels);
		prefs->get_string(prefs, "system.audio.format", "s16", &val);
		if (!strcasecmp("s16", val))
			av_audio_format = AV_AUDIO_SIGNED_16;
		else
		if (!strcasecmp("u16", val))
			av_audio_format = AV_AUDIO_UNSIGNED_16;
		else
		if (!strcasecmp("s8", val))
			av_audio_format = AV_AUDIO_SIGNED_8;
		else
		if (!strcasecmp("u8", val))
			av_audio_format = AV_AUDIO_UNSIGNED_8;
		else
		{
			/* FIXME: handle other formats */
			av_audio_format = AV_AUDIO_SIGNED_16;
		}
		av_torb_service_release("prefs");
	}

	return AV_OK;
}

/* Registers audio class into TORBA class repository */
av_result_t av_audio_register_torba(void)
{
	return av_torb_register_class("audio", AV_NULL, sizeof(av_audio_t), av_audio_constructor, AV_NULL);
}
