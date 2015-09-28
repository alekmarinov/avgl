/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_media_ffmpeg.c                                  */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_FFMPEG

#define WITH_DEBUG_SYNCH 0
#define WITH_DEBUG_SYNCH_VIDEO 1
#define WITH_DEBUG_SYNCH_AUDIO 1
#define WITH_DEBUG_SYNCH_AUDIO_DECODER 0

#include <math.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_INCLUDE_FFMPEG
	#include <ffmpeg/avformat.h>
#endif

#ifdef USE_INCLUDE_LIBAVFORMAT
	#include <libavformat/avformat.h>
#endif

#include <av_torb.h>
#include <av_log.h>
#include <av_audio.h>
#include <av_media.h>

#define AV_STREAM_NONE (-1)
#define AV_INVALID_TIME (-1)

#define CONTEXT "media_ffmpeg_ctx"
#define O_context(o) O_attr(o, CONTEXT)

av_result_t av_media_ffmpeg_register_torba(void);

static av_bool_t g_ffmpeg_registered = AV_FALSE;
static av_log_p _log = AV_NULL;

/* Error messages */
static const char* _err_nomem            = "not enough memory";
static const char* _err_unkn             = "unknown error";
static const char* _err_io               = "I/O error";
static const char* _err_numexpected      = "number syntax expected in filename";
static const char* _err_nofmt            = "unknown format";
static const char* _err_notsupp          = "operation not supported";
#ifdef AVERROR_NOENT
static const char* _err_noent            = "no such file or directory.";
#endif

static av_result_t av_ffmpeg_error_process(int rc, const char* funcname, const char* srcfilename, int linenumber)
{
	av_result_t averr = AV_OK;
	if (rc != 0)
	{
		const char* errmsg = 0;
		switch (rc)
		{
			case AVERROR_NOMEM:       averr = AV_EMEM;       errmsg = _err_nomem;       break;
			case AVERROR_UNKNOWN:     averr = AV_EGENERAL;   errmsg = _err_unkn;        break;
			case AVERROR_IO:          averr = AV_EIO;        errmsg = _err_io;          break;
			case AVERROR_NUMEXPECTED: averr = AV_EGENERAL;   errmsg = _err_numexpected; break;
			case AVERROR_NOFMT:       averr = AV_ESUPPORTED; errmsg = _err_nofmt;       break;
			case AVERROR_NOTSUPP:     averr = AV_ESUPPORTED; errmsg = _err_notsupp;     break;
#ifdef AVERROR_NOENT
			case AVERROR_NOENT:       averr = AV_EFOUND;     errmsg = _err_noent;       break;
#endif
			default:
				printf("not handled ffmpeg error #%d, `%s' %s:%d\n", rc, funcname, srcfilename, linenumber);
				// av_assert(0, "unhandled error");
			break;
		}
		if (errmsg)
		{
			if (_log) _log->error(_log, "%s returned error (%d) `%s' %s:%d",
									funcname, rc, errmsg, srcfilename, linenumber);
		}
		else
		{
			if (_log) _log->error(_log, "%s returned unknown error code (%d, `%s') %s:%d",
									funcname, rc, strerror(AVUNERROR(rc)), srcfilename, linenumber);
		}
	}
	return averr;
}

#define av_ffmpeg_error_check(funcname, rc) av_ffmpeg_error_process(rc, funcname, __FILE__, __LINE__)

static int64_t global_video_pkt_pts = AV_NOPTS_VALUE;

typedef struct av_media_ffmpeg_ctx
{
	AVFormatContext *pFormatCtx;
	int audio_index;
	int video_index;
	int subtitle_index;
	av_decoder_audio_p audio_decoder;
	av_decoder_video_p video_decoder;

	int64_t video_timer;
	int64_t video_last_pts;
	int64_t video_last_delay;

	int64_t audio_last_pts;
	int64_t audio_diff_threshold;
	int audio_diff_avg_coef;
	int audio_diff_avg_count;
	int64_t audio_diff_cum;

} av_media_ffmpeg_ctx_t, *av_media_ffmpeg_ctx_p;

