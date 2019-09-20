/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2018,  Alexander Marinov                            */
/*                                                                   */
/* Project:       lavgl                                              */
/* Filename:      lavgl.c                                            */
/* Description:   Main binding interface                             */
/*                                                                   */
/*********************************************************************/

#ifndef __LAVGL_H
#define __LAVGL_H

#include <lua.h>
#include <lauxlib.h>
#include <avgl.h>

/*!
* Defines API exportation type
*/
#ifndef LAVGL_API
#  ifdef _WIN32
#    ifndef LAVGL_STATIC_LIB
#      ifdef LAVGL_BUILD
#        define LAVGL_API __declspec (dllexport)
#      else
#        define LAVGL_API __declspec (dllimport)
#      endif
#    else
#      define LAVGL_API
#    endif
#  else
#    define LAVGL_API extern
#  endif
#endif

#define LAVGL_PACKAGE "avgl"

#endif /* __LAVGL_H */
