/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_stdc.h                                          */
/* Description:   Standard C API                                     */
/*                                                                   */
/*********************************************************************/

#ifndef __AV_STDC_H
#define __AV_STDC_H

#include <av.h>

#ifdef _MSC_VER
# define strncpy(d, s, c) strncpy_s(d, strlen(d), s, c);
# ifdef _DEBUG
#  define _CRTDBG_MAP_ALLOC 
#  include <crtdbg.h>  
# endif
#endif

#define av_malloc malloc
#define av_calloc calloc
#define av_free free

int av_strlen(const char* str);
void av_strncpy(char* dst, const char* src, int dstsize);
int av_strcmp(const char* str1, const char* str2);
char* av_strdup(const char* str);
void av_memset(void* dst, unsigned char val, int size);

#endif /* __AV_STDC_H */
