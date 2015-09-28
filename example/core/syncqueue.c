/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      syncqueue.c                                        */
/* Description:   Demonstrate sync queue usage                       */
/*                                                                   */
/*********************************************************************/

/* FIXME: use av_timer for usleep */
#define _XOPEN_SOURCE 500
#include <unistd.h>

#include <av_thread.h>
#include <av_log.h>

#define sleep(x) usleep(1000*(x))

#define THREADS_COUNT 2
#define SYNC_QUEUE_SIZE 2

int _pushcount = 0;
int _popcount = 0;
av_mutex_p _mtx_push;
av_mutex_p _mtx_pop;
av_log_p _log = 0;
av_sync_queue_p sync_queue;
av_bool_t pusher = AV_TRUE;
av_bool_t poper = AV_FALSE;

static int thread_body(av_thread_p thread)
{
	av_bool_t pushorpoper = *((av_bool_t*)thread->arg);
	for (;;)
	{
		av_bool_t interrupted;
		av_thread_p element = thread;

		if (pushorpoper)
		{
			sync_queue->push(sync_queue, (void*)thread);
			_mtx_push->lock(_mtx_push);
			if (_pushcount>0)
				_pushcount++;
			_mtx_push->unlock(_mtx_push);
		}
		else
		{
			sync_queue->pop(sync_queue, (void**)&element);
			_mtx_pop->lock(_mtx_pop);
			if (_popcount>0)
				_popcount++;
			_mtx_pop->unlock(_mtx_pop);
		}

		thread->is_interrupted(thread, &interrupted);
		if (interrupted) break;
		usleep(10);
	}
	return 0;
}

int main( void )
{
	int i;
	av_result_t rc;
	av_thread_p threads[THREADS_COUNT];
	
	if (AV_OK != (rc = av_torb_init()))
		return rc;

	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&_log)))
		return rc;

	if (AV_OK != (rc = av_sync_queue_create(SYNC_QUEUE_SIZE, &sync_queue)))
		return rc;

	av_mutex_create(&_mtx_push);
	av_mutex_create(&_mtx_pop);

	for (i=0; i<THREADS_COUNT; i++)
	{
		av_bool_t* pushorpoper;
		if (i & 1)
			pushorpoper = &pusher;
		else
			pushorpoper = &poper;

		av_thread_create(thread_body, (void*)pushorpoper, &threads[i]);
	}
	for (i=0; i<THREADS_COUNT; i++)
	{
		_log->info(_log, "starting thread %d", i);
		threads[i]->start(threads[i]);
		usleep(10);
	}

	_mtx_pop->lock(_mtx_pop);
	_popcount++;
	_mtx_pop->unlock(_mtx_pop);
	_mtx_push->lock(_mtx_push);
	_pushcount++;
	_mtx_push->unlock(_mtx_push);
	_log->info(_log, "Working...");
	sleep(5);
	_log->info(_log, "Stoping...");

	for (i=0; i<THREADS_COUNT; i++)
	{
		threads[i]->interrupt(threads[i]);
	}
	sync_queue->abort(sync_queue);

	for (i=0; i<THREADS_COUNT; i++)
	{
		threads[i]->destroy(threads[i]);
	}
	
	sync_queue->destroy(sync_queue);
	_mtx_pop->destroy(_mtx_pop);
	_mtx_push->destroy(_mtx_push);
	av_torb_service_release("log");
	av_torb_done();
	_log->info(_log, "pushes count = %d", _pushcount-1);
	_log->info(_log, "pops   count = %d", _popcount-1);
	return 0;
}
