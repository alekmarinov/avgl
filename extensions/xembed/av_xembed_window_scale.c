/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_xembed_window.c                                 */
/*                                                                   */
/*********************************************************************/

#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <av_log.h>
#include <av_list.h>
#include <av_prefs.h>
#include <av_system.h>
#include <av_graphics.h>
#include <av_xembed.h>
#include "av_xembed_window.h"
#include "av_xembed_surface.h"

/* FIXME: av_scaler is horribly dependeny by ffmpeg objects */
/* Refactore av_scaler and avoid the dependency below */
#include <ffmpeg/swscale.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONTEXT "xembed_window_ctx"
#define O_context(o) O_attr(o, CONTEXT)

#define SCLOG2 2

#define SCALE_MIN 0
#define SCALE_MAX 0.89
#define MOUSE_EVENTS_SKIP 2

#define SET_IN_RANGE(r, v, n, x) \
	if (v < n) r = n;            \
	else if (v > x) r = x;       \
	else r = v;

static av_log_p _log = AV_NULL;
static av_prefs_p _prefs = AV_NULL;


typedef struct av_xembed_window_ctx
{
	av_system_p system;
	av_video_p video;
	av_window_p window;
	av_xembed_p xembed;
	av_xembed_window_p self;
	av_video_overlay_p overlay;
	av_video_surface_p surface;
	av_video_surface_p back_buffer;
	av_bool_t surface_locked;
	av_pixel_p rgbbuf;
	int rgbpitch;
	int prev_mouse_x;
	int prev_mouse_y;
	av_list_p dmg_rects;
	av_bool_t mouse_left_down;
	av_bool_t mouse_right_down;
	int mouse_down_x;
	int mouse_down_y;
	double scale_start;
	unsigned int offset_start_x;
	unsigned int offset_start_y;
	enum {
		MOVE_MODE_NORMAL = 0,
		MOVE_MODE_SCALE,
		MOVE_MODE_SCROLL,
	} move_mode;
	int event_skip_counter;
	unsigned int sw;
	unsigned int sh;
	unsigned int scale;
	unsigned int offsetx;
	unsigned int offsety;
	unsigned int owmin, owmax;
	unsigned int ohmin, ohmax;
	unsigned int owmaxmin, ohmaxmin;
	av_bool_t isshow;
	int gridsize;
	int scaledgridsize;

	/* preferences */
	int swscale_flags;
	enum {
		XEMBED_SCALER_NATIVE,
		XEMBED_SCALER_SWSCALE
	} scaler;
	int xmouse;
	int useoverlay;
	int overlayformat;
	int swscaleformat;
	int fullscale;
} av_xembed_window_ctx_t, *av_xembed_window_ctx_p;

static void av_xembed_window_get_max_offsets(av_xembed_window_ctx_p ctx,
											 unsigned int* max_offsetx, unsigned int* max_offsety)
{
	unsigned int ow = ctx->owmin + (((0x10000-ctx->scale) * ctx->owmaxmin) >> 16);
	unsigned int oh = ctx->ohmin + (((0x10000-ctx->scale) * ctx->ohmaxmin) >> 16);
	*max_offsetx = ctx->sw - ow;
	*max_offsety = ctx->sh - oh;
}

static void av_xembed_window_get_src_rect(av_xembed_window_ctx_p ctx, av_rect_p srcrect)
{
	unsigned int maxoffsx;
	unsigned int maxoffsy;
	av_xembed_window_get_max_offsets(ctx, &maxoffsx, &maxoffsy);
	if (ctx->offsetx > maxoffsx) ctx->offsetx = maxoffsx;
	if (ctx->offsety > maxoffsy) ctx->offsety = maxoffsy;

	av_rect_init(srcrect, ctx->offsetx, ctx->offsety, ctx->sw - maxoffsx, ctx->sh - maxoffsy);
}

static void av_xembed_window_rgb2yuv(av_xembed_window_ctx_p ctx,
									 const av_pix_p* yuvdst, int* dlinesize,
									 av_pixel_p rgbsrc, int slinesize,
									 av_rect_p rect)
{
	unsigned char* srcpix[4];
	unsigned char* dstpix[1];
	int srcpitch[4];
	int dstpitch[1];
	av_rect_t crect;
	struct SwsContext *sws;

	av_rect_copy(&crect, rect);

	if (crect.x % 2 == 1)
	{
		crect.x--;
		crect.w++;
	}

	if (crect.w < 8)
	{
		crect.w = 8;
	}
	else
	{
		crect.w += (7 - (crect.w & 7));
	}

	if (crect.x < 0)
		crect.x = 0;

	if (crect.y < 0)
		crect.y = 0;

	sws = sws_getContext(crect.w, crect.h, PIX_FMT_RGB32,
						 crect.w, crect.h, ctx->swscaleformat,
						 ctx->swscale_flags,
						 0, 0, 0);

	srcpix[0] = (unsigned char*)rgbsrc + 4*crect.x + crect.y*slinesize;
	srcpix[3] = srcpix[2] = srcpix[1] = srcpix[0];
	srcpitch[3] = srcpitch[2] = srcpitch[1] = srcpitch[0] = slinesize;

	dstpix[0] = yuvdst[0] + 2*(crect.x) + crect.y*dlinesize[0];
	dstpitch[0] = dlinesize[0];
	
	_log->debug(_log, "av_xembed_window:rgb2yuv/swscale (%d %d %d %d)",
		crect.x, crect.y, crect.w, crect.h);
	sws_scale(sws, srcpix, srcpitch, 0, crect.h, dstpix, dstpitch);
	sws_freeContext(sws);
}

