/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_surface.h                                       */
/*                                                                   */
/*********************************************************************/

/*! \file av_surface.h
*   \brief av_surface definition
*/

#ifndef __AV_SURFACE_H
#define __AV_SURFACE_H

#include <av.h>
#include <av_oop.h>
#include <av_rect.h>
#include <av_bitmap.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief Single pixel type
*/
typedef unsigned int av_pixel_t, *av_pixel_p;

/*!
* \brief Class surface
*/
typedef struct av_surface
{
	/*! Parent class object */
	av_object_t object;

	/*!
	* \brief Set surface width and height
	* \param self is a reference to this object
	* \param width of the surface
	* \param height of the surface
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_size)    (struct av_surface* self, int width, int height);

	/*!
	* \brief Get surface width and height
	* \param self is a reference to this object
	* \param pwidth points the width of the surface to be returned
	* \param pheight points the height of the surface to be returned
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_size)    (struct av_surface* self, int* pwidth, int* pheight);

	/*!
	* \brief Locks surface for direct memory access
	* \param self is a reference to this object
	* \param lockflags determine read or write the surface memory
	* \param ppixels result pointer to surface memory
	* \param ppitch result surface pitch
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*lock)        (struct av_surface* self,
								av_pixel_p* ppixels,
								int* ppitch);

	/*!
	* \brief Unlocks locked surface
	* \param self is a reference to this object
	*/
	void (*unlock)             (struct av_surface* self);

	void (*set_bitmap)         (struct av_surface* self, av_bitmap_p bitmap);

	void (*render)             (struct av_surface* self, av_rect_p src_rect, av_rect_p dst_rect);
} av_surface_t, *av_surface_p;

/*!
* \brief Registers surface class into OOP
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_surface_register_oop(av_oop_p);

#ifdef __cplusplus
}
#endif

#endif /* __AV_SURFACE_H */
