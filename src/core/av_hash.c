/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_hash.c                                          */
/* Description:   Hash Table                                         */
/*                                                                   */
/* Copyright 2006 David Crawshaw, released under the new BSD license.*/
/* Version 2, from http://www.zentus.com/c/hash.html                 */
/*                                                                   */
/*********************************************************************/

#include <math.h>
#include <av_hash.h>
#include <av_thread.h>
#include <av_stdc.h>

/* Table is sized by primes to minimise clustering.
   See: http://planetmath.org/encyclopedia/GoodHashTablePrimes.html */
static const unsigned int sizes[] = 
{
	53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
	196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
	50331653, 100663319, 201326611, 402653189, 805306457, 1610612741
};

static const unsigned int sizes_count = sizeof(sizes) / sizeof(sizes[0]);
static const float load_factor = 0.65f;

struct record 
{
	unsigned int hash;
	char *key;
	void *value;
};

struct hash 
{
	struct record *records;
	unsigned int records_count;
	unsigned int size_index;
	unsigned int current_record;
	av_rwmutex_p mutex;
};

static av_result_t hash_grow(av_hash_p self)
{
	unsigned int i;
	struct record *old_recs;
	unsigned int old_recs_length;
	struct hash *h = (struct hash *)self->context;
	
	av_assert(h, "hash is not properly initialized");
	
	old_recs_length = sizes[h->size_index];
	old_recs = h->records;
	
	if (h->size_index == sizes_count - 1)
	{
		/* too big hash table, the most probably caused by an application bug */
		return AV_EMEM; 
	}
	
	if (0 == (h->records = calloc(sizes[++h->size_index], sizeof(struct record))))
	{
		h->records = old_recs;
		return AV_EMEM;
	}
	
	h->records_count = 0;
	
	/* rehash table */
	for (i=0; i < old_recs_length; i++)
	{
		if (old_recs[i].hash && old_recs[i].key)
		{
			self->add(self, old_recs[i].key, old_recs[i].value);
			free(old_recs[i].key);
		}
	}
	
	free(old_recs);
	
	return AV_OK;
}

/* algorithm djb2 */
static unsigned int strhash(const char *str)
{
	int c;
	int hash = 5381;
	while ((c = *str++))
		hash = hash * 33 + c;
	return hash == 0 ? 1 : hash;
}

static av_result_t av_hash_add(av_hash_p self, const char* key, void* value)
{
	struct hash *h = (struct hash *)self->context;
	struct record *recs;
	int rc;
	unsigned int off, ind, size, code;
	
	av_assert(h, "hash is not properly initialized");
	
	if ((0 == key) || (0 == *key))
		return AV_EARG;
	
	h->mutex->lock_write(h->mutex);
	if (h->records_count > sizes[h->size_index] * load_factor)
	{
		if (AV_OK != (rc = hash_grow(self)))
		{
			h->mutex->unlock(h->mutex);
			return rc;
		}
	}

	code = strhash(key);
	recs = h->records;
	size = sizes[h->size_index];
	
	ind = code % size;
	off = 0;
	
	while (recs[ind].key)
	{
		ind = (code + (int)pow(++off,2)) % size;
	}
	
	recs[ind].hash  = code;
	recs[ind].key   = av_strdup(key);
	recs[ind].value = value;
	
	h->records_count++;
	h->mutex->unlock(h->mutex);

	return AV_OK;
}

static void* av_hash_get(av_hash_p self, const char* key)
{
	struct hash *h = (struct hash *)self->context;
	struct record *recs;
	void * value = AV_NULL;
	unsigned int off, ind, size;
	unsigned int code = strhash(key);

	av_assert(h, "hash is not properly initialized");

	h->mutex->lock_read(h->mutex);
	recs = h->records;
	size = sizes[h->size_index];
	ind  = code % size;
	off  = 0;
	/* 
		search on hash which remains even if a record has been removed,
		so hash_remove() does not need to move any collision records
	*/
	while (recs[ind].hash) 
	{
		if ((code == recs[ind].hash) && recs[ind].key && 0 == av_strcmp(key, recs[ind].key))
		{
			value = recs[ind].value;
			break;
		}
		ind = (code + (int)pow(++off,2)) % size;
	}
	h->mutex->unlock(h->mutex);

	return value;
}

