/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_player_vlc.c                                    */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_VLC

#define _GNU_SOURCE
#include <math.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include <av_log.h>
#include <av_timer.h>
#include <av_system.h>
#include <av_prefs.h>
#include <av_audio.h>
#include <av_player.h>
#include <av_graphics.h>
#include <av_media.h>
#include <av_thread.h>

#include <vlc/vlc.h>

#define is_vlc(x) (AV_NULL!=(x))
#define CONTEXT "player_vlc_ctx"
#define O_context(o) O_attr(o, CONTEXT)

/*******************************/
/*  VLC interface defintions   */
/*******************************/
typedef libvlc_media_player_t *libvlc_media_player_p;
typedef libvlc_instance_t *libvlc_instance_p;
typedef libvlc_media_t *libvlc_media_p;

struct av_player_vlc_ctx;

typedef struct av_player_vlc_config
{
	int debug;
	const char* ppsz_aout;
	const char* ppsz_vout;
	const char* ppsz_sub_enc;
	const char* ppsz_sub_font;
	const char* ppsz_pluginpath;
	const char** ppsz_argv;
	int ppsz_argc;
} av_player_vlc_config_t, *av_player_vlc_config_p;

typedef struct av_player_vlc_ctx
{
	libvlc_media_player_p pl;
	libvlc_instance_p vlc;
	libvlc_exception_t ex;
	libvlc_media_p md;

	av_input_p input;
	av_video_p video;
	av_audio_p audio;
	av_timer_p timer;
	av_window_p owner;
	av_bool_t draw_borders;
	struct { int ppsz_argc; char** ppsz_argv; } config;
	av_bool_t (*on_update)(av_window_p self, av_video_surface_p dstsurface, av_rect_p rect);

	Display* display;
	Window parent;
	Window win;
	int screen;
	int x, y, w, h;
	av_player_state_t state;
} av_player_vlc_ctx_t, *av_player_vlc_ctx_p;


static av_system_p sys;
static const char* ppsz_argv[] = { "--no-interact", "--no-plugins-cache", "--no-osd" };
static av_player_vlc_config_t defconfig =
{
	.debug = 0,
	.ppsz_aout = AV_NULL,
	.ppsz_vout = AV_NULL,
	.ppsz_sub_enc = AV_NULL,
	.ppsz_sub_font = AV_NULL,
	.ppsz_pluginpath = AV_NULL,
	.ppsz_argv = ppsz_argv,
	.ppsz_argc = sizeof(ppsz_argv)/sizeof(ppsz_argv[0]),
};

static inline void log_error(const char* message)
{
	av_log_p logging;
	if (AV_OK == av_torb_service_addref("log", (av_service_p*)&logging))
	{
		logging->error(logging,message);
		av_torb_service_release("log");
	}
}

static inline void catch(av_player_vlc_ctx_p ctx)
{
	if (libvlc_exception_raised(&ctx->ex))
	{
		log_error(libvlc_exception_get_message(&ctx->ex)?:"VLC raise unknown exception");
	}
	libvlc_exception_clear(&ctx->ex);
}

static inline av_bool_t is_exception(av_player_vlc_ctx_p ctx)
{
	if (libvlc_exception_raised(&ctx->ex))
	{
		catch(ctx);
		return AV_TRUE;
	}
	return AV_FALSE;
}

static char* strformatintdup(const char* format, int val)
{
	char* p = AV_NULL;
	return (0 < asprintf(&p, format, val))? p : AV_NULL;
}

static char* strformatstrdup(const char* format, const char* val)
{
	char* p = AV_NULL;
	return (0 < asprintf(&p, format, val))? p : AV_NULL;
}

static void load_prefs(void)
{
	av_prefs_p prefs;
	if (AV_OK == av_torb_service_addref("prefs", (av_service_p*)&prefs))
	{
		prefs->get_int(prefs, "player.vlc.debug", 0,
					   &defconfig.debug);
		prefs->get_string(prefs, "player.vlc.aout", "alsa",
						  &defconfig.ppsz_vout);
		prefs->get_string(prefs, "player.vlc.vout", "xvideo",
						  &defconfig.ppsz_vout);
		prefs->get_string(prefs, "player.vlc.subenc", "Windows-1251",
						  &defconfig.ppsz_sub_enc);
		prefs->get_string(prefs, "player.vlc.subfont", "/usr/share/fonts/truetype/freefont/FreeSerifBold.ttf",
						  &defconfig.ppsz_sub_font);
		prefs->get_string(prefs, "player.vlc.pluginpath", "/usr/lib/vlc",
						  &defconfig.ppsz_pluginpath);
		av_torb_service_release("prefs");
	}
}

