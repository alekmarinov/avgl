/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_event.h                                         */
/*                                                                   */
/*********************************************************************/

/*! \file av_event.h
*   \brief av_event definition
*/

#ifndef __AV_EVENT_H
#define __AV_EVENT_H

#include <av.h>
#include <av_oop.h>
#include <av_keys.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief Event types enumeration
*/
typedef enum
{
	/*! Application exiting event */
	AV_EVENT_QUIT           = (1<<0),

	/*!
	*  Keyboard event
	*  \sa av_event_init_keyboard, av_event_button_status_t
	*/
	AV_EVENT_KEYBOARD       = (1<<1),

	/*!
	*  Mouse event
	*  \sa av_event_init_mouse_button, av_event_mouse_button_t, av_event_button_status_t
	*/
	AV_EVENT_MOUSE_BUTTON   = (1<<2),

	/*!
	*  Mouse motion event
	*  \sa av_event_init_mouse_motion
	*/
	AV_EVENT_MOUSE_MOTION   = (1<<3),

	/*!
	*  Mouse enter event
	*  \sa av_event_init_mouse_enter
	*/
	AV_EVENT_MOUSE_ENTER    = (1<<4),

	/*!
	*  Mouse hover event
	*  \sa av_event_init_mouse_hover
	*/
	AV_EVENT_MOUSE_HOVER    = (1<<5),

	/*!
	*  Mouse leave event
	*  \sa av_event_init_mouse_leave
	*/
	AV_EVENT_MOUSE_LEAVE    = (1<<6),

	/*!
	*  Updates video screen
	*/
	AV_EVENT_UPDATE         = (1<<7),

	/*!
	*  Window focus event
	*/
	AV_EVENT_FOCUS          = (1<<8),

	/*!
	*  User event
	*/
	AV_EVENT_USER           = (1<<9)
} av_event_type_t;

/*!
* \brief Masks determining the valid fields of av_event object
*/
typedef enum
{
	/*!
	*	No flags are valid
	*/
	AV_EVENT_FLAGS_NONE          = (0),

	/*!
	*	Determines the fields \c mouse_x and \c mouse_y valid in av_event
	*/
	AV_EVENT_FLAGS_MOUSE_XY      = (1<<0),

	/*!
	*	Determines the field \c button valid in av_event
	*/
	AV_EVENT_FLAGS_MOUSE_BUTTON  = (1<<1),

	/*!
	*	Determines the field \c is_pressed valid in av_event
	*/
	AV_EVENT_FLAGS_BUTTON_STATUS = (1<<2),

	/*!
	*	Determines the field \c key valid in av_event
	*/
	AV_EVENT_FLAGS_KEY           = (1<<3),

	/*!
	*	Determines the field \c user_id valid in av_event
	*/
	AV_EVENT_FLAGS_USER_ID       = (1<<4),

	/*!
	*	Determines the field \c window valid in av_event
	*/
	AV_EVENT_FLAGS_WINDOW        = (1<<5),

	/*!
	*	Determines the field \c data valid in av_event
	*/
	AV_EVENT_FLAGS_DATA          = (1<<6)
} av_event_flags_t;

/*!
* \brief Mouse buttons enumeration
*/
typedef enum
{
	/*! Left mouse button */
	AV_MOUSE_BUTTON_LEFT,

	/*! Right mouse button */
	AV_MOUSE_BUTTON_RIGHT,

	/*! Middle mouse button */
	AV_MOUSE_BUTTON_MIDDLE,

	/*! Mouse wheel */
	AV_MOUSE_BUTTON_WHEEL,

	/*! Mouse extended button 1 */
	AV_MOUSE_BUTTON_X1,

	/*! Mouse extended button 2 */
	AV_MOUSE_BUTTON_X2
} av_event_mouse_button_t;

/*!
* \brief Mouse button or key status
*/
typedef enum
{
	/*! Button is released or mouse wheeled up */
	AV_BUTTON_RELEASED = AV_FALSE,

	/*! Button is pressed or mouse wheeled down */
	AV_BUTTON_PRESSED  = AV_TRUE
} av_event_button_status_t;

