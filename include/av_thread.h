/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_thread.h                                        */
/*                                                                   */
/*********************************************************************/

/*! \file av_thread.h
*   \brief av_thread, av_mutex, av_rwmutex, av_condition definitions
*/

#ifndef __AV_THREAD_H
#define __AV_THREAD_H

#include <av.h>
#include <av_list.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! 
* \brief mutex type
*
* Defines a mutex
*/
typedef struct av_mutex
{
	/*! Implementation specific handle */
	void* mid;

	/*!
	* \brief Locks the mutex
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*lock)   (struct av_mutex* self);
	
	/*!
	* \brief Locks the mutex if not locked
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EBUSY if mutex is already locked
	*         - other kind of failure
	*/
	av_result_t (*trylock)(struct av_mutex* self);

	/*!
	* \brief Unlocks the mutex
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*unlock) (struct av_mutex* self);

	/*!
	* \brief Destroys this mutex
	* \param self is a reference to this object
	*/
	void (*destroy)       (struct av_mutex* self);
} av_mutex_t, *av_mutex_p;

/*!
* \brief Creates a mutex object
* \param ppmutex points the result mutex
* \return av_result_t
*         - AV_OK on success
*         - != AV_OK on failure
*/
AV_API av_result_t av_mutex_create( av_mutex_p* ppmutex);

/*! 
* \brief R/W mutex type
*
*/
typedef struct av_rwmutex
{
	/*! Implementation specific handle */
	void* mid;

	/*!
	* \brief Locks the R/W mutex for reading
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*lock_read)    (struct av_rwmutex* self);

	/*!
	* \brief Locks the R/W mutex for reading or writing
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*lock_write)   (struct av_rwmutex* self);

	/*!
	* \brief Locks the R/W mutex for reading if not locked
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EBUSY if mutex is already locked
	*         - other kind of failure
	*/
	av_result_t (*trylock_read) (struct av_rwmutex* self);

	/*!
	* \brief Locks the R/W mutex for reading or writing if not locked
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EBUSY if mutex is already locked
	*         - other kind of failure
	*/
	av_result_t (*trylock_write)(struct av_rwmutex* self);

	/*!
	* \brief Unlocks the R/W mutex
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*unlock)       (struct av_rwmutex* self);

	/*!
	* \brief Destroys this R/W mutex
	* \param self is a reference to this object
	*/
	void (*destroy)             (struct av_rwmutex* self);
} av_rwmutex_t, *av_rwmutex_p;

/*!
* \brief Creates a R/W mutex object
* \param pprwmutex points the result R/W mutex
* \return av_result_t
*         - AV_OK on success
*         - != AV_OK on failure
*/
AV_API av_result_t av_rwmutex_create(av_rwmutex_p* pprwmutex);

/*! 
* \brief condition type
*
*/
typedef struct av_condition
{
	/*! Implementation specific handle */
	void* cid;

	/*!
	* \brief Wakes up all threads waiting on this signal
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t  (*broadcast)(struct av_condition* self);

	/*!
	* \brief Wakes up one thread waiting on this signal
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t  (*signal)   (struct av_condition* self);

	/*!
	* \brief Waits for the condition to be signaled or broadcast.
    *        The \c mutex is assumed to be locked before
	* \param self is a reference to this object
	* \param mutex is a locked mutex
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t  (*wait)     (struct av_condition* self,
				 			  av_mutex_p mutex);

	/*!
	* \brief Waits for the condition to be signaled
	*        with timeout given in milliseconds (1/1000 seconds)
	*        The \c mutex is assumed to be locked
	* \param self is a reference to this object
	* \param mutex is a locked mutex
	* \param mills is the time to wait in millisecs until the condition is signaled
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t  (*wait_ms)  (struct av_condition* self,
				 			  av_mutex_p mutex,
							  unsigned long mills);

	/*!
	* \brief Waits for the condition to be signaled
	*        with timeout given in nanoseconds (1/1000,000,000 seconds)
	*        The \c mutex is assumed to be locked
	* \param self is a reference to this object
	* \param mutex is a locked mutex
	* \param nanos is the time to wait in nanoseconds until the condition is signaled
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t  (*wait_ns)  (struct av_condition* self,
				 			  av_mutex_p mutex,
							  unsigned long nanos);

	/*!
	* \brief Destroys this condition
	* \param self is a reference to this object
	*/
	void (*destroy)          (struct av_condition* self);
} av_condition_t, *av_condition_p;

