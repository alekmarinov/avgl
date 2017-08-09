/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_core_sdl.h                                      */
/* Description:                                                      */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_SDL

#include <av.h>

av_result_t av_sdl_error_process(int rc, const char* funcname, const char* srcfilename, int linenumber);
#define av_sdl_error_check(funcname, rc) av_sdl_error_process(rc, funcname, __FILE__, __LINE__)

#endif /* WITH_SYSTEM_SDL */
