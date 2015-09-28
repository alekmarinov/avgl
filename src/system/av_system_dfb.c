/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_system_dfb.c                                    */
/* Description:   Defines class system dfb (av_system_dfb_t)         */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_DIRECTFB

#include <av.h>
#include <av_log.h>
#include <av_system.h>
#include <malloc.h>

#include "../video/av_video_dfb.h"
#include "av_system_dfb.h"

/* exported prototypes */
av_result_t av_dfb_error_process(DFBResult, const char*, const char*, int);
av_result_t av_system_dfb_register_torba(void);

/* imported prototypes */
av_result_t av_audio_dfb_register_torba(void);
av_result_t av_video_dfb_register_torba(void);
av_result_t av_input_dfb_register_torba(void);
av_result_t av_timer_dfb_register_torba(void);

static av_log_p _log = AV_NULL;

static const char* _err_mem        = "out of memory";
static const char* _err_vmem       = "out of video memory";
static const char* _err_init       = "initialization error";
static const char* _err_supported  = "not supported";
static const char* _err_busy       = "busy during operation";
static const char* _err_arg        = "invalid argument";
static const char* _err_found      = "not found error";
static const char* _err_timeout    = "timed out";
static const char* _err_interrupt  = "interrupted";
static const char* _err_again      = "temporary unavailable";
static const char* _err_io         = "general I/O error";
static const char* _err_perm       = "permission denied";
static const char* _err_eof        = "end of file";
static const char* _err_locked     = "already locked";
static const char* _err_limit      = "limit exceeded";
#if DIRECTFB_NEWER(1,0,0)
static const char* _err_suspended  = "requested object suspended";
#endif
static const char* _err_general    = "DirectFB failure";

av_result_t av_dfb_error_process(DFBResult rc, const char* funcname, const char* srcfilename, int linenumber)
{
	av_result_t averr = AV_OK;
	if (rc != DFB_OK)
	{
		const char* errmsg = 0;
		switch (rc)
		{
			case DFB_NOSHAREDMEMORY:
			case DFB_NOSYSTEMMEMORY: averr = AV_EMEM;       errmsg = _err_mem;       break;
			case DFB_NOVIDEOMEMORY:  averr = AV_EVMEM;      errmsg = _err_vmem;      break;
			case DFB_INIT:           averr = AV_EINIT;      errmsg = _err_init;      break;
			case DFB_NOIMPL:
			case DFB_UNIMPLEMENTED:
			case DFB_UNSUPPORTED:    averr = AV_ESUPPORTED; errmsg = _err_supported; break;
			case DFB_NOSUCHMETHOD:
			case DFB_NOSUCHINSTANCE:
			case DFB_INVAREA:
			case DFB_THIZNULL:
			case DFB_DESTROYED:
			case DFB_NOCONTEXT:
			case DFB_BUFFEREMPTY:
			case DFB_INVARG:         averr = AV_EARG;       errmsg = _err_arg;       break;
			case DFB_IDNOTFOUND:
			case DFB_MISSINGFONT:
			case DFB_MISSINGIMAGE:
			case DFB_ITEMNOTFOUND:
			case DFB_FILENOTFOUND:   averr = AV_EFOUND;     errmsg = _err_found;     break;
			case DFB_BUSY:           averr = AV_EBUSY;      errmsg = _err_busy;      break;
			case DFB_INTERRUPTED:    averr = AV_EINTERRUPT; errmsg = _err_interrupt; break;
			case DFB_TIMEOUT:        averr = AV_ETIMEDOUT;  errmsg = _err_timeout;   break;
			case DFB_TEMPUNAVAIL:    averr = AV_EAGAIN;     errmsg = _err_again;     break;
			case DFB_IO:             averr = AV_EIO;        errmsg = _err_io;        break;
			case DFB_ACCESSDENIED:   averr = AV_EPERM;      errmsg = _err_perm;      break;
			case DFB_EOF:            averr = AV_EEOF;       errmsg = _err_eof;       break;
			case DFB_LOCKED:         averr = AV_ELOCKED;    errmsg = _err_locked;    break;
			case DFB_LIMITEXCEEDED:  averr = AV_ELIMIT;     errmsg = _err_limit;     break;
#if DIRECTFB_NEWER(1,0,0)
			case DFB_SUSPENDED:      averr = AV_ESUSPENDED; errmsg = _err_suspended; break;
#endif
			default:
									 averr = AV_EGENERAL;   errmsg = _err_general;   break;
		}

		if (_log) _log->error(_log, "%s returned error (%d) `%s' %s:%d",
								funcname, rc, errmsg, srcfilename, linenumber);
	}
	return averr;
}

