/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      modals.c                                           */
/* Description:   Demonstrate modal window usage                     */
/*                                                                   */
/*********************************************************************/

#include <stdlib.h>
#include <av_system.h>
#include <av_window.h>

#define XRES 1024
#define YRES 768

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT 200

av_system_p sys;
av_window_p window;

static av_bool_t on_paint(av_window_p self, av_graphics_p graphics)
{
	int r, g, b;
	long addr=(long)self;
	av_rect_t rect;

	self->get_rect(self, &rect);
	printf("on_paint %d %d %d %d\n", rect.x, rect.y, rect.w, rect.h);
	rect.x = rect.y = 0;
	graphics->rectangle(graphics, &rect);
	addr = (addr & 0xFFFF) ^ ((addr >> 16) & 0xFFFF);
	r=(unsigned char)(addr & 0xFF); addr>>=8;
	g=(unsigned char)(addr & 0xFF); addr>>=8;
	b=(unsigned char)(addr & 0xFF);
	graphics->set_color_rgba(graphics, (double)r/255, (double)g/255, (double)b/255, 0.9);
	graphics->fill(graphics, 0);
	return AV_TRUE;
}

static av_bool_t on_mouse_button_down(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	AV_UNUSED(x);
	AV_UNUSED(y);
	printf("on_mouse_button_down: button = %d\n", button);
	if (AV_MOUSE_BUTTON_LEFT == button )
	{
		sys->modal_exit(sys, self, 1);
		return AV_TRUE;
	}
	else if (AV_MOUSE_BUTTON_MIDDLE == button )
	{
		self->set_visible(self, AV_FALSE);
		return AV_TRUE;
	}
	return AV_FALSE;
}

static void create_window(void)
{
	/* creates test window */
	av_rect_t rect;
	rect.x = 10;
	rect.y = 10;
	rect.w = 100;
	rect.h = 100;
	sys->create_window(sys, AV_NULL, &rect, &window);
	window->on_paint            = on_paint;
	window->on_mouse_button_down  = on_mouse_button_down;
	window->update(window, AV_UPDATE_REPAINT);
	// window->set_visible(window, AV_FALSE);

}

static av_bool_t root_on_mouse_button_down(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	av_rect_t rect;
	AV_UNUSED(x);
	AV_UNUSED(y);
	AV_UNUSED(self);
	printf("root_on_mouse_button_down: button = %d\n", button);
	if (AV_MOUSE_BUTTON_LEFT == button )
	{
		O_destroy(window);
		create_window();
		av_rect_init(&rect, 0, 0, XRES, YRES);
		sys->invalidate_rect(sys, &rect);
		return AV_TRUE;
	}
	else if (AV_MOUSE_BUTTON_RIGHT == button )
	{
		int result_code = sys->modal_enter(sys, window, AV_MODALITY_BLOCK);
		printf("Modal window exited with result = %d\n", result_code);
	}
	return AV_FALSE;
}

int main(void)
{
	av_result_t rc;
	av_window_p root;

	if (AV_OK != (rc = av_torb_init()))
		return rc;

	/* create SDL system */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
		goto err_done_torba;

	/* creates root window */
	sys->create_window(sys, AV_NULL, AV_NULL, &root);
	root->on_mouse_button_down  = root_on_mouse_button_down;

	/* creates test window */
	create_window();

	sys->loop(sys);
	av_torb_service_release("system");

err_done_torba:
	av_torb_done();

	return rc;
}
