/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_media.h                                         */
/*                                                                   */
/*********************************************************************/

#ifndef __AV_MEDIA_H
#define __AV_MEDIA_H

#include <stdint.h>
#include <av_torb.h>
#include <av_audio.h>
#include <av_surface.h>

#ifdef __cplusplus
extern "C" {
#endif

/* no AV sync correction is done if below the AV sync threshold */
#define AV_SYNC_THRESHOLD 10
#define AV_SYNC_THRESHOLD_DOUBLE 0.01
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10000
#define AV_NOSYNC_THRESHOLD_DOUBLE 10.0
/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10
#define SAMPLE_CORRECTION_PERCENT_MAX_DOUBLE 10.
/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB 20
/* min delay to skip picture */
#define AV_SKIP_PICTURE_THRESHOLD 10
/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* 1 second of 48khz 32bit audio */
#define AV_AUDIO_FRAME_SIZE 192000
/* Hardware buffer size for audio output */
#define AV_HW_AUDIO_BUFFER_SIZE 1024

typedef union av_fourcc
{
	char fc[4];
	unsigned int fcc;
} av_fourcc_t, *av_fourcc_p;

typedef enum
{
	AV_VIDEO_FORMAT_UNKNOWN,
	AV_VIDEO_FORMAT_RGB24,
	AV_VIDEO_FORMAT_RGB32,
	AV_VIDEO_FORMAT_YUV420P
} av_video_format_t;

/*! \brief audio info
*/
typedef struct av_audio_info
{
	int codec_id;
	int nchannels;
	int freq;
	av_audio_format_t format;
} av_audio_info_t, *av_audio_info_p;

/*! \brief video info
*/
typedef struct av_video_info
{
	int codec_id;
	av_fourcc_t fourcc;
	av_video_format_t format;
	int width;
	int height;
	double fps;
} av_video_info_t, *av_video_info_p;

typedef enum
{
	AV_PACKET_TYPE_UNKNOWN,
	AV_PACKET_TYPE_AUDIO,
	AV_PACKET_TYPE_VIDEO,
	AV_PACKET_TYPE_SUBTITLE
} av_packet_type_t;

typedef struct av_packet
{
	/*! implementation specific */
	void* ctx;

	/*! type */
	av_packet_type_t type;

	/*! data */
	void* data;

	/*! packet data size */
	int size;

	/*! decoding timestamp */
	int64_t dts;

	/*! presentation timestamp */
	int64_t pts;

	/*! DEBUG: counter */
	int counter;

	/*! destructor */
	void (*destroy)(void* self);
} av_packet_t, *av_packet_p;

/*! \brief raw audio frame
*/
typedef struct av_frame_audio
{
	int size;
	int64_t dts;
	int64_t pts;
	unsigned char data[AV_AUDIO_FRAME_SIZE];
} av_frame_audio_t, *av_frame_audio_p;

/*! \brief audio decoder
*/
typedef struct av_decoder_audio
{
	void* ctx;
	av_result_t (*decode)(struct av_decoder_audio* self, av_packet_p packet, av_frame_audio_p* ppaudioframe);
} av_decoder_audio_t, *av_decoder_audio_p;

/*! \brief raw video frame
*/
typedef struct av_frame_video
{
	void* data;
	int* linesize;
	int height;
	int64_t dts;
	int64_t pts;
	int delay_ms;
} av_frame_video_t, *av_frame_video_p;

/*! \brief video decoder
*/
typedef struct av_decoder_video
{
	void* ctx;

	/*!
	* NOTE:
	* If you keep a reference to decoded frame in the time of decoding new frame,
	* the old one will be destroyed/overwritten
	*/
	av_result_t (*decode)(struct av_decoder_video* self, av_packet_p packet, av_frame_video_p* ppvideoframe);
} av_decoder_video_t, *av_decoder_video_p;

/*! \brief media
*/
typedef struct av_media
{
	/*! Parent class object */
	av_object_t object;

	av_result_t (*open)(struct av_media* self, const char* url);
	av_result_t (*seek)(struct av_media* self, double pos_seconds);
	void (*close)(struct av_media* self);
	av_result_t (*get_audio_info)(struct av_media* self, av_audio_info_p audio_info);
	av_result_t (*get_video_info)(struct av_media* self, av_video_info_p video_info);
	av_result_t (*read_packet)(struct av_media* self, av_packet_p* packet);
	av_result_t (*get_audio_decoder)(struct av_media* self, av_decoder_audio_p* ppaudiodecoder);
	av_result_t (*get_video_decoder)(struct av_media* self, av_decoder_video_p* ppvideodecoder);
	av_result_t (*synchronize_video)(struct av_media* self, av_frame_video_p pvideoframe);
	av_result_t (*synchronize_audio)(struct av_media* self, av_frame_audio_p paudioframe);
	int64_t (*clock)(struct av_media* self);
} av_media_t, *av_media_p;

/*! \brief scale_info
*/
typedef struct av_scale_info
{
	av_video_format_t src_format;
	int src_width;
	int src_height;
	av_video_format_t dst_format;
	int dst_width;
	int dst_height;
} av_scale_info_t, *av_scale_info_p;

/*! \brief scaler
*/
typedef struct av_scaler
{
	/*! Parent class object */
	av_object_t object;

	av_result_t (*set_configuration)(struct av_scaler* self, av_scale_info_p scale_info);
	av_result_t (*get_configuration)(struct av_scaler* self, av_scale_info_p scale_info);
	av_result_t (*scale)(struct av_scaler* self, av_frame_video_p frame, av_surface_p surface);
	av_result_t (*scale_raw)(struct av_scaler* self, void* src, void* dst);
} av_scaler_t, *av_scaler_p;

av_result_t av_media_register_torba(void);
av_result_t av_scaler_register_torba(void);

#ifdef __cplusplus
}
#endif

#endif /* __AV_MEDIA_H */