#define av_dfb_error_check(funcname, rc) av_dfb_error_process(rc, funcname, __FILE__, __LINE__)

/* gets instance to DirectFB audio */
static av_result_t av_system_dfb_get_audio(av_system_p self, av_audio_p* ppaudio)
{
	av_result_t rc;
	if (AV_NULL == self->input)
		if (AV_OK != (rc = av_torb_create_object("audio_dfb", (av_object_p*)&(self->audio))))
			return rc;
	*ppaudio = self->audio;
	return AV_OK;
}

/* gets instance to DirectFB video */
static av_result_t av_system_dfb_get_video(av_system_p self, av_video_p* ppvideo)
{
	av_result_t rc;
	if (AV_NULL == self->video)
		if (AV_OK != (rc = av_torb_create_object("video_dfb", (av_object_p*)&(self->video))))
			return rc;
	*ppvideo = self->video;
	self->video->system = self;
	return AV_OK;
}

/* gets instance to DirectFB input */
static av_result_t av_system_dfb_get_input(av_system_p self, av_input_p* ppinput)
{
	av_result_t rc;
	if (AV_NULL == self->input)
	{
		/* Checks if video is already initialized. If not, produce error */
		if (AV_NULL == self->video)
		{
			_log->error(_log, "dfb video must be initialized before creating dfb input");
			return AV_EINIT;
		}
		/* Creates input object */
		if (AV_OK != (rc = av_torb_create_object("input_dfb", (av_object_p*)&(self->input))))
			return rc;
	}
	*ppinput = self->input;
	return AV_OK;
}

/* gets instance to DirectFB timer */
static av_result_t av_system_dfb_get_timer(av_system_p self, av_timer_p* pptimer)
{
	av_result_t rc;
	if (AV_NULL == self->timer)
		if (AV_OK != (rc = av_torb_create_object("timer_dfb", (av_object_p*)&(self->timer))))
			return rc;
	*pptimer = self->timer;
	return AV_OK;
}

static void av_system_dfb_destructor(void* psystemdfb)
{
	av_system_p self = (av_system_p)psystemdfb;
	if (self->audio) O_destroy(self->audio);
	if (self->timer) O_destroy(self->timer);
	if (self->input) O_destroy(self->input);
	if (self->video) O_destroy(self->video);
	/* release log service */
	av_torb_service_release("log");
}

/* Initializes memory given by the input pointer with the system dfb class information */
static av_result_t av_system_dfb_constructor(av_object_p object)
{
	av_result_t rc;
	av_system_p self = (av_system_p)object;

	self->get_audio = av_system_dfb_get_audio;
	self->get_video = av_system_dfb_get_video;
	self->get_input = av_system_dfb_get_input;
	self->get_timer = av_system_dfb_get_timer;

	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&_log)))
		return rc;

	/* Setup environment for proper DFB functionality */
	av_system_dfb_setup_param();

	/* initializes DirectFB library */
	if (AV_OK != (rc = av_dfb_error_check("DFB_Init", DirectFBInit(0, 0))))
	{
		av_torb_service_release("log");
		return rc;
	}

	return AV_OK;
}

/* Registers system dfb class into TORBA class repository */
av_result_t av_system_dfb_register_torba(void)
{
	av_result_t rc;
	if (AV_OK != (rc = av_audio_dfb_register_torba()))
		return rc;
	if (AV_OK != (rc = av_video_dfb_register_torba()))
		return rc;
	if (AV_OK != (rc = av_input_dfb_register_torba()))
		return rc;
	if (AV_OK != (rc = av_timer_dfb_register_torba()))
		return rc;
	if (AV_OK != (rc = av_torb_register_class("system_dfb", "system",
											  sizeof(av_system_t),
											  av_system_dfb_constructor, av_system_dfb_destructor)))
		return rc;

	return AV_OK;
}

#endif /* WITH_SYSTEM_DIRECTFB */
