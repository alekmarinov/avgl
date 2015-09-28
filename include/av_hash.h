/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_hash.h                                          */
/*                                                                   */
/* Copyright 2006 David Crawshaw, released under the new BSD license.*/
/* Version 2, from http://www.zentus.com/c/hash.html                 */
/*                                                                   */
/*********************************************************************/

/*! \file av_hash.h
*   \brief av_hash definition
*/

#ifndef __AV_HASH_H
#define __AV_HASH_H

#include <av.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AV_HASH_CAPACITY_SMALL  50
#define AV_HASH_CAPACITY_MEDIUM 5000
#define AV_HASH_CAPACITY_HUGE   500000

/*! 
* \brief hash table
*
* Data structure associating keys with values
*/
typedef struct av_hash
{
	/*! Implementation specific */
	void* context;

	/*!
	* \brief Adds key/value pair
	* \param self is a reference to this object
	* \param key a non empty, zero terminated string
	* \param value is a user pointer
	* \return - AV_OK on success
	*         - AV_EARG on null or empty string given as a key
	*         - AV_EMEM on allocation error
	*/
	av_result_t (*add)   (struct av_hash* self,
						  const char* key,
						  void* value);

	/*!
	* \brief Returns value matching given key
	* \param self is a reference to this object
	* \param key which associated value is requested
	* \return the value associated to the given \c key
	*/
	void* (*get)         (struct av_hash* self,
						  const char* key);

	/*!
	* \brief Removes key from table, returning value
	* \param self is a reference to this object
	* \param key to be removed
	* \return the value associated to the given \c key
	*/
	void *(*remove)      (struct av_hash* self,
						  const char* key
	);

	/*!
	* \brief Returns hash table size
	* \param self is a reference to this object
	* \return hash table size
	*/
	unsigned int (*size) (struct av_hash* self);

	/*!
	* \brief Sets the current item to point the first element in the hash
	* \param self is a reference to this object
	* \return AV_TRUE if there are more elements
	*/
	void (*first)   (struct av_hash* self);

	/*!
	* \brief Gets next key,value pair from hash
	* \param self is a reference to this object
	* \param key output
	* \param value output
	* \return AV_TRUE if the returned key,value pair is valid
	*/
	av_bool_t (*next)    (struct av_hash* self, const char** key, void** value);

	/*!
	* \brief Destroys this hashtable
	* \param self is a reference to this object
	*/
	void (*destroy)      (struct av_hash* self);
} av_hash_t, *av_hash_p;

/*!
* \brief Create new hashtable
* \param capacity is the estimated hash table capacity
* \param pphash is the result hashtable
* \return AV_OK on success
*         AV_EMEM on allocation error
*/
AV_API av_result_t av_hash_create(unsigned int capacity, av_hash_p *pphash);

#ifdef __cplusplus
}
#endif

#endif /* __AV_HASH_H */
