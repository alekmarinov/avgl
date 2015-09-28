/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_video_cursor_sdl.h                              */
/* Description:   SDL video cursor                                   */
/*                                                                   */
/*********************************************************************/

#ifndef __AV_VIDEO_CURSOR_SDL_H
#define __AV_VIDEO_CURSOR_SDL_H

#include "av_video_sdl.h"

void av_video_cursor_sdl_init(void);
av_result_t av_video_cursor_sdl_set_shape(av_video_cursor_shape_t cursor_shape);
void av_video_cursor_sdl_done(void);

#endif /* __AV_VIDEO_CURSOR_SDL_H */
