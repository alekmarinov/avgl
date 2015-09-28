/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      avplayer.c                                         */
/* Description:   Demonstrate player usage                           */
/*                                                                   */
/*********************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <av_prefs.h>
#include <av_system.h>
// #include <av_sound.h>
#include <av_player.h>
#include <av_graphics.h>
#include <av_log.h>

static int BORDER = 50;
static int WIDTH  = 800;
static int HEIGHT = 600;

av_log_p _log = 0;

// static av_sound_p sound;
// static struct av_sound_handle* sound_click;

static av_prefs_p prefs;
static av_player_p player;
static char* url;
static int fontsize = 14;
static av_bool_t dragging = AV_FALSE;
static av_bool_t is_position_change = AV_FALSE;
static int mouse_x = 0;
static int mouse_y = 0;
static av_graphics_surface_p logo = AV_NULL;
static char* medialib[32];
static int media_count = 0;
static int media_current = 0;

typedef struct navbutton
{
	int state;
	void (*action)(void);
	av_window_p window;
	av_window_p parent;
	av_graphics_surface_p surface;
	av_graphics_surface_p surface_onhover;
	const char* normal;
	const char* onhover;
}
navbutton_t, *navbutton_p;

static navbutton_t navbar[] =
{
	{ 0, AV_NULL, AV_NULL, AV_NULL, AV_NULL, AV_NULL, "stop_small.png", "stop_small_onhover.png" },
	{ 0, AV_NULL, AV_NULL, AV_NULL, AV_NULL, AV_NULL, "play_small.png", "play_small_onhover.png" },
	{ 0, AV_NULL, AV_NULL, AV_NULL, AV_NULL, AV_NULL, "pause_small.png", "pause_small_onhover.png" },
	{ 0, AV_NULL, AV_NULL, AV_NULL, AV_NULL, AV_NULL, "fastbackward_small.png", "fastbackward_small_onhover.png" },
	{ 0, AV_NULL, AV_NULL, AV_NULL, AV_NULL, AV_NULL, "fastforward_small.png", "fastforward_small_onhover.png" },
// 	{ 0, AV_NULL, AV_NULL, AV_NULL, AV_NULL, AV_NULL, "aspect-ratio_small.png", "aspect-ratio_small_onhover.png" },
// 	{ 0, AV_NULL, AV_NULL, AV_NULL, AV_NULL, AV_NULL, "fullscreen_small.png", "fullscreen_small_onhover.png" }
};

static int navstatus_x, navstatus_y;

typedef void (*action_t)(void);

static void action_stop(void) { player->stop(player); }
static void action_pause(void) { player->pause(player); }
static void action_seek_b(void) { player->seek_rel(player,-20.); }
static void action_seek_f(void) { player->seek_rel(player,+20.); }
static void action_dummy(void) { ; }
static void action_play(void)
{
	av_result_t rc;
	url = medialib[media_current++];
	if (media_current == media_count)
		media_current = 0;

	if (AV_OK == (rc = player->open(player, url)))
	{
		av_result_t old_st = avps_idle;
		av_player_state_t st;
		int quit;
		for(quit=0;!quit;)
		{
			st = (av_player_state_t)player->is_playing(player);
			_log->info(_log, "player state: (%.4X)",(int)st.state);
			if(old_st != st.state)
			{
				if(st.state_ext.state==avps_idle)
				{
					_log->info(_log, "avps_idle");
					quit = 1;
				}
				else
				if(st.state_ext.state & avps_connecting)
				{
					_log->info(_log, "avps_connecting");
				}
				else
				if(st.state_ext.state & avps_buffering)
				{
					_log->info(_log, "avps_buffering: (%d)",(int)st.state_ext.progress);
				}
				else
				if(st.state_ext.state & avps_running)
				{
					_log->info(_log, "avps_running");
					player->play(player);
					while(1)
					{
						st = (av_player_state_t)player->is_playing(player);
						_log->info(_log, "player state: (%.4X)",(int)st.state);
						if (st.state & avps_playing)
							break;
						if (!(st.state & avps_running))
							break;
						usleep(10000);
					}
					_log->info(_log, "avps_running and avps_playing");
					quit = 1;
				}
				else
					if(st.state_ext.state & avps_finished)
				{
					_log->info(_log, "avps_finished");
					player->stop(player);
					quit = 1;
				}
				old_st = st.state;
			}
			else
				usleep(10000);
		}
	}
	else
		_log->error(_log, "can't open %s (%d)", url, rc);
}

static action_t action[] =
{
	action_stop,
	action_play,
	action_pause,
	action_seek_b,
	action_seek_f,
	action_dummy,
	action_dummy
};

static int navbar_items = sizeof(navbar)/sizeof(navbutton_t);

static av_bool_t on_mouse_button_down(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	if (AV_MOUSE_BUTTON_LEFT == button)
	{
		mouse_x = x;
		mouse_y = y;
		self->capture(self);
		dragging = AV_TRUE;
		is_position_change = AV_TRUE;
		return AV_TRUE;
	}
	else if (AV_MOUSE_BUTTON_RIGHT == button)
	{
		mouse_x = x;
		mouse_y = y;
		self->capture(self);
		dragging = AV_TRUE;
		is_position_change = AV_FALSE;
		return AV_TRUE;
	}
	return AV_FALSE;
}

static av_bool_t on_mouse_left_right_up(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	AV_UNUSED(x);
	AV_UNUSED(y);
	AV_UNUSED(button);
	if (dragging)
	{
		self->uncapture(self);
		dragging = AV_FALSE;
		return AV_TRUE;
	}
	return AV_FALSE;
}

static av_bool_t on_mouse_move(av_window_p self, int x, int y)
{
	if (dragging)
	{
		av_rect_t rect;
		self->get_rect(self, &rect);
		if (is_position_change)
		{
			rect.x += (x - mouse_x);
			rect.y += (y - mouse_y);
		}
		else
		{
			rect.w += (x - mouse_x);
			rect.h += (y - mouse_y);
		}
		self->set_rect(self, &rect);
		mouse_x = x;
		mouse_y = y;
		return AV_TRUE;
	}
	else
	{
		int i;
		for (i=0;i<navbar_items;i++) navbar[i].state = 0;
	}
	return AV_FALSE;
}

static av_bool_t root_on_paint(av_window_p self, av_graphics_p graphics)
{
	av_rect_t rect;
	self->get_rect(self, &rect);
	rect.x = rect.y = 0;
	graphics->rectangle(graphics, &rect);
	graphics->set_color_rgba(graphics, 0., 0., 0., 1.);
	graphics->fill(graphics, AV_FALSE);
	return AV_TRUE;
}

static av_bool_t navbar_on_paint(av_window_p self, av_graphics_p graphics)
{
	av_rect_t rect;
	char label[64];
	self->get_rect(self, &rect);
	rect.x = rect.y = 0;
	graphics->rectangle(graphics, &rect);
	graphics->set_color_rgba(graphics, 0.5, 0.5, 0.5, 0.3);
	graphics->fill(graphics, AV_FALSE);

	if(player && player->is_playing(player))
	{
		double p, d;
		int ph, pm, ps;
		int dh, dm, ds;
		p = player->position(player);
		d = player->duration(player);
		ph = p/3600.; pm = (p - ph*3600.)/60.; ps = p - ph*3600. - pm*60.;
		dh = d/3600.; dm = (d - dh*3600.)/60.; ds = d - dh*3600. - dm*60.;
		sprintf(label,"%.2d:%.2d:%.2d / %.2d:%.2d:%.2d", ph, pm, ps, dh, dm, ds);
	}
	else
		sprintf(label,"00:00:00 / 00:00:00");

	graphics->select_font_face(graphics, "Arial", AV_FONT_SLANT_NORMAL, AV_FONT_WEIGHT_NORMAL);
	graphics->set_font_size(graphics,fontsize);
	graphics->set_color_rgba(graphics, 1., 1., 1., 1.);
	graphics->move_to(graphics, navstatus_x, navstatus_y);
	graphics->show_text(graphics, label);
	return AV_TRUE;
}

static av_bool_t logo_on_paint(av_window_p self, av_graphics_p graphics)
{
	AV_UNUSED(self);
	if (AV_NULL!=logo)
		graphics->show_image(graphics, 0, 0, logo);
	return AV_TRUE;
}

static av_bool_t navbutton_on_paint(av_window_p self, av_graphics_p graphics)
{
	navbutton_p btn = (navbutton_p)self->data;
	if (btn && AV_NULL!=btn->surface && AV_NULL!=btn->surface_onhover)
		graphics->show_image(graphics, 0, 0, (!btn->state)?btn->surface:btn->surface_onhover);
	return AV_TRUE;
}

static av_bool_t navbutton_on_mouse_move(av_window_p self, int x, int y)
{
	navbutton_p btn = (navbutton_p)self->data;
	AV_UNUSED(x);
	AV_UNUSED(y);
	if (btn)
	{
		int i;
		for (i=0;i<navbar_items;i++) navbar[i].state = 0;
		btn->state = 1;
		btn->parent->update(btn->parent, AV_UPDATE_INVALIDATE);
	}
	return AV_TRUE;
}

static av_bool_t navbutton_on_mouse_button_down(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
//	sound->play(sound,sound_click);
	AV_UNUSED(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	AV_UNUSED(button);
	return AV_TRUE;
}

static av_bool_t navbutton_on_mouse_button_up(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	navbutton_p btn = (navbutton_p)self->data;
	AV_UNUSED(x);
	AV_UNUSED(y);
	if (AV_MOUSE_BUTTON_LEFT == button)
	{
		if(btn->action)
		{
			btn->action();
		}
		return AV_TRUE;
	}
	return AV_FALSE;
}

static void create_navbar(av_system_p sys, av_graphics_p graphics, av_window_p parent)
{
	int i;
	int x = 0;
	int width;
	int height;
	av_rect_t rect;
	for (i=0;i<navbar_items;i++)
	{
		char p[255];
		av_surface_p surface;
		sprintf(p, "%s",navbar[i].normal);
		graphics->create_surface_file(graphics,p,&navbar[i].surface);
		sprintf(p, "%s",navbar[i].onhover);
		graphics->create_surface_file(graphics,p,&navbar[i].surface_onhover);

		surface = (av_surface_p)navbar[i].surface;
		surface->get_size(surface, &width, &height);
		rect.x = x;
		rect.y = 0;
		rect.w = width;
		rect.h = height;
		x += 3*width/2;
		sys->create_window(sys, parent, &rect, &navbar[i].window);
		navbar[i].action = action[i];
		navbar[i].window->data = &navbar[i];
		navbar[i].parent = parent;
		navbar[i].window->on_paint = navbutton_on_paint;
		navbar[i].window->on_mouse_move = navbutton_on_mouse_move;
		navbar[i].window->on_mouse_button_down = navbutton_on_mouse_button_down;
		navbar[i].window->on_mouse_button_up = navbutton_on_mouse_button_up;
		navbar[i].window->update(navbar[i].window, AV_UPDATE_REPAINT);
	}
	navstatus_x = x; navstatus_y = (height+fontsize)/2;
}

int main(int nargs, char* argv[])
{
	int i;
	av_result_t rc;
	av_system_p sys;
	av_graphics_p graphics;
	av_window_p root_window;
	av_rect_t root_rect;
	av_window_p player_window;
	av_rect_t player_rect;
	av_window_p navbar_window;
	av_rect_t navbar_rect;
	av_window_p logo_window;
	av_rect_t logo_rect;

	if (nargs < 2)
	{
		fprintf(stderr, "Usage: %s <media_file1> <media_file2> ... <media_fileN>\n",argv[0]);
		return 1;
	}

	media_current = media_count = 0;
	for (i=1 ; i<nargs; i++)
	{
		medialib[media_count++] = strdup(argv[i]);
	}

	if (AV_OK != (rc = av_torb_init()))
		return rc;

	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&_log)))
		return rc;

	/* get reference to system service */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
	{
		av_torb_service_release("log");
		av_torb_done();
		return rc;
	}

	/* get reference to prefs service */
	if (AV_OK != (rc = av_torb_service_addref("prefs", (av_service_p*)&prefs)))
	{
		av_torb_service_release("system");
		av_torb_service_release("log");
		av_torb_done();
		return rc;
	}

	prefs->get_int(prefs, "system.video.xres", WIDTH, &WIDTH);
	prefs->get_int(prefs, "system.video.yres", HEIGHT, &HEIGHT);
	av_torb_service_release("prefs");

	root_rect.x = 0;
	root_rect.y = 0;
	root_rect.w = WIDTH;
	root_rect.h = HEIGHT;

	player_rect.x = BORDER;
	player_rect.y = BORDER;
	player_rect.w = WIDTH-2*BORDER;
	player_rect.h = HEIGHT-3*BORDER;

	navbar_rect.x = BORDER;
	navbar_rect.y = HEIGHT-3*BORDER/2;
	navbar_rect.w = WIDTH-2*BORDER;
	navbar_rect.h = BORDER;

	logo_rect.x = WIDTH-2*BORDER;
	logo_rect.y = BORDER;
	logo_rect.w = 0;
	logo_rect.h = 0;

	/* create sound object
	if (AV_OK != (rc = av_torb_create_object("sound", (av_object_p*)&sound)))
	{
		_log->error(_log, "Error %d: sound create failed", rc);
		goto err_exit;
		return 1;
	} */

