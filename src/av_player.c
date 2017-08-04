/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_player.c                                        */
/* Description:   Multimedia player                                  */
/*                                                                   */
/* 11/10/07: The reported leaks/errors from valgrind                 */
/*           are caused by SDL and FFMPEG. Compare with mplayer      */
/* 11/10/07: bug-> Freeze on stop to be investigated                 */
/*********************************************************************/

#define WITH_FAST_VIDEO 0
#define WITH_THREAD_INTERRUPTION 0

#include <math.h>
#include <malloc.h>
#include <string.h>
#ifdef _WIN32
#  define strcasecmp _stricmp
#else
#  include <strings.h>
#endif
#include <av_timer.h>
#include <av_system.h>
#include <av_audio.h>
#include <av_prefs.h>
#include <av_player.h>
#include <av_graphics.h>
#include <av_media.h>
#include <av_thread.h>
#include <av_log.h>

#define MAX_AUDIO_QUEUE_SIZE (16 * 1024)
#define MAX_VIDEO_QUEUE_SIZE (256 * 1024)

#define EVENT_REFRESH_ID 1

#define CONTEXT "player_ctx"
#define O_context(o) O_attr(o, CONTEXT)

/* FIXME: add to preferences */
static const char* media_driver = "media_ffmpeg";
static const char* scaler_driver = "scaler_swscale";

av_result_t av_media_ffmpeg_register_torba(void);
av_result_t av_scaler_swscale_register_torba(void);

typedef struct av_player_ctx
{
	av_window_p owner;
	av_system_p system;
	av_log_p log;
	av_input_p input;
	av_video_p video;
	av_audio_p audio;
	av_media_p media;
	av_timer_p timer;
	av_graphics_p graphics;
	struct av_audio_handle* audio_handle;
	av_graphics_surface_p graphics_surface;
	av_thread_p thd_packet_reader;
	av_thread_p thd_audio_decoder;
	av_thread_p thd_video_decoder;
	av_decoder_audio_p audio_decoder;
	av_decoder_video_p video_decoder;
	av_sync_queue_p audio_queue;
	av_sync_queue_p video_queue;
	av_audio_info_t audio_info;
	av_video_info_t video_info;
	av_scale_info_t scale_info;
	av_scaler_p scaler;
	av_bool_t stop_request;
	av_bool_t stop_request_video;
	av_bool_t stop_request_audio;
	av_bool_t has_video;
	av_bool_t has_audio;
	av_bool_t has_subtitle;

	av_frame_video_p picture;
	av_bool_t picture_ready;
	av_mutex_p mtx_picture;
	av_condition_p cond_picture;

} av_player_ctx_t, *av_player_ctx_p;

/* Forward reference */
static void schedule_refresh(av_player_ctx_p ctx, unsigned int delay);

static int packet_reader_thread(av_thread_p thread)
{
	av_result_t rc;
	av_player_ctx_p ctx = thread->arg;
	av_packet_p packet;

	if(ctx->has_video)
	{
		schedule_refresh(ctx, 40);
		ctx->thd_video_decoder->start(ctx->thd_video_decoder);
	}

	if(ctx->has_audio)
	{
		ctx->thd_audio_decoder->start(ctx->thd_audio_decoder);
	}

	while (!ctx->stop_request && AV_OK == (rc = ctx->media->read_packet(ctx->media, &packet)))
	{
		#if WITH_THREAD_INTERRUPTION
		av_bool_t interrupted;
		#endif
		if (packet)
		{
			if (AV_PACKET_TYPE_AUDIO == packet->type)
			{
				if (AV_OK != ctx->audio_queue->push(ctx->audio_queue, packet))
				{
					packet->destroy(packet);
					break;
				}
			}
			else
			if (AV_PACKET_TYPE_VIDEO == packet->type)
			{
				if (AV_OK != ctx->video_queue->push(ctx->video_queue, packet))
				{
					packet->destroy(packet);
					break;
				}
			}
			else
				packet->destroy(packet);
		}
		#if WITH_THREAD_INTERRUPTION
		thread->is_interrupted(thread, &interrupted);
		if (interrupted || ctx->stop_request) break;
		else
		#endif
		ctx->timer->sleep_ms(1);
	}

	while (!ctx->stop_request)
		ctx->timer->sleep_ms(10);

	if (ctx->has_audio)
	{
		ctx->stop_request_audio = AV_TRUE;
		ctx->audio_queue->abort(ctx->audio_queue);
		#if WITH_THREAD_INTERRUPTION
		ctx->thd_audio_decoder->interrupt(ctx->thd_audio_decoder);
		#endif
		ctx->thd_audio_decoder->join(ctx->thd_audio_decoder);
	}

	if (ctx->has_video)
	{
		ctx->stop_request_video = AV_TRUE;
		ctx->video_queue->abort(ctx->video_queue);
		#if WITH_THREAD_INTERRUPTION
		ctx->thd_video_decoder->interrupt(ctx->thd_video_decoder);
		#endif
		ctx->thd_video_decoder->join(ctx->thd_video_decoder);
	}

	return 0;
}

