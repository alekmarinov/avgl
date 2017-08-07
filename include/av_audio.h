/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_audio.h                                         */
/*                                                                   */
/*********************************************************************/

/*! \file av_audio.h
*   \brief av_audio definition
*/

#ifndef __AV_AUDIO_H
#define __AV_AUDIO_H

#include <stdint.h>
#include <av_oop.h>

#ifdef __cplusplus
extern "C" {
#endif

/* forward reference to system and media classes definitions */
struct av_system;
struct av_media;
struct av_frame_audio;
struct av_audio_handle;

typedef void (*av_audio_callback_t)(void* userdata, unsigned char* data, int length);

typedef enum av_audio_format
{
	AV_AUDIO_SIGNED_8 = 8,
	AV_AUDIO_SIGNED_16 = 16,
	AV_AUDIO_UNSIGNED_8 = -8,
	AV_AUDIO_UNSIGNED_16 = -16
} av_audio_format_t, *av_audio_format_p;

/*!
* \brief audio interface
*
*/
typedef struct av_audio
{
	/*! Parent class object */
	av_object_t object;

	/*! Owner system object */
	struct av_system* system;

	/*!
	* \brief Gets audio frequecy
	* \param self is a reference to this object
	* \param saplerate DSP frequency -- samples per second
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EINIT on failure
	*/
	av_result_t (*get_samplerate)(struct av_audio* self, int* saplerate);

	/*!
	* \brief Gets audio channels
	* \param self is a reference to this object
	* \param channels Number of channels: 1 mono, 2 stereo, ...
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EINIT on failure
	*/
	av_result_t (*get_channels)(struct av_audio* self, int* channels);

	/*!
	* \brief Gets audio format
	* \param self is a reference to this object
	* \param format Audio format (see enum av_audio_format)
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EINIT on failure
	*/
	av_result_t (*get_format)(struct av_audio* self, av_audio_format_p format);

	/*!
	* \brief Sets media handler
	* \param self is a reference to this object
	* \param media is the media class for audio synchronization
	*/
	av_result_t (*set_media)(struct av_audio* self, struct av_audio_handle* phandle, struct av_media* media);

	/*!
	*
	*/
	av_result_t (*set_user_callback)(struct av_audio* self, struct av_audio_handle* phandle, av_audio_callback_t cb, void* cb_data);

	/*!
	* \brief Initializes audio system using freq and channels preset
	* \param self is a reference to this object
	*         - AV_OK on success
	*         - AV_EINIT on failure
	*/
	av_result_t (*open)(struct av_audio*, int samplerate, int channels, av_audio_format_t format, struct av_audio_handle**);

	/*!
	* \brief Stops audio playback
	* \param self is a reference to this object
	*         - AV_OK on success
	*         - AV_EINIT on failure
	*/
	av_result_t (*close)(struct av_audio*, struct av_audio_handle*);

	/*!
	*
	*/
	av_result_t (*play)(struct av_audio*, struct av_audio_handle*);

	/*!
	*
	*/
	av_result_t (*pause)(struct av_audio*, struct av_audio_handle*);

	/*!
	*
	*/
	av_bool_t (*is_paused)(struct av_audio*, struct av_audio_handle*);

	/*!
	*
	*/
	av_result_t (*write)(struct av_audio*, struct av_audio_handle* phandle, struct av_frame_audio* frame_audio);

	/*!
	*
	*/
	av_result_t (*enable)(struct av_audio*);

	/*!
	*
	*/
	void (*disable)(struct av_audio*);

} av_audio_t, *av_audio_p;

/*!
* \brief Registers audio class into TORBA
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_audio_register_torba(void);

#ifdef __cplusplus
}
#endif

#endif /* __AV_AUDIO_H */