static av_result_t av_player_vlc_show_window(av_player_vlc_ctx_p ctx)
{
	if (ctx)
	{
		if (/* FIXME ctx->has_video */ AV_TRUE)
		{
			XLockDisplay(ctx->display);
			XMapWindow(ctx->display, ctx->win);
			XSync(ctx->display, False);
			XUnlockDisplay(ctx->display);
		}
		return AV_OK;
	}
	return AV_EGENERAL;
}

static av_result_t av_player_vlc_hide_window(av_player_vlc_ctx_p ctx)
{
	if (ctx)
	{
		ctx->x = 0;
		ctx->y = 0;
		ctx->w = 1;
		ctx->h = 1;

		XLockDisplay(ctx->display);
		XUnmapWindow(ctx->display, ctx->win);
		XSync(ctx->display, False);
		XUnlockDisplay(ctx->display);

		return AV_OK;
	}
	return AV_EGENERAL;
}

static av_result_t av_player_vlc_set_window_pos_size(av_player_vlc_ctx_p ctx, av_rect_p rect)
{
	if (ctx)
	{
		if (ctx->x != rect->x || ctx->y != rect->y ||
			ctx->w != rect->w || ctx->h != rect->h)
		{
			ctx->x = rect->x;
			ctx->y = rect->y;
			ctx->w = rect->w;
			ctx->h = rect->h;

			XLockDisplay(ctx->display);
			XMoveResizeWindow(ctx->display, ctx->win, rect->x, rect->y, rect->w, rect->h);
			XUnlockDisplay(ctx->display);
		}
		return AV_OK;
	}
	return AV_EGENERAL;
}

static av_result_t av_player_vlc_play(av_player_p self)
{
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);

	if (ctx)
	{
		if (is_vlc(ctx->vlc))
		{
			libvlc_media_player_play(ctx->pl, &ctx->ex);
			catch(ctx);

			av_player_vlc_show_window(ctx);
		}
		else
			return AV_EINIT;
	}
	return AV_OK;
}

static av_result_t av_player_vlc_pause(av_player_p self)
{
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);
	if (ctx)
	{
		if (is_vlc(ctx->vlc))
		{
			libvlc_media_player_pause(ctx->pl,&ctx->ex);
			catch(ctx);
		}
		else
			return AV_EINIT;
	}
	return AV_OK;
}

static av_result_t av_player_vlc_seek(av_player_p self, double pos_seconds)
{
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);
	if (ctx)
	{
		if (is_vlc(ctx->vlc))
		{
			libvlc_media_player_set_time(ctx->pl, pos_seconds*1000., &ctx->ex);
			catch(ctx);
		}
		else
			return AV_EINIT;
	}
	else
		return AV_EARG;
	return AV_OK;
}

static av_result_t av_player_vlc_seek_rel(struct av_player* self, double pos_seconds)
{
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);
	if (ctx)
	{
		if (is_vlc(ctx->vlc))
		{
			double position_seconds = 0.;
			position_seconds = libvlc_media_player_get_time(ctx->pl, &ctx->ex);
			position_seconds += pos_seconds*1000.;
			catch(ctx);
			libvlc_media_player_set_time(ctx->pl, position_seconds, &ctx->ex);
			catch(ctx);
		}
		else
			return AV_EINIT;
	}
	else
		return AV_EARG;
	return AV_OK;
}

static double av_player_vlc_duration(struct av_player* self)
{
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);
	if (ctx)
	{
		if (is_vlc(ctx->vlc))
		{
			double length_seconds = 0.;
			length_seconds = libvlc_media_player_get_length(ctx->pl, &ctx->ex);
			catch(ctx);
			return length_seconds/1000.;
		}
	}
	return 0.;
}

static double av_player_vlc_position(struct av_player* self)
{
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);
	if (ctx)
	{
		if (is_vlc(ctx->vlc))
		{
			double position_seconds = 0.;
			position_seconds = libvlc_media_player_get_time(ctx->pl, &ctx->ex);
			catch(ctx);
			return position_seconds/1000.;
		}
	}
	return 0.;
}
static av_bool_t av_player_on_update(av_window_p self, av_video_surface_p dstsurface, av_rect_p rect)
{
	av_rect_t windowrect;
	av_rect_p winrect = &windowrect;
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);
	self->get_rect(self, winrect);
	winrect->x = winrect->y = 0;
	self->rect_absolute(self, winrect);

	if (ctx)
	{
		if (ctx->win != None)
		{
			av_player_vlc_set_window_pos_size(ctx, winrect);
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
	AV_UNUSED(data);
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);
	AV_UNUSED(ctx);
	switch(id)
	{
		default:
			break;
	}

	return AV_TRUE;
}

