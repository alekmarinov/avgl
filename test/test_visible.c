#include <stdio.h>
#include <avgl.h>

static const char* IMAGE_NAME = "data/spritestrip.png";

typedef struct {
	av_visible_t visible;

} av_custom_visible_t, *av_custom_visible_p;

static av_result_t av_custom_visible_constructor(av_object_p object)
{
	av_custom_visible_p self = (av_custom_visible_p)object;
	av_visible_p visible = (av_visible_p)object;
	visible->set_surface(visible, avgl_load_surface(IMAGE_NAME));
	return AV_OK;
}

int register_visible(av_visible_p main_visible)
{
	av_oop_p oop = ((av_object_p)main_visible)->classref->oop;
	return oop->define_class(oop, "custom_visible", "visible",
		sizeof(av_custom_visible_t),
		av_custom_visible_constructor, AV_NULL);
}

int test_visible()
{
	av_visible_p main = avgl_create(AV_NULL);
	av_visible_p visible;
	av_result_t rc;
	av_rect_t rect;
	av_system_p system;

	if (AV_OK != (rc = register_visible(main)))
		return rc;

	if (AV_OK != (rc = main->create_child(main, "custom_visible", &visible)))
		return rc;

	system = (av_system_p)visible->system;
	((av_window_p)visible)->get_rect((av_window_p)visible, &rect);
	
	rect.y = (visible->system->display->display_config.height - rect.h) / 2;
	((av_window_p)visible)->set_rect((av_window_p)visible, &rect);

	avgl_loop();
	avgl_destroy();

	return 1;
}
