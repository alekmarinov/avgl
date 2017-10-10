/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2017,  Intelibo Ltd                                 */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_sprite.h                                        */
/*                                                                   */
/*********************************************************************/

/*! \file av_sprite.h
*   \brief Sprite visible
*/

#ifndef __AV_SPRITE_H
#define __AV_SPRITE_H

#include <av_oop.h>
#include <av_system.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief Sprite class
*/
typedef struct av_sprite
{
	/*! Parent class visible */
	av_visible_t visible;



} av_sprite_t, *av_sprite_p;

/*!
* \brief Registers sprite class into OOP
* \return av_result_t
*         - AV_OK on success
*         - != AV_OK on error
*/
AV_API av_result_t av_sprite_register_oop(av_oop_p);

#ifdef __cplusplus
}
#endif

#endif /* __AV_SPRITE_H */
