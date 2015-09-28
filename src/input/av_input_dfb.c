/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_input_dfb.c                                     */
/* Description:   dfb input class                                    */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_DIRECTFB

#include <malloc.h>
#include <av_input.h>
#include <av_system.h>
#include "../video/av_video_dfb.h"

#define CONTEXT CONTEXT_DFB_SURFACE
#define O_context(o) O_attr(o, CONTEXT)

/* exported prototype */
av_result_t av_input_dfb_register_torba(void);

static av_bool_t av_input_dfb_translate_key(DFBInputDeviceKeyIdentifier key, av_key_t* pkey)
{
	/* Numeric keypad */
	if (key >= DIKI_KP_0 && key <= DIKI_KP_9)
	{
		*pkey = AV_KEY_0 + key - DIKI_KP_0;
		return AV_TRUE;
	}
	
	/* Function keys */
	if (key >= DIKI_F1 && key <= DIKI_F12)
	{
		*pkey = AV_KEY_F1 + key - DIKI_F1;
		return AV_TRUE;
	}

	/* Letter keys */
	if (key >= DIKI_A && key <= DIKI_Z)
	{
		*pkey = AV_KEY_A + key - DIKI_A;
		return AV_TRUE;
	}

	if (key >= DIKI_0 && key <= DIKI_9)
	{
		*pkey = AV_KEY_0 + key - DIKI_0;
		return AV_TRUE;
	}

	switch (key) 
	{
		case DIKI_QUOTE_LEFT:
			*pkey = AV_KEY_Backquote;
			return AV_TRUE;
		case DIKI_QUOTE_RIGHT:
			*pkey = AV_KEY_Quote;
			return AV_TRUE;
		case DIKI_COMMA:
			*pkey = AV_KEY_Comma;
			return AV_TRUE;
		case DIKS_MINUS_SIGN:
			*pkey = AV_KEY_Minus;
			return AV_TRUE;
		case DIKI_PERIOD:
			*pkey = AV_KEY_Period;
			return AV_TRUE;
		case DIKI_SLASH:
			*pkey = AV_KEY_Slash;
			return AV_TRUE;
		case DIKI_SEMICOLON:
			*pkey = AV_KEY_Semicolon;
			return AV_TRUE;
		case DIKI_LESS_SIGN:
			*pkey = AV_KEY_Less;
			return AV_TRUE;
		case DIKI_EQUALS_SIGN:
			*pkey = AV_KEY_Equal;
			return AV_TRUE;
		case DIKI_BRACKET_LEFT:
			*pkey = AV_KEY_Bracketleft;
			return AV_TRUE;
		case DIKI_BRACKET_RIGHT:
			*pkey = AV_KEY_Bracketright;
			return AV_TRUE;
		case DIKI_BACKSLASH:
			*pkey = AV_KEY_Backslash;
			return AV_TRUE;

		/* Numeric keypad */
		case DIKI_KP_DECIMAL:
			*pkey = AV_KEY_Period;
			return AV_TRUE;
		case DIKI_KP_DIV:
			*pkey = AV_KEY_Slash;
			return AV_TRUE;
		case DIKI_KP_MULT:
			*pkey = AV_KEY_Asterisk;
			return AV_TRUE;
		case DIKI_KP_MINUS:
			*pkey = AV_KEY_Minus;
			return AV_TRUE;
		case DIKI_KP_PLUS:
			*pkey = AV_KEY_Plus;
			return AV_TRUE;
		case DIKI_KP_ENTER:
			*pkey = AV_KEY_Return;
			return AV_TRUE;
		case DIKI_KP_EQUAL:
			*pkey = AV_KEY_Equal;
			return AV_TRUE;

		case DIKI_ESCAPE:
			*pkey = AV_KEY_Escape;
			return AV_TRUE;
		case DIKI_TAB:
			*pkey = AV_KEY_Tab;
			return AV_TRUE;
		case DIKI_ENTER:
			*pkey = AV_KEY_Return;
			return AV_TRUE;
		case DIKI_SPACE:
			*pkey = AV_KEY_Space;
			return AV_TRUE;
		case DIKI_BACKSPACE:
			*pkey = AV_KEY_Backspace;
			return AV_TRUE;
		case DIKI_INSERT:
			*pkey = AV_KEY_Insert;
			return AV_TRUE;
		case DIKI_DELETE:
			*pkey = AV_KEY_Delete;
			return AV_TRUE;
		case DIKI_PRINT:
			*pkey = AV_KEY_Print;
			return AV_TRUE;
		case DIKI_PAUSE:
			*pkey = AV_KEY_Pause;
			return AV_TRUE;

		/* Arrows + Home/End pad */
		case DIKI_UP:
			*pkey = AV_KEY_Up;
			return AV_TRUE;
		case DIKI_DOWN:
			*pkey = AV_KEY_Down;
			return AV_TRUE;
		case DIKI_RIGHT:
			*pkey = AV_KEY_Right;
			return AV_TRUE;
		case DIKI_LEFT:
			*pkey = AV_KEY_Left;
			return AV_TRUE;
		case DIKI_HOME:
			*pkey = AV_KEY_Home;
			return AV_TRUE;
		case DIKI_END:
			*pkey = AV_KEY_End;
			return AV_TRUE;
		case DIKI_PAGE_UP:
			*pkey = AV_KEY_Page_Up;
			return AV_TRUE;
		case DIKI_PAGE_DOWN:
			*pkey = AV_KEY_Page_Down;
			return AV_TRUE;

		/* Key state modifier keys */
		case DIKI_NUM_LOCK:
			*pkey = AV_KEY_Numlock;
			return AV_TRUE;	
		case DIKI_CAPS_LOCK:
			*pkey = AV_KEY_Capslock;
			return AV_TRUE;
		case DIKI_SCROLL_LOCK:
			*pkey = AV_KEY_Scrolllock;
			return AV_TRUE;
		case DIKI_SHIFT_R:
			*pkey = AV_KEY_RightShift;
			return AV_TRUE;
		case DIKI_SHIFT_L:
			*pkey = AV_KEY_LeftShift;
			return AV_TRUE;
		case DIKI_CONTROL_R:
			*pkey = AV_KEY_RightControl;
			return AV_TRUE;
		case DIKI_CONTROL_L:
			*pkey = AV_KEY_LeftControl;
			return AV_TRUE;
		case DIKI_ALT_R:
			*pkey = AV_KEY_RightAlt;
			return AV_TRUE;
		case DIKI_ALT_L:
			*pkey = AV_KEY_LeftAlt;
			return AV_TRUE;
		case DIKI_META_R:
			*pkey = AV_KEY_RightMeta;
			return AV_TRUE;
		case DIKI_META_L:
			*pkey = AV_KEY_LeftMeta;
			return AV_TRUE;
		case DIKI_SUPER_L:
			*pkey = AV_KEY_LeftSuper;
			return AV_TRUE;
		case DIKI_SUPER_R:
			*pkey = AV_KEY_RightSuper;
			return AV_TRUE;
		default:
			break;
     }
     return AV_FALSE;
}

