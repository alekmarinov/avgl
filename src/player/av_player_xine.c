/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_player_xine.c                                   */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_XINE

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <math.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WITH_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#endif

#define XINE_ENGINE_INTERNAL

#include <xine.h>
#include <xine/xineutils.h>
#include <xine/xine_internal.h>

#include <av_log.h>
#include <av_timer.h>
#include <av_system.h>
#include <av_prefs.h>
#include <av_audio.h>
#include <av_player.h>
#include <av_graphics.h>
#include <av_media.h>
#include <av_thread.h>

#define USE_EVENT 0

#define CONTEXT "player_xine_ctx"
#define O_context(o) O_attr(o, CONTEXT)

#define CONTEXTOWNER "player_owner_xine_ctx"
#define O_player(o) O_attr(o, CONTEXTOWNER)

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

enum
{
	EVENT_ALLOC_ID   = 1,
	EVENT_REFRESH_ID = 2
};

struct av_player_xine_ctx;

typedef void (*frame_to_overlay_func_p)(av_video_overlay_p overlay, void *data0, void *data1, void *data2, int w, int h);

typedef struct av_player_xine_config
{
	const char* ppsz_ao;
	const char* ppsz_vo;
	const char* xine_config;
	int alpha_enabled;
	av_video_overlay_format_t video_overlay_format;
} av_player_xine_config_t, *av_player_xine_config_p;


typedef struct xine_ctl_t
{
	struct av_player_xine_ctx* ctx;

	xine_t*             xine;
	xine_video_port_t*  video_port;
	xine_audio_port_t*  audio_port;
	xine_event_queue_t* event_queue;

	xine_stream_t*      stream;
	xine_stream_t*      stream_sub;
	int                 offset_sub;
	char                stream_sub_filename[MAX_PATH];

	int                 num_ovl;
	raw_overlay_t*      overlays_array;

	raw_visual_t        rawvis;
	x11_visual_t        x11vis;

	double              pixel_aspect;
	int                 x, y, w, h;

} xine_ctl_t, *xine_ctl_p;

enum
{
	state_idle = 0,
	state_opening,
	state_playing,
	state_stopping
};

typedef struct av_player_xine_ctx
{
	av_player_p self;
	av_input_p input;
	av_video_p video;
	av_audio_p audio;
	av_timer_p timer;
	av_window_p owner;
	av_bool_t has_video;
	av_bool_t (*on_update)(av_window_p self, av_video_surface_p dstsurface, av_rect_p rect);

	Display* display;
	Window parent;
	Window win;
	int screen;

	xine_ctl_t xine_ctl;
	av_player_xine_config_t config;

	av_bool_t is_raw;
	av_bool_t draw_borders;
	av_video_overlay_p video_overlay;
	frame_to_overlay_func_p frame_to_overlay;
	double aspect_ratio;

	int state;
    av_player_state_t user_state;
    av_thread_p thd_opener;
	av_bool_t thd_opener_created;
	char* url;

} av_player_xine_ctx_t, *av_player_xine_ctx_p;


static av_system_p sys;
static av_player_xine_config_t defconfig =
{
	"alsa",
	"raw",
	"/etc/avgl.xine.prefs",
	1,
	-1
};

static const char* subext[] = { ".sub", ".srt", ".asc", ".smi", ".ssa", ".txt" };
static const int subext_num = sizeof(subext)/sizeof(subext[0]);

static av_bool_t av_player_verbose = AV_FALSE;

