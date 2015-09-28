/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_input_sdl.c                                     */
/* Description:   SDL input class                                    */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_SDL

#include <av_input.h>
#include <SDL.h>

/* exported prototype */
av_result_t av_input_sdl_register_torba(void);

enum
{
	KEY_SDL_TO_AVGL = AV_TRUE,
	KEY_AVGL_TO_SDL = AV_FALSE
};

static av_bool_t av_input_translate_key(SDLKey *skey, av_key_t* akey, av_bool_t toavgl)
{
	/* Numeric keys */
	if (toavgl)
	{
		if (*skey >= SDLK_0 && *skey <= SDLK_9)
		{
			*akey = AV_KEY_0 + *skey - SDLK_0;
			return AV_TRUE;
		}
	}
	else
	{
		if (*akey >= AV_KEY_0 && *akey <= AV_KEY_9)
		{
			*skey = SDLK_0 + *akey - AV_KEY_0;
			return AV_TRUE;
		}
	}

	/* Numeric keypad */
	if (toavgl)
	{
		if (*skey >= SDLK_KP0 && *skey <= SDLK_KP9)
		{
			*akey = AV_KEY_0 + *skey - SDLK_KP0;
			return AV_TRUE;
		}
	}
	else
	{
		if (*akey >= AV_KEY_0 && *akey <= AV_KEY_9)
		{
			*skey = SDLK_KP0 + *akey - AV_KEY_0;
			return AV_TRUE;
		}
	}

	/* Function keys */
	if (toavgl)
	{
		if (*skey >= SDLK_F1 && *skey <= SDLK_F12)
		{
			*akey = AV_KEY_F1 + *skey - SDLK_F1;
			return AV_TRUE;
		}
	}
	else
	{
		if (*akey >= AV_KEY_F1 && *akey <= AV_KEY_F12)
		{
			*skey = SDLK_F1 + *akey - AV_KEY_F1;
			return AV_TRUE;
		}
	}

	/* Lower letter keys */
	if (toavgl)
	{
		if (*skey >= SDLK_a && *skey <= SDLK_z)
		{
			*akey = AV_KEY_a + *skey - SDLK_a;
			return AV_TRUE;
		}
	}
	else
	{
		if (*akey >= AV_KEY_a && *akey <= AV_KEY_z)
		{
			*skey = SDLK_a + *akey - SDLK_z;
			return AV_TRUE;
		}
	}

	if (toavgl)
	{
		switch (*skey)
		{
			case SDLK_QUOTE:
				*akey = AV_KEY_Quote;
				return AV_TRUE;
			case SDLK_BACKQUOTE:
				*akey = AV_KEY_Backquote;
				return AV_TRUE;
			case SDLK_COMMA:
				*akey = AV_KEY_Comma;
				return AV_TRUE;
			case SDLK_MINUS:
				*akey = AV_KEY_Minus;
				return AV_TRUE;
			case SDLK_PERIOD:
				*akey = AV_KEY_Period;
				return AV_TRUE;
			case SDLK_SLASH:
				*akey = AV_KEY_Slash;
				return AV_TRUE;
			case SDLK_SEMICOLON:
				*akey = AV_KEY_Semicolon;
				return AV_TRUE;
			case SDLK_LESS:
				*akey = AV_KEY_Less;
				return AV_TRUE;
			case SDLK_EQUALS:
				*akey = AV_KEY_Equal;
				return AV_TRUE;
			case SDLK_LEFTBRACKET:
				*akey = AV_KEY_Bracketleft;
				return AV_TRUE;
			case SDLK_RIGHTBRACKET:
				*akey = AV_KEY_Bracketright;
				return AV_TRUE;
			case SDLK_BACKSLASH:
				*akey = AV_KEY_Backslash;
				return AV_TRUE;

			/* Numeric keypad */
			case SDLK_KP_PERIOD:
				*akey = AV_KEY_Period;
				return AV_TRUE;
			case SDLK_KP_DIVIDE:
				*akey = AV_KEY_Slash;
				return AV_TRUE;
			case SDLK_KP_MULTIPLY:
				*akey = AV_KEY_Asterisk;
				return AV_TRUE;
			case SDLK_KP_MINUS:
				*akey = AV_KEY_Minus;
				return AV_TRUE;
			case SDLK_KP_PLUS:
				*akey = AV_KEY_Plus;
				return AV_TRUE;
			case SDLK_KP_ENTER:
				*akey = AV_KEY_Return;
				return AV_TRUE;
			case SDLK_KP_EQUALS:
				*akey = AV_KEY_Equal;
				return AV_TRUE;

			case SDLK_ESCAPE:
				*akey = AV_KEY_Escape;
				return AV_TRUE;
			case SDLK_TAB:
				*akey = AV_KEY_Tab;
				return AV_TRUE;
			case SDLK_RETURN:
				*akey = AV_KEY_Return;
				return AV_TRUE;
			case SDLK_SPACE:
				*akey = AV_KEY_Space;
				return AV_TRUE;
			case SDLK_BACKSPACE:
				*akey = AV_KEY_Backspace;
				return AV_TRUE;
			case SDLK_INSERT:
				*akey = AV_KEY_Insert;
				return AV_TRUE;
			case SDLK_DELETE:
				*akey = AV_KEY_Delete;
				return AV_TRUE;
			case SDLK_PRINT:
				*akey = AV_KEY_Print;
				return AV_TRUE;
			case SDLK_PAUSE:
				*akey = AV_KEY_Pause;
				return AV_TRUE;

			/* Arrows + Home/End pad */
			case SDLK_UP:
				*akey = AV_KEY_Up;
				return AV_TRUE;
			case SDLK_DOWN:
				*akey = AV_KEY_Down;
				return AV_TRUE;
			case SDLK_RIGHT:
				*akey = AV_KEY_Right;
				return AV_TRUE;
			case SDLK_LEFT:
				*akey = AV_KEY_Left;
				return AV_TRUE;
			case SDLK_HOME:
				*akey = AV_KEY_Home;
				return AV_TRUE;
			case SDLK_END:
				*akey = AV_KEY_End;
				return AV_TRUE;
			case SDLK_PAGEUP:
				*akey = AV_KEY_Page_Up;
				return AV_TRUE;
			case SDLK_PAGEDOWN:
				*akey = AV_KEY_Page_Down;
				return AV_TRUE;

			/* Key state modifier keys */
			case SDLK_NUMLOCK:
				*akey = AV_KEY_Numlock;
				return AV_TRUE;
			case SDLK_CAPSLOCK:
				*akey = AV_KEY_Capslock;
				return AV_TRUE;
			case SDLK_SCROLLOCK:
				*akey = AV_KEY_Scrolllock;
				return AV_TRUE;
			case SDLK_RSHIFT:
				*akey = AV_KEY_RightShift;
				return AV_TRUE;
			case SDLK_LSHIFT:
				*akey = AV_KEY_LeftShift;
				return AV_TRUE;
			case SDLK_RCTRL:
				*akey = AV_KEY_RightControl;
				return AV_TRUE;
			case SDLK_LCTRL:
				*akey = AV_KEY_LeftControl;
				return AV_TRUE;
			case SDLK_RALT:
				*akey = AV_KEY_RightAlt;
				return AV_TRUE;
			case SDLK_LALT:
				*akey = AV_KEY_LeftAlt;
				return AV_TRUE;
			case SDLK_RMETA:
				*akey = AV_KEY_RightMeta;
				return AV_TRUE;
			case SDLK_LMETA:
				*akey = AV_KEY_LeftMeta;
				return AV_TRUE;
			case SDLK_LSUPER:
				*akey = AV_KEY_LeftSuper;
				return AV_TRUE;
			case SDLK_RSUPER:
				*akey = AV_KEY_RightSuper;
				return AV_TRUE;
			default:
				break;
		}
	}
	else
	{
		switch (*akey)
		{
			case AV_KEY_Quote:
				*skey = SDLK_QUOTE;
				return AV_TRUE;
			case AV_KEY_Backquote:
				*skey = SDLK_BACKQUOTE;
				return AV_TRUE;
			case AV_KEY_Comma:
				*skey = SDLK_COMMA;
				return AV_TRUE;
			case AV_KEY_Minus:
				*skey = SDLK_MINUS;
				return AV_TRUE;
			case AV_KEY_Period:
				*skey = SDLK_PERIOD;
				return AV_TRUE;
			case AV_KEY_Slash:
				*skey = SDLK_SLASH;
				return AV_TRUE;
			case AV_KEY_Semicolon:
				*skey = SDLK_SEMICOLON;
				return AV_TRUE;
			case AV_KEY_Less:
				*skey = SDLK_LESS;
				return AV_TRUE;
			case AV_KEY_Equal:
				*skey = SDLK_EQUALS;
				return AV_TRUE;
			case AV_KEY_Bracketleft:
				*skey = SDLK_LEFTBRACKET;
				return AV_TRUE;
			case AV_KEY_Bracketright:
				*skey = SDLK_RIGHTBRACKET;
				return AV_TRUE;
			case AV_KEY_Backslash:
				*skey = SDLK_BACKSLASH;
				return AV_TRUE;

			/* Numeric keypad */
			case AV_KEY_Asterisk:
				*skey = SDLK_KP_MULTIPLY;
				return AV_TRUE;
			case AV_KEY_Plus:
				*skey = SDLK_KP_PLUS;
				return AV_TRUE;

			case AV_KEY_Escape:
				*skey = SDLK_ESCAPE;
				return AV_TRUE;
			case AV_KEY_Tab:
				*skey = SDLK_TAB;
				return AV_TRUE;
			case AV_KEY_Return:
				*skey = SDLK_RETURN;
				return AV_TRUE;
			case AV_KEY_Space:
				*skey = SDLK_SPACE;
				return AV_TRUE;
			case AV_KEY_Backspace:
				*skey = SDLK_BACKSPACE;
				return AV_TRUE;
			case AV_KEY_Insert:
				*skey = SDLK_INSERT;
				return AV_TRUE;
			case AV_KEY_Delete:
				*skey = SDLK_DELETE;
				return AV_TRUE;
			case AV_KEY_Print:
				*skey = SDLK_PRINT;
				return AV_TRUE;
			case AV_KEY_Pause:
				*skey = SDLK_PAUSE;
				return AV_TRUE;

			/* Arrows + Home/End pad */
			case AV_KEY_Up:
				*skey = SDLK_UP;
				return AV_TRUE;
			case AV_KEY_Down:
				*skey = SDLK_DOWN;
				return AV_TRUE;
			case AV_KEY_Right:
				*skey = SDLK_RIGHT;
				return AV_TRUE;
			case AV_KEY_Left:
				*skey = SDLK_LEFT;
				return AV_TRUE;
			case AV_KEY_Home:
				*skey = SDLK_HOME;
				return AV_TRUE;
			case AV_KEY_End:
				*skey = SDLK_END;
				return AV_TRUE;
			case AV_KEY_Page_Up:
				*skey = SDLK_PAGEUP;
				return AV_TRUE;
			case AV_KEY_Page_Down:
				*skey = SDLK_PAGEDOWN;
				return AV_TRUE;

			/* Key state modifier keys */
			case AV_KEY_Numlock:
				*skey = SDLK_NUMLOCK;
				return AV_TRUE;
			case AV_KEY_Capslock:
				*skey = SDLK_CAPSLOCK;
				return AV_TRUE;
			case AV_KEY_Scrolllock:
				*skey = SDLK_SCROLLOCK;
				return AV_TRUE;
			case AV_KEY_RightShift:
				*skey = SDLK_RSHIFT;
				return AV_TRUE;
			case AV_KEY_LeftShift:
				*skey = SDLK_LSHIFT;
				return AV_TRUE;
			case AV_KEY_RightControl:
				*skey = SDLK_RCTRL;
				return AV_TRUE;
			case AV_KEY_LeftControl:
				*skey = SDLK_LCTRL;
				return AV_TRUE;
			case AV_KEY_RightAlt:
				*skey = SDLK_RALT;
				return AV_TRUE;
			case AV_KEY_LeftAlt:
				*skey = SDLK_LALT;
				return AV_TRUE;
			case AV_KEY_RightMeta:
				*skey = SDLK_RMETA;
				return AV_TRUE;
			case AV_KEY_LeftMeta:
				*skey = SDLK_LMETA;
				return AV_TRUE;
			case AV_KEY_LeftSuper:
				*skey = SDLK_LSUPER;
				return AV_TRUE;
			case AV_KEY_RightSuper:
				*skey = SDLK_RSUPER;
				return AV_TRUE;
			default:
				break;
     	}
	}
	return AV_FALSE;
}

