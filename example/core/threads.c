/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      threads.c                                          */
/* Description:   Demonstrate threads usage                          */
/*                                                                   */
/*********************************************************************/

#define _BSD_SOURCE
#include <unistd.h>     /* sleep/usleep */
#include <av_thread.h>  /* threading API */
#include <av_torb.h>    /* TORBA */
#include <av_log.h>     /* logging API */

#define sleep(x) usleep(1000*(x))

/* work time (in seconds) */
#define SLEEP_SECS 5

/* time (in microseconds) the workers a sleeping */
#define WORKERS_USLEEP 10

/*	
*	amount of consumers and producers, total workers = 2*WORKERS_COUNT
*
*	Note: When the example is started with valgrind (memory checker)
*	and the value WORKERS_COUNT exceeds 142 (sometimes 143) the next thread 
*	creation returns with AV_EARG resulted by the pthread error code EINVAL.
*	When the example is started without valgrind and the value THREADS_COUNT
*	exceeds 151 the next thread creation returns AV_EMEM resulted by the
*	pthread_create returning ENOMEM. The notice above is only observed on 
*	one machine with Linux Fedora Core 6.
*/

/*	The maximum allowed with valgrind
	#define WORKERS_COUNT 142

	The maximum allowed without valgrind
	#define WORKERS_COUNT 151
*/
#define WORKERS_COUNT  42

/* a store limit */
#define STORE_LIMIT 4

/* shared storage for producers and consumers */
int store = 0;

/* a condition the store is not full */
av_condition_p condition_not_full;

/* a condition the store is not empty */
av_condition_p condition_not_empty;

/* a mutex protecting access to the shared store variable */
av_mutex_p mutex;

/* logger object */
av_log_p _log;

const char *error_msg[] = 
{
	"OK",
	"Out of memory",
	"Invalid argument",
	"Busy",
	"Deadlock",
	"No such process",
	"Out of resources",
	"Timeout"
};

#define check_error(rc) av_assert(AV_OK == rc, error_msg[rc]);

/* the consumer algorithm */
static int consumer_thread(av_thread_p self)
{
	av_bool_t interrupted;
	check_error(self->is_interrupted(self, &interrupted));

	while (!interrupted)
	{
		_log->debug(_log, "Consumer %p lock", self);
		check_error(mutex->lock(mutex));
		_log->debug(_log, "Consumer %p is working...", self);
		check_error(self->is_interrupted(self, &interrupted));
		while ((store == 0) && !interrupted)
		{
			_log->debug(_log, "Consumer %p waits...", self);
			check_error(condition_not_empty->wait(condition_not_empty, mutex));
			_log->debug(_log, "Consumer %p waked up");
			check_error(self->is_interrupted(self, &interrupted));
		}
		check_error(self->is_interrupted(self, &interrupted));
		if (!interrupted) store--;
		_log->debug(_log, "Consumer %p: store = %d", self, store);
		check_error(mutex->unlock(mutex));
		check_error(condition_not_full->broadcast(condition_not_full));
		usleep(WORKERS_USLEEP);
		check_error(self->is_interrupted(self, &interrupted));
	}
	_log->debug(_log, "Consumer %p finished");
	return 0;
}

/* the producer algorithm */
static int producer_thread(av_thread_p self)
{
	av_bool_t interrupted;
	check_error(self->is_interrupted(self, &interrupted));

	while (!interrupted)
	{
		_log->debug(_log, "Producer %p lock", self);
		check_error(mutex->lock(mutex));
		_log->debug(_log, "Producer %p is working...", self);
		check_error(self->is_interrupted(self, &interrupted));
		while ((store == STORE_LIMIT) && !interrupted)
		{
			_log->debug(_log, "Producer %p waits...", self);
			check_error(condition_not_full->wait(condition_not_full, mutex));
			_log->debug(_log, "Producer %p waked up");
			check_error(self->is_interrupted(self, &interrupted));
		}
		check_error(self->is_interrupted(self, &interrupted));
		if (!interrupted) store++;
		_log->debug(_log, "Producer %p: store = %d", self, store);
		check_error(mutex->unlock(mutex));
		check_error(condition_not_empty->broadcast(condition_not_empty));
		usleep(WORKERS_USLEEP);
		check_error(self->is_interrupted(self, &interrupted));
	}
	_log->debug(_log, "Producer %p finished");
	return 0;
}


/* entry point */
int main(void)
{
	int i;
	av_thread_p consumers[WORKERS_COUNT],
			    producers[WORKERS_COUNT];

	/* initialization */
	av_torb_init();
	av_torb_service_addref("log", (av_service_p*)&_log);

	_log->info(_log, "Threads initialized");

	check_error(av_condition_create(&condition_not_full));
	check_error(av_condition_create(&condition_not_empty));

	check_error(av_mutex_create(&mutex));
	
	/* create workers */
	_log->info(_log, "Creates workers");
	for (i=0; i<WORKERS_COUNT; i++)
	{
		check_error(av_thread_create(consumer_thread, AV_NULL, &consumers[i]));
		check_error(av_thread_create(producer_thread, AV_NULL, &producers[i]));
	}

	/* start workers */
	_log->info(_log, "Starts workers");
	for (i=0; i<WORKERS_COUNT; i++)
	{
		check_error(consumers[i]->start(consumers[i]));
		check_error(producers[i]->start(producers[i]));
	}

	/* sleep while the workers work */
	_log->info(_log, "Waits workers to do something");
	sleep(SLEEP_SECS);

	/* marking workers interrupted */
	_log->info(_log, "Stops workers");
	for (i=0; i<WORKERS_COUNT; i++)
	{
		_log->debug(_log, "Interrupt consumer %p", consumers[i]);
		check_error(consumers[i]->interrupt(consumers[i]));
		_log->debug(_log, "Interrupt producer %p", producers[i]);
		check_error(producers[i]->interrupt(producers[i]));
	}

	/* wake up sleeprs so they be able respond to the interruption */
	check_error(condition_not_empty->broadcast(condition_not_empty));
	check_error(condition_not_full->broadcast(condition_not_full));

	/* join with workers threads */
	for (i=0; i<WORKERS_COUNT; i++)
	{
		_log->debug(_log, "Joins with consumer %p", consumers[i]);
		check_error(consumers[i]->join(consumers[i]));
		_log->debug(_log, "Joins producer %p", producers[i]);
		check_error(producers[i]->join(producers[i]));
	}

	/* deinitialization */
	for (i=0; i<WORKERS_COUNT; i++)
	{
		consumers[i]->destroy(consumers[i]);
		producers[i]->destroy(producers[i]);
	}

	mutex->destroy(mutex);

	condition_not_full->destroy(condition_not_full);
	condition_not_empty->destroy(condition_not_empty);

	_log->info(_log, "Threads deinitialized");

	av_torb_service_release("log");
	av_torb_done();

	return 0;
}