static void av_xembed_window_update_overlay(av_xembed_window_ctx_p ctx, av_rect_p rect)
{
	const av_pix_p* yuvbuf;
	const short int* yuvpitch;
	int yuvplanes;
	av_result_t rc;
	if (AV_OK != (rc = ctx->overlay->lock(ctx->overlay, &yuvplanes, &yuvbuf, &yuvpitch)))
	{
		_log->error(_log, "Unable to lock overlay (rc=%d)\n", rc);
	}
	else
	{
		int yuvpitch4[4];
		yuvpitch4[0] = yuvpitch[0];
		yuvpitch4[1] = yuvpitch[1];
		yuvpitch4[2] = yuvpitch[2];
		yuvpitch4[3] = 0;
		av_xembed_window_rgb2yuv(ctx, yuvbuf, yuvpitch4, ctx->rgbbuf, ctx->rgbpitch, rect);
		ctx->overlay->unlock(ctx->overlay);
	}
}

static av_bool_t av_xembed_window_on_update_overlay(av_window_p self, av_video_surface_p video_surface, av_rect_p rect)

{
	av_result_t rc;
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	AV_UNUSED(video_surface);
	
	if (ctx->overlay)
	{
		av_rect_t scale_rect;
		av_rect_t absrect;
		self->get_rect(self, &absrect);
		absrect.x = absrect.y = 0;
		self->rect_absolute(self, &absrect);

		/* FIXME: scale mode */
		av_rect_init(&scale_rect, 0, 0, absrect.w, absrect.h);
		
		av_xembed_window_update_overlay(ctx, rect);

		if (AV_OK != (rc = ctx->overlay->blit(ctx->overlay, &scale_rect)))
		{
			_log->error(_log, "Unable to blit overlay (rc=%d)", rc);
			return AV_FALSE;
		}

		absrect.w = rect->w;
		absrect.h = rect->h;
		if (AV_OK != (rc = ctx->overlay->blit_back(ctx->overlay, rect, &absrect)))
		{
			_log->error(_log, "Unable to blit overlay back surface (rc=%d)", rc);
			return AV_FALSE;
		}
	}
	return AV_TRUE;
}

static void av_xembed_window_rgb2rgb(av_xembed_window_ctx_p ctx,
									 av_pixel_p rgbdst, int dlinesize, av_rect_p dstrect,
									 av_pixel_p rgbsrc, int slinesize, av_rect_p srcrect)
{
	unsigned char* srcpix[4];
	unsigned char* dstpix[4];
	int srcpitch[4];
	int dstpitch[4];
	struct SwsContext *sws;
	srcpix[0] = (unsigned char*)rgbsrc + 4*srcrect->x + srcrect->y*slinesize;
	srcpix[3] = srcpix[2] = srcpix[1] = srcpix[0];
	srcpitch[3] = srcpitch[2] = srcpitch[1] = srcpitch[0] = slinesize;
	dstpix[0] = (unsigned char*)rgbdst + 4*dstrect->x + dstrect->y*dlinesize;
	dstpix[3] = dstpix[2] = dstpix[1] = dstpix[0];
	dstpitch[3] = dstpitch[2] = dstpitch[1] = dstpitch[0] = dlinesize;
	
	sws = sws_getContext(srcrect->w, srcrect->h, PIX_FMT_RGB32,
						 dstrect->w, dstrect->h, PIX_FMT_RGB32,
						 ctx->swscale_flags, 0, 0, 0);
	_log->debug(_log, "av_xembed_window:rgb2rgb/swscale (%d %d %d %d) -> (%d %d %d %d)",
		srcrect->x, srcrect->y, srcrect->w, srcrect->h,
		dstrect->x, dstrect->y, dstrect->w, dstrect->h);
	sws_scale(sws, srcpix, srcpitch, 0, srcrect->h, dstpix, dstpitch);
	sws_freeContext(sws);
}

