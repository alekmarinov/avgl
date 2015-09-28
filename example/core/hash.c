/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      hash.c                                             */
/* Description:   Demonstrate hash usage                             */
/*                                                                   */
/*********************************************************************/

#define _GNU_SOURCE

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <av_hash.h>

const char* items[] = 
{
	"item 1",
	"item 2",
	"item 3"
};

/* entry point */
int main( void )
{
	av_hash_p hash;
	int i;
	int n;
	const char* key;
	void* value;
	if (AV_OK != av_hash_create(AV_HASH_CAPACITY_SMALL, &hash))
	{
		printf("Out of memory error\n"); 
		return 1;
	}

	printf("action:%s \t av_hash_p->size=%d\n", "av_hash_create",hash->size(hash));
	
	for (i=0; i<3; i++)
	{
		/* push some items in different ways */
		hash->add(hash, items[2], strdup(items[2]));
		hash->add(hash, items[0], strdup(items[0]));
		hash->add(hash, items[1], strdup(items[1]));
	}

	n=0;
	/* iterate the hash and show the element pairs */
	hash->first(hash);
	while ( hash->next(hash, &key, &value) )
	{
		printf("key = %s, value = %s\n", key, (char*)value);
		n++;
	}

	printf("hash contains %d items and hash->size(hash)=%d\n", n, hash->size(hash));

	hash->destroy(hash);

	return 0;
}
