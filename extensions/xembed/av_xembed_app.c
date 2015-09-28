/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_xembed_app.c                                    */
/*                                                                   */
/*********************************************************************/

/* DEPRICATED MODULE */

#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <av_torb.h>
#include <av_log.h>
#include <av_window.h>
#include <av_thread.h>
#include <av_event.h>
#include <av_system.h>
#include <av_input.h>
#include <av_media.h>
#include <av_xembed.h>
#include <av_xembed_app.h>
#include "av_xembed_surface.h"

#define EVENT_REFRESH_ID 1

#define CONTEXT "xembed_app_ctx"
#define O_context(o) O_attr(o, CONTEXT)

int kill(pid_t pid, int sig);

static av_log_p _log = AV_NULL;

typedef struct av_xembed_app_ctx
{
	av_xembed_p xembed;
	av_xembed_app_config_t config;
	av_xembed_video_t xembed_video;
	av_timer_p timer;
	av_input_p input;
	av_video_p video;
	av_system_p system;
	av_video_surface_p backbuffer;
	av_bool_t stop_request;
	unsigned int c;
} av_xembed_app_ctx_t, *av_xembed_app_ctx_p;

/* Forward reference */
static void schedule_refresh(av_xembed_app_ctx_p ctx, unsigned int delay);

static av_bool_t schedule_refresh_cb(void* arg)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)arg;
	av_event_t event;
	if (!ctx->stop_request)
	{
		memset(&event, 0, sizeof(av_event_t));
		event.type = AV_EVENT_USER;
		event.user_id = EVENT_REFRESH_ID;
		event.window = ctx->config.window;
		//ctx->input->push_event(ctx->input, &event);
	}
	return AV_FALSE;
}

/* schedule a video refresh in 'delay' ms */
static void schedule_refresh(av_xembed_app_ctx_p ctx, unsigned int delay)
{
	ctx->timer->add_timer(ctx->timer, schedule_refresh_cb, delay, ctx, 0);
}

