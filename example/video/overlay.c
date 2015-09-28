/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      overlay.c                                          */
/* Description:   Demonstrate overlay usage                          */
/*                                                                   */
/*********************************************************************/

#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <av.h>
#include <av_log.h>
#include <av_system.h>
#include <av_video.h>
#include <av_window.h>

/* FIXME: SDL depend defines....*/
#define SDL_YV12_OVERLAY  0x32315659  /* Planar mode: Y + V + U */
#define SDL_IYUV_OVERLAY  0x56555949  /* Planar mode: Y + U + V */
#define SDL_YUY2_OVERLAY  0x32595559  /* Packed mode: Y0+U0+Y1+V0 */
#define SDL_UYVY_OVERLAY  0x59565955  /* Packed mode: U0+Y0+V0+Y1 */
#define SDL_YVYU_OVERLAY  0x55595659  /* Packed mode: Y0+V0+Y1+U0 */

#define XRES 1024
#define YRES 768

static av_log_p _log;
static av_system_p _sys;
static av_video_p _video;

static av_result_t overlay_on_paint(av_window_p self, av_rect_p wrect)
{
	av_video_overlay_p video_overlay = O_attr(self, "video_overlay");

	if (video_overlay)
	{
		av_rect_t rect;
		av_rect_copy(&rect, wrect);
		rect.x = rect.y = 0;
		self->rect_absolute(self, &rect);

		rect.x += 5;
		rect.y += 5;
		rect.w -= 10;
		rect.h -= 10;
		
		video_overlay->blit(video_overlay, AV_NULL);
		video_overlay->blit_back(video_overlay, AV_NULL, &rect);
	}
	else
	{
		_log->info(_log,"no overlay");
	}
	return AV_TRUE;
}

static av_bool_t window_on_paint(av_window_p self, av_graphics_p graphics)
{
	av_rect_t rect;
	self->get_rect(self, &rect);

	rect.x = rect.y = 0;
	graphics->rectangle(graphics, &rect);
	graphics->set_color_rgba(graphics, 0., 1., 0., 1.);
	graphics->fill(graphics, 0);

	self->get_rect(self, &rect);
	overlay_on_paint(self, &rect);

	self->get_rect(self, &rect);
	rect.x = 10;
	rect.y = rect.h - 50;
	rect.w -= 20;
	rect.h = 40;
	graphics->rectangle(graphics, &rect);
	graphics->set_color_rgba(graphics, 0., 0., 1., 1.);
	graphics->fill(graphics, 0);

	return AV_TRUE;
}

static av_bool_t window_osd_on_paint(av_window_p self, av_graphics_p graphics)
{
	av_rect_t rect;
	self->get_rect(self, &rect);

	rect.x = rect.y = 0;
	graphics->rectangle(graphics, &rect);
	graphics->set_color_rgba(graphics, .3, .5, .9, 1.);
	graphics->fill(graphics, 0);

	return AV_TRUE;
}

static av_bool_t root_on_paint(av_window_p self, av_graphics_p graphics)
{
	av_rect_t rect;
	self->get_rect(self, &rect);

	rect.x = rect.y = 0;
	graphics->rectangle(graphics, &rect);
	graphics->set_color_rgba(graphics, 1., 0., 0., 1.);
	graphics->fill(graphics, 0);

	return AV_TRUE;
}

static av_bool_t dragging = AV_FALSE;
static av_bool_t is_position_change = AV_FALSE;
static int mouse_x = 0;
static int mouse_y = 0;

static av_bool_t on_mouse_left_down(av_window_p self, int x, int y)
{
	mouse_x = x;
	mouse_y = y;
	self->capture(self);
	self->raise_top(self);
	self->update(self, AV_UPDATE_INVALIDATE);
	dragging = AV_TRUE;
	is_position_change = AV_TRUE;
	return AV_TRUE;
}

static av_bool_t on_mouse_right_down(av_window_p self, int x, int y)
{
	mouse_x = x;
	mouse_y = y;
	self->capture(self);
	dragging = AV_TRUE;
	is_position_change = AV_FALSE;
	return AV_TRUE;
}

static av_bool_t on_mouse_left_right_up(av_window_p self, int x, int y)
{
	AV_UNUSED(x);
	AV_UNUSED(y);
	if (dragging)
	{
		self->uncapture(self);
		dragging = AV_FALSE;
		return AV_TRUE;
	}
	self->update(self, AV_UPDATE_INVALIDATE);
	return AV_FALSE;
}

static av_bool_t on_mouse_button_down(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	if (AV_MOUSE_BUTTON_LEFT == button)
		return on_mouse_left_down(self, x, y);
	else if (AV_MOUSE_BUTTON_RIGHT == button)
		return on_mouse_right_down(self, x, y);
	return AV_FALSE;
}

static av_bool_t on_mouse_button_up(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	if ( (AV_MOUSE_BUTTON_LEFT == button) || (AV_MOUSE_BUTTON_RIGHT == button))
		return on_mouse_left_right_up(self, x, y);
	return AV_FALSE;
}

