/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_sound.c                                         */
/* Description:   Abstract sound class                               */
/*                                                                   */
/*********************************************************************/

#include <av_torb.h>
#include <av_sound.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <mem.h> --> alek: is this necessary? how provide it? */

/* Export prototypes */
av_bool_t av_sound_format_open_wave(const char* filename, struct av_sound_handle** pphandle);
void av_sound_format_close_wave(struct av_sound_handle* phandle);
av_bool_t av_sound_format_setformat_wave(int format, struct av_sound_handle* phandle, void* psamples, int size, int* psize);
av_result_t av_sound_format_getinfo_wave(struct av_sound_handle* phandle, av_sound_info_p pinfo);

/* header of wav file */
typedef struct {
	char rID[4];               /* 'RIFF' */
	long int rLen;
	char wID[4];               /* 'WAVE' */
	char fId[4];               /* 'fmt ' */
	long int pcm_header_len;   /* varies... */
	short int wFormatTag;
	short int nChannels;       /* 1,2 for stereo data is (l,r) pairs */
	long int nSamplesPerSec;
	long int nAvgBytesPerSec;
	short int nBlockAlign;
	short int nBitsPerSample;
} WAV_HDR;

/* header of wav file */
typedef struct
{
	char dId[4];               /* 'data' or 'fact' */
	long int dLen;
} CHUNK_HDR;

typedef struct av_sound_handle_wave
{
	av_sound_info_t info;
	int g_num_isamp;
	double *g_wdata_in;
	long int g_max_isamp;
} av_sound_handle_wave_t, *av_sound_handle_wave_p;

av_bool_t av_sound_format_open_wave(const char* filename, struct av_sound_handle** pphandle)
{
	int i;
	FILE *fw;
	unsigned int wstat;
	char obuff[80];

	WAV_HDR wav;
	CHUNK_HDR chk;
	short int *uptr;
	unsigned char *cptr;
	int sflag;
	long int rmore;

	int wbuff_len;
	char *wbuff = NULL;

	av_bool_t rc = AV_TRUE;
	av_sound_handle_wave_t hw;

	/** set defaults */
	hw.g_wdata_in = NULL;
	hw.g_num_isamp = 0;
	hw.g_max_isamp = 0;

	/** open wav file */
	if (NULL == (fw = fopen(filename,"rb")))
	{
		/* cant open wav file */
		rc = AV_FALSE;
		goto exit_err_0;
	}

	/** read riff/wav header */
	wstat = fread((void *)&wav,sizeof(WAV_HDR),(size_t)1,fw);
	if (wstat!=1)
	{
		/* cant read wav */
		rc = AV_FALSE;
		goto exit_err_1;
	}

	/** check format of header */
	for(i=0;i<4;i++) obuff[i] = wav.rID[i];
	obuff[4] = 0;
	if (strcmp(obuff,"RIFF")!=0)
	{
		/* bad RIFF format" */
		rc = AV_FALSE;
		goto exit_err_1;
	}

	for(i=0;i<4;i++) obuff[i] = wav.wID[i];
	obuff[4] = 0;
	if (strcmp(obuff,"WAVE")!=0)
	{
		/* bad WAVE format */
		rc = AV_FALSE;
		goto exit_err_1;
	}

	for(i=0;i<3;i++) obuff[i] = wav.fId[i];
	obuff[3] = 0;
	if (strcmp(obuff,"fmt")!=0)
	{
		/* bad fmt format */
		rc = AV_FALSE;
		goto exit_err_1;
	}

	/*
	if (wav.wFormatTag != 1)
	{
		rc = AV_FALSE;
		goto exit_err_1;
	}
*/

	if ((wav.nBitsPerSample != 16) && (wav.nBitsPerSample != 8) )
	{
		/* bad wav nBitsPerSample */
		rc = AV_FALSE;
		goto exit_err_1;
	}

	/** skip over any remaining portion of wav header */
	rmore = wav.pcm_header_len - (sizeof(WAV_HDR) - 20);
	wstat = fseek(fw,rmore,SEEK_CUR);
	if (wstat!=0)
	{
		/* cant seek */
		rc = AV_FALSE;
		goto exit_err_1;
	}

	/** read chunks until a 'data' chunk is found */
	sflag = 1;
	while (sflag!=0)
	{
		/* check attempts */
		if(sflag>10)
		{
			/* too many chunks */
			rc = AV_FALSE;
			goto exit_err_1;
		}

		/** read chunk header */
		wstat = fread((void *)&chk,sizeof(CHUNK_HDR),(size_t)1,fw);
		if (wstat!=1)
		{
			/* cant read chunk */
			rc = AV_FALSE;
			goto exit_err_1;
		}

		/* check chunk type */
		for(i=0;i<4;i++) obuff[i] = chk.dId[i];
		obuff[4] = 0;
		if (strcmp(obuff,"data")==0)
			break;

		/* skip over chunk */
		sflag++;
		wstat = fseek(fw,chk.dLen,SEEK_CUR);
		if (wstat!=0)
		{
			/* cant seek */
			rc = AV_FALSE;
			goto exit_err_1;
		}
	}

	/** find length of remaining data */
	wbuff_len = chk.dLen;

	/** find number of samples */
	hw.g_max_isamp = chk.dLen;
	hw.g_max_isamp /= wav.nBitsPerSample / 8;

	/** allocate new buffers */
	wbuff = malloc(wbuff_len);
	if (!wbuff)
	{
		/* cant alloc */
		rc = AV_FALSE;
		goto exit_err_1;
	}

	hw.g_wdata_in = malloc(sizeof(double) * hw.g_max_isamp);
	if (hw.g_wdata_in==NULL)
	{
		/* cant alloc */
		rc = AV_FALSE;
		goto exit_err_2;
	}

	/** read signal data */
	wstat = fread((void *)wbuff,wbuff_len,(size_t)1,fw);
	if (wstat!=1)
	{
		/* cant read wbuff */
		rc = AV_FALSE;
		free(hw.g_wdata_in);
		goto exit_err_2;
	}

	/** convert data */
	if(wav.nBitsPerSample == 16)
	{
		uptr = (short *) wbuff;
		for(i=0;i<hw.g_max_isamp;i++) hw.g_wdata_in[i] = (double) (uptr[i]);
	}
	else
	{
		cptr = (unsigned char *) wbuff;
		for(i=0;i<hw.g_max_isamp;i++) hw.g_wdata_in[i] = (double) (cptr[i]);
	}

	/** reset buffer stream index */
	hw.g_num_isamp = 0;

	/** save demographics */
	hw.info.samplerate = wav.nSamplesPerSec;
	hw.info.channels = wav.nChannels;
	hw.info.duration_ms = 1000.*(double)hw.g_max_isamp/(double)(hw.info.samplerate*hw.info.channels);
	switch(wav.nBitsPerSample)
	{
		default: /* FIXME */
		case 16 : hw.info.format = AV_AUDIO_SIGNED_16; break;
		case 8 : hw.info.format = AV_AUDIO_SIGNED_8; break;
	}

	#if 0
	printf("\nLoaded WAV File: %s\n",filename);
	printf(" Sample Rate = %d (Hz)\n",hw.info.samplerate);
	printf(" Number of Samples = %ld\n",hw.g_max_isamp);
	printf(" Bits Per Sample = %d\n",hw.info.format);
	printf(" Number of Channels = %d\n\n",hw.info.channels);
	printf(" Duration = %f\n\n",(double)hw.info.duration_ms/1000.);
	#endif

	*pphandle = (struct av_sound_handle*)malloc(sizeof(hw));
	if (!*pphandle)
	{
		/* cant read wbuff */
		rc = AV_FALSE;
		free(hw.g_wdata_in);
		goto exit_err_2;
	}

	memcpy(*pphandle, &hw, sizeof(hw));

exit_err_2:
	free(wbuff);
exit_err_1:
	fclose(fw);
exit_err_0:

	return rc;
}