static av_bool_t av_xembed_app_on_user(av_window_p window, int id, void* data)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(window);
	AV_UNUSED(data);
	av_assert(id == EVENT_REFRESH_ID, "Invalid event ID");

	if (!ctx->stop_request)
	{
		window->invalidate(window);
		//ctx->system->update(ctx->system);
		//ctx->xembed->idle(ctx->xembed);
		ctx->c++;
		//schedule_refresh(ctx, 40);
	}
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_paint(av_window_p window, av_graphics_p graphics)
{/*
	av_rect_t srect;
	av_rect_t drect;
	av_rect_t clip_rect;
	av_video_surface_p surface;
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(window);
	ctx->xembed->get_video_surface(ctx->xembed, &surface);

	av_assert(AV_OK == graphics->get_clip(graphics, &clip_rect), "get_clip support required");

	//av_rect_init(&srect, 0, 0, 500, 400);
	av_rect_init(&drect, 0, 0, 1024, 768);
	clip_rect.w--; clip_rect.h--;
	//((av_surface_p)ctx->backbuffer)->set_clip((av_surface_p)ctx->backbuffer, &clip_rect);
	printf("clip rect %d %d %d %d\n", clip_rect.x, clip_rect.y, clip_rect.w, clip_rect.h);
	ctx->backbuffer->blit(ctx->backbuffer, &clip_rect, surface, &clip_rect);
*/

	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(window);
	unsigned int *src;
	av_pixel_p dst;
	int pitch;
	av_rect_t winrect;
	av_rect_t screenrect;
	av_rect_t visible_rect;
	av_rect_t clip_rect;
	int i;
	int width, height;

	av_assert(AV_OK == graphics->get_clip(graphics, &clip_rect), "get_clip support is required");

	av_rect_copy(&winrect, window->get_rect(window));
	screenrect.x = screenrect.y = winrect.x = winrect.y = 0;
	window->rect_absolute(window, &winrect);

	((av_surface_p)ctx->backbuffer)->get_size((av_surface_p)ctx->backbuffer,
											  &screenrect.w, &screenrect.h);

//	if (!av_rect_intersect(&winrect, &screenrect, &visible_rect))
	//	return AV_TRUE;

	//printf("clipped rect %d, %d, %d, %d \n", clip_rect.x, clip_rect.y, clip_rect.w, clip_rect.h);
	if (!av_rect_intersect(&winrect, &clip_rect, &visible_rect))
		return AV_TRUE;

	src = (unsigned int *)ctx->xembed_video.vmem;
	((av_surface_p)ctx->backbuffer)->lock((av_surface_p)ctx->backbuffer,
										  AV_SURFACE_LOCK_WRITE, &dst, &pitch);
	dst += visible_rect.y * screenrect.w + visible_rect.x;
	width  = AV_MIN(visible_rect.w, ctx->xembed_video.xres - clip_rect.x);
	height = AV_MIN(visible_rect.h, ctx->xembed_video.yres - clip_rect.y);

	src += clip_rect.y * ctx->xembed_video.xres + clip_rect.x;
	//printf("blit region %d, %d \n", width, height);
	for (i=0; i<height; i++)
	{
		memcpy(dst, src, width << 2);
		dst += screenrect.w;
		src += ctx->xembed_video.xres;
	}
	((av_surface_p)ctx->backbuffer)->unlock((av_surface_p)ctx->backbuffer);
	//ctx->xembed->copy(ctx->xembed, &visible_rect, &visible_rect);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_mouse_enter(av_window_p self, int x, int y)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	ctx->video->set_cursor_visible(ctx->video, AV_FALSE);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_mouse_leave(av_window_p self, int x, int y)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	ctx->video->set_cursor_visible(ctx->video, AV_TRUE);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_mouse_move(av_window_p self, int x, int y)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	ctx->xembed->send_mouse_move(ctx->xembed, x, y);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_mouse_left_up(av_window_p self, int x, int y)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	ctx->xembed->send_mouse_button(ctx->xembed, AV_MOUSE_BUTTON_LEFT,
												AV_BUTTON_RELEASED);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_mouse_left_down(av_window_p self, int x, int y)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	self->system->set_focus(self->system, self);
	ctx->xembed->send_mouse_button(ctx->xembed, AV_MOUSE_BUTTON_LEFT,
												AV_BUTTON_PRESSED);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_mouse_right_up(av_window_p self, int x, int y)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	ctx->xembed->send_mouse_button(ctx->xembed, AV_MOUSE_BUTTON_RIGHT,
												AV_BUTTON_RELEASED);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_mouse_right_down(av_window_p self, int x, int y)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	ctx->xembed->send_mouse_button(ctx->xembed, AV_MOUSE_BUTTON_RIGHT,
												AV_BUTTON_PRESSED);

	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_mouse_middle_up(av_window_p self, int x, int y)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	ctx->xembed->send_mouse_button(ctx->xembed, AV_MOUSE_BUTTON_MIDDLE,
												AV_BUTTON_RELEASED);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_mouse_middle_down(av_window_p self, int x, int y)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	ctx->xembed->send_mouse_button(ctx->xembed, AV_MOUSE_BUTTON_MIDDLE,
												AV_BUTTON_PRESSED);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_mouse_wheel_up(av_window_p self, int x, int y)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	ctx->xembed->send_mouse_button(ctx->xembed, AV_MOUSE_BUTTON_WHEEL,
												AV_BUTTON_RELEASED);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_mouse_wheel_down(av_window_p self, int x, int y)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	ctx->xembed->send_mouse_button(ctx->xembed, AV_MOUSE_BUTTON_WHEEL,
												AV_BUTTON_PRESSED);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_key_up(av_window_p self, av_key_t key)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	ctx->xembed->send_key(ctx->xembed, key, AV_BUTTON_RELEASED);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_key_down(av_window_p self, av_key_t key)
{
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	ctx->xembed->send_key(ctx->xembed, key, AV_BUTTON_PRESSED);
	return AV_TRUE;
}

static av_bool_t av_xembed_app_on_focus(av_window_p self, av_bool_t isfocus)
{
	AV_UNUSED(self);
	AV_UNUSED(isfocus);
	return AV_TRUE;
}