static void load_prefs(void)
{
	av_prefs_p prefs;
	if (AV_OK == av_torb_service_addref("prefs", (av_service_p*)&prefs))
	{
		const char* val = AV_NULL;
		prefs->get_string(prefs, "player.xine.vo", "raw", &defconfig.ppsz_vo);
		prefs->get_string(prefs, "player.xine.ao", "alsa", &defconfig.ppsz_ao);
		prefs->get_string(prefs, "player.xine.config", "/usr/local/etc/avgl.xine.prefs", &defconfig.xine_config);
		prefs->get_int(prefs, "player.xine.alphaenabled", 1, &defconfig.alpha_enabled);
		prefs->get_string(prefs, "log.console.verbosity", "error", &val);
		av_player_verbose = (!strcasecmp("debug",val))? AV_TRUE : AV_FALSE;
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

static void frame_to_overlay_YV12(av_video_overlay_p overlay, void *data0, void *data1, void *data2, int w, int h)
{
	int planes;
	const av_pix_p* pix;
	const short int* pitches;

	overlay->lock(overlay, &planes, &pix, &pitches);

	w *= h;
	memcpy(pix[0],data0,w); w /= 4;
	if (data2) memcpy(pix[1],data2,w);
	if (data1) memcpy(pix[2],data1,w);

	overlay->unlock(overlay);
}

static void frame_to_overlay_IYUV(av_video_overlay_p overlay, void *data0, void *data1, void *data2, int w, int h)
{
	int planes;
	const av_pix_p* pix;
	const short int* pitches;

	overlay->lock(overlay, &planes, &pix, &pitches);

	memcpy(pix[0],data0,2 * w * h);

	overlay->unlock(overlay);
	AV_UNUSED(data1);
	AV_UNUSED(data2);
}

static void video_borders_display(av_player_xine_ctx_p ctx, av_rect_p prect)
{
	av_video_surface_p surface;
	if (ctx->has_video && AV_OK == ctx->video->get_backbuffer(ctx->video, &surface))
	{
		av_rect_t brect, mrect;
		double aspect_ratio = ctx->aspect_ratio;
		double window_aspect_ratio = ((double)prect->w)/prect->h;

		av_rect_copy(&mrect, prect);
		if(window_aspect_ratio < aspect_ratio)
		{
			mrect.h = mrect.w;

			if (1.<=aspect_ratio)
				mrect.h /= aspect_ratio;
			else
				mrect.h *= aspect_ratio;

			av_rect_copy(&brect, prect);
			brect.h -= mrect.h;
			brect.h /= 2;
			mrect.y += brect.h;
			if (ctx->draw_borders)
			{
				surface->fill_rect_rgba(surface, &brect, 0xFF000000);
				brect.y = mrect.y + mrect.h;
				surface->fill_rect_rgba(surface, &brect, 0xFF000000);
			}
		}
		else
		{
			mrect.w = mrect.h;

			if (1.<=aspect_ratio)
				mrect.w *= aspect_ratio;
			else
				mrect.w /= aspect_ratio;

			av_rect_copy(&brect, prect);
			brect.w -= mrect.w;
			brect.w /= 2;
			mrect.x += brect.w;
			if (ctx->draw_borders)
			{
				surface->fill_rect_rgba(surface, &brect, 0xFF000000);
				brect.x = mrect.x + mrect.w;
				surface->fill_rect_rgba(surface, &brect, 0xFF000000);
			}
		}
		surface->fill_rect_rgba(surface, &mrect, 0xFF000000);

		/* update dimensions */
		av_rect_copy(prect, &mrect);
	}
}

static av_bool_t url_look_like_file(const char *url)
{
	if(url && strlen(url))
	{
		if((strncasecmp(url, "file:", 5)) && strstr (url, ":/") && (strstr (url, ":/") < strchr(url, '/')))
			return AV_FALSE;
	}
	return AV_TRUE;
}

static av_bool_t av_player_xine_open_subtitles(av_player_xine_ctx_p ctx, const char* url)
{
	char *p;
	if (url_look_like_file(url))
	{
		strncpy(ctx->xine_ctl.stream_sub_filename, url, MAX_PATH);
		ctx->xine_ctl.stream_sub_filename[MAX_PATH-1] = 0;
		p = strrchr(ctx->xine_ctl.stream_sub_filename, '.');
		if (p)
		{
			char *fname = ctx->xine_ctl.stream_sub_filename;
			int len_path = p - fname;
			int max_path = MAX_PATH - len_path;
			int i;

			if (!strncasecmp(fname, "file:", 5))
				fname += 5;

			for (i=0; i<subext_num; i++)
			{
				struct stat pstat;
				strncpy(p,subext[i],max_path);
				ctx->xine_ctl.stream_sub_filename[MAX_PATH-1] = 0;
				if (((stat(fname, &pstat)) > -1) && (S_ISREG(pstat.st_mode)) && strcmp(fname, url))
				{
					return AV_TRUE;
				}
			}
		}
	}

	return AV_FALSE;
}

/** Host interface callbacks */
static void av_player_xine_raw_output_cb(void *user_data, int frame_format, int frame_width, int frame_height,
											double frame_aspect, void *data0, void *data1, void *data2)
{
	xine_ctl_t *this = (xine_ctl_t *)user_data;
	av_player_xine_ctx_p ctx = this->ctx;

	ctx->aspect_ratio = frame_aspect;

	if (ctx->video_overlay)
	{
		av_video_overlay_format_t old_format = ctx->config.video_overlay_format;

		switch(frame_format)
		{
			case XINE_VORAW_YUY2:
				ctx->config.video_overlay_format = AV_YUY2_OVERLAY;
				ctx->frame_to_overlay = frame_to_overlay_IYUV;
				break;
			case XINE_VORAW_YV12:
			default:
				ctx->config.video_overlay_format = AV_YV12_OVERLAY,
				ctx->frame_to_overlay = frame_to_overlay_YV12;
				break;
		}

		if (old_format != ctx->config.video_overlay_format ||
			!ctx->video_overlay->validate_size(ctx->video_overlay, frame_width, frame_height))
		{
			ctx->video_overlay->set_size_format(ctx->video_overlay, frame_width, frame_height, ctx->config.video_overlay_format);
		}
		else
		if (ctx->video_overlay->validate(ctx->video_overlay))
		{
			ctx->frame_to_overlay(ctx->video_overlay, data0, data1, data2, frame_width, frame_height);

			/* patch the spu chanells
			int i;
			for (i=0; i < this->num_ovl; i++)
			{
				av_rect_t srcrect;
				srcrect.x = this->overlays_array[i].ovl_x;
				srcrect.y = this->overlays_array[i].ovl_y;
				srcrect.w = this->overlays_array[i].ovl_w;
				srcrect.h = this->overlays_array[i].ovl_h;
				ctx->video_overlay->blit_back_to(ctx->video_overlay, &srcrect, (void*)this->overlays_array[i].ovl_rgba, &srcrect);
			}*/

			ctx->video_overlay->blit(ctx->video_overlay, AV_NULL);

			if (ctx->config.alpha_enabled)
			{
				av_event_t event;
				memset(&event, 0, sizeof(av_event_t));
				event.type = AV_EVENT_USER;
				event.user_id = EVENT_REFRESH_ID;
				event.window = ctx->owner;
				ctx->input->push_event(ctx->input, &event);
			}
			else
			{
				/* NOP */
			}
		}
	}
}

static void av_player_xine_raw_overlay_cb(void *user_data, int num_ovl, raw_overlay_t *overlays_array)
{
	xine_ctl_t *this = (xine_ctl_t *)user_data;
	this->num_ovl = num_ovl;
	this->overlays_array = (0<num_ovl)? overlays_array : AV_NULL;
}

static void av_player_xine_dest_size_cb(void *data, int video_width, int video_height, double video_pixel_aspect,
										int *dest_width, int *dest_height, double *dest_pixel_aspect)
{
	xine_ctl_t *this = (xine_ctl_t *)data;

	if (this)
	{
		*dest_width = this->w;
		*dest_height = this->h;
		*dest_pixel_aspect = this->pixel_aspect;
	}
	AV_UNUSED(video_width);
	AV_UNUSED(video_height);
	AV_UNUSED(video_pixel_aspect);
}

static void av_player_xine_frame_output_cb(void *data, int video_width, int video_height, double video_pixel_aspect,
											int *dest_x, int *dest_y,  int *dest_width, int *dest_height,
											double *dest_pixel_aspect, int *win_x, int *win_y)
{
	xine_ctl_t *this = (xine_ctl_t *)data;
	if (this)
	{
		*win_x = this->x;
		*win_y = this->y;
		*dest_x = 0;
		*dest_y = 0;
		*dest_width  = this->w;
		*dest_height = this->h;
		*dest_pixel_aspect = this->pixel_aspect;
	}
	AV_UNUSED(video_width);
	AV_UNUSED(video_height);
	AV_UNUSED(video_pixel_aspect);
}

static void av_player_xine_event_cb(void *user_data, const xine_event_t *event)
{
	xine_ctl_t *this = (xine_ctl_t *)user_data;
	if (this->ctx)
	{
		/* av_player_xine_ctx_p ctx = this->ctx; */
		switch (event->type)
		{
			case XINE_EVENT_UI_PLAYBACK_FINISHED:
				xine_stop(this->stream);
                this->ctx->user_state.state_ext.state = avps_finished;
                break;
                
            case XINE_EVENT_PROGRESS:
                if(event->data)
                {
                    xine_progress_data_t *data = event->data;
                    this->ctx->user_state.state_ext.state = avps_buffering;
                    this->ctx->user_state.state_ext.progress = data->percent;
                }
                break;
                
            default:
				break;
		}
	}
}

/* Host interface */
static av_result_t av_player_xine_create_player(av_player_xine_ctx_p ctx)
{
	/** init xine engine */
	ctx->xine_ctl.stream = NULL;
	ctx->xine_ctl.stream_sub = NULL;
	ctx->xine_ctl.video_port = NULL;
	ctx->xine_ctl.audio_port = NULL;

	/** create a new xine */
	ctx->xine_ctl.xine = xine_new();
	xine_config_load(ctx->xine_ctl.xine, ctx->config.xine_config);
	xine_init(ctx->xine_ctl.xine);

	ctx->is_raw = (!strcasecmp("raw",ctx->config.ppsz_vo))? AV_TRUE : AV_FALSE;

	if (ctx->is_raw)
	{
		/** create video port */
		/* OR'ed frame_format
		* a frontend must at least support rgb
		* a frontend supporting yuv must support both yv12 and yuy2
		* then possible combinations are:
		*   XINE_VORAW_RGB                 ( rgb )
		*   XINE_VORAW_YV12|XINE_VORAW_YUY2|XINE_VORAW_RGB   ( yv12, yuy2 and rgb )
		*
		* Be aware that rgb requires more cpu than yuv,
		* so avoid its usage for video playback.
		* However, it's usefull for single frame capture (e.g. thumbs)
		*/
		ctx->xine_ctl.rawvis.supported_formats = XINE_VORAW_YV12|XINE_VORAW_YUY2;

		if (ctx->config.alpha_enabled)
		{
			if (AV_OK != ctx->video->create_overlay_buffered(ctx->video, &ctx->video_overlay))
				return AV_EGENERAL;
		}
		else
		{
			if (AV_OK != ctx->video->create_overlay(ctx->video, &ctx->video_overlay))
				return AV_EGENERAL;
		}


		/* raw output callback
		* this will be called by the video driver for every frame
		*
		* If frame_format==XINE_VORAW_YV12, data0 points to frame_width*frame_height Y values
		*                                   data1 points to (frame_width/2)*(frame_height/2) U values
		*                                   data2 points to (frame_width/2)*(frame_height/2) V values
		*
		* If frame_format==XINE_VORAW_YUY2, data0 points to frame_width*frame_height*2 YU/Y²V values
		*                                   data1 is NULL
		*                                   data2 is NULL
		*
		* If frame_format==XINE_VORAW_RGB, data0 points to frame_width*frame_height*3 RGB values
		*                                  data1 is NULL
		*                                  data2 is NULL
		*/
		ctx->xine_ctl.rawvis.raw_output_cb     = av_player_xine_raw_output_cb;

		/* raw overlay callback
		* this will be called by the video driver for every new overlay state
		* overlays_array points to an array of num_ovl raw_overlay_t
		* Note that num_ovl can be 0, meaning "end of overlay display"
		* num_ovl is at most XINE_VORAW_MAX_OVL */
		ctx->xine_ctl.rawvis.raw_overlay_cb    = av_player_xine_raw_overlay_cb;

		ctx->xine_ctl.rawvis.user_data        = (void*)&ctx->xine_ctl;

		ctx->xine_ctl.video_port = xine_open_video_driver(ctx->xine_ctl.xine, ctx->config.ppsz_vo, XINE_VISUAL_TYPE_RAW, (void *) &(ctx->xine_ctl.rawvis));

		/** create audio port */
		ctx->xine_ctl.audio_port = xine_open_audio_driver (ctx->xine_ctl.xine, ctx->config.ppsz_ao, NULL);

		/** create stream */
		ctx->xine_ctl.stream = xine_stream_new(ctx->xine_ctl.xine, ctx->xine_ctl.audio_port, ctx->xine_ctl.video_port);

		/** create subtitle stream */
		ctx->xine_ctl.stream_sub = xine_stream_new(ctx->xine_ctl.xine, NULL, ctx->xine_ctl.video_port);
		xine_set_param(ctx->xine_ctl.stream_sub, XINE_PARAM_AUDIO_REPORT_LEVEL, 0);

		/** xine debugging */
		xine_set_param(ctx->xine_ctl.stream, XINE_PARAM_VERBOSITY,
			(av_player_verbose)? (XINE_VERBOSITY_DEBUG|XINE_VERBOSITY_LOG):XINE_VERBOSITY_NONE);

		/** event handling */
		ctx->xine_ctl.event_queue = xine_event_new_queue(ctx->xine_ctl.stream);
		xine_event_create_listener_thread(ctx->xine_ctl.event_queue, av_player_xine_event_cb, (void*)&ctx->xine_ctl);
	}
	else
	{
		double res_h, res_v, aspect_diff;
		XSetWindowAttributes win_attrib;
		unsigned long win_mask;

		if (AV_OK != ctx->video->get_root_window(ctx->video, (void**)&ctx->display, (void**)&ctx->parent))
			return AV_EGENERAL;

		XLockDisplay(ctx->display);

		win_attrib.border_pixel = 0x00FFFFFFU;
		win_attrib.background_pixel = 0x00000000U;
		win_attrib.override_redirect = 0;
		win_mask = CWBackPixel | CWBorderPixel | CWOverrideRedirect;

		ctx->xine_ctl.x = 0;
		ctx->xine_ctl.y = 0;
		ctx->xine_ctl.w = 1;
		ctx->xine_ctl.h = 1;

		ctx->screen = DefaultScreen(ctx->display);
		ctx->win = XCreateWindow(ctx->display, ctx->parent,
			ctx->xine_ctl.x, ctx->xine_ctl.y, ctx->xine_ctl.w, ctx->xine_ctl.h,
			0, 0, InputOutput, CopyFromParent, win_mask, &win_attrib);

		if (None == ctx->win)
		{
			XSync(ctx->display, False);
			XUnlockDisplay(ctx->display);
			return AV_EGENERAL;
		}

		XSelectInput(ctx->display, ctx->win, 0);

		/* determine display aspect ratio */
		res_h = (DisplayWidth(ctx->display, ctx->screen)*1000 / DisplayWidthMM(ctx->display, ctx->screen));
		res_v = (DisplayHeight(ctx->display, ctx->screen)*1000 / DisplayHeightMM(ctx->display, ctx->screen));
		ctx->xine_ctl.pixel_aspect = res_v / res_h;
		aspect_diff = ctx->xine_ctl.pixel_aspect - 1.0;
		if ((aspect_diff < 0.01) && (aspect_diff > -0.01))
			ctx->xine_ctl.pixel_aspect = 1.0;

		XSync(ctx->display, False);
		XUnlockDisplay(ctx->display);

		/** create video port */
		ctx->xine_ctl.x11vis.display          = ctx->display;
		ctx->xine_ctl.x11vis.screen           = ctx->screen;
		ctx->xine_ctl.x11vis.d                = ctx->win;
		ctx->xine_ctl.x11vis.frame_output_cb  = av_player_xine_frame_output_cb;
		ctx->xine_ctl.x11vis.dest_size_cb     = av_player_xine_dest_size_cb;
		ctx->xine_ctl.x11vis.user_data        = (void*)&ctx->xine_ctl;

		if (AV_NULL == (ctx->xine_ctl.video_port = xine_open_video_driver(ctx->xine_ctl.xine,
			ctx->config.ppsz_vo, XINE_VISUAL_TYPE_X11, (void *) &(ctx->xine_ctl.x11vis))))
			return AV_EGENERAL;

		/** create audio port */
		if (AV_NULL == (ctx->xine_ctl.audio_port = xine_open_audio_driver (ctx->xine_ctl.xine,
			ctx->config.ppsz_ao, NULL)))
			return AV_EGENERAL;

		/** create stream */
		ctx->xine_ctl.stream = xine_stream_new(ctx->xine_ctl.xine, ctx->xine_ctl.audio_port, ctx->xine_ctl.video_port);

		/** create subtitle stream */
		ctx->xine_ctl.stream_sub = xine_stream_new(ctx->xine_ctl.xine, NULL, ctx->xine_ctl.video_port);
		xine_set_param(ctx->xine_ctl.stream_sub, XINE_PARAM_AUDIO_REPORT_LEVEL, 0);

		/** xine debugging */
		xine_set_param(ctx->xine_ctl.stream, XINE_PARAM_VERBOSITY,
			(av_player_verbose)? (XINE_VERBOSITY_DEBUG|XINE_VERBOSITY_LOG):XINE_VERBOSITY_NONE);

		/** event handling */
		ctx->xine_ctl.event_queue = xine_event_new_queue(ctx->xine_ctl.stream);
		xine_event_create_listener_thread(ctx->xine_ctl.event_queue, av_player_xine_event_cb, (void*)&ctx->xine_ctl);

		xine_port_send_gui_data(ctx->xine_ctl.video_port, XINE_GUI_SEND_DRAWABLE_CHANGED, (void *)ctx->win);
		xine_port_send_gui_data(ctx->xine_ctl.video_port, XINE_GUI_SEND_VIDEOWIN_VISIBLE, (void *)1);
	}


	return AV_OK;
}

static av_result_t av_player_xine_destroy_player(av_player_p self)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);
	if (ctx)
	{
		self->stop(self);

		if (ctx->xine_ctl.stream_sub)
			xine_close(ctx->xine_ctl.stream_sub);

		if (ctx->xine_ctl.stream)
			xine_close(ctx->xine_ctl.stream);

		xine_event_dispose_queue(ctx->xine_ctl.event_queue);
		xine_dispose(ctx->xine_ctl.stream);

		if (ctx->xine_ctl.audio_port)
			xine_close_audio_driver(ctx->xine_ctl.xine, ctx->xine_ctl.audio_port);
		if (ctx->xine_ctl.video_port)
			xine_close_video_driver(ctx->xine_ctl.xine, ctx->xine_ctl.video_port);

		xine_exit(ctx->xine_ctl.xine);

		if (ctx->is_raw)
		{
			if (ctx->video_overlay)
			{
				O_destroy(ctx->video_overlay);
				ctx->video_overlay = AV_NULL;
			}
		}
		else
		{
			if (None != ctx->win)
			{
				XLockDisplay(ctx->display);
				XDestroyWindow(ctx->display, ctx->win);
				XUnlockDisplay(ctx->display);
				ctx->win = None;
			}
		}
		return AV_OK;
	}
	return AV_EGENERAL;
}

