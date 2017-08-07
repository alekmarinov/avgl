#include <avgl.h>

int test_avgl_create_destroy()
{
	avgl_p avgl;
	avgl_create(&avgl);
	avgl->loop(avgl);
	avgl->destroy(avgl);

	return 1;
}
