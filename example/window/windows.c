/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      windows.c                                          */
/* Description:   Demonstrates and tests window management           */
/*                                                                   */
/*********************************************************************/

#include <malloc.h>
#include <string.h>
#include <av_window.h>
#include <av_input.h>
#include <av_system.h>

#define av_assert_ok(rc) av_assert(AV_OK == (rc), "assertion failed")

#define XRES 5
#define YRES 5

static char video_mem[XRES][YRES];
static av_list_p update_list = 0;

typedef struct 
{
	av_rect_t rect;
	char color;
} update_rect_t, *update_rect_p;

static void print_update_rect(void* param)
{
	update_rect_p update_rect = (update_rect_p)param;
	printf("update %d,%d,%d,%d (%c)\n", 
		    update_rect->rect.x, update_rect->rect.y, update_rect->rect.w, update_rect->rect.h, update_rect->color);
}

static void paint_rect(av_rect_p absrect, char color)
{
	int i, j;
	update_rect_p update_rect = (update_rect_p)malloc(sizeof(update_rect_t));
	av_rect_copy(&update_rect->rect, absrect);
	update_rect->color = color;
	
	av_assert(update_list, "update list is not initialized");
	update_list->push_last(update_list, update_rect);

	for (i=0; i<absrect->h; i++)
		for (j=0; j<absrect->w; j++)
		{
			video_mem[j][i] = color;
		}
}

#define PAINTER(on_paint, color) \
static av_bool_t on_paint(av_window_p window, av_graphics_p g) \
{ \
	av_rect_t absrect; \
	g->get_clip(g, &absrect); \
	window->rect_absolute(window, &absrect); \
	paint_rect(&absrect, color); \
	return AV_TRUE; \
}

/*    painter name  color */
PAINTER(paint_root, '0')
PAINTER(paint_1,    '1')
PAINTER(paint_2,    '2')
PAINTER(paint_3,    '3')

av_window_p root = 0;
av_window_p window1;
av_window_p window2;
av_window_p window3;

/* initialize test windows */
static void init_windows( void )
{
	av_result_t rc;
	av_video_config_t video_config;
	av_video_p video;
	av_input_p input;
	av_system_p sys;
	av_rect_t rect;

	if (root)
	{
		O_destroy(root);
		root = AV_NULL;
	}

	av_rect_init(&rect, 0, 0, XRES, YRES);
	//av_assert_ok(av_torb_create_object("window", (av_object_p*)&root));
	//root->set_rect(root, &rect);

	/* get reference to system service */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
	{
		av_torb_done();
		printf("av_torb_service_addref failed\n");
		return rc; /* exit cleanly with error status */
	}
	sys->create_window(sys, NULL, &rect, &root);
	av_torb_service_release("system");
	root->on_paint = paint_root;

	av_rect_init(&rect, 1, 1, 1, 1);
	av_assert_ok(av_torb_create_object("window", (av_object_p*)&window1));
	window1->set_parent(window1, root);
	window1->set_rect(window1, &rect);
	window1->on_paint = paint_1;

	av_rect_init(&rect, 2, 2, 1, 1);
	av_assert_ok(av_torb_create_object("window", (av_object_p*)&window2));
	window2->set_parent(window2, root);
	window2->set_rect(window2, &rect);
	window2->on_paint = paint_2;

	av_rect_init(&rect, 3, 3, 1, 1);
	av_assert_ok(av_torb_create_object("window", (av_object_p*)&window3));
	window3->set_parent(window3, root);
	window3->set_rect(window3, &rect);
	window3->on_paint = paint_3;

	/* clear the gathered results during destroy */
	update_list->remove_all(update_list, free);
}

/* adds update_rect info to a list */
static void add_expected_update_rect(av_list_p list, int x, int y, int w, int h, char color)
{
	update_rect_p update_rect = (update_rect_p)malloc(sizeof(update_rect_t));
	av_rect_init(&update_rect->rect, x, y, w, h);
	update_rect->color = color;
	list->push_last(list, update_rect);
}