/*!
* \brief Creates a condition object
* \param ppcondition points the result condition
* \return av_result_t
*         - AV_OK on success
*         - != AV_OK on failure
*/
AV_API av_result_t av_condition_create(av_condition_p *ppcondition);


struct av_thread;
typedef int (*av_runnable_t)(struct av_thread*);
/*! 
* \brief thread type
*/
typedef struct av_thread
{
	/*! Implementation specific handle */
	void*                tid;

	/*! Thread routine */
	av_runnable_t        runnable;

	/*! Argument passed to the thread routine */
	void*                arg;

	/*! Mark if the thread is attempted to be interrupted.
		The thread must exit asap on detecting this flag enabled */
	av_bool_t            interrupted;

	/*! Mark if the thread is normally started */
	av_bool_t            started;

	/*! R/W mutex locking the shared property \c interrupted */
	av_rwmutex_p         mtx_interrupted;

	/*! Mark if the thread is still running (AV_TRUE) or normally exited (AV_FALSE) */
	av_bool_t            active;

	/*! R/W mutex locking the shared property \c active */
	av_rwmutex_p         mtx_active;

	/*! Mutex for start condition */
	av_mutex_p           mtx_start;

	/*! Start condition */
	av_condition_p       cnd_start;


	/*!
	* \brief Getter for the property \c active
	* \param self is a reference to this object
	* \param pactive results the thread active status
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*is_active)(struct av_thread* self, av_bool_t* pactive);

	/*!
	* \brief Getter for the property \c interrupted
	* \param self is a reference to this object
	* \param pinterrupted results the thread interrupted status
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*is_interrupted)(struct av_thread* self, av_bool_t* pinterrupted);

	/*!
	* \brief Enables the \c interrupted flag
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*interrupt)(struct av_thread* self);

	/*!
	* \brief Starts a thread
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*start)(struct av_thread* self);

	/*!
	* \brief Suspends the calling thread until the target thread finishes
	* \returns av_result_t
	*          - AV_OK on success
	*          - AV_EARG on passing non joinable thread or the thread context is invalid 
	*                    and haven't corresponding system thread
	*          - AV_EDEADLK if a deadlock was detected or the value of thread specifies the calling thread
	*/
	av_result_t (*join)(struct av_thread* self);

	/*!
	* \brief Brings the thread in a detached (non-joinable) state
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*detach)(struct av_thread* self);

	/*!
	* \brief Tests if two threads are equal
	* \param self is a reference to this thread
	* \param thread is a reference to another thread
	* \return - AV_TRUE if both threads are equal
	*         - AV_FALSE otherwise
	*/
	av_bool_t  (*equals)(struct av_thread* self, struct av_thread* thread);

	/*!
	* \brief Destroys this thread
	* \param self is a reference to this object
	*/
	void (*destroy)(struct av_thread* self);
} av_thread_t, *av_thread_p;

/*!
* \brief Creates new thread
* \param runnable is the thread procedure
* \param arg is a custom parameter passed to the thread procedure
* \param ppthread returns the new thread
* \return av_result_t
*         - AV_OK on success
*         - != AV_OK on failure
*/
AV_API av_result_t av_thread_create(av_runnable_t runnable,
									void* arg,
									av_thread_p* ppthread);

/*!
* \brief Get the current running thread
* \param ppcurrent returns the current running thread
* \return av_result_t
*         - AV_OK on success
*         - != AV_OK on failure
*/
AV_API av_result_t av_thread_current(av_thread_p* ppthread);

/*!
* \brief synchronized queue
*/
typedef struct av_sync_queue
{
	av_list_p queue;
	av_mutex_p mutex;
	av_condition_p condnotfull;
	av_condition_p condnotempty;
	unsigned int elements_max;
	av_bool_t is_abort;

	unsigned int (*size)(struct av_sync_queue* self);
	av_result_t (*push)(struct av_sync_queue* self, void* element);
	av_result_t (*pop)(struct av_sync_queue* self, void** ppelement);
	av_result_t (*peek)(struct av_sync_queue* self, void** ppelement);
	void        (*abort)(struct av_sync_queue* self);
	void        (*destroy)(void*);
} av_sync_queue_t, *av_sync_queue_p;

/*!
* \brief Creates new synchronized queue
* \param ppqueue returns the new synchronized queue
* \return av_result_t
*         - AV_OK on success
*         - != AV_OK on failure
*/
AV_API av_result_t av_sync_queue_create(int elements_max, av_sync_queue_p* ppqueue);

#ifdef __cplusplus
}
#endif

#endif /* __AV_THREAD_H */