typedef struct av_media_ffmpeg_audio_decoder_ctx
{
	/* FFMPEG audio codec */
	AVCodecContext* codecCtx;

	/* AVFrame wrapper */
	av_frame_audio_p audioframe;

	/* Audio clock */
	int64_t clock;
} av_media_ffmpeg_audio_decoder_ctx_t, *av_media_ffmpeg_audio_decoder_ctx_p;

typedef struct av_media_ffmpeg_video_decoder_ctx
{
	/* FFMPEG video codec */
	AVCodecContext* codecCtx;

	/* Unique instance storing decoded frames*/
	AVFrame* frame;

	/* AVFrame wrapper */
	av_frame_video_p videoframe;

	/* Video clock */
	int64_t clock;
} av_media_ffmpeg_video_decoder_ctx_t, *av_media_ffmpeg_video_decoder_ctx_p;


static av_audio_format_t FF_AUDIO_FORMAT(int format)
{
	switch(format)
	{
		default:
		case SAMPLE_FMT_NONE :
		/* DEPRECATED - removed in new version of ffmpeg
		case SAMPLE_FMT_S24 : */
		case SAMPLE_FMT_S32 :
		case SAMPLE_FMT_FLT :
			/* FIXME */
		case SAMPLE_FMT_S16 :
			format = AV_AUDIO_SIGNED_16; break;
		case SAMPLE_FMT_U8 :
			format = AV_AUDIO_UNSIGNED_8; break;
	}
	return format;
}


/* Audio Decoder */

static av_result_t av_media_ffmpeg_decode_audio(av_decoder_audio_p audiodecoder, av_packet_p packet, av_frame_audio_p* ppaudioframe)
{
	av_media_ffmpeg_audio_decoder_ctx_p ctx = (av_media_ffmpeg_audio_decoder_ctx_p)audiodecoder->ctx;
	AVCodecContext* codec = (AVCodecContext*)ctx->codecCtx;

	int len;
	int packet_size = packet->size;
	unsigned char* packet_data = packet->data;
	int raw_size = 0;
	unsigned char* raw_data = ctx->audioframe->data;

	*ppaudioframe = 0;
	if (AV_PACKET_TYPE_AUDIO != packet->type)
		return AV_EARG;

	/* if no pts, then compute it */
	ctx->audioframe->pts = ctx->audioframe->dts = ctx->clock;

	ctx->audioframe->size = 0;
	/*! NOTE: the audio packet can contain several frames */
	while (packet_size > 0)
	{
		raw_size = AV_AUDIO_FRAME_SIZE - ctx->audioframe->size;
		len = avcodec_decode_audio2(codec, (int16_t *)raw_data, &raw_size, packet_data, packet_size);
		if(len < 0)
		{
			/* error decoding audio packet */
			return av_ffmpeg_error_check("av_media_ffmpeg_decode_audio", len);
		}

		packet_data += len;
		packet_size -= len;
		if (raw_size <= 0) continue;
		raw_data += raw_size;
		ctx->audioframe->size += raw_size;
		/* update clock */
		ctx->clock += (1000 * raw_size) / (2 * codec->channels * codec->sample_rate);
	}

	#if WITH_DEBUG_SYNCH_AUDIO_DECODER
	{
	printf("decod:[pts=%6lld] duration=%0.3f\n",
		ctx->audioframe->pts,
		(double)(ctx->clock - ctx->audioframe->pts)/1000.);
	}
	#endif
    /* if update, update the audio clock w/pts */
	/*! NOTE: this doesn't work: "ctx->codecCtx->time_base" is always 0 (zero) !!! */
	/*! NOTE: ctx->codecCtx->time_base.num is actualy 0
	if (packet->pts != (signed)AV_NOPTS_VALUE)
	{
		ctx->clock = 1000 * packet->pts * ctx->codecCtx->time_base.num / ctx->codecCtx->time_base.den;
	}*/

	*ppaudioframe = ctx->audioframe;
	return AV_OK;
}

