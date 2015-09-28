/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      list.c                                             */
/* Description:   Demonstrate list usage                             */
/*                                                                   */
/*********************************************************************/

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <av_list.h>

const char* items[] = 
{
	"item 1",
	"item 2",
	"item 3"
};

/* entry point */
int main( void )
{
	av_list_p list;
	int i;
	int n;
	if (AV_OK != av_list_create(&list))
	{
		printf("Out of memory error\n"); 
		return 1;
	}

	printf("action:%s \t av_list_p->size=%d\n", "av_list_create",list->size(list));
	for (i=0; i<10; i++)
	{
		/* push some items in different ways */
		list->push_last(list, strdup(items[2]));
		printf("action:%s \t av_list_p->size=%d\n", "list->push_last",list->size(list));
		list->push_first(list, strdup(items[0]));
		printf("action:%s \t av_list_p->size=%d\n", "list->push_first",list->size(list));
		list->last(list);
		list->insert_prev(list, strdup(items[1]));
		printf("action:%s \t av_list_p->size=%d\n", "list->insert_prev",list->size(list));
	}

	n=0;
	/* iterate the list and show the items */
	for (list->first(list); list->has_more(list); list->next(list))
	{
		printf("item #%d is %s\n", n++, (char*)list->get(list));
	}
	
	printf("list contains %d items and list->size(list)=%d\n", n, list->size(list));

	list->iterate_all(list, free, AV_TRUE);
	printf("action:%s \t av_list_p->size=%d\n", "list->iterate_all(list, free, AV_TRUE)",list->size(list));
	list->destroy(list);

	return 0;
}
