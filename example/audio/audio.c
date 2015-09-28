/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      audio.c                                            */
/* Description:   Demonstrate av_audio usage                         */
/*                                                                   */
/*********************************************************************/

#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <av_system.h>
#include <av_audio.h>
#include <av_media.h>
#include <av_log.h>

av_log_p _log = 0;

// header of wav file
typedef struct {
	char rID[4];            // 'RIFF'
	long int rLen;

	char wID[4];            // 'WAVE'

	char fId[4];            // 'fmt '
	long int pcm_header_len;   // varies...
	short int wFormatTag;
	short int nChannels;      // 1,2 for stereo data is (l,r) pairs
	long int nSamplesPerSec;
	long int nAvgBytesPerSec;
	short int nBlockAlign;
	short int nBitsPerSample;
} WAV_HDR;

// header of wav file
typedef struct
{
	char dId[4];            // 'data' or 'fact'
	long int dLen;
	//   unsigned char *data;
} CHUNK_HDR;


int fs_hz;
int num_ch;
int bits_per_sample;
int duration_ms;

double *g_wdata_in;
int g_num_isamp;
long int g_max_isamp;

static int open_wav(char* filename);
static int fill_out_buf(void* o_buf, int size);

static int fill_out_buf(void* o_buf, int size)
{
	int i = 0;
	signed short int* out_buf = (signed short int*)o_buf;
	size /= sizeof(signed short int);

	while(g_num_isamp < g_max_isamp && i < size)
		out_buf[i++] = (signed short int)g_wdata_in[g_num_isamp++];

	return (i*sizeof(signed short int));
}