static av_result_t av_media_ffmpeg_audio_decoder_create(AVCodecContext *codecCtx, av_decoder_audio_p *ppaudiodecoder)
{
	AVCodec *codec;
	av_decoder_audio_p audio_decoder;
	av_media_ffmpeg_audio_decoder_ctx_p ctx = (av_media_ffmpeg_audio_decoder_ctx_p)malloc(sizeof(av_media_ffmpeg_audio_decoder_ctx_t));
	if (!ctx)
		return AV_EMEM;

	ctx->codecCtx = codecCtx;

	ctx->audioframe = (av_frame_audio_p)malloc(sizeof(av_frame_audio_t));
	if (!ctx->audioframe)
	{
		free(ctx);
		return AV_EMEM;
	}
	ctx->clock = 0ll;

	audio_decoder = (av_decoder_audio_p)malloc(sizeof(av_decoder_audio_t));
	if (!audio_decoder)
	{
		free(ctx->audioframe);
		free(ctx);
	}

	codec = avcodec_find_decoder(codecCtx->codec_id);
	if(!codec || (avcodec_open(codecCtx, codec) < 0))
	{
		free(audio_decoder);
		free(ctx->audioframe);
		free(ctx);
		return AV_ESUPPORTED;
	}

	audio_decoder->ctx    = ctx;
	audio_decoder->decode = av_media_ffmpeg_decode_audio;
	*ppaudiodecoder = audio_decoder;
	return AV_OK;
}

static void av_media_ffmpeg_audio_decoder_destroy(av_decoder_audio_p audiodecoder)
{
	av_media_ffmpeg_audio_decoder_ctx_p ctx = (av_media_ffmpeg_audio_decoder_ctx_p)audiodecoder->ctx;
	avcodec_close(ctx->codecCtx);
	free(ctx->audioframe);
	free(ctx);
	free(audiodecoder);
}

/* Video Decoder */

/*! NOTE:
* The returned videoframe instance is one for all decoded video frames,
* so the next decoding call will overwrite its content.
*/
static av_result_t av_media_ffmpeg_video_decode(av_decoder_video_p video_decoder,
												av_packet_p packet,
												av_frame_video_p *ppvideoframe)
{
	av_media_ffmpeg_video_decoder_ctx_p ctx = (av_media_ffmpeg_video_decoder_ctx_p)video_decoder->ctx;
	int len;
	int finished;

	*ppvideoframe = 0;

	if (AV_PACKET_TYPE_VIDEO != packet->type)
		return AV_EARG;

	global_video_pkt_pts = packet->pts;

	/* decode video frame */
	len = avcodec_decode_video(ctx->codecCtx, ctx->frame, &finished, packet->data, packet->size);
	if (len < 0)
	{
		return av_ffmpeg_error_check("av_media_ffmpeg_video_decode", len);
	}

	if (finished)
	{
		int64_t pts;
		int64_t frame_delay;

		if (packet->dts == (int64_t)AV_NOPTS_VALUE && ctx->frame->opaque &&
			*((int64_t*)ctx->frame->opaque) != (int64_t)AV_NOPTS_VALUE)
		{
			pts = *(int64_t*)ctx->frame->opaque;
		}
		else if (packet->dts != (int64_t)AV_NOPTS_VALUE)
		{
			pts = packet->dts;
		}
		else
		{
			pts = 0;
		}
		/* update the video clock */
		frame_delay = 1000 * ctx->codecCtx->time_base.num / ctx->codecCtx->time_base.den;
		pts *= frame_delay;

		/* synchronize video */
		if (pts != 0)
		{
			/* if we have pts, set video clock to it */
			ctx->clock = pts;
		}
		else
		{
			/* if we aren't given a pts, set it to the clock */
			pts = ctx->clock;
		}
		ctx->videoframe->dts = ctx->clock;
		ctx->videoframe->pts = pts;

		/* if we are repeating a frame, adjust clock accordingly */
		frame_delay += ctx->frame->repeat_pict * (frame_delay / 2);
		ctx->clock += frame_delay;

		*ppvideoframe = ctx->videoframe;
	}

	return AV_OK;
}