/*
* Polls for currently pending events
* \return AV_TRUE if there are any pending events, AV_FALSE otherwise
*/
static av_bool_t av_input_sdl_poll_event(av_input_p self, av_event_p event)
{
	SDL_Event e;
	AV_UNUSED(self);
	event->type = 0;
	event->flags = 0;
	if (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
			case SDL_MOUSEMOTION:
				av_event_init_mouse_motion(event, e.motion.x, e.motion.y);
			break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			{
				int mousex, mousey;
				av_event_button_status_t button_status = (e.type == SDL_MOUSEBUTTONUP)?AV_BUTTON_RELEASED:AV_BUTTON_PRESSED;
				av_event_mouse_button_t mouse_button;
				switch (e.button.button)
				{
					case SDL_BUTTON_LEFT:
						mouse_button = AV_MOUSE_BUTTON_LEFT;
					break;
					case SDL_BUTTON_RIGHT:
						mouse_button = AV_MOUSE_BUTTON_RIGHT;
					break;
					case SDL_BUTTON_MIDDLE:
						mouse_button = AV_MOUSE_BUTTON_MIDDLE;
					break;
					case SDL_BUTTON_WHEELUP:
					case SDL_BUTTON_WHEELDOWN:
						if (e.type == SDL_MOUSEBUTTONDOWN)
						{
							mouse_button = AV_MOUSE_BUTTON_WHEEL;
							button_status = (e.button.button == SDL_BUTTON_WHEELUP)?AV_BUTTON_RELEASED:AV_BUTTON_PRESSED;
						}
						else return AV_FALSE;
					break;
					case SDL_BUTTON_X1:
						mouse_button = AV_MOUSE_BUTTON_X1;
					break;
					case SDL_BUTTON_X2:
						mouse_button = AV_MOUSE_BUTTON_X2;
					break;
					default: return AV_FALSE;
				}
				SDL_GetMouseState(&mousex, &mousey);
				av_event_init_mouse_button(event, mouse_button, button_status, mousex, mousey);
			}
			break;
			case SDL_KEYUP:
			case SDL_KEYDOWN:
			{
				av_event_button_status_t button_status = (e.type == SDL_KEYUP)?AV_BUTTON_RELEASED:AV_BUTTON_PRESSED;
				av_key_t key;
				if (0 == e.key.keysym.unicode)
				{
					if (AV_FALSE == av_input_translate_key(&e.key.keysym.sym, &key, KEY_SDL_TO_AVGL))
						return AV_FALSE;
				}
				else
				{
					key = e.key.keysym.unicode;
				}

				av_event_init_keyboard(event, key, button_status);

				/* TODO: use e.key.keysym.unicode if provided */
			}
			break;
			case SDL_QUIT:
			{
				event->type = AV_EVENT_QUIT;
				event->flags = 0;
			}
			break;
			case SDL_USEREVENT:
			{
				if (e.user.code < AV_EVENT_USER)
				{
					event->type = e.user.code;
					event->flags = AV_EVENT_FLAGS_NONE;
				}
				else
				{
					event->type = AV_EVENT_USER;
					event->user_id = e.user.code & ~AV_EVENT_USER;
					event->data = e.user.data1;
					event->window = e.user.data2;
					event->flags = AV_EVENT_FLAGS_USER_ID | AV_EVENT_FLAGS_WINDOW | AV_EVENT_FLAGS_DATA;
				}
			}
			break;
			default:
				break;
		}
		return AV_TRUE;
	}
	return AV_FALSE;
}

