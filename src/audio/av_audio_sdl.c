/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_audio_sdl.c                                     */
/* Description:   SDL audio class                                    */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_SDL

#include <malloc.h>
#include <av_log.h>
#include <av_audio.h>
#include <av_media.h>
#include <av_thread.h>

#include <SDL.h>

#define SDL_AUDIO_SILENCE 0
#define SDL_AUDIO_QUEUE_SIZE 16
#define SDL_AUDIO_BUFFER_SIZE AV_HW_AUDIO_BUFFER_SIZE

#define av_offsetof(strctr,membr) (((char*)strctr->membr)-((char*)strctr))

#define CONTEXT "audio_sdl_ctx"
#define O_context(o) O_attr(o, CONTEXT)

/* exported prototype */
av_result_t av_audio_sdl_register_torba(void);

typedef struct
{
	av_mutex_p mtx;
	av_bool_t enabled;
} av_audio_sdl_ctx_t, *av_audio_sdl_ctx_p;

typedef struct av_audio_handle
{
	SDL_AudioCVT cvt;
	av_bool_t has_cvt;
	av_bool_t is_paused;

	av_media_p media;
	av_sync_queue_p queue;
	void* user_callback_data;
	av_audio_callback_t user_callback;
	av_frame_audio_p frame_audio;
	int frame_audio_out_index;
	av_frame_audio_t frame_audio_out;

	struct av_audio_handle* next;
} av_audio_handle_t, *av_audio_handle_p;

static av_log_p sys_log = AV_NULL;
static av_audio_handle_p av_audio_root = AV_NULL;

static int SDL_AUDIO_FORMAT(av_audio_format_t format)
{
	switch(format)
	{
		default: sys_log->error(sys_log, "Unhandled audio format: '%d'", format);
		case AV_AUDIO_SIGNED_16: format = AUDIO_S16; break;
		case AV_AUDIO_UNSIGNED_16: format = AUDIO_U16; break;
		case AV_AUDIO_SIGNED_8: format = AUDIO_S8; break;
		case AV_AUDIO_UNSIGNED_8: format = AUDIO_U8; break;
	}
	return format;
}

/*
* This function is called when the audio device needs more data.
* data is a pointer to the audio data buffer
* length is the length of that buffer in bytes.
*
* Once the callback returns, the buffer will no longer be valid.
* Stereo samples are stored in a LRLRLR ordering.
*/
static void default_sdl_audio_callback(void* userdata, unsigned char* data, int length)
{
	int size;
	unsigned char* src;
	unsigned char* dst = data;
	av_audio_handle_p p = (av_audio_handle_p)userdata;

	av_assert(length < AV_AUDIO_FRAME_SIZE, "av_frame_audio_p->buffer too small");

	while (length > 0)
	{
		while (p->frame_audio_out.size > 0 && length > 0)
		{
			int i;
			signed short int *d, *s;
			size = AV_MIN(p->frame_audio_out.size, length);
			src = p->frame_audio_out.data;
			src += p->frame_audio_out_index;

			/* FIXME: mixer routines */
			d = (signed short int *)dst;
			s = (signed short int *)src;
			for (i=0;i<size/2;i++) d[i] = (d[i]+s[i])/2;

			dst += size;
			length -= size;
			p->frame_audio_out.size -= size;
			p->frame_audio_out_index += size;
		}

		if (0 == length)
			break;

		if (0 < p->queue->size(p->queue))
		{
			if (AV_OK == p->queue->pop(p->queue, (void**)&p->frame_audio))
			{
				if (p->media)
					p->media->synchronize_audio(p->media, p->frame_audio);
				memcpy(&p->frame_audio_out, p->frame_audio, av_offsetof(p->frame_audio,data) + p->frame_audio->size);
				p->frame_audio_out_index = 0;
				free(p->frame_audio);
			}
			else
				break;
		}
		else
			break;
	}
}