static av_result_t av_xembed_app_set_configuration(av_xembed_app_p self,
												   av_xembed_app_config_p config)
{	
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);

	av_assert(config->pid, "pid can't be NULL");
	av_assert(config->window, "window can't be NULL");
	
	ctx->config.pid = config->pid;
	ctx->config.window = config->window;

	O_set_attr(ctx->config.window, CONTEXT, ctx);

	ctx->config.window->on_paint             = av_xembed_app_on_paint;
	ctx->config.window->on_user              = av_xembed_app_on_user;
	ctx->config.window->on_mouse_enter       = av_xembed_app_on_mouse_enter;
	ctx->config.window->on_mouse_leave       = av_xembed_app_on_mouse_leave;
	ctx->config.window->on_mouse_move        = av_xembed_app_on_mouse_move;
	ctx->config.window->on_mouse_left_up     = av_xembed_app_on_mouse_left_up;
	ctx->config.window->on_mouse_left_down   = av_xembed_app_on_mouse_left_down;
	ctx->config.window->on_mouse_right_up    = av_xembed_app_on_mouse_right_up;
	ctx->config.window->on_mouse_right_down  = av_xembed_app_on_mouse_right_down;
	ctx->config.window->on_mouse_middle_up   = av_xembed_app_on_mouse_middle_up;
	ctx->config.window->on_mouse_middle_down = av_xembed_app_on_mouse_middle_down;
	ctx->config.window->on_mouse_wheel_up    = av_xembed_app_on_mouse_wheel_up;
	ctx->config.window->on_mouse_wheel_down  = av_xembed_app_on_mouse_wheel_down;
	ctx->config.window->on_key_up            = av_xembed_app_on_key_up;
	ctx->config.window->on_key_down          = av_xembed_app_on_key_down;
	ctx->config.window->on_focus             = av_xembed_app_on_focus;

	schedule_refresh(ctx, 40);

	return AV_OK;
}

static void av_xembed_app_get_configuration(av_xembed_app_p self,
											av_xembed_app_config_p config)
{	
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	config->pid = ctx->config.pid;
	config->window = ctx->config.window;
}

static av_result_t av_xembed_app_constructor(av_object_p pobject)
{
	av_result_t rc;
	av_xembed_app_p self = (av_xembed_app_p)pobject;
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)malloc(sizeof(av_xembed_app_ctx_t));

	memset(ctx, 0, sizeof(av_xembed_app_ctx_t));
	
	if (AV_OK != (rc = av_torb_service_addref("xembed", (av_service_p*)&ctx->xembed)))
	{
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = ctx->xembed->get_video_info(ctx->xembed, &ctx->xembed_video)))
	{
		av_torb_service_release("xembed");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&ctx->system)))
	{
		av_torb_service_release("xembed");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = ctx->system->get_input(ctx->system, &ctx->input)))
	{
		av_torb_service_release("system");
		av_torb_service_release("xembed");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = ctx->system->get_timer(ctx->system, &ctx->timer)))
	{
		av_torb_service_release("system");
		av_torb_service_release("xembed");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = ctx->system->get_video(ctx->system, &ctx->video)))
	{
		av_torb_service_release("system");
		av_torb_service_release("xembed");
		free(ctx);
		return rc;
	}

	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&_log)))
	{
		av_torb_service_release("system");
		av_torb_service_release("xembed");
		free(ctx);
		return rc;
	}

	ctx->video->get_backbuffer(ctx->video, &ctx->backbuffer);

	O_set_attr(self, CONTEXT, ctx);
	self->set_configuration = av_xembed_app_set_configuration;
	self->get_configuration = av_xembed_app_get_configuration;
	return AV_OK;
}

static void av_xembed_app_destructor(void* pobject)
{	
	av_xembed_app_p self = (av_xembed_app_p)pobject;
	av_xembed_app_ctx_p ctx = (av_xembed_app_ctx_p)O_context(self);
	ctx->stop_request = AV_TRUE;
	ctx->timer->sleep_ms(100);

	if (ctx->config.pid > 0)
	{
		/* terminate application */
		int exit_status;
		kill(ctx->config.pid, SIGTERM);
		waitpid(ctx->config.pid, &exit_status, 0);
		if (WIFEXITED(exit_status))
		{
			_log->info(_log, "Child process %d terminated with exit status %d",
							ctx->config.pid, WEXITSTATUS(exit_status));
		}
	}

	av_torb_service_release("log");
	av_torb_service_release("xembed");
	av_torb_service_release("system");
	free(ctx);
}

av_result_t av_xembed_app_register_class()
{
	av_result_t rc;

	if (AV_OK != (rc = av_torb_register_class("xembed_app", AV_NULL,
											  sizeof(av_xembed_app_t),
											  av_xembed_app_constructor,
											  av_xembed_app_destructor)))
		return rc;

	return AV_OK;
}
