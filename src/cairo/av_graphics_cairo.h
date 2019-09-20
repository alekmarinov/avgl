/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_graphics_cairo.h                                */
/* Description:   Cairo graphics interface                           */
/*                                                                   */
/*********************************************************************/

#ifndef __AV_GRAPHICS_CAIRO_H
#define __AV_GRAPHICS_CAIRO_H

#include <av.h>

#define MAKE_VERSION_NUM(x,y,z) (z + (y << 8) + (x << 16))
#define CAIRO_VERSION_NUM        MAKE_VERSION_NUM(CAIRO_VERSION_MAJOR, CAIRO_VERSION_MINOR, CAIRO_VERSION_MICRO)
#define CAIRO_NEWER(x,y,z)      (MAKE_VERSION_NUM(x,y,z) <= CAIRO_VERSION_NUM)
#define CAIRO_OLDER(x,y,z)      (MAKE_VERSION_NUM(x,y,z) > CAIRO_VERSION_NUM)

av_result_t av_cairo_error_process(int rc, const char* funcname, const char* srcfilename, int linenumber);
#define av_cairo_error_check(funcname, rc) av_cairo_error_process(rc, funcname, __FILE__, __LINE__)

#endif /* __AV_GRAPHICS_CAIRO_H */
