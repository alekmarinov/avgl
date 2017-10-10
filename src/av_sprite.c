/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_sprite.c                                        */
/*                                                                   */
/*********************************************************************/ 

#include <av_sprite.h>

static void av_sprite_destructor(void* psystem)
{
}

/* constructor */
static av_result_t av_sprite_constructor(av_object_p pobject)
{
	av_sprite_p self = (av_sprite_p)pobject;

	// self->add_timer = av_timer_add_timer;

	return AV_OK;
}

av_result_t av_sprite_register_oop(av_oop_p oop)
{
	return oop->define_class(oop, "sprite", "visible", sizeof(av_sprite_t), av_sprite_constructor, av_sprite_destructor);
}
