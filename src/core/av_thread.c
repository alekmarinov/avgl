/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_thread.c                                        */
/* Description:   Threads, Mutexes, Conditions                       */
/*                                                                   */
/*********************************************************************/

#include <av.h>
#include <av_thread.h>
#include <av_hash.h>
#include <av_log.h>
#include <malloc.h>
#ifdef AV_MT
#  ifndef __USE_UNIX98
#    define __USE_UNIX98
#  endif
#include <pthread.h>
#include <errno.h>
#endif

/* Hash table mapping handles pthread_t to av_thread_p */
static av_hash_p    _threads_ht = 0;

/* Mutex used for safe accessing _threads_ht */
static av_rwmutex_p _mtx_threads_ht = 0;

/* Logging object */
static av_log_p _log = 0;

/* Error messages */
static const char* _err_mem        = "out of memory";
static const char* _err_arg        = "invalid argument";
static const char* _err_busy       = "busy during operation";
static const char* _err_again      = "out of resources";
static const char* _err_timeout    = "timed out";
static const char* _err_deadlock   = "dead lock";
static const char* _err_nosuchprcs = "no such process";
static const char* _err_perm       = "permission denied";

static av_result_t av_thread_error_process(int rc, const char* funcname, const char* srcfilename, int linenumber)
{
#ifdef AV_MT
	av_result_t averr = AV_OK;
	if (0 != rc)
	{
		const char* errmsg = 0;
		switch (rc)
		{
			case ENOMEM:    averr = AV_EMEM;           errmsg = _err_mem;        break;
			case EINVAL:    averr = AV_EARG;           errmsg = _err_arg;        break;
			case EBUSY:     averr = AV_EBUSY;          errmsg = _err_busy;       break;
			case EAGAIN:    averr = AV_EAGAIN;         errmsg = _err_again;      break;
			case ETIMEDOUT: averr = AV_ETIMEDOUT;      errmsg = _err_timeout;    break;
			case EDEADLK:   averr = AV_EDEADLK;        errmsg = _err_deadlock;   break;
			case ESRCH:     averr = AV_ENOSUCHPROCESS; errmsg = _err_nosuchprcs; break;
			case EPERM:     averr = AV_EPERM;          errmsg = _err_perm;       break;
		}
		if (errmsg)
		{
			if (_log) _log->error(_log, "%s returned error (%d) `%s' %s:%d",
									funcname, rc, errmsg, srcfilename, linenumber);
		}
		else
		{
			if (_log) _log->error(_log, "%s returned unknown error code (%d) %s:%d",
									funcname, rc, srcfilename, linenumber);
		}
	}
	return averr;
#else
	return AV_OK;
#endif
}

#define av_thread_error_check(funcname, rc) av_thread_error_process(rc, funcname, __FILE__, __LINE__)

av_result_t av_thread_current(av_thread_p* ppthread)
{
#ifdef AV_MT
	char tkey[11]; /* a key string with size of "0x12345678\0" */
	av_result_t rc;
	pthread_t tid = pthread_self();
	unsigned long* ptid = (unsigned long*)&tid;

	sprintf(tkey, "%lu", *ptid);

	if (AV_OK == (rc = _mtx_threads_ht->lock_read(_mtx_threads_ht)))
	{
		if (_threads_ht)
		{
			*ppthread = (av_thread_p)_threads_ht->get(_threads_ht, tkey);
		}
		else
		{
			/*
			*	_threads_ht == 0 indicates there are no active threads ever created or all of them are destroyed.
			*	it is not allowed asking for the main process thread
			*/
			*ppthread = 0;
			rc = AV_EARG;
		}
		_mtx_threads_ht->unlock(_mtx_threads_ht);
	}
	return rc;
#else
	av_assert(0, "AVGL is compiled without threading support. Recompile with defined AV_MT macro");
	return 0; /* suppress eventual warnings */
#endif /* AV_MT */
}

#ifdef AV_MT

