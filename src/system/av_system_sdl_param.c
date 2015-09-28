/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_system_sdl_param.c                              */
/* Description:   Setups proper SDL environment                      */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_SYSTEM_SDL

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#ifdef WITH_X11
# include <X11/X.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/keysym.h>
# include <X11/extensions/Xvlib.h>
#endif

#include <av_prefs.h>
#include "av_system_sdl.h"
#include "../video/av_video_sdl.h"

#define AUDIODEV "AUDIODEV"
/* 	The audio device to use, if SDL_PATH_DSP isn't set.*/

#define SDL_AUDIODRIVER "SDL_AUDIODRIVER"
/* 	Selects the audio driver for SDL to use. Possible values, in the
* 	order they are tried if this variable is not set:
*
* 	openbsd	   (OpenBSD)
* 	dsp	   (OSS /dev/dsp: Linux, Solaris, BSD etc)
* 	alsa	   (Linux)
* 	audio	   (Unix style /dev/audio: SunOS, Solaris etc)
* 	AL	   (Irix)
* 	artsc	   (ARTS audio daemon)
* 	esd	   (esound audio daemon)
* 	nas	   (NAS audio daemon)
* 	dma	   (OSS /dev/dsp, using DMA)
* 	dsound	   (Win32 DirectX)
* 	waveout	   (Win32 WaveOut)
* 	baudio	   (BeOS)
* 	sndmgr	   (MacOS SoundManager)
* 	paud	   (AIX)
* 	AHI	   (Amiga)
* 	disk	   (all; output to file)
*/

#define SDL_CDROM "SDL_CDROM"
/* 	A colon-separated list of CD-ROM devices to use, in addition to
* 	the standard devices (typically /dev/cdrom, platform-dependant).
*/

#define SDL_DEBUG "SDL_DEBUG"
/* 	If set, causes every call to SDL_SetError() to also print an
* 	error message on stderr.
*/

#define SDL_DISKAUDIOFILE "SDL_DISKAUDIOFILE"
/* 	The name of the output file for the "disk" audio driver. If not
* 	set, the name "sdlaudio.raw" is used.
*/

#define SDL_DISKAUDIODELAY "SDL_DISKAUDIODELAY"
/* 	For the "disk" audio driver, how long to wait (in ms) before writing
* 	a full sound buffer. The default is 150 ms.
*/

#define SDL_DSP_NOSELECT "SDL_DSP_NOSELECT"
/* 	For some audio drivers (alsa, paud, dma and dsp), don't use
* 	select() but a timed method instead. May cure some audio problems,
* 	or cause others.
*/

#define SDL_ELO_MIN_X "SDL_ELO_MIN_X"
#define SDL_ELO_MIN_Y "SDL_ELO_MIN_Y"
#define SDL_ELO_MAX_X "SDL_ELO_MAX_X"
#define SDL_ELO_MAX_Y "SDL_ELO_MAX_Y"
/* 	Calibration values for the fbcon driver, for the Elographics
*         touchscreen controller. Default values are 400, 500, 3670, 3540
*         respectively.
*/

#define SDL_FBACCEL "SDL_FBACCEL"
/* 	If set to 0, disable hardware acceleration in the linux fbcon driver.
*/

#define SDL_FBDEV "SDL_FBDEV"
/* 	Frame buffer device to use in the linux fbcon driver, instead of
* 	/dev/fb0
*/

#define SDL_FULLSCREEN_UPDATE "SDL_FULLSCREEN_UPDATE"
/* 	In the ps2gs driver, sets the SDL_ASYNCBLIT flag on the display
* 	surface.
*/

#define SDL_JOYSTICK_DEVICE "SDL_JOYSTICK_DEVICE"
/* 	Joystick device to use in the linux joystick driver, in addition
* 	to the usual: /dev/js*, /dev/input/event*, /dev/input/js*
*/

#define SDL_LINUX_JOYSTICK "SDL_LINUX_JOYSTICK"
/* 	Special joystick configuration string for linux. The format is
* 	"name numaxes numhats numballs"
* 	where name is the name string of the joystick (possibly in single
* 	quotes), and the rest are the number of axes, hats and balls
* 	respectively.
*/

#define SDL_MOUSE_RELATIVE "SDL_MOUSE_RELATIVE"
/* 	If set to 0, do not use mouse relative mode in X11. The default is
* 	to use it if the mouse is hidden and input is grabbed.
*/