static av_bool_t on_mouse_move(av_window_p self, int x, int y)
{
	if (dragging)
	{
		av_rect_t rect;
		self->get_rect(self, &rect);
		if (is_position_change)
		{
			rect.x += (x - mouse_x);
			rect.y += (y - mouse_y);
		}
		else
		{
			rect.w += (x - mouse_x);
			rect.h += (y - mouse_y);
		}
		self->set_rect(self, &rect);
		self->update(self, AV_UPDATE_INVALIDATE);
		mouse_x = x;
		mouse_y = y;
		return AV_TRUE;
	}
	return AV_FALSE;
}

static int load_image(av_video_overlay_p ovr, const char* fname)
{
	int w = 1024;
	int h = 576;

	if (AV_OK == ovr->set_size_format(ovr, w, h, SDL_YV12_OVERLAY))
// 	if (AV_OK == ovr->set_size_format(ovr, w, h, SDL_IYUV_OVERLAY))
	{
		int i;
		char p[255];
		FILE* f;
		int planes;
		const short int* pitches;
		const av_pix_p* pix;

		ovr->lock(ovr, &planes, &pix, &pitches);

		_log->info(_log,"planes:%d", planes);
		for (i=0; i<planes; i++) _log->info(_log,"p[%d]:%d",i,pitches[i]);

		if (0 < sprintf(p, "%s.Y", fname))
		{
			f = fopen(p,"rb");
			if (f)
			{
				_log->info(_log,"read.Y:%d / %d", fread(pix[0], 1, pitches[0]*h, f), pitches[0]*h);
			}
			else
			{
				_log->info(_log,"Can't open image:%s", p);
				memset(pix[0], 0xFF, pitches[0]*h);
			}
		}
		if (0 < sprintf(p, "%s.V", fname))
		{
			f = fopen(p,"rb");
			if (f)
			{
				_log->info(_log,"read.V:%d / %d", fread(pix[1], 1, pitches[1]*h/2, f), pitches[2]*h/2);
			}
			else
			{
				_log->info(_log,"Can't open image:%s", p);
				memset(pix[2], 0xFF, pitches[2]*h/2);
			}
		}
		if (0 < sprintf(p, "%s.U", fname))
		{
			f = fopen(p,"rb");
			if (f)
			{
				_log->info(_log,"read.U:%d / %d", fread(pix[2], 1, pitches[2]*h/2, f), pitches[1]*h/2);
			}
			else
			{
				_log->info(_log,"Can't open image:%s", p);
				memset(pix[1], 0xFF, pitches[1]*h/2);
			}
		}
		ovr->unlock(ovr);
	}
	else
	{
	}
	return 0;
}

int main(void)
{
	av_rect_t rect;
	av_result_t rc;
	av_graphics_p graphics;
	av_video_overlay_p video_overlay;
	av_window_p root;
	av_window_p window;
	av_window_p window_osd;

	if (AV_OK != (rc = av_torb_init()))
		return rc;

	/* create SDL system */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&_sys)))
		goto err_done_torba;

	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&_log)))
		goto err_done_torba;

	if (AV_OK != (rc = _sys->get_video(_sys, &_video)))
		goto err_done_torba;

	if (AV_OK != (rc = _sys->get_graphics(_sys, &graphics)))
		goto err_done_torba;

	rect.x = 0; rect.y = 0;
	rect.w = XRES; rect.h = YRES;
	_sys->create_window(_sys, AV_NULL, &rect, AV_NULL);
	_sys->update(_sys);

	rect.x = XRES/10; rect.y = YRES/10;
	rect.w = 8*XRES/10; rect.h = 8*YRES/10;
	_sys->create_window(_sys, AV_NULL, &rect, &root);
	root->on_paint = root_on_paint;
	_sys->update(_sys);

	rect.x = (8*XRES/10)/4; rect.y = (8*YRES/10)/4;
	rect.w = 2*(8*XRES/10)/4; rect.h = 2*(8*YRES/10)/4;

	_video->create_overlay(_video, &video_overlay);
	load_image(video_overlay,"image");

	_sys->create_window(_sys, root, &rect, &window);
	O_set_attr(window, "video_overlay", video_overlay);
	
	window->on_mouse_button_down = on_mouse_button_down;
	window->on_mouse_button_up   = on_mouse_button_up;
	window->on_mouse_move       = on_mouse_move;
	window->on_paint            = window_on_paint;
	window->set_rect(window, &rect);
	_sys->update(_sys);


	rect.x = 60; rect.y = 60;
	rect.w = 2*(8*XRES/10)/4-120; rect.h = 2*(8*YRES/10)/4-120;
	_sys->create_window(_sys, window, &rect, &window_osd);
	window_osd->on_paint = window_osd_on_paint;
	window_osd->set_rect(window_osd, &rect);
	_sys->update(_sys);

	_sys->loop(_sys);

	O_destroy(video_overlay);

	av_torb_service_release("log");
	av_torb_service_release("system");

err_done_torba:
	av_torb_done();

	return rc;
}
