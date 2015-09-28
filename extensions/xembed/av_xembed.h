/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_xembed.h                                        */
/*                                                                   */
/*********************************************************************/

/*! \file av_xembed.h
*   \brief av_xembed is AVGL extension for xorg-x11-xvfb remote access and control
*/

#ifndef __AV_XEMBED_H
#define __AV_XEMBED_H

#include <av_torb.h>
#include <av_video.h>
#include <av_window.h>
#include <av_xembed_app.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct av_xembed_video_t
{
	int xres;
	int yres;
	int bpp;
	unsigned char* vmem;
} av_xembed_video_t, *av_xembed_video_p;

/*!
* \brief av_xembed is libxembed wrapper class
*
*/
typedef struct av_xembed
{
	/*! Parent class service */
	av_service_t service;

	/*!
	* \brief Starts XVFB server and opens connection via xembed
	*
	* \param self is a reference to this object
	* \param xres is the horizontal resolution of Xvfb
	* \param yres is the vertical resolution of Xvfb
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on error
	*/
	av_result_t (*open)(struct av_xembed* self, int xres, int yres);

	/*!
	* \brief Executes X application inside the XVFB server
	*
	* \param self is a reference to this object
	* \param argv is a string array representing arg0, arg1, ..., argn
	*             where the first argument must point X11 binary executable
	* \param window is the window for displaying the xembeded app
	* \param ppapp is result av_xembed_app object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on error
	*/
	av_result_t (*execute)(struct av_xembed* self,
						   char* const argv[],
						   av_window_p window,
						   av_xembed_app_p* ppapp);

	/*!
	* \brief Call this function when the application is idle
	*
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on error
	*/
	av_result_t (*idle)(struct av_xembed* self);

	/*!
	* \brief Optain video surface over Xvfb video memory
	*
	* \param self is a reference to this object
	* \param ppsurface result surface
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on error
	*/
	av_result_t (*get_video_surface)(struct av_xembed* self, av_video_surface_p* ppsurface);

	/*!
	* \brief Gets direct video access information
	*
	* \param self is a reference to this object
	* \param xembed_video result direct video access information
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on error
	*/
	av_result_t (*get_video_info)(struct av_xembed* self,
								  av_xembed_video_t* xembed_video);
	/*!
	* \brief Shuts down the XVFB server
	* The method disconnects and stops the XVFB server
	*
	* \param self is a reference to this object
	*/
	void (*close)(struct av_xembed* self);

	/*!
	 * \brief Copy rect from Xvfb video mem to rect in the backbuffer
	 *
	 * \param self is a reference to this object
	 * \param dstrect is destination rectangle in the backbuffer
	 * \param srcrect is source rectangle in Xvfb video memory
	 */
	av_result_t (*copy)(struct av_xembed* self, av_rect_p dstrect, av_rect_p srcrect);

	/*!
	 * \brief Creates Xvfb screenshot
	 *
	 * \param self is a reference to this object
	 * \param filename is the screenshot filename in .png format
	 */
	av_result_t (*screenshot)(struct av_xembed* self, const char* filename);

	/*!
	 * \brief Callback invoked on damaged area in Xvfb
	 *
	 * \param self is a reference to this object
	 * \param dmgrect is the damaged rectangle
	 */
	void (*on_damage)(struct av_xembed* self, av_rect_p dmgrect);

	/*!
	 * \brief Sends mouse move to X server
	 *
	 * \param self is a reference to this object
	 * \param x mouse x position
	 * \param y mouse y position
	 */
	void (*send_mouse_move)  (struct av_xembed* self, int x, int y);

	/*!
	 * \brief Sends mouse button event to X server
	 *
	 * \param self is a reference to this object
	 * \param button which button (\sa av_event_mouse_button_t)
	 * \param status is a button status (\sa av_event_button_status_t)
	 */
	void (*send_mouse_button)(struct av_xembed* self,
							  av_event_mouse_button_t button,
							  av_event_button_status_t status);

	/*!
	 * \brief Sends key event to X server
	 *
	 * \param self is a reference to this object
	 * \param key which key
	 * \param status is a key status (\sa av_event_button_status_t)
	 */
	void (*send_key)         (struct av_xembed* self,
							  av_key_t key, av_keymod_t modifiers,
							  av_event_button_status_t status);

	void (*set_cursor_visible)(struct av_xembed* self, av_bool_t cursor_visible);
} av_xembed_t, *av_xembed_p;

AV_API av_result_t av_xembed_register( void );

#ifdef __cplusplus
}
#endif

#endif /* __AV_XEMBED_H */
