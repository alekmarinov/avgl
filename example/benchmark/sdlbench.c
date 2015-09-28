/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      sdlbench.c                                         */
/* Description:   SDL performance benchmark                          */
/*                                                                   */
/*********************************************************************/

#include <unistd.h>
#include <stdlib.h>

#include <SDL.h>

#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT  768
#define VIDEO_BPP       32
#define VIDEO_FLAGS    SDL_HWSURFACE | SDL_ASYNCBLIT /* | SDL_FULLSCREEN*/

int main(int argc, char* argv[])
{
	int    done;
	SDL_Event event;
	SDL_Surface *screen;

	/* Initialize SDL */
	if ( SDL_Init(SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	/* Initialize the display */
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, VIDEO_BPP, VIDEO_FLAGS);
	if (  screen == NULL ) {
		fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n",
						SCREEN_WIDTH, SCREEN_HEIGHT, VIDEO_BPP, SDL_GetError());
		exit(1);
	}

	printf("MUSTLOCK = %d\n", SDL_MUSTLOCK(screen));
	if ( SDL_LockSurface(screen) < 0 ) {
		fprintf(stderr, "Couldn't lock surface: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_UnlockSurface(screen);

	done = 0;
	while ( !done && SDL_WaitEvent(&event) )
	{
		switch (event.type)
		{
			case SDL_QUIT:
				done = 1;
			break;
			case SDL_KEYDOWN:
				done = 1;
			break;
		}
	}

	SDL_Quit();

	return 0;
}
