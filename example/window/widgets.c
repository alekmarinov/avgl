/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      widget.c                                           */
/* Description:   Demonstrate widget usage                           */
/*                                                                   */
/*********************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <av_system.h>
#include <av_window.h>

#define XRES 1024
#define YRES 768

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT 200

static av_bool_t on_paint(av_window_p self, av_graphics_p graphics)
{
	int r, g, b;
	long addr=(long)self;
	av_rect_t rect;
	self->get_rect(self, &rect);
	rect.x = rect.y = 0;
	graphics->rectangle(graphics, &rect);
	addr = (addr & 0xFFFF) ^ ((addr >> 16) & 0xFFFF);
	r=(unsigned char)(addr & 0xFF); addr>>=8;
	g=(unsigned char)(addr & 0xFF); addr>>=8;
	b=(unsigned char)(addr & 0xFF);
	graphics->set_color_rgba(graphics, (double)r/255, (double)g/255, (double)b/255, 0.8);
	//graphics->set_color_rgba(graphics, 0.5, 0, 1, 0.3);
	graphics->fill(graphics, 0);
	return AV_TRUE;
}

static av_bool_t dragging = AV_FALSE;
static av_bool_t is_position_change = AV_FALSE;
static int mouse_x = 0;
static int mouse_y = 0;

static av_bool_t on_mouse_button_down(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	printf("%p button down\n", self);
	if (AV_MOUSE_BUTTON_LEFT == button )
	{
		mouse_x = x;
		mouse_y = y;
		self->capture(self);
		self->raise_top(self);
		dragging = AV_TRUE;
		is_position_change = AV_TRUE;
		return AV_TRUE;
	}
	else if (AV_MOUSE_BUTTON_RIGHT == button )
	{
		mouse_x = x;
		mouse_y = y;
		self->capture(self);
		dragging = AV_TRUE;
		is_position_change = AV_FALSE;
		return AV_TRUE;
	}
	else if (AV_MOUSE_BUTTON_WHEEL == button)
	{
/*		self->origin_y ++;
		self->update(self, AV_UPDATE_INVALIDATE);*/
	}
	return AV_FALSE;
}

static av_bool_t on_mouse_button_up(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	printf("%p button up\n", self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	if ( (AV_MOUSE_BUTTON_LEFT == button) || (AV_MOUSE_BUTTON_RIGHT == button))
	{
		if (dragging)
		{
			self->uncapture(self);
			dragging = AV_FALSE;
			return AV_TRUE;
		}
	}
	else if (AV_MOUSE_BUTTON_WHEEL == button)
	{
/*		self->origin_y--;
		self->update(self, AV_UPDATE_INVALIDATE);*/
	}
	return AV_FALSE;
}

static av_bool_t on_mouse_enter(av_window_p self, int x, int y)
{
	printf("%p mouse enter %d %d\n", self, x, y);
	return AV_TRUE;
}

static av_bool_t on_mouse_leave(av_window_p self, int x, int y)
{
	printf("%p mouse leave %d %d\n", self, x, y);
	return AV_TRUE;
}

static av_bool_t on_mouse_hover(av_window_p self, int x, int y)
{
	printf("%p mouse hover %d %d\n", self, x, y);
	return AV_TRUE;
}

static av_bool_t on_root_button_down(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	self->origin_y++;
	self->update(self, AV_UPDATE_INVALIDATE);
	return AV_FALSE;
}

static av_bool_t on_root_button_up(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	self->origin_y--;
	self->update(self, AV_UPDATE_INVALIDATE);
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
		mouse_x = x;
		mouse_y = y;
		return AV_TRUE;
	}
	return AV_FALSE;
}

av_window_p new_window(av_system_p sys, av_window_p parent, int x, int y, int w, int h)
{
	av_window_p window;
	av_rect_t rect;
	av_rect_init(&rect, x, y, w, h);
	
	sys->create_window(sys, parent, &rect, &window);
	window->on_paint             = on_paint;
	window->on_mouse_button_down = on_mouse_button_down;
	window->on_mouse_button_up   = on_mouse_button_up;
	window->on_mouse_enter       = on_mouse_enter;
	window->on_mouse_hover       = on_mouse_hover;
	return window;
}

int main(void)
{
	av_rect_t rect;
	av_result_t rc;
	av_system_p sys;
	av_window_p window;
	av_window_p root;
	int i;

	if (AV_OK != (rc = av_torb_init()))
		return rc;

	/* create SDL system */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
		goto err_done_torba;

	// root
	sys->create_window(sys, AV_NULL, AV_NULL, &root);
	
// 	root->on_mouse_button_down = on_root_button_down;
// 	root->on_mouse_button_up   = on_root_button_up;


	// window
// 	av_rect_init(&rect, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
// 	sys->create_window(sys, AV_NULL, &rect, &window);
// 	window->on_paint             = on_paint;
// 	window->on_mouse_button_down = on_mouse_button_down;
// 	window->on_mouse_button_up   = on_mouse_button_up;
// 	window->on_mouse_move        = on_mouse_move;
// 	window->update(window, AV_UPDATE_REPAINT);

	for (i=0; i<2; i++)
	{
		sys->create_window(sys, AV_NULL, AV_NULL, &window);
		window->on_paint             = on_paint;
		window->on_mouse_button_down = on_mouse_button_down;
		window->on_mouse_button_up   = on_mouse_button_up;
		window->on_mouse_move        = on_mouse_move;
		window->on_mouse_enter       = on_mouse_enter;
		window->on_mouse_leave       = on_mouse_leave;
		window->on_mouse_hover       = on_mouse_hover;
		window->set_hover_delay(window, 1000);

		//window->set_clip_children(window, i%2 == 0);
		window->set_clip_children(window, 1);
		window->update(window, AV_UPDATE_REPAINT);
		sys->create_window(sys, window, AV_NULL, &window);
		window->on_paint             = on_paint;
		window->on_mouse_button_down = on_mouse_button_down;
		window->on_mouse_button_up   = on_mouse_button_up;
		window->on_mouse_move        = on_mouse_move;
		window->on_mouse_enter       = on_mouse_enter;
		window->on_mouse_leave       = on_mouse_leave;
		window->on_mouse_hover       = on_mouse_hover;
		window->update(window, AV_UPDATE_REPAINT);
		window->set_hover_delay(window, 1000);
	}

/*	window = new_window(sys, AV_NULL, 100, 100, 300, 300);
	window->update(window, AV_UPDATE_REPAINT);
	window = new_window(sys, window, 100, 100, 200, 200);
	window->scroll(window, -50, -50);
	window->update(window, AV_UPDATE_REPAINT);
	window = new_window(sys, window, 10, 10, 100, 100);
	window->update(window, AV_UPDATE_REPAINT);
*/
	sys->loop(sys);
	av_torb_service_release("system");

err_done_torba:
	av_torb_done();

	return rc;
}