static int open_wav(char* filename)
{
	int i;
	FILE *fw;
	unsigned int wstat;
	char obuff[80];

	WAV_HDR *wav;
	CHUNK_HDR *chk;
	short int *uptr;
	unsigned char *cptr;
	int sflag;
	long int rmore;

	char *wbuff;
	int wbuff_len;

	// set defaults
	g_wdata_in = NULL;
	g_num_isamp = 0;
	g_max_isamp = 0;

	// allocate wav header
	wav = malloc(sizeof(WAV_HDR));
	chk = malloc(sizeof(CHUNK_HDR));
	if(wav==NULL){ printf("cant new headers\n"); return(-1); }
	if(chk==NULL){ printf("cant new headers\n"); return(-1); }

	/* open wav file */
	fw = fopen(filename,"rb");
	if(fw==NULL){ printf("cant open wav file\n"); return(-1); }

	/* read riff/wav header */
	wstat = fread((void *)wav,sizeof(WAV_HDR),(size_t)1,fw);
	if(wstat!=1){ printf("cant read wav\n"); return(-1); }

	// check format of header
	for(i=0;i<4;i++) obuff[i] = wav->rID[i];
	obuff[4] = 0;
	if(strcmp(obuff,"RIFF")!=0){ printf("bad RIFF format\n"); return(-1); }

	for(i=0;i<4;i++) obuff[i] = wav->wID[i];
	obuff[4] = 0;
	if(strcmp(obuff,"WAVE")!=0){ printf("bad WAVE format\n"); return(-1); }

	for(i=0;i<3;i++) obuff[i] = wav->fId[i];
	obuff[3] = 0;
	if(strcmp(obuff,"fmt")!=0){ printf("bad fmt format\n"); return(-1); }

	if(wav->wFormatTag!=1){ printf("bad wav wFormatTag\n"); return(-1); }

	if( (wav->nBitsPerSample != 16) && (wav->nBitsPerSample != 8) ){
		printf("bad wav nBitsPerSample\n"); return(-1); }


	// skip over any remaining portion of wav header
	rmore = wav->pcm_header_len - (sizeof(WAV_HDR) - 20);
	wstat = fseek(fw,rmore,SEEK_CUR);
	if(wstat!=0){ printf("cant seek\n"); return(-1); }


	// read chunks until a 'data' chunk is found
	sflag = 1;
	while(sflag!=0){

		// check attempts
		if(sflag>10){ printf("too many chunks\n"); return(-1); }

		// read chunk header
		wstat = fread((void *)chk,sizeof(CHUNK_HDR),(size_t)1,fw);
		if(wstat!=1){ printf("cant read chunk\n"); return(-1); }

		// check chunk type
		for(i=0;i<4;i++) obuff[i] = chk->dId[i];
		obuff[4] = 0;
		if(strcmp(obuff,"data")==0) break;

		// skip over chunk
		sflag++;
		wstat = fseek(fw,chk->dLen,SEEK_CUR);
		if(wstat!=0){ printf("cant seek\n"); return(-1); }
	}

	/* find length of remaining data */
	wbuff_len = chk->dLen;

	// find number of samples
	g_max_isamp = chk->dLen;
	g_max_isamp /= wav->nBitsPerSample / 8;

	/* allocate new buffers */
	wbuff = malloc(wbuff_len);
	if(wbuff==NULL){ printf("cant alloc\n"); return(-1); }

	g_wdata_in = malloc(sizeof(double) * g_max_isamp);
	if(g_wdata_in==NULL){ printf("cant alloc\n"); return(-1); }


	/* read signal data */
	wstat = fread((void *)wbuff,wbuff_len,(size_t)1,fw);
	if(wstat!=1){ printf("cant read wbuff\n"); return(-1); }

	// convert data
	if(wav->nBitsPerSample == 16){
		uptr = (short *) wbuff;
		for(i=0;i<g_max_isamp;i++) g_wdata_in[i] = (double) (uptr[i]);
	}
	else{
		cptr = (unsigned char *) wbuff;
		for(i=0;i<g_max_isamp;i++) g_wdata_in[i] = (double) (cptr[i]);
	}

	// save demographics
	fs_hz = wav->nSamplesPerSec;
	bits_per_sample = wav->nBitsPerSample;
	num_ch = wav->nChannels;

	printf("\nLoaded WAV File: %s\n",filename);
	printf(" Sample Rate = %d (Hz)\n",fs_hz);
	printf(" Number of Samples = %ld\n",g_max_isamp);
	printf(" Bits Per Sample = %d\n",bits_per_sample);
	printf(" Number of Channels = %d\n\n",num_ch);
	duration_ms = 1000.*(double)g_max_isamp/(double)(fs_hz*num_ch);
	printf(" Duration = %f\n\n",(double)duration_ms/1000.);


	// reset buffer stream index
	g_num_isamp = 0;

	// be polite - clean up
	if(wbuff!=NULL) free(wbuff);
	if(wav!=NULL) free(wav);
	if(chk!=NULL) free(chk);
	fclose(fw);

	return 0;
}

int main(int nargs, char* argv[])
{
	int len;
	av_result_t rc = 0;
	av_system_p sys;
	av_timer_p timer;
	av_audio_p audio;
	av_frame_audio_t frame_audio;
	struct av_audio_handle* pahandle;

	if (nargs < 2)
	{
		fprintf(stderr, "Usage: %s <sound_file.wav>\n",argv[0]);
		return 1;
	}

	if (open_wav(argv[1]))
	{
		return 1;
	}

	if (AV_OK != (rc = av_torb_init()))
		return rc;

	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&_log)))
		return rc;

	/* get reference to system service */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&sys)))
	{
		av_torb_service_release("log");
		av_torb_done();
		return rc;
	}

	if (AV_OK != (rc = sys->get_timer(sys, &timer)))
	{
		goto err_exit;
	}

	if (AV_OK != (rc = sys->get_audio(sys, &audio)))
	{
		goto err_exit;
	}

	if (AV_OK != (rc = audio->open(audio, fs_hz, num_ch, bits_per_sample, &pahandle)))
	{
		goto err_exit;
	}

	while(0 < (len = fill_out_buf(&frame_audio.data,sizeof(frame_audio.data))))
	{
		frame_audio.size = len;
		audio->write(audio, pahandle, &frame_audio);
	}

	audio->play(audio, pahandle);
	timer->sleep_ms(duration_ms);

	audio->close(audio, pahandle);

err_exit:
	av_torb_service_release("system");
	av_torb_service_release("log");
	av_torb_done();
	return rc;
}
