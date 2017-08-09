/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_core_sdl.c                                      */
/* Description:                                                      */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_SDL

#include "av_core_sdl.h"
#include <av_stdc.h>
#include <SDL.h>

av_result_t av_sdl_error_process(int rc, const char* funcname, const char* srcfilename, int linenumber)
{
	av_result_t averr = AV_OK;
	if (rc < 0)
	{
		const char* errmsg = SDL_GetError();
		if (!av_strcmp(errmsg, "Out of memory"))
			averr = AV_EMEM;
		else if (!av_strcmp(errmsg, "Error reading from datastream"))
			averr = AV_EREAD;
		else if (!av_strcmp(errmsg, "Error writing to datastream"))
			averr = AV_EWRITE;
		else if (!av_strcmp(errmsg, "Error seeking in datastream"))
			averr = AV_ESEEK;

		if (averr != AV_OK)
		{
			// _log->error(_log, "%s returned error (%d) `%s' %s:%d", 
			//						funcname, rc, errmsg, srcfilename, linenumber);
		}
		else
		{
			// _log->error(_log, "%s returned unknown error with message `%s' %s:%d", 
			//						funcname, errmsg, srcfilename, linenumber);
		}
	}
	return averr;
}

#endif /* WITH_SYSTEM_SDL */