static void av_xembed_window_update_grid(av_xembed_window_ctx_p ctx, av_pixel_p backbuf, int pitch,
										 av_rect_p arect, av_rect_p dstrect, av_rect_p srcrect)
{
	int si, sj, di, dj;
	int qx, qy, qw, qh;
	int remainder;
	av_rect_t srect;
	av_rect_t drect;

	remainder = srcrect->x % ctx->gridsize;
	srect.x = srcrect->x - remainder;
	srect.w = srcrect->w + remainder;
	remainder = srect.w % ctx->gridsize;
	if (remainder > 0)
		srect.w += (ctx->gridsize - remainder);
	/* FIXME: check srect.w < MAX_SRC_WIDTH */
	
	remainder = srcrect->y % ctx->gridsize;
	srect.y = srcrect->y - remainder;
	srect.h = srcrect->h + remainder;
	remainder = srect.h % ctx->gridsize;
	if (remainder > 0)
		srect.h += (ctx->gridsize - remainder);
	/* FIXME: check srect.h < MAX_SRC_HEIGHT */
	
	remainder = dstrect->x % ctx->scaledgridsize;
	drect.x = dstrect->x - remainder;
	drect.w = dstrect->w + remainder;
	remainder = drect.w % ctx->scaledgridsize;
	if (remainder > 0)
		drect.w += (ctx->scaledgridsize - remainder);
	/* FIXME: check drect.w < MAX_DST_WIDTH */

	remainder = dstrect->y % ctx->scaledgridsize;
	drect.y = dstrect->y - remainder;
	drect.h = dstrect->h + remainder;
	remainder = drect.h % ctx->scaledgridsize;
	if (remainder > 0)
		drect.h += (ctx->scaledgridsize - remainder);
	/* FIXME: check drect.w < MAX_DST_HEIGHT */

	qx = drect.x / ctx->scaledgridsize;
	qy = drect.y / ctx->scaledgridsize;
	qw = drect.w / ctx->scaledgridsize;
	qh = drect.h / ctx->scaledgridsize;

	srect.x = qx * ctx->gridsize;
	srect.y = qy * ctx->gridsize;
	srect.w = qw * ctx->gridsize;
	srect.h = qh * ctx->gridsize;

	for (si=srect.y, di = drect.y; si<srect.y+srect.h && di<drect.y+drect.h; si+=ctx->gridsize, di+=ctx->scaledgridsize)
	for (sj=srect.x, dj = drect.x; sj<srect.x+srect.w && dj<drect.x+drect.w; sj+=ctx->gridsize, dj+=ctx->scaledgridsize)
	{
		av_rect_t adrect;
		av_rect_t asrect;
		int sgw = ctx->gridsize;
		int sgh = ctx->gridsize;
		int dgw = ctx->scaledgridsize;
		int dgh = ctx->scaledgridsize;
		if (sj+sgw > (int)ctx->sw) sgw = ctx->sw - sj;
		if (si+sgh > (int)ctx->sh) sgh = ctx->sh - si;
		if (dj+dgw > arect->w) dgw = arect->w - dj;
		if (di+dgh > arect->h) dgh = arect->h - di;

		adrect.x = dj + arect->x;
		adrect.y = di + arect->y;
		adrect.w = dgw;
		adrect.h = dgh;

		asrect.x = sj;
		asrect.y = si;
		asrect.w = sgw;
		asrect.h = sgh;

		if (XEMBED_SCALER_SWSCALE == ctx->scaler)
		{
			av_xembed_window_rgb2rgb(ctx, backbuf, pitch, &adrect, ctx->rgbbuf, ctx->rgbpitch, &asrect);
		}
		else
		{
			ctx->back_buffer->blit(ctx->back_buffer, &adrect, (av_surface_p)ctx->surface, &asrect);
		}
	}
}

static void av_xembed_window_update_rgb(av_xembed_window_ctx_p ctx, av_video_surface_p video_surface, av_rect_p dstrect)
{
/*	av_result_t rc;*/
	av_pixel_p backbuf;
	int pitch;

/*	if (AV_OK != (rc = ctx->back_buffer->lock(ctx->back_buffer, AV_SURFACE_LOCK_READ,
											&backbuf, &pitch)))
	{
		_log->error(_log, "Unable to lock back buffer surface (rc=%d)", rc);
	}
	else*/
	{
		av_rect_t arect;
		av_rect_t srect;
		ctx->window->get_rect(ctx->window, &arect);
		arect.x = arect.y = 0;
		ctx->window->rect_absolute(ctx->window, &arect);

		av_xembed_window_get_src_rect(ctx, &srect);
		
		dstrect->x -= arect.x;
		dstrect->y -= arect.y;

		srect.x += (srect.w * dstrect->x) / ctx->sw;
		srect.y += (srect.h * dstrect->y) / ctx->sh;
		srect.w = (srect.w * dstrect->w) / ctx->sw;
		srect.h = (srect.h * dstrect->h) / ctx->sh;

		if (srect.w>0 && srect.h>0 && arect.w>0 && arect.h>0)
		{
			if (ctx->gridsize > 1)
				av_xembed_window_update_grid(ctx, backbuf, pitch, &arect, dstrect, &srect);
			else
			{
				arect.x += dstrect->x;
				arect.y += dstrect->y;
				arect.w = dstrect->w;
				arect.h = dstrect->h;

				_log->debug(_log, "av_xembed_window:rgb2rgb/native (%d %d %d %d) -> (%d %d %d %d)",
					srect.x, srect.y, srect.w, srect.h,
					arect.x, arect.y, arect.w, arect.h);

				if (XEMBED_SCALER_SWSCALE == ctx->scaler)
				{
					av_xembed_window_rgb2rgb(ctx, backbuf, pitch, &arect, ctx->rgbbuf, ctx->rgbpitch, &srect);
				}
				else
				{
					video_surface->blit(video_surface, &arect, (av_surface_p)ctx->surface,  &srect);
				}
			}
		}

/*		ctx->back_buffer->unlock(ctx->back_buffer);*/
	}
}

static av_bool_t av_xembed_window_on_update_rgb32(av_window_p self, av_video_surface_p video_surface, av_rect_p rect)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	av_xembed_window_update_rgb(ctx, video_surface, rect);
	return AV_TRUE;
}

static void av_xembed_window_xembed_on_damage(av_xembed_p self, av_rect_p dmgrect)
{
	av_bool_t intersected = AV_FALSE;
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);

	for (ctx->dmg_rects->first(ctx->dmg_rects);
		ctx->dmg_rects->has_more(ctx->dmg_rects);
		ctx->dmg_rects->next(ctx->dmg_rects))
	{
		av_rect_p dmgr = (av_rect_p)ctx->dmg_rects->get(ctx->dmg_rects);
		av_rect_t irect;
		if (av_rect_intersect(dmgr, dmgrect, &irect))
		{
			av_rect_extend(dmgr, dmgrect);
			intersected = AV_TRUE;
		}
	}

	if (!intersected)
	{
		av_rect_p newrect = (av_rect_p)malloc(sizeof(av_rect_t));
		av_rect_copy(newrect, dmgrect);
		ctx->dmg_rects->push_last(ctx->dmg_rects, newrect);
	}
}

