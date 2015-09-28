/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_player_ffmpeg.c                                 */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_FFMPEG

#include <math.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>

#ifdef USE_INCLUDE_FFMPEG
	#include <ffmpeg/avformat.h>
	#include <ffmpeg/swscale.h>
	#include <ffmpeg/rtsp.h>
	#define URLARG(x) x
	#define URLICPB(icpb,eofreached) icpb->eofreached
#endif

#ifdef USE_INCLUDE_LIBAVFORMAT
	#include <libavformat/avformat.h>
	#include <libavformat/rtsp.h>
	#include <libswscale/swscale.h>
	#define URLARG(x) x
	#define URLICPB(icpb,eofreached) icpb->eofreached
#endif

#include <av_log.h>
#include <av_timer.h>
#include <av_system.h>
#include <av_prefs.h>
#include <av_audio.h>
#include <av_video.h>
#include <av_player.h>
#include <av_graphics.h>
#include <av_media.h>
#include <av_thread.h>

#define WITH_OVERLAY          0
#define WITH_INTERRUPT        0
#define WITH_DEBUG_SYNC       0

#if WITH_OVERLAY
typedef void (*frame_to_overlay_func_p)(av_video_overlay_p overlay, AVFrame* frame, int height);
#else
static const char _scaler_driver[] = "scaler_swscale";
av_result_t av_scaler_swscale_register_torba(void);
#endif

#define MAX_VIDEO_QUEUE_SIZE        (16 * 1024)
#define MAX_AUDIO_QUEUE_SIZE        (16 * 1024)
#define MAX_SUBTITLE_QUEUE_SIZE     (16 * 1024)

#define VIDEO_PICTURE_QUEUE_SIZE    (1)
#define SUBTITLE_PICTURE_QUEUE_SIZE (4)

#define CONTEXT "player_ffplay_ctx"
#define O_context(o) O_attr(o, CONTEXT)

enum
{
	EVENT_ALLOC_ID   = 1,
	EVENT_REFRESH_ID = 2,
};

typedef struct av_packet_queue
{
	AVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	int abort_request;
	av_mutex_p mtx;
	av_condition_p cond;
} av_packet_queue_t, *av_packet_queue_p;

typedef struct av_player_ffplay_videopicture_ctx
{
	int skip;
	int allocated;
	double pts;
	/* actual frame dimensions */
	int w_in, w_out;
	int h_in, h_out;
} av_player_ffplay_videopicture_ctx;

typedef struct av_player_ffplay_subpicture_ctx
{
	double pts; /* presentation time stamp for this picture */
	AVSubtitle sub;
} av_player_ffplay_subpicture_ctx;

enum
{
	AV_SYNC_AUDIO_MASTER, /* default choice */
	AV_SYNC_VIDEO_MASTER,
	AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

/* forward def */
struct av_player_ffplay_ctx;

/* options specified by the user */
typedef struct av_player_ffplay_config
{
	int av_sync_type;
	int debug;
	int debug_mv;
	int thread_count;
	int workaround_bugs;
	int fast;
	int genpts;
	int lowres;
	int idct;
	enum AVDiscard skip_frame;
	enum AVDiscard skip_idct;
	enum AVDiscard skip_loop_filter;
	int error_resilience;
	int error_concealment;
	int decoder_reorder_pts;
	#if WITH_OVERLAY
	av_video_overlay_format_t video_overlay_format;
	#else
	const char* scaler_driver;
	#endif
	int skip_enabled;
	int alpha_enabled;
} av_player_ffplay_config_t, *av_player_ffplay_config_p;

typedef struct av_player_ffplay_status
{
	av_mutex_p mtx_picture;
	av_condition_p cond_picture;
	av_mutex_p mtx_subpicture;
	av_condition_p cond_subpicture;

	AVInputFormat *iformat;
	int abort_request;
	int paused;
	int playing;
	int last_paused;
	int seek_request;
	int seek_flags;
	int64_t seek_pos;
	AVFormatContext *ic;
	int dtg_active_format;

	int audio_stream;
	int av_sync_type;
	double external_clock; /* external clock base */
	int64_t external_clock_time;

	double audio_clock;
	double audio_diff_cum; /* used for AV difference average computation */
	double audio_diff_avg_coef;
	double audio_diff_threshold;
	int audio_diff_avg_count;
	AVStream *audio_st;
	av_packet_queue_t audioq;
	/* samples output by the codec. we reserve more space for avsync compensation */
	DECLARE_ALIGNED(16,unsigned char,audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2]);
	int audio_buf_size; /* in bytes */
	int audio_buf_index; /* in bytes */
	AVPacket audio_pkt;
	int audio_pkt_size;
	unsigned char *audio_pkt_data;

	int subtitle_stream;
	int subtitle_stream_changed;
	AVStream *subtitle_st;
	av_packet_queue_t subtitleq;
	av_player_ffplay_subpicture_ctx subpq[SUBTITLE_PICTURE_QUEUE_SIZE];
	int subpq_size, subpq_rindex, subpq_windex;

	int video_stream;
	double frame_timer;
	double frame_last_pts;
	double frame_last_delay;
	double video_clock; /* pts of last decoded frame / predicted pts of next decoded frame */
	AVStream *video_st;
	av_packet_queue_t videoq;
	double video_current_pts; /* current displayed pts (different from video_clock if frame fifos are used) */
	int64_t video_current_pts_time; /* time (av_gettime) at which we updated video_current_pts - used to have running video pts */
	av_player_ffplay_videopicture_ctx pictq[VIDEO_PICTURE_QUEUE_SIZE];
	int pictq_size, pictq_rindex, pictq_windex;

	int64_t audio_callback_time;
	AVPacket flush_pkt;

	int slow_computer;
	double aspect_ratio;
	struct av_player_ffplay_ctx* ctx;
	struct av_player_ffplay_config config;

} av_player_ffplay_status_t, *av_player_ffplay_status_p;

typedef struct av_player_ffplay_ctx
{
	av_input_p input;
	av_video_p video;
	av_audio_p audio;
	av_timer_p timer;
	av_window_p owner;

	av_audio_info_t audio_info;
	av_video_info_t video_info;
	struct av_audio_handle* audio_handle;

	#if WITH_OVERLAY
	av_video_overlay_p video_overlay;
	av_video_overlay_format_t video_overlay_format;
	frame_to_overlay_func_p frame_to_overlay;
	#else
	av_scaler_p scaler;
	av_scale_info_t scale_info;
	av_video_surface_p video_surface;
	#endif

	av_thread_p thd_packet_reader;
	av_bool_t thd_packet_reader_created;
	av_thread_p thd_video_decoder;
	av_bool_t thd_video_decoder_created;
	av_thread_p thd_subtitle_decoder;
	av_bool_t thd_subtitle_decoder_created;

	av_bool_t has_video;
	av_bool_t has_audio;
	av_bool_t has_subtitle;

	av_bool_t draw_borders;
	av_player_ffplay_status_p status;
} av_player_ffplay_ctx_t, *av_player_ffplay_ctx_p;

static av_player_ffplay_config_t config =
{
	.av_sync_type = AV_SYNC_AUDIO_MASTER,
	.debug = 0,
	.debug_mv = 0,
	.thread_count = 1,
	.workaround_bugs = 1,
	.fast = 0,
	.genpts = 1,
	.lowres = 0,
	.idct = FF_IDCT_AUTO,
	.skip_frame= AVDISCARD_DEFAULT,
	.skip_idct= AVDISCARD_DEFAULT,
	.skip_loop_filter= AVDISCARD_DEFAULT,
	.error_resilience = FF_ER_CAREFUL,
	.error_concealment = 3,
	.decoder_reorder_pts = 1,
#if WITH_OVERLAY
	.video_overlay_format = AV_YV12_OVERLAY,
#else
	.scaler_driver = _scaler_driver,
#endif
	.skip_enabled = 0,
	.alpha_enabled = 1,
};

static av_system_p sys = AV_NULL;
static AVPacket global_flush_pkt;
/** FIXME static av_player_ffplay_status_p global_video_state = AV_NULL; */

