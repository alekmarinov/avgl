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
#include <av_debug.h>

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

const char* mouse_button_to_str(av_event_mouse_button_t mouse_button)
{
	switch (mouse_button)
	{
	case AV_MOUSE_BUTTON_LEFT: return "left";
	case AV_MOUSE_BUTTON_RIGHT: return "right";
	case AV_MOUSE_BUTTON_MIDDLE: return "middle";
	case AV_MOUSE_BUTTON_WHEEL: return "wheel";
	case AV_MOUSE_BUTTON_X1: return "x1";
	case AV_MOUSE_BUTTON_X2: return "x2";
	}
	return "unknown";
}

const char* key_to_str(int key)
{
#define KEY_STR_CASE(n) case n: return #n;

	switch (key)
	{
	KEY_STR_CASE(AV_KEY_Backspace)
	KEY_STR_CASE(AV_KEY_Tab)
	KEY_STR_CASE(AV_KEY_Return)
	KEY_STR_CASE(AV_KEY_Escape)
	KEY_STR_CASE(AV_KEY_Space)
	KEY_STR_CASE(AV_KEY_Exclam)
	KEY_STR_CASE(AV_KEY_Quotedbl)
	KEY_STR_CASE(AV_KEY_Hash)
	KEY_STR_CASE(AV_KEY_Dollar)
	KEY_STR_CASE(AV_KEY_Percent)
	KEY_STR_CASE(AV_KEY_Ampersand)
	KEY_STR_CASE(AV_KEY_Quote)
	KEY_STR_CASE(AV_KEY_Parenleft)
	KEY_STR_CASE(AV_KEY_Parenright)
	KEY_STR_CASE(AV_KEY_Asterisk)
	KEY_STR_CASE(AV_KEY_Plus)
	KEY_STR_CASE(AV_KEY_Comma)
	KEY_STR_CASE(AV_KEY_Minus)
	KEY_STR_CASE(AV_KEY_Period)
	KEY_STR_CASE(AV_KEY_Slash)
	KEY_STR_CASE(AV_KEY_0)
	KEY_STR_CASE(AV_KEY_1)
	KEY_STR_CASE(AV_KEY_2)
	KEY_STR_CASE(AV_KEY_3)
	KEY_STR_CASE(AV_KEY_4)
	KEY_STR_CASE(AV_KEY_5)
	KEY_STR_CASE(AV_KEY_6)
	KEY_STR_CASE(AV_KEY_7)
	KEY_STR_CASE(AV_KEY_8)
	KEY_STR_CASE(AV_KEY_9)
	KEY_STR_CASE(AV_KEY_Colon)
	KEY_STR_CASE(AV_KEY_Semicolon)
	KEY_STR_CASE(AV_KEY_Less)
	KEY_STR_CASE(AV_KEY_Equal)
	KEY_STR_CASE(AV_KEY_Greater)
	KEY_STR_CASE(AV_KEY_Question)
	KEY_STR_CASE(AV_KEY_At)
	KEY_STR_CASE(AV_KEY_A)
	KEY_STR_CASE(AV_KEY_B)
	KEY_STR_CASE(AV_KEY_C)
	KEY_STR_CASE(AV_KEY_D)
	KEY_STR_CASE(AV_KEY_E)
	KEY_STR_CASE(AV_KEY_F)
	KEY_STR_CASE(AV_KEY_G)
	KEY_STR_CASE(AV_KEY_H)
	KEY_STR_CASE(AV_KEY_I)
	KEY_STR_CASE(AV_KEY_J)
	KEY_STR_CASE(AV_KEY_K)
	KEY_STR_CASE(AV_KEY_L)
	KEY_STR_CASE(AV_KEY_M)
	KEY_STR_CASE(AV_KEY_N)
	KEY_STR_CASE(AV_KEY_O)
	KEY_STR_CASE(AV_KEY_P)
	KEY_STR_CASE(AV_KEY_Q)
	KEY_STR_CASE(AV_KEY_R)
	KEY_STR_CASE(AV_KEY_S)
	KEY_STR_CASE(AV_KEY_T)
	KEY_STR_CASE(AV_KEY_U)
	KEY_STR_CASE(AV_KEY_V)
	KEY_STR_CASE(AV_KEY_W)
	KEY_STR_CASE(AV_KEY_X)
	KEY_STR_CASE(AV_KEY_Y)
	KEY_STR_CASE(AV_KEY_Z)
	KEY_STR_CASE(AV_KEY_Bracketleft)
	KEY_STR_CASE(AV_KEY_Backslash)
	KEY_STR_CASE(AV_KEY_Bracketright)
	KEY_STR_CASE(AV_KEY_Caret)
	KEY_STR_CASE(AV_KEY_Underscore)
	KEY_STR_CASE(AV_KEY_Backquote)
	KEY_STR_CASE(AV_KEY_a)
	KEY_STR_CASE(AV_KEY_b)
	KEY_STR_CASE(AV_KEY_c)
	KEY_STR_CASE(AV_KEY_d)
	KEY_STR_CASE(AV_KEY_e)
	KEY_STR_CASE(AV_KEY_f)
	KEY_STR_CASE(AV_KEY_g)
	KEY_STR_CASE(AV_KEY_h)
	KEY_STR_CASE(AV_KEY_i)
	KEY_STR_CASE(AV_KEY_j)
	KEY_STR_CASE(AV_KEY_k)
	KEY_STR_CASE(AV_KEY_l)
	KEY_STR_CASE(AV_KEY_m)
	KEY_STR_CASE(AV_KEY_n)
	KEY_STR_CASE(AV_KEY_o)
	KEY_STR_CASE(AV_KEY_p)
	KEY_STR_CASE(AV_KEY_q)
	KEY_STR_CASE(AV_KEY_r)
	KEY_STR_CASE(AV_KEY_s)
	KEY_STR_CASE(AV_KEY_t)
	KEY_STR_CASE(AV_KEY_u)
	KEY_STR_CASE(AV_KEY_v)
	KEY_STR_CASE(AV_KEY_w)
	KEY_STR_CASE(AV_KEY_x)
	KEY_STR_CASE(AV_KEY_y)
	KEY_STR_CASE(AV_KEY_z)
	KEY_STR_CASE(AV_KEY_Braceleft)
	KEY_STR_CASE(AV_KEY_Bar)
	KEY_STR_CASE(AV_KEY_Braceright)
	KEY_STR_CASE(AV_KEY_Asciitilde)
	KEY_STR_CASE(AV_KEY_Delete)
	KEY_STR_CASE(AV_KEY_Left)
	KEY_STR_CASE(AV_KEY_Right)
	KEY_STR_CASE(AV_KEY_Up)
	KEY_STR_CASE(AV_KEY_Down)
	KEY_STR_CASE(AV_KEY_Insert)
	KEY_STR_CASE(AV_KEY_Home)
	KEY_STR_CASE(AV_KEY_End)
	KEY_STR_CASE(AV_KEY_Page_Up)
	KEY_STR_CASE(AV_KEY_Page_Down)
	KEY_STR_CASE(AV_KEY_Print)
	KEY_STR_CASE(AV_KEY_Pause)
	KEY_STR_CASE(AV_KEY_Ok)
	KEY_STR_CASE(AV_KEY_Select)
	KEY_STR_CASE(AV_KEY_Goto)
	KEY_STR_CASE(AV_KEY_Clear)
	KEY_STR_CASE(AV_KEY_Power)
	KEY_STR_CASE(AV_KEY_Power2)
	KEY_STR_CASE(AV_KEY_Option)
	KEY_STR_CASE(AV_KEY_Menu)
	KEY_STR_CASE(AV_KEY_Help)
	KEY_STR_CASE(AV_KEY_Info)
	KEY_STR_CASE(AV_KEY_Time)
	KEY_STR_CASE(AV_KEY_Vendor)
	KEY_STR_CASE(AV_KEY_Album)
	KEY_STR_CASE(AV_KEY_Program)
	KEY_STR_CASE(AV_KEY_Channel)
	KEY_STR_CASE(AV_KEY_Favorites)
	KEY_STR_CASE(AV_KEY_EPG)
	KEY_STR_CASE(AV_KEY_PVR)
	KEY_STR_CASE(AV_KEY_MHP)
	KEY_STR_CASE(AV_KEY_Language)
	KEY_STR_CASE(AV_KEY_Title)
	KEY_STR_CASE(AV_KEY_Subtitle)
	KEY_STR_CASE(AV_KEY_Angle)
	KEY_STR_CASE(AV_KEY_Zoom)
	KEY_STR_CASE(AV_KEY_Mode)
	KEY_STR_CASE(AV_KEY_Keyboard)
	KEY_STR_CASE(AV_KEY_Pc)
	KEY_STR_CASE(AV_KEY_Screen)
	KEY_STR_CASE(AV_KEY_Tv)
	KEY_STR_CASE(AV_KEY_Tv2)
	KEY_STR_CASE(AV_KEY_Vcr2)
	KEY_STR_CASE(AV_KEY_Sat)
	KEY_STR_CASE(AV_KEY_Sat2)
	KEY_STR_CASE(AV_KEY_CD)
	KEY_STR_CASE(AV_KEY_Tape)
	KEY_STR_CASE(AV_KEY_Radio)
	KEY_STR_CASE(AV_KEY_Tuner)
	KEY_STR_CASE(AV_KEY_Player)
	KEY_STR_CASE(AV_KEY_Text)
	KEY_STR_CASE(AV_KEY_DVD)
	KEY_STR_CASE(AV_KEY_Aux)
	KEY_STR_CASE(AV_KEY_MP3)
	KEY_STR_CASE(AV_KEY_Phone)
	KEY_STR_CASE(AV_KEY_Audio)
	KEY_STR_CASE(AV_KEY_Video)
	KEY_STR_CASE(AV_KEY_Internet)
	KEY_STR_CASE(AV_KEY_List)
	KEY_STR_CASE(AV_KEY_Green)
	KEY_STR_CASE(AV_KEY_Yellow)
	KEY_STR_CASE(AV_KEY_Channel_Up)
	KEY_STR_CASE(AV_KEY_Channel_Down)
	KEY_STR_CASE(AV_KEY_Back)
	KEY_STR_CASE(AV_KEY_Forward)
	KEY_STR_CASE(AV_KEY_First)
	KEY_STR_CASE(AV_KEY_Last)
	KEY_STR_CASE(AV_KEY_Volume_Up)
	KEY_STR_CASE(AV_KEY_Volume_Down)
	KEY_STR_CASE(AV_KEY_Mute)
	KEY_STR_CASE(AV_KEY_AB)
	KEY_STR_CASE(AV_KEY_Playpause)
	KEY_STR_CASE(AV_KEY_PLAY)
	KEY_STR_CASE(AV_KEY_Stop)
	KEY_STR_CASE(AV_KEY_Restart)
	KEY_STR_CASE(AV_KEY_Slow)
	KEY_STR_CASE(AV_KEY_Fast)
	KEY_STR_CASE(AV_KEY_Record)
	KEY_STR_CASE(AV_KEY_Eject)
	KEY_STR_CASE(AV_KEY_Shuffle)
	KEY_STR_CASE(AV_KEY_Rewind)
	KEY_STR_CASE(AV_KEY_Fastforward)
	KEY_STR_CASE(AV_KEY_Previous)
	KEY_STR_CASE(AV_KEY_Next)
	KEY_STR_CASE(AV_KEY_Begin)
	KEY_STR_CASE(AV_KEY_Digits)
	KEY_STR_CASE(AV_KEY_Teen)
	KEY_STR_CASE(AV_KEY_Twen)
	KEY_STR_CASE(AV_KEY_Break)
	KEY_STR_CASE(AV_KEY_Exit)
	KEY_STR_CASE(AV_KEY_Setup)
	KEY_STR_CASE(AV_KEY_F1)
	KEY_STR_CASE(AV_KEY_F2)
	KEY_STR_CASE(AV_KEY_F3)
	KEY_STR_CASE(AV_KEY_F4)
	KEY_STR_CASE(AV_KEY_F5)
	KEY_STR_CASE(AV_KEY_F6)
	KEY_STR_CASE(AV_KEY_F7)
	KEY_STR_CASE(AV_KEY_F8)
	KEY_STR_CASE(AV_KEY_F9)
	KEY_STR_CASE(AV_KEY_F10)
	KEY_STR_CASE(AV_KEY_F11)
	KEY_STR_CASE(AV_KEY_F12)
	KEY_STR_CASE(AV_KEY_Numlock)
	KEY_STR_CASE(AV_KEY_Capslock)
	KEY_STR_CASE(AV_KEY_Scrolllock)
	KEY_STR_CASE(AV_KEY_RightShift)
	KEY_STR_CASE(AV_KEY_LeftShift)
	KEY_STR_CASE(AV_KEY_RightControl)
	KEY_STR_CASE(AV_KEY_LeftControl)
	KEY_STR_CASE(AV_KEY_RightAlt)
	KEY_STR_CASE(AV_KEY_LeftAlt)
	}
	return "unknown";
}