static int av_ffmpeg_get_buffer(struct AVCodecContext *codecCtx, AVFrame *frame)
{
	int ret = avcodec_default_get_buffer(codecCtx, frame);
	int64_t *pts = av_malloc(sizeof(int64_t));
	*pts = global_video_pkt_pts;
	frame->opaque = pts;
	return ret;
}

static void av_ffmpeg_release_buffer(struct AVCodecContext *codecCtx, AVFrame *frame)
{
	if (frame) av_freep(&frame->opaque);
	avcodec_default_release_buffer(codecCtx, frame);
}

static av_result_t av_media_ffmpeg_video_decoder_create(AVCodecContext *codecCtx, av_decoder_video_p *ppdecoder)
{
	AVCodec *codec;
	av_decoder_video_p video_decoder;
	av_media_ffmpeg_video_decoder_ctx_p ctx = malloc(sizeof(av_media_ffmpeg_video_decoder_ctx_t));
	if (!ctx)
		return AV_EMEM;

	ctx->codecCtx = codecCtx;

	ctx->videoframe = (av_frame_video_p)malloc(sizeof(av_frame_video_t));
	if (!ctx->videoframe)
	{
		free(ctx);
		return AV_EMEM;
	}

	ctx->frame = avcodec_alloc_frame();
	if (!ctx->frame)
	{
		free(ctx);
		free(ctx->videoframe);
		return AV_EMEM;
	}
	ctx->videoframe->height   = codecCtx->height;
	ctx->videoframe->linesize = ctx->frame->linesize;
	ctx->videoframe->data     = ctx->frame->data;
	ctx->videoframe->dts      = 0ll;
	ctx->videoframe->pts      = 0ll;
	ctx->clock                = 0ll;

 	video_decoder = (av_decoder_video_p)malloc(sizeof(av_decoder_video_t));
	if (!video_decoder)
	{
		av_free(ctx->frame);
		free(ctx->videoframe);
		free(ctx);
		return AV_EMEM;
	}

	codec = avcodec_find_decoder(codecCtx->codec_id);
	if(!codec || (avcodec_open(codecCtx, codec) < 0))
	{
		av_free(ctx->frame);
		free(ctx->videoframe);
		free(ctx);
		return AV_ESUPPORTED;
	}

    codecCtx->get_buffer = av_ffmpeg_get_buffer;
    codecCtx->release_buffer = av_ffmpeg_release_buffer;

	video_decoder->ctx = ctx;
	video_decoder->decode = av_media_ffmpeg_video_decode;
	*ppdecoder = video_decoder;
	return AV_OK;
}

static void av_media_ffmpeg_video_decoder_destroy(av_decoder_video_p video_decoder)
{
	av_media_ffmpeg_video_decoder_ctx_p ctx = (av_media_ffmpeg_video_decoder_ctx_p)video_decoder->ctx;
	av_free(ctx->frame);
	free(ctx->videoframe);
	avcodec_close(ctx->codecCtx);
	free(ctx);
	free(video_decoder);
}

