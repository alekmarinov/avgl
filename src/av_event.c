/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_event.c                                         */
/* Description:   Event class                                        */
/*                                                                   */
/*********************************************************************/

/*! \file av_event.c
*   \brief Implementation of av_event class
*/

#include <av_event.h>

void av_event_init_keyboard(av_event_p event, av_key_t key, av_event_button_status_t button_status)
{
	event->type = AV_EVENT_KEYBOARD;
	event->flags = AV_EVENT_FLAGS_KEY | AV_EVENT_FLAGS_BUTTON_STATUS;
	event->key = key;
	event->button_status = button_status;
	event->data = AV_NULL;
}

void av_event_init_mouse_motion(av_event_p event, int mouse_x, int mouse_y)
{
	event->type = AV_EVENT_MOUSE_MOTION;
	event->flags = AV_EVENT_FLAGS_MOUSE_XY;
	event->mouse_x = mouse_x;
	event->mouse_y = mouse_y;
	event->data = AV_NULL;
}

void av_event_init_mouse_enter(av_event_p event, int mouse_x, int mouse_y)
{
	event->type = AV_EVENT_MOUSE_ENTER;
	event->flags = AV_EVENT_FLAGS_MOUSE_XY;
	event->mouse_x = mouse_x;
	event->mouse_y = mouse_y;
	event->data = AV_NULL;
}

void av_event_init_mouse_hover(av_event_p event, int mouse_x, int mouse_y)
{
	event->type = AV_EVENT_MOUSE_HOVER;
	event->flags = AV_EVENT_FLAGS_MOUSE_XY;
	event->mouse_x = mouse_x;
	event->mouse_y = mouse_y;
	event->data = AV_NULL;
}

void av_event_init_mouse_leave(av_event_p event, int mouse_x, int mouse_y)
{
	event->type = AV_EVENT_MOUSE_LEAVE;
	event->flags = AV_EVENT_FLAGS_MOUSE_XY;
	event->mouse_x = mouse_x;
	event->mouse_y = mouse_y;
	event->data = AV_NULL;
}

void av_event_init_mouse_button(av_event_p event,
								av_event_mouse_button_t mouse_button,
								av_event_button_status_t button_status,
								int mousex,
								int mousey)
{
	event->type = AV_EVENT_MOUSE_BUTTON;
	event->flags = AV_EVENT_FLAGS_MOUSE_BUTTON  | AV_EVENT_FLAGS_BUTTON_STATUS | AV_EVENT_FLAGS_MOUSE_XY;
	event->mouse_button = mouse_button;
	event->button_status = button_status;
	event->mouse_x = mousex;
	event->mouse_y = mousey;
	event->data = AV_NULL;
}