static void* runnable_executor(void* arg)
{
	av_thread_p self = (av_thread_p)arg;
	int retval;

	self->mtx_start->lock(self->mtx_start);
	while (!self->started && !self->interrupted)
	{
		self->cnd_start->wait(self->cnd_start, self->mtx_start);
	}
	self->mtx_start->unlock(self->mtx_start);

	/* A flag indicating the thread is normally started by the method start */
	if (AV_TRUE == self->started)
	{
		self->mtx_active->lock_write(self->mtx_active);
		self->active = AV_TRUE;
		self->mtx_active->unlock(self->mtx_active);

		retval = self->runnable(self);

		self->mtx_active->lock_write(self->mtx_active);
		self->active = AV_FALSE;
		self->mtx_active->unlock(self->mtx_active);
	}

	return 0;
}

static av_result_t av_thread_is_interrupted(av_thread_p self, av_bool_t *pinterrupted)
{
	av_result_t rc;
	if (AV_OK == (rc = self->mtx_interrupted->lock_read(self->mtx_interrupted)))
	{
		*pinterrupted = self->interrupted;
		rc = self->mtx_interrupted->unlock(self->mtx_interrupted);
	}
	return rc;
}

static av_result_t av_thread_is_active(av_thread_p self, av_bool_t *pactive)
{
	av_result_t rc;
	if (AV_OK == (rc = self->mtx_active->lock_read(self->mtx_active)))
	{
		*pactive = self->active;
		rc = self->mtx_active->unlock(self->mtx_active);
	}
	return rc;
}

static av_result_t av_thread_interrupt(av_thread_p self)
{
	av_result_t rc;
	if (AV_OK == (rc = self->mtx_interrupted->lock_write(self->mtx_interrupted)))
	{
		self->interrupted = AV_TRUE;
		rc = self->mtx_interrupted->unlock(self->mtx_interrupted);
	}
	return rc;
}

static av_result_t av_thread_start(av_thread_p self)
{
	int rc;
	if (AV_OK == (rc = self->mtx_start->lock(self->mtx_start)))
	{
		self->started = AV_TRUE;
		self->cnd_start->signal(self->cnd_start);
		self->mtx_start->unlock(self->mtx_start);
	}
	return rc;
}

static av_result_t av_thread_join(av_thread_p self)
{
	return av_thread_error_check("pthread_join", pthread_join(self->tid, 0));
}

static av_result_t av_thread_detach(av_thread_p self)
{
	return av_thread_error_check("pthread_detach", pthread_detach(self->tid));
}

static av_bool_t av_thread_equals(av_thread_p self, av_thread_p athread)
{
	return pthread_equal(self->tid, athread->tid)?AV_TRUE:AV_FALSE;
}

static void av_thread_destroy(av_thread_p self)
{
	char tkey[11]; /* a key string with size of "0x12345678\0" */
	av_bool_t active;
	unsigned long* ptid = (unsigned long*)&(self->tid);

	/* makes sure the tread is stopped before destroyed */
	if ((AV_OK == (self->is_active(self, &active))) && active)
	{
		/* force the thread to stop */
		self->interrupt(self);
		self->join(self);
	}

	sprintf(tkey, "%lu", *ptid);

	/* discards thread data */
	_mtx_threads_ht->lock_write(_mtx_threads_ht);
	_threads_ht->remove(_threads_ht, tkey);
	self->mtx_interrupted->destroy(self->mtx_interrupted);
	self->mtx_active->destroy(self->mtx_active);
	self->cnd_start->destroy(self->cnd_start);
	self->mtx_start->destroy(self->mtx_start);
	free(self);
	_mtx_threads_ht->unlock(_mtx_threads_ht);

	if (0 == _threads_ht->size(_threads_ht))
	{
		/*
		*	_threads_ht is empty indicates the last thread has been just stopped.
		*	That indicates the threading module is no longer necessary so we discard all global data here
		*/
		/* destroys hashtable of threads */
		_threads_ht->destroy(_threads_ht);

		/* marks the _threads_ht is not initialized. */
		_threads_ht = AV_NULL;

		/* destroys the mutex for the threads hashtable */
		_mtx_threads_ht->destroy(_mtx_threads_ht);

		/* marks the _mtx_threads_ht is not initialized. */
		_mtx_threads_ht = AV_NULL;

		av_torb_service_release("log");
		_log = AV_NULL;
	}
}
#endif /* AV_MT */