static av_bool_t av_xembed_window_invalidate_src_rect(av_xembed_window_ctx_p ctx, av_rect_p rect)
{
	av_rect_t arect;
	av_rect_t srect;
	av_rect_t scaled;
	av_rect_t irect;

	av_xembed_window_get_src_rect(ctx, &srect);
	if (av_rect_intersect(rect, &srect, &irect))
	{
		scaled.x = ((irect.x - srect.x) * ctx->sw) / srect.w;
		scaled.y = ((irect.y - srect.y) * ctx->sh) / srect.h;
		scaled.w = (irect.w * ctx->sw) / srect.w;
		scaled.h = (irect.h * ctx->sh) / srect.h;
		
		ctx->window->get_rect(ctx->window, &arect);
		arect.x = arect.y = 0;
		ctx->window->rect_absolute(ctx->window, &arect);
		
		scaled.x += arect.x;
		scaled.y += arect.y;

		ctx->system->invalidate_rect(ctx->system, &scaled);
		return AV_TRUE;
	}
	return AV_FALSE;
}

static void av_xembed_window_invalidate_mouse(av_xembed_window_ctx_p ctx, int x, int y)
{
	av_rect_t mrect;
	int mrad = 40; //40 + (40*(ctx->scale) >> 16);

	av_rect_init(&mrect, x - mrad, y - mrad, 2*mrad, 2*mrad);
	ctx->system->invalidate_rect(ctx->system, &mrect);
}

static av_bool_t av_xembed_window_on_mouse_enter(av_window_p self, int x, int y)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	if (ctx->xmouse)
	{
		_log->info(_log, "xembed_window hide cursor");
		ctx->video->set_cursor_visible(ctx->video, 0);
	}
	if (ctx->self->on_mouse_event)
		ctx->self->on_mouse_event(ctx->self, "enter", x, y);
	return AV_TRUE;
}

static av_bool_t av_xembed_window_on_mouse_leave(av_window_p self, int x, int y)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);	
	if (ctx->xmouse)
	{
		_log->info(_log, "xembed_window show cursor");
		ctx->video->set_cursor_visible(ctx->video, 1);
		ctx->video->set_mouse_position(ctx->video, x, y);
	}
	if (ctx->self->on_mouse_event)
		ctx->self->on_mouse_event(ctx->self, "leave", x, y);
	return AV_TRUE;
}

static av_bool_t av_xembed_window_on_mouse_move(av_window_p self, int x, int y)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	av_rect_t srect;
	av_rect_t arect;
	int ox = x;
	int oy = y;

	ctx->event_skip_counter++;
	if (ctx->event_skip_counter < MOUSE_EVENTS_SKIP)
	{
		return AV_TRUE;
	}
	ctx->event_skip_counter = 0;

	if (MOVE_MODE_SCALE == ctx->move_mode)
	{
		int delta = ctx->mouse_down_y - y;
		av_rect_t srcrect;
		double scale_offset = (double)delta / 512; /* FIXME: Why 512? */
		double scale = ctx->scale_start + scale_offset;

		av_xembed_window_get_src_rect(ctx, &srcrect);

/*		unsigned int offsetx = (ctx->offset_start_x * ctx->sw) / srcrect.w;
		unsigned int offsety = (ctx->offset_start_y * ctx->sh) / srcrect.h;*/

		//ctx->self->scale(ctx->self, scale, -1, -1);
	}
	else
	//if (MOVE_MODE_SCROLL == ctx->move_mode || MOVE_MODE_NORMAL == ctx->move_mode )
	{
		av_rect_t srcrect;
		int deltax = x - ctx->mouse_down_x;
		int deltay = y - ctx->mouse_down_y;

		av_xembed_window_get_src_rect(ctx, &srcrect);
		unsigned int offsetx = (ctx->offset_start_x * ctx->sw) / srcrect.w;
		unsigned int offsety = (ctx->offset_start_y * ctx->sh) / srcrect.h;
		//ctx->self->scale(ctx->self, -1, offsetx - deltax, offsety - deltay);

		if (!ctx->xmouse)
		{
			ctx->xembed->send_mouse_move(ctx->xembed, 5000, 5000);
		}
		else
		if ((ctx->mouse_left_down && !ctx->mouse_right_down) || ctx->xmouse)
		{
			ctx->window->get_rect(ctx->window, &arect);
			arect.x = arect.y = 0;
			ctx->window->rect_absolute(ctx->window, &arect);
		
			av_xembed_window_invalidate_mouse(ctx, ctx->prev_mouse_x, ctx->prev_mouse_y);
			av_xembed_window_invalidate_mouse(ctx, x, y);
	
			ctx->prev_mouse_x = x;
			ctx->prev_mouse_y = y;
		
			x -= arect.x;
			y -= arect.y;
			
			av_xembed_window_get_src_rect(ctx, &srect);
			x = srect.x + (x * srect.w) / ctx->sw;
			y = srect.y + (y * srect.h) / ctx->sh;
			ctx->xembed->send_mouse_move(ctx->xembed, x, y);
		}
	}
	if (ctx->self->on_mouse_event)
		ctx->self->on_mouse_event(ctx->self, "move", ox, oy);
	return AV_TRUE;
}

