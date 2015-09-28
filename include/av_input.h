/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_input.h                                         */
/*                                                                   */
/*********************************************************************/

/*! \file av_input.h
*   \brief av_input definition
*/

#ifndef __AV_INPUT_H
#define __AV_INPUT_H

#include <av.h>
#include <av_keys.h>
#include <av_torb.h>
#include <av_event.h>

#ifdef __cplusplus
extern "C" {
#endif

/* forward reference to system class definition */
struct av_system;
/*!
* \brief input interface
*/
typedef struct av_input
{
	/*! Parent class object */
	av_object_t object;

	/*!
	* \brief Polls for currently pending events
	* \param self is a reference to this object
	* \param event is the result event if method returned AV_TRUE
	* \return - AV_TRUE if event is returned
	*         - AV_FALSE if no pending events
	*/
	av_bool_t (*poll_event)(struct av_input* self, av_event_p event);

	/*!
	* \brief Push event into queue
	* \param self is a reference to this object
	* \param event is the pushed event
	*/
	av_result_t (*push_event)(struct av_input* self, av_event_p event);

	/*!
	* \brief Get key modifiers
	* \param self is a reference to this object
	* \return Mask of the key modifiers state
	*/
	av_keymod_t (*get_key_modifiers)(struct av_input* self);

	/*!
	* \brief Flush any pending events from the event queue
	* \param self is a reference to this object
	* \return the number of events being flushed
	*/
	int (*flush_events)(struct av_input* self);

} av_input_t, *av_input_p;

/*!
* \brief Registers input class into TORBA
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_input_register_torba(void);

#ifdef __cplusplus
}
#endif

#endif /* __AV_INPUT_H */
