/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      media.c                                            */
/* Description:   Demonstrate media usage                            */
/*                                                                   */
/*********************************************************************/

#define _XOPEN_SOURCE 500
#include <unistd.h>     /* sleep/usleep */
#include <errno.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <av_torb.h>
#include <av_thread.h>
#include <av_media.h>
#include <av_log.h>
#include <av_system.h>

#define MAX_AUDIOQ_SIZE (1024)
#define MAX_VIDEOQ_SIZE (1024)
#define MAX_PICTQ_SIZE 1

#define SURFACE_WIDTH 640
#define SURFACE_HEIGHT 480

av_log_p _log = 0;

av_result_t av_media_ffmpeg_register_torba(void);
av_result_t av_scaler_swscale_register_torba(void);

int main(int nargs, char* argv[])
{
	av_result_t rc;
	av_video_p video;
	av_system_p sys;
	av_media_p media;
	av_surface_p surface;
	const char* url;
	av_decoder_video_p video_decoder;
	av_graphics_p graphics;
	av_graphics_surface_p graphics_surface;
	av_window_p root;
	av_scaler_p scaler;
	av_scale_info_t scale_info;
	av_video_info_t video_info;
	int i;

	if (nargs < 2)
	{
		fprintf(stderr, "Usage: media <url>\n");
		return 1;
	}
	url = argv[1];

	if (AV_OK != (rc = av_torb_init()))
		return rc;

	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&_log)))
		return rc;

	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
		return rc;

	/* register media components */
	av_scaler_register_torba();
	av_scaler_swscale_register_torba();
	av_media_register_torba();
	av_media_ffmpeg_register_torba();

	/* create media object */
	if (AV_OK != (rc = av_torb_create_object("media_ffmpeg", (av_object_p*)&media)))
	{
		fprintf(stderr, "Error %d: media_ffmpeg create failed\n", rc);
		return 1;
	}

	/* create scaler object */
	if (AV_OK != (rc = av_torb_create_object("scaler_swscale", (av_object_p*)&scaler)))
	{
		fprintf(stderr, "Error %d: scaler_swscale create failed\n", rc);
		return 1;
	}

	/* open movie file */
	if (AV_OK != (rc = media->open(media, url)))
	{
		fprintf(stderr, "Error %d: unable to open file %s\n", rc, url);
		return 1;
	}

	/* setup video */
	sys->create_window(sys, 0, 0, &root);
	sys->get_video(sys, &video);
	video->get_backbuffer(video, (av_video_surface_p*)&surface);
	sys->get_graphics(sys, &graphics);

	/* setup media */
	media->get_video_decoder(media, &video_decoder);
	media->get_video_info(media, &video_info);

	/* setup scaler */
	scale_info.src_format = video_info.format;
	scale_info.src_width = video_info.width;
	scale_info.src_height = video_info.height;
	scale_info.dst_format = AV_VIDEO_FORMAT_RGB32;
	surface->get_size(surface, &scale_info.dst_width, &scale_info.dst_height);
	scaler->set_configuration(scaler, &scale_info);

	/* setup graphics */
	graphics->wrap_surface(graphics, surface, &graphics_surface);

	for (i=0; i<10000; i++)
	{
		av_frame_video_p frame_video;
		av_packet_p packet = AV_NULL;
		av_rect_t rect;
		av_rect_init(&rect, 0, 0, scale_info.dst_width, scale_info.dst_height);
		if (AV_OK != media->read_packet(media, &packet))
			break;
		if (packet)
		{
			video_decoder->decode(video_decoder, packet, &frame_video);
			if (frame_video)
			{
				av_graphics_surface_p target_surface;

				/* begin graphics  */
				graphics->begin(graphics, graphics_surface);

				/* scale directly on the graphics target surface */
				graphics->get_target_surface(graphics, &target_surface);
				scaler->scale(scaler, frame_video, (av_surface_p)target_surface);

				/* finish graphics */
				graphics->end(graphics);

				/* show backbuffer on screen */
				video->update(video, AV_NULL);

				usleep(1000000);
			}
			packet->destroy(packet);
		}
		if (AV_FALSE == sys->step(sys)) break;
	}

	O_destroy(scaler);
	O_destroy(media);

	av_torb_service_release("log");
	av_torb_service_release("system");

	av_torb_done();
	return rc;
}
