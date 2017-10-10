#include <stdio.h>
#include <avgl.h>

const char* IMAGE_NAME = "data/spritestrip.png";

int test_bitmap_load()
{
	av_surface_p surface;
	av_visible_p main = avgl_create(AV_NULL);
	av_visible_p visible;
	av_rect_t rect;
	surface = avgl_load_surface(IMAGE_NAME);
	if (!surface) {
		printf("Can't load %s\n", IMAGE_NAME);
		goto quit;
	}
	rect.x = rect.y = 0;
	main->create_child(main, &rect, AV_NULL, surface, &visible);
	avgl_loop();
quit:
	avgl_destroy();

	return 1;
}
