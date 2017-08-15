/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_debug.h                                         */
/*                                                                   */
/*********************************************************************/

#ifndef __AV_DEBUG_H
#define __AV_DEBUG_H

#define AV_DEBUG

#ifdef AV_DEBUG
#  include <stdio.h>
#  include <stdlib.h>
#  define av_dbg(...) { fprintf(stderr, "%s:%d ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__);  }
#  define av_assert(exp, msg) if (!(exp)) \
  { fprintf(stderr, "%s:%d assertion %s FAILED - %s\n", __FILE__, __LINE__, #exp, msg); exit(1); }
#else
#  define av_dbg(msg)
#  define av_assert(exp, msg)
#  ifndef NDEBUG
#    define NDEBUG
#  endif
#endif

#endif
