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
typedef struct _av_sprite_t
{
	/*! Parent class visible */
	av_visible_t visible;

	void (*set_size)(struct _av_sprite_t* _self, int frame_width, int frame_height);
	void (*get_size)(struct _av_sprite_t* _self, int* frame_width, int* frame_height);
	int  (*get_frames_count) (struct _av_sprite_t* _self);
	void (*set_current_frame) (struct _av_sprite_t* _self, int frame);
	int  (*get_current_frame) (struct _av_sprite_t* _self);
	void (*set_sequence)(struct _av_sprite_t* _self, int* sequence, int count, unsigned long duration, av_bool_t is_loop);
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