/*
* Polls for currently pending events
*/
static av_bool_t av_input_dfb_poll_event(av_input_p self, av_event_p event)
{
	IDirectFBEventBuffer *evtbuf = (IDirectFBEventBuffer *)O_context(self);
	IDirectFB *dfb;
	IDirectFBDisplayLayer *layer;
	av_system_p sys;
	av_video_p video;

	/* get reference to system service */
	if (AV_OK != av_torb_service_addref("system", (av_service_p*)&sys))
		return AV_FALSE;
	sys->get_video(sys, &video);
	av_torb_service_release("system");

	av_assert(video, "video is not initialized");
	/* gets reference to DirectFB super interface */
	dfb = ((av_video_dfb_p)video)->dfb;
	av_assert(dfb, "DirectFB super interface is not initialized");
	/* gets reference to DirectFB Primary Display layer */
	layer = ((av_video_dfb_p)video)->primary_layer;
	av_assert(layer, "DirectFB Primary Display layer is not initialized");

	event->type = 0;
	event->flags = 0;

	if (!evtbuf)
	{
		/* create DirectFB input buffer and assign it as a context to input object */
		if (DFB_OK != dfb->CreateInputEventBuffer(dfb, DICAPS_KEYS | DICAPS_AXES | DICAPS_BUTTONS, DFB_TRUE, &evtbuf))
			return AV_FALSE;
		O_set_attr(self, CONTEXT, evtbuf);
	}

	if (DFB_OK == evtbuf->HasEvent(evtbuf))
	{
		DFBInputEvent e;
		if (DFB_OK == evtbuf->GetEvent(evtbuf, DFB_EVENT(&e)))
		{
			switch (e.type)
			{
				case DIET_AXISMOTION:
				{
					int mousex, mousey;
					layer->GetCursorPosition(layer, &mousex, &mousey);
					if (e.axis != DIAI_Z)
					{
						/* motion by x or y axis */
						av_event_init_mouse_motion(event, mousex, mousey);
					}
					else
					{
						/* motion by z axis is a mouse wheel in terms of DirectFB */
						av_event_init_mouse_button(event, AV_MOUSE_BUTTON_WHEEL,
												   (-1 == e.axisrel)?AV_MOUSE_WHEEL_UP:AV_MOUSE_WHEEL_DOWN,
												   mousex, mousey);
					}
				}
				break;
				case DIET_BUTTONPRESS:
				case DIET_BUTTONRELEASE:
				{
					int mousex, mousey;
					av_event_button_status_t button_status = (e.type == DIET_BUTTONRELEASE)?AV_BUTTON_RELEASED:AV_BUTTON_PRESSED;
					av_event_mouse_button_t mouse_button;
					switch (e.button)
					{
						case DIBI_LEFT:
							mouse_button = AV_MOUSE_BUTTON_LEFT;
						break;
						case DIBI_RIGHT:
							mouse_button = AV_MOUSE_BUTTON_RIGHT;
						break;
						case DIBI_MIDDLE:
							mouse_button = AV_MOUSE_BUTTON_MIDDLE;
						break;
						default: av_assert(0, "unhandled button type"); break;
					}
					layer->GetCursorPosition(layer, &mousex, &mousey);
					av_event_init_mouse_button(event, mouse_button, button_status, mousex, mousey);
				}
				break;
				case DIET_KEYPRESS:
				case DIET_KEYRELEASE:
				{
					av_event_button_status_t button_status = (e.type == DIET_KEYRELEASE)?AV_BUTTON_RELEASED:AV_BUTTON_PRESSED;
					av_key_t key;
					if (AV_FALSE == av_input_dfb_translate_key(e.key_id, &key))
						return AV_FALSE;
	
					av_event_init_keyboard(event, key, button_status);

					if (e.flags & DIEF_KEYSYMBOL)
					{
						/* TODO: use e.key_symbol if provided */
					}
					break;
				}
				default: return AV_FALSE;
			}
			return AV_TRUE;
		}
	}
	return AV_FALSE;
}