static void av_player_vlc_set_window(av_player_p self, av_window_p window)
{
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);

	ctx->owner = window;
	ctx->owner->on_update = av_player_on_update;
	ctx->owner->on_user = av_player_on_user;
	O_set_attr(ctx->owner, CONTEXT, ctx);

	window->update(window, AV_UPDATE_INVALIDATE);
}

static av_result_t av_player_vlc_stop(av_player_p self)
{
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);
	if(ctx)
	{
		if (is_vlc(ctx->vlc))
		{
			av_player_vlc_hide_window(ctx);

			libvlc_media_player_stop(ctx->pl, &ctx->ex);
			catch(ctx);
			if (ctx->md)
			{
				libvlc_media_release(ctx->md);
				catch(ctx);
				ctx->md = AV_NULL;
			}
		}
		else
			return AV_EINIT;

		ctx->state.state = avps_idle;
	}
	return AV_OK;
}

/* open media file */
static av_result_t av_player_vlc_open(av_player_p self, const char* url)
{
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);

	if (ctx)
	{
		self->stop(self);
		if (is_vlc(ctx->vlc))
		{
			ctx->md = libvlc_media_new(ctx->vlc, url, &ctx->ex);
			catch(ctx);
			if (!ctx->md)
				return AV_EARG;

			libvlc_media_player_set_media(ctx->pl, ctx->md, &ctx->ex);
			catch(ctx);
		}
		else
			return AV_EINIT;
	}
	return AV_OK;
}

static av_result_t av_player_vlc_is_playing(struct av_player* self)
{
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);

	if(ctx)
	{
		if (is_vlc(ctx->vlc))
		{
			libvlc_state_t state;
			state = libvlc_media_player_get_state (ctx->pl, &ctx->ex);
			switch(state)
			{
				case libvlc_NothingSpecial:
				case libvlc_Error:
				default:
					ctx->state.state_ext.state = avps_idle;
					break;

				case libvlc_Opening:
					ctx->state.state_ext.state = avps_playing|avps_connecting;
					break;

				case libvlc_Buffering:
					ctx->state.state_ext.state = avps_playing|avps_buffering;
					break;

				case libvlc_Paused:
				case libvlc_Playing:
					ctx->state.state_ext.state = avps_playing|avps_running;
					break;

				case libvlc_Stopped:
				case libvlc_Ended:
					/*ctx->state.state_ext.state = avps_playing|avps_finished;*/
					ctx->state.state_ext.state = avps_running;
					break;
			}
			return ctx->state.state;
		}
	}
	return avps_idle;
}

static av_result_t av_player_vlc_is_pause(struct av_player* self)
{
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);

	if(ctx)
	{
		if (is_vlc(ctx->vlc))
		{
			libvlc_state_t state;
			state = libvlc_media_player_get_state (ctx->pl, &ctx->ex);
			switch(state)
			{
				case libvlc_NothingSpecial:
				case libvlc_Error:
				default:
					ctx->state.state = avps_idle;
					break;

				case libvlc_Opening:
					ctx->state.state = avps_connecting;
					break;

				case libvlc_Buffering:
					ctx->state.state = avps_buffering;
					break;

				case libvlc_Paused:
					ctx->state.state = avps_paused|avps_running;
					break;

				case libvlc_Playing:
					ctx->state.state = avps_running;
					break;

				case libvlc_Stopped:
				case libvlc_Ended:
					ctx->state.state = avps_finished;
					break;
			}
			return ctx->state.state;
		}
	}
	return avps_idle;
}

static void av_player_vlc_set_borders(av_player_p self, av_bool_t val)
{
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);
	if (ctx)
		ctx->draw_borders = val;
}

/* Destructor */
static void av_player_vlc_destructor(void* pobject)
{
	av_player_p self = (av_player_p)pobject;
	av_player_vlc_ctx_p ctx = (av_player_vlc_ctx_p)O_context(self);
	if (ctx)
	{
		if (is_vlc(ctx->vlc))
		{
			self->stop(self);

			if(ctx->pl)
			{
				libvlc_media_player_release(ctx->pl);
				ctx->pl = AV_NULL;
				catch(ctx);
			}

			libvlc_release(ctx->vlc);
			ctx->vlc = AV_NULL;
			catch(ctx);
		}

		if (None != ctx->win)
		{
			XLockDisplay(ctx->display);
			XDestroyWindow(ctx->display, ctx->win);
			XUnlockDisplay(ctx->display);
			ctx->win = None;
		}

		while (ctx->config.ppsz_argc-->0)
			free(ctx->config.ppsz_argv[ctx->config.ppsz_argc]);
		free(ctx->config.ppsz_argv);
		free(ctx);
	}
	av_torb_service_release("system");
}