static av_audio_format_t FF_AUDIO_FORMAT(int format)
{
	switch(format)
	{
		default:
		case SAMPLE_FMT_NONE :
		case SAMPLE_FMT_S24 :
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


/* packet queue handling */
static av_result_t packet_queue_init(av_packet_queue_t *q)
{
	av_result_t rc = AV_OK;
	memset(q, 0, sizeof(av_packet_queue_t));
	if (AV_OK != (rc = av_mutex_create(&q->mtx)))
	{
		return rc;
	}
	if (AV_OK != (rc = av_condition_create(&q->cond)))
	{
		return rc;
	}
	return rc;
}

static void packet_queue_flush(av_packet_queue_t *q)
{
	AVPacketList *pkt, *pkt1;

	q->mtx->lock(q->mtx);
	for(pkt = q->first_pkt; pkt != AV_NULL; pkt = pkt1)
	{
		pkt1 = pkt->next;
		av_free_packet(&pkt->pkt);
		av_freep(&pkt);
	}
	q->last_pkt = AV_NULL;
	q->first_pkt = AV_NULL;
	q->nb_packets = 0;
	q->size = 0;
	q->mtx->unlock(q->mtx);
}

static void packet_queue_end(av_packet_queue_t *q)
{
	packet_queue_flush(q);
	q->mtx->destroy(q->mtx);
	q->cond->destroy(q->cond);
}

static int packet_queue_put(av_packet_queue_t *q, AVPacket *pkt)
{
	AVPacketList *pkt1;

	/* duplicate the packet */
	if (pkt->data != global_flush_pkt.data && av_dup_packet(pkt) < 0)
		return -1;

	pkt1 = av_malloc(sizeof(AVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = AV_NULL;

	q->mtx->lock(q->mtx);

	if (!q->last_pkt)
		q->first_pkt = pkt1;
	else
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size;
	/* XXX: should duplicate packet data in DV case */
	q->cond->signal(q->cond);
	q->mtx->unlock(q->mtx);
	return 0;
}

static void packet_queue_abort(av_packet_queue_t *q)
{
	q->mtx->lock(q->mtx);
	q->abort_request = 1;
	q->cond->signal(q->cond);
	q->mtx->unlock(q->mtx);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
static int packet_queue_get(av_packet_queue_t *q, AVPacket *pkt, int block)
{
	AVPacketList *pkt1;
	int ret;

	q->mtx->lock(q->mtx);
	for(;;)
	{
		if (q->abort_request)
		{
			ret = -1;
			break;
		}
		pkt1 = q->first_pkt;
		if (pkt1)
		{
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = AV_NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size;
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		}
		else
		if (!block)
		{
			ret = 0;
			break;
		}
		else
		{
			q->cond->wait(q->cond,q->mtx);
		}
	}
	q->mtx->unlock(q->mtx);
	return ret;
}

#if WITH_OVERLAY
static void frame_to_overlay_YV12(av_video_overlay_p overlay, AVFrame* frame, int height)
{
	int planes;
	int wy,wu,wv;
	const av_pix_p* pix;
	av_pix_p swy, swu, swv;
	av_pix_p dwy, dwu, dwv;
	const short int* pitches;

	overlay->lock(overlay, &planes, &pix, &pitches);

	wy = AV_MIN(pitches[0],frame->linesize[0]);
	wu = AV_MIN(pitches[1],frame->linesize[2]);
	wv = AV_MIN(pitches[2],frame->linesize[1]);

	swy = frame->data[0];
	swu = frame->data[2];
	swv = frame->data[1];

	dwy = pix[0];
	dwu = pix[1];
	dwv = pix[2];

	for (height/=2; height-->0;)
	{
		memcpy(dwy, swy, wy); dwy += pitches[0]; swy += frame->linesize[0];
		memcpy(dwy, swy, wy); dwy += pitches[0]; swy += frame->linesize[0];
		memcpy(dwu, swu, wu); dwu += pitches[1]; swu += frame->linesize[2];
		memcpy(dwv, swv, wv); dwv += pitches[2]; swv += frame->linesize[1];
	}
	overlay->unlock(overlay);
}

static void frame_to_overlay_IYUV(av_video_overlay_p overlay, AVFrame* frame, int height)
{
	int planes;
	int wy,wu,wv;
	const av_pix_p* pix;
	av_pix_p swy, swu, swv;
	av_pix_p dwy, dwu, dwv;
	const short int* pitches;

	overlay->lock(overlay, &planes, &pix, &pitches);

	wy = AV_MIN(pitches[0],frame->linesize[0]);
	wu = AV_MIN(pitches[1],frame->linesize[1]);
	wv = AV_MIN(pitches[2],frame->linesize[2]);

	swy = frame->data[0];
	swu = frame->data[1];
	swv = frame->data[2];

	dwy = pix[0];
	dwu = pix[1];
	dwv = pix[2];

	for (height/=2; height-->0;)
	{
		memcpy(dwy, swy, wy); dwy += pitches[0]; swy += frame->linesize[0];
		memcpy(dwy, swy, wy); dwy += pitches[0]; swy += frame->linesize[0];
		memcpy(dwu, swu, wu); dwu += pitches[1]; swu += frame->linesize[1];
		memcpy(dwv, swv, wv); dwv += pitches[2]; swv += frame->linesize[2];
	}
	overlay->unlock(overlay);
}
#else
#endif

static void load_prefs(void)
{
	av_prefs_p prefs;
	if (AV_OK == av_torb_service_addref("prefs", (av_service_p*)&prefs))
	{
		#if WITH_OVERLAY
		const char* val = AV_NULL;
		prefs->get_string(prefs, "player.ffplay.overlay", "YV12", &val);
		config.video_overlay_format = (val && !strcasecmp("IYUV",val))? AV_IYUV_OVERLAY : AV_YV12_OVERLAY;
		#else
		prefs->get_string(prefs, "player.ffplay.scaler", _scaler_driver, &config.scaler_driver);
		#endif
		prefs->get_int(prefs, "player.ffplay.skipenabled", 0, &config.skip_enabled);
		prefs->get_int(prefs, "player.ffplay.alphaenabled", 1, &config.alpha_enabled);
		av_torb_service_release("prefs");
	}
}

static void log_error(const char* message)
{
	av_log_p logging;
	if (AV_OK == av_torb_service_addref("log", (av_service_p*)&logging))
	{
		logging->error(logging,message);
		av_torb_service_release("log");
	}
}

#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))

#define QEARGB(a,r,g,b) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define QERGB(r,g,b) QEARGB(0xff, r, g, b)

#define RGB_TO_Y_CCIR(r, g, b) \
((FIX(0.29900*219.0/255.0) * (r) + FIX(0.58700*219.0/255.0) * (g) + \
FIX(0.11400*219.0/255.0) * (b) + (ONE_HALF + (16 << SCALEBITS))) >> SCALEBITS)

#define RGB_TO_U_CCIR(r1, g1, b1, shift)\
(((- FIX(0.16874*224.0/255.0) * r1 - FIX(0.33126*224.0/255.0) * g1 +         \
	FIX(0.50000*224.0/255.0) * b1 + (ONE_HALF << shift) - 1) >> (SCALEBITS + shift)) + 128)

#define RGB_TO_V_CCIR(r1, g1, b1, shift)\
(((FIX(0.50000*224.0/255.0) * r1 - FIX(0.41869*224.0/255.0) * g1 -           \
FIX(0.08131*224.0/255.0) * b1 + (ONE_HALF << shift) - 1) >> (SCALEBITS + shift)) + 128)

#define ALPHA_BLEND(a, oldp, newp, s)\
((((oldp << s) * (255 - (a))) + (newp * (a))) / (255 << s))

#define RGBA_IN(r, g, b, a, s)\
{\
	unsigned int v = ((const uint32_t *)(s))[0];\
	a = (v >> 24) & 0xff;\
	r = (v >> 16) & 0xff;\
	g = (v >> 8) & 0xff;\
	b = v & 0xff;\
}

#define YUVA_IN(y, u, v, a, s, pal)\
{\
	unsigned int val = ((const uint32_t *)(pal))[*(const unsigned char*)(s)];\
	a = (val >> 24) & 0xff;\
	y = (val >> 16) & 0xff;\
	u = (val >> 8) & 0xff;\
	v = val & 0xff;\
}

#define YUVA_OUT(d, y, u, v, a)\
{\
	((uint32_t *)(d))[0] = (a << 24) | (y << 16) | (u << 8) | v;\
}

#if 0
#define BPP 1
static void blend_subrect(AVPicture *dst, const AVSubtitleRect *rect, int imgw, int imgh)
{
	int wrap, wrap3, width2, skip2;
	int y, u, v0, a, u1, v1, a1, w, h;
	unsigned char *lum, *cb, *cr;
	const unsigned char *p;
	const uint32_t *pal;
	int dstx, dsty, dstw, dsth;

	dstx = FFMIN(FFMAX(rect->x, 0), imgw);
	dstw = FFMIN(FFMAX(rect->w, 0), imgw - dstx);
	dsty = FFMIN(FFMAX(rect->y, 0), imgh);
	dsth = FFMIN(FFMAX(rect->h, 0), imgh - dsty);
	lum = dst->data[0] + dsty * dst->linesize[0];
	cb = dst->data[1] + (dsty >> 1) * dst->linesize[1];
	cr = dst->data[2] + (dsty >> 1) * dst->linesize[2];

	width2 = (dstw + 1) >> 1;
	skip2 = dstx >> 1;
	wrap = dst->linesize[0];
	wrap3 = rect->linesize;
	p = rect->bitmap;
	pal = rect->rgba_palette;  /* Now in YCrCb! */

	if (dsty & 1) {
		lum += dstx;
		cb += skip2;
		cr += skip2;

		if (dstx & 1) {
			YUVA_IN(y, u, v0, a, p, pal);
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
			cr[0] = ALPHA_BLEND(a >> 2, cr[0], v0, 0);
			cb++;
			cr++;
			lum++;
			p += BPP;
		}
		for(w = dstw - (dstx & 1); w >= 2; w -= 2) {
			YUVA_IN(y, u, v0, a, p, pal);
			u1 = u;
			v1 = v0;
			a1 = a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

			YUVA_IN(y, u, v0, a, p + BPP, pal);
			u1 += u;
			v1 += v0;
			a1 += a;
			lum[1] = ALPHA_BLEND(a, lum[1], y, 0);
			cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 1);
			cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 1);
			cb++;
			cr++;
			p += 2 * BPP;
			lum += 2;
		}
		if (w) {
			YUVA_IN(y, u, v0, a, p, pal);
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
			cr[0] = ALPHA_BLEND(a >> 2, cr[0], v0, 0);
		}
		p += wrap3 + (wrap3 - dstw * BPP);
		lum += wrap + (wrap - dstw - dstx);
		cb += dst->linesize[1] - width2 - skip2;
		cr += dst->linesize[2] - width2 - skip2;
	}
	for(h = dsth - (dsty & 1); h >= 2; h -= 2) {
		lum += dstx;
		cb += skip2;
		cr += skip2;

		if (dstx & 1) {
			YUVA_IN(y, u, v0, a, p, pal);
			u1 = u;
			v1 = v0;
			a1 = a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			p += wrap3;
			lum += wrap;
			YUVA_IN(y, u, v0, a, p, pal);
			u1 += u;
			v1 += v0;
			a1 += a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 1);
			cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 1);
			cb++;
			cr++;
			p += -wrap3 + BPP;
			lum += -wrap + 1;
		}
		for(w = dstw - (dstx & 1); w >= 2; w -= 2) {
			YUVA_IN(y, u, v0, a, p, pal);
			u1 = u;
			v1 = v0;
			a1 = a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

			YUVA_IN(y, u, v0, a, p, pal);
			u1 += u;
			v1 += v0;
			a1 += a;
			lum[1] = ALPHA_BLEND(a, lum[1], y, 0);
			p += wrap3;
			lum += wrap;

			YUVA_IN(y, u, v0, a, p, pal);
			u1 += u;
			v1 += v0;
			a1 += a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

			YUVA_IN(y, u, v0, a, p, pal);
			u1 += u;
			v1 += v0;
			a1 += a;
			lum[1] = ALPHA_BLEND(a, lum[1], y, 0);

			cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 2);
			cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 2);

			cb++;
			cr++;
			p += -wrap3 + 2 * BPP;
			lum += -wrap + 2;
		}
		if (w) {
			YUVA_IN(y, u, v0, a, p, pal);
			u1 = u;
			v1 = v0;
			a1 = a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			p += wrap3;
			lum += wrap;
			YUVA_IN(y, u, v0, a, p, pal);
			u1 += u;
			v1 += v0;
			a1 += a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 1);
			cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 1);
			cb++;
			cr++;
			p += -wrap3 + BPP;
			lum += -wrap + 1;
		}
		p += wrap3 + (wrap3 - dstw * BPP);
		lum += wrap + (wrap - dstw - dstx);
		cb += dst->linesize[1] - width2 - skip2;
		cr += dst->linesize[2] - width2 - skip2;
	}
	/* handle odd height */
	if (h) {
		lum += dstx;
		cb += skip2;
		cr += skip2;

		if (dstx & 1) {
			YUVA_IN(y, u, v0, a, p, pal);
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
			cr[0] = ALPHA_BLEND(a >> 2, cr[0], v0, 0);
			cb++;
			cr++;
			lum++;
			p += BPP;
		}
		for(w = dstw - (dstx & 1); w >= 2; w -= 2) {
			YUVA_IN(y, u, v0, a, p, pal);
			u1 = u;
			v1 = v0;
			a1 = a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

			YUVA_IN(y, u, v0, a, p + BPP, pal);
			u1 += u;
			v1 += v0;
			a1 += a;
			lum[1] = ALPHA_BLEND(a, lum[1], y, 0);
			cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u, 1);
			cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v0, 1);
			cb++;
			cr++;
			p += 2 * BPP;
			lum += 2;
		}
		if (w) {
			YUVA_IN(y, u, v0, a, p, pal);
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
			cr[0] = ALPHA_BLEND(a >> 2, cr[0], v0, 0);
		}
	}
}
#endif

