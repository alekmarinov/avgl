/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_bitmap.c                                        */
/* Description:   Abstract bitmap class                              */
/*                                                                   */
/*********************************************************************/

#include <av_bitmap.h>

/* set bitmap width and height */
static av_result_t av_bitmap_set_size(av_bitmap_t* self, int width, int height)
{
	AV_UNUSED(self);
	AV_UNUSED(width);
	AV_UNUSED(height);
	return AV_ESUPPORTED;
}

/* get bitmap width and height */
static av_result_t av_bitmap_get_size(av_bitmap_t* self, int* pwidth, int* pheight)
{
	AV_UNUSED(self);
	AV_UNUSED(pwidth);
	AV_UNUSED(pheight);
	return AV_ESUPPORTED;
}

static av_result_t av_bitmap_load(av_bitmap_t* self, const char* filename)
{
	AV_UNUSED(self);
	AV_UNUSED(filename);
	return AV_ESUPPORTED;
}

/* Initializes memory given by the input pointer with the bitmap's class information */
static av_result_t av_bitmap_constructor(av_object_p object)
{
	av_bitmap_p self = (av_bitmap_p)object;
	self->set_size   = av_bitmap_set_size;
	self->get_size   = av_bitmap_get_size;
	self->load       = av_bitmap_load;
	return AV_OK;
}

/*	Registers bitmap class into OOP class repository */
av_result_t av_bitmap_register_oop(av_oop_p oop)
{
	return oop->define_class(oop, "bitmap", AV_NULL, sizeof(av_bitmap_t), av_bitmap_constructor, AV_NULL);
}
