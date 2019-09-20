
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <avgl.h>

static const char* GIRL_IMAGE = "data/spritestrip.png";
static int seq_girl[] = { 0, 1, 2, 3, 4, 5 };
static const int girl_width = 256;
static const int girl_height = 256;

static const char* EXPLOSION_IMAGE = "data/explosion_transparent.png";
static int seq_explosion[] = {
	0, 1, 2, 3, 4, 5,
	6, 7, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17,
	18, 19, 20, 21, 22, 23
};
#define GIRL_SEQ_COUNT sizeof(seq_explosion) / sizeof(int)
static const int explosion_width = 64;
static const int explosion_height = 64;

#define EXPLOSION_SEQ_COUNT sizeof(seq_explosion) / sizeof(int)

#define RAND ((double)rand() / RAND_MAX)

av_bool_t sprite_timer(void* arg)
{
	av_sprite_p sprite = (av_sprite_p)arg;
	sprite->set_sequence(sprite, seq_explosion, EXPLOSION_SEQ_COUNT, 700, AV_TRUE);
	return AV_FALSE;
}


int girl_dx = 40;
int girl_dy = 40;
av_bool_t girl_move_timer(void* arg)
{
	av_rect_t rect;
	av_sprite_p sprite = (av_sprite_p)arg;
	((av_window_p)sprite)->get_rect((av_window_p)sprite, &rect);
	int screen_width = ((av_visible_p)sprite)->system->display->display_config.width;
	int screen_height = ((av_visible_p)sprite)->system->display->display_config.height;

	if (rect.x + girl_dx > screen_width - rect.w)
		girl_dx = -girl_dx;
	if (rect.y + girl_dy > screen_height - rect.h)
		girl_dy = -girl_dy;
	if (rect.x + girl_dx < 0)
		girl_dx = -girl_dx;
	if (rect.y + girl_dy < 0)
		girl_dy = -girl_dy;
	rect.x += girl_dx;
	rect.y += girl_dy;
	((av_window_p)sprite)->set_rect((av_window_p)sprite, &rect);
	return AV_TRUE;
}

int test_sprite()
{
	int a;
	av_surface_p explosion_surface = AV_NULL;
	av_surface_p girl_surface = AV_NULL;
	av_visible_p main = avgl_create(AV_NULL);
	av_sprite_p sprite;
	av_rect_t rect;
	av_system_p system = (av_system_p)main->system;

	explosion_surface = avgl_load_surface(EXPLOSION_IMAGE);
	if (!explosion_surface) {
		printf("Can't load %s\n", EXPLOSION_IMAGE);
		goto quit;
	}

	girl_surface = avgl_load_surface(GIRL_IMAGE);
	if (!girl_surface) {
		printf("Can't load %s\n", GIRL_IMAGE);
		goto quit;
	}

	main->create_child(main, "sprite", (av_visible_p*)&sprite);
	((av_visible_p)sprite)->set_surface((av_visible_p)sprite, girl_surface);
	sprite->set_size(sprite, girl_width, girl_height);
	rect.x = rect.y = 0;
	rect.w = girl_width;
	rect.h = girl_height;
	((av_window_p)sprite)->set_rect((av_window_p)sprite, &rect);
	sprite->set_sequence(sprite, seq_girl, 6, 700, AV_TRUE);

	/*
	system->timer->add_timer(system->timer, girl_move_timer, 200, sprite, AV_NULL);

	for (int i = 0; i < 10; i++)
	{
		rect.x = (int)((float)1000 * RAND);
		rect.y = (int)((float)1000 * RAND);
		main->create_child(main, "sprite", (av_visible_p*)&sprite);
		((av_visible_p)sprite)->set_surface((av_visible_p)sprite, explosion_surface);
		sprite->set_size(sprite, explosion_width, explosion_height);
		rect.w = explosion_width;
		rect.h = explosion_height;
		((av_window_p)sprite)->set_rect((av_window_p)sprite, &rect);
		system->timer->add_timer(system->timer, sprite_timer, (int)((float)2000 * RAND), sprite, AV_NULL);
	}*/


	avgl_loop();
quit:
	if (explosion_surface) O_destroy(explosion_surface);
	if (girl_surface) O_destroy(girl_surface);
	avgl_destroy();
	printf("Exiting\n");
	scanf("%d\n", &a);
	return 1;
}