static void free_subpicture(av_player_ffplay_subpicture_ctx *sp)
{
	unsigned int i;
	for (i = 0; i < sp->sub.num_rects; i++)
	{
		av_free(sp->sub.rects[i].bitmap);
		av_free(sp->sub.rects[i].rgba_palette);
	}
	av_free(sp->sub.rects);
	memset(&sp->sub, 0, sizeof(AVSubtitle));
}

static av_bool_t vp_size_changed(av_player_ffplay_status_p status)
{
	av_player_ffplay_videopicture_ctx *vp = &status->pictq[status->pictq_windex];

	if (vp->w_in == status->video_st->codec->width && vp->h_in == status->video_st->codec->height)
	{
		return AV_FALSE;
	}

	vp->w_in = status->video_st->codec->width;
	vp->h_in = status->video_st->codec->height;
	status->aspect_ratio = (double)vp->w_in / (double)vp->h_in;
	if (status->video_st->codec->sample_aspect_ratio.num != 0)
		status->aspect_ratio *= av_q2d(status->video_st->codec->sample_aspect_ratio);

	/* if an active format status indicated, then it overrides the mpeg format */
	if (status->video_st->codec->dtg_active_format != status->dtg_active_format)
	{
		status->dtg_active_format = status->video_st->codec->dtg_active_format;
	}
	switch(status->video_st->codec->dtg_active_format)
	{
		case FF_DTG_AFD_SAME:
		default:
			break;
		case FF_DTG_AFD_4_3:
			status->aspect_ratio = 4.0 / 3.0;
			break;
		case FF_DTG_AFD_16_9:
			status->aspect_ratio = 16.0 / 9.0;
			break;
		case FF_DTG_AFD_14_9:
			status->aspect_ratio = 14.0 / 9.0;
			break;
		case FF_DTG_AFD_4_3_SP_14_9:
			status->aspect_ratio = 14.0 / 9.0;
			break;
		case FF_DTG_AFD_16_9_SP_14_9:
			status->aspect_ratio = 14.0 / 9.0;
			break;
		case FF_DTG_AFD_SP_4_3:
			status->aspect_ratio = 4.0 / 3.0;
			break;
	}

	vp->h_out = vp->h_in;
	vp->w_out = (int)rint(vp->h_out * status->aspect_ratio);

	return AV_TRUE;
}

static void video_subtitles_display(av_player_ffplay_status_p status)
{
	AV_UNUSED(status);
	#if 0 /*~~~SUBTITLES~~~*/
	if (status->subtitle_st)
	{
		if (status->subpq_size > 0)
		{
			av_player_ffplay_subpicture_ctx *sp;
			sp = &status->subpq[status->subpq_rindex];

			if (vp->pts >= sp->pts + ((float) sp->sub.start_display_time / 1000))
			{
				AVPicture pict;
				SDL_LockYUVOverlay (vp->bmp);

				pict.data[0] = vp->bmp->pixels[0];
				pict.data[1] = vp->bmp->pixels[2];
				pict.data[2] = vp->bmp->pixels[1];

				pict.linesize[0] = vp->bmp->pitches[0];
				pict.linesize[1] = vp->bmp->pitches[2];
				pict.linesize[2] = vp->bmp->pitches[1];

				for (i = 0; i < sp->sub.num_rects; i++)
					blend_subrect(&pict, &sp->sub.rects[i],
								vp->bmp->w, vp->bmp->h);

				SDL_UnlockYUVOverlay (vp->bmp);
			}
		}
	}
	#endif
}

static void video_borders_display(av_player_ffplay_status_p status, av_rect_p prect)
{
	av_video_surface_p surface;
	if (AV_OK == status->ctx->video->get_backbuffer(status->ctx->video, &surface))
	{
		av_rect_t brect, mrect;
		double window_aspect_ratio = ((double)prect->w)/prect->h;

		av_rect_copy(&mrect, prect);
		if(window_aspect_ratio < status->aspect_ratio)
		{
			mrect.h = mrect.w;

			if (1.<=status->aspect_ratio)
				mrect.h /= status->aspect_ratio;
			else
				mrect.h *= status->aspect_ratio;

			av_rect_copy(&brect, prect);
			brect.h -= mrect.h;
			brect.h /= 2;
			mrect.y += brect.h;
			if (status->ctx->draw_borders)
			{
				surface->fill_rect(surface, &brect, 0., 0., 0., 1.);
				brect.y = mrect.y + mrect.h;
				surface->fill_rect(surface, &brect, 0., 0., 0., 1.);
			}
		}
		else
		{
			mrect.w = mrect.h;

			if (1.<=status->aspect_ratio)
				mrect.w *= status->aspect_ratio;
			else
				mrect.w /= status->aspect_ratio;

			av_rect_copy(&brect, prect);
			brect.w -= mrect.w;
			brect.w /= 2;
			mrect.x += brect.w;
			if (status->ctx->draw_borders)
			{
				surface->fill_rect(surface, &brect, 0., 0., 0., 1.);
				brect.x = mrect.x + mrect.w;
				surface->fill_rect(surface, &brect, 0., 0., 0., 1.);
			}
		}
		/* update dimensions */
		av_rect_copy(prect, &mrect);
	}
}

static inline int compute_mod(int a, int b)
{
	a = a % b;
	return ((a >= 0)? a : (a + b));
}

/* get the current audio output buffer size, in samples. */
static inline int audio_write_get_buf_size(av_player_ffplay_status_p status)
{
	return status->audio_buf_size - status->audio_buf_index;
}

static av_bool_t schedule_refresh_cb(void* arg)
{
	av_event_t event;
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)arg;
	memset(&event, 0, sizeof(av_event_t));
	event.type = AV_EVENT_USER;
	event.user_id = EVENT_REFRESH_ID;
	event.window = ctx->owner;
	ctx->input->push_event(ctx->input, &event);
	return AV_FALSE;
}

/* schedule a video refresh in 'delay' ms */
static void schedule_refresh(av_player_ffplay_status_p status, int delay)
{
	av_player_ffplay_ctx_p ctx = status->ctx;
	ctx->timer->add_timer(ctx->timer, schedule_refresh_cb, delay, ctx, 0);
}

/* get the current audio clock value */
static double get_audio_clock(av_player_ffplay_status_p status)
{
	double pts;
	int hw_buf_size, bytes_per_sec;
	pts = status->audio_clock;
	hw_buf_size = audio_write_get_buf_size(status);
	bytes_per_sec = 0;
	if (status->audio_st)
	{
		bytes_per_sec = status->audio_st->codec->sample_rate * 2 * status->audio_st->codec->channels;
	}
	if (bytes_per_sec)
		pts -= (double)hw_buf_size / bytes_per_sec;
	return pts;
}

/* get the current video clock value */
static double get_video_clock(av_player_ffplay_status_p status)
{
	double delta = (status->paused)? 0.:((av_gettime() - status->video_current_pts_time) / 1000000.0);
	return status->video_current_pts + delta;
}

/* get the current external clock value */
static double get_external_clock(av_player_ffplay_status_p status)
{
	return status->external_clock + ((av_gettime() - status->external_clock_time) / 1000000.0);
}

/* get the current master clock value */
static double get_master_clock(av_player_ffplay_status_p status)
{
	double val;
	if (status->av_sync_type == AV_SYNC_VIDEO_MASTER)
	{
		if (status->video_st)
			val = get_video_clock(status);
		else
			val = get_audio_clock(status);
	}
	else
	if (status->av_sync_type == AV_SYNC_AUDIO_MASTER)
	{
		if (status->audio_st)
			val = get_audio_clock(status);
		else
			val = get_video_clock(status);
	}
	else
	{
		val = get_external_clock(status);
	}
	return val;
}

