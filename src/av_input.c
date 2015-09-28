/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_input.c                                         */
/* Description:   Abstract input class                               */
/*                                                                   */
/*********************************************************************/

#include <av_input.h>
#include <av_system.h>

/*
* Polls for currently pending events
*/
static av_bool_t av_input_poll_event(av_input_p self, av_event_p event)
{
	AV_UNUSED(self);
	AV_UNUSED(event);
	return AV_FALSE;
}

/*
* Push event into queue
*/
static av_result_t av_input_push_event(struct av_input* self, av_event_p event)
{
	AV_UNUSED(self);
	AV_UNUSED(event);
	return AV_ESUPPORTED;
}

/*
* Get key modifiers
*/
static av_keymod_t av_input_get_key_modifiers(struct av_input* self)
{
	AV_UNUSED(self);
	return AV_KEYMOD_NONE;
}

/* Constructor */
static av_result_t av_input_constructor(av_object_p pobject)
{
	av_input_p self         = (av_input_p)pobject;
	self->poll_event        = av_input_poll_event;
	self->push_event        = av_input_push_event;
	self->get_key_modifiers = av_input_get_key_modifiers;
	return AV_OK;
}

/* Registers input class into TORBA class repository */
av_result_t av_input_register_torba(void)
{
	return av_torb_register_class("input", AV_NULL, sizeof(av_input_t), av_input_constructor, AV_NULL);
}
