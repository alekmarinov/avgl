#include <avgl.h>
#include <stdio.h>

static av_bool_t parent_move = AV_FALSE;
static av_bool_t child_move = AV_FALSE;
static av_bool_t child_move_bubble = AV_FALSE;

static av_bool_t on_mouse_move_parent(av_window_p self, int x, int y)
{
	printf("on_mouse_move_parent: x = %d, y = %d\n", x, y);
	parent_move = AV_TRUE;
	return AV_TRUE;
}

static av_bool_t on_mouse_move_child(av_window_p self, int x, int y)
{
	printf("on_mouse_move_child: x = %d, y = %d\n", x, y);
	child_move = AV_TRUE;
	return AV_TRUE;
}

static av_bool_t on_mouse_move_child_bubble(av_window_p self, int x, int y)
{
	printf("on_mouse_move_child_bubble: x = %d, y = %d\n", x, y);
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
	avgl_create();

	parent = (av_window_p)avgl_create_visible(AV_NULL, 10, 10, 5, 5, AV_NULL);
	parent->on_mouse_move = on_mouse_move_parent;
	child1 = (av_window_p)avgl_create_visible(parent, 1, 1, 2, 2, AV_NULL);
	child1->on_mouse_move = on_mouse_move_child;
	child2 = (av_window_p)avgl_create_visible(parent, 3, 3, 2, 2, AV_NULL);
	child2->on_mouse_move = on_mouse_move_child_bubble;

	// assert no event
	send_mouse_move(9, 9);
	parent_move = AV_FALSE;
	if (wait_flag(&parent_move))
		return 0; // failure, parent should not get move event

	// assert mouse move on parent, not on child
	send_mouse_move(10, 10);
	parent_move = AV_FALSE;
	if (!wait_flag(&parent_move))
		return 0; // failure, parent should get move event

	// assert mouse move on child1, but not on parent
	send_mouse_move(11, 11);
	parent_move = AV_FALSE;
	child_move = AV_FALSE;
	if (!wait_flag(&child_move) || parent_move)
		return 0; // failure, child should get move and parent should not get move event

	// assert mouse move on child2 and parent
	parent_move = AV_FALSE;
	child_move = AV_FALSE;
	child_move_bubble = AV_FALSE;
	send_mouse_move(13, 13);
	if (!wait_flag(&child_move_bubble) || !parent_move)
		return 0; // failure, child bubble should get move and parent should get move event

	avgl_destroy();
	return 1;
}