/*! Mouse wheeled up */
#define AV_MOUSE_WHEEL_UP   AV_BUTTON_RELEASED

/*! Mouse wheeled down */
#define AV_MOUSE_WHEEL_DOWN AV_BUTTON_PRESSED

/*!
* \brief av_event holds event information
*        happening from an input device, such as keyboard and mouse,
*        or from the surrounding system
*/
typedef struct av_event
{
	/*! Event type */
	av_event_type_t type;

	/*! Flags determining the valid fields in av_event object */
	av_event_flags_t flags;

	/*! Mouse X position for \c AV_EVENT_MOUSE_MOTION event type */
	int mouse_x;

	/*! Mouse Y position for \c AV_EVENT_MOUSE_MOTION event type */
	int mouse_y;

	/*! Mouse button for \c AV_EVENT_MOUSE_BUTTON event type */
	av_event_mouse_button_t mouse_button;

	/*! Key or mouse button status for \c AV_EVENT_MOUSE_BUTTON or \c AV_EVENT_KEYBOARD event types */
	av_event_button_status_t button_status;

	/*! Key code for \c AV_EVENT_KEYBOARD event type */
	av_key_t key;

	/*! User event id attached to event type AV_EVENT_USER
	* \note Use range \c 0..31 only ! 
	*/
	int user_id;

	/*! Target window */
	void* window;

	/*! User data attached to event type AV_EVENT_USER */
	void* data;
} av_event_t, *av_event_p;

/*!
* \brief Initializes keyboard event
* \param event is a reference to \c av_event object to be initialized
* \param key is a key code
* \param button_status can be one of \c av_event_button_status_t
* \sa av_event, av_key_t and av_event_button_status_t
*/
AV_API void av_event_init_keyboard(av_event_p event,
								   av_key_t key,
								   av_event_button_status_t button_status);

/*!
* \brief Initializes mouse motion event
* \param event is a reference to \c av_event object to be initialized
* \param mouse_x is the mouse position by X axis
* \param mouse_y is the mouse position by Y axis
* \sa av_event
*/
AV_API void av_event_init_mouse_motion(av_event_p event,
									   int mouse_x,
									   int mouse_y);

/*!
* \brief Initializes mouse enter event
* \param event is a reference to \c av_event object to be initialized
* \param mouse_x is the mouse position by X axis
* \param mouse_y is the mouse position by Y axis
* \sa av_event
*/
AV_API void av_event_init_mouse_enter(av_event_p event,
									  int mouse_x,
									  int mouse_y);

/*!
* \brief Initializes mouse hover event
* \param event is a reference to \c av_event object to be initialized
* \param mouse_x is the mouse position by X axis
* \param mouse_y is the mouse position by Y axis
* \sa av_event
*/
AV_API void av_event_init_mouse_hover(av_event_p event,
									  int mouse_x,
									  int mouse_y);

/*!
* \brief Initializes mouse leave event
* \param event is a reference to \c av_event object to be initialized
* \param mouse_x is the mouse position by X axis
* \param mouse_y is the mouse position by Y axis
* \sa av_event
*/
AV_API void av_event_init_mouse_leave(av_event_p event,
									  int mouse_x,
									  int mouse_y);

/*!
* \brief Initializes mouse button event
* \param event is a reference to \c av_event object to be initialized
* \param mouse_button is one of \c av_event_mouse_button_t
* \param button_status is one of \c av_event_button_status_t
* \param mousex the mouse x position
* \param mousey the mouse y position
* \sa av_event, av_event_mouse_button_t and av_event_button_status_t
*/
AV_API void av_event_init_mouse_button(av_event_p event,
									   av_event_mouse_button_t mouse_button,
									   av_event_button_status_t button_status,
									   int mousex,
									   int mousey);
#ifdef __cplusplus
}
#endif

#endif /* __AV_EVENT_H */