#define SDL_MOUSEDEV "SDL_MOUSEDEV"
/* 	The mouse device to use for the linux fbcon driver. If not set,
* 	SDL first tries to use GPM in repeater mode, then various other
* 	devices (/dev/pcaux, /dev/adbmouse, /dev/mouse etc).
*/

#define SDL_MOUSEDEV_IMPS2 "SDL_MOUSEDEV_IMPS2"
/* 	If set, SDL will not try to auto-detect	the IMPS/2 protocol of
* 	a PS/2 mouse but use it right away. For the fbcon and ps2gs drivers.
*/

#define SDL_MOUSEDRV "SDL_MOUSEDRV"
/* 	For the linux fbcon driver: if set to ELO, use the ELO touchscreen
* 	controller as a pointer device
*/

#define SDL_NO_RAWKBD "SDL_NO_RAWKBD"
/* 	For the libvga driver: If set, do not attempt to put the keyboard
* 	in raw mode.
*/

#define SDL_NOMOUSE "SDL_NOMOUSE"
/* 	If set, the linux fbcon driver will not use a mouse at all.
*/

#define SDL_PATH_DSP "SDL_PATH_DSP"
/* 	The audio device to use. If not set, SDL tries AUDIODEV and then
* 	a platform-dependent default value (/dev/audio on Solaris,
* 	/dev/dsp on Linux etc).
*/

#define SDL_VIDEODRIVER "SDL_VIDEODRIVER"
/* 	Selectes the video driver for SDL to use. Possible values, in the
*         order they are tried if this variable is not set:
*
* 	x11
* 	dga		(the XFree86 DGA2)
* 	nanox		(Linux)
* 	fbcon		(Linux)
* 	directfb	(Linux)
* 	ps2gs		(Playstation 2)
* 	ggi
* 	vgl		(BSD)
* 	svgalib		(Linux)
* 	aalib
* 	directx		(Win32)
* 	windib		(Win32)
* 	bwindow		(BeOS)
* 	toolbox		(MacOS Classic)
* 	DSp		(MacOS Classic)
* 	Quartz		(MacOS X)
* 	CGX		(Amiga)
* 	photon		(QNX)
* 	epoc		(Epoc)
* 	dummy
*/

#define SDL_VIDEO_CENTERED "SDL_VIDEO_CENTERED"
/* 	If set, tries to center the SDL window when running in X11 windowed
* 	mode, or using the CyberGrafix driver.
*/

#define SDL_VIDEO_GL_DRIVER "SDL_VIDEO_GL_DRIVER"
/* 	The openGL driver (shared library) to use for X11. Default is
* 	libGL.so.1
*/

#define SDL_VIDEO_X11_DGAMOUSE "SDL_VIDEO_X11_DGAMOUSE"
/* 	With XFree86, enables use of DGA mouse if set.
*/

#define SDL_VIDEO_X11_MOUSEACCEL "SDL_VIDEO_X11_MOUSEACCEL"
/* 	For X11, sets the mouse acceleration. The value should be a string
* 	on the form: n/d/t
* 	where n and d are the acceleration numerator/denumerators (so
* 	mouse movement is accelerated by n/d), and t is the threshold
* 	above which acceleration applies (counted as number of pixels
* 	the mouse moves at once).
*/

#define SDL_VIDEO_X11_NODIRECTCOLOR "SDL_VIDEO_X11_NODIRECTCOLOR"
/* 	If set, don't attempt to use DirectColor visuals even if they are
* 	present. (SDL will use them otherwise for gamma correction).
* 	This is needed with older X servers when using the XVideo extension.
*/

#define SDL_VIDEO_X11_VISUALID "SDL_VIDEO_X11_VISUALID"
/* 	ID of an X11 visual to use, overriding SDL's default visual selection
* 	algorithm. It can be in decimal or in hex (prefixed by 0x).
*/

#define SDL_VIDEO_YUV_DIRECT "SDL_VIDEO_YUV_DIRECT"
/* 	If set, display YUV overlay directly on the video surface if possible,
* 	instead of on the surface passed to SDL_CreateYUVOverlay().
*/

#define SDL_VIDEO_YUV_HWACCEL "SDL_VIDEO_YUV_HWACCEL"
/* 	If not set or set to a nonzero value, SDL will attempt to use
* 	hardware YUV acceleration for video playback.
*/

#define SDL_WINDOWID "SDL_WINDOWID"
/* 	For X11 or Win32, contains the ID number of the window to be used by
* 	SDL instead of creating its own window. Either in decimal or
* 	in hex (prefixed by 0x).
*/