/* pause or resume the video */
static void stream_pause(av_player_ffplay_status_p status)
{
	status->paused = !status->paused;
	if (!status->paused)
	{
		status->video_current_pts = get_video_clock(status);
		status->frame_timer += (av_gettime() - status->video_current_pts_time) / 1000000.0;
	}
	if (status->ctx->audio_handle)
		status->ctx->audio->pause(status->ctx->audio, status->ctx->audio_handle);
}

/* called to display each frame */
static void video_refresh_timer(av_player_ffplay_status_p status)
{
	if (status)
	{
		if (status->video_st)
		{
			if (status->pictq_size == 0)
			{
				/* if no picture, need to wait */
				schedule_refresh(status, 2);
			}
			else
			{
				double actual_delay, delay, sync_threshold, ref_clock, diff;

				/* dequeue the picture */
				av_player_ffplay_videopicture_ctx *vp = &status->pictq[status->pictq_rindex];

				/* update current video pts */
				status->video_current_pts = vp->pts;
				status->video_current_pts_time = av_gettime();

				/* compute nominal delay */
				delay = vp->pts - status->frame_last_pts;
				if (delay <= 0 || delay >= 2.0) {
					/* if incorrect delay, use previous one */
					delay = status->frame_last_delay;
				}
				status->frame_last_delay = delay;
				status->frame_last_pts = vp->pts;

				/* update delay to follow master synchronisation source */
				if (((status->av_sync_type == AV_SYNC_AUDIO_MASTER && status->audio_st) ||
					status->av_sync_type == AV_SYNC_EXTERNAL_CLOCK))
				{
					/* if video status slave, we try to correct big delays by
					duplicating or deleting a frame */
					ref_clock = get_master_clock(status);
					diff = vp->pts - ref_clock;

					/* skip or repeat frame. We take into account the
					delay to compute the threshold. I still don't know
					if it status the best guess */
					sync_threshold = AV_SYNC_THRESHOLD_DOUBLE;
					if (delay > sync_threshold)
						sync_threshold = delay;
					if (fabs(diff) < AV_NOSYNC_THRESHOLD_DOUBLE)
					{
						if (diff <= -sync_threshold)
							delay = 0;
						else if (diff >= sync_threshold)
							delay = 2 * delay;
					}
				}

				status->frame_timer += delay;
				/* compute the REAL delay (we need to do that to avoid long term errors */
				actual_delay = status->frame_timer - (av_gettime() / 1000000.0);
				if (actual_delay < 0.010)
				{
					if (status->slow_computer < 100)
					{
						vp->skip = status->config.skip_enabled;
						status->slow_computer++;
					}
					else
					if (status->slow_computer == 100)
					{
						status->slow_computer++;
						log_error("Computer to slow. Synchronization is now off.");
					}
					schedule_refresh(status, 2);
				}
				else
				{
					/* launch timer for next picture */
					schedule_refresh(status, (int)(actual_delay * 1000 + 0.5));
				}

				#if WITH_DEBUG_SYNC
				printf("video:[%c] delay=%0.3f actual_delay=%0.3f pts=%0.3f A-V=%f\n",
					(vp->skip? '@':' '), delay, actual_delay, vp->pts, -diff);
				#endif

				if (!vp->skip)
				{
					if(status->subtitle_st)
					{
						if (status->subtitle_stream_changed)
						{
							status->mtx_subpicture->lock(status->mtx_subpicture);

							while (status->subpq_size)
							{
								free_subpicture(&status->subpq[status->subpq_rindex]);

								/* update queue size and signal for next picture */
								if (++status->subpq_rindex == SUBTITLE_PICTURE_QUEUE_SIZE)
									status->subpq_rindex = 0;

								status->subpq_size--;
							}
							status->subtitle_stream_changed = 0;

							status->cond_subpicture->signal(status->cond_subpicture);
							status->mtx_subpicture->unlock(status->mtx_subpicture);
						}
						else
						{
							if (status->subpq_size > 0)
							{
								av_player_ffplay_subpicture_ctx *sp, *sp2;
								sp = &status->subpq[status->subpq_rindex];

								if (status->subpq_size > 1)
									sp2 = &status->subpq[(status->subpq_rindex + 1) % SUBTITLE_PICTURE_QUEUE_SIZE];
								else
									sp2 = AV_NULL;

								if ((status->video_current_pts > (sp->pts + ((float) sp->sub.end_display_time / 1000)))
										|| (sp2 && status->video_current_pts > (sp2->pts + ((float) sp2->sub.start_display_time / 1000))))
								{
									free_subpicture(sp);

									/* update queue size and signal for next picture */
									if (++status->subpq_rindex == SUBTITLE_PICTURE_QUEUE_SIZE)
										status->subpq_rindex = 0;

									status->mtx_subpicture->lock(status->mtx_subpicture);
									status->subpq_size--;
									status->cond_subpicture->signal(status->cond_subpicture);
									status->mtx_subpicture->unlock(status->mtx_subpicture);
								}
							}
						}
						/* draw subtitles */
						video_subtitles_display(status);
					}

					#if WITH_OVERLAY
					/* blit picture to backbuffer */
					status->ctx->video_overlay->blit(status->ctx->video_overlay, AV_NULL);
					#else
					/* FIXME: rgb code */
					#endif
				}

				/* update queue size and signal for next picture */
				if (++status->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE)
					status->pictq_rindex = 0;

				status->mtx_picture->lock(status->mtx_picture);
				status->pictq_size--;
				status->cond_picture->signal(status->cond_picture);
				status->mtx_picture->unlock(status->mtx_picture);
			}
		}
		else
		if (status->audio_st)
		{
			/* draw the next audio frame */
			schedule_refresh(status, 40);
		}
		else
		{
			schedule_refresh(status, 100);
		}

		#if WITH_DEBUG_SYNC
		{
			static int64_t last_time;
			int64_t cur_time;
			int aqsize, vqsize, sqsize;
			double av_diff;

			cur_time = av_gettime();
			if (!last_time || (cur_time - last_time) >= 500 * 1000)
			{
				aqsize = 0;
				vqsize = 0;
				sqsize = 0;
				if (status->audio_st) aqsize = status->audioq.size;
				if (status->video_st) vqsize = status->videoq.size;
				if (status->subtitle_st) sqsize = status->subtitleq.size;
				av_diff = 0;
				if (status->audio_st && status->video_st)
					av_diff = get_audio_clock(status) - get_video_clock(status);
				printf("%7.2f A-V:%7.3f aq=%5dKB vq=%5dKB sq=%5dB    \r", get_master_clock(status), av_diff, aqsize / 1024, vqsize / 1024, sqsize);
				fflush(stdout);
				last_time = cur_time;
			}
		}
		#endif
	}
}

/* allocate a picture (needs to do that in main thread to avoid potential locking problems */
static void alloc_picture(av_player_ffplay_status_p status)
{
	if (status)
	{
		av_player_ffplay_videopicture_ctx *vp = &status->pictq[status->pictq_windex];

		#if WITH_OVERLAY
		av_video_overlay_p overlay = status->ctx->video_overlay;
		if (overlay)
		{
			overlay->set_size_format(overlay, vp->w_in, vp->h_in, status->ctx->video_overlay_format);
		}
		#else
		av_surface_p surface = (av_surface_p)status->ctx->video_surface;
		if (surface)
		{
			surface->set_size(surface, vp->w_out, vp->h_out);
		}
		#endif

		status->mtx_picture->lock(status->mtx_picture);
		vp->allocated = 1;
		status->cond_picture->signal(status->cond_picture);
		status->mtx_picture->unlock(status->mtx_picture);
	}
}

static av_bool_t validate_picture_size(av_player_ffplay_status_p status, int width, int height)
{
	if (!vp_size_changed(status))
	{
		#if WITH_OVERLAY
		if (status->ctx->video_overlay)
		{
			return status->ctx->video_overlay->validate_size(status->ctx->video_overlay, width, height);
		}
		#else
		int w,h;
		av_surface_p surface = (av_surface_p)status->ctx->video_surface;
		if (surface)
		{
			surface->get_size(surface, &w, &h);
			return (w==width && h==height);
		}
		#endif
	}
	return AV_FALSE;;
}

/* pts the dts of the pkt / pts of the frame and guessed if not known */
static int queue_picture(av_player_ffplay_status_p status, AVFrame* src_frame, double pts)
{
	av_player_ffplay_videopicture_ctx *vp = &status->pictq[status->pictq_windex];

	/* wait until we have space to put a new picture */
	status->mtx_picture->lock(status->mtx_picture);
	while (status->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && !status->videoq.abort_request)
	{
		status->cond_picture->wait(status->cond_picture, status->mtx_picture);
	}
	status->mtx_picture->unlock(status->mtx_picture);

	if (status->videoq.abort_request)
		return -1;

	if (!validate_picture_size(status, vp->w_in, vp->h_in))
	{
		av_event_t event;
		/* alloc or resize hardware picture buffer */
		vp->allocated = 0;
		memset(&event, 0, sizeof(av_event_t));
		event.type = AV_EVENT_USER;
		event.user_id = EVENT_ALLOC_ID;
		event.window = status->ctx->owner;
		status->ctx->input->push_event(status->ctx->input, &event);

		/* wait until the picture status allocated */
		status->mtx_picture->lock(status->mtx_picture);
		while (!vp->allocated && !status->videoq.abort_request)
		{
			status->cond_picture->wait(status->cond_picture, status->mtx_picture);
		}
		status->mtx_picture->unlock(status->mtx_picture);

		if (status->videoq.abort_request)
			return -1;
	}

	if (vp->skip)
		vp->skip = 0;
	else
	{
		/* if the frame status not skipped, then display it */
		#if WITH_OVERLAY
		if (status->ctx->video_overlay)
		{
			status->ctx->frame_to_overlay(status->ctx->video_overlay, src_frame, vp->h_in);
		}
		#else
		if (status->ctx->video_surface)
		{
			av_frame_video_t src_frame_video;
			status->ctx->scale_info.dst_format = AV_VIDEO_FORMAT_RGB32;
			status->ctx->scale_info.dst_width = vp->w_out;
			status->ctx->scale_info.dst_height = vp->h_out;
			status->ctx->scaler->set_configuration(status->ctx->scaler,
													&status->ctx->scale_info);

			src_frame_video.data = src_frame->data;
			src_frame_video.linesize = src_frame->linesize;
			src_frame_video.height = status->ctx->scale_info.src_height;
			status->ctx->scaler->scale(status->ctx->scaler, &src_frame_video,
										(av_surface_p)status->ctx->video_surface);
		}
		#endif

		vp->pts = pts;
		/* now we can update the picture count */
		if (++status->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE)
			status->pictq_windex = 0;
		status->mtx_picture->lock(status->mtx_picture);
		status->pictq_size++;
		status->mtx_picture->unlock(status->mtx_picture);
	}
	return 0;
}