/* Media */
static av_result_t av_media_ffmpeg_open(av_media_p self, const char* url)
{
	unsigned int i;
	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)O_context(self);
	self->close(self);

	/* Register all formats and codecs */
	if (!g_ffmpeg_registered)
	{
		av_register_all();
		g_ffmpeg_registered = AV_TRUE;
	}

	/* open media file */
	if (0 != av_open_input_file(&ctx->pFormatCtx, url, NULL, 0, NULL))
		return AV_EFOUND; /* couldn't open file */

	/* retrieve stream information */
	if (0 > av_find_stream_info(ctx->pFormatCtx))
	{
		self->close(self);
		return AV_ESUPPORTED; /* couldn't find stream information */
	}

	/* dump information about file onto standard error */
	dump_format(ctx->pFormatCtx, 0, url, 0);

	ctx->video_index = ctx->audio_index = ctx->subtitle_index = AV_STREAM_NONE;
	for (i=0; i<ctx->pFormatCtx->nb_streams; i++)
	{
		AVCodecContext *codecCtx = ctx->pFormatCtx->streams[i]->codec;
		if (CODEC_TYPE_VIDEO == codecCtx->codec_type && ctx->video_index == AV_STREAM_NONE)
		{
			/* create video decoder */
			if (AV_OK != av_media_ffmpeg_video_decoder_create(codecCtx, &ctx->video_decoder))
				continue;
			ctx->video_index = i;
			ctx->video_timer = 0ll;
			ctx->video_last_delay = 40ll;
		}

		if (CODEC_TYPE_AUDIO == codecCtx->codec_type && ctx->audio_index == AV_STREAM_NONE)
		{
			/* create audio decoder */
			if (AV_OK != av_media_ffmpeg_audio_decoder_create(codecCtx, &ctx->audio_decoder))
				continue;
			ctx->audio_index = i;
			ctx->audio_diff_threshold = (2000 * AV_HW_AUDIO_BUFFER_SIZE) / codecCtx->sample_rate;
		}

		if (CODEC_TYPE_SUBTITLE == codecCtx->codec_type && ctx->subtitle_index == AV_STREAM_NONE)
		{
			ctx->subtitle_index = i;
		}
	}

	if (AV_STREAM_NONE == ctx->video_index && AV_STREAM_NONE == ctx->audio_index)
		return AV_ESUPPORTED; /* couldn't find decodable stream */

	return AV_OK;
}

static av_result_t av_media_ffmpeg_get_audio_info(av_media_p self, av_audio_info_p audio_info)
{
	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)O_context(self);
	av_assert(audio_info, "audio_info can't be NULL");
	if (ctx->audio_decoder)
	{
		av_media_ffmpeg_audio_decoder_ctx_p audioCtx = (av_media_ffmpeg_audio_decoder_ctx_p)ctx->audio_decoder->ctx;
		AVCodecContext *codecCtx = audioCtx->codecCtx;
		audio_info->codec_id = codecCtx->codec_id;
		audio_info->nchannels = codecCtx->channels;
		audio_info->freq = codecCtx->sample_rate;
		audio_info->format = FF_AUDIO_FORMAT(codecCtx->sample_fmt);

		return AV_OK;
	}
	return AV_ESUPPORTED;
}

static av_result_t av_media_ffmpeg_get_video_info(av_media_p self, av_video_info_p video_info)
{
	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)O_context(self);
	av_assert(video_info, "video_info can't be NULL");
	if (ctx->video_decoder)
	{
		av_media_ffmpeg_video_decoder_ctx_p videoCtx = (av_media_ffmpeg_video_decoder_ctx_p)ctx->video_decoder->ctx;
		AVCodecContext *codecCtx = videoCtx->codecCtx;
		video_info->codec_id = codecCtx->codec_id;
		switch (codecCtx->pix_fmt)
		{
			case PIX_FMT_RGB32:
				video_info->format = AV_VIDEO_FORMAT_RGB32;
			break;
			case PIX_FMT_RGB24:
				video_info->format = AV_VIDEO_FORMAT_RGB24;
			break;
			case PIX_FMT_YUV420P:
				video_info->format = AV_VIDEO_FORMAT_YUV420P;
			break;
			default:
				video_info->format = AV_VIDEO_FORMAT_UNKNOWN;
			break;
		}
		video_info->width = codecCtx->width;
		video_info->height = codecCtx->height;

		/* get video stream */
		AVStream *st = ctx->pFormatCtx->streams[ctx->video_index];
		if (st->codec->codec_type == CODEC_TYPE_VIDEO)
		{
			if (st->r_frame_rate.den && st->r_frame_rate.num)
				video_info->fps = av_q2d(st->r_frame_rate);
			else
				video_info->fps = 1. / av_q2d(st->codec->time_base);
			return AV_OK;
		}
	}
	return AV_ESUPPORTED;
}