static av_result_t av_player_xine_show_window(av_player_xine_ctx_p ctx)
{
	if (ctx)
	{
		if (ctx->has_video)
		{
			ctx->owner->update(ctx->owner, AV_UPDATE_INVALIDATE);

			if (ctx->is_raw)
			{
				/* NOP */
			}
			else
			{
				XLockDisplay(ctx->display);
				XMapWindow(ctx->display, ctx->win);
				XSync(ctx->display, False);
				XUnlockDisplay(ctx->display);
			}
		}
		return AV_OK;
	}
	return AV_EGENERAL;
}

static av_result_t av_player_xine_hide_window(av_player_xine_ctx_p ctx)
{
	if (ctx)
	{
		ctx->xine_ctl.x = 0;
		ctx->xine_ctl.y = 0;
		ctx->xine_ctl.w = 1;
		ctx->xine_ctl.h = 1;

		if (ctx->is_raw)
		{
			/* NOP */
		}
		else
		{
			XLockDisplay(ctx->display);
			XUnmapWindow(ctx->display, ctx->win);
			XSync(ctx->display, False);
			XUnlockDisplay(ctx->display);
		}
		return AV_OK;
	}
	return AV_EGENERAL;
}

static av_result_t av_player_xine_set_window_pos_size(av_player_xine_ctx_p ctx, av_rect_p rect)
{
	if (ctx)
	{
		if (ctx->xine_ctl.x != rect->x || ctx->xine_ctl.y != rect->y ||
			ctx->xine_ctl.w != rect->w || ctx->xine_ctl.h != rect->h)
		{
			ctx->xine_ctl.x = rect->x;
			ctx->xine_ctl.y = rect->y;
			ctx->xine_ctl.w = rect->w;
			ctx->xine_ctl.h = rect->h;

			if (ctx->is_raw)
			{
				if (ctx->config.alpha_enabled)
				{
					if (ctx->video_overlay->validate(ctx->video_overlay))
						ctx->video_overlay->set_size_back(ctx->video_overlay, ctx->xine_ctl.w, ctx->xine_ctl.h);
				}
				else
					ctx->video_overlay->set_size_back(ctx->video_overlay, ctx->xine_ctl.w, ctx->xine_ctl.h);
			}
			else
			{
				XLockDisplay(ctx->display);
				XMoveResizeWindow(ctx->display, ctx->win, rect->x, rect->y, rect->w, rect->h);
				XUnlockDisplay(ctx->display);
			}
		}
		return AV_OK;
	}
	return AV_EGENERAL;
}