/* compute the exact PTS for the picture if it status omitted in the stream */
static int output_picture2(av_player_ffplay_status_p status, AVFrame *src_frame, double pts)
{
	double frame_delay, clk;

	clk = pts;

	if (clk != 0)
	{
		/* update video clk with clk, if present */
		status->video_clock = clk;
	}
	else
	{
		clk = status->video_clock;
	}
	/* update video clk for next frame */
	frame_delay = av_q2d(status->video_st->codec->time_base);
	/* for MPEG2, the frame can be repeated, so we update the clk accordingly */
	frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
	status->video_clock += frame_delay;

	#if WITH_DEBUG_SYNC
	{
		int ftype = (src_frame->pict_type == FF_B_TYPE)? 'B':
			((src_frame->pict_type == FF_I_TYPE)? 'I' : 'P');
		printf("frame_type=%c clock=%0.3f pts=%0.3f\n", ftype, clk, pts);
	}
	#endif
	return queue_picture(status, src_frame, clk);
}

static int my_get_buffer(struct AVCodecContext *c, AVFrame *pic)
{
	int rc = avcodec_default_get_buffer(c, pic);
	int64_t *pts = av_malloc(sizeof(int64_t));
	*pts = (pic->opaque)? *(int64_t*)pic->opaque : 0;
	pic->opaque = pts;
	return rc;
}

static void my_release_buffer(struct AVCodecContext *c, AVFrame *pic)
{
	if(pic) av_freep(&pic->opaque);
	avcodec_default_release_buffer(c, pic);
}

static int video_decoder_thread(av_thread_p thread)
{
	av_bool_t want_key_frame = AV_FALSE;
	av_result_t rc = AV_OK;
	av_player_ffplay_ctx_p ctx = thread->arg;
	av_player_ffplay_status_p status = ctx->status;
	AVFrame *frame= avcodec_alloc_frame();
	int64_t frame_pts = AV_NOPTS_VALUE;
	AVPacket pkt1, *pkt = &pkt1;
	int len1, got_picture;
	double pts;

	while (!status->abort_request)
	{
		#if WITH_INTERRUPT
		av_bool_t interrupted;
		thread->is_interrupted(thread, &interrupted);
		if (interrupted) break;
		#endif

		while (status->paused && !status->videoq.abort_request)
		{
			status->ctx->timer->sleep_ms(10);
		}
		if (packet_queue_get(&status->videoq, pkt, 1) < 0)
			break;

		if(pkt->data == status->flush_pkt.data)
		{
			avcodec_flush_buffers(status->video_st->codec);
			continue;
		}

		/* NOTE: ipts status the PTS of the _first_ picture beginning in this packet, if any */
		frame_pts = pkt->pts;
		frame->opaque = &frame_pts;
		len1 = avcodec_decode_video(status->video_st->codec, frame, &got_picture, pkt->data, pkt->size);

		if((status->config.decoder_reorder_pts || pkt->dts == (int64_t)AV_NOPTS_VALUE)
		&& frame->opaque && *(int64_t*)frame->opaque != (int64_t)AV_NOPTS_VALUE)
			pts = *(int64_t*)frame->opaque;
		else
		if(pkt->dts != (int64_t)AV_NOPTS_VALUE)
			pts = pkt->dts;
		else
			pts = 0;

		pts *= av_q2d(status->video_st->time_base);

		if (len1 < 0)
		{
			want_key_frame = AV_TRUE;
			/** FIXED: break; */
			continue;
		}

		if (got_picture)
		{
			if(want_key_frame)
			{
				if(frame->pict_type != FF_I_TYPE)
				{
					av_free_packet(pkt);
					continue;
				}
				want_key_frame = AV_FALSE;
			}
			if (output_picture2(status, frame, pts) < 0)
			{
				av_free_packet(pkt);
				break;
			}
		}
		else
		{
			want_key_frame = AV_TRUE;
		}
		av_free_packet(pkt);
	}
	av_free(frame);
	return rc;
}

static int subtitle_decoder_thread(av_thread_p thread)
{
	av_result_t rc = AV_OK;
	av_player_ffplay_ctx_p ctx = thread->arg;
	av_player_ffplay_status_p status = ctx->status;
	av_player_ffplay_subpicture_ctx *sp;
	AVPacket pkt1, *pkt = &pkt1;
	int len1, got_subtitle;
	double pts;
	unsigned int i;
	int j;
	int r, g, b, y, u, v0, a;

	while (!status->abort_request)
	{
		#if WITH_INTERRUPT
		av_bool_t interrupted;
		thread->is_interrupted(thread, &interrupted);
		if (interrupted) break;
		#endif

		while (status->paused && !status->subtitleq.abort_request)
		{
			status->ctx->timer->sleep_ms(10);
		}
		if (packet_queue_get(&status->subtitleq, pkt, 1) < 0)
			break;

		if(pkt->data == status->flush_pkt.data)
		{
			avcodec_flush_buffers(status->subtitle_st->codec);
			continue;
		}
		status->mtx_subpicture->lock(status->mtx_subpicture);
		while (status->subpq_size >= SUBTITLE_PICTURE_QUEUE_SIZE && !status->subtitleq.abort_request)
		{
			status->cond_subpicture->wait(status->cond_subpicture,status->mtx_subpicture);
		}
		status->mtx_subpicture->unlock(status->mtx_subpicture);

		if (status->subtitleq.abort_request)
			break;

		sp = &status->subpq[status->subpq_windex];

		/* NOTE: ipts status the PTS of the _first_ picture beginning in this packet, if any */
		pts = 0;
		if (pkt->pts != (int64_t)AV_NOPTS_VALUE)
			pts = av_q2d(status->subtitle_st->time_base)*pkt->pts;

		len1 = avcodec_decode_subtitle(status->subtitle_st->codec, &sp->sub, &got_subtitle, pkt->data, pkt->size);
		if (len1 < 0)
			break;
		if (got_subtitle && sp->sub.format == 0)
		{
			sp->pts = pts;
			for (i = 0; i < sp->sub.num_rects; i++)
			{
				for (j = 0; j < sp->sub.rects[i].nb_colors; j++)
				{
					RGBA_IN(r, g, b, a, sp->sub.rects[i].rgba_palette + j);
					y = RGB_TO_Y_CCIR(r, g, b);
					u = RGB_TO_U_CCIR(r, g, b, 0);
					v0 = RGB_TO_V_CCIR(r, g, b, 0);
					YUVA_OUT(sp->sub.rects[i].rgba_palette + j, y, u, v0, a);
				}
			}
			/* now we can update the picture count */
			if (++status->subpq_windex == SUBTITLE_PICTURE_QUEUE_SIZE)
				status->subpq_windex = 0;
			status->mtx_subpicture->lock(status->mtx_subpicture);
			status->subpq_size++;
			status->mtx_subpicture->unlock(status->mtx_subpicture);
		}
		av_free_packet(pkt);
	}
	return rc;
}

/* return the new audio buffer size (samples can be added or deleted
to get better sync if video or external master clock) */
static int synchronize_audio(av_player_ffplay_status_p status, short *samples, int samples_size_in)
{
	int n, samples_size;
	double ref_clock;

	n = 2 * status->audio_st->codec->channels;
	samples_size = samples_size_in;

	/* if not master, then we try to remove or add samples to correct the clock */
	if ((status->av_sync_type == AV_SYNC_VIDEO_MASTER && status->video_st) || status->av_sync_type == AV_SYNC_EXTERNAL_CLOCK)
	{
		double diff, avg_diff;
		int wanted_size, min_size, max_size, nb_samples;

		ref_clock = get_master_clock(status);
		diff = get_audio_clock(status) - ref_clock;

		if (diff < AV_NOSYNC_THRESHOLD_DOUBLE)
		{
			status->audio_diff_cum = diff + status->audio_diff_avg_coef * status->audio_diff_cum;
			if (status->audio_diff_avg_count < AUDIO_DIFF_AVG_NB)
			{
				/* not enough measures to have a correct estimate */
				status->audio_diff_avg_count++;
			}
			else
			{
				/* estimate the A-V difference */
				avg_diff = status->audio_diff_cum * (1.0 - status->audio_diff_avg_coef);

				if (fabs(avg_diff) >= status->audio_diff_threshold)
				{
					nb_samples = samples_size / n;
					wanted_size = samples_size + ((int)(diff * status->audio_st->codec->sample_rate) * n);

					min_size = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX_DOUBLE)) / 100) * n;
					max_size = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX_DOUBLE)) / 100) * n;
					if (wanted_size < min_size)
						wanted_size = min_size;
					else
					if (wanted_size > max_size)
						wanted_size = max_size;

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
						unsigned char *samples_end, *q;
						/* add samples */
						nb = (samples_size - wanted_size);
						samples_end = (unsigned char *)samples + samples_size - n;
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
				#if 0
				printf("diff=%f adiff=%f sample_diff=%d apts=%0.3f vpts=%0.3f %f\n",
					diff, avg_diff, samples_size - samples_size_in,
					status->audio_clock, status->video_clock, status->audio_diff_threshold);
				#endif
			}
		}
		else
		{
			/* too big difference : may be initial PTS errors, so reset A-V filter */
			status->audio_diff_avg_count = 0;
			status->audio_diff_cum = 0;
		}
	}

	return samples_size;
}