/* Push event into queue */
static av_result_t av_input_sdl_push_event(av_input_p self, av_event_p event)
{
	SDL_Event e;
	AV_UNUSED(self);

	memset(&e, 0, sizeof(SDL_Event));
	switch (event->type)
	{
		case AV_EVENT_QUIT:
			e.quit.type = e.type = SDL_QUIT;
		break;
		case AV_EVENT_KEYBOARD:
			if (AV_BUTTON_PRESSED == event->button_status)
			{
				e.key.type = e.type = SDL_KEYDOWN;
				e.key.state = SDL_PRESSED;
			}
			else
			{
				e.key.type = e.type = SDL_KEYUP;
				e.key.state = SDL_RELEASED;
			}
			e.key.which = 0; /* FIXME: The keyboard device index? */

			if (AV_FALSE == av_input_translate_key(&e.key.keysym.sym, &event->key, KEY_AVGL_TO_SDL))
				return AV_EARG;
		break;
		case AV_EVENT_MOUSE_BUTTON:
			if (AV_BUTTON_PRESSED == event->button_status)
			{
				e.button.type = e.type = SDL_MOUSEBUTTONDOWN;
				e.button.state = SDL_PRESSED;
			}
			else
			{
				e.button.type = e.type = SDL_MOUSEBUTTONUP;
				e.button.state = SDL_RELEASED;
			}
			e.button.which = 0; /* FIXME: The mouse device index? */

			switch (event->mouse_button)
			{
				case AV_MOUSE_BUTTON_LEFT:
					e.button.button = SDL_BUTTON_LEFT;
				break;
				case AV_MOUSE_BUTTON_RIGHT:
					e.button.button = SDL_BUTTON_RIGHT;
				break;
				case AV_MOUSE_BUTTON_MIDDLE:
					e.button.button = SDL_BUTTON_MIDDLE;
				break;
				case AV_MOUSE_BUTTON_WHEEL:
					if (AV_BUTTON_PRESSED == event->button_status)
						e.button.button = SDL_BUTTON_WHEELDOWN;
					else
						e.button.button = SDL_BUTTON_WHEELUP;
				break;
				case AV_MOUSE_BUTTON_X1:
					e.button.button = SDL_BUTTON_X1;
				break;
				case AV_MOUSE_BUTTON_X2:
					e.button.button = SDL_BUTTON_X2;
				break;
			}
			e.button.x = event->mouse_x;
			e.button.y = event->mouse_y;
		break;
		case AV_EVENT_MOUSE_MOTION:
			e.type = e.motion.type = SDL_MOUSEMOTION;
			e.motion.which = 0; /* FIXME: The mouse device index? */
			if (AV_BUTTON_PRESSED == event->button_status)
				e.motion.state = SDL_PRESSED;
			else
				e.motion.state = SDL_RELEASED;
			e.motion.x = event->mouse_x;
			e.motion.y = event->mouse_y;

			/* FIXME: unknown relative coordinates */
			e.motion.xrel = 0;
			e.motion.yrel = 0;
		break;
		case AV_EVENT_UPDATE:
			e.type = e.user.type = SDL_USEREVENT;
			e.user.code = AV_EVENT_UPDATE;
		break;
		case AV_EVENT_USER:
			e.type = e.user.type = SDL_USEREVENT;
			e.user.code = event->user_id | AV_EVENT_USER;
			e.user.data1 = event->data;
			e.user.data2 = event->window;
		break;
		default:
			return AV_EARG;
	}

	if (0 != SDL_PushEvent(&e))
		return AV_EGENERAL;
	return AV_OK;
}

