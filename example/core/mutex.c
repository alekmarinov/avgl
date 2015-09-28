/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      mutex.c                                            */
/* Description:   Demonstrate mutex usage                            */
/*                                                                   */
/*********************************************************************/

#define _BSD_SOURCE
#include <stdio.h>      /* printf */
#include <stdlib.h>     /* exit */
#include <unistd.h>     /* usleep */
#include <av_torb.h>    /* TORBA */
#include <av_thread.h>  /* threading API */

#define sleep(x) usleep(1000*(x))

/* work time (in seconds) */
#define SLEEP_SECS 2

/* time (in microseconds) the threads a sleeping */
#define THREAD_USLEEP 10

/*
*	amount of concurrent threads
* 
*	Note: When the example is started with valgrind (memory checker)
*	and the value THREADS_COUNT exceeds 284 the next thread creation returns
*	with AV_EARG resulted by the pthread error code EINVAL.
*	When the example is started without valgrind and the value THREADS_COUNT
*	exceeds 304 the next thread creation returns AV_EMEM resulted by the
*	pthread_create returning ENOMEM. The notice above is only observed on 
*	one machine with Linux Fedora Core 6.
*/

/*	The maximum allowed with valgrind
	#define THREADS_COUNT 284

	The maximum allowed without valgrind
	#define THREADS_COUNT 302
*/
#define THREADS_COUNT 284

/* a mutex protecting access to the shared store variable */
av_mutex_p mutex;

/* the consumer algorithm */
static int thread_body(av_thread_p self)
{
	av_bool_t interrupted;

	self->is_interrupted(self, &interrupted);
	while (!interrupted)
	{
		mutex->lock(mutex);
		/* printf("Thread %p is working\n", self); */
		usleep(THREAD_USLEEP);
		mutex->unlock(mutex);
		self->is_interrupted(self, &interrupted);
	}
	return 0;
}

/* entry point */
int main(void)
{
	int i;
	av_thread_p threads[THREADS_COUNT];

	/* initialization */
	av_torb_init();
	av_mutex_create(&mutex);

	/* create threads */
	for (i=0; i<THREADS_COUNT; i++)
	{
		av_result_t rc;
		if (AV_OK != (rc = av_thread_create(thread_body, AV_NULL, &threads[i])))
		{
			printf("Error (%d) while creating thread\n", rc);
			exit(1);
		}
	}

	/* start threads */
	for (i=0; i<THREADS_COUNT; i++)
	{
		threads[i]->start(threads[i]);
	}

	/* sleep while the threads work */
	sleep(SLEEP_SECS);

	/* marking threads interrupted */
	for (i=0; i<THREADS_COUNT; i++)
	{
		threads[i]->interrupt(threads[i]);
	}

	/* join with workers threads */
	for (i=0; i<THREADS_COUNT; i++)
	{
		threads[i]->join(threads[i]);
	}

	/* deinitialization */
	for (i=0; i<THREADS_COUNT; i++)
	{
		threads[i]->destroy(threads[i]);
	}

	mutex->destroy(mutex);
	av_torb_done();

	return 0;
}