/* Control interface */
static av_result_t av_player_xine_play(av_player_p self)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);
	if (ctx)
	{
		while(ctx->state == state_opening)
			xine_usec_sleep(10000);

		if(ctx->state != state_playing)
			return AV_EGENERAL;

		ctx->has_video = (xine_get_stream_info(ctx->xine_ctl.stream, XINE_STREAM_INFO_HAS_VIDEO))? AV_TRUE : AV_FALSE;
		av_player_xine_show_window(ctx);
	}
	return AV_OK;
}

static av_result_t av_player_xine_pause(av_player_p self)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);
	if (ctx)
	{
		int ispause = xine_get_param(ctx->xine_ctl.stream, XINE_PARAM_SPEED);
		xine_set_param(ctx->xine_ctl.stream, XINE_PARAM_SPEED, (ispause==XINE_SPEED_PAUSE)?XINE_SPEED_NORMAL:XINE_SPEED_PAUSE);
	}
	return AV_OK;
}

static av_result_t av_player_xine_seek(av_player_p self, double pos_seconds)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);
	AV_UNUSED(pos_seconds);
	if (ctx)
	{
		xine_play(ctx->xine_ctl.stream, 0, (int)1000*pos_seconds);
	}
	else
		return AV_EARG;
	return AV_OK;
}

