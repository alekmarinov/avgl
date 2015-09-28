/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_player.h                                        */
/*                                                                   */
/*********************************************************************/

/*! \file av_player.h
*   \brief Audio/Video player
*/

#ifndef __AV_PLAYER_H
#define __AV_PLAYER_H

#include <av_window.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    avps_idle = 0,
    avps_playing = 1,
    avps_paused = 1,
    avps_connecting = 2,
    avps_buffering = 4,
    avps_running = 8,
    avps_finished = 16
};

typedef union av_player_state_s
{
    av_result_t state;
    struct
    {
        int state:16;
        int progress:16;
    } state_ext;
} av_player_state_t, *av_player_state_p;

/*! \brief Audio/Video player interface
*/
typedef struct av_player
{
	/*! Parent class object */
	av_object_t object;

	/*!
	* \brief Open media URL
	* \param self the player himself
	* \param filename URL to media content
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*open)(struct av_player* self, const char* url);

	/*!
	* \brief Starts playback
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on error
	*/
	av_result_t (*play)(struct av_player* self);

	/*!
	* \brief Stops playback
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on error
	*/
	av_result_t (*stop)(struct av_player* self);

	/*!
	* \brief Toggle pause playback
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on error
	*/
	av_result_t (*pause)(struct av_player* self);

	/*!
	* \brief Seeks to given absolute position
	* \param self is a reference to this object
	* \param pos_seconds
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on error
	*/
	av_result_t (*seek)(struct av_player* self, double pos_seconds);

	/*!
	* \brief Seeks to given relative position
	* \param self is a reference to this object
	* \param pos_seconds
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on error
	*/
	av_result_t (*seek_rel)(struct av_player* self, double pos_seconds);

	/*!
	* \brief Set player window
	* \param self is a reference to this object
	* \param window the player window
	*/
	void (*set_window)(struct av_player* self, av_window_p window);

	/*!
	* \brief Set to draw or not borders
	* \param self is a reference to this object
	* \param val boolean
	*/
	void (*set_borders)(struct av_player* self, av_bool_t val);

	/*!
	*
	*/
	double (*duration)(struct av_player* self);

	/*!
	*
	*/
	double (*position)(struct av_player* self);

	/*!
	* Return current state of player
	*/
	av_player_state_t (*is_playing)(struct av_player* self);

	/*!
     * Return current state of player
	*/
    av_result_t (*is_pause)(struct av_player* self);

} av_player_t, *av_player_p;

/*!
* \brief Registers player class into TORBA
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_player_register_torba(void);

#ifdef __cplusplus
}
#endif

#endif /* __AV_PLAYER_H */