av_result_t av_thread_create(av_runnable_t runnable, void *arg, av_thread_p *ppthread)
{
#ifdef AV_MT
	char               tkey[11]; /* a key string with size of "0x12345678\0" */
	av_thread_p        self = (av_thread_p)malloc(sizeof(av_thread_t));
	pthread_t          tid;
	pthread_attr_t     attr;
	av_result_t rc;

	if (!self) return AV_EMEM;

	self->runnable        = runnable;
	self->arg             = arg;
	self->interrupted     = AV_FALSE;
	self->active          = AV_FALSE;
	self->started         = AV_FALSE;

	if (AV_OK != (rc = av_rwmutex_create(&self->mtx_interrupted)))
	{
		free(self);
		return rc;
	}

	if (AV_OK != (rc = av_rwmutex_create(&self->mtx_active)))
	{
		self->mtx_interrupted->destroy(self->mtx_interrupted);
		free(self);
		return rc;
	}

	if (AV_OK != (rc = av_mutex_create(&self->mtx_start)))
	{
		self->mtx_active->destroy(self->mtx_active);
		self->mtx_interrupted->destroy(self->mtx_interrupted);
		free(self);
		return rc;
	}

	if (AV_OK != (rc = av_condition_create(&self->cnd_start)))
	{
		self->mtx_start->destroy(self->mtx_start);
		self->mtx_active->destroy(self->mtx_active);
		self->mtx_interrupted->destroy(self->mtx_interrupted);
		free(self);
		return rc;
	}

	self->is_interrupted  = av_thread_is_interrupted;
	self->is_active       = av_thread_is_active;
	self->interrupt       = av_thread_interrupt;
	self->start           = av_thread_start;
	self->join            = av_thread_join;
	self->detach          = av_thread_detach;
	self->equals          = av_thread_equals;
	self->destroy         = av_thread_destroy;

	/* on first created thread the global _mtx_threads_ht is not initialized yet */
	if (!_mtx_threads_ht)
	{
		av_torb_service_addref("log", (av_service_p*)&_log);

		if (AV_OK != (rc = av_rwmutex_create(&_mtx_threads_ht)))
		{
			av_torb_service_release("log");
			self->cnd_start->destroy(self->cnd_start);
			self->mtx_start->destroy(self->mtx_start);
			self->mtx_active->destroy(self->mtx_active);
			self->mtx_interrupted->destroy(self->mtx_interrupted);
			free(self);
			return rc;
		}
	}

	/* locks the global _threads_ht until the new spowned thread is registered into it */
	if (AV_OK != (rc = _mtx_threads_ht->lock_write(_mtx_threads_ht)))
	{
		self->cnd_start->destroy(self->cnd_start);
		self->mtx_start->destroy(self->mtx_start);
		self->mtx_active->destroy(self->mtx_active);
		self->mtx_interrupted->destroy(self->mtx_interrupted);
		free(self);
		return rc;
	}

	/* on first created thread the global _threads_ht is not initialized yet */
	if (!_threads_ht)
	{
		if (AV_OK != (rc = av_hash_create(AV_HASH_CAPACITY_MEDIUM, &_threads_ht)))
		{
			_mtx_threads_ht->unlock(_mtx_threads_ht);
			self->cnd_start->destroy(self->cnd_start);
			self->mtx_start->destroy(self->mtx_start);
			self->mtx_active->destroy(self->mtx_active);
			self->mtx_interrupted->destroy(self->mtx_interrupted);
			free(self);
			return rc;
		}
	}

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	/* spawns the new thread which will wait until invoked method \c start */
	if (AV_OK != (rc = av_thread_error_check("pthread_create",
											 pthread_create(&tid, &attr, runnable_executor, (void*)self))))
	{
		_mtx_threads_ht->unlock(_mtx_threads_ht);
		self->cnd_start->destroy(self->cnd_start);
		self->mtx_start->destroy(self->mtx_start);
		self->mtx_active->destroy(self->mtx_active);
		self->mtx_interrupted->destroy(self->mtx_interrupted);
		free(self);
		return rc;
	}

	self->tid = tid;

	/* adds the new (tid, self) pair to _threads_ht */
	{
		unsigned long* ptid = (unsigned long*)&tid;
		sprintf(tkey, "%lu", *ptid);
	}

	rc = _threads_ht->add(_threads_ht, tkey, self);
	if (AV_OK != rc)
	{
		_mtx_threads_ht->unlock(_mtx_threads_ht);
		self->cnd_start->signal(self->cnd_start);
		self->join(self);
		self->cnd_start->destroy(self->cnd_start);
		self->mtx_start->destroy(self->mtx_start);
		self->mtx_active->destroy(self->mtx_active);
		self->mtx_interrupted->destroy(self->mtx_interrupted);
		free(self);
		return rc;
	}

	_mtx_threads_ht->unlock(_mtx_threads_ht);

	*ppthread = self;
	return AV_OK;
#else
	av_assert(0, "AVGL is compiled without threading support. Recompile with defined AV_MT macro");
	return 0; /* suppress eventual warnings */
#endif /* AV_MT */
}