static av_bool_t av_xembed_window_on_mouse_button_down(av_window_p self, av_event_mouse_button_t button,
													   int x, int y)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	const char* eventname = AV_NULL;
	printf("on_mouse_button_down -> %d %d %d wh=%d\n", button, x, y, AV_MOUSE_BUTTON_WHEEL);

	/* Emulates middle button */
	if ((button == AV_MOUSE_BUTTON_RIGHT && ctx->mouse_left_down) ||
		(button == AV_MOUSE_BUTTON_LEFT && ctx->mouse_right_down))
	{
		button = AV_MOUSE_BUTTON_MIDDLE;
	}

	switch (button)
	{
		case AV_MOUSE_BUTTON_LEFT:
			ctx->mouse_left_down = AV_TRUE;
			av_xembed_window_on_mouse_move(ctx->window, x, y);
			ctx->xembed->send_mouse_button(ctx->xembed, AV_MOUSE_BUTTON_LEFT, AV_BUTTON_PRESSED);
			self->capture(self);
			eventname = "leftdown";
		break;
		case AV_MOUSE_BUTTON_RIGHT:
			ctx->mouse_right_down = AV_TRUE;
			ctx->move_mode = MOVE_MODE_SCALE;
			ctx->mouse_down_x = x;
			ctx->mouse_down_y = y;
			ctx->scale_start = ((double)ctx->scale / 0x10000);
			ctx->video->set_cursor_shape(ctx->video, AV_VIDEO_CURSOR_ZOOM);
			self->capture(self);
			eventname = "rightdown";
		break;
		case AV_MOUSE_BUTTON_MIDDLE:
			ctx->mouse_left_down = ctx->mouse_right_down = AV_TRUE;
			ctx->move_mode = MOVE_MODE_SCROLL;
			ctx->mouse_down_x = x;
			ctx->mouse_down_y = y;
			ctx->offset_start_x = ctx->offsetx;
			ctx->offset_start_y = ctx->offsety;
			ctx->video->set_cursor_shape(ctx->video, AV_VIDEO_CURSOR_SCROLL);
			self->capture(self);
			eventname = "middledown";
		break;
		case AV_MOUSE_BUTTON_WHEEL:
			printf("wheel down\n");
			ctx->xembed->send_mouse_button(ctx->xembed, AV_MOUSE_BUTTON_WHEEL, AV_BUTTON_PRESSED);
		break;
		default:
		return AV_FALSE;
	}
	if (eventname && ctx->self->on_mouse_event)
		ctx->self->on_mouse_event(ctx->self, eventname, x, y);
	return AV_TRUE;
}

static av_bool_t av_xembed_window_on_mouse_button_up(av_window_p self, av_event_mouse_button_t button,
													 int x, int y)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	const char* eventname = AV_NULL;
	printf("on_mouse_button_up -> %d %d %d wh=%d\n", button, x, y, AV_MOUSE_BUTTON_WHEEL);
// 	if (!ctx->xmouse)
// 		ctx->xembed->send_mouse_move(ctx->xembed, 5000, 5000);

	/* Emulates middle button */
	if (ctx->mouse_left_down && ctx->mouse_right_down)
	{
		button = AV_MOUSE_BUTTON_MIDDLE;
	}

	switch (button)
	{
		case AV_MOUSE_BUTTON_LEFT:
			av_xembed_window_on_mouse_move(ctx->window, x, y);
			ctx->mouse_left_down = AV_FALSE;
			ctx->xembed->send_mouse_button(ctx->xembed, AV_MOUSE_BUTTON_LEFT, AV_BUTTON_RELEASED);
			self->uncapture(self);
			eventname = "leftup";
			ctx->system->set_focus(ctx->system, self);
			if (!ctx->xmouse)
				ctx->xembed->send_mouse_move(ctx->xembed, 5000, 5000);
		break;
		case AV_MOUSE_BUTTON_RIGHT:
			ctx->mouse_right_down = AV_FALSE;
			ctx->move_mode = MOVE_MODE_NORMAL;
			self->uncapture(self);
			ctx->video->set_cursor_shape(ctx->video, AV_VIDEO_CURSOR_DEFAULT);
			eventname = "rightup";
		break;
		case AV_MOUSE_BUTTON_MIDDLE:
			ctx->mouse_left_down = ctx->mouse_right_down = AV_FALSE;
			ctx->move_mode = MOVE_MODE_NORMAL;
			self->uncapture(self);
			ctx->video->set_cursor_shape(ctx->video, AV_VIDEO_CURSOR_DEFAULT);
			eventname = "middleup";
		break;
		case AV_MOUSE_BUTTON_WHEEL:
			printf("wheel up\n");
			ctx->xembed->send_mouse_button(ctx->xembed, AV_MOUSE_BUTTON_WHEEL, AV_BUTTON_RELEASED);
		break;
		default:
		return AV_FALSE;
	}
	if (eventname && ctx->self->on_mouse_event)
		ctx->self->on_mouse_event(ctx->self, eventname, x, y);
	return AV_TRUE;
}

static av_bool_t av_xembed_window_on_key_down(av_window_p self, av_key_t key)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	av_input_p input;
	ctx->system->get_input(ctx->system, &input);
	ctx->self->send_key(ctx->self, key, input->get_key_modifiers(input), AV_BUTTON_PRESSED);
	return AV_TRUE;
}