/* decode one audio frame and returns its uncompressed size */
static int audio_decode_frame(av_player_ffplay_status_p status, unsigned char *audio_buf, int buf_size, double *pts_ptr)
{
	AVPacket *pkt = &status->audio_pkt;
	int n, len1, data_size;
	double pts;

	for(;;) {
		/* NOTE: the audio packet can contain several frames */
		while (status->audio_pkt_size > 0)
		{
			data_size = buf_size;
			len1 = avcodec_decode_audio2(status->audio_st->codec, (int16_t*)audio_buf, &data_size, status->audio_pkt_data, status->audio_pkt_size);
			if (len1 < 0)
			{
				/* if error, we skip the frame */
				status->audio_pkt_size = 0;
				break;
			}

			status->audio_pkt_data += len1;
			status->audio_pkt_size -= len1;
			if (data_size <= 0)
				continue;
			/* if no pts, then compute it */
			pts = status->audio_clock;
			*pts_ptr = pts;
			n = 2 * status->audio_st->codec->channels;
			status->audio_clock += (double)data_size/(double)(n * status->audio_st->codec->sample_rate);
			#if WITH_DEBUG_SYNC
			{
				static double last_clock;
				printf("audio: delay=%0.3f clock=%0.3f pts=%0.3f\n", status->audio_clock - last_clock, status->audio_clock, pts);
				last_clock = status->audio_clock;
			}
			#endif
			return data_size;
		}

		/* free the current packet */
		if (pkt->data)
			av_free_packet(pkt);

		if (status->paused || status->audioq.abort_request)
		{
			return -1;
		}

		/* read next packet */
		if (packet_queue_get(&status->audioq, pkt, 1) < 0)
			return -1;
		if(pkt->data == status->flush_pkt.data)
		{
			avcodec_flush_buffers(status->audio_st->codec);
			continue;
		}

		status->audio_pkt_data = pkt->data;
		status->audio_pkt_size = pkt->size;

		/* if update the audio clock with the pts */
		if (pkt->pts != (int64_t)AV_NOPTS_VALUE)
		{
			status->audio_clock = av_q2d(status->audio_st->time_base)*pkt->pts;
		}
	}
}

/* prepare a new audio buffer */
static void write_audio_cb(void* userdata, unsigned char* data, int length)
{
	av_player_ffplay_status_p status = userdata;
	int audio_size, size;
	double pts;

	status->audio_callback_time = av_gettime();

	while (length > 0)
	{
		if (status->audio_buf_index >= status->audio_buf_size)
		{
			audio_size = audio_decode_frame(status, status->audio_buf, sizeof(status->audio_buf), &pts);
			if (audio_size < 0)
			{
				/* if error, just output silence */
				status->audio_buf_size = 1024;
				memset(status->audio_buf, 0, status->audio_buf_size);
			}
			else
			{
				audio_size = synchronize_audio(status, (int16_t *)status->audio_buf, audio_size);
				status->audio_buf_size = audio_size;
			}
			status->audio_buf_index = 0;
		}
		size = status->audio_buf_size - status->audio_buf_index;
		if (size > length)
			size = length;
		memcpy(data, (unsigned char *)status->audio_buf + status->audio_buf_index, size);
		length -= size;
		data += size;
		status->audio_buf_index += size;
	}
}

/* open a given stream. Return 0 if OK */
static int stream_component_open(av_player_ffplay_status_p status, int stream_index)
{
	AVFormatContext *ic = status->ic;
	AVCodecContext *enc;
	AVCodec *codec;

	if (stream_index < 0 || stream_index >= (int)ic->nb_streams)
		return -1;
	enc = ic->streams[stream_index]->codec;

	codec = avcodec_find_decoder(enc->codec_id);
	enc->debug_mv = status->config.debug_mv;
	enc->debug = status->config.debug;
	enc->workaround_bugs = status->config.workaround_bugs;
	enc->lowres = status->config.lowres;
	if (status->config.lowres) enc->flags |= CODEC_FLAG_EMU_EDGE;
	enc->idct_algo= status->config.idct;
	if (status->config.fast) enc->flags2 |= CODEC_FLAG2_FAST;
	enc->skip_frame= status->config.skip_frame;
	enc->skip_idct= status->config.skip_idct;
	enc->skip_loop_filter= status->config.skip_loop_filter;
	enc->error_resilience= status->config.error_resilience;
	enc->error_concealment= status->config.error_concealment;
	if (!codec || avcodec_open(enc, codec) < 0) return -1;
	if (status->config.thread_count>1) avcodec_thread_init(enc, status->config.thread_count);
	enc->thread_count= status->config.thread_count;

	switch(enc->codec_type)
	{
		case CODEC_TYPE_AUDIO:
			status->audio_stream = stream_index;
			status->audio_st = ic->streams[stream_index];
			status->audio_buf_size = 0;
			status->audio_buf_index = 0;
			/* init averaging filter */
			status->audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
			status->audio_diff_avg_count = 0;
			/* since we do not have a precise anough audio fifo fullness,
			we correct audio sync only if larger than this threshold */
			status->audio_diff_threshold = 2.0 * AV_HW_AUDIO_BUFFER_SIZE / enc->sample_rate;
			memset(&status->audio_pkt, 0, sizeof(status->audio_pkt));
			packet_queue_init(&status->audioq);
			break;
		case CODEC_TYPE_VIDEO:
			status->video_stream = stream_index;
			status->video_st = ic->streams[stream_index];
			status->frame_last_delay = 0.040;
			status->frame_timer = (double)av_gettime() / 1000000.0;
			status->video_current_pts_time = av_gettime();
			packet_queue_init(&status->videoq);
			if (AV_OK == av_thread_create(video_decoder_thread, status->ctx, &status->ctx->thd_video_decoder))
			{
				status->ctx->thd_video_decoder_created = AV_TRUE;
				status->ctx->thd_video_decoder->start(status->ctx->thd_video_decoder);
			}
			enc->get_buffer = my_get_buffer;
			enc->release_buffer = my_release_buffer;
			break;
		case CODEC_TYPE_SUBTITLE:
			status->subtitle_stream = stream_index;
			status->subtitle_st = ic->streams[stream_index];
			packet_queue_init(&status->subtitleq);
			if (AV_OK == av_thread_create(subtitle_decoder_thread, status->ctx, &status->ctx->thd_subtitle_decoder))
			{
				status->ctx->thd_subtitle_decoder_created = AV_TRUE;
				status->ctx->thd_subtitle_decoder->start(status->ctx->thd_subtitle_decoder);
			}
			break;
		default:
			break;
	}
	return 0;
}

static void stream_component_close(av_player_ffplay_status_p status, int stream_index)
{
	AVFormatContext *ic = status->ic;
	AVCodecContext *enc;

	if (stream_index < 0 || stream_index >= (int)ic->nb_streams)
		return;
	enc = ic->streams[stream_index]->codec;

	switch(enc->codec_type)
	{
		case CODEC_TYPE_AUDIO:
			packet_queue_abort(&status->audioq);
			packet_queue_end(&status->audioq);
			if (status->ctx->audio_handle)
			{
				status->ctx->audio->close(status->ctx->audio, status->ctx->audio_handle);
				status->ctx->audio_handle = AV_NULL;
			}
			break;
		case CODEC_TYPE_VIDEO:
			packet_queue_abort(&status->videoq);
			/* NOTE: we also signal this mutex to make sure we deblock the video thread in all cases */
			status->mtx_picture->lock(status->mtx_picture);
			status->cond_picture->signal(status->cond_picture);
			status->mtx_picture->unlock(status->mtx_picture);
			#if WITH_INTERRUPT
			status->ctx->thd_video_decoder->interrupt(status->ctx->thd_video_decoder);
			#endif
			status->ctx->thd_video_decoder->join(status->ctx->thd_video_decoder);
			packet_queue_end(&status->videoq);
			break;
		case CODEC_TYPE_SUBTITLE:
			packet_queue_abort(&status->subtitleq);
			/* NOTE: we also signal this mutex to make sure we deblock the sutitle thread in all cases */
			status->mtx_subpicture->lock(status->mtx_subpicture);
			status->subtitle_stream_changed = 1;
			status->cond_subpicture->signal(status->cond_subpicture);
			status->mtx_subpicture->unlock(status->mtx_subpicture);
			#if WITH_INTERRUPT
			status->ctx->thd_subtitle_decoder->interrupt(status->ctx->thd_subtitle_decoder);
			#endif
			status->ctx->thd_subtitle_decoder->join(status->ctx->thd_subtitle_decoder);
			packet_queue_end(&status->subtitleq);
			break;
		default:
			break;
	}
	avcodec_close(enc);

	switch(enc->codec_type)
	{
		case CODEC_TYPE_AUDIO:
			status->audio_st = AV_NULL;
			status->audio_stream = -1;
			break;
		case CODEC_TYPE_VIDEO:
			status->video_st = AV_NULL;
			status->video_stream = -1;
			break;
		case CODEC_TYPE_SUBTITLE:
			status->subtitle_st = AV_NULL;
			status->subtitle_stream = -1;
			break;
		default:
			break;
	}
}

/** FIXME
static int decode_interrupt_cb(void)
{
	return (global_video_state && global_video_state->abort_request);
} */

