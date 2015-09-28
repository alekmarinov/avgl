/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_timer.c                                         */
/*                                                                   */
/*********************************************************************/

#include <av_timer.h>

/* Adds callback function executed in intervals (given in milliseconds) */
static av_result_t av_timer_add_timer(struct av_timer* self,
									  av_timer_callback_p callback,
									  unsigned long delay_ms,
									  void* arg,
									  int* ptimer_id)
{
	AV_UNUSED(self);
	AV_UNUSED(callback);
	AV_UNUSED(delay_ms);
	AV_UNUSED(arg);
	AV_UNUSED(ptimer_id);
	return AV_ESUPPORTED;
}

/* Removes added timer */
static av_result_t av_timer_remove_timer(struct av_timer* self, int timer_id)
{
	AV_UNUSED(self);
	AV_UNUSED(timer_id);
	return AV_ESUPPORTED;
}

/* Sleeps for timeout given in seconds */
static void av_timer_sleep(unsigned long s)
{
	AV_UNUSED(s);
}

/* Sleeps for timeout given in milliseconds */
static void av_timer_sleep_ms(unsigned long mills)
{
	AV_UNUSED(mills);
}

/* Sleeps for timeout given in microseconds */
static void av_timer_sleep_micro(unsigned long micrs)
{
	AV_UNUSED(micrs);
}

/* Returns current time since computer started in ms */
static unsigned long av_timer_now(void)
{
	return AV_ESUPPORTED;
}

/* constructor */
static av_result_t av_timer_constructor(av_object_p pobject)
{
	av_timer_p self = (av_timer_p)pobject;

	self->add_timer    = av_timer_add_timer;
	self->remove_timer = av_timer_remove_timer;
	self->sleep        = av_timer_sleep;
	self->sleep_ms     = av_timer_sleep_ms;
	self->sleep_micro  = av_timer_sleep_micro;
	self->now          = av_timer_now;

	return AV_OK;
}

av_result_t av_timer_register_torba(void)
{
	return av_torb_register_class("timer", AV_NULL, sizeof(av_timer_t), av_timer_constructor, AV_NULL);
}