/* Constructor */
static av_result_t av_player_vlc_constructor(av_object_p object)
{
	int i;
	av_result_t rc;
	av_player_vlc_ctx_p ctx;
	av_player_p self = (av_player_p)object;

	/* initializes self */
	ctx = malloc(sizeof(av_player_vlc_ctx_t));
	if (!ctx)
		return AV_EMEM;
	memset(ctx, 0, sizeof(av_player_vlc_ctx_t));

	ctx->win = None;
	ctx->vlc = AV_NULL;
	ctx->pl = AV_NULL;
	ctx->md = AV_NULL;

	ctx->draw_borders = AV_TRUE;
	ctx->state.state = avps_idle;

	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
		goto const_err_0;

	load_prefs();

	rc = AV_EINIT;

	if (AV_OK != (rc = sys->get_timer(sys, &ctx->timer)))
		goto const_err_1;

	if (AV_OK != (rc = sys->get_input(sys, &ctx->input)))
		goto const_err_1;

	if (AV_OK != (rc = sys->get_audio(sys, &ctx->audio)))
		goto const_err_1;

	if (AV_OK != (rc = sys->get_video(sys, &ctx->video)))
		goto const_err_1;

	if (AV_OK != ctx->video->get_root_window(ctx->video, (void**)&ctx->display, (void**)&ctx->parent))
		goto const_err_1;

	
	ctx->config.ppsz_argc = 0;
	ctx->config.ppsz_argv = AV_NULL;

	for (i=0; i<defconfig.ppsz_argc; i++)
	{
		ctx->config.ppsz_argv = (char**)realloc(ctx->config.ppsz_argv, sizeof(char*)*(ctx->config.ppsz_argc+1));
		ctx->config.ppsz_argv[ctx->config.ppsz_argc++] = strdup(defconfig.ppsz_argv[i]);
	}

	ctx->config.ppsz_argv = (char**)realloc(ctx->config.ppsz_argv, sizeof(char*)*(ctx->config.ppsz_argc+1));
	ctx->config.ppsz_argv[ctx->config.ppsz_argc++] = strformatstrdup("--plugin-path=%s",defconfig.ppsz_pluginpath);

	ctx->config.ppsz_argv = (char**)realloc(ctx->config.ppsz_argv, sizeof(char*)*(ctx->config.ppsz_argc+1));
    ctx->config.ppsz_argv[ctx->config.ppsz_argc++] = strformatstrdup("--aout=%s",defconfig.ppsz_aout);

	ctx->config.ppsz_argv = (char**)realloc(ctx->config.ppsz_argv, sizeof(char*)*(ctx->config.ppsz_argc+1));
	ctx->config.ppsz_argv[ctx->config.ppsz_argc++] = strformatstrdup("--vout=%s",defconfig.ppsz_vout);
	ctx->config.ppsz_argv = (char**)realloc(ctx->config.ppsz_argv, sizeof(char*)*(ctx->config.ppsz_argc+1));
	ctx->config.ppsz_argv[ctx->config.ppsz_argc++] = strformatstrdup("--xvideo-display=%s",DisplayString(ctx->display));
	ctx->config.ppsz_argv = (char**)realloc(ctx->config.ppsz_argv, sizeof(char*)*(ctx->config.ppsz_argc+1));
	ctx->config.ppsz_argv[ctx->config.ppsz_argc++] = strdup("--xvideo-shm");
	ctx->config.ppsz_argv = (char**)realloc(ctx->config.ppsz_argv, sizeof(char*)*(ctx->config.ppsz_argc+1));
	ctx->config.ppsz_argv[ctx->config.ppsz_argc++] = strdup("--overlay");
	ctx->config.ppsz_argv = (char**)realloc(ctx->config.ppsz_argv, sizeof(char*)*(ctx->config.ppsz_argc+1));
	ctx->config.ppsz_argv[ctx->config.ppsz_argc++] = strdup("--no-video-deco");

	ctx->config.ppsz_argv = (char**)realloc(ctx->config.ppsz_argv, sizeof(char*)*(ctx->config.ppsz_argc+1));
	ctx->config.ppsz_argv[ctx->config.ppsz_argc++] = strformatstrdup("--subsdec-encoding=%s",defconfig.ppsz_sub_enc);

	ctx->config.ppsz_argv = (char**)realloc(ctx->config.ppsz_argv, sizeof(char*)*(ctx->config.ppsz_argc+1));
	ctx->config.ppsz_argv[ctx->config.ppsz_argc++] = strformatstrdup("--freetype-font=%s",defconfig.ppsz_sub_font);

	ctx->config.ppsz_argv = (char**)realloc(ctx->config.ppsz_argv, sizeof(char*)*(ctx->config.ppsz_argc+1));
	ctx->config.ppsz_argv[ctx->config.ppsz_argc++] = defconfig.debug?
			strformatintdup("-v%d",defconfig.debug) : strdup("--quiet");

	ctx->config.ppsz_argv = (char**)realloc(ctx->config.ppsz_argv, sizeof(char*)*(ctx->config.ppsz_argc+1));
	ctx->config.ppsz_argv[ctx->config.ppsz_argc] = AV_NULL;

	/* initializes vlc interface */
	libvlc_exception_init(&ctx->ex);
	if (AV_NULL == (ctx->vlc = libvlc_new(ctx->config.ppsz_argc, (const char**)ctx->config.ppsz_argv, &ctx->ex)))
	{
		catch(ctx);
		rc = AV_EMEM;
		goto const_err_1;
	}
	catch(ctx);

	if (AV_NULL == (ctx->pl = libvlc_media_player_new(ctx->vlc, &ctx->ex)))
	{
		catch(ctx);
		rc = AV_EMEM;
		goto const_err_2;
	}
	catch(ctx);

	if(ctx->video)
	{
		XLockDisplay(ctx->display);

		XSetWindowAttributes win_attrib;
		win_attrib.border_pixel = 0x00FFFFFFU;
		win_attrib.background_pixel = 0x00000000U;
		win_attrib.override_redirect = 0;
		unsigned long win_mask = CWBackPixel | CWBorderPixel | CWOverrideRedirect;

		ctx->x = 0;
		ctx->y = 0;
		ctx->w = 1;
		ctx->h = 1;

		ctx->screen = DefaultScreen(ctx->display);
		ctx->win = XCreateWindow(ctx->display, ctx->parent,
								 ctx->x, ctx->y, ctx->w, ctx->h,
								 0, 0, InputOutput, CopyFromParent, win_mask, &win_attrib);

		if (None == ctx->win)
		{
			XSync(ctx->display, False);
			XUnlockDisplay(ctx->display);
			rc = AV_EINIT;
			goto const_err_3;
		}

		XSelectInput(ctx->display, ctx->win, 0);

		XSync(ctx->display, False);
		XUnlockDisplay(ctx->display);

		libvlc_media_player_set_xwindow(ctx->pl, (libvlc_drawable_t)ctx->win, &ctx->ex);
		catch(ctx);
	}

	O_set_attr(self, CONTEXT, ctx);
	self->open         = av_player_vlc_open;
	self->play         = av_player_vlc_play;
	self->stop         = av_player_vlc_stop;
	self->pause        = av_player_vlc_pause;
	self->set_window   = av_player_vlc_set_window;
	self->seek         = av_player_vlc_seek;
	self->seek_rel     = av_player_vlc_seek_rel;
	self->duration     = av_player_vlc_duration;
	self->position     = av_player_vlc_position;
	self->is_playing   = av_player_vlc_is_playing;
	self->is_pause     = av_player_vlc_is_pause;
	self->set_borders  = av_player_vlc_set_borders;
	return AV_OK;

/* Cleanup */
const_err_3: libvlc_media_player_release(ctx->pl); ctx->pl = AV_NULL; catch(ctx);
const_err_2: libvlc_release(ctx->vlc); ctx->vlc = AV_NULL; catch(ctx);
const_err_1: log_error("av_player_vlc_constructor failed"); av_torb_service_release("system");
const_err_0: free(ctx); ctx = AV_NULL;
	return rc;
}

AV_API av_result_t av_player_vlc_register_torba(void);
av_result_t av_player_vlc_register_torba(void)
{
	if (!XInitThreads())
		return AV_EGENERAL;

	return av_torb_register_class("player", AV_NULL, sizeof(av_player_t), av_player_vlc_constructor, av_player_vlc_destructor);
}

#endif /* WITH_VLC */
