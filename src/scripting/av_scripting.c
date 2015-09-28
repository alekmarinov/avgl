/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_scripting.c                                     */
/*                                                                   */
/*********************************************************************/

#include <av_scripting.h>

static av_result_t av_scripting_dofile(av_scripting_p self,
									   const char* filename,
									   int nargs,
									   char* argv[])
{
	AV_UNUSED(self);
	AV_UNUSED(filename);
	AV_UNUSED(nargs);
	AV_UNUSED(argv);
	return AV_ESUPPORTED;
}

static av_result_t av_scripting_dobuffer(av_scripting_p self,
										 const char* buffer,
										 int bufsize,
										 const char* name)
{
	AV_UNUSED(self);
	AV_UNUSED(buffer);
	AV_UNUSED(bufsize);
	AV_UNUSED(name);
	return AV_ESUPPORTED;
}

static av_result_t av_scripting_constructor(av_object_p object)
{
	av_scripting_p self = (av_scripting_p)object;
	self->dofile   = av_scripting_dofile;
	self->dobuffer = av_scripting_dobuffer;
	return AV_OK;
}

/* Registers scripting lua class into TORBA */
av_result_t av_scripting_register_torba(void)
{
	return av_torb_register_class("scripting", AV_NULL,
								  sizeof(av_scripting_t),
								  av_scripting_constructor, AV_NULL);
}