static av_result_t av_player_xine_seek_rel(struct av_player* self, double pos_seconds)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);
	AV_UNUSED(pos_seconds);
	if (ctx)
	{
		int pos_stream;  /* 0..65535     */
		int pos_time;    /* milliseconds */
		int length_time; /* milliseconds */
		if (xine_get_pos_length(ctx->xine_ctl.stream, &pos_stream, &pos_time, &length_time))
		{
			xine_play(ctx->xine_ctl.stream, 0, pos_time+((int)1000*pos_seconds));
		}
	}
	else
		return AV_EARG;
	return AV_OK;
}

static double av_player_xine_duration(struct av_player* self)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);
	if (ctx)
	{
		int pos_stream;  /* 0..65535     */
		int pos_time;    /* milliseconds */
		int length_time; /* milliseconds */
		if (xine_get_pos_length(ctx->xine_ctl.stream, &pos_stream, &pos_time, &length_time))
		{
			return (double)length_time/1000.;
		}
	}
	return 0.;
}

static double av_player_xine_position(struct av_player* self)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);
	if (ctx)
	{
		int pos_stream;  /* 0..65535     */
		int pos_time;    /* milliseconds */
		int length_time; /* milliseconds */
		if (xine_get_pos_length(ctx->xine_ctl.stream, &pos_stream, &pos_time, &length_time))
		{
			return (double)pos_time/1000.;
		}
    }
	return 0.;
}
static av_bool_t av_player_on_update(av_window_p self, av_video_surface_p dstsurface, av_rect_p rect)
{
	av_rect_t windowrect;
	av_rect_p winrect = &windowrect;
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);
	self->get_rect(self, winrect);
	winrect->x = winrect->y = 0;
	self->rect_absolute(self, winrect);

	if (ctx)
	{
		if (ctx->is_raw)
		{
			if (ctx->has_video)
			{
				if (ctx->video_overlay)
				{
					video_borders_display(ctx, winrect);
					av_player_xine_set_window_pos_size(ctx, winrect);
					ctx->video_overlay->blit_back(ctx->video_overlay, AV_NULL, winrect);
				}
				else
				{
					/* Black rectangle */
					dstsurface->fill_rect(dstsurface, winrect, 0., 0., 0., 1.);
				}
			}
		}
		else
		if (ctx->win != None)
		{
			av_player_xine_set_window_pos_size(ctx, winrect);
		}
		else
		{
			/* Black rectangle */
			dstsurface->fill_rect(dstsurface, winrect, 0., 0., 0., 1.);
		}
	}
	else
	{
		/* Black rectangle */
		dstsurface->fill_rect(dstsurface, winrect, 0., 0., 0., 1.);
	}

	AV_UNUSED(rect);
	return AV_TRUE;
}