static av_bool_t av_xembed_window_on_key_up(av_window_p self, av_key_t key)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	av_input_p input;
	ctx->system->get_input(ctx->system, &input);
	ctx->self->send_key(ctx->self, key, input->get_key_modifiers(input), AV_BUTTON_RELEASED);
	return AV_TRUE;
}

static av_result_t av_xembed_window_open(av_xembed_window_p self,
										 av_window_p parent,
										 av_rect_p rect)
{
	av_result_t rc;
	av_xembed_video_t video_info;
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);

	if (AV_OK != (rc = ctx->system->get_video(ctx->system, &ctx->video)))
	{
		_log->error(_log, "Unable to obtain video object (rc=%d)", rc);
		return rc;
	}

	if (AV_OK != (rc = ctx->xembed->open(ctx->xembed, rect->w, rect->h)))
	{
		_log->error(_log, "Unable to create xembed object (rc=%d)", rc);
		return rc;
	}

	if (!ctx->xmouse)
		ctx->xembed->send_mouse_move(ctx->xembed, 5000, 5000);

	O_set_attr(ctx->xembed, CONTEXT, ctx);
	ctx->xembed->on_damage = av_xembed_window_xembed_on_damage;

	if (AV_OK != (rc = ctx->system->create_window(ctx->system, parent, rect, &ctx->window)))
	{
		_log->error(_log, "Unable to create window object (rc=%d)", rc);
		return rc;
	}

	O_set_attr(ctx->window, CONTEXT, ctx);

	if (ctx->useoverlay)
		ctx->window->on_update = av_xembed_window_on_update_overlay;
	else
		ctx->window->on_update = av_xembed_window_on_update_rgb32;

	ctx->window->on_mouse_enter = av_xembed_window_on_mouse_enter;
	ctx->window->on_mouse_leave = av_xembed_window_on_mouse_leave;
	ctx->window->on_mouse_move = av_xembed_window_on_mouse_move;
	ctx->window->on_mouse_button_down = av_xembed_window_on_mouse_button_down;
	ctx->window->on_mouse_button_up = av_xembed_window_on_mouse_button_up;
	ctx->window->on_key_down = av_xembed_window_on_key_down;
	ctx->window->on_key_up = av_xembed_window_on_key_up;
	
	ctx->window->set_visible(ctx->window, 0);

	if (AV_OK != (rc = ctx->xembed->get_video_info(ctx->xembed, &video_info)))
	{
		_log->error(_log, "Unable to obtain xembed video info for direct access (rc=%d)", rc);
		return rc;
	}
	
	if (AV_OK != (rc = ctx->xembed->get_video_surface(ctx->xembed, &ctx->surface)))
	{
		_log->error(_log, "Unable to obtain Xvfb surface (rc=%d)\n", rc);
		return rc;
	}

	if (AV_OK != (rc = ctx->video->get_backbuffer(ctx->video, &ctx->back_buffer)))
	{
		_log->error(_log, "Unable to obtain back buffer surface (rc=%d)\n", rc);
		return rc;
	}
	
	ctx->sw = video_info.xres;
	ctx->sh = video_info.yres;
	ctx->owmin = video_info.xres >> SCLOG2;
	ctx->owmax = video_info.xres;
	ctx->owmaxmin = ctx->owmax - ctx->owmin;
	ctx->ohmin = video_info.yres >> SCLOG2;
	ctx->ohmax = video_info.yres;
	ctx->ohmaxmin = ctx->ohmax - ctx->ohmin;

	return AV_OK;
}

static av_result_t av_xembed_window_show(av_xembed_window_p self)
{
	av_result_t rc;
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);

	if (ctx->useoverlay)
	{
		if (!ctx->overlay)
		{
	
			if (AV_OK != (rc = ctx->video->create_overlay_buffered(ctx->video, &ctx->overlay)))
			{
				_log->error(_log, "Unable to create buffered video overlay (rc=%d)", rc);
				return rc;
			}
		
		
			if (AV_OK != (rc = ctx->overlay->set_size_format(ctx->overlay,
										ctx->sw, ctx->sh,
										ctx->overlayformat)))
			{
				_log->error(_log, "Unable to set overlay size/format (rc=%d)", rc);
				return rc;
			}
		}
	}

	if (AV_OK != (rc = ctx->surface->lock(ctx->surface, AV_SURFACE_LOCK_READ,
											&ctx->rgbbuf, &ctx->rgbpitch)))
	{
		_log->error(_log, "Unable to lock Xvfb surface (rc=%d)\n", rc);
		return rc;
	}
	ctx->surface_locked = 1;

	ctx->window->set_visible(ctx->window, 1);
	ctx->isshow = AV_TRUE;
	return AV_OK;
}

static void av_xembed_window_hide(av_xembed_window_p self)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	if (ctx->overlay)
	{
		O_destroy(ctx->overlay);
		ctx->overlay = 0;
	}

	if (ctx->surface)
	{
		if (ctx->surface_locked)
		{
			ctx->surface->unlock(ctx->surface);
			ctx->surface_locked = 0;
		}
	}

	ctx->window->set_visible(ctx->window, 0);
	ctx->isshow = AV_FALSE;
}

static void av_xembed_window_set_scale_start(av_xembed_window_p self, double scale_start)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	SET_IN_RANGE(scale_start, scale_start, SCALE_MIN, SCALE_MAX)
	ctx->scale_start = scale_start;
}

