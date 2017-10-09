/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_list.c                                          */
/* Description:   Lists, Stacks, Queue                               */
/*                                                                   */
/*********************************************************************/

#include <av_list.h>
#include <av_stdc.h>

/* defines list item */
struct list_item
{
	void *value;
	struct list_item* next;
	struct list_item* prev;
};

/* defines list structure */
struct list
{
	struct list_item* first;
	struct list_item* last;
	struct list_item* current;
	unsigned int size;
};

/*
*	Adds element before the first list element
*
*	\arg user defined value
*	\return AV_OK on success
			AV_EMEM on allocation error
*/
static av_result_t av_list_push_first(av_list_p self, void* value)
{
	struct list_item* item;
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");

	/* initializes new item */
	item = (struct list_item*)av_malloc(sizeof(struct list_item));
	if (!item)
		return AV_EMEM;

	item->value = value;
	item->prev = AV_NULL;

	/* attach the new item before the first list element */
	item->next = ctx->first;
	if (ctx->first)
		ctx->first->prev = item;
	ctx->first = item;
	if (!ctx->last) ctx->last = ctx->first;
	ctx->size++;

	return AV_OK;
}

/*
*	Adds element after the last list element
*
*	\arg user defined value
*	\return AV_OK on success
			AV_EMEM on allocation error
*/
static av_result_t av_list_push_last(av_list_p self, void* value)
{
	struct list_item* item;
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");

	/* initializes new item */
	item = (struct list_item*)av_malloc(sizeof(struct list_item));
	if (!item)
		return AV_EMEM;

	item->value = value;
	item->next = AV_NULL;

	/* attach the new item after the last list element */
	item->prev = ctx->last;
	if (ctx->last)
		ctx->last->next = item;
	ctx->last = item;
	if (!ctx->first) ctx->first = ctx->last;
	ctx->size++;

	return AV_OK;
}

av_result_t av_list_push_all(av_list_p self, av_list_p list)
{
	struct list* list_ctx = (struct list*)list->context;
	struct list_item* item;
	av_assert(list_ctx, "list is not properly initialized");

	item = list_ctx->first;
	while (item)
	{
		self->push_last(self, item->value);
		item = item->next;
	}
}

/*
*	Adds element next to the current element
*
*	\arg user defined value
*	\return AV_OK on success
*			AV_EFOUND current element is not defined
*			AV_EMEM on allocation error
*/
static av_result_t av_list_insert_next(av_list_p self, void* value)
{
	struct list_item* item;
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");

	/* checks if current element is defined */
	if (self->has_more(self))
	{
		/* initializes new item */
		item = (struct list_item*)av_malloc(sizeof(struct list_item));
		if (!item)
			return AV_EMEM;

		item->value = value;
		
		/* attachs the item after the current */
		item->next = ctx->current->next;
		if (item->next)
			item->next->prev = item;
		ctx->current->next = item;
		item->prev = ctx->current;
		ctx->size++;
	}
	else
	{
		/* current element is not defined */
		return AV_EFOUND;
	}

	return AV_OK;
}

/*
*	Adds element before the current element
*
*	\arg user defined value
*	\return AV_OK on success
*			AV_EFOUND current element is not defined
*			AV_EMEM on allocation error
*/
static av_result_t av_list_insert_prev(av_list_p self, void* value)
{
	struct list_item* item;
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");

	/* checks if current element is defined */
	if (self->has_more(self))
	{
		/* initializes new item */
		item = (struct list_item*)av_malloc(sizeof(struct list_item));
		if (!item)
			return AV_EMEM;

		item->value = value;
		
		/* attachs the item before the current */
		item->prev = ctx->current->prev;
		if (item->prev)
			item->prev->next = item;
		ctx->current->prev = item;
		item->next = ctx->current;
		ctx->size++;
	}
	else
	{
		/* current element is not defined */
		return AV_EFOUND;
	}

	return AV_OK;
}

/*
*	Removes the first element from the list and returns its associated value.
*	The returned value is valid until list is not empty (size > 0),
*	otherwise the returned value is AV_NULL.
*	The \b current element is set to point the first list element.
*/
static void* av_list_pop_first(av_list_p self)
{
	self->first(self);
	return self->remove(self);
}