/* mutexes API */

/* Note: In non multithreading environment (AVGL compiled with AV_MT undefined)
*		 mutex and condition methods always succeed but do nothing.
*		 That allows other modules to use the av_thread module for thread safe code implementation.
*/

static av_result_t av_mutex_lock(av_mutex_p self)
{
	return
#ifdef AV_MT
		av_thread_error_check("pthread_mutex_lock", pthread_mutex_lock((pthread_mutex_t *)(self->mid)));
#else
		AV_OK;
#endif
}

static av_result_t av_mutex_trylock(av_mutex_p self)
{
	return
#ifdef AV_MT
		av_thread_error_check("pthread_mutex_trylock", pthread_mutex_trylock((pthread_mutex_t *)(self->mid)));
#else
		AV_OK;
#endif
}

static av_result_t av_mutex_unlock(av_mutex_p self)
{
	return
#ifdef AV_MT
		av_thread_error_check("pthread_mutex_unlock", pthread_mutex_unlock((pthread_mutex_t *)(self->mid)));
#else
		AV_OK;
#endif
}

static void av_mutex_destroy(av_mutex_p self)
{
#ifdef AV_MT
	pthread_mutex_destroy((pthread_mutex_t *)(self->mid));
	free(self->mid);
#endif
	free(self);
}

av_result_t av_mutex_create(av_mutex_p* ppmutex)
{
	int rc;
	av_mutex_p self = (av_mutex_p)malloc(sizeof(av_mutex_t));
	if (!self)
		return AV_EMEM;

	self->lock      = av_mutex_lock;
	self->trylock   = av_mutex_trylock;
	self->unlock    = av_mutex_unlock;
	self->destroy   = av_mutex_destroy;
#ifdef AV_MT
	self->mid       = malloc(sizeof(pthread_mutex_t));
	if (!self->mid)
	{
		free(self);
		return AV_EMEM;
	}

	if (AV_OK != (rc = av_thread_error_check("pthread_mutex_init", pthread_mutex_init((pthread_mutex_t*)(self->mid), 0))))
	{
		free(self->mid);
		free(self);
		return rc;
	}
#else
	self->mid = 0;
#endif
	*ppmutex = self;
	return AV_OK;
}

/* R/W mutexes API */

static av_result_t av_rwmutex_lock_read(av_rwmutex_p self)
{
	return
#ifdef AV_MT
		pthread_rwlock_rdlock((pthread_rwlock_t *)(self->mid));
#else
		AV_OK;
#endif
}

static av_result_t av_rwmutex_lock_write(av_rwmutex_p self)
{
	return
#ifdef AV_MT
		av_thread_error_check("pthread_rwlock_wrlock", pthread_rwlock_wrlock((pthread_rwlock_t *)(self->mid)));
#else
		AV_OK;
#endif
}

static av_result_t av_rwmutex_trylock_read(av_rwmutex_p self)
{
	return
#ifdef AV_MT
		av_thread_error_check("pthread_rwlock_tryrdlock", pthread_rwlock_tryrdlock((pthread_rwlock_t *)(self->mid)));
#else
		AV_OK;
#endif
}

static av_result_t av_rwmutex_trylock_write(av_rwmutex_p self)
{
	return
#ifdef AV_MT
		av_thread_error_check("pthread_rwlock_trywrlock", pthread_rwlock_trywrlock((pthread_rwlock_t *)(self->mid)));
#else
		AV_OK;
#endif
}

static av_result_t av_rwmutex_unlock(av_rwmutex_p self)
{
	return
#ifdef AV_MT
		av_thread_error_check("pthread_rwlock_unlock", pthread_rwlock_unlock((pthread_rwlock_t *)(self->mid)));
#else
		AV_OK;
#endif
}

