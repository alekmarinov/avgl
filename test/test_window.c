#include <avgl.h>

void dump_window(av_window_p window)
{
	av_rect_t rect;
	av_rect_t arect;
	window->get_rect(window, &rect);
	window->get_absolute_rect(window, &arect);
	printf("window %d %d %d %d {%d %d %d %d}\n", rect.x, rect.y, rect.w, rect.h, arect.x, arect.y, arect.w, arect.h);
}

int test_window_absolute()
{
	av_window_p parent;
	av_window_p child;
	av_oop_p oop;
	av_rect_t rect;
	av_oop_create(&oop);
	av_window_register_oop(oop);
	oop->new(oop, "window", (av_object_p*)&parent);
	oop->new(oop, "window", (av_object_p*)&child);
	
	av_rect_init(&rect, 10, 10, 5, 5);
	parent->set_rect(parent, &rect);

	av_rect_init(&rect, 0, 0, 5, 5);
	child->set_rect(child, &rect);
	child->set_parent(child, parent);

	dump_window(child);
	dump_window(parent);
	O_release(child);
	O_release(parent);
	oop->destroy(oop);
	return 1;
}