static void av_xembed_window_scale(av_xembed_window_p self, double scale, int offsetx, int offsety)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	av_rect_t srcrect;
	if (scale != -1)
	{
		/* int scale_width, scale_height; */

		/* scale */
		SET_IN_RANGE(scale, scale, SCALE_MIN, SCALE_MAX)
		ctx->scale = (unsigned int)(scale * 0x10000);
		av_xembed_window_get_src_rect(ctx, &srcrect);
		ctx->scaledgridsize = (ctx->gridsize * ctx->sw) / srcrect.w;
		
/*		offsetx = ctx->offset_start_x + ((new_scale_width - scale_width) >> 1);
		offsety = ctx->offset_start_y + ((new_scale_height - scale_height) >> 1);*/
/*		offsetx = (ctx->sw - srcrect.w) >> 0;
		offsety = (ctx->sh - srcrect.h) >> 0;*/
		//printf("%g %d %d %d %d\n", (double)ctx->sw / srcrect.w, srcrect.w, srcrect.h, offsetx, offsety);
		
	}
	
	if (offsetx != -1 && offsety != -1)
	{
		/* offsets */
		unsigned int maxoffsx, maxoffsy;
		av_xembed_window_get_max_offsets(ctx, &maxoffsx, &maxoffsy);
		av_xembed_window_get_src_rect(ctx, &srcrect);
		maxoffsx = (maxoffsx * ctx->sw) / srcrect.w;
		maxoffsy = (maxoffsy * ctx->sh) / srcrect.h;
		SET_IN_RANGE(offsetx, offsetx, 0, (int)maxoffsx)
		SET_IN_RANGE(offsety, offsety, 0, (int)maxoffsy)
		offsetx = (offsetx * srcrect.w) / ctx->sw;
		offsety = (offsety * srcrect.h) / ctx->sh;
		ctx->offsetx = (unsigned int)offsetx;
		ctx->offsety = (unsigned int)offsety;
	}
	/* FIXME: Refector this */
	ctx->window->update(ctx->window, AV_UPDATE_INVALIDATE);
}

static void av_xembed_window_idle(av_xembed_window_p self)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	if (ctx->isshow)
	{
		ctx->xembed->idle(ctx->xembed);
		if (ctx->fullscale)
		{
			/* FIXME: Refector this */
			ctx->window->update(ctx->window, AV_UPDATE_INVALIDATE);
		}
		else
		{
			for (ctx->dmg_rects->first(ctx->dmg_rects);
				ctx->dmg_rects->has_more(ctx->dmg_rects);
				ctx->dmg_rects->next(ctx->dmg_rects))
			{
				av_rect_p dmgr = (av_rect_p)ctx->dmg_rects->get(ctx->dmg_rects);
				av_xembed_window_invalidate_src_rect(ctx, dmgr);
			}
		}
		ctx->dmg_rects->remove_all(ctx->dmg_rects, free);
	}
}

static double av_xembed_window_get_scale(av_xembed_window_p self)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	return ((double)ctx->scale / 0x10000);
}

static void av_xembed_window_get_offsets(av_xembed_window_p self, int* poffsetx, int* poffsety)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	*poffsetx = (int)ctx->offsetx;
	*poffsety = (int)ctx->offsety;
}

static av_result_t av_xembed_window_screenshot(av_xembed_window_p self, const char* filename)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	return ctx->xembed->screenshot(ctx->xembed, filename);
}

static void av_xembed_window_send_key(av_xembed_window_p self, av_key_t key,
									  av_keymod_t modifiers, av_event_button_status_t status)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	ctx->xembed->send_key(ctx->xembed, key, modifiers, status);
}

static void av_xembed_window_get_window(av_xembed_window_p self, av_window_p* ppwindow)
{
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);
	*ppwindow = ctx->window;
}