static void sdl_audio_callback(void *userdata, unsigned char* data, int length)
{
	av_audio_handle_p p;
	av_audio_sdl_ctx_p ctx = (av_audio_sdl_ctx_p)O_context(userdata);

	if (ctx->mtx)
	{
		ctx->mtx->lock(ctx->mtx);
		for (p = av_audio_root; p; p = p->next)
		{
			if (!p->is_paused)
			{
				if (p->user_callback == default_sdl_audio_callback)
				{
					default_sdl_audio_callback(p, data, length);
				}
				else
				{
					av_assert(0!=p->user_callback, "audio->user_callback can't be null");
					p->user_callback(p->user_callback_data, data, length);
				}
			}
		}
		ctx->mtx->unlock(ctx->mtx);
	}
}

static av_result_t av_audio_sdl_set_media(av_audio_p self, struct av_audio_handle* phandle, struct av_media* media)
{
	AV_UNUSED(self);
	phandle->media = media;
	return AV_OK;
}

static av_result_t av_audio_sdl_set_user_callback(struct av_audio* self, struct av_audio_handle* phandle, av_audio_callback_t cb, void* cb_data)
{
	AV_UNUSED(self);
	phandle->user_callback = cb;
	phandle->user_callback_data = cb_data;
	return AV_OK;
}

static av_result_t av_audio_sdl_write(av_audio_p self, struct av_audio_handle* phandle, av_frame_audio_p frame_audio)
{
	av_audio_sdl_ctx_p ctx = (av_audio_sdl_ctx_p)O_context(self);
	if (ctx->enabled)
	{
		av_frame_audio_p frame_audio_new;
		if (phandle->has_cvt)
		{
			int length;
			unsigned char* src;
			phandle->cvt.len = frame_audio->size;
			phandle->cvt.buf = malloc(frame_audio->size * phandle->cvt.len_mult);
			if (!phandle->cvt.buf)
				return AV_EMEM;
			memcpy(phandle->cvt.buf, &frame_audio->data, frame_audio->size);
			SDL_ConvertAudio(&phandle->cvt);
			src = phandle->cvt.buf;
			length = frame_audio->size * phandle->cvt.len_ratio;
			while (length > 0)
			{
				int size;
				frame_audio_new = (av_frame_audio_p)malloc(sizeof(av_frame_audio_t));
				if (!frame_audio_new)
					return AV_EMEM;
				size = AV_MIN(AV_AUDIO_FRAME_SIZE,length);
				/* FIXME: adjust pts */
				memcpy(frame_audio_new, frame_audio, av_offsetof(frame_audio,data));
				memcpy(frame_audio_new->data, src, size);
				frame_audio_new->size = size;
				phandle->queue->push(phandle->queue, frame_audio_new);
				length -= size;
				src += size;
			}
			free(phandle->cvt.buf);
		}
		else
		{
			frame_audio_new = (av_frame_audio_p)malloc(av_offsetof(frame_audio,data) + frame_audio->size);
			if (!frame_audio_new)
				return AV_EMEM;
			memcpy(frame_audio_new, frame_audio, av_offsetof(frame_audio,data) + frame_audio->size);
			phandle->queue->push(phandle->queue, frame_audio_new);
		}
	}
	return AV_OK;
}

static av_result_t av_audio_sdl_play(av_audio_p self, struct av_audio_handle* phandle)
{
	av_audio_sdl_ctx_p ctx = (av_audio_sdl_ctx_p)O_context(self);
	/* starts the callback */
	if (ctx->mtx)
	{
		if (ctx->enabled)
		{
			ctx->mtx->lock(ctx->mtx);
			SDL_PauseAudio(0);
			phandle->is_paused = AV_FALSE;
			ctx->mtx->unlock(ctx->mtx);
		}
	}
	return AV_OK;
}

static av_result_t av_audio_sdl_pause(av_audio_p self, struct av_audio_handle* phandle)
{
	av_audio_sdl_ctx_p ctx = (av_audio_sdl_ctx_p)O_context(self);
	if (ctx->mtx)
	{
		ctx->mtx->lock(ctx->mtx);
		phandle->is_paused = AV_TRUE;
		ctx->mtx->unlock(ctx->mtx);
	}
	return AV_OK;
}

static av_bool_t av_audio_sdl_is_paused(av_audio_p self, struct av_audio_handle* phandle)
{
	AV_UNUSED(self);
	return phandle->is_paused;
}