/* this thread gets the stream from the disk or the network */
static int packet_reader_thread(av_thread_p thread)
{
	av_player_ffplay_ctx_p ctx = thread->arg;
	av_player_ffplay_status_p status = ctx->status;

	AVPacket pkt1, *pkt = &pkt1;

	status->playing = 1;

	if (status->ctx->audio_handle)
		status->ctx->audio->play(status->ctx->audio, status->ctx->audio_handle);

	while (!status->abort_request)
	{
		#if WITH_INTERRUPT
		av_bool_t interrupted;
		thread->is_interrupted(thread, &interrupted);
		if (interrupted) break;
		#endif

		if (status->paused != status->last_paused)
		{
			status->last_paused = status->paused;
			if (status->paused)
				av_read_pause(status->ic);
			else
				av_read_play(status->ic);
		}
		#if defined(CONFIG_RTSP_DEMUXER) || defined(CONFIG_MMSH_PROTOCOL)
		if (status->paused && (!strcmp(ic->iformat->name, "rtsp") || !strcmp(url_fileno(&ic->pb)->prot->name, "mmsh")))
		{
			/* wait 10 ms to avoid trying to get another packet XXX: horrible */
			status->ctx->timer->sleep_ms(10);
			continue;
		}
		#endif
		if (status->seek_request)
		{
			int stream_index= -1;
			int64_t seek_target= status->seek_pos;

			if (status->video_stream >= 0)
				stream_index = status->video_stream;
			else
			if (status->audio_stream >= 0)
				stream_index = status->audio_stream;
			else
			if (status->subtitle_stream >= 0)
				stream_index = status->subtitle_stream;

			if (stream_index>=0)
			{
				seek_target = av_rescale_q(seek_target, AV_TIME_BASE_Q, status->ic->streams[stream_index]->time_base);
			}

			if ( 0 <= av_seek_frame(status->ic, stream_index, seek_target, status->seek_flags))
			{
				if (status->video_stream >= 0)
				{
					packet_queue_flush(&status->videoq);
					packet_queue_put(&status->videoq, &status->flush_pkt);
				}
				if (status->audio_stream >= 0)
				{
					packet_queue_flush(&status->audioq);
					packet_queue_put(&status->audioq, &status->flush_pkt);
				}
				if (status->subtitle_stream >= 0)
				{
					packet_queue_flush(&status->subtitleq);
					packet_queue_put(&status->subtitleq, &status->flush_pkt);
				}
			}
			status->seek_request = 0;
		}
		/* if the queue are full, no need to read more */
		if (status->audioq.size > MAX_AUDIO_QUEUE_SIZE ||
			status->videoq.size > MAX_VIDEO_QUEUE_SIZE ||
			status->subtitleq.size > MAX_SUBTITLE_QUEUE_SIZE ||
			url_feof(URLARG(status->ic->pb)))
		{
			/* wait 10 ms */
			status->ctx->timer->sleep_ms(10);
			status->playing = !url_feof(URLARG(status->ic->pb));
			continue;
		}
		if( 0 > av_read_frame(status->ic, pkt))
		{
			if (url_ferror(URLARG(status->ic->pb)) == 0)
			{
				status->ctx->timer->sleep_ms(100); /* wait for user event */
				continue;
			} else
				break;
		}
		if (pkt->stream_index == status->audio_stream)
		{
			packet_queue_put(&status->audioq, pkt);
		}
		else
		if (pkt->stream_index == status->video_stream)
		{
			packet_queue_put(&status->videoq, pkt);
		}
		else
		if (pkt->stream_index == status->subtitle_stream)
		{
			packet_queue_put(&status->subtitleq, pkt);
		}
		else
		{
			av_free_packet(pkt);
		}
	}
	status->playing = 0;
	/* wait until the end */
	while (!status->abort_request)
	{
		status->ctx->timer->sleep_ms(100);
	}

	return 0;
}

static av_result_t prepare_player(av_player_ffplay_status_p status, const char* url)
{
	int i;
	AVFormatContext *ic;

	int audio_index = -1;
	int video_index = -1;
	status->video_stream = -1;
	status->audio_stream = -1;
	status->subtitle_stream = -1;

	if (0 > av_open_input_file(&ic, url, status->iformat, 0, AV_NULL))
	{
		return AV_EARG;
	}
	status->ic = ic;

	if(status->config.genpts)
		ic->flags |= AVFMT_FLAG_GENPTS;

	if (av_find_stream_info(ic) < 0)
	{
		return AV_EARG;
	}
	if (ic->pb)
		URLICPB(ic->pb,eof_reached) = 0;

	/** FIXME
	global_video_state = status;
	url_set_interrupt_cb(decode_interrupt_cb); */
	url_set_interrupt_cb(AV_NULL);

	for(i = 0; i < (int)ic->nb_streams; i++)
	{
		AVCodecContext *enc = ic->streams[i]->codec;
		switch(enc->codec_type)
		{
			case CODEC_TYPE_AUDIO:
				if (audio_index < 0)
				{
					audio_index = i;
					status->ctx->audio_info.codec_id = enc->codec_id;
					status->ctx->audio_info.nchannels = enc->channels;
					status->ctx->audio_info.freq = enc->sample_rate;
					status->ctx->audio_info.format = FF_AUDIO_FORMAT(enc->sample_fmt);
				}
				break;
			case CODEC_TYPE_VIDEO:
				if (video_index < 0)
				{
					video_index = i;
					status->ctx->video_info.codec_id = enc->codec_id;
					if (ic->streams[i]->r_frame_rate.den && ic->streams[i]->r_frame_rate.num)
						status->ctx->video_info.fps = av_q2d(ic->streams[i]->r_frame_rate);
					else
						status->ctx->video_info.fps = 1. / av_q2d(ic->streams[i]->codec->time_base);

					status->ctx->video_info.fourcc.fcc = enc->pix_fmt;

					switch (enc->pix_fmt)
					{
						case PIX_FMT_RGB32:
							status->ctx->video_info.format = AV_VIDEO_FORMAT_RGB32;
						break;
						case PIX_FMT_RGB24:
							status->ctx->video_info.format = AV_VIDEO_FORMAT_RGB24;
						break;
						case PIX_FMT_YUV420P:
							status->ctx->video_info.format = AV_VIDEO_FORMAT_YUV420P;
						break;
						default:
							status->ctx->video_info.format = AV_VIDEO_FORMAT_UNKNOWN;
						break;
					}
					status->ctx->video_info.width = enc->width;
					status->ctx->video_info.height = enc->height;
				}
				break;
			default:
				break;
		}
	}

	/* open the streams */
	if (audio_index >= 0)
	{
		stream_component_open(status, audio_index);
	}

	if (video_index >= 0)
	{
		stream_component_open(status, video_index);
	}

	status->ctx->has_video = (0 <= status->video_stream)? AV_TRUE : AV_FALSE;
	status->ctx->has_audio = (0 <= status->audio_stream)? AV_TRUE : AV_FALSE;

	if (status->video_stream < 0 && status->audio_stream < 0)
	{
		return AV_EFOUND;
	}

	/* create packet reader thread */
	if (AV_OK == av_thread_create(packet_reader_thread, status->ctx, &status->ctx->thd_packet_reader))
	{
		status->ctx->thd_packet_reader_created = AV_TRUE;
	}

	return AV_OK;
}

static av_result_t av_player_ffplay_play(av_player_p self)
{
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);

	if (ctx->status)
	{
		av_player_ffplay_status_p status = ctx->status;
		status->abort_request = 0;
		/* start video display */
		av_mutex_create(&status->mtx_picture);
		av_condition_create(&status->cond_picture);
		av_mutex_create(&status->mtx_subpicture);
		av_condition_create(&status->cond_subpicture);
		/* add the refresh timer to draw the picture */
		schedule_refresh(status, 1);
		status->av_sync_type = status->config.av_sync_type;
		status->ctx->thd_packet_reader->start(status->ctx->thd_packet_reader);
	}
	else
		return AV_EARG;

	return AV_OK;
}

static av_result_t av_player_ffplay_pause(av_player_p self)
{
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);
	if (ctx->status)
		stream_pause(ctx->status);
	else
		return AV_EARG;
	return AV_OK;
}

static av_result_t av_player_ffplay_seek(av_player_p self, double pos_seconds)
{
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);
	if (ctx->status)
	{
		if(!ctx->status->abort_request && !ctx->status->seek_request)
		{
			if (0. > pos_seconds)
			{
				pos_seconds = 0.;
			}
			if (self->duration(self) < pos_seconds)
			{
				self->stop(self);
				return AV_OK;
			}
			ctx->status->seek_flags = 0;
			ctx->status->seek_pos = (int64_t)(pos_seconds * AV_TIME_BASE);
			ctx->status->seek_request = 1;
		}
	}
	else
		return AV_EFOUND;
	return AV_OK;
}

static av_result_t av_player_ffplay_seek_rel(struct av_player* self, double pos_seconds)
{
	return self->seek(self,(self->position(self)+pos_seconds));
}

static double av_player_ffplay_duration(struct av_player* self)
{
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);
	return ((!ctx->status)? 0. : (double)ctx->status->ic->duration/(double)AV_TIME_BASE);
}

static double av_player_ffplay_position(struct av_player* self)
{
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);
	return ((!ctx->status)? 0. : get_master_clock(ctx->status));
}

static av_result_t av_player_ffplay_is_playing(struct av_player* self)
{
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);
	return (ctx->status)?
            (ctx->status->playing? avps_running|avps_playing : avps_running) : avps_finished;
}

static av_result_t av_player_ffplay_is_pause(struct av_player* self)
{
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);
    return (ctx->status)?
            (ctx->status->paused? avps_running|avps_paused : avps_running) : avps_finished;
}

static av_bool_t av_player_on_update(av_window_p self, av_video_surface_p dstsurface, av_rect_p rect)
{
	av_rect_t dstrect;
	//av_video_surface_p dstsurface;
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);

	self->get_rect(self, &dstrect);
	dstrect.x = dstrect.y = 0;
	self->rect_absolute(self, &dstrect);
	//ctx->video->get_backbuffer(ctx->video, &dstsurface);

	AV_UNUSED(rect);

	if (ctx->status)
	{
		#if WITH_OVERLAY
		if (ctx->video_overlay && ctx->video_overlay->validate(ctx->video_overlay))
		{
			video_borders_display(ctx->status, &dstrect);
			ctx->video_overlay->set_size_back(ctx->video_overlay, dstrect.w, dstrect.h);
			ctx->video_overlay->blit_back(ctx->video_overlay, AV_NULL, &dstrect);
		}
		#else
		if (ctx->video_surface)
		{
			av_rect_t srcrect;
			av_surface_p srcsurface = (av_surface_p)ctx->video_surface;
			video_borders_display(ctx->status, &dstrect);
			srcrect.x = srcrect.y = 0;
			srcsurface->get_size(srcsurface, &srcrect.w, &srcrect.h);
			ctx->video_surface->blit(dstsurface, &dstrect, srcsurface, &srcrect);
		}
		#endif
		else
		{
			/* Black rectangle */
			dstsurface->fill_rect(dstsurface, &dstrect, 0., 0., 0., 1.);
		}
	}
	else
	{
		/* Black rectangle */
		dstsurface->fill_rect(dstsurface, &dstrect, 0., 0., 0., 1.);
	}
	return AV_TRUE;
}

