/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_stdc.h                                          */
/* Description:   Standard C API                                     */
/*                                                                   */
/*********************************************************************/

#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdlib.h>  
#include <string.h>  
#include <av_stdc.h> 

int av_strlen(const char* str)
{
	return strlen(str);
}

void av_strncpy(char* dst, const char* src, int size)
{
	strcpy(dst, src);
}

int av_strcmp(const char* str1, const char* str2)
{
	return strcmp(str1, str2);
}

char* av_strdup(const char* str)
{
	return strdup(str);
}

void av_memset(void* dst, unsigned char val, int size)
{
	memset(dst, val, size);
}