/*
*	Removes the last element from the list and returns its associated value.
*	The returned value is valid until list is not empty (size > 0),
*	otherwise the returned value is AV_NULL.
*	The \b current element is set to point the last list element.
*/
static void* av_list_pop_last(av_list_p self)
{
	self->last(self);
	return self->remove(self);
}

/*
*	Returns value positioned at the current list position.
*	The returned value is valid if \c has_more
*	returned AV_TRUE before calling \c get
*/
static void* av_list_get(av_list_p self)
{
	void* value;
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");
	
	value = (ctx->current?ctx->current->value:AV_NULL);

	return value;
}

/*
*	Removes current item and returns its associated value.
*	An item is removed and the returned value is valid
*	if \c has_more returned AV_TRUE before calling \c remove.
*	The new \c current item is shifted to the next list item.
*/
static void* av_list_remove(av_list_p self)
{
	void* value = AV_NULL;
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");
	
	if (self->has_more(self))
	{
		struct list_item* item;
		item = ctx->current;
		value = item->value;

		if (item == ctx->first)
		{
			if (ctx->first == ctx->last)
			{
				ctx->current = ctx->first = ctx->last = AV_NULL;
			}
			else
			{
				ctx->first = ctx->current->next;
			}
		}
		else
		if (ctx->current == ctx->last)
		{
			ctx->last = ctx->current->prev;
		}

		if (ctx->current)
		{
			if (ctx->current->prev)
				ctx->current->prev->next = ctx->current->next;
			if (ctx->current->next)
				ctx->current->next->prev = ctx->current->prev;
			ctx->current = ctx->current->next;
		}
		free(item);
		ctx->size--;
	}

	av_assert((ctx->size != 0) || ((ctx->first == ctx->last) && (AV_NULL == ctx->first)), "invalid list state");

	return value;
}

/*
*	Removes all list items.
*/
static void av_list_remove_all(av_list_p self, void (*iterator)(void*))
{
	self->first(self);
	while (self->size(self)>0)
	{
		void* value = self->remove(self);
		if (iterator)
			iterator(value);
	}
}

/*
*	\returns AV_TRUE if the current element is valid, AV_FALSE otherwise
*/
static av_bool_t av_list_has_more(av_list_p self)
{
	av_bool_t res;
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");

	res = (ctx->current)?AV_TRUE:AV_FALSE;
	return res;
}

/*
*	Moves the current list item to the next list element.
*	
*	\returns AV_TRUE if current has been defined prior
*			 calling \c next, otherwise returns AV_FALSE
*/
static av_bool_t av_list_next(av_list_p self)
{
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");

	if (self->has_more(self))
	{
		av_assert(ctx->current, "has_more returned TRUE but current element is not defined");

		ctx->current = ctx->current->next;

		return AV_TRUE;
	}
	return AV_FALSE;
}

/*
*	Moves the current list item to the previous list element.
*	
*	\returns AV_TRUE if current has been defined prior
*			 calling \c next, otherwise returns AV_FALSE
*/
static av_bool_t av_list_prev(av_list_p self)
{
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");

	if (self->has_more(self))
	{
		av_assert(ctx->current, "has_more returned TRUE but current element is not defined");

		ctx->current = ctx->current->prev;
		return AV_TRUE;
	}
	return AV_FALSE;
}

/*
*	Checks if there is next list item available according to the current
*	
*	\returns AV_TRUE if current has been defined and there is element next to it
*			 AV_FALSE otherwise. Use \c has_more method for ensuring valid result
*/
static av_bool_t av_list_has_next(av_list_p self)
{
	av_bool_t res = AV_FALSE;
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");

	if (self->has_more(self))
	{
		res = (ctx->current && ctx->current->next)?AV_TRUE:AV_FALSE;
	}

	return res;
}

/*
*	Checks if there is a previous list item available according to the current
*	
*	\returns AV_TRUE if current has been defined and there is element before it
*			 AV_FALSE otherwise. Use \c has_more method for ensuring valid result
*/
static av_bool_t av_list_has_prev(av_list_p self)
{
	av_bool_t res = AV_FALSE;
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");

	if (self->has_more(self))
	{
		res = (ctx->current && ctx->current->prev)?AV_TRUE:AV_FALSE;
	}

	return res;
}