static void* av_hash_remove(av_hash_p self, const char* key)
{
	struct hash *h = (struct hash *)self->context;
	unsigned int code = strhash(key);
	struct record *recs;
	void * value = AV_NULL;
	unsigned int off, ind, size;

	av_assert(h, "hash is not properly initialized");

	h->mutex->lock_write(h->mutex);
	recs = h->records;
	size = sizes[h->size_index];
	ind = code % size;
	off = 0;
	while (recs[ind].hash)
	{
		if ((code == recs[ind].hash) && recs[ind].key && 0 == av_strcmp(key, recs[ind].key))
		{
			/* do not erase hash, so probes for collisions succeed */
			value = recs[ind].value;
			free(recs[ind].key);
			recs[ind].key = 0;
			recs[ind].value = 0;
			h->records_count--;
			break;
		}
		ind = (code + (int)pow(++off, 2)) % size;
	}
	h->mutex->unlock(h->mutex);

	return value;
}

static unsigned int av_hash_size(av_hash_p self)
{
	struct hash *h = (struct hash *)self->context;
	unsigned int size;
	av_assert(h, "hash is not properly initialized");
	h->mutex->lock_read(h->mutex);
	size = h->records_count;
	h->mutex->unlock(h->mutex);

	return size;
}

static void av_hash_destroy(av_hash_p self)
{
	struct hash *h = (struct hash *)self->context;
	int records_size, i;
	av_assert(h && h->records && h->mutex, "hash is not properly initialized");
	av_assert(AV_OK == h->mutex->trylock_write(h->mutex), "hash is busy during destruction");

	/* free keys */
	records_size = sizes[h->size_index];
	for (i=0; i < records_size; i++)
		if (h->records[i].key)
			free(h->records[i].key);

	free(h->records);
	av_assert(AV_OK == h->mutex->unlock(h->mutex), "unable to unlock the hash mutex");
	h->mutex->destroy(h->mutex);
	free(h);
	free(self);
}

static void av_hash_first(av_hash_p self)
{
	struct hash *h = (struct hash *)self->context;
	h->current_record = 0;
}

static av_bool_t av_hash_next(av_hash_p self, const char** key, void** value)
{
	struct hash *h = (struct hash *)self->context;
	unsigned int records_size = sizes[h->size_index];
	while ((h->current_record < records_size) && (0 == h->records[h->current_record].hash))
	{
		h->current_record++;
	}
	if (h->current_record < records_size)
	{
		*key = h->records[h->current_record].key;
		*value = h->records[h->current_record].value;
		h->current_record++;
		return AV_TRUE;
	}
	return AV_FALSE;
}

av_result_t av_hash_create(unsigned int capacity, av_hash_p *pphash)
{
	av_hash_p self;
	struct hash *h;
	unsigned int i, sind=0;
	av_result_t rc;
	
	if (0 == (self = (av_hash_p)av_malloc(sizeof(av_hash_t))))
	{
		return AV_EMEM;
	}
	
	if (0 == (h = (struct hash *)av_malloc(sizeof(struct hash))))
	{
		free(self);
		return AV_EMEM;
	}
	
	capacity = (unsigned int)floor(capacity / load_factor);
	
	for (i=0; i < sizes_count; i++) 
		if (sizes[i] > capacity) 
		{ 
			sind = i; 
			break;
		}
	
	if (0 == (h->records = calloc(sizes[sind], sizeof(struct record))))
	{
		free(h);
		free(self);
		return AV_EMEM;
	}

	h->records_count = 0;
	h->size_index = sind;

	if (AV_OK != (rc = av_rwmutex_create(&h->mutex)))
	{
		free(h->records);
		free(h);
		free(self);
		return rc;
	}

	self->context = (void *)h;
	self->add     = av_hash_add;
	self->get     = av_hash_get;
	self->remove  = av_hash_remove;
	self->size    = av_hash_size;
	self->first   = av_hash_first;
	self->next    = av_hash_next;
	self->destroy = av_hash_destroy;

	*pphash = self;
	return AV_OK;
}
