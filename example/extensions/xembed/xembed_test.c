/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      xembed_test.c                                      */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <av.h>
#include <av_xembed.h>
#include <av_graphics.h>

#define xresolution 1024
#define yresolution 768

int main( void )
{
	av_result_t rc;
	av_xembed_p xembed = 0;
	av_xembed_video_t video_info;
	av_surface_p surface;
	av_graphics_p graphics;
	av_graphics_surface_p graphics_surface;
	av_pixel_p vmem;
	int pitch;

	av_torb_init();
	av_xembed_register();
	av_torb_service_addref("xembed", (av_service_p*)&xembed);

	if (AV_OK != (rc = av_torb_create_object("graphics_cairo", (av_object_p*)&graphics)))
	{
		printf("unable to create graphics object\n");
		return 1;
	}

	if (AV_OK != (rc = xembed->open(xembed, xresolution, yresolution)))
	{
		printf("xembed: open failed\n");
		return 1;
	}

	if (AV_OK != (rc = xembed->get_video_info(xembed, &video_info)))
	{
		printf("xembed: get_video_info failed\n");
		return 1;
	}

	printf("video_info: %dx%dx%d %p\n",
		video_info.xres, video_info.yres, video_info.bpp, video_info.vmem);

/*	if (AV_OK != (rc = xembed->create_surface(xembed, &surface)))
	{
		printf("xembed: create_surface failed\n");
		return 1;
	}*/
	if (AV_OK != (rc = xembed->get_video_surface(xembed, (av_video_surface_p*)&surface)))
	{
		printf("xembed: get_video_surface failed\n");
		return 1;
	}

	if (AV_OK != (rc = graphics->wrap_surface(graphics, surface, &graphics_surface)))
	{
		printf("graphics: wrap_surface failed\n");
		return 1;
	}

	graphics->begin(graphics, graphics_surface);
	if (AV_OK != (rc = graphics->save_surface_file(graphics, graphics_surface, "image.png")))
	{
		printf("graphics: save_surface_file failed\n");
		return 1;
	}
	graphics->end(graphics);
	O_destroy(graphics_surface);

	printf("xembed create_surface -> %p:\n", surface);

	if (AV_OK != (rc = surface->lock(surface, AV_SURFACE_LOCK_READ, &vmem, &pitch)))
	{
		printf("surface: lock failed\n");
		return 1;
	}

	printf("xembed lock -> mem = %p, pitch = %d:\n", vmem, pitch);

	if (AV_OK != (rc = surface->unlock(surface)))
	{
		printf("surface: unlock failed\n");
		return 1;
	}

	O_destroy(surface);

	O_destroy(graphics);
	av_torb_service_release("xembed");
	av_torb_done();

	return 0;
}