static av_result_t av_audio_sdl_open(struct av_audio* self, int samplerate, int channels, av_audio_format_t format, struct av_audio_handle** pphandle)
{
	av_audio_format_t defformat;
	int defsamplerate;
	int defchannels;
	av_result_t rc;
	av_audio_handle_p phandle = (av_audio_handle_p)malloc(sizeof(av_audio_handle_t));
	av_audio_sdl_ctx_p ctx = (av_audio_sdl_ctx_p)O_context(self);

	phandle = (av_audio_handle_p)malloc(sizeof(av_audio_handle_t));
	if (!phandle)
	{
		return AV_EMEM;
	}

	if (AV_OK != (rc = av_sync_queue_create(SDL_AUDIO_QUEUE_SIZE, &phandle->queue)))
	{
		free(phandle);
		return rc;
	}

	phandle->media = AV_NULL;
	phandle->is_paused = AV_TRUE;
	phandle->user_callback_data = phandle;
	phandle->user_callback = default_sdl_audio_callback;
	phandle->frame_audio_out.size = 0;
	phandle->frame_audio_out_index = 0;
	phandle->next = AV_NULL;

	self->get_samplerate(self, &defsamplerate);
	self->get_channels(self, &defchannels);
	self->get_format(self, &defformat);

	if (samplerate==defsamplerate && channels==defchannels && format==defformat)
	{
		phandle->has_cvt = AV_FALSE;
	}
	else
	{
		phandle->has_cvt = AV_TRUE;

		#if 0
		sys_log->debug(sys_log, "in format      : %d", SDL_AUDIO_FORMAT(format));
		sys_log->debug(sys_log, "in channels    : %d", channels);
		sys_log->debug(sys_log, "in samplerate  : %d", samplerate);
		sys_log->debug(sys_log, "out format     : %d", SDL_AUDIO_FORMAT(defformat));
		sys_log->debug(sys_log, "out channels   : %d", defchannels);
		sys_log->debug(sys_log, "out samplerate : %d", defsamplerate);
		#endif
		if (0 > SDL_BuildAudioCVT(&phandle->cvt, SDL_AUDIO_FORMAT(format), channels, samplerate,
			SDL_AUDIO_FORMAT(defformat), defchannels, defsamplerate))
		{
			free(phandle);
			sys_log->error(sys_log, "SDL_BuildAudioCVT failed: '%s'", SDL_GetError());
			return AV_ESUPPORTED;
		}
	}

	if (ctx->mtx)
	{
		ctx->mtx->lock(ctx->mtx);
		if (av_audio_root)
		{
			av_audio_handle_p p = av_audio_root;
			while (p->next) p = p->next;
			p->next = phandle;
		}
		else
		{
			av_audio_root = phandle;
		}
		ctx->mtx->unlock(ctx->mtx);
	}
	*pphandle =  phandle;

	return AV_OK;
}

static av_result_t av_audio_sdl_close(struct av_audio* self, struct av_audio_handle* phandle)
{
	av_audio_sdl_ctx_p ctx = (av_audio_sdl_ctx_p)O_context(self);
	if (ctx->mtx)
	{
		phandle->queue->abort(phandle->queue);
		ctx->mtx->lock(ctx->mtx);
		if (phandle)
		{
			if (av_audio_root)
			{
				if (av_audio_root == phandle)
				{
					av_audio_root = phandle->next;
				}
				else
				{
					av_audio_handle_p p = av_audio_root;
					while (p)
					{
						if (p->next == phandle)
						{
							p->next = phandle->next;
							break;
						}
						p = p->next;
					}
				}
			}
			free(phandle);
		}
		/* stops the callback if no more open handles */
		if (!av_audio_root)
			if (ctx->enabled)
				SDL_PauseAudio(1);

		ctx->mtx->unlock(ctx->mtx);
	}

	return AV_OK;
}


/* Destructor */
static void av_audio_sdl_destructor(void* object)
{
	av_audio_p self = (av_audio_p)object;
	av_audio_sdl_ctx_p ctx = (av_audio_sdl_ctx_p)O_context(self);
	if (ctx)
	{
		while (av_audio_root)
			av_audio_sdl_close(self, av_audio_root);

		if (ctx->mtx)
		{
			self->disable(self);
			ctx->mtx->destroy(ctx->mtx);
			ctx->mtx = AV_NULL;
		}
		av_torb_service_release("log");
		free(ctx);
	}
}

