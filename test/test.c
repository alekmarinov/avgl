#ifdef _MSC_VER
#  define _CRTDBG_MAP_ALLOC 
#  include <crtdbg.h>  
#endif

#include <stdio.h>
#include "test.h"

#define TEST(t) if (t()) printf("Test %s PASSED\n", #t); else ("Test %s FAILED\n", #t);

main()
{
//	TEST(test_oop_inheritance)
//	TEST(test_avgl_create_destroy)
//	TEST(test_window_absolute)
//	TEST(test_event_mouse)
//	TEST(test_widgets)
//	TEST(test_surface)
//	TEST(test_visible)
	TEST(test_sprite)

#ifdef _MSC_VER
		_CrtDumpMemoryLeaks();
#endif

	return 0;
}