typedef struct av_system_sdl_prefs
{
	const char* name;
	const char* envname;
	const char* defvalue;
	const int   set_always;
	char  buf[80];
} av_system_sdl_prefs_t, *av_system_sdl_prefs_p;

static int x11_get_colorkey(void);

static av_system_sdl_prefs_t sdl_prefs[] =
{
	{
		"driver",
		SDL_VIDEODRIVER,
		"x11", /*"x11", "dga", "fbcon", "directfb"*/
		1,
	},
	{
		"yuvhwaccel",
		SDL_VIDEO_YUV_HWACCEL,
		"1",
		1,
	},
	{
		"yuvdirect",
		SDL_VIDEO_YUV_DIRECT,
		"1",
		0,
	},
};

static const int sdl_prefs_count = sizeof(sdl_prefs)/sizeof(sdl_prefs[0]);

static int _is_double_buffer = 0;
static av_pixel_t _color_key = 0x00000821;

av_bool_t is_sdl_double_buffer(void)
{
	return _is_double_buffer;
}

av_pixel_t color_key(void)
{
	return _color_key;
}

void av_system_sdl_setup_param(void)
{
	int i;
	char buf[80];
	av_prefs_p prefs;
	const char* prefstr = "backend.sdl.";

	x11_get_colorkey();

	if (AV_OK == av_torb_service_addref("prefs", (av_service_p*)&prefs))
	{
		const char* val;
		for (i=0; i<sdl_prefs_count; i++)
		{
			val = AV_NULL;
			strcpy(buf, prefstr);
			strcat(buf, sdl_prefs[i].name);
			prefs->get_string(prefs, buf, sdl_prefs[i].defvalue, &val);
			snprintf(sdl_prefs[i].buf, 80, "%s=%s", sdl_prefs[i].envname, val);
			if (sdl_prefs[i].set_always || (0 < atoi(val)))
            {
                printf("sdl_prefs: %s\n", sdl_prefs[i].buf);
				putenv(sdl_prefs[i].buf);
            }
		}

		strcpy(buf, prefstr);
		strcat(buf, "doublebuffer");
		prefs->get_int(prefs, buf, 0, &_is_double_buffer);
		av_torb_service_release("prefs");
	}

	return;
}

/* FIXME: brake X11 dependency */
static int x11_get_colorkey()
{
	int rc = 0;  /* Cannot get color key */
#ifdef WITH_X11
	Display* dpy;
	XvAdaptorInfo *adaptor_info;
	unsigned int n, num_adaptors = 0;
	unsigned int xv_version, xv_release;
	unsigned int xv_request_base, xv_event_base, xv_error_base;

	if(NULL == (dpy = XOpenDisplay(NULL)))
	{
		return 0; /* Couldn't open display */
	}

	if(XvQueryExtension
	(
			dpy,
			&xv_version,
			&xv_release,
			&xv_request_base,
			&xv_event_base,
			&xv_error_base
			) != Success)
	{
		XCloseDisplay(dpy);
		return 0; /* Xv not found */
	}

	if (Success == XvQueryAdaptors(
		dpy,
		DefaultRootWindow(dpy),
		&num_adaptors,
		&adaptor_info))
	{
		for(n = 0; n < num_adaptors; n++)
		{
			int port;
			int base_id;
			int num_ports;

			base_id = adaptor_info[n].base_id;
			num_ports = adaptor_info[n].num_ports;

			for(port = base_id; port < base_id+num_ports; port++)
			{
				int k;
				int num;
				XvAttribute *xvattr;

				xvattr = XvQueryPortAttributes(dpy, port, &num);
				for(k = 0; k < num; k++)
				{
					Atom val_atom;
					int cur_val = 0;
					val_atom = XInternAtom(dpy, xvattr[k].name, False);
					if(xvattr[k].flags & XvGettable)
					{
						XvGetPortAttribute(
							dpy,
							port,
							val_atom,
							&cur_val);
					}

					if (!strcmp(xvattr[k].name, "XV_COLORKEY"))
					{
						_color_key = (unsigned int)cur_val;
						rc = 1; /* success status */
						break;
					}
				}
				if (xvattr)
					XFree(xvattr);
			}
		}
		if (adaptor_info)
			XvFreeAdaptorInfo(adaptor_info);
	}
	XCloseDisplay(dpy);
#endif
	return rc;
}

#endif /*WITH_SYSTEM_SDL*/