static av_result_t av_xembed_window_constructor(av_object_p pobject)
{
	av_result_t rc;
	av_xembed_window_p self = (av_xembed_window_p)pobject;
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)malloc(sizeof(av_xembed_window_ctx_t));
	memset(ctx, 0, sizeof(av_xembed_window_ctx_t));
	
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

	if (AV_OK != (rc = av_torb_service_addref("xembed",
											 (av_service_p*)&ctx->xembed)))
	{
		av_torb_service_release("system");
		av_torb_service_release("prefs");
		av_torb_service_release("log");
		free(ctx);
		return rc;
	}
	
	if (AV_OK != (rc = av_list_create(&ctx->dmg_rects)))
	{
		av_torb_service_release("xembed");
		av_torb_service_release("system");
		av_torb_service_release("prefs");
		av_torb_service_release("log");
		free(ctx);
		return rc;
	}
	
	ctx->self = self;
	ctx->scale = 0;
	{
		/* load preference */

		char msgbuf[255];
		const char* svalue;
		int nvalue;

		ctx->scaler = XEMBED_SCALER_NATIVE;
		if (AV_OK == _prefs->get_string(_prefs, "extensions.xembed.window.scale.method", "native", &svalue))
		{
			if (!strcasecmp(svalue, "swscale"))
			{
				ctx->scaler = XEMBED_SCALER_SWSCALE;
				ctx->swscale_flags = 0;
			}
		}

		if (XEMBED_SCALER_SWSCALE == ctx->scaler)
			strcpy(msgbuf, "scale=swscale");
		else
			strcpy(msgbuf, "scale=native");

		if (XEMBED_SCALER_SWSCALE == ctx->scaler)
		{
			if (AV_OK == _prefs->get_int(_prefs, "extensions.xembed.window.scale.swscale.mmx", 1, &nvalue))
			{
				if (nvalue)
					ctx->swscale_flags |= SWS_CPU_CAPS_MMX;
			}

			if (ctx->swscale_flags & SWS_CPU_CAPS_MMX)
			{
				strcat(msgbuf, ",mmx");
			}

			if (AV_OK == _prefs->get_string(_prefs, "extensions.xembed.window.scale.swscale.algo",
							  "fast_bilinear", &svalue))
			{
				if (!strcasecmp("fast_bilinear", svalue))
				{
					ctx->swscale_flags |= SWS_FAST_BILINEAR;
				}
				else if (!strcasecmp("gauss", svalue))
				{
					ctx->swscale_flags |= SWS_GAUSS;
				}
				else if (!strcasecmp("spline", svalue))
				{
					ctx->swscale_flags |= SWS_SPLINE;
				}
				else if (!strcasecmp("bilinear", svalue))
				{
					ctx->swscale_flags |= SWS_BILINEAR;
				}
				else if (!strcasecmp("point", svalue))
				{
					ctx->swscale_flags |= SWS_POINT;
				}
			}
			else
			{
				ctx->swscale_flags |= SWS_FAST_BILINEAR;
			}

			if (ctx->swscale_flags & SWS_FAST_BILINEAR)
					strcat(msgbuf, ",fast_bilinear");
			else if (ctx->swscale_flags & SWS_POINT)
					strcat(msgbuf, ",point");
			else if (ctx->swscale_flags & SWS_BILINEAR)
					strcat(msgbuf, ",bilinear");
			else if (ctx->swscale_flags & SWS_SPLINE)
					strcat(msgbuf, ",spline");
			else if (ctx->swscale_flags & SWS_GAUSS)
					strcat(msgbuf, ",gauss");
		}

		_prefs->get_int(_prefs, "extensions.xembed.window.xmouse", 0, &ctx->xmouse);

		if (ctx->xmouse)
		{
			strcat(msgbuf, "; xmouse");
		}
		else
		{
			strcat(msgbuf, "; no xmouse");
		}

		_prefs->get_int(_prefs, "extensions.xembed.window.fullscale", 0, &ctx->fullscale);
		if (ctx->fullscale)
		{
			strcat(msgbuf, "; fullscale");
		}

		ctx->useoverlay = 0;
		if (AV_OK == _prefs->get_string(_prefs, "extensions.xembed.window.overlay", "no", &svalue))
		{
			if (!strcasecmp("UYVY422", svalue))
			{
				ctx->useoverlay = 1;
				ctx->overlayformat = AV_UYVY_OVERLAY;
				ctx->swscaleformat = PIX_FMT_UYVY422;
			}
			else if (!strcasecmp("YUYV422", svalue))
			{
				ctx->useoverlay = 1;
				ctx->overlayformat = AV_YUY2_OVERLAY;
				ctx->swscaleformat = PIX_FMT_YUYV422;
			}
		}

		if (PIX_FMT_UYVY422 == ctx->swscaleformat)
			strcat(msgbuf, "; overlay = UYVY422");
		else if (PIX_FMT_YUYV422 == ctx->swscaleformat)
			strcat(msgbuf, "; overlay = YUYV422");

		_prefs->get_int(_prefs, "extensions.xembed.window.gridsize", 0, &ctx->gridsize);
		if (ctx->gridsize > 0)
		{
			char numbuf[250];
			sprintf(numbuf, "; gridsize=%d", ctx->gridsize);
			strcat(msgbuf, numbuf);
		}

		ctx->scaledgridsize = ctx->gridsize;

		_log->info(_log, "xembed_window initialized (%s)", msgbuf);
	}

	O_set_attr(self, CONTEXT, ctx);

	self->open            = av_xembed_window_open;
	self->show            = av_xembed_window_show;
	self->hide            = av_xembed_window_hide;
	self->set_scale_start = av_xembed_window_set_scale_start;
	self->scale           = av_xembed_window_scale;
	self->idle            = av_xembed_window_idle;
	self->get_scale       = av_xembed_window_get_scale;
	self->get_offsets     = av_xembed_window_get_offsets;
	self->screenshot      = av_xembed_window_screenshot;
	self->send_key        = av_xembed_window_send_key;
	self->get_window      = av_xembed_window_get_window;

	return AV_OK;
}

static void av_xembed_window_destructor(void* pobject)
{
	av_xembed_window_p self = (av_xembed_window_p)pobject;
	av_xembed_window_ctx_p ctx = (av_xembed_window_ctx_p)O_context(self);

	self->hide(self);
	if (ctx->window)
	{
		O_destroy(ctx->window);
	}

	ctx->dmg_rects->remove_all(ctx->dmg_rects, free);
	ctx->dmg_rects->destroy(ctx->dmg_rects);

	free(ctx);
	
	av_torb_service_release("xembed");
	av_torb_service_release("system");
	av_torb_service_release("prefs");
	av_torb_service_release("log");
}

av_result_t av_xembed_window_register_torba(void)
{
	return av_torb_register_class("xembed_window", AV_NULL,
								  sizeof(av_xembed_window_t),
								  av_xembed_window_constructor,
								  av_xembed_window_destructor);
}

#ifdef __cplusplus
}
#endif