/*
*	Sets the current item to point the first list element
*/
static void av_list_first(av_list_p self)
{
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");
	ctx->current = ctx->first;
}

/*
*	Sets the current item to point the last list element
*/
static void av_list_last(av_list_p self)
{
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");
	ctx->current = ctx->last;
}

static av_result_t av_list_copy(av_list_p self, av_list_p source)
{
	if (source)
	{
		for (source->first(source); source->has_more(source); source->next(source))
		{
			self->push_last(self, source->get(source));
		}
	}
	return AV_OK;
}

/*
*	Iterates list elements starting from the first until the iterator callback 
*	returns AV_TRUE. The callback iterator receives the given custom parameter 
*	as a first argument, and the itereated item value as a second.
*
*	\return the last returned value from the iterator callback
*/
static av_bool_t av_list_iterate(av_list_p self, av_list_iterator_t iterator, void* param)
{
	struct list* ctx = (struct list*)self->context;
	struct list_item* item;
	av_bool_t res = AV_FALSE;
	av_assert(ctx, "list is not properly initialized");

	item = ctx->first;
	while (item)
	{
		if (AV_TRUE == (res = iterator(param, item->value)))
			break;
		item = item->next;
	}

	return res;
}

/*
*	Iterates all list elements from the first to the last with passing
*	the itereated item value as an argument to the iterator callback 
*	
*	A convenient way to free all list values previously malloc'd is
*	by using the syntax list->iterate_all(list, free) or
*	destroying all values being av_object_p by using the syntax
*	list->iterate_all(list, destructor)
*/
static void av_list_iterate_all(av_list_p self, void (*iterator)(void*), av_bool_t direction)
{
	struct list* ctx = (struct list*)self->context;
	struct list_item* item;
	av_assert(ctx, "list is not properly initialized");

	item = (direction)?ctx->first:ctx->last;
	while (item)
	{
		iterator(item->value);
		item = (direction)?item->next:item->prev;
	}
}

/*
*	Returns the number of elements stored in the list
*/
static unsigned int av_list_size(av_list_p self)
{
	int size;
	struct list* ctx = (struct list*)self->context;
	av_assert(ctx, "list is not properly initialized");
	size = ctx->size;
	return size;
}

/*! 
*	List destructor 
*/
static void av_list_destroy(av_list_p self)
{
	struct list* ctx = (struct list*)self->context;
	struct list_item* item;
	struct list_item* pitem = AV_NULL;
	av_assert(ctx, "list is not properly initialized");
	item = ctx->first;
	while (item)
	{
		pitem = item;
		item = item->next;
		free(pitem);
	}
	free(self->context);
	free(self);
}

av_result_t av_list_create(av_list_p* pplist)
{
	av_list_p self;
	struct list* ctx;

	self = (av_list_p)av_malloc(sizeof(av_list_t));
	if (!self) return AV_EMEM;

	ctx = (struct list*)av_malloc(sizeof(struct list));
	if (!ctx)
	{
		free(self);
		return AV_EMEM;
	}

	ctx->first = ctx->last = ctx->current = AV_NULL;
	ctx->size = 0;

	self->context     = ctx;
	self->push_last   = av_list_push_last;
	self->push_all    = av_list_push_all;
	self->push_first  = av_list_push_first;
	self->insert_next = av_list_insert_next;
	self->insert_prev = av_list_insert_prev;
	self->pop_last    = av_list_pop_last;
	self->pop_first   = av_list_pop_first;
	self->get         = av_list_get;
	self->remove      = av_list_remove;
	self->remove_all  = av_list_remove_all;
	self->next        = av_list_next;
	self->has_next    = av_list_has_next;
	self->prev        = av_list_prev;
	self->has_prev    = av_list_has_prev;
	self->has_more    = av_list_has_more;
	self->first       = av_list_first;
	self->last        = av_list_last;
	self->copy        = av_list_copy;
	self->iterate     = av_list_iterate;
	self->iterate_all = av_list_iterate_all;
	self->size        = av_list_size;
	self->destroy     = av_list_destroy;
	*pplist = self;
	return AV_OK;
}