/* compares two update_rect lists */
static av_bool_t compare_update_rect_lists(av_list_p expected, av_list_p actual, const char* testname)
{
	if (expected->size(expected) != actual->size(actual))
		return AV_FALSE;
	for (expected->first(expected), actual->first(actual);
		expected->has_more(expected); expected->next(expected), actual->next(actual))
	{
		update_rect_p expected_rect = (update_rect_p)expected->get(expected);
		update_rect_p actual_rect   = (update_rect_p)actual->get(actual);
		if (expected_rect->rect.x != actual_rect->rect.x ||
			expected_rect->rect.y != actual_rect->rect.y ||
			expected_rect->rect.w != actual_rect->rect.w ||
			expected_rect->rect.h != actual_rect->rect.h ||
			expected_rect->color != actual_rect->color)
		{
			printf("%s failed!\n", testname);
			printf("expected update rect:\n");
			print_update_rect(expected_rect);
			printf("actual update rect:\n");
			print_update_rect(actual_rect);
			return AV_FALSE;
		}
	}
	return AV_TRUE;
}

#define TEST(testname) if (testname()) printf("%s SUCCEEDED\n", #testname); else printf("%s FAILED\n", #testname);

/* TEST: invalidate root window */

static av_list_p expected_invalidate_root( void )
{
	av_list_p list;
	av_list_create(&list);
	add_expected_update_rect(list, 0,0,5,5,'0');
	add_expected_update_rect(list, 1,1,1,1,'1');
	add_expected_update_rect(list, 2,2,1,1,'2');
	add_expected_update_rect(list, 3,3,1,1,'3');
	return list;
}

static av_bool_t test_invalidate_root( void )
{
	av_bool_t res;
	/* init */
	av_list_p expected_list = expected_invalidate_root();
	init_windows();

	/* test */
	
	/* FIXME: Refactor this */
	root->update(root, AV_UPDATE_INVALIDATE);

	/* compare results */
	res = compare_update_rect_lists(expected_list, update_list, "test_invalidate_root");

	/* print */
	printf("test_invalidate_root\n");
	update_list->iterate_all(update_list, print_update_rect, AV_TRUE);

	/* done */
	expected_list->remove_all(expected_list, free);
	expected_list->destroy(expected_list);
	update_list->remove_all(update_list, free);

	return res;
}

/* TEST: case 1, window set rect */
/* the rect of window2 is enlarged to completely cover window 3 */

static av_list_p expected_set_rect_case1( void )
{
	av_list_p list;
	av_list_create(&list);
	add_expected_update_rect(list, 2,2,2,2,'0');
	add_expected_update_rect(list, 2,2,2,2,'2');
	add_expected_update_rect(list, 3,3,1,1,'3');
	return list;
}

static av_bool_t test_set_rect_case1( void )
{
	av_rect_t rect;
	av_bool_t res;
	/* init */
	av_list_p expected_list = expected_set_rect_case1();
	init_windows();

	/* test */
	av_rect_init(&rect, 2, 2, 2, 2);
	window2->set_rect(window2, &rect);

	/* compare results */
	res = compare_update_rect_lists(expected_list, update_list, "test_set_rect_case1");

	/* print */
	printf("test_set_rect_case1\n");
	update_list->iterate_all(update_list, print_update_rect, AV_TRUE);

	/* done */
	expected_list->remove_all(expected_list, free);
	expected_list->destroy(expected_list);
	update_list->remove_all(update_list, free);

	return res;
}

/* TEST: case 2, window set rect */
/* window2 from (1,1,3,3) is shrinked to (2,2,1,1) */

static av_list_p expected_set_rect_case2( void )
{
	av_list_p list;
	av_list_create(&list);
	add_expected_update_rect(list, 1,1,3,3,'0');
	add_expected_update_rect(list, 1,1,1,1,'1');
	add_expected_update_rect(list, 2,2,1,1,'2');
	add_expected_update_rect(list, 3,3,1,1,'3');
	return list;
}

static av_bool_t test_set_rect_case2( void )
{
	av_rect_t rect;
	av_bool_t res;
	/* init */
	av_list_p expected_list = expected_set_rect_case2();
	init_windows();

	/* test */

	/* makes the window enlarged */
	av_rect_init(&rect, 1, 1, 3, 3);
	window2->set_rect(window2, &rect);

	/* clear the gathered unnecessary results*/
	update_list->remove_all(update_list, free);

	/* shrink the window to its original position */	
	av_rect_init(&rect, 2, 2, 1, 1);
	window2->set_rect(window2, &rect);

	/* compare results */
	res = compare_update_rect_lists(expected_list, update_list, "test_set_rect_case2");

	/* print */
	printf("test_set_rect_case2\n");
	update_list->iterate_all(update_list, print_update_rect, AV_TRUE);

	/* done */
	expected_list->remove_all(expected_list, free);
	expected_list->destroy(expected_list);
	update_list->remove_all(update_list, free);

	return res;
}


