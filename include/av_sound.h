/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_sound.h                                         */
/*                                                                   */
/*********************************************************************/

/*! \file av_sound.h
*   \brief av_sound definition
*/

#ifndef __AV_SOUND_H
#define __AV_SOUND_H

#include <stdint.h>
#include <av_torb.h>
#include <av_audio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct av_sound_info
{
	int samplerate;
	int channels;
	int format;
	int duration_ms;
} av_sound_info_t, *av_sound_info_p;


typedef struct av_sound_handle av_sound_handle_t, *av_sound_handle_p;

/*!
* \brief sound interface
*
*/
typedef struct av_sound
{
	/*! Parent class object */
	av_object_t object;

	av_result_t (*open)(struct av_sound* self, const char* filename, struct av_sound_handle** pphandle);

	av_result_t (*play)(struct av_sound* self, struct av_sound_handle* phandle);

	av_result_t (*close)(struct av_sound* self, struct av_sound_handle* phandle);

	av_bool_t (*is_playing)(struct av_sound* self, struct av_sound_handle* phandle);

} av_sound_t, *av_sound_p;

/*!
* \brief Registers sound class into TORBA
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_sound_register_torba(void);

#ifdef __cplusplus
}
#endif

#endif /* __AV_SOUND_H */
