/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_video.h                                         */
/*                                                                   */
/*********************************************************************/

/*! \file av_video.h
*   \brief av_video definition
*/

#ifndef __AV_VIDEO_H
#define __AV_VIDEO_H

#include <av.h>
#include <av_torb.h>
#include <av_surface.h>
#include <av_rect.h>

#ifdef __cplusplus
extern "C" {
#endif

/* forward reference to video class definition */
struct av_video;
struct av_video_overlay;

/*!
* \brief video surface interface
*
* video surface is a surface created by a video interface,
* which can provide hardware acceleration if present in
* the video device
*/
typedef struct av_video_surface
{
	/*! Parent class surface */
	av_surface_t surface;

	/*! Owner video object */
	struct av_video* video;

	/*!
    * \brief Blit source surface to this video surface
	* \param self is a reference to this object
	* \param dstrect is a destination rectangle
	* \param src is a source surface
	* \param srcrect is a source rectangle
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*blit)(struct av_video_surface* self, av_rect_p dstrect, av_surface_p src, av_rect_p srcrect);

	/*!
	* \brief Create video overlay attached to backbuffer
	* \param psurface is attached surface
	* \param ppoverlay to hold returned pointer to newly created overlay
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*create_overlay)(struct av_video_surface* psurface, struct av_video_overlay** ppoverlay);

	/*!
	* \brief Create video overlay attached to surface
	* \param psurface is attached surface
	* \param ppoverlay to hold returned pointer to newly created overlay
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*create_overlay_buffered)(struct av_video_surface* psurface, struct av_video_overlay** ppoverlay);

	/*!
	* \brief Set surface width and height
	* \brief Export av_surface's method
	* \param self is a reference to this object
	* \param width of the surface
	* \param height of the surface
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_size)    (struct av_video_surface* self, int width, int height);

	/*!
	* \brief Get surface width and height
	* \brief Export av_surface's method
	* \param self is a reference to this object
	* \param pwidth points the width of the surface to be returned
	* \param pheight points the height of the surface to be returned
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_size)    (struct av_video_surface* self, int* pwidth, int* pheight);

	/*!
	* \brief Get surface color depth
	* \brief Export av_surface's method
	* \param self is a reference to this object
	* \param pdepth points the color depth of the surface to be returned
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_depth)    (struct av_video_surface* self, int* pdepth);

	/*!
	* \brief Fill rect with collor
	* \param self is a reference to this object
	* \param rect target rectangle
	* \param r,g,b,a color components red, green, blue and alpha
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*fill_rect)    (struct av_video_surface* self, av_rect_p prect, double r, double g, double b, double a);

	/*!
	*/
	av_result_t (*fill_rect_rgba)(struct av_video_surface* self, av_rect_p prect, av_pixel_t rgba);

	/*!
	* \brief Lock surface for direct memory access
	* \brief Export av_surface's method
	* \param self is a reference to this object
	* \param lockflags determine read or write the surface memory
	* \param ppixels result pointer to surface memory
	* \param ppitch result surface pitch
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*lock)        (struct av_video_surface* self,
								av_surface_lock_flags_t lockflags,
								av_pixel_p* ppixels,
								int* ppitch);

	/*!
	* \brief Unlock locked surface
	* \brief Export av_surface's method
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*unlock)      (struct av_video_surface* self);

} av_video_surface_t, *av_video_surface_p;

typedef unsigned long av_video_overlay_format_t, *av_video_overlay_format_p;

#define AV_YV12_OVERLAY  0x32315659  /* Planar mode: Y + V + U */
#define AV_IYUV_OVERLAY  0x56555949  /* Planar mode: Y + U + V */
#define AV_YVYU_OVERLAY  0x55595659
#define AV_YUY2_OVERLAY  0x32595559
#define AV_UYVY_OVERLAY  0x59565955

/*!
* \brief video overlay interface
*
* video overlay is a special surface created by a video surface interface
* used for fast resizing, color space conversation and blit into surface
*/
typedef struct av_video_overlay
{
	/*! Parent class object */
	av_object_t object;

	/*! Owner video object */
	struct av_video* video;

	/*!
    * \brief Blit overlay to parent surface
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*blit)(struct av_video_overlay* self, av_rect_p dstrect);

	/*!
    * \brief Blit overlay's backbuffer to parent surface
	* \param self is a reference to this object
	* \param dstrect is a destination rectangle
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*blit_back)(struct av_video_overlay* self, av_rect_p srcrect, av_rect_p dstrect);

	/*!
    * \brief Blit to overlay's backbuffer
	* \param self is a reference to this object
	* \param srcrect is a source rectangle
	* \param dstrect is a destination rectangle
	* \param argb_data is a source data in argb format
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*blit_back_to)(struct av_video_overlay* self, av_rect_p dstrect, void* rgba_data, av_rect_p srcrect);

	/*!
	* \brief Set overlay width and height
	* \param self is a reference to this object
	* \param width of the overlay
	* \param height of the overlay
	* \param format the overlay pixel format
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_size_format)(struct av_video_overlay* self, int width, int height,
									av_video_overlay_format_t format);

	/*!
	* \brief Validate overlay state
	* \param self is a reference to this object
	* \return av_bool_t
	*         - AV_TRUE if overlay created
	*         - AV_FALSE if overlay not created
	*/
	av_bool_t (*validate)(struct av_video_overlay* self);

	/*!
	* \brief Validate current sizes
	* \param self is a reference to this object
	* \param width desired winth
	* \param height desired height
	* \return av_bool_t
	*         - AV_TRUE if overlay sizes are the same
	*         - AV_FALSE else...
	*/
	av_bool_t (*validate_size)(struct av_video_overlay* self, int width, int height);

	/*!
	* \brief Set overlay's backbuffer width and height
	* \param self is a reference to this object
	* \param back_width of destination surface
	* \param back_height of destination surface
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_size_back)(struct av_video_overlay* self, int back_width, int back_height);

	/*!
	* \brief Get overlay width and height
	* \param self is a reference to this object
	* \param pwidth points the width of the overlay to be returned
	* \param pheight points the height of the overlay to be returned
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_size_format)(struct av_video_overlay* self, int* pwidth, int* pheight,
									int* pback_width, int* pback_height, av_video_overlay_format_p pformat);

	/*!
	* \brief Locks overlay for direct memory access
	* \param self is a reference to this object
	* \param pplanes result number of planes
	* \param ppixels result pointer to overlay memory
	* \param ppitch result overlay pitch in every plane
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*lock)(struct av_video_overlay* self, int* pplanes,
						const av_pix_p** ppixels, const short int** ppitch);

	/*!
	* \brief Unlocks locked overlay
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*unlock)(struct av_video_overlay* self);

	/*!
	* \brief Check if hardware
	* \param self is a reference to this object
	* \param hardware result: AV_TRUE = HW | AV_FALSE = SW
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*is_hardware)(struct av_video_overlay* self, av_bool_t* hardware);

} av_video_overlay_t, *av_video_overlay_p;


/*!
* \brief video mode flags
*
* Defines flags for fullscreen or windowed video mode
*/
typedef enum
{
	AV_VIDEO_MODE_WINDOWED   = 0,
	AV_VIDEO_MODE_FULLSCREEN = (1<<0)
} av_video_mode_t;

/*!
* \brief video configuration flags
*
* Defines masks determining which fields from av_video_config_t are valid
*/
typedef enum
{
	/*!
	* Determines the fields \c mode valid in type av_video_config_t.
	*/
	AV_VIDEO_CONFIG_MODE = (1<<0),

	/*!
	* Determines the fields \c width and \c height valid in type av_video_config_t.
	*/
	AV_VIDEO_CONFIG_SIZE = (1<<1),

	/*!
	* Determines the field \c bpp valid from type av_video_config_t.
	* The option is read only.
	*/
	AV_VIDEO_CONFIG_BPP  = (1<<2)
} av_video_config_flags_t;

/*!
* \brief video configuration type
*
* Defines prefered video mode settings used to setup video mode
* \sa set_configuration and get_configuration
*/
typedef struct av_video_config
{
	/*! Masks determining the valid fields */
	av_video_config_flags_t flags;

	/*! Fullscreen ot windowed video mode */
	av_video_mode_t mode;

	/*! Video resolution width */
	int width;

	/*! Video resolution height */
	int height;

	/*! Video mode bits per pixel */
	int bpp;
} av_video_config_t, *av_video_config_p;


/*!
* \brief video cursor shape type
*
* Defines enumeration of various cursor shapes
* \sa set_cursor_shape
*/
typedef enum
{
	AV_VIDEO_CURSOR_DEFAULT,
	AV_VIDEO_CURSOR_HAND,
	AV_VIDEO_CURSOR_WAIT,
	AV_VIDEO_CURSOR_SCROLL,
	AV_VIDEO_CURSOR_ZOOM,
	AV_VIDEO_CURSOR_BEAM,
	AV_VIDEO_CURSOR_KEYBOARD,
	AV_VIDEO_CURSOR_LAST
} av_video_cursor_shape_t;

/* forward reference to system class definition */
struct av_system;

typedef int (*video_mode_callback_t)(int xres, int yres, int bpp);
/*!
*	\brief video interface
*
*/
typedef struct av_video
{
	/*! Parent class video surface */
	av_video_surface_t surface;

	/*! Owner system object */
	struct av_system* system;

	/*! Back buffer surface */
	av_video_surface_p backbuffer;

	av_result_t (*enum_video_modes)(struct av_video* self, video_mode_callback_t vmcbk);

	/*!
	* \brief Sets video configuration options
	* After setting configuration the \c config parameter is filled
	* with the new successfully applied settings
	* \param self is a reference to this object
	* \param config video configuration info
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_configuration)(struct av_video* self, av_video_config_p config);

	/*!
	* \brief Gets video configuration options
	* \param self is a reference to this object
	* \param config result video configuration info
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_configuration)(struct av_video* self, av_video_config_p config);

	/*!
	* \brief Creates video surface
	* \param self is a reference to this object
	* \param width surface width
	* \param height surface height
	* \param ppsurface result surface
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*create_surface)(struct av_video* self, int width, int height, av_video_surface_p* ppsurface);

	/*!
	* \brief Creates video overlay attached to backbuffer
	* \param self is a reference to this object
	* \param ppoverlay result overlay
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*create_overlay)(struct av_video* self, av_video_overlay_p* ppoverlay);

	/*!
	* \brief Creates video overlay attached to surface
	* \param self is a reference to this object
	* \param ppoverlay result overlay
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*create_overlay_buffered)(struct av_video* self, av_video_overlay_p* ppoverlay);

	/*!
	* \brief Creates video surface from system memory
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
	av_result_t (*create_surface_from)(struct av_video* self, void *ptr, int width, int height,
									   int pitch, av_video_surface_p* ppsurface);

	/*!
	* \brief Get backbuffer video surface
	* \param self is a reference to this object
	* \param ppsurface returned backbuffer video surface
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_backbuffer)(struct av_video* self,
								  av_video_surface_p* ppsurface);

	/*!
	*/
	av_result_t (*update)(struct av_video* self, av_rect_p rect);

	/*!
	*/
	void        (*set_capture)(struct av_video* self, av_bool_t iscaptured);

	/*!
	* \brief Set cursor visibility status
	* \param self is a reference to this object
	* \param visible AV_TRUE if cursor visible, AV_FALSE - invisible
	*/
	void        (*set_cursor_visible)(struct av_video* self,
									  av_bool_t visible);

	/*!
	* \brief Gets cursor visibility status
	* \param self is a reference to this object
	*/
	av_bool_t   (*is_cursor_visible)(struct av_video* self);

	/*!
	* \brief Set cursor shape
	* \param self is a reference to this object
	* \param shape is the new cursor shape
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EFOUND if the requested shape is not supported
	*         - != AV_OK on failure
	*/
	av_result_t (*set_cursor_shape)(struct av_video* self,
									av_video_cursor_shape_t shape);

	/*!
	* \brief Set mouse position
	* \param self is a reference to this object
	* \param mx is the mouse X axis
	* \param my is the mouse Y axis
	*/
	void        (*set_mouse_position)(struct av_video* self, int mx, int my);

	/*!
	* \brief Get mouse position
	* \param self is a reference to this object
	* \param pmx is the result mouse X axis
	* \param pmy is the result mouse Y axis
	*/
	void        (*get_mouse_position)(struct av_video* self, int* pmx, int* pmy);

	/*!
	* \brief Get root window platform depend
	* \param self is a reference to this object
	* \param phandle
	*/
	av_result_t (*get_root_window)(struct av_video* self, void** pdisplay, void** pwindow);

	/*!
	*/
	av_result_t (*get_color_key)(struct av_video* self, av_pixel_t* pcolorkey);

} av_video_t, *av_video_p;

/*!
* \brief Registers video class into TORBA
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_video_register_torba(void);

#ifdef __cplusplus
}
#endif

#endif /* __AV_VIDEO_H */