static int audio_decoder_thread(av_thread_p thread)
{
	av_packet_p packet;
	av_player_ctx_p ctx = thread->arg;

	if (ctx->audio_handle)
	{
		ctx->audio->play(ctx->audio, ctx->audio_handle);

		while (AV_OK == ctx->audio_queue->pop(ctx->audio_queue, (void**)&packet))
		{
			#if WITH_THREAD_INTERRUPTION
			av_bool_t interrupted;
			#endif
			av_frame_audio_p audio_frame;

			audio_frame = AV_NULL;
			if (AV_OK == ctx->audio_decoder->decode(ctx->audio_decoder, packet, &audio_frame))
			{
				if (audio_frame)
				{
					ctx->audio->write(ctx->audio, ctx->audio_handle, audio_frame);
				}
			}
			packet->destroy(packet);
			#if WITH_THREAD_INTERRUPTION
			thread->is_interrupted(thread, &interrupted);
			if (interrupted) break;
			else
			#endif
			ctx->timer->sleep_ms(1);
		}

		while (!ctx->stop_request_audio)
			ctx->timer->sleep_ms(10);

		ctx->audio->close(ctx->audio, ctx->audio_handle);
		ctx->audio_handle = AV_NULL;
	}
	return 0;
}

static int video_decoder_thread(av_thread_p thread)
{
	av_packet_p packet;
	av_player_ctx_p ctx = thread->arg;

	while (AV_OK == ctx->video_queue->pop(ctx->video_queue, (void**)&packet))
	{
		#if WITH_THREAD_INTERRUPTION
		av_bool_t interrupted;
		#endif
		av_frame_video_p videoframe;

		videoframe = AV_NULL;
		ctx->mtx_picture->lock(ctx->mtx_picture);
		while (ctx->picture_ready)
		{
			ctx->cond_picture->wait(ctx->cond_picture, ctx->mtx_picture);
		}

		if (!ctx->stop_request_video)
		{
			ctx->video_decoder->decode(ctx->video_decoder, packet, &videoframe);
			if (videoframe)
			{
				ctx->picture_ready = AV_TRUE;
				ctx->picture = videoframe;
			}
		}
		packet->destroy(packet);
		ctx->mtx_picture->unlock(ctx->mtx_picture);
		#if WITH_THREAD_INTERRUPTION
		thread->is_interrupted(thread, &interrupted);
		if (interrupted) break;
		else
		#endif
		ctx->timer->sleep_ms(1);
	}

	while (!ctx->stop_request_video)
		ctx->timer->sleep_ms(10);

	return 0;
}

static av_bool_t schedule_refresh_cb(void* arg)
{
	av_player_ctx_p ctx = (av_player_ctx_p)arg;

	if (AV_FALSE == ctx->picture_ready)
	{
		if(!ctx->stop_request_video)
		{
			schedule_refresh(ctx, 1);
		}
	}
	else
	{
		av_event_t event;
		memset(&event, 0, sizeof(av_event_t));
		event.type = AV_EVENT_USER;
		event.user_id = EVENT_REFRESH_ID;
		event.window = ctx->owner;
		ctx->input->push_event(ctx->input, &event);
	}
	return AV_FALSE;
}

