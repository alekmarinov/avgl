/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_display.h                                         */
/*                                                                   */
/*********************************************************************/

/*! \file av_display.h
*   \brief av_display definition
*/

#ifndef __AV_DISPLAY_H
#define __AV_DISPLAY_H

#include <av.h>
#include <av_oop.h>
#include <av_surface.h>
#include <av_rect.h>
#include <av_bitmap.h>
#include <av_surface.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief display mode flags
*
* Defines flags for fullscreen or windowed display mode
*/
typedef enum
{
	AV_DISPLAY_MODE_WINDOWED   = 0,
	AV_DISPLAY_MODE_FULLSCREEN = (1 << 0),
	AV_DISPLAY_MODE_HW_ACCEL = (1 << 1),
	AV_DISPLAY_MODE_DOUBLE_BUFFER   = (1 << 2)
} av_display_mode_t;

/*!
* \brief display configuration type
*
* Defines prefered display mode settings used to setup display mode
* \sa set_configuration and get_configuration
*/
typedef struct av_display_config
{
	/*! Fullscreen ot windowed display mode */
	av_display_mode_t mode;

	/*! display resolution width */
	int width;

	/*! display resolution height */
	int height;

	/*! display scale x factor */
	int scale_x;

	/*! display scale y factor */
	int scale_y;
} av_display_config_t, *av_display_config_p;


/*!
* \brief display cursor shape type
*
* Defines enumeration of various cursor shapes
* \sa set_cursor_shape
*/
typedef enum
{
	AV_DISPLAY_CURSOR_DEFAULT,
	AV_DISPLAY_CURSOR_HAND,
	AV_DISPLAY_CURSOR_WAIT,
	AV_DISPLAY_CURSOR_SCROLL,
	AV_DISPLAY_CURSOR_ZOOM,
	AV_DISPLAY_CURSOR_BEAM,
	AV_DISPLAY_CURSOR_KEYBOARD,
	AV_DISPLAY_CURSOR_LAST
} av_display_cursor_shape_t;

typedef int (*display_mode_callback_t)(int xres, int yres, int bpp);

/*!
*	\brief display interface
*
*/
typedef struct av_display
{
	/*! Parent class service */
	av_service_t object;

	/*! Display configuration */
	av_display_config_t display_config;

	av_result_t (*enum_display_modes)(struct av_display* self, display_mode_callback_t vmcbk);

	/*!
	* \brief Sets display configuration options
	* After setting configuration the \c config parameter is filled
	* with the new successfully applied settings
	* \param self is a reference to this object
	* \param config display configuration info
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_configuration)(struct av_display* self, av_display_config_p config);

	/*!
	* \brief Gets display configuration options
	* \param self is a reference to this object
	* \param config result display configuration info
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_configuration)(struct av_display* self, av_display_config_p config);

	// FIXME: consider keeping only the one in system?
	av_result_t (*create_bitmap)    (struct av_display* self, av_bitmap_p* bitmap);
	av_result_t(*create_surface)   (struct av_display* self, av_surface_p* surface);

	/*!
	* \brief Creates display surface
	* \param self is a reference to this object
	* \param width surface width
	* \param height surface height
	* \param ppsurface result surface
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	// av_result_t (*create_surface)(struct av_display* self, int width, int height, av_display_surface_p* ppsurface);

	/*!
	* \brief Creates display surface from system memory
	* \param self is a reference to this object
	* \param ptr system memory buffer to create surface from
	* \param width surface width
	* \param height surface height
	* \param pitch surface pitch
	* \param ppsurface result surface
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	//av_result_t (*create_surface_from)(struct av_display* self, void *ptr, int width, int height,
	//								   int pitch, av_display_surface_p* ppsurface);

	/*!
	*/
	void        (*set_capture)(struct av_display* self, av_bool_t iscaptured);

	/*!
	* \brief Set cursor visibility status
	* \param self is a reference to this object
	* \param visible AV_TRUE if cursor visible, AV_FALSE - invisible
	*/
	void        (*set_cursor_visible)(struct av_display* self,
									  av_bool_t visible);

	/*!
	* \brief Gets cursor visibility status
	* \param self is a reference to this object
	*/
	av_bool_t   (*is_cursor_visible)(struct av_display* self);

	/*!
	* \brief Set cursor shape
	* \param self is a reference to this object
	* \param shape is the new cursor shape
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EFOUND if the requested shape is not supported
	*         - != AV_OK on failure
	*/
	av_result_t (*set_cursor_shape)(struct av_display* self,
									av_display_cursor_shape_t shape);

	/*!
	* \brief Set mouse position
	* \param self is a reference to this object
	* \param mx is the mouse X axis
	* \param my is the mouse Y axis
	*/
	void        (*set_mouse_position)(struct av_display* self, int mx, int my);

	/*!
	* \brief Get mouse position
	* \param self is a reference to this object
	* \param pmx is the result mouse X axis
	* \param pmy is the result mouse Y axis
	*/
	void        (*get_mouse_position)(struct av_display* self, int* pmx, int* pmy);

	void        (*render)            (struct av_display* self);
} av_display_t, *av_display_p;

/*!
* \brief Registers display class into TORBA
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_display_register_oop(av_oop_p);

#ifdef __cplusplus
}
#endif

#endif /* __AV_DISPLAY_H */
