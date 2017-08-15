/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_display_cursor_sdl.c                            */
/* Description:   SDL display cursor                                 */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_SDL

#include <SDL.h>
#include "av_display_sdl.h"
#include "av_display_cursor_sdl.h"
#include "av_core_sdl.h"

#include "cursor/arrow.xpm"
#include "cursor/beam.xpm"
#include "cursor/busy.xpm"
#include "cursor/hand.xpm"
#include "cursor/keyboard.xpm"
#include "cursor/scroll.xpm"
#include "cursor/zoom.xpm"

static int initialized = 0;
static SDL_Cursor* cursors[AV_DISPLAY_CURSOR_LAST];

static SDL_Cursor* create_cursor_from_xpm(const char** xpm, int hotx, int hoty)
{
	int i, w, h, c;
	char white=0, black=0, none=0;
	int lc;
	unsigned char *data;
	unsigned char *mask;
	sscanf(xpm[0], "%d %d %d", &w, &h, &c);
	av_assert(w % 8 == 0, "width must be divisible to 8");

	lc = w >> 3;
	data = (unsigned char*)calloc(lc * h, 1);
	mask = (unsigned char*)calloc(lc * h, 1);
	for (i=0; i<c; i++)
	{
		char dummy;
		char chr;
		char colorname[100];
		sscanf(xpm[1+i], "%c %c %s", &chr, &dummy, colorname);
		if (!strcasecmp("none", colorname))
		{
			none = chr;
		}
		else if (!strcasecmp("black", colorname))
		{
			black = chr;
		}
		else if (!strcasecmp("white", colorname))
		{
			white = chr;
		}
	}
	for (i=0; i<h; i++)
	{
		const char* line = xpm[1+c+i];
		int y = i*lc;
		int j;
		for (j=0; j<w; j++)
		{
			int x = j>>3;
			int z = 1<<(7-(j&7));
			if (line[j] == white)
			{
				mask[y+x] |= z;
			}
			else if (line[j] == black)
			{
				mask[y+x] |= z;
				data[y+x] |= z;
			}
		}
	}
	return SDL_CreateCursor(data, mask, w, h, hotx, hoty);
}

void av_display_cursor_sdl_init()
{
	av_display_cursor_sdl_done();
	if (!initialized)
	{
		cursors[AV_DISPLAY_CURSOR_DEFAULT]  = create_cursor_from_xpm(arrow_xpm, 6,  0);
		cursors[AV_DISPLAY_CURSOR_BEAM]     = create_cursor_from_xpm(beam_xpm, 6, 0);
		cursors[AV_DISPLAY_CURSOR_WAIT]     = create_cursor_from_xpm(busy_xpm, 6, 0);
		cursors[AV_DISPLAY_CURSOR_HAND]     = create_cursor_from_xpm(hand_xpm,  6, 0);
		cursors[AV_DISPLAY_CURSOR_KEYBOARD] = create_cursor_from_xpm(keyboard_xpm,  6, 0);
		cursors[AV_DISPLAY_CURSOR_ZOOM]     = create_cursor_from_xpm(zoom_xpm,  6, 0);
		cursors[AV_DISPLAY_CURSOR_SCROLL]   = create_cursor_from_xpm(scroll_xpm,  6, 0);

		initialized = 1;
	}
}

av_result_t av_display_cursor_sdl_set_shape(av_display_cursor_shape_t cursor_shape)
{
	if (cursor_shape < AV_DISPLAY_CURSOR_LAST)
	{
		SDL_SetCursor(cursors[cursor_shape]);
	}
	return AV_EFOUND;
}

void av_display_cursor_sdl_done(void)
{
	if (initialized)
	{
		int i;
		for (i = 0;  i < AV_DISPLAY_CURSOR_LAST; i++)
		{
			SDL_FreeCursor(cursors[i]);
		}
		initialized = 0;
	}
}

#endif // WITH_SYSTEM_SDL