void av_sound_format_close_wave(struct av_sound_handle* phandle)
{
	av_sound_handle_wave_p hw = (av_sound_handle_wave_p)phandle;
	if (hw)
	{
		if (hw->g_wdata_in)
			free(hw->g_wdata_in);
		free(hw);
	}
}

av_bool_t av_sound_format_setformat_wave(int format, struct av_sound_handle* phandle, void* psamples, int size, int* psize)
{
	av_sound_handle_wave_p hw = (av_sound_handle_wave_p)phandle;
	if (hw)
	{
		switch(format)
		{
			case AV_AUDIO_SIGNED_16:
				{
					int i = 0;
					signed short int* out_buf = (signed short int*)psamples;
					size /= sizeof(signed short int);
					while(hw->g_num_isamp < hw->g_max_isamp && i < size)
					{
						out_buf[i++] = (signed short int)hw->g_wdata_in[hw->g_num_isamp++];
					}
					*psize = (i*sizeof(signed short int));
					if (hw->g_num_isamp == hw->g_max_isamp)
					{
						/* reset counter */
						hw->g_num_isamp = 0;
						return AV_FALSE;
					}
				}
				break;
			case AV_AUDIO_SIGNED_8:
				{
					int i = 0;
					char* out_buf = (char*)psamples;
					size /= sizeof(char);
					while(hw->g_num_isamp < hw->g_max_isamp && i < size)
					{
						out_buf[i++] = (char)hw->g_wdata_in[hw->g_num_isamp++];
					}
					*psize = (i*sizeof(char));
					if (hw->g_num_isamp == hw->g_max_isamp)
					{
						/* reset counter */
						hw->g_num_isamp = 0;
						return AV_FALSE;
					}
				}
				break;
			default:
				{
					/* silence */
					*psize = size;
					memset(psamples,0,size);
				}
				return AV_FALSE;
		}
	}
	return AV_TRUE;
}

av_result_t av_sound_format_getinfo_wave(struct av_sound_handle* phandle, av_sound_info_p pinfo)
{
	av_sound_handle_wave_p hw = (av_sound_handle_wave_p)phandle;
	if (hw)
	{
		pinfo->format = hw->info.format;
		pinfo->channels = hw->info.channels;
		pinfo->samplerate = hw->info.samplerate;
		pinfo->duration_ms = hw->info.duration_ms;
	}
	return AV_OK;
}