static void av_media_ffmpeg_packet_destructor(void* packet)
{
	av_packet_p self = (av_packet_p)packet;
	if (self->ctx)
	{
		AVPacket* pkt = (AVPacket*)self->ctx;
		av_free_packet(pkt);
		av_free(pkt);
	}
	free(packet);
}

static av_result_t av_media_ffmpeg_read_packet(av_media_p self, av_packet_p* pppacket)
{
	AVPacket* pkt;
	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)O_context(self);
	av_packet_p packet;

	*pppacket = 0;

	pkt = (AVPacket*)av_mallocz(sizeof(AVPacket));
	if (!pkt)
		return AV_EMEM;

	if (av_read_frame(ctx->pFormatCtx, pkt) < 0)
	{
		av_free_packet(pkt);
		av_free(pkt);
		/* FIXME: define proper error result */
		return AV_EGENERAL;
	}

	/* create packet */
	packet = (av_packet_p)malloc(sizeof(av_packet_t));
	if (!packet)
	{
		av_free_packet(pkt);
		av_free(pkt);
		return AV_EMEM;
	}

	av_dup_packet(pkt);
	packet->ctx = pkt;

	if (ctx->audio_index == pkt->stream_index)
		packet->type = AV_PACKET_TYPE_AUDIO;
	else
	if (ctx->video_index == pkt->stream_index)
		packet->type = AV_PACKET_TYPE_VIDEO;
	else
	if (ctx->subtitle_index == pkt->stream_index)
		packet->type = AV_PACKET_TYPE_SUBTITLE;
	else
		packet->type = AV_PACKET_TYPE_UNKNOWN;

	packet->data    = pkt->data;
	packet->size    = pkt->size;
	packet->dts     = pkt->dts;
	packet->pts     = pkt->pts;
	packet->counter = 0;
	packet->destroy = av_media_ffmpeg_packet_destructor;
	*pppacket = packet;
	return AV_OK;
}

static av_result_t av_media_ffmpeg_get_audio_decoder(av_media_p self, av_decoder_audio_p* ppaudiodecoder)
{
	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)O_context(self);
	if (ctx->audio_decoder)
	{
		*ppaudiodecoder = ctx->audio_decoder;
		return AV_OK;
	}
	*ppaudiodecoder = AV_NULL;
	return AV_ESUPPORTED;
}

static av_result_t av_media_ffmpeg_get_video_decoder(av_media_p self, av_decoder_video_p* ppvideodecoder)
{
	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)O_context(self);
	if (ctx->video_decoder)
	{
		*ppvideodecoder = ctx->video_decoder;
		return AV_OK;
	}
	*ppvideodecoder = AV_NULL;
	return AV_ESUPPORTED;
}

static void av_media_ffmpeg_close(av_media_p self)
{
	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)O_context(self);

	if (ctx->audio_decoder)
	{
		av_media_ffmpeg_audio_decoder_destroy(ctx->audio_decoder);
		ctx->audio_decoder = AV_NULL;
	}

	if (ctx->video_decoder)
	{
		av_media_ffmpeg_video_decoder_destroy(ctx->video_decoder);
		ctx->video_decoder = AV_NULL;
	}

	if (ctx->pFormatCtx)
	{
		av_close_input_file(ctx->pFormatCtx);
		ctx->pFormatCtx = 0;
	}
}

static int64_t av_media_ffmpeg_clock(struct av_media* self)
{
// 	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)O_context(self);
	AV_UNUSED(self);
	return (av_gettime() / 1000);
}

