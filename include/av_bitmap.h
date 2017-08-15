/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_bitmap.h                                        */
/*                                                                   */
/*********************************************************************/

/*! \file av_bitmap.h
*   \brief av_bitmap definition
*/

#ifndef __AV_BITMAP_H
#define __AV_BITMAP_H

#include <av.h>
#include <av_oop.h>
#include <av_rect.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief Single pixel type
*/
typedef unsigned int av_pixel_t, *av_pixel_p;

/*!
* \brief Class bitmap
*/
typedef struct _av_bitmap_t
{
	/*! Parent class object */
	av_object_t object;

	/*!
	* \brief Set bitmap width and height
	* \param self is a reference to this object
	* \param width of the surface
	* \param height of the surface
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_size)    (struct _av_bitmap_t* self, int width, int height);

	/*!
	* \brief Get surface width and height
	* \param self is a reference to this object
	* \param pwidth points the width of the surface to be returned
	* \param pheight points the height of the surface to be returned
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_size)    (struct _av_bitmap_t* self, int* pwidth, int* pheight);

	av_result_t (*load)        (struct _av_bitmap_t* self, const char* filename);
} av_bitmap_t, *av_bitmap_p;

/*!
* \brief Registers surface class into OOP
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_bitmap_register_oop(av_oop_p);

#ifdef __cplusplus
}
#endif

#endif /* __AV_BITMAP_H */
