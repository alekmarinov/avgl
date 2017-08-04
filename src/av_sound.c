/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_sound.c                                         */
/* Description:   Abstract sound class                               */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
/* #include <mem.h> --> alek: is this necessary? how provide it? */

#include <av_torb.h>
#include <av_audio.h>
#include <av_sound.h>
#include <av_media.h>
#include <av_system.h>
#include <av_thread.h>

typedef av_bool_t (*av_sound_format_open_fn)(const char* filename, struct av_sound_handle** pphandle);
typedef void (*av_sound_format_close_fn)(struct av_sound_handle* phandle);
typedef av_bool_t (*av_sound_format_setformat_fn)(int format, struct av_sound_handle* phandle, void* psamples, int size, int* psize);
typedef av_result_t (*av_sound_format_getinfo_fn)(struct av_sound_handle* phandle, av_sound_info_p pinfo);

/* Interface to external format handlers */
extern av_bool_t av_sound_format_open_wave(const char* filename, struct av_sound_handle** pphandle);
extern void av_sound_format_close_wave(struct av_sound_handle* phandle);
extern av_bool_t av_sound_format_setformat_wave(int format, struct av_sound_handle* phandle, void* psamples, int size, int* psize);
extern av_result_t av_sound_format_getinfo_wave(struct av_sound_handle* phandle, av_sound_info_p pinfo);

typedef struct av_sound_context
{
	int id;
	av_sound_info_t info;
	av_bool_t is_playing;
	struct av_sound_handle* pshandle;
	struct av_audio_handle* pahandle;
} av_sound_context_t, *av_sound_context_p;

static struct
{
	av_sound_format_open_fn open;
	av_sound_format_close_fn close;
	av_sound_format_setformat_fn setformat;
	av_sound_format_getinfo_fn getinfo;
}
av_sound_format[] =
{
	{/*! WAVE format handler */
		av_sound_format_open_wave,
		av_sound_format_close_wave,
		av_sound_format_setformat_wave,
		av_sound_format_getinfo_wave,
	},
};

static int av_sound_format_count = sizeof(av_sound_format)/sizeof(av_sound_format[0]);

static av_audio_p audio = AV_NULL;
static av_timer_p timer = AV_NULL;

static av_result_t av_sound_open(struct av_sound* self, const char* filename, struct av_sound_handle** pphandle)
{
	int i;
	for (i=0; i<av_sound_format_count; i++)
	{
		av_sound_handle_p p;
		if (av_sound_format[i].open(filename, &p))
		{
			av_sound_context_p ctx = malloc(sizeof(av_sound_context_t));
			if (!ctx)
			{
				av_sound_format[i].close(p);
				return AV_EMEM;
			}
			ctx->id = i;
			av_sound_format[i].getinfo(p, &ctx->info);
			ctx->is_playing = AV_FALSE;
			ctx->pahandle = AV_NULL;
			ctx->pshandle = p;

			*pphandle = (struct av_sound_handle*)ctx;

			return AV_OK;
		}
	}
	AV_UNUSED(self);
	return AV_ESUPPORTED;
}

static av_bool_t av_sound_finish_cb(void* arg)
{
	av_sound_context_p ctx = (av_sound_context_p)arg;
	if (ctx->is_playing)
	{
		ctx->is_playing = AV_FALSE;
		audio->close(audio, ctx->pahandle);
	}
	return AV_FALSE;
}

static av_result_t av_sound_play(struct av_sound* self, struct av_sound_handle* phandle)
{
	av_sound_context_p ctx = (av_sound_context_p)phandle;

	if (ctx && !ctx->is_playing)
	{
		int len;
		av_result_t rc;
		av_bool_t has_moredata;
		av_audio_format_t format;
		av_frame_audio_t frame_audio;
		audio->get_format(audio, &format);

		if (AV_OK != (rc = audio->open(audio, ctx->info.samplerate, ctx->info.channels, ctx->info.format, &ctx->pahandle)))
		{
			return rc;
		}

		if (AV_OK != (rc = audio->enable(audio)))
		{
			return rc;
		}

		do
		{
			has_moredata = av_sound_format[ctx->id].setformat(format, ctx->pshandle, &frame_audio.data, sizeof(frame_audio.data), &len);
			frame_audio.size = len;
			audio->write(audio, ctx->pahandle, &frame_audio);
		}
		while(has_moredata);

		timer->add_timer(timer, av_sound_finish_cb, ctx->info.duration_ms, ctx, 0);
		audio->play(audio, ctx->pahandle);
		ctx->is_playing = AV_TRUE;
	}
	AV_UNUSED(self);
	return AV_OK;
}

static av_result_t av_sound_close(struct av_sound* self, struct av_sound_handle* phandle)
{
	av_sound_context_p ctx = (av_sound_context_p)phandle;

	if (ctx)
	{
		if (ctx->is_playing)
		{
			ctx->is_playing = AV_FALSE;
			audio->close(audio, ctx->pahandle);
		}
		av_sound_format[ctx->id].close(ctx->pshandle);
		free(ctx);
	}
	AV_UNUSED(self);

	return AV_OK;
}

static av_bool_t av_sound_is_playing(struct av_sound* self, struct av_sound_handle* phandle)
{
	av_sound_context_p ctx = (av_sound_context_p)phandle;
	AV_UNUSED(self);
	return ctx->is_playing;
}

/* Initializes memory given by the input pointer with the sound's class information */
static av_result_t av_sound_constructor(av_object_p psound)
{
	av_result_t rc;
	av_system_p sys;

	av_sound_p self  = (av_sound_p)psound;
	self->open       = av_sound_open;
	self->play       = av_sound_play;
	self->close      = av_sound_close;
	self->is_playing = av_sound_is_playing;

	/* get reference to system service */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
		return rc;
	sys->get_audio(sys, &audio);
	sys->get_timer(sys, &timer);
	av_torb_service_release("system");

	return AV_OK;
}

/* Registers sound class into TORBA class repository */
av_result_t av_sound_register_torba(void)
{
	return av_torb_register_class("sound", AV_NULL, sizeof(av_sound_t), av_sound_constructor, AV_NULL);
}