static av_result_t av_media_ffmpeg_synchronize_video(struct av_media* self, av_frame_video_p pvideoframe)
{
	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)O_context(self);

	int64_t diff;
	int64_t delay;
	int64_t actual_delay;
	int64_t sync_threshold;
	int64_t ref_clock;

	ref_clock = self->clock(self);

	delay = pvideoframe->pts - ctx->video_last_pts; /* the pts from last time */
	if (delay <= 0 || delay >= 2000)
	{
		/* if incorrect delay, use previous one */
		delay = ctx->video_last_delay;
	}

	/* save for next time */
	ctx->video_last_delay = delay;
	ctx->video_last_pts = pvideoframe->pts;

	/* Update delay to sync to master clock */
	diff = pvideoframe->pts - ref_clock;

	/* Skip or repeat the frame. Take delay into account */
	sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
	if (fabs(diff) < AV_NOSYNC_THRESHOLD)
	{
		if (diff <= -sync_threshold)
		{
			delay = 0;
		}
		else if (diff >= sync_threshold)
		{
			delay = 2 * delay;
		}
	}
	ctx->video_timer += delay;

	/* computer the REAL delay */
	actual_delay = ctx->video_timer - ref_clock;
	if (actual_delay < AV_SKIP_PICTURE_THRESHOLD)
	{
		/* Really it should skip the picture instead */
		actual_delay = 10;
	}

	pvideoframe->delay_ms = actual_delay;

	#if WITH_DEBUG_SYNCH_VIDEO
	printf("video:[%6lld - pts=%6lld] delay:%+0.3f diff=%+0.3f actual_delay=%+0.3f\n",
		ref_clock,
		pvideoframe->pts,
		(double)delay/1000.,
		(double)-diff / 1000.,
		(double)actual_delay/1000.);
	#endif

	#if WITH_DEBUG_SYNCH
	printf("media: VPTS:%+0.3f APTS:%+0.3f\n",
		(double)(ctx->video_last_pts)/1000.,
		(double)(ctx->audio_last_pts)/1000.);
	#endif
	return AV_OK;
}

static av_result_t av_media_ffmpeg_synchronize_audio(struct av_media* self, av_frame_audio_p paudioframe)
{
	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)O_context(self);
	av_decoder_audio_p audiodecoder = ctx->audio_decoder;
	av_media_ffmpeg_audio_decoder_ctx_p codec_ctx = (av_media_ffmpeg_audio_decoder_ctx_p)audiodecoder->ctx;
	AVCodecContext* codec = (AVCodecContext*)codec_ctx->codecCtx;
	int64_t diff;
	int64_t avg_diff;
	int64_t ref_clock;
	int wanted_size;
	int min_size;
	int max_size;
	int n = 2 * codec->channels;
	int samples_size = paudioframe->size;

	ref_clock = self->clock(self);

	diff = paudioframe->pts - ref_clock;

	if (diff < AV_NOSYNC_THRESHOLD)
	{
		ctx->audio_diff_cum = diff + ctx->audio_diff_cum * ctx->audio_diff_avg_coef / 1000;

		if (ctx->audio_diff_avg_count < AUDIO_DIFF_AVG_NB)
		{
			/* not enough measures to have a correct estimate */
			ctx->audio_diff_avg_count++;
		}
		else
		{
			/* estimate the A-V difference */
			avg_diff = ctx->audio_diff_cum * (1000 - ctx->audio_diff_avg_coef) / 1000;
			if (abs(avg_diff) >= ctx->audio_diff_threshold)
			{
				min_size = (samples_size * (100 - SAMPLE_CORRECTION_PERCENT_MAX))/100;
				max_size = (samples_size * (100 + SAMPLE_CORRECTION_PERCENT_MAX))/100;

				wanted_size = samples_size + ((diff * codec->sample_rate * n)/1000);

				if (wanted_size < min_size) wanted_size = min_size;
				else if (wanted_size > max_size) wanted_size = max_size;

				/* add or remove samples to correction the synchro */
				if (wanted_size < samples_size)
				{
					/* remove samples */
					samples_size = wanted_size;
				}
				else
				if (wanted_size > samples_size)
				{
					int nb;
					unsigned char* q;
					unsigned char* samples_end;
					/* add samples */
					nb = (samples_size - wanted_size);
					samples_end = (unsigned char*)paudioframe->data + samples_size - n;
					q = samples_end + n;
					while (nb > 0)
					{
						memcpy(q, samples_end, n);
						q += n;
						nb -= n;
					}
					samples_size = wanted_size;
				}
			}
			#if WITH_DEBUG_SYNCH_AUDIO
			printf("audio:[%6lld - pts=%6lld] diff=%+0.3f avg_diff=%+0.3f size_diff=%d\n",
				ref_clock,
				paudioframe->pts,
				(double)diff/1000.,
				(double)avg_diff/1000.,
				samples_size - paudioframe->size);
			#endif
			/* update new size */
			paudioframe->size = samples_size;
		}
	}
	else
	{
		/* too big difference : may be initial PTS errors, so reset A-V filter */
		ctx->audio_diff_avg_count = 0;
		ctx->audio_diff_cum = 0ll;
	}
	ctx->audio_last_pts = paudioframe->pts;
	return AV_OK;
}

