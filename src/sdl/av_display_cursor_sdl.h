/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_display_cursor_sdl.h                              */
/* Description:   SDL display cursor                                   */
/*                                                                   */
/*********************************************************************/

#ifndef __AV_DISPLAY_CURSOR_SDL_H
#define __AV_DISPLAY_CURSOR_SDL_H

#include "av_display_sdl.h"

void av_display_cursor_sdl_init(void);
av_result_t av_display_cursor_sdl_set_shape(av_display_cursor_shape_t cursor_shape);
void av_display_cursor_sdl_done(void);

#endif /* __AV_DISPLAY_CURSOR_SDL_H */
