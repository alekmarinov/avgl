#include <stdio.h>
#include <avgl.h>

#define RAND_COLOR ((double)rand() / RAND_MAX)

void on_draw(av_visible_p self, av_graphics_p graphics)
{
	av_rect_t rect;
	((av_window_p)self)->get_rect((av_window_p)self, &rect);
	rect.x = rect.y = 0;
	graphics->set_color_rgba(graphics, RAND_COLOR, RAND_COLOR, RAND_COLOR, RAND_COLOR);
	graphics->rectangle(graphics, &rect);
	graphics->fill(graphics, AV_FALSE);
}

av_bool_t on_mouse_button_down(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	self->raise_top(self);
	return AV_TRUE;
}

void create_visible(av_visible_p parent, int x, int y)
{
	av_rect_t rect;
	av_visible_p visible;
	av_rect_init(&rect, x, y, 30, 16);
	parent->create_child(parent, &rect, AV_NULL, AV_NULL, &visible);
	av_window_p window = (av_window_p)visible;
	window->on_mouse_button_down = on_mouse_button_down;
}

int test_avgl_create_destroy()
{
	av_visible_p main_visible = avgl_create(AV_NULL);
	for (int i = 0; i < 10; i++)
		create_visible(main_visible, 5 * i, 5 * i);
	avgl_loop();
	avgl_destroy();

	return 1;
}