/* schedule a video refresh in 'delay' ms */
static void schedule_refresh(av_player_ctx_p ctx, unsigned int delay)
{
	ctx->timer->add_timer(ctx->timer, schedule_refresh_cb, delay, ctx, 0);
}

/* open media file */
static av_result_t av_player_open(av_player_p self, const char* url)
{
	av_result_t rc;
	av_rect_t rect;
	av_player_ctx_p ctx = (av_player_ctx_p)O_context(self);
	ctx->log->info(ctx->log, "player::open %s", url);
	if (AV_OK != (rc = ctx->media->open(ctx->media, url)))
		return rc;

	ctx->has_audio = (AV_OK == ctx->media->get_audio_decoder(ctx->media, &ctx->audio_decoder));
	ctx->has_audio = ctx->has_audio && (AV_OK == ctx->media->get_audio_info(ctx->media, &ctx->audio_info));

	ctx->has_video = (AV_OK == ctx->media->get_video_decoder(ctx->media, &ctx->video_decoder));
	ctx->has_video = ctx->has_video && (AV_OK == ctx->media->get_video_info(ctx->media, &ctx->video_info));

	if(!ctx->has_video && !ctx->has_audio)
	{
		ctx->media->close(ctx->media);
		return AV_EARG;
	}

	if(ctx->has_video)
	{
	    /* prepare video scaler*/
		ctx->owner->get_rect(ctx->owner, &rect);
		if (AV_OK != (rc = ((av_surface_p)ctx->graphics_surface)->set_size((av_surface_p)ctx->graphics_surface, rect.w, rect.h)))
		{
			ctx->media->close(ctx->media);
			return rc;
		}

		ctx->scale_info.src_format  = ctx->video_info.format;
		ctx->scale_info.src_width   = ctx->video_info.width;
		ctx->scale_info.src_height  = ctx->video_info.height;
		ctx->scale_info.dst_format  = AV_VIDEO_FORMAT_RGB32;
		ctx->scale_info.dst_width   = rect.w;
		ctx->scale_info.dst_height  = rect.h;
		ctx->log->info(ctx->log, "format = %d", ctx->scale_info.src_format);
		ctx->log->info(ctx->log, "width = %d, height = %d", ctx->scale_info.src_width, ctx->scale_info.src_height);
		ctx->scaler->set_configuration(ctx->scaler, &ctx->scale_info);
	}

	if(ctx->has_audio)
	{
	    /* prepare audio output */
    	if (AV_OK != (rc = ctx->audio->open(ctx->audio, ctx->audio_info.freq, ctx->audio_info.nchannels, ctx->audio_info.format, &ctx->audio_handle)))
		{
			ctx->audio_handle = AV_NULL;
			return rc;
		}
	}

	return AV_OK;
}

/* Control */
static av_result_t av_player_play(av_player_p self)
{
	av_player_ctx_p ctx = (av_player_ctx_p)O_context(self);
	ctx->log->info(ctx->log, "player::play");

	ctx->stop_request = AV_FALSE;
	ctx->stop_request_video = AV_FALSE;
	ctx->stop_request_audio = AV_FALSE;
	ctx->thd_packet_reader->start(ctx->thd_packet_reader);

	return AV_OK;
}

static av_result_t av_player_stop(av_player_p self)
{
	av_player_ctx_p ctx = (av_player_ctx_p)O_context(self);
	ctx->log->info(ctx->log, "player::stop");

	ctx->stop_request = AV_TRUE;
/* 	ctx->thd_packet_reader->interrupt(ctx->thd_packet_reader);*/
	ctx->thd_packet_reader->join(ctx->thd_packet_reader);

	return AV_OK;
}

static av_result_t av_player_pause(av_player_p self)
{
	av_player_ctx_p ctx = (av_player_ctx_p)O_context(self);
	ctx->log->info(ctx->log, "player::pause");
	if (ctx->audio_handle)
	ctx->audio->pause(ctx->audio, ctx->audio_handle);
	return AV_ESUPPORTED;
}

static av_bool_t av_player_on_paint(av_window_p self, av_graphics_p graphics)
{
	#if WITH_FAST_VIDEO
	AV_UNUSED(self);
	AV_UNUSED(graphics);
	#else
	av_player_ctx_p ctx = (av_player_ctx_p)O_context(self);
	if (ctx->picture)
	{
		ctx->scaler->scale(ctx->scaler, ctx->picture, (av_surface_p)ctx->graphics_surface);
		graphics->show_image(graphics, 0, 0, ctx->graphics_surface);
	}
	#endif
	return AV_TRUE;
}

