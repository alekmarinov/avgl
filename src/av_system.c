/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_system.c                                        */
/* Description:   Abstract system class                              */
/*                                                                   */
/*********************************************************************/

#include <avgl.h>

static av_bool_t av_system_step(av_system_p self)
{
	av_event_t event;
	if (self->input->poll_event(self->input, &event))
	{
		return AV_EVENT_QUIT != event.type;
	}
	return AV_TRUE;
}

static void av_system_loop(av_system_p self)
{
	while (self->step(self))
		self->timer->sleep_ms(10);
}

static void av_system_destructor(void* psystem)
{
	av_system_p self = (av_system_p)psystem;
}

static av_result_t av_system_constructor(av_object_p object)
{
	av_system_p self        = (av_system_p)object;
	self->audio             = AV_NULL;
	self->display           = AV_NULL;
	self->input             = AV_NULL;
	self->timer             = AV_NULL;
	self->step              = av_system_step;
	self->loop              = av_system_loop;

	return AV_OK;
}

/* Registers system class into oop */
av_result_t av_system_register_oop(av_oop_p oop)
{
	av_result_t rc;

	if (AV_OK != (rc = av_display_register_oop(oop)))
		return rc;

	if (AV_OK != (rc = av_input_register_oop(oop)))
		return rc;

	if (AV_OK != (rc = av_timer_register_oop(oop)))
		return rc;
	/*
	if (AV_OK != (rc = av_sound_register_oop(oop)))
		return rc;
	if (AV_OK != (rc = av_audio_register_oop(oop)))
		return rc;
	*/
	return oop->define_class(oop, "system", "service",
								  sizeof(av_system_t),
								  av_system_constructor, av_system_destructor);
}
