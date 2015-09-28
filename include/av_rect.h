/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_rect.h                                          */
/*                                                                   */
/*********************************************************************/

/*! \file av_rect.h
*   \brief av_rect definition
*/

#ifndef __AV_RECT_H
#define __AV_RECT_H

#include <av_list.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief rectangle
*
* Defines rectanglular area
*/
typedef struct av_rect
{
	/*! rectangle left position */
	int x;

	/*! rectangle top position */
	int y;

	/*! rectangle width (>=0) */
	int w;

	/*! rectangle height (>=0) */
	int h;
} av_rect_t, *av_rect_p;

/*!
* \brief Initializes a rectangle with x,y,w,h
* \param rectangle
* \param point x position
* \param point y position
* \param width
* \param height
* \return rectangle validity status
*         - AV_TRUE if rectangle parameters are valid
*         - AV_FALSE if any of width or height arguments are less than zero
*/
AV_API av_bool_t av_rect_init(av_rect_p, int, int, int, int);

/*!
* \brief Compares two rectangles
* \param rectangle 1
* \param rectangle 2
* \return comparision status
*         - AV_TRUE if both rectangles are equal
*         - AV_FALSE otherwise
*/
AV_API av_bool_t av_rect_compare(av_rect_p, av_rect_p);

/*!
* \brief Tests if a point lies inside a rectangle
* \param rectangle
* \param point x position
* \param point y position
* \return - AV_TRUE if a given point lies inside the rectangle, 
*         - AV_FALSE otherwise
*/
AV_API av_bool_t av_rect_point_inside(av_rect_p, int, int);

/*!
* \brief Tests if one rectangle lies inside another rectangle
* \param rectangle1
* \param rectangle2
* \return - AV_TRUE rectangle2 lies inside rectangle1,
*         - AV_FALSE otherwise
*/
AV_API av_bool_t av_rect_contains(av_rect_p, av_rect_p);

/*!
* \brief Intersects two rectangles
* \param in rectangle 1
* \param in rectangle 2
* \param the intersection rectangle between rectangle 1 and rectangle 2
* \return - AV_TRUE if the two rectangles have intersected area
*/
AV_API av_bool_t av_rect_intersect(av_rect_p, av_rect_p, av_rect_p);

/*!
* \brief Substract rectangle from other rectangle
* \param in rectangle 1 to substract from
* \param in substracting rectangle 2
* \param out a list containing the rectangles included in rectangle 1 but not in rectangle 2
* \return - AV_OK if rectangle 1 and rectangle 2 have intersection area and the result list is initialized
*         - AV_EFOUND when both rectangles have no intersection area
*         - AV_EMEM in case of out of memory
*/
AV_API av_result_t av_rect_substract(av_rect_p, av_rect_p, av_list_p*);

/*!
* \brief Moves a rectangle in a direction
* \param rectangle
* \param offset according x axis
* \param offset according y axis
*/
AV_API void av_rect_move(av_rect_p, int, int);

/*!
* \brief Resizes a rectangle. Moves the bottom/right rectangle corner with specified offsets.
* The result rectangle width and height are always zero or positive defined (>=0)
* \param rectangle to be resized
* \param offset according x axis to move the bottom/right corner
* \param offset according y axis to move the bottom/right corner
* \return - AV_FALSE if rectangle width or height exceeds 0 limits by the given offsets.
*         - AV_TRUE otherwise
*/
AV_API av_bool_t av_rect_resize(av_rect_p, int, int);

/*!
* \brief Set rectangle width and height
* The result rectangle width and height are always zero or positive defined (>=0)
* if any of the size arguments is < 0, it is set to 0
* \param rectangle
* \param new rectangle width
* \param new rectangle height
*/
AV_API void av_rect_set_size(av_rect_p, int, int);

/*!
* \brief Copy one rectangle to another
* \param destination rectangle
* \param source rectangle
*/
AV_API void av_rect_copy(av_rect_p, av_rect_p);

/*!
* \brief Extends a rect to cover another rect
* \param rect_extend is a rectangle to extend
* \param rect_cover is a rectangle to be covered
*/
AV_API void av_rect_extend(av_rect_p rect_extend, av_rect_p rect_cover);

#ifdef __cplusplus
}
#endif

#endif /* __AV_RECT_H */