static void av_audio_sdl_disable(av_audio_p self)
{
	av_audio_sdl_ctx_p ctx = (av_audio_sdl_ctx_p)O_context(self);
	if (ctx->enabled)
	{
		ctx->mtx->lock(ctx->mtx);
		SDL_CloseAudio();
		ctx->mtx->unlock(ctx->mtx);
		ctx->enabled = AV_FALSE;
	}
}

static av_result_t av_audio_sdl_enable(av_audio_p self)
{
	int freq;
	int format;
	int nchannels;
	SDL_AudioSpec desired;
	SDL_AudioSpec actual;
	av_audio_sdl_ctx_p ctx = (av_audio_sdl_ctx_p)O_context(self);

	if (!ctx->enabled)
	{
		memset(&desired, 0, sizeof(SDL_AudioSpec));
		memset(&actual, 0, sizeof(SDL_AudioSpec));

		self->get_samplerate(self, &freq);
		desired.freq = freq;

		self->get_channels(self, &nchannels);
		desired.channels = nchannels;
		desired.callback = sdl_audio_callback;
		desired.userdata = self;

		self->get_format(self, &format);
		desired.format = SDL_AUDIO_FORMAT(format);
		desired.silence = SDL_AUDIO_SILENCE;
		desired.samples = SDL_AUDIO_BUFFER_SIZE;

		if (SDL_OpenAudio(&desired, &actual) < 0)
		{
			sys_log->error(sys_log, "SDL_OpenAudio failed: '%s'", SDL_GetError());
			goto enable_error;
		}

		if (desired.format != actual.format)
		{
			sys_log->error(sys_log, "Unsupported audio format: '%d'", desired.format);
			goto enable_error_close;
		}

		if (desired.channels != actual.channels)
		{
			sys_log->error(sys_log, "Unsupported audio channels: '%d'", desired.channels);
			goto enable_error_close;
		}

		if (desired.freq != actual.freq)
		{
			sys_log->error(sys_log, "Unsupported audio samplerate: '%d'", desired.freq);
			goto enable_error_close;
		}

		ctx->enabled = AV_TRUE;
	}
	return AV_OK;

enable_error_close:
	SDL_CloseAudio();

enable_error:
	return AV_ESUPPORTED;
}

/* Constructor */
static av_result_t av_audio_sdl_constructor(av_object_p object)
{
	av_result_t rc;
	av_audio_p self = (av_audio_p)object;
	av_audio_sdl_ctx_p ctx = (av_audio_sdl_ctx_p)malloc(sizeof(av_audio_sdl_ctx_t));

	if (!ctx)
	{
		return AV_EMEM;
	}

	ctx->mtx = AV_NULL;
	ctx->enabled = AV_FALSE;
	av_mutex_create(&ctx->mtx);

	O_set_attr(self, CONTEXT, ctx);
	/* NOTE: inherited self->get_samplerate    = av_audio_get_samplerate; */
	/* NOTE: inherited self->get_channels      = av_audio_get_channels; */
	/* NOTE: inherited self->get_format        = av_audio_get_format; */
	self->set_media             = av_audio_sdl_set_media;
	self->set_user_callback     = av_audio_sdl_set_user_callback;
	self->open                  = av_audio_sdl_open;
	self->close                 = av_audio_sdl_close;
	self->write                 = av_audio_sdl_write;
	self->play                  = av_audio_sdl_play;
	self->pause                 = av_audio_sdl_pause;
	self->is_paused             = av_audio_sdl_is_paused;
	self->enable                = av_audio_sdl_enable;
	self->disable               = av_audio_sdl_disable;

	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&sys_log)))
	{
		free(ctx);
		return rc;
	}

	return AV_OK;
}

/* Registers sdl audio class into TORBA class repository */
av_result_t av_audio_sdl_register_torba(void)
{
	return av_torb_register_class("audio_sdl", "audio", sizeof(av_audio_t),
								  av_audio_sdl_constructor, av_audio_sdl_destructor);
}

#endif /* WITH_SYSTEM_SDL */
