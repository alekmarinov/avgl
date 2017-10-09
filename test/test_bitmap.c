#include <stdio.h>
#include <avgl.h>

int test_bitmap_load()
{
	av_surface_p surface;
	av_visible_p visible;
	avgl_create(AV_NULL);

	surface = avgl_load_surface("data/spritestrip.png");
	visible = avgl_create_visible_from_surface(AV_NULL, 0, 0, surface);

	avgl_loop();
	avgl_destroy();

	return 1;
}