static av_result_t av_media_ffmpeg_seek(struct av_media* self, double pos_seconds)
{
	/* FIXME: implement as absolute pos */
	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)O_context(self);
	int64_t pos = ctx->pFormatCtx->duration * pos_seconds / 100.;

	if (!av_seek_frame(ctx->pFormatCtx, -1, pos, 0))
	{
		return AV_EARG;
	}

	return AV_OK;
}

static void av_media_ffmpeg_destructor(void* pobject)
{
	av_media_p self = (av_media_p)pobject;
	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)O_context(pobject);
	self->close(self);
	free(ctx);
}

static av_result_t av_media_ffmpeg_constructor(av_object_p pobject)
{
	av_media_p self = (av_media_p)pobject;
	av_media_ffmpeg_ctx_p ctx = (av_media_ffmpeg_ctx_p)malloc(sizeof(av_media_ffmpeg_ctx_t));
	if (!ctx)
		return AV_EMEM;

	memset(ctx, 0, sizeof(av_media_ffmpeg_ctx_t));

	ctx->audio_diff_avg_coef = 1000 * exp(log(0.01) / AUDIO_DIFF_AVG_NB);
	ctx->audio_diff_avg_count = 0;
	ctx->audio_diff_threshold = 0ll; /* updated when codec found */
	ctx->audio_diff_cum = 0ll;
	ctx->audio_last_pts = 0ll;

	ctx->video_timer = 0ll;  /* updated when codec found */
	ctx->video_last_pts = 0ll;
	ctx->video_last_delay = 0ll;  /* updated when codec found */

	O_set_attr(self, CONTEXT, ctx);
	self->open              = av_media_ffmpeg_open;
	self->get_audio_info    = av_media_ffmpeg_get_audio_info;
	self->get_video_info    = av_media_ffmpeg_get_video_info;
	self->read_packet       = av_media_ffmpeg_read_packet;
	self->get_audio_decoder = av_media_ffmpeg_get_audio_decoder;
	self->get_video_decoder = av_media_ffmpeg_get_video_decoder;
	self->clock             = av_media_ffmpeg_clock;
	self->close             = av_media_ffmpeg_close;
	self->synchronize_video = av_media_ffmpeg_synchronize_video;
	self->synchronize_audio = av_media_ffmpeg_synchronize_audio;
	self->seek              = av_media_ffmpeg_seek;

	return AV_OK;
}

av_result_t av_media_ffmpeg_register_torba(void)
{
	av_result_t rc;

	if (AV_OK != (rc = av_torb_register_class("media_ffmpeg",
											  "media",
											  sizeof(av_media_t),
											  av_media_ffmpeg_constructor,
											  av_media_ffmpeg_destructor)))
		return rc;

	return AV_OK;
}

#endif /* WITH_FFMPEG */