static av_bool_t av_player_on_user(av_window_p self, int id, void* data)
{
	av_player_p player = (av_player_p)O_player(self);
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(player);

	if (ctx)
	{
		if (ctx->config.alpha_enabled)
		{
			switch(id)
			{
				case EVENT_REFRESH_ID:
					self->update(self, AV_UPDATE_INVALIDATE);
					break;
				case EVENT_ALLOC_ID:
					break;
			}
		}
		else
		{
			/* NOP */
		}
	}
	AV_UNUSED(data);
	return AV_TRUE;
}

static void av_player_xine_set_window(av_player_p self, av_window_p window)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);

	ctx->owner = window;
	ctx->owner->on_update = av_player_on_update;
	ctx->owner->on_user = av_player_on_user;
	O_set_attr(ctx->owner, CONTEXT, ctx);
	O_set_attr(window, CONTEXTOWNER, self);

	window->update(window, AV_UPDATE_INVALIDATE);
}

static av_result_t av_player_xine_stop(av_player_p self)
{
	av_result_t rc = AV_OK;
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);

	if(ctx)
	{
		xine_stop(ctx->xine_ctl.stream);

		/*! Signal demuxer to interrupt */
		ctx->xine_ctl.stream->demux_action_pending = 1;

		if (ctx->thd_opener_created)
		{
			int tout;
			ctx->state = state_stopping;

			for (tout=5; tout-->0 && ctx->state == state_stopping;)
				xine_usec_sleep(10000);

			if (ctx->state == state_stopping)
				ctx->thd_opener->interrupt(ctx->thd_opener);

			ctx->thd_opener->join(ctx->thd_opener);
			ctx->thd_opener->destroy(ctx->thd_opener);
			ctx->thd_opener_created = AV_FALSE;
		}

		ctx->has_video = AV_FALSE;
		av_player_xine_hide_window(ctx);

		ctx->xine_ctl.num_ovl = 0;
		ctx->xine_ctl.overlays_array = AV_NULL;
		ctx->config.video_overlay_format = -1;
		if (ctx->video_overlay)
		{
			ctx->video_overlay->set_size_format(ctx->video_overlay, 1, 1, ctx->config.video_overlay_format);
			ctx->video_overlay->set_size_back(ctx->video_overlay, 1, 1);
		}

		ctx->state = state_idle;
        ctx->user_state.state = avps_idle;
	}
	return rc;
}