static av_bool_t av_player_on_user(av_window_p self, int id, void* data)
{
	AV_UNUSED(data);
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);
	switch(id)
	{
		case EVENT_REFRESH_ID:
			video_refresh_timer(ctx->status);
			self->update(self, AV_UPDATE_INVALIDATE);
		break;
		case EVENT_ALLOC_ID:
			alloc_picture(ctx->status);
		break;
	}

	return AV_TRUE;
}

static void av_player_ffplay_set_window(av_player_p self, av_window_p window)
{
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);
	ctx->owner = window;
	ctx->owner->on_update = av_player_on_update;
	ctx->owner->on_user = av_player_on_user;
	O_set_attr(ctx->owner, CONTEXT, ctx);

	window->update(window, AV_UPDATE_INVALIDATE);
}

static av_result_t av_player_ffplay_stop(av_player_p self)
{
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);
	av_player_ffplay_status_p status = ctx->status;

	if(status)
	{
		if (!status->abort_request)
		{
			status->abort_request = 1;
			#if WITH_INTERRUPT
			ctx->thd_packet_reader->interrupt(status->ctx->thd_packet_reader);
			#endif
			ctx->thd_packet_reader->join(status->ctx->thd_packet_reader);

			/** FIXME
			global_video_state = AV_NULL; */

			/* close each stream */
			if (status->audio_stream >= 0)
				stream_component_close(status, status->audio_stream);
			if (status->video_stream >= 0)
				stream_component_close(status, status->video_stream);
			if (status->subtitle_stream >= 0)
				stream_component_close(status, status->subtitle_stream);

			if (status->ic)
			{
				av_close_input_file(status->ic);
				status->ic = AV_NULL;
			}
			url_set_interrupt_cb(AV_NULL);

			status->cond_subpicture->destroy(status->cond_subpicture);
			status->mtx_subpicture->destroy(status->mtx_subpicture);
			status->cond_picture->destroy(status->cond_picture);
			status->mtx_picture->destroy(status->mtx_picture);

			if (ctx->thd_subtitle_decoder_created)
				ctx->thd_subtitle_decoder->destroy(ctx->thd_subtitle_decoder);
			if (ctx->thd_video_decoder_created)
				ctx->thd_video_decoder->destroy(ctx->thd_video_decoder);
			if (ctx->thd_packet_reader_created)
				ctx->thd_packet_reader->destroy(ctx->thd_packet_reader);

			#if WITH_OVERLAY
			if (ctx->video_overlay)
			{
				O_destroy(ctx->video_overlay);
				ctx->video_overlay = AV_NULL;
			}
			#else
			if (ctx->video_surface)
			{
				O_destroy(ctx->video_surface);
				ctx->video_surface = AV_NULL;
			}
			#endif
			status->abort_request = 0;
		}
		free(ctx->status);
		ctx->status = AV_NULL;
	}
	return AV_OK;
}

/* open media file */
static av_result_t av_player_ffplay_open(av_player_p self, const char* url)
{
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);
	av_player_ffplay_status_p status;

	self->stop(self);

	status = malloc(sizeof(av_player_ffplay_status_t));
	if (!status)
		return AV_EMEM;
	else
		memset(status, 0, sizeof(av_player_ffplay_status_t));

	ctx->status = status;
	ctx->status->ctx = ctx;
	ctx->has_video = AV_FALSE;
	ctx->has_audio = AV_FALSE;
	ctx->has_subtitle = AV_FALSE;
	ctx->thd_packet_reader_created = AV_FALSE;
	ctx->thd_video_decoder_created = AV_FALSE;
	ctx->thd_subtitle_decoder_created = AV_FALSE;
	#if WITH_OVERLAY
	ctx->video_overlay = AV_NULL;
	#else
	ctx->video_surface = AV_NULL;
	#endif

	memcpy(&status->config, &config, sizeof(config));

	av_init_packet(&status->flush_pkt);
	status->flush_pkt.data= global_flush_pkt.data;

	if (AV_OK != prepare_player(status, url))
	{
		goto const_err;
	}

	if(ctx->has_video)
	{
	    /* prepare video */
		#if WITH_OVERLAY
		if (ctx->status->config.alpha_enabled)
		{
			if (AV_OK != ctx->video->create_overlay_buffered(ctx->video, &ctx->video_overlay))
			{
				goto const_err;
			}
		}
		else
		{
			if (AV_OK != ctx->video->create_overlay(ctx->video, &ctx->video_overlay))
			{
				goto const_err;
			}
		}
		#else
		ctx->scale_info.dst_format  = AV_VIDEO_FORMAT_RGB32;
		ctx->scale_info.src_format  = ctx->video_info.format;
		ctx->scale_info.src_width   = ctx->video_info.width;
		ctx->scale_info.src_height  = ctx->video_info.height;
		ctx->scale_info.dst_width   = ctx->video_info.width;
		ctx->scale_info.dst_height  = ctx->video_info.height;
		ctx->scaler->set_configuration(ctx->scaler, &ctx->scale_info);
		if (AV_OK != ctx->video->create_surface(ctx->video, ctx->video_info.width, ctx->video_info.height,
													&ctx->video_surface))
		{
			goto const_err;
		}
		#endif
	}

	if(ctx->has_audio)
	{
	    /* prepare audio output */
    	if (AV_OK != ctx->audio->open(ctx->audio, ctx->audio_info.freq, ctx->audio_info.nchannels, ctx->audio_info.format, &ctx->audio_handle))
		{
			ctx->audio_handle = AV_NULL;
			goto const_err;
		}
		ctx->audio->set_user_callback(ctx->audio, ctx->audio_handle, write_audio_cb, status);
	}
	return AV_OK;

const_err:
	free(ctx->status);
	ctx->status = AV_NULL;
	return AV_EINIT;
}

static void av_player_ffplay_set_borders(av_player_p self, av_bool_t val)
{
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(self);
	if (ctx)
		ctx->draw_borders = val;
}

/* Destructor */
static void av_player_ffplay_destructor(void* pobject)
{
	av_player_ffplay_ctx_p ctx = (av_player_ffplay_ctx_p)O_context(pobject);
	av_player_p self = (av_player_p)pobject;
	self->stop(self);
	#if WITH_OVERLAY
	#else
	O_destroy(ctx->scaler);
	#endif
	free(ctx);
	av_torb_service_release("system");
}

/* Constructor */
static av_result_t av_player_ffplay_constructor(av_object_p object)
{
	av_result_t rc;
	av_player_ffplay_ctx_p ctx;
	av_player_p self = (av_player_p)object;

	/* register all codecs, demux and protocols */
	av_register_all();
	av_init_packet(&global_flush_pkt);
	global_flush_pkt.data = (unsigned char*)&"FLUSH";

	/* initializes self */
	ctx = malloc(sizeof(av_player_ffplay_ctx_t));
	if (!ctx)
		return AV_EMEM;
	memset(ctx, 0, sizeof(av_player_ffplay_ctx_t));

	ctx->draw_borders = AV_TRUE;

	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
		goto const_err_0;

	load_prefs();

	if (AV_OK != (rc = sys->get_timer(sys, &ctx->timer)))
		goto const_err_1;

	if (AV_OK != (rc = sys->get_input(sys, &ctx->input)))
		goto const_err_1;

	if (AV_OK != (rc = sys->get_audio(sys, &ctx->audio)))
		goto const_err_1;

	if (AV_OK != (rc = ctx->audio->enable(ctx->audio)))
		goto const_err_1;

	if (AV_OK != (rc = sys->get_video(sys, &ctx->video)))
		goto const_err_1;
	#if WITH_OVERLAY
	switch(ctx->video_overlay_format = config.video_overlay_format)
	{
		case AV_IYUV_OVERLAY: ctx->frame_to_overlay = frame_to_overlay_IYUV; break;
		case AV_YV12_OVERLAY: default : ctx->frame_to_overlay = frame_to_overlay_YV12; break;
	}
	#else
	if (AV_OK != (rc = av_torb_create_object(config.scaler_driver, (av_object_p*)&ctx->scaler)))
		goto const_err_1;
	#endif

	O_set_attr(self, CONTEXT, ctx);
	self->open         = av_player_ffplay_open;
	self->play         = av_player_ffplay_play;
	self->stop         = av_player_ffplay_stop;
	self->pause        = av_player_ffplay_pause;
	self->set_window   = av_player_ffplay_set_window;
	self->seek         = av_player_ffplay_seek;
	self->seek_rel     = av_player_ffplay_seek_rel;
	self->duration     = av_player_ffplay_duration;
	self->position     = av_player_ffplay_position;
	self->is_playing   = av_player_ffplay_is_playing;
	self->is_pause     = av_player_ffplay_is_pause;
	self->set_borders  = av_player_ffplay_set_borders;
	return AV_OK;

/* Cleanup */
const_err_1: av_torb_service_release("system");
const_err_0: free(ctx);
	return rc;
}

AV_API av_result_t av_player_ffplay_register_torba(void);
av_result_t av_player_ffplay_register_torba(void)
{
	#if WITH_OVERLAY
	#else
	av_scaler_register_torba();
	av_scaler_swscale_register_torba();
	#endif
	return av_torb_register_class("player", AV_NULL, sizeof(av_player_t), av_player_ffplay_constructor, av_player_ffplay_destructor);
}

#endif /* WITH_FFMPEG */
