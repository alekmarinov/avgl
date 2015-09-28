/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_timer.h                                         */
/*                                                                   */
/*********************************************************************/

/*! \file av_timer.h
*   \brief Some time related utillity functions
*/

#ifndef __AV_TIMER_H
#define __AV_TIMER_H

#include <av_torb.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief Timer callback definition
* \param arg is a user specified parameter
* \return av_bool_t
*        - AV_TRUE - timer continues
*        - AV_FALSE - timer stop
*/
typedef av_bool_t (*av_timer_callback_p)(void* arg);

/*!
* \brief Timer class
*/
typedef struct av_timer
{
	/*! Parent class object */
	av_object_t object;

	/*!
	* \brief Adds callback function executed in intervals (given in milliseconds)
	*/
	av_result_t (*add_timer)(struct av_timer* self, av_timer_callback_p callback, unsigned long delay_ms, void* arg, int* timer_id);

	/*!
	* \brief Removes added timer
	*/
	av_result_t (*remove_timer)(struct av_timer* self, int timer_id);

	/*!
	* \brief Sleeps for timeout given in seconds
	* \param s is the time to sleep in secs
	*/
	void (*sleep)(unsigned long s);
	
	/*!
	* \brief Sleeps for timeout given in milliseconds
	* \param mills is the time to sleep in millisecs (1/1000 seconds ;)
	*/
	void (*sleep_ms)(unsigned long mills);
	
	/*!
	* \brief Sleeps for timeout given in microseconds
	* \param micrs is the time to sleep in microseconds (1/1,000,000 seconds ;)
	*/
	void (*sleep_micro)(unsigned long micrs);

	/*!
	* \brief Returns current time since computer started in ms
	* \param none
	* \return current time since computer started in ms
	*/
	unsigned long (*now)(void);

} av_timer_t, *av_timer_p;

/*!
* \brief Registers timer class into TORBA
* \return av_result_t
*         - AV_OK on success
*         - != AV_OK on error
*/
AV_API av_result_t av_timer_register_torba(void);

#ifdef __cplusplus
}
#endif

#endif /* __AV_TIMER_H */
