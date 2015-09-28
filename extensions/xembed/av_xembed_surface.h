/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_xembed_surface.h                                */
/*                                                                   */
/*********************************************************************/

/*! \file av_xembed_surface.h
*   \brief av_xembed_surface interface definition
*/

#ifndef __AV_XEMBED_SURFACE_H
#define __AV_XEMBED_SURFACE_H

#include <av_torb.h>
#include <av_surface.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief av_xembed_surface is Xvfb memory wrapping surface
*
*/
typedef struct av_xembed_surface
{
	/*! Parent class surface */
	av_surface_t surface;

} av_xembed_surface_t, *av_xembed_surface_p;

#ifdef __cplusplus
}
#endif

AV_API av_result_t av_xembed_surface_register_class( void );

#endif /* __AV_XEMBED_SURFACE_H */
