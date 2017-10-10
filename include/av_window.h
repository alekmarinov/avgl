/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_window.h                                        */
/*                                                                   */
/*********************************************************************/

/*! \file av_window.h
*   \brief av_window definition
*/

#ifndef __AV_WINDOW_H
#define __AV_WINDOW_H

#include <av_rect.h>
#include <av_event.h>
#include <av_oop.h>
#include <av_graphics.h>
#include <av_display.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	AV_UPDATE_INVALIDATE,
	AV_UPDATE_REPAINT
} av_window_update_t;

/*! \brief Definition of a window object as a part of tree of windows objects.
*
*	A window may have a parent window to which it is attached. A window without a parent is a root window.
*	All windows represents a rectangular areas which can overlap to each other.
*	The windows have a z-order as they appear on the screen which by default is their creation order.
*	The newer windows are on top of the olders. The z-order of a window can be changed via the methods
*	\c raise_top, \c lower_bottom. A part of area occuppied by a window can be \b invalidated caused
*	by the usage any of the following methods: \c set_window_rect, \c invalidate, \c invalidate_rect
*	executed against a certain window. Once an area is invalidated it is registered to the system object
*	which will make it valid when the \c av_system method \c update is invoked. The method \c update
*	is implicitly invoked on each processed event.
*	On \c update the windows tree is recursively tested for intersection with an invalidated region
*	starting from bottom to top according their z-order.
*	Once intersection is detected to a window then the method \c on_paint is executed with parameter
*	a graphics object where the window can draw its content.
*/
typedef struct av_window
{
	/*! Parent class object */
	av_object_t object;

	/*! Origin X */
	int origin_x;

	/*! Origin Y */
	int origin_y;

	/*! Delay in millisecs before mouse enter event to be sent to this window  */
	unsigned long hover_delay;
	
	/*! Window cursor shape */
	av_display_cursor_shape_t cursor;

	/*! Window cursor visitibility status */
	av_bool_t cursor_visible;

	/*! Are events bubbled */
	av_bool_t bubble_events;
	
	/*! Are events handled */
	av_bool_t handle_events;

	/*!
	* \brief Dispatcher object
	* used to hook and pre-filter events
	*/
	av_object_p dispatcher;

	/*!
	* \brief Set a parent to the window
	* \param self is a reference to this object
	* \param parent is new window parent
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*set_parent)          (struct av_window* self, struct av_window* parent);

	/*!
	* \brief Checks if a given window is direct or indirect parent to this window
	* \param self is a reference to this object
	* \param parent is a window to check if is parent to this window
	* \return av_bool_t
	*         - AV_TRUE if the window \c parent is a parent to this window
	*         - AV_FALSE otherwise
	*/
	av_bool_t  (*is_parent)            (struct av_window* self, struct av_window* parent);

	/*!
	* \brief Returns the direct parent of this window or AV_NULL if the window is root
	* \param self is a reference to this object
	* \return the parent to this window or AV_NULL if the window is root
	*/
	struct av_window* (*get_parent)    (struct av_window* self);

	av_list_p (*get_children)          (struct av_window* self);

	/*!
	* \brief Adds a window as a child to this window on top of the other window's children
	* \param self is a reference to this object
	* \param child is a window to add as a child to this window
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*add_child_top)       (struct av_window* self, struct av_window* child);

	/*!
	* \brief Adds a window as a child to this window to bottom of the other window's children
	* \param self is a reference to this object
	* \param child is a window to add as a child to this window
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*add_child_bottom)    (struct av_window* self, struct av_window* child);


	/*!
	* \brief Removes a child from the window's children list
	* \param self is a reference to this object
	* \param child to be removed
	* \return av_bool_t
	*         - AV_TRUE if the child is found between children of this window and removed
	*         - AV_FALSE when the child is not found
	*/
	av_bool_t (*remove_child)          (struct av_window* self, struct av_window* child);

	/*!
	* \brief Sets window visibility status
	* \param self is a reference to this object
	* \param visible is visibility status. AV_TRUE if visible, AV_FALSE if hidden
	*/
	void (*set_visible)                (struct av_window* self, av_bool_t visible);

	/*!
	* \brief Gets the window visibility status
	* \param self is a reference to this object
	* \return av_bool_t
	*         - AV_TRUE if the window is visible
	*         - AV_FALSE if hidden
	*/
	av_bool_t (*is_visible)            (struct av_window* self);

	/*!
	* \brief Set this window to clip its children or not
	* \param self is a reference to this object
	* \param clipped - AV_TRUE if the window children must be clipped by this window,
	*                  AV_FALSE if no clipping
	*/
	void (*set_clip_children)          (struct av_window* self, av_bool_t clipped);

	/*!
	* \brief Ask the window if clipping its children
	* \param self is a reference to this object
	* \return av_bool_t
	*         - AV_TRUE if the window children are clipped by this window
	*         - AV_FALSE no clipping
	*/
	av_bool_t (*are_children_clipped)  (struct av_window* self);

	/*!
	* \brief Returns the window rectangle
	* \param self is a reference to this object
	* \return a rectangle occuppied by this window relative to its parent
	*/
	void(*get_rect)                   (struct av_window* self, av_rect_p rect);

	/*!
	* \brief Returns the window rectangle in absolute coordinates
	* \param self is a reference to this object
	* \return a rectangle occuppied by this window
	*/
	void(*get_absolute_rect)          (struct av_window* self, av_rect_p rect);

	/*!
	* \brief Converts a given rect in absolute coordinates according the root window from the tree
	* the target window belongs to. The specified rect is assumed to be given according the
	* target window as an origin.
	* \param self is a reference to this object
	* \param rect to be converted in absolute coordinates
	*/
	void (*rect_absolute)              (struct av_window* self, av_rect_p rect);
	
	av_bool_t (*point_inside)          (struct av_window* self, int x, int y);

	/*!
	* \brief Sets new window rectangle
	* \param self is a reference to this object
	* \param rect is the new window rectangle
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*set_rect)            (struct av_window* self, av_rect_p rect);


	/*!
	* \brief Raises this window on top of the other sibling windows
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*raise_top)           (struct av_window* self);

	/*!
	* \brief Lower this window beneath the other sibling windows
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*lower_bottom)        (struct av_window* self);

	/*!
	* \brief Detach this window from its parent and siblings. If the window is a root then do nothing
	* \param self is a reference to this object
	*/
	void (*detach)                     (struct av_window* self);

	/*!
	* \brief Returns a direct child to this window holding a given point (\c x, \c y)
	* \param self is a reference to this object
	* \param x coordinate
	* \param y coordinate
	* \return a child window holding (x,y) point if AV_NULL if no such child
	*/
	struct av_window* (*get_child_xy)  (struct av_window* self, int x, int y);

	void (*move)                       (struct av_window* self, int x, int y);

	void (*set_cursor)                 (struct av_window* self, av_display_cursor_shape_t cursor);
	av_display_cursor_shape_t (*get_cursor) (struct av_window* self);
	void (*set_cursor_visible)         (struct av_window* self, av_bool_t visible);
	av_bool_t (*is_cursor_visible)     (struct av_window* self);
	void (*set_hover_delay)            (struct av_window* self, unsigned long hover_delay);
	unsigned long (*get_hover_delay)   (struct av_window* self);
	void (*set_bubble_events)          (struct av_window* self, av_bool_t bubble_events);
	av_bool_t (*is_bubble_events)      (struct av_window* self);
	void (*set_handle_events)          (struct av_window* self, av_bool_t handle_events);
	av_bool_t (*is_handle_events)      (struct av_window* self);

	/*!
	* \brief Event handler for all system events
	* Do not override this method, but the methods on_mouse_XXX, on_key_XXX and on_paint
	* \param self is a reference to this object
	* \param event is a new comming input event
	* \return av_bool_t
	*         - AV_TRUE to notify the event is processed and event bubbling to stop
	*         - AV_FALSE the event is skipped or bubbling must continue
	*/
	av_bool_t (*on_event)              (struct av_window* self, av_event_p event);

	/*!
	* \brief Mouse motion event
	* \param self is a reference to this object
	* \param x is mouse X coordinate
	* \param y is mouse Y coordinate
	* \return av_bool_t
	*         - AV_TRUE to notify the event is processed and event bubbling to stop
	*         - AV_FALSE the event is skipped or bubbling must continue
	*/
	av_bool_t (*on_mouse_move)         (struct av_window* self, int x, int y);

	/*!
	* \brief Mouse enter event
	* \param self is a reference to this object
	* \param x is mouse X coordinate
	* \param y is mouse Y coordinate
	* \return av_bool_t
	*         - AV_TRUE to notify the event is processed and event bubbling to stop
	*         - AV_FALSE the event is skipped or bubbling must continue
	*/
	av_bool_t (*on_mouse_enter)        (struct av_window* self, int x, int y);

	/*!
	* \brief Mouse hover event
	* \param self is a reference to this object
	* \param x is mouse X coordinate
	* \param y is mouse Y coordinate
	* \return av_bool_t
	*         - AV_TRUE to notify the event is processed and event bubbling to stop
	*         - AV_FALSE the event is skipped or bubbling must continue
	*/
	av_bool_t (*on_mouse_hover)        (struct av_window* self, int x, int y);

	/*!
	* \brief Mouse leave event
	* \param self is a reference to this object
	* \param x is mouse X coordinate
	* \param y is mouse Y coordinate
	* \return av_bool_t
	*         - AV_TRUE to notify the event is processed and event bubbling to stop
	*         - AV_FALSE the event is skipped or bubbling must continue
	*/
	av_bool_t (*on_mouse_leave)        (struct av_window* self, int x, int y);

	/*!
	* \brief Mouse button up
	* \param self is a reference to this object
	* \param button is which mouse button pressed
	* \param x is mouse X coordinate
	* \param y is mouse Y coordinate
	* \return av_bool_t
	*         - AV_TRUE to notify the event is processed and event bubbling to stop
	*         - AV_FALSE the event is skipped or bubbling must continue
	*/
	av_bool_t (*on_mouse_button_up)    (struct av_window* self, av_event_mouse_button_t button, int x, int y);

	/*!
	* \brief Mouse left button down
	* \param self is a reference to this object
	* \param button is which mouse button pressed
	* \param x is mouse X coordinate
	* \param y is mouse Y coordinate
	* \return av_bool_t
	*         - AV_TRUE to notify the event is processed and event bubbling to stop
	*         - AV_FALSE the event is skipped or bubbling must continue
	*/
	av_bool_t (*on_mouse_button_down)  (struct av_window* self, av_event_mouse_button_t button, int x, int y);

	/*!
	* \brief Key pressed
	* \param self is a reference to this object
	* \param key is the key code has been pressed
	* \return av_bool_t
	*         - AV_TRUE to notify the event is processed and event bubbling to stop
	*         - AV_FALSE the event is skipped or bubbling must continue
	*/
	av_bool_t (*on_key_up)             (struct av_window* self, av_key_t key);

	/*!
	* \brief Key released
	* \param self is a reference to this object
	* \param key is the key code has been pressed
	* \return av_bool_t
	*         - AV_TRUE to notify the event is processed and event bubbling to stop
	*         - AV_FALSE the event is skipped or bubbling must continue
	*/
	av_bool_t (*on_key_down)           (struct av_window* self, av_key_t key);

	/*!
	* \brief Window on focus
	* \param self is a reference to this object
	* \param isfocus AV_TRUE if the window is set on focus, AV_FALSE otherwise
	* \return av_bool_t
	*         - AV_TRUE to notify the event is processed and event bubbling to stop
	*         - AV_FALSE the event is skipped or bubbling must continue
	*/
	av_bool_t (*on_focus)              (struct av_window* self, av_bool_t isfocus);

	/*!
	* \brief On user event
	* \param self is a reference to this object
	* \param id is a user specified event identifier
	* \param data is the associated data with the user event
	* \return av_bool_t
	*         - AV_TRUE to notify the event is processed and event bubbling to stop
	*         - AV_FALSE the event is skipped or bubbling must continue
	*/
	av_bool_t (*on_user)               (struct av_window* self, int id, void* data);

	/*!
	* \brief Paint event handler
	* \param self is a reference to this object
	* \param graphics where this window can paint itself on this event
	* \return av_bool_t
	*         - AV_TRUE to notify the event is processed and event bubbling to stop
	*         - AV_FALSE the event is skipped or bubbling must continue
	*/
	av_bool_t (*on_paint)             (struct av_window* self, av_graphics_p graphics);

	/*!
	* \brief On invalidated rect event handler
	* \param self is a reference to this object
	* \param rect an invalidated rect in absolute coordinates
	*/
	void (*on_invalidate)             (struct av_window* self, av_rect_p rect);

} av_window_t, *av_window_p;

/*!
* \brief Registers window class into TORBA
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_window_register_oop(av_oop_p);

#ifdef __cplusplus
}
#endif

#endif /* __AV_WINDOW_H */