static void av_rwmutex_destroy(av_rwmutex_p self)
{
#ifdef AV_MT
	pthread_rwlock_destroy((pthread_rwlock_t *)(self->mid));
	free(self->mid);
#endif
	free(self);
}

av_result_t av_rwmutex_create( av_rwmutex_p* ppmutex )
{
	int rc;
	av_rwmutex_p self   = (av_rwmutex_p)malloc(sizeof(av_rwmutex_t));
	if (!self) return AV_EMEM;

	self->lock_read     = av_rwmutex_lock_read;
	self->lock_write    = av_rwmutex_lock_write;
	self->trylock_read  = av_rwmutex_trylock_read;
	self->trylock_write = av_rwmutex_trylock_write;
	self->unlock        = av_rwmutex_unlock;
	self->destroy       = av_rwmutex_destroy;

#ifdef AV_MT
	self->mid           = malloc(sizeof(pthread_rwlock_t));
	if (!self->mid)
	{
		free(self);
		return AV_EMEM;
	}

	if (AV_OK != (rc = av_thread_error_check("pthread_rwlock_init", pthread_rwlock_init((pthread_rwlock_t*)(self->mid), 0))))
	{
		free(self->mid);
		free(self);
		return rc;
	}
#else
	self->mid = 0;
#endif
	*ppmutex = self;
	return AV_OK;
}

/* conditions API */

static av_result_t av_condition_broadcast(av_condition_p self)
{
	return
#ifdef AV_MT
		av_thread_error_check("pthread_cond_broadcast", pthread_cond_broadcast((pthread_cond_t*)(self->cid)));
#else
		AV_OK;
#endif
}

static av_result_t av_condition_signal(av_condition_p self)
{
	return
#ifdef AV_MT
		av_thread_error_check("pthread_cond_signal", pthread_cond_signal((pthread_cond_t*)(self->cid)));
#else
		AV_OK;
#endif
}

static av_result_t av_condition_wait(av_condition_p self, av_mutex_p mutex)
{
	return
#ifdef AV_MT
		av_thread_error_check("pthread_cond_wait", pthread_cond_wait((pthread_cond_t*)(self->cid), (pthread_mutex_t*)(mutex->mid)));
#else
		AV_OK;
#endif
}

static av_result_t av_condition_wait_ms(av_condition_p self, av_mutex_p mutex, unsigned long mills)
{
#ifdef AV_MT
	struct timespec abstime;
	abstime.tv_sec  = mills / 1000;
	abstime.tv_nsec = (mills % 1000)*1000000L;
	return av_thread_error_check("pthread_cond_timedwait", pthread_cond_timedwait((pthread_cond_t*)(self->cid),
																				  (pthread_mutex_t*)(mutex->mid), &abstime));
#else
	return AV_OK;
#endif
}

static av_result_t av_condition_wait_ns(av_condition_p self, av_mutex_p mutex, unsigned long nanos)
{
#ifdef AV_MT
	struct timespec abstime;
	abstime.tv_sec  = 0;
	abstime.tv_nsec = nanos;
	return av_thread_error_check("pthread_cond_timedwait", pthread_cond_timedwait((pthread_cond_t*)(self->cid),
																				  (pthread_mutex_t*)(mutex->mid), &abstime));
#else
	return AV_OK;
#endif
}

static void av_condition_destroy(av_condition_p self)
{
#ifdef AV_MT
	pthread_cond_destroy((pthread_cond_t*)(self->cid));
	free(self->cid);
#endif
	free(self);
}

av_result_t av_condition_create( av_condition_p* ppcondition )
{
	int rc;
	av_condition_p self   = (av_condition_p)malloc(sizeof(av_condition_t));
	if (!self)
		return AV_EMEM;

	self->broadcast       = av_condition_broadcast;
	self->signal          = av_condition_signal;
	self->wait            = av_condition_wait;
	self->wait_ms         = av_condition_wait_ms;
	self->wait_ns         = av_condition_wait_ns;
	self->destroy         = av_condition_destroy;

#ifdef AV_MT
	self->cid             = malloc(sizeof(pthread_cond_t));
	if (!self->cid)
	{
		free(self);
		return AV_EMEM;
	}

	if (AV_OK != (rc = av_thread_error_check("pthread_cond_init", pthread_cond_init((pthread_cond_t*)(self->cid), 0))))
	{
		free(self->cid);
		free(self);
		return rc;
	}
#else
	self->cid = 0;
#endif
	*ppcondition = self;
	return AV_OK;
}