/* TEST: case 3, window set rect */
/* the rect of window2 is moved on the place of window 3 */

static av_list_p expected_set_rect_case3( void )
{
	av_list_p list;
	av_list_create(&list);
	add_expected_update_rect(list, 2,2,1,1,'0');
	add_expected_update_rect(list, 3,3,1,1,'0');
	add_expected_update_rect(list, 3,3,1,1,'2');
	add_expected_update_rect(list, 3,3,1,1,'3');
	return list;
}

static av_bool_t test_set_rect_case3( void )
{
	av_rect_t rect;
	av_bool_t res;
	/* init */
	av_list_p expected_list = expected_set_rect_case3();
	init_windows();

	/* test */
	av_rect_init(&rect, 3, 3, 1, 1);
	window2->set_rect(window2, &rect);

	/* compare results */
	res = compare_update_rect_lists(expected_list, update_list, "test_set_rect_case3");

	/* print */
	printf("test_set_rect_case3\n");
	update_list->iterate_all(update_list, print_update_rect, AV_TRUE);

	/* done */
	expected_list->remove_all(expected_list, free);
	expected_list->destroy(expected_list);
	update_list->remove_all(update_list, free);

	return res;
}

/* TEST: case 4, window set rect */
/* the rect of window2 (0,0,2,2) is moved in a way to have 
*  intersection with the old window position (1,1,2,2)
*/

static av_list_p expected_set_rect_case4( void )
{
	av_list_p list;
	av_list_create(&list);
	add_expected_update_rect(list, 0,1,1,1,'0');
	add_expected_update_rect(list, 0,0,2,1,'0');
	add_expected_update_rect(list, 1,1,2,2,'0');
	add_expected_update_rect(list, 1,1,1,1,'1');
	add_expected_update_rect(list, 1,1,2,2,'2');
	return list;
}

static av_bool_t test_set_rect_case4( void )
{
	av_rect_t rect;
	av_bool_t res;
	/* init */
	av_list_p expected_list = expected_set_rect_case4();
	init_windows();

	/* test */
	av_rect_init(&rect, 0, 0, 2, 2);
	window2->set_rect(window2, &rect);

	/* clear the gathered unnecessary results*/
	update_list->remove_all(update_list, free);

	/* shift the window one pixel to right and bottom */
	av_rect_init(&rect, 1, 1, 2, 2);
	window2->set_rect(window2, &rect);

	/* compare results */
	res = compare_update_rect_lists(expected_list, update_list, "test_set_rect_case4");

	/* print */
	printf("test_set_rect_case4\n");
	update_list->iterate_all(update_list, print_update_rect, AV_TRUE);

	/* done */
	expected_list->remove_all(expected_list, free);
	expected_list->destroy(expected_list);
	update_list->remove_all(update_list, free);

	return res;
}

/* TEST: case 5, window set rect */
/* the rect of window2 (2,2,1,1) is set to its initial position
*/

static av_list_p expected_set_rect_case5( void )
{
	av_list_p list;
	av_list_create(&list);
	return list;
}

static av_bool_t test_set_rect_case5( void )
{
	av_rect_t rect;
	av_bool_t res;
	/* init */
	av_list_p expected_list = expected_set_rect_case5();
	init_windows();

	/* test */
	av_rect_init(&rect, 2, 2, 1, 1);
	window2->set_rect(window2, &rect);

	/* compare results */
	res = compare_update_rect_lists(expected_list, update_list, "expected_set_rect_case5");

	/* print */
	printf("expected_set_rect_case5\n");
	update_list->iterate_all(update_list, print_update_rect, AV_TRUE);

	/* done */
	expected_list->remove_all(expected_list, free);
	expected_list->destroy(expected_list);
	update_list->remove_all(update_list, free);

	return res;
}

/* entry point */
int main(void)
{
	memset(video_mem, 0, XRES*YRES);

	av_torb_init();
	av_window_register_torba();
	
	av_list_create(&update_list);

	TEST(test_invalidate_root);
	TEST(test_set_rect_case1);
	TEST(test_set_rect_case2);
 	TEST(test_set_rect_case3);
	TEST(test_set_rect_case4);
	TEST(test_set_rect_case5);

/*
	av_rect_init(&rect, 3, 3, 1, 1);
	testwindow->set_rect(testwindow, &rect);
*/

	if (root)
		((av_object_p)root)->destroy((av_object_p)root);

	/* clear the gathered results during destroy */
	update_list->remove_all(update_list, free);

	update_list->destroy(update_list);
	av_torb_done();
	return 0;
}
