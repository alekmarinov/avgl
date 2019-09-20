#include <avgl.h>
#include <stdio.h>

static av_bool_t parent_move = AV_FALSE;
static av_bool_t child_move = AV_FALSE;
static av_bool_t child_move_bubble = AV_FALSE;

static void print_window_xy(const char* name, av_window_p self, int x, int y)
{
	av_rect_t rect;
	self->get_absolute_rect(self, &rect);
	printf("%s: x = %d, y = %d (%d %d %d %d)\n", name, x, y, rect.x, rect.y, rect.w, rect.h);
}

static av_bool_t on_mouse_move_parent(av_window_p self, int x, int y)
{
	print_window_xy("on_mouse_move_parent", self, x, y);
	parent_move = AV_TRUE;
	return AV_TRUE;
}

static av_bool_t on_mouse_move_child(av_window_p self, int x, int y)
{
	print_window_xy("on_mouse_move_child", self, x, y);
	child_move = AV_TRUE;
	return AV_TRUE;
}

static av_bool_t on_mouse_move_child_bubble(av_window_p self, int x, int y)
{
	print_window_xy("on_mouse_move_child_bubble", self, x, y);
	child_move_bubble = AV_TRUE;
	return AV_FALSE;
}

static void send_mouse_move(int x, int y)
{
	av_event_t event;
	event.type = AV_EVENT_MOUSE_MOTION;
	event.mouse_x = x;
	event.mouse_y = y;
	avgl_event_push(&event);
	printf("send_mouse_move %d %d\n", x, y);
}

av_bool_t wait_flag(av_bool_t* flag)
{
	unsigned long now = avgl_time_now();
	while (!*flag)
	{
		avgl_step();
		if (avgl_time_now() - now > 1000)
			return AV_FALSE;
	}
	return AV_TRUE;
}

int test_event_mouse()
{
	av_window_p parent;
	av_window_p child1;
	av_window_p child2;
	av_visible_p root = avgl_create(AV_NULL);
	av_rect_t rect;

	root->create_child(root, "visible", (av_visible_p*)&parent);
	av_rect_init(&rect, 10, 10, 5, 5);
	parent->set_rect(parent, &rect);
	parent->on_mouse_move = on_mouse_move_parent;

	((av_visible_p)parent)->create_child((av_visible_p)parent, "visible", (av_visible_p*)&child1);
	av_rect_init(&rect, 1, 1, 2, 2);
	child1->set_rect(child1, &rect);
	child1->on_mouse_move = on_mouse_move_child;

	((av_visible_p)parent)->create_child((av_visible_p)parent, "visible", (av_visible_p*)&child2);
	av_rect_init(&rect, 3, 3, 2, 2);
	child2->set_rect(child2, &rect);
	child2->on_mouse_move = on_mouse_move_child_bubble;

	// assert no event
	send_mouse_move(9, 9);
	parent_move = AV_FALSE;
	if (wait_flag(&parent_move))
	{
		avgl_destroy();
		return 0; // failure, parent should not get move event
	}

	// assert mouse move on parent, not on child
	send_mouse_move(10, 10);
	parent_move = AV_FALSE;
	if (!wait_flag(&parent_move))
	{
		avgl_destroy();
		return 0; // failure, parent should get move event
	}

	// assert mouse move on child1, but not on parent
	send_mouse_move(11, 11);
	parent_move = AV_FALSE;
	child_move = AV_FALSE;
	if (!wait_flag(&child_move) || parent_move)
	{
		avgl_destroy();
		return 0; // failure, child should get move and parent should not get move event
	}

	// assert mouse move on child2 and parent
	parent_move = AV_FALSE;
	child_move = AV_FALSE;
	child_move_bubble = AV_FALSE;
	send_mouse_move(13, 13);
	if (!wait_flag(&child_move_bubble) || !parent_move)
	{
		avgl_destroy();
		return 0; // failure, child bubble should get move and parent should get move event
	}

	avgl_destroy();
	return 1;
}