static av_bool_t av_player_on_user(av_window_p self, int id, void* data)
{
	av_player_ctx_p ctx = (av_player_ctx_p)O_context(self);
	AV_UNUSED(id);
	AV_UNUSED(data);

	if (AV_FALSE == ctx->picture_ready)
	{
		if(!ctx->stop_request_video)
		{
			schedule_refresh(ctx, 1);
		}
	}
	else
	{
		/* A/V sync */
		ctx->media->synchronize_video(ctx->media,ctx->picture);
		/* If late picture - skip it */
		if (ctx->picture->delay_ms > 0)
		{
			schedule_refresh(ctx, ctx->picture->delay_ms);
			#if WITH_FAST_VIDEO
			av_graphics_surface_p target_surface;
			/* begin graphics  */
			ctx->graphics->begin(ctx->graphics, ctx->graphics_surface);
			/* scale directly on the graphics target surface */
			ctx->graphics->get_target_surface(ctx->graphics, &target_surface);
			ctx->scaler->scale(ctx->scaler, ctx->picture, (av_surface_p)target_surface);
			/* finish graphics */
			ctx->graphics->end(ctx->graphics);
			/* show backbuffer on screen */
			ctx->video->update(ctx->video, AV_NULL);
			#else
			/* show the picture on top of the picture queue */

			/* FIXME: Refactor!!! */
			/* ctx->owner->invalidate(ctx->owner); */
			ctx->owner->update(ctx->owner, AV_UPDATE_REPAINT);

			ctx->system->update(ctx->system);
			#endif
		}
		else
		{
			schedule_refresh(ctx, 1);
		}
		ctx->mtx_picture->lock(ctx->mtx_picture);
		ctx->picture_ready = AV_FALSE;
		ctx->cond_picture->signal(ctx->cond_picture);
		ctx->mtx_picture->unlock(ctx->mtx_picture);
	}

	return AV_TRUE;
}

static void av_player_set_window(av_player_p self, av_window_p window)
{
	av_player_ctx_p ctx = (av_player_ctx_p)O_context(self);
	ctx->owner = window;
	ctx->owner->on_paint = av_player_on_paint;
	ctx->owner->on_user  = av_player_on_user;

	O_set_attr(ctx->owner, CONTEXT, ctx);
}

static av_result_t av_player_seek(struct av_player* self, double pos_seconds)
{
	av_player_ctx_p ctx = (av_player_ctx_p)O_context(self);
	return ctx->media->seek(ctx->media,pos_seconds);
}

static av_result_t av_player_seek_rel(struct av_player* self, double pos_seconds)
{
	AV_UNUSED(self);
	AV_UNUSED(pos_seconds);
	return AV_ESUPPORTED;
}

static double av_player_duration(struct av_player* self)
{
	AV_UNUSED(self);
	return 0.;
}

static double av_player_position(struct av_player* self)
{
	AV_UNUSED(self);
	return 0.;
}

static av_result_t av_player_is_playing(struct av_player* self)
{
    AV_UNUSED(self);
    return avps_idle;
}

static av_result_t av_player_is_pause(struct av_player* self)
{
    AV_UNUSED(self);
    return avps_idle;
}

static void av_player_set_borders(av_player_p self, av_bool_t val)
{
	AV_UNUSED(self);
	AV_UNUSED(val);
}

/* Destructor */
static void av_player_destructor(void* pobject)
{
	av_player_ctx_p ctx = (av_player_ctx_p)O_context(pobject);

	ctx->video_queue->destroy(ctx->video_queue);
	ctx->audio_queue->destroy(ctx->audio_queue);
	ctx->cond_picture->destroy(ctx->cond_picture);
	ctx->mtx_picture->destroy(ctx->mtx_picture);
	ctx->thd_video_decoder->destroy(ctx->thd_video_decoder);
	ctx->thd_audio_decoder->destroy(ctx->thd_audio_decoder);
	ctx->thd_packet_reader->destroy(ctx->thd_packet_reader);
	O_destroy(ctx->scaler);
	O_destroy(ctx->media);
	O_destroy(ctx->graphics_surface);
	av_torb_service_release("log");
	av_torb_service_release("system");
	free(ctx);
}