/*
* Get key modifiers
*/
static av_keymod_t av_input_sdl_get_key_modifiers(struct av_input* self)
{
	SDLMod modstate = SDL_GetModState();
	av_keymod_t                 keymods  = AV_KEYMOD_NONE;
	if (modstate & KMOD_LSHIFT) keymods |= (1 << AV_KEYMOD_SHIFT_LEFT);
	if (modstate & KMOD_RSHIFT) keymods |= (1 << AV_KEYMOD_SHIFT_RIGHT);
	if (modstate & KMOD_LCTRL)  keymods |= (1 << AV_KEYMOD_CTRL_LEFT);
	if (modstate & KMOD_RCTRL)  keymods |= (1 << AV_KEYMOD_CTRL_RIGHT);
	if (modstate & KMOD_LALT)   keymods |= (1 << AV_KEYMOD_ALT_LEFT);
	if (modstate & KMOD_RALT)   keymods |= (1 << AV_KEYMOD_ALT_RIGHT);
	if (modstate & KMOD_LMETA)  keymods |= (1 << AV_KEYMOD_META_LEFT);
	if (modstate & KMOD_RMETA)  keymods |= (1 << AV_KEYMOD_META_RIGHT);
	AV_UNUSED(self);
	return keymods;
}


/*
* Flush pending events
*/
static int av_input_sdl_flush_events(struct av_input* self)
{
	SDL_Event events[100];
	int count, flushed_count = 0;
	AV_UNUSED(self);

	while ( (count = SDL_PeepEvents(events, 100, SDL_GETEVENT, SDL_ALLEVENTS)) > 0)
	{
		flushed_count += count;
	}
	return flushed_count;
}

/* Initializes memory given by the input pointer with the input class information */
static av_result_t av_input_sdl_constructor(av_object_p object)
{
	av_input_p self         = (av_input_p)object;
	self->poll_event        = av_input_sdl_poll_event;
	self->push_event        = av_input_sdl_push_event;
	self->get_key_modifiers = av_input_sdl_get_key_modifiers;
	self->flush_events      = av_input_sdl_flush_events;
	return AV_OK;
}

/* Registers sdl input class into TORBA class repository */
av_result_t av_input_sdl_register_torba(void)
{
	return av_torb_register_class("input_sdl", "input", sizeof(av_input_t), av_input_sdl_constructor, AV_NULL);
}

#endif /* WITH_SYSTEM_SDL */