// 	sound->open(sound,"click.wav",&sound_click);

	sys->get_graphics(sys, &graphics);
	graphics->create_surface_file(graphics,"image.png",&logo);
	av_surface_p surface = (av_surface_p)logo;
	surface->get_size(surface, &logo_rect.w, &logo_rect.h);
	logo_rect.x -= logo_rect.w;

	sys->create_window(sys, AV_NULL, &root_rect, &root_window);

	sys->create_window(sys, AV_NULL, &root_rect, &root_window);
	sys->create_window(sys, root_window, &player_rect, &player_window);
	sys->create_window(sys, root_window, &navbar_rect, &navbar_window);
	sys->create_window(sys, root_window, &logo_rect, &logo_window);
	root_window->on_paint = root_on_paint;
	root_window->set_clip_children(root_window, AV_TRUE);

	sys->update(sys);

	player_window->on_mouse_button_down = on_mouse_button_down;
	player_window->on_mouse_button_up   = on_mouse_left_right_up;
	player_window->on_mouse_move        = on_mouse_move;

	navbar_window->on_paint             = navbar_on_paint;
	navbar_window->on_mouse_button_down = on_mouse_button_down;
	navbar_window->on_mouse_button_up   = on_mouse_left_right_up;
	navbar_window->on_mouse_move        = on_mouse_move;

	logo_window->on_paint               = logo_on_paint;

	create_navbar(sys, graphics, navbar_window);

	sys->update(sys);

	/* create player object */
	if (AV_OK != (rc = av_torb_create_object("player", (av_object_p*)&player)))
	{
		_log->error(_log, "Error %d: player create failed", rc);
		goto err_exit;
		return 1;
	}

	player->set_window(player, player_window);

	navbar_window->update(navbar_window, AV_UPDATE_REPAINT);
	player_window->update(player_window, AV_UPDATE_REPAINT);
	logo_window->update(logo_window, AV_UPDATE_REPAINT);

	sys->update(sys);
	sys->loop(sys);

	player->stop(player);

//	sound->close(sound,sound_click);

	O_destroy(player);
//	O_destroy(sound);

err_exit:
	av_torb_service_release("system");
	av_torb_service_release("log");
	av_torb_done();
	return 0;
}