static int av_player_xine_opener_thread(av_thread_p thread)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(thread->arg);
	char* url = ctx->url;

	if (ctx && url && *url)
	{
        ctx->user_state.state = avps_connecting;
		if (xine_open(ctx->xine_ctl.stream, url))
		{
			if (av_player_xine_open_subtitles(ctx,url))
			{
				if (xine_open(ctx->xine_ctl.stream_sub, ctx->xine_ctl.stream_sub_filename))
				{
					xine_stream_master_slave(ctx->xine_ctl.stream, ctx->xine_ctl.stream_sub,
											 XINE_MASTER_SLAVE_PLAY|XINE_MASTER_SLAVE_STOP);
				}
				xine_set_param(ctx->xine_ctl.stream, XINE_PARAM_SPU_OFFSET, ctx->xine_ctl.offset_sub=0);
			}
			if (xine_play(ctx->xine_ctl.stream, 0, 0))
            {
                ctx->user_state.state = avps_running;
            }
            else
            {
                ctx->user_state.state = avps_idle;
            }
		}
        else
        {
            ctx->user_state.state = avps_idle;
        }

		if (url)
			free(url);
		ctx->url = 0;

		ctx->state = state_playing;
	}

	while(ctx->state != state_stopping)
		xine_usec_sleep(10000);

	ctx->state = state_idle;
    ctx->user_state.state = avps_idle;

	return 0;
}