void av_event_dbg(av_event_p event)
{
	switch (event->type)
	{
	case AV_EVENT_QUIT:
		av_dbg("quit\n")
	break;
	case AV_EVENT_KEYBOARD:
		av_dbg("%s %s\n", event->button_status ? "pressed" : "released", key_to_str(event->key));
	break;
	case AV_EVENT_MOUSE_BUTTON:
		av_dbg("%s %s\n", event->button_status ? "pressed" : "released", mouse_button_to_str(event->mouse_button));
	break;
	case AV_EVENT_MOUSE_MOTION:
		av_dbg("mouse move at %d %d\n", event->mouse_x, event->mouse_y);
	break;
	case AV_EVENT_MOUSE_ENTER:
		av_dbg("mouse enter at %d %d\n", event->mouse_x, event->mouse_y);
	break;
	case AV_EVENT_MOUSE_HOVER:
		av_dbg("mouse hover at %d %d\n", event->mouse_x, event->mouse_y);
	break;
	case AV_EVENT_MOUSE_LEAVE:
		av_dbg("mouse leave at %d %d\n", event->mouse_x, event->mouse_y);
	break;
	case AV_EVENT_UPDATE:
		av_dbg("update\n")
	break;
	case AV_EVENT_FOCUS:
		av_dbg("focus window %p\n", event->window);
	break;
	case AV_EVENT_USER:
		av_dbg("user %d with data = %p\n", event->user_id, event->data);
	break;
	}
}