static unsigned int av_sync_queue_size(av_sync_queue_p self)
{
	int size;
	self->mutex->lock(self->mutex);
	size = self->queue->size(self->queue);
	self->mutex->unlock(self->mutex);
	return size;
}

static av_result_t av_sync_queue_push(av_sync_queue_p self, void* element)
{
	av_result_t rc;
	self->mutex->lock(self->mutex);
	while (self->elements_max == self->queue->size(self->queue) && !self->is_abort)
	{
		self->condnotfull->wait(self->condnotfull, self->mutex);
	}
	if (!self->is_abort)
	{
		rc = self->queue->push_last(self->queue, element);
		if (AV_OK == rc)
		{
			self->condnotempty->signal(self->condnotempty);
		}
	}
	else
	{
		rc = AV_EINTERRUPT;
	}
	self->mutex->unlock(self->mutex);
	return rc;
}

static av_result_t av_sync_queue_pop(av_sync_queue_p self, void** element)
{
	av_result_t rc;
	self->mutex->lock(self->mutex);
	while (0 == self->queue->size(self->queue) && !self->is_abort)
	{
		self->condnotempty->wait(self->condnotempty, self->mutex);
	}
	if (!self->is_abort)
	{
		*element = self->queue->pop_first(self->queue);
		self->condnotfull->signal(self->condnotfull);
		rc = AV_OK;
	}
	else
	{
		rc = AV_EINTERRUPT;
	}
	self->mutex->unlock(self->mutex);
	return rc;
}

static av_result_t av_sync_queue_peek(av_sync_queue_p self, void** element)
{
	av_result_t rc;
	self->mutex->lock(self->mutex);
	while (0 == self->queue->size(self->queue) && !self->is_abort)
	{
		self->condnotempty->wait(self->condnotempty, self->mutex);
	}
	if (!self->is_abort)
	{
		self->queue->first(self->queue);
		*element = self->queue->get(self->queue);
		rc = AV_OK;
	}
	else
	{
		rc = AV_EINTERRUPT;
	}
	self->mutex->unlock(self->mutex);
	return rc;
}

static void av_sync_queue_abort(av_sync_queue_p self)
{
	self->mutex->lock(self->mutex);
	self->is_abort = AV_TRUE;
	self->condnotfull->broadcast(self->condnotfull);
	self->condnotempty->broadcast(self->condnotempty);
	self->mutex->unlock(self->mutex);
}

static void av_sync_queue_destroy(void* pqueue)
{
	av_sync_queue_p self = (av_sync_queue_p)pqueue;
	self->abort(self);
	self->mutex->destroy(self->mutex);
	self->condnotfull->destroy(self->condnotfull);
	self->condnotempty->destroy(self->condnotempty);
	self->queue->destroy(self->queue);
	free(self);
}

av_result_t av_sync_queue_create(int elements_max, av_sync_queue_p* ppqueue)
{
	av_result_t rc;
	av_sync_queue_p self = (av_sync_queue_p)malloc(sizeof(av_sync_queue_t));
	if (!self)
		return AV_EMEM;

	if (AV_OK != (rc = av_list_create(&self->queue)))
	{
		free(self);
		return rc;
	}

	if (AV_OK != (rc = av_mutex_create(&self->mutex)))
	{
		self->queue->destroy(self->queue);
		free(self);
		return rc;
	}

	if (AV_OK != (rc = av_condition_create(&self->condnotfull)))
	{
		self->queue->destroy(self->queue);
		self->mutex->destroy(self->mutex);
		free(self);
		return rc;
	}

	if (AV_OK != (rc = av_condition_create(&self->condnotempty)))
	{
		self->condnotfull->destroy(self->condnotfull);
		self->queue->destroy(self->queue);
		self->mutex->destroy(self->mutex);
		free(self);
		return rc;
	}

	self->is_abort     = AV_FALSE;
	self->elements_max = elements_max;
	self->size         = av_sync_queue_size;
	self->push         = av_sync_queue_push;
	self->pop          = av_sync_queue_pop;
	self->peek         = av_sync_queue_peek;
	self->abort        = av_sync_queue_abort;
	self->destroy      = av_sync_queue_destroy;
	*ppqueue           = self;
	return AV_OK;
}