/* open media file */
static av_result_t av_player_xine_open(av_player_p self, const char* url)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);
	if (ctx)
	{
		av_result_t rc;
		
		if(state_idle != ctx->state)
			self->stop(self);

		ctx->url = strdup(url);
		if (!ctx->url)
			return AV_EMEM;

		/*! Clear demuxer interrupt */
		ctx->xine_ctl.stream->demux_action_pending = 0;

		ctx->state = state_opening;
		/* create opener thread */
		if (AV_OK != (rc = av_thread_create(av_player_xine_opener_thread, self, &ctx->thd_opener)))
		{
			ctx->state = state_idle;
			free(ctx->url);
			ctx->url = 0;
			return rc;
		}
		ctx->thd_opener->start(ctx->thd_opener);
		ctx->thd_opener_created = AV_TRUE;
	}
	return AV_OK;
}

static av_result_t av_player_xine_is_playing(struct av_player* self)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);

    av_result_t rc = ctx->user_state.state;
    int isplaying = xine_get_status (ctx->xine_ctl.stream);
	return (isplaying==XINE_STATUS_PLAY)?
            (rc | avps_playing) : (rc & ~avps_playing);
}

static av_result_t av_player_xine_is_pause(struct av_player* self)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);
	
    av_result_t rc = ctx->user_state.state;
    int ispause = xine_get_param(ctx->xine_ctl.stream, XINE_PARAM_SPEED);
	return (ispause==XINE_SPEED_PAUSE)? 
            (rc | avps_paused) : (rc & ~avps_paused);
}

static void av_player_xine_set_borders(av_player_p self, av_bool_t val)
{
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);
	if (ctx)
		ctx->draw_borders = val;
	return;
}

/* Destructor */
static void av_player_xine_destructor(void* pobject)
{
	av_player_p self = (av_player_p)pobject;
	av_player_xine_ctx_p ctx = (av_player_xine_ctx_p)O_context(self);
	if (ctx)
	{
        self->stop(self);
		av_player_xine_destroy_player(self);
		free(ctx);
	}
	av_torb_service_release("system");
}

/* Constructor */
static av_result_t av_player_xine_constructor(av_object_p object)
{
	av_result_t rc;
	av_player_xine_ctx_p ctx;
	av_player_p self = (av_player_p)object;

	/* initializes self */
	ctx = malloc(sizeof(av_player_xine_ctx_t));
	if (!ctx)
		return AV_EMEM;
	memset(ctx, 0, sizeof(av_player_xine_ctx_t));

	ctx->self = self;
	ctx->xine_ctl.ctx = ctx;
	ctx->has_video = AV_FALSE;
	ctx->draw_borders = AV_TRUE;
	ctx->thd_opener_created = AV_FALSE;
	ctx->state = state_idle;
    ctx->user_state.state = avps_idle;
	ctx->url = 0;

	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
		goto const_err_0;

	load_prefs();
	memcpy(&ctx->config, &defconfig, sizeof(av_player_xine_config_t));

	if (AV_OK != (rc = sys->get_timer(sys, &ctx->timer)))
		goto const_err_1;

	if (AV_OK != (rc = sys->get_input(sys, &ctx->input)))
		goto const_err_1;

	if (AV_OK != (rc = sys->get_audio(sys, &ctx->audio)))
		goto const_err_1;

	if (AV_OK != (rc = sys->get_video(sys, &ctx->video)))
		goto const_err_1;

	ctx->video_overlay = AV_NULL;
	ctx->display = AV_NULL;
	ctx->parent = None;
	ctx->win = None;

	if (AV_OK != (rc = av_player_xine_create_player(ctx)))
		goto const_err_1;

	O_set_attr(self, CONTEXT, ctx);
	self->open         = av_player_xine_open;
	self->play         = av_player_xine_play;
	self->stop         = av_player_xine_stop;
	self->pause        = av_player_xine_pause;
	self->set_window   = av_player_xine_set_window;
	self->seek         = av_player_xine_seek;
	self->seek_rel     = av_player_xine_seek_rel;
	self->duration     = av_player_xine_duration;
	self->position     = av_player_xine_position;
	self->is_playing   = av_player_xine_is_playing;
	self->is_pause     = av_player_xine_is_pause;
	self->set_borders  = av_player_xine_set_borders;
	return AV_OK;

/* Cleanup */
const_err_1: log_error("av_player_xine_constructor failed"); av_torb_service_release("system");
const_err_0: free(ctx);
	return rc;
}

AV_API av_result_t av_player_xine_register_torba(void);
av_result_t av_player_xine_register_torba(void)
{
	if (!XInitThreads())
		return AV_EGENERAL;

	return av_torb_register_class("player", AV_NULL, sizeof(av_player_t), av_player_xine_constructor, av_player_xine_destructor);
}

#endif /* WITH_XINE */

