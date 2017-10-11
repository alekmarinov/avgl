#include <stdio.h>
#include <avgl.h>

static const char* IMAGE_NAME = "data/spritestrip.png";

int test_surface()
{
	av_surface_p surface;
	av_visible_p main = avgl_create(AV_NULL);
	av_visible_p visible;
	surface = avgl_load_surface(IMAGE_NAME);
	if (!surface) {
		printf("Can't load %s\n", IMAGE_NAME);
		goto quit;
	}
	main->create_child(main, "visible", &visible);
	visible->set_surface(visible, surface);
	avgl_loop();
quit:
	avgl_destroy();

	return 1;
}