/*
* Push event into queue
*/
/* FIXME: implement this method */
static av_result_t av_input_dfb_push_event(struct av_input* self, av_event_p event)
{
	AV_UNUSED(self);
	AV_UNUSED(event);
	return AV_ESUPPORTED;
}

/*
* Get key modifiers
*/
/* FIXME: implement this method */
static av_keymod_t av_input_dfb_get_key_modifiers(struct av_input* self)
{
	AV_UNUSED(self);
	return AV_KEYMOD_NONE;
}

/*
* Flush pending events
*/
static int av_input_dfb_flush_events(struct av_input* self)
{
	AV_UNUSED(self);
	return 0;
}

/*
* Constructor
*/
static av_result_t av_input_dfb_constructor(av_object_p object)
{
	av_input_p self         = (av_input_p)object;
	self->poll_event        = av_input_dfb_poll_event;
	self->push_event        = av_input_dfb_push_event;
	self->get_key_modifiers = av_input_dfb_get_key_modifiers;
	self->flush_events      = av_input_dfb_flush_events;
	return AV_OK;
}

/* Registers dfb input class into TORBA class repository */
av_result_t av_input_dfb_register_torba(void)
{
	return av_torb_register_class("input_dfb", "input", sizeof(av_input_t),
								  av_input_dfb_constructor, AV_NULL);
}

#endif /* WITH_SYSTEM_DIRECTFB */
