/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      prefs.c                                            */
/* Description:   Demonstrate Preferences usage                      */
/*                                                                   */
/*********************************************************************/

#include <string.h>
#include <av_prefs.h>

static void prefs_observer(void* param, const char* key, av_prefs_value_p value)
{
	AV_UNUSED(param);
	if (AV_PREFS_VALUE_NUMBER == value->value_type)
		printf("%s changed to %d\n", key, value->nvalue);
	else
		printf("%s changed to `%s'\n", key, value->svalue);
}

/* entry point */
int main( void )
{
	/* Initialize TORBA */
	if (AV_OK == av_torb_init())
	{
		av_prefs_p prefs;

		/* Get reference to service log */
		if (AV_OK == av_torb_service_addref("prefs", (av_service_p*)&prefs))
		{
			/* Use preferences object */
			int n;
			const char* s;
			FILE* file;

			prefs->register_observer(prefs, "a.b.e", prefs_observer, AV_NULL);

			prefs->set_int(prefs, "a.b.c", 14);
			prefs->get_int(prefs, "a.b.c", 0, &n);
			av_assert(14 == n, "test failed");
			prefs->set_string(prefs, "a.b.d", "pesho");
			prefs->get_string(prefs, "a.b.d", AV_NULL, &s);
			av_assert(0 == strcmp("pesho", s), "test failed");
			prefs->set_string(prefs, "a.b.e", "25");
			prefs->get_int(prefs, "a.b.e", 0, &n);
			av_assert(25 == n, "test failed");
			prefs->set_int(prefs, "a.b.e", 41);
			prefs->get_string(prefs, "a.b.e", AV_NULL, &s);
			av_assert(0 == strcmp("41", s), "test failed");

			file = fopen("test.prefs", "w");
			if (!file)
			{
				printf("can't create file\n");
				return 1;
			}
			fprintf(file, "a.c.a=12\n");
			fprintf(file, "a.c.b=peshka\n");
			fprintf(file, "a.b.e.f=100\n");
			fprintf(file, "# a.c.c=34 <- comment\n");
			fclose(file);
			prefs->load(prefs, "test.prefs");
			//prefs->save(prefs, "test.prefs");

			prefs->get_int(prefs, "a.c.a", 0, &n);
			av_assert(12 == n, "test failed");
			prefs->get_string(prefs, "a.c.b", AV_NULL, &s);
			av_assert(0 == strcmp("peshka", s), "test failed");

			av_torb_service_release("prefs");
		}

		/* Deinitialize TORBA */
		av_torb_done();
	}

	return 0;
}
