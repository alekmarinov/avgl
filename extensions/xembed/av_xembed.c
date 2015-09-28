/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_xembed.c                                        */
/*                                                                   */
/*********************************************************************/

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <bits/signum.h>
#include <errno.h>
#include <xembed.h>
#include <av_xembed.h>
#include <av_xembed_app.h>
#include "av_xembed_surface.h"
#include <av_log.h>
#include <av_prefs.h>
#include <av_system.h>
#include <av_input.h>

#ifdef __cplusplus
extern "C" {
#endif

int kill(pid_t pid, int sig);
int setenv(const char *name, const char *value, int overwrite);
/*const char *sys_errlist[0];*/
int asprintf(char **strp, const char *fmt, ...);

#define XVFB_DISPLAY_DEFAULT 1
#define XVFB_SCREEN_DEFAULT 0
#define XVFB_VMEM(shptr) (((unsigned char*)shptr) + 3232)
#define XVFB_XRES_DEFAULT 1024
#define XVFB_YRES_DEFAULT 768
#define XVFB_MODE_BPP 24
#define XVFB_BITS_PER_PIXEL 32

#define CONTEXT "xembed_ctx"
#define O_context(o) O_attr(o, CONTEXT)

typedef struct av_xembed_ctx
{
	pid_t xvfb_pid;
	xembed_init_t xe_init;
	av_system_p system;
	av_video_surface_p surface;
	av_video_surface_p bbuffer_surface;
	int xres;
	int yres;
	int bpp;
	av_xembed_p xembed;
} av_xembed_ctx_t, *av_xembed_ctx_p;

static const char* xvfb_path_default = "Xvfb";
static const char* xmodmap_path_default = "xmodmap";
static const char* xdpyinfo_path_default = "xdpyinfo";

static av_log_p _log = AV_NULL;
static av_prefs_p _prefs = AV_NULL;

/* FIXME: av_xembed_damage_cb doesn't provide user parameter */
static av_xembed_ctx_p g_xembed_ctx = AV_NULL;

static av_result_t handle_error(const char* filename, int nline)
{
	switch (errno)
	{
		case EFAULT:
			_log->error(_log, "Error in source %s:%d, Invalid file descriptor",
							  filename, nline);
		return AV_EIO;
		case EMFILE:
			_log->error(_log, "Error in source %s:%d, Too many file descriptors are in use by the process",
							  filename, nline);
		return AV_EIO;
		case ENFILE:
			_log->error(_log, "Error in source %s:%d, The system limit on the total number of open files has been reached", filename, nline);
		return AV_EIO;
	}

	return AV_EGENERAL;
}

static void av_xembed_damage_cb(xembed_damage_t* dmginfo)
{
	if (g_xembed_ctx)
	{
		av_rect_t rect;
		av_rect_init(&rect, dmginfo->x, dmginfo->y, dmginfo->w, dmginfo->h);
		if (g_xembed_ctx->xembed->on_damage)
		{
			g_xembed_ctx->xembed->on_damage(g_xembed_ctx->xembed, &rect);
		}
	}
}

static void av_xembed_map_cb(xembed_map_t* mapinfo)
{
	AV_UNUSED(mapinfo);
	_log->debug(_log, "av_xembed_map_cb");
}

static void av_xembed_unmap_cb(xembed_unmap_t* unmapinfo)
{
	AV_UNUSED(unmapinfo);
	_log->debug(_log, "av_xembed_unmap_cb");
}

static void av_xembed_configure_cb(xembed_configure_t* configinfo)
{
	AV_UNUSED(configinfo);
	_log->debug(_log, "av_xembed_configure_cb");
}

static void av_xembed_create_cb(xembed_create_t* createinfo)
{
	AV_UNUSED(createinfo);
	_log->debug(_log, "av_xembed_create_cb");
}

static av_result_t av_xembed_pout(const char** argv, int stream, int nlines, char** lines, int *pnlines)
{
	int pfd[2];
	int nline;
	pid_t pid, cpid;

	/* creates pipe */
	if (-1 == pipe(pfd))
	{
		_log->error(_log, "pipe failed");
		return handle_error(__FILE__, __LINE__);
	}

	/* fork process */
	if (0 > (pid = fork()))
	{
		_log->error(_log, "fork failed");
		return handle_error(__FILE__, __LINE__);
	}

	/* pid = 0 -> child process, pid > 0 parent process, pid = child pid */
	if (0 == pid)
	{
		/* the child process */

		int retval;
		pid = getpid();

		/* closes in-pipe */
		close(pfd[0]);

		/* replaces stderr with out-pipe */
		dup2(pfd[1], stream);
		close(pfd[1]);

		/* Executes program */
		retval = execvp(argv[0], (char * const*)argv);
		if (-1 != retval)
		{
			_log->info(_log, "pid %d: `%s' started.", pid, argv[0]);
			wait(&retval);
		}
		else
		{
			_log->info(_log, "pid %d: %s", pid, sys_errlist[errno]);
		}
		exit(0);
	}

	/* the parent process */

	/* saves child pid */
	cpid = pid;
	
	/* get parent pid */
	pid = getpid();

	/* closes out-pipe, dup2 in-pipe = stdin */
	close(pfd[1]);
	dup2(pfd[0], 0);
	close(pfd[0]);

	nline = 0;
	lines[nline] = (char*)malloc(255);
	while (nline < nlines)
	{
		if (!fgets(lines[nline], 255, stdin))
		{
			break;
		}
		lines[++nline] = (char*)malloc(255);
	}
	*pnlines = nline;
	return AV_OK;
}

static av_result_t xvfb_exec(av_xembed_ctx_p ctx, int xres, int yres)
{
	av_result_t rc;
	char xvfb_dpy[5];
	char xvfb_res[20];
	char xvfb_screen[4];
	const char *xvfbpath;
	const char *xmodmappath;
	const char *xdpyinfopath;
	const char* xvfbshmid;
	char xmodmap_expr_param[255];
	int dpy, screen, i;
	char* lines[200];
	int nlines;
	const char** argv;
	int cxres = 0, cyres = 0;
	FILE* file;
	char dummy[50];

	ctx->bpp = XVFB_BITS_PER_PIXEL;

	_prefs->get_int(_prefs, "extensions.xembed.xvfb.display",
					XVFB_DISPLAY_DEFAULT, &dpy);

	_prefs->get_int(_prefs, "extensions.xembed.xvfb.screen",
					XVFB_SCREEN_DEFAULT, &screen);
	sprintf(xvfb_screen, "%d", screen);

	sprintf(xvfb_dpy, ":%d", dpy);

	ctx->xres = xres;
	ctx->yres = yres;

	sprintf(xvfb_res, "%dx%dx%d", ctx->xres, ctx->yres, XVFB_MODE_BPP);

	_prefs->get_string(_prefs, "extensions.xembed.xvfb.path",
					xvfb_path_default, &xvfbpath);
	_prefs->get_string(_prefs, "extensions.xembed.xmodmap.path",
					xmodmap_path_default, &xmodmappath);
	_prefs->get_string(_prefs, "extensions.xembed.xdpyinfo.path",
					xdpyinfo_path_default, &xdpyinfopath);

	setenv("DISPLAY", xvfb_dpy, AV_TRUE);

	argv = (const char**)malloc(4*sizeof(char*));
	argv[0] = xdpyinfopath;
	argv[1] = AV_NULL;

	if (AV_OK != (rc = av_xembed_pout((const char **)argv, 1, 200, lines, &nlines)))
	{
		free(argv);
		return rc;
	}
	free(argv);

	if (nlines > 0)
	{
		/* parses xdpyinfo output */
		
		nlines--;
		while (nlines--)
		{
			if (strstr(lines[nlines], "dimensions:"))
			{
				sscanf(lines[nlines], "%s %s", dummy, dummy);
				sscanf(dummy, "%dx%d", &cxres, &cyres);
				break;
			}
		}
		_log->info(_log, "xdpyinfo parse output results %dx%d", cxres, cyres);
	}

	_prefs->get_string(_prefs, "extensions.xembed.xvfb.shmid", AV_NULL, &xvfbshmid);
	if (!xvfbshmid)
	{
		return AV_EFOUND;
	}

	if (cxres == xres && cyres == yres)
	{
		/* Xvfb already running with correct resolution set */
		_log->info(_log, "Xvfb is already running on %dx%d, reading `%s'", cxres, cyres, xvfbshmid);
		file = fopen(xvfbshmid, "r");
		if (file)
		{
			fscanf(file, "%d", &ctx->xe_init.shmid);
			fclose(file);
		}
		else
		{
			_log->info(_log, "File `%s' not found!", xvfbshmid);
		}
	}

	if (ctx->xe_init.shmid <= 0)
	{
		/* Kill Xvfb */
		const char* xvfbprocname;

		if (AV_OK == _prefs->get_string(_prefs, "extensions.xembed.xvfb.procname", AV_NULL, &xvfbprocname))
		{
			char buf[200];
			snprintf(buf, 200, "killall %s 2> /dev/null", xvfbprocname);
			system(buf);
			snprintf(buf, 200, "killall -9 %s 2> /dev/null", xvfbprocname);
			system(buf);
			_log->info(_log, "Killed all %s processes!", xvfbprocname);
		}

		/* Start Xvfb */
		argv = (const char**)malloc(18*sizeof(char*));
		int na = 0;
		argv[na++] = (const char*)xvfbpath;
		argv[na++] = xvfb_dpy;
		argv[na++] = "-ac";
		argv[na++] = "-br";
		argv[na++] = "-screen";
		argv[na++] = xvfb_screen;
		argv[na++] = xvfb_res;
		argv[na++] = "-nolisten";
		argv[na++] = "tcp";
		argv[na++] = "-shmem";
		/*argv[na++] = "+kb";*/
		argv[na++] = "+extension";
		argv[na++] = "DAMAGE";
		argv[na++] = "+extension";
		argv[na++] = "Composite";
		argv[na++] = "+extension";
		argv[na++] = "XInputExtension";
		argv[na] = AV_NULL;
		{
			/* log xvfb command */	
			char msgbuf[512];
			msgbuf[0] = 0;
			for (i=0; i<na; i++) { strcat(msgbuf, argv[i]); strcat(msgbuf, " "); }
			_log->info(_log, msgbuf);
		}

		if (AV_OK != (rc = av_xembed_pout(argv, 2, 1, lines, &nlines)))
		{
			free(argv);
			return rc;
		}
		free(argv);

		sscanf(lines[0], "%s %d %s %d", dummy, &screen, dummy, &ctx->xe_init.shmid);
		_log->info(_log, "Child process %d resulted `%s': screen=%d, shmid=%d",
					ctx->xvfb_pid, lines[0], screen, ctx->xe_init.shmid);

		file = fopen(xvfbshmid, "w");
		fprintf(file, "%d", ctx->xe_init.shmid);
		fclose(file);
		
		/* Setup Xvfb key mappings */
		for (i=0; i<255; i++)
		{
			const char *expr;
			sprintf(xmodmap_expr_param, "extensions.xembed.xmodmap.expr.%d", i);
			_prefs->get_string(_prefs, xmodmap_expr_param, AV_NULL, &expr);
			if (expr)
			{
				char command[256];
				int status;
				strncpy(command, xmodmappath, 255);
				strcat (command, " -display ");
				strcat (command, xvfb_dpy);
				strcat (command, " -e");
				strcat (command, " \"");
				strncat(command, expr, 250-strlen(command));
				strcat (command, "\"");
				status = system (command);
				_log->debug(_log, "Executed `%s' with exit status %d", command, status);
			}
		}
	}

	if (ctx->xe_init.shmid > 0)
	{
		ctx->xe_init.display             = xvfb_dpy;
		ctx->xe_init.xembed_map_cb       = av_xembed_map_cb;
		ctx->xe_init.xembed_unmap_cb     = av_xembed_unmap_cb;
		ctx->xe_init.xembed_create_cb    = av_xembed_create_cb;
		ctx->xe_init.xembed_damage_cb    = av_xembed_damage_cb;
		ctx->xe_init.xembed_configure_cb = av_xembed_configure_cb;

		_log->info(_log, "initializing xembed");
		if (!xembed_init(&ctx->xe_init))
		{
			_log->error(_log, "xembed_init failed");
			return AV_EGENERAL;
		}

		return AV_OK;
	}
	return AV_EGENERAL;
}

static av_result_t av_xembed_open(av_xembed_p self, int xres, int yres)
{
	av_result_t rc;
	av_video_p video;
	av_xembed_video_t xembed_video;
	av_xembed_ctx_p ctx = (av_xembed_ctx_p)O_context(self);

	if (AV_OK != (rc = xvfb_exec(ctx, xres, yres)))
	{
		self->close(self);
		return rc;
	}

	if (!ctx->xe_init.shmptr)
		return AV_ESTATE;

	self->get_video_info(self, &xembed_video);
	ctx->system->get_video(ctx->system, &video);
	if (AV_OK != video->create_surface_from(video, xembed_video.vmem,
											xembed_video.xres, xembed_video.yres,
											xembed_video.xres << 2, &(ctx->surface)))
	{
		self->close(self);
		return rc;
	}

	video->get_backbuffer(video, &ctx->bbuffer_surface);

	return AV_OK;
}

static av_result_t av_xembed_execute(av_xembed_p self, char* const argv[],
									 av_window_p window, av_xembed_app_p* ppapp)
{
	av_xembed_app_p app;
	av_xembed_app_config_t config;
	av_result_t rc;
	pid_t pid;

	AV_UNUSED(self);

	if (AV_OK != (rc = av_torb_create_object("xembed_app", (av_object_p*)&app)))
		return rc;

	/* fork process */
	if (0 > (pid = fork()))
	{
		_log->error(_log, "fork failed");
		return handle_error(__FILE__, __LINE__);
	}

	/* pid = 0 -> child process, pid > 0 parent process, pid = child pid */
	if (0 == pid)
	{
		int retval;
		pid = getpid();
		if (-1 != execvp(argv[0], argv))
		{
			_log->info(_log, "pid %d: `%s' started.", pid, argv[0]);
			wait(&retval);
		}
		else
		{
			_log->info(_log, "pid %d: %s", pid, sys_errlist[errno]);
		}
		exit(0);
	}

	config.pid = pid; /* pid to the xembedded application */
	config.window = window;
	pid = getpid();   /* the pid to the currently process */

	if (AV_OK != (rc = app->set_configuration(app, &config)))
	{
		return rc;
	}
	*ppapp = app;

	return AV_OK;
}

static av_result_t av_xembed_idle(av_xembed_p self)
{
	AV_UNUSED(self);
	xembed_step();
	return AV_OK;
}

static void av_xembed_close(av_xembed_p self)
{
	int exit_status;
	av_xembed_ctx_p ctx = (av_xembed_ctx_p)O_context(self);
	if (ctx->xvfb_pid > 0)
	{
		if (ctx->xe_init.shmptr)
		{
			xembed_stop(&ctx->xe_init);
			memset(&ctx->xe_init, 0, sizeof(xembed_init_t));
		}

		kill(ctx->xvfb_pid, SIGTERM);
		waitpid(ctx->xvfb_pid, &exit_status, 0);
		if (WIFEXITED(exit_status))
		{
			_log->info(_log, "Child process %d terminated with status %d",
							ctx->xvfb_pid, WEXITSTATUS(exit_status));
		}
	}
	ctx->xvfb_pid = 0;

	if (ctx->surface)
		O_destroy(ctx->surface);
}

static av_result_t av_xembed_copy(av_xembed_p self, av_rect_p dstrect, av_rect_p srcrect)
{
	av_xembed_ctx_p ctx = (av_xembed_ctx_p)O_context(self);
	av_rect_t src_rect, dst_rect;
	av_rect_t rect;

	av_rect_init(&rect, 0, 0, ctx->xres, ctx->yres);

	if (!av_rect_intersect(&rect, srcrect, &src_rect))
		return AV_EARG;

	if (!av_rect_intersect(&rect, dstrect, &dst_rect))
		return AV_EARG;

	return ctx->bbuffer_surface->blit(ctx->bbuffer_surface, &dst_rect, (av_surface_p)ctx->surface, &src_rect);
/*
	{
		av_xembed_video_t xembed_video;
		unsigned int *src;
		av_pixel_p dst;
		int pitch, i;
		self->get_video_info(self, &xembed_video);

		src = (unsigned int *)xembed_video.vmem;
		((av_surface_p)bbuffer_surface)->lock((av_surface_p)bbuffer_surface,
		  AV_SURFACE_LOCK_WRITE, &dst, &pitch);

		src += dst_rect.y * xembed_video.xres + dst_rect.x;
		dst += dst_rect.y * bbuffer_rect.w + dst_rect.x;
		for (i=0; i<dst_rect.h; i++)
		{
			memcpy(dst, src, 4*dst_rect.w);
			dst += bbuffer_rect.w;
			src += xembed_video.xres;
		}
		((av_surface_p)bbuffer_surface)->unlock((av_surface_p)bbuffer_surface);
	}
	return AV_OK;
*/
}

static av_result_t av_xembed_screenshot(av_xembed_p self, const char* filename)
{
	av_result_t rc;
	av_video_p video;
	av_graphics_p graphics;
	av_graphics_surface_p graphics_surface;
	av_xembed_ctx_p ctx = (av_xembed_ctx_p)O_context(self);
	av_video_surface_p surface;
	int width, height;
	av_pixel_p srcpixels;
	int srcpitch;
	av_pixel_p dstpixels;
	int dstpitch;
	int i, j;

	if (AV_OK != (rc = ctx->system->get_video(ctx->system, &video)))
	{
		return rc;
	}

	if (AV_OK != (rc = ctx->system->get_graphics(ctx->system, &graphics)))
	{
		return rc;
	}

	ctx->surface->get_size(ctx->surface, &width, &height);

	if (AV_OK != (rc = video->create_surface(video, width, height, &surface)))
	{
		return rc;
	}

	if (AV_OK != (rc = ((av_surface_p)ctx->surface)->lock((av_surface_p)ctx->surface, AV_SURFACE_LOCK_READ,
														  &srcpixels, &srcpitch)))
	{
		O_destroy(surface);
		return rc;
	}

	if (AV_OK != (rc = surface->lock(surface, AV_SURFACE_LOCK_WRITE, &dstpixels, &dstpitch)))
	{
		O_destroy(surface);
		return rc;
	}

	for (i=0; i<height; i++)
		for (j=0; j<width; j++)
		{
			dstpixels[i*width + j] = srcpixels[i*width + j] | 0xff000000U;
		}

	surface->unlock(surface);
	((av_surface_p)ctx->surface)->unlock((av_surface_p)ctx->surface);

	if (AV_OK != (rc = graphics->wrap_surface(graphics, (av_surface_p)surface, &graphics_surface)))
	{
		O_destroy(surface);
		return rc;
	}

	rc = graphics->save_surface_file(graphics, graphics_surface, filename);
	O_destroy(graphics_surface);
	O_destroy(surface);
	return rc;
}

static av_result_t av_xembed_get_video_surface(av_xembed_p self,
											   av_video_surface_p* ppsurface)
{
	av_xembed_ctx_p ctx = (av_xembed_ctx_p)O_context(self);
	*ppsurface = ctx->surface;
	return AV_OK;
}

static av_result_t av_xembed_get_video_info(av_xembed_p self,
									 		av_xembed_video_t* xembed_video)
{
	av_xembed_ctx_p ctx = (av_xembed_ctx_p)O_context(self);
	if (!ctx->xe_init.shmptr)
		return AV_ESTATE;

	xembed_video->xres = ctx->xres;
	xembed_video->yres = ctx->yres;
	xembed_video->bpp  = ctx->bpp;
	xembed_video->vmem = XVFB_VMEM(ctx->xe_init.shmptr);

	return AV_OK;
}

static void av_xembed_send_mouse_move(av_xembed_p self, int x, int y)
{
	AV_UNUSED(self);
	xembed_send_mouse_move(x, y);
}

static void av_xembed_send_mouse_button(av_xembed_p self,
										av_event_mouse_button_t button,
							  			av_event_button_status_t status)
{
	mouse_button_t send_button = 0;
	mouse_button_state_t button_state = 0;
	AV_UNUSED(self);

	switch (button)
	{
		case AV_MOUSE_BUTTON_LEFT:
			send_button = MOUSE_BUTTON_LEFT;
		break;
		case AV_MOUSE_BUTTON_RIGHT:
			send_button = MOUSE_BUTTON_RIGHT;
		break;
		case AV_MOUSE_BUTTON_MIDDLE:
			send_button = MOUSE_BUTTON_MIDDLE;
		break;
		case AV_MOUSE_BUTTON_WHEEL:
			send_button = MOUSE_BUTTON_WHEEL;
		break;
		default:
			_log->warn(_log, "av_xembed_send_mouse_button: Unhandled mouse button %d", button);
			return ;
		break;
	}

	switch (status)
	{
		case AV_BUTTON_RELEASED:
			button_state = MOUSE_BUTTON_STATE_UP;
		break;
		case AV_BUTTON_PRESSED:
			button_state = MOUSE_BUTTON_STATE_DOWN;
		break;
	}

	xembed_send_mouse_button(send_button, button_state);
}

static void av_xembed_send_key(av_xembed_p self, av_key_t key, av_keymod_t modifiers,
							   av_event_button_status_t status)
{
	av_xembed_ctx_p ctx = (av_xembed_ctx_p)O_context(self);
	av_input_p input;

	if (AV_OK != ctx->system->get_input(ctx->system, &input))
	{
		return ;
	}

	switch (status)
	{
		case AV_BUTTON_RELEASED:
			xembed_send_key_release(key, modifiers);
		break;
		case AV_BUTTON_PRESSED:
			xembed_send_key_press(key, modifiers);
		break;
	}
}

static void av_xembed_set_cursor_visible(av_xembed_p self, av_bool_t cursor_visible)
{
	AV_UNUSED(self);
	xembed_set_cursor_visible(cursor_visible);
}

static av_result_t av_xembed_constructor(av_object_p pobject)
{
	av_result_t rc;
	av_xembed_p self = (av_xembed_p)pobject;
	av_xembed_ctx_p ctx = (av_xembed_ctx_p)malloc(sizeof(av_xembed_ctx_t));
	memset(ctx, 0, sizeof(av_xembed_ctx_t));
	g_xembed_ctx = ctx;
	ctx->xembed = self;
	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&_log)))
	{
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = av_torb_service_addref("prefs", (av_service_p*)&_prefs)))
	{
		av_torb_service_release("log");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = av_torb_service_addref("system",
											  (av_service_p*)&ctx->system)))
	{
		av_torb_service_release("prefs");
		av_torb_service_release("log");
		free(ctx);
		return rc;
	}

	ctx->xvfb_pid = 0;
	memset(&ctx->xe_init, 0, sizeof(xembed_init_t));

	O_set_attr(self, CONTEXT, ctx);
	self->open               = av_xembed_open;
	self->execute            = av_xembed_execute;
	self->idle               = av_xembed_idle;
	self->close              = av_xembed_close;
	self->copy               = av_xembed_copy;
	self->screenshot         = av_xembed_screenshot;
	self->get_video_surface  = av_xembed_get_video_surface;
	self->get_video_info     = av_xembed_get_video_info;
	self->send_mouse_move    = av_xembed_send_mouse_move;
	self->send_mouse_button  = av_xembed_send_mouse_button;
	self->send_key           = av_xembed_send_key;
	self->set_cursor_visible = av_xembed_set_cursor_visible;

	return AV_OK;
}

static void av_xembed_destructor(void* pobject)
{
	av_xembed_p self = (av_xembed_p)pobject;
	av_xembed_ctx_p ctx = (av_xembed_ctx_p)O_context(self);
	self->close(self);
	free(ctx);
	g_xembed_ctx = AV_NULL;

	av_torb_service_release("system");
	av_torb_service_release("prefs");
	av_torb_service_release("log");
}

av_result_t av_xembed_register()
{
	av_result_t rc;
	av_xembed_p xembed;

	if (AV_OK != (rc = av_xembed_surface_register_class()))
		return rc;

	if (AV_OK != (rc = av_torb_register_class("xembed", "service", sizeof(av_xembed_t),
											  av_xembed_constructor, av_xembed_destructor)))
		return rc;

	if (AV_OK != (rc = av_torb_create_object("xembed", (av_object_p*)&xembed)))
		return rc;

	return av_torb_service_register("xembed", (av_service_p)xembed);
}

#ifdef __cplusplus
}
#endif
