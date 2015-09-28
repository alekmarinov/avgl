/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_system.h                                        */
/*                                                                   */
/*********************************************************************/

/*! \file av_system.h
*   \brief av_system definition
*/

#ifndef __AV_SYSTEM_H
#define __AV_SYSTEM_H

#include <av_audio.h>
#include <av_video.h>
#include <av_input.h>
#include <av_timer.h>
#include <av_sound.h>
#include <av_window.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief window modality mode
*
* Defines the modal window modes
*/
typedef enum
{
	/*! modal_enter returns when the window exits modal state */
	AV_MODALITY_BLOCK   = 0,

	/*! modal_enter returns immediatly */
	AV_MODALITY_UNBLOCK = 1
} av_modality_mode_t;

/*!
* \brief system interface
*
* OS abstraction layer dialing with system events,
* providing audio, video and graphics interfaces
*/
typedef struct av_system
{
	/*! Parent class service */
	av_service_t service;

	/*! Audio object */
	av_audio_p audio;

	/*! Video object */
	av_video_p video;

	/*! Input object */
	av_input_p input;

	/*! Timer object */
	av_timer_p timer;

	/*! Graphics object */
	av_graphics_p graphics;

	/*! Graphics surface object */
	av_graphics_surface_p graphics_surface;

	/*! List of {window,rect} waiting for paint */
	av_list_p invalid_rects;

	/*! Root window */
	av_window_p root;

	/*! Focus window */
	av_window_p focus;

	/*! Captured window */
	av_window_p capture;

	/*! Hovered windows */
	av_list_p hover_windows;

	/*! Modal window */
	/* FIXME: implement modality stack to support multiple modal windows at a time */
	av_window_p modal;

	/*! The result code to modal window */
	int modal_result_code;

	/*! Current base X resolution */
	int xbase;

	/*! Current base Y resolution */
	int ybase;

	/*!
	* \brief Gets audio object
	* \param self is a reference to this object
	* \param ppaudio result audio object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_audio)      (struct av_system* self, av_audio_p* ppaudio);

	/*!
	* \brief Gets video object
	* \param self is a reference to this object
	* \param ppaudio result video object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_video)      (struct av_system* self, av_video_p* ppvideo);

	/*!
	* \brief Gets input object
	* \param self is a reference to this object
	* \param ppinput result input object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_input)      (struct av_system* self, av_input_p* ppinput);

	/*!
	* \brief Gets timer object
	* \param self is a reference to this object
	* \param pptimer result timer object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_timer)      (struct av_system* self, av_timer_p* pptimer);

	/*!
	* \brief Gets graphics object
	* \param self is a reference to this object
	* \param ppgraphics result graphics object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_graphics)   (struct av_system* self, av_graphics_p* ppgraphics);

	/*!
	* \brief Creates new window
	* \param self is a reference to this object
	* \param parent to the new window
	* \param rect is window occuppied area
	* \param ppwindow returns the new created window
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*create_window)  (struct av_system* self,
								   av_window_p parent,
								   av_rect_p rect,
								   av_window_p* ppwindow);

	/*!
	* \brief Notify the system about removing a window
	* The method will remove the internal references to that window.
	* \param self is a reference to this object
	* \param window being removed
	*/
	void (*removing_window)       (struct av_system* self, av_window_p window);

	/*!
	* \brief Invalidates screen rectangle
	* \param self is a reference to this object
	* \param rect to be invalidated
	*/
	void (*invalidate_rect)       (struct av_system* self, av_rect_p rect);

	/*!
	* \brief Invalidates all windows
	* \param self is a reference to this object
	*/
	void (*invalidate_all)       (struct av_system* self);

	/*!
	* \brief Set a window in modal state
	* During a window is in modal state all mouse and keyboard messages
	* are handled only if related to that window.
	* To exit a window from modal state it can be destroyed, hidden or
	* by calling the modal_exit method with specified modal window
	* and result code which is later returned by modal_enter.
	* In all other cases the function returns 0.
	* if modality_mode is AV_MODALITY_BLOCK the function doesn't return
	* until the window is in modal state.
	* if modality_mode is AV_MODALITY_UNBLOCK the function returns immediatly
	*
	* \param self is a reference to this object
	* \param window to be made modal
	* \param modality_mode the window modality mode. \see av_modality_mode_t
	* \return int
	*         - 0 if the modal window is explicitly hidden/destroyed
	*           or modality_mode is AV_MODALITY_UNBLOCK
	*         - result code by modal_exit
	*/
	int (*modal_enter)            (struct av_system* self,
								   av_window_p window,
								   av_modality_mode_t modality_mode);

	/*!
	* \brief Leave from modal state for specified window
	* The function returns immediatly making the window invisible.
	*
	* \param self is a reference to this object
	* \param window is a modal window which modal state must be ended
	*/
	void (*modal_exit)            (struct av_system* self, av_window_p window,
								   int result_code);

	/*!
	* \brief Updates invalidated rects
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*update)         (struct av_system* self);

	/*!
	* \brief Set base resolution from which the current resolution is transformed
	* \param self is a reference to this object
	* \param xbase is the base X resolution
	* \param ybase is the base Y resolution
	*/
	void (*set_base_resolution)  (struct av_system* self, int xbase, int ybase);

	/*!
	* \brief Get base resolution from which the current resolution is transformed
	* \param self is a reference to this object
	* \param pxbase is the result base X resolution
	* \param pybase is the result base Y resolution
	*/
	void (*get_base_resolution)  (struct av_system* self, int* pxbase, int* pybase);

	/*!
	* \brief Captures window to receive all mouse event
	* \param self is a reference to this object
	* \param window to be captured or AV_NULL to uncapture
	*/
	void (*set_capture)           (struct av_system* self, av_window_p window);

	/*!
	* \brief Set focused window
	* \param self is a reference to this object
	* \param window is the new focused window
	*/
	void (*set_focus)             (struct av_system* self, av_window_p window);

	/*!
	* \brief Return AV_TRUE if the given window is focused
	* \param self is a reference to this object
	* \param window is the requested window
	* \return AV_TRUE if the requested window is on focus
	*         AV_FALSE if the requested window is on focus
	*/
	av_bool_t (*is_focus)         (struct av_system* self, av_window_p window);

	/*!
	* \brief Process one system event
	* \param self is a reference to this object
	* \return - AV_FALSE is the last processed event has been AV_EVENT_QUIT
	*         - AV_TRUE otherwise
	*/
	av_bool_t (*step)             (struct av_system* self);

	int (*flush_events)           (struct av_system* self);

	/*!
	* \brief Main application loop
	* \param self is a reference to this object
	*/
	void (*loop)                  (struct av_system* self);

} av_system_t, *av_system_p;

/*!
* \brief Registers system class into TORBA
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_system_register_torba(void);

#ifdef __cplusplus
}
#endif

#endif /* __AV_SYSTEM_H */