/* Constructor */
static av_result_t av_player_constructor(av_object_p object)
{
	av_result_t rc;
	av_player_ctx_p ctx;
	av_player_p self = (av_player_p)object;
	av_system_p sys;

	/* initializes self */
	ctx = malloc(sizeof(av_player_ctx_t));
	if (!ctx) return AV_EMEM;

	memset(ctx, 0, sizeof(av_player_ctx_t));

	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
	{
		free(ctx);
		return rc;
	}

	ctx->system = sys;

	if (AV_OK != (rc = sys->get_timer(sys, &ctx->timer)))
	{
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = sys->get_input(sys, &ctx->input)))
	{
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = sys->get_audio(sys, &ctx->audio)))
	{
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = sys->get_video(sys, &ctx->video)))
	{
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = sys->get_graphics(sys, &ctx->graphics)))
	{
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	#if WITH_FAST_VIDEO
	/* setup graphics */
	av_surface_p surface;
	ctx->video->get_backbuffer(ctx->video, (av_video_surface_p*)&surface);
	ctx->graphics->wrap_surface(ctx->graphics, surface, &ctx->graphics_surface);
	#else
	if (AV_OK != (rc = ctx->graphics->create_surface(ctx->graphics, 1, 1, &ctx->graphics_surface)))
	{
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}
	#endif

	/* create media object */
	if (AV_OK != (rc = av_torb_create_object(media_driver, (av_object_p*)&ctx->media)))
	{
		O_destroy(ctx->graphics_surface);
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	/* create scaler object */
	if (AV_OK != (rc = av_torb_create_object(scaler_driver, (av_object_p*)&ctx->scaler)))
	{
		O_destroy(ctx->media);
		O_destroy(ctx->graphics_surface);
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	/* create packet reader thread */
	if (AV_OK != (rc = av_thread_create(packet_reader_thread, ctx, &ctx->thd_packet_reader)))
	{
		O_destroy(ctx->scaler);
		O_destroy(ctx->media);
		O_destroy(ctx->graphics_surface);
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	/* create audio decoder thread */
	if (AV_OK != (rc = av_thread_create(audio_decoder_thread, ctx, &ctx->thd_audio_decoder)))
	{
		ctx->thd_packet_reader->destroy(ctx->thd_packet_reader);
		O_destroy(ctx->scaler);
		O_destroy(ctx->media);
		O_destroy(ctx->graphics_surface);
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	/* create video decoder thread */
	if (AV_OK != (rc = av_thread_create(video_decoder_thread, ctx, &ctx->thd_video_decoder)))
	{
		ctx->thd_audio_decoder->destroy(ctx->thd_audio_decoder);
		ctx->thd_packet_reader->destroy(ctx->thd_packet_reader);
		O_destroy(ctx->scaler);
		O_destroy(ctx->media);
		O_destroy(ctx->graphics_surface);
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	/* create picture mutex */
	if (AV_OK != (rc = av_mutex_create(&ctx->mtx_picture)))
	{
		ctx->thd_video_decoder->destroy(ctx->thd_video_decoder);
		ctx->thd_audio_decoder->destroy(ctx->thd_audio_decoder);
		ctx->thd_packet_reader->destroy(ctx->thd_packet_reader);
		O_destroy(ctx->scaler);
		O_destroy(ctx->media);
		O_destroy(ctx->graphics_surface);
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	/* create picture condition */
	if (AV_OK != (rc = av_condition_create(&ctx->cond_picture)))
	{
		ctx->mtx_picture->destroy(ctx->mtx_picture);
		ctx->thd_video_decoder->destroy(ctx->thd_video_decoder);
		ctx->thd_audio_decoder->destroy(ctx->thd_audio_decoder);
		ctx->thd_packet_reader->destroy(ctx->thd_packet_reader);
		O_destroy(ctx->scaler);
		O_destroy(ctx->media);
		O_destroy(ctx->graphics_surface);
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = av_sync_queue_create(MAX_AUDIO_QUEUE_SIZE, &ctx->audio_queue)))
	{
		ctx->cond_picture->destroy(ctx->cond_picture);
		ctx->mtx_picture->destroy(ctx->mtx_picture);
		ctx->thd_video_decoder->destroy(ctx->thd_video_decoder);
		ctx->thd_audio_decoder->destroy(ctx->thd_audio_decoder);
		ctx->thd_packet_reader->destroy(ctx->thd_packet_reader);
		O_destroy(ctx->scaler);
		O_destroy(ctx->media);
		O_destroy(ctx->graphics_surface);
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = av_sync_queue_create(MAX_VIDEO_QUEUE_SIZE, &ctx->video_queue)))
	{
		ctx->audio_queue->destroy(ctx->audio_queue);
		ctx->cond_picture->destroy(ctx->cond_picture);
		ctx->mtx_picture->destroy(ctx->mtx_picture);
		ctx->thd_video_decoder->destroy(ctx->thd_video_decoder);
		ctx->thd_audio_decoder->destroy(ctx->thd_audio_decoder);
		ctx->thd_packet_reader->destroy(ctx->thd_packet_reader);
		O_destroy(ctx->scaler);
		O_destroy(ctx->media);
		O_destroy(ctx->graphics_surface);
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&ctx->log)))
	{
		ctx->video_queue->destroy(ctx->video_queue);
		ctx->audio_queue->destroy(ctx->audio_queue);
		ctx->cond_picture->destroy(ctx->cond_picture);
		ctx->mtx_picture->destroy(ctx->mtx_picture);
		ctx->thd_video_decoder->destroy(ctx->thd_video_decoder);
		ctx->thd_audio_decoder->destroy(ctx->thd_audio_decoder);
		ctx->thd_packet_reader->destroy(ctx->thd_packet_reader);
		O_destroy(ctx->scaler);
		O_destroy(ctx->media);
		O_destroy(ctx->graphics_surface);
		av_torb_service_release("system");
		free(ctx);
		return rc;
	}

	O_set_attr(self, CONTEXT, ctx);
	self->open         = av_player_open;
	self->play         = av_player_play;
	self->stop         = av_player_stop;
	self->pause        = av_player_pause;
	self->set_window   = av_player_set_window;
	self->seek         = av_player_seek;
	self->seek_rel     = av_player_seek_rel;
	self->duration     = av_player_duration;
	self->position     = av_player_position;
	self->is_playing   = av_player_is_playing;
	self->is_pause     = av_player_is_pause;
	self->set_borders  = av_player_set_borders;
	return AV_OK;
}

#ifdef WITH_XINE
AV_API av_result_t av_player_xine_register_torba(void);
#endif

#ifdef WITH_FFMPEG
AV_API av_result_t av_player_ffplay_register_torba(void);
#endif

#ifdef WITH_VLC
AV_API av_result_t av_player_vlc_register_torba(void);
#endif

av_result_t av_player_register_torba(void)
{
	av_prefs_p prefs;
	const char* player;
	if (AV_OK == av_torb_service_addref("prefs", (av_service_p*)&prefs))
	{
		prefs->get_string(prefs, "player.backend", "player", &player);
		prefs->set_string(prefs, "player.backend", player);
		av_torb_service_release("prefs");
	}

	/* FIXME: move these to appropriate place... */
	av_scaler_register_torba();
    #ifdef WITH_FFMPEG
	av_scaler_swscale_register_torba();
    #endif
	av_media_register_torba();
    #ifdef WITH_FFMPEG
	av_media_ffmpeg_register_torba();
    #endif

	#ifdef WITH_XINE
	if (!strcasecmp(player,"xine"))
	{
		return av_player_xine_register_torba();
	}
	#endif

	#ifdef WITH_FFMPEG
	if (!strcasecmp(player,"ffplay"))
	{
		return av_player_ffplay_register_torba();
	}
	#endif

	#ifdef WITH_VLC
	if (!strcasecmp(player,"vlc"))
	{
		return av_player_vlc_register_torba();
	}
	#endif

	return av_torb_register_class("player", AV_NULL, sizeof(av_player_t), av_player_constructor, av_player_destructor);
}
