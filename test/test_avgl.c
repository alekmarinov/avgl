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

int test_avgl_create_destroy()
{
	avgl_create();
	for (int i = 0; i < 100; i++) avgl_create_visible(AV_NULL, 5 * i, 5 * i, 300, 160, on_draw);
	avgl_loop();
	avgl_destroy();

	return 1;
}
