/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_list.h                                          */
/*                                                                   */
/*********************************************************************/

/*! \file av_list.h
*   \brief av_list definition representing dynamic lists, statcks, queues
*/

#ifndef __AV_LIST_H
#define __AV_LIST_H

#include <av.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! list iterator callback prototype */
typedef av_bool_t (*av_list_iterator_t)(void*, void*);

/*!
* \brief Class list defining two directional linked list
*
* The list class can behave as a simple list, stack or queue
* depending on the way its methods are used.
*
* The list have a \b current element which can point any of the list
* items or AV_NULL meaning it is not defined.
* The current element is accessed via the method \c get.
* The methods \c first and \c last put the current element respectively 
* to the first or the last elements in the list
* The method \c has_more indicates whether the \b current element is defined
* and the result of method \c get is valid (including result value AV_NULL)
* If the \b current element is defined the methods \c next and \c prev
* move the \b current element respectivly to the next or the previous item.
* The return value of \c next and \c prev is AV_TRUE if the \b current
* element is defined, and AV_FALSE if the \b current element was not defined
* at the time of the call. Methods \c has_next and \c has_prev returns
* AV_TRUE if the element respectively next to, or before the current element
* are defined. AV_FALSE otherwise or the \b current element hasn't been
* defined at the time of the call.
*
* There are 4 methods used for adding items to the list:
* \c push_first adds an item before the first list element.
* \c push_last adds an item after the last list element.
* The methods above doesn't depend of the state of the \b current item.
* \c insert_next adds an item after the current list element.
* \c insert_prev adds an item before the current list element.
* Both methods expect the \b current element to be defined, meaning that
* the \c has_more is returning AV_TRUE. Notice that empty list have no
* current element.
* The only reason these 4 methods may fail is out of memory error.
*
* There are 3 methods for removing element from the list:
* \c pop_first removes the first item. If \b current is pointing the first item
* then the \b current is moved automatically to the next available item or goes 
* to undefined state.
* \c pop_last is analogical to \c pop_first except that it behaves in the 
* opposite direction.
* \c remove removes the current element moving the \b current to the next
* list item or set the \b current to undefined state. The method \c remove 
* returns the value associated with the current element which can be considered
* valid only if \c has_more is returning AV_TRUE prior calling the function.
*
*/
typedef struct av_list
{
	/*! Implementation specific */
	void* context;

	/*!
	* \brief Adds element before the first list element
	*
	* \param self is a reference to this object
	* \param value is a user defined value
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*push_first)(struct av_list* self, void* value);

	/*! 
	* \brief Adds element after the last list element
	*
	* \param self is a reference to this object
	* \param value is a user defined value
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*push_last) (struct av_list* self, void* value);

	/*!
	* \brief Adds all elements of a list after the last element of this list
	*
	* \param self is a reference to this object
	* \param another list to add elements from
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*push_all) (struct av_list* self, struct av_list* list);

	/*!
	* \brief Adds element next to the current element
	*
	* \param self is a reference to this object
	* \param value is a user defined value
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EFOUND current element is not defined
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*insert_next)(struct av_list* self, void* value);

	/*!
	* \brief Adds element before the current element
	*
	* \param self is a reference to this object
	* \param value is a user defined value
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EFOUND current element is not defined
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*insert_prev)(struct av_list* self, void* value);

	/*! 
	* \brief Removes the first element from the list and returns its associated value.
	* The returned value is valid until list is not empty (size > 0),
	* otherwise the returned value is AV_NULL.
	* The \b current element is set to point the first list element.
	* \param self is a reference to this object
	* \return the popped value
	*/
	void* (*pop_first)       (struct av_list* self);

	/*!
	* \brief Removes the last element from the list and returns its associated value.
	* The returned value is valid until list is not empty (size > 0),
	* otherwise the returned value is AV_NULL.
	* The \b current element is set to point the last list element.
	* \param self is a reference to this object
	* \return the popped value
	*/
	void* (*pop_last)        (struct av_list* self);

	/*! 
	* \brief Returns value positioned at the current list position.
	* The returned value is valid if \c has_more
	* returned AV_TRUE before calling \c get
	* \param self is a reference to this object
	* \return the current value
	*/
	void* (*get)             (struct av_list* self);

	/*!
	* \brief Removes current item and returns its associated value.
	* An item is removed and the returned value is valid
	* if \c has_more returned AV_TRUE before calling \c remove.
	* The new \c current item is shifted to the next list item.
	* \param self is a reference to this object
	* \return the removed value
	*/
	void* (*remove)          (struct av_list* self);

	/*!
	* \brief Removes all list items. If the iterator arugment is not NULL
	* then each value associated with the removed item is given to that iterator
	* \param self is a reference to this object
	* \param iterator is iterator callback receiving each list value before removed
	*/
	void (*remove_all)       (struct av_list* self, void (*iterator)(void*));

	/*! 
	* \brief Moves the current list item to the next list element.
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_TRUE if current has been defined prior \c next call
	*         - AV_FALSE otherwise
	*/
	av_bool_t (*next)        (struct av_list* self);

	/*! 
	* \brief Moves the current list item to the previous list element
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_TRUE if current has been defined prior \c prev call
	*         - AV_FALSE otherwise
	*/
	av_bool_t (*prev)        (struct av_list* self);

	/*!
	* \brief Checks if there is next list item available according to the current
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_TRUE if current has been defined and there is element next to it
	*         - AV_FALSE otherwise
	* \note Use the method \c has_more while list iteration
	*/
	av_bool_t (*has_next)    (struct av_list* self);

	/*!
	* \brief Checks if there is a previous list item available according to the current
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_TRUE if current has been defined and there is previous element
	*         - AV_FALSE otherwise
	* \note Use the method \c has_more while list iteration
	*/
	av_bool_t (*has_prev)    (struct av_list* self);

	/*!
	* \brief Checks if are more elements during iteration via \c next and \c prev
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_TRUE if more elements are available
	*         - AV_FALSE no more elements
	*/
	av_bool_t (*has_more)    (struct av_list* self);

	/*!
	* \brief Sets the current item to point the first list element
	* \param self is a reference to this object
	*/
	void (*first)            (struct av_list* self);

	/*!
	* \brief Sets the current item to point the last list element
	* \param self is a reference to this object
	*/
	void (*last)             (struct av_list* self);

	/*!
	* \brief Copy all elements from a given source list
	* \param self is a reference to this object
	* \param source is the list to be copied into \c self
	*/
	av_result_t  (*copy)     (struct av_list* self, struct av_list* source);

	/*! 
	* \brief Iterates list elements starting from the first until the iterator callback
	* return AV_TRUE. The callback iterator receives the given custom parameter
	* as a first argument, and the itereated item value as a second
	* \param self is a reference to this object
	* \param iterator is iterator callback receiving each list value during iteration
	* \param param is a custom parameter passed to the iterator
	* \return the last returned value from the iterator callback
	*/
	av_bool_t (*iterate)     (struct av_list* self, av_list_iterator_t iterator, void * param);

	/*! 
	* \brief Iterates all list elements with passing the itereated item value as an argument
	* to the iterator callback. If the direction argument is AV_TRUE the iteration is
	* from the first to the last. If the direction argument is AV_FALSE the iteration
	* is from the last to the first.
	* 
	* A convenient way to free all list values previously malloc'd is
	* by using the syntax list->iterate_all(list, free, AV_TRUE) or
	* destroying all values being av_object_p by using the syntax
	* list->iterate_all(list, destructor, AV_TRUE)
	* \param self is a reference to this object
	* \param iterator is iterator callback receiving each list value during iteration
	* \param direction the direction the list is iterated.
	*                  - AV_TRUE from first to last
	*                  - AV_FALSE from last to first
	*/
	void (*iterate_all)      (struct av_list* self, void (*iterator)(void*), av_bool_t direction);

	/*!
	* \brief Returns the number of elements stored in the list
	* \param self is a reference to this object
	* \return number of list elements
	*/
	unsigned int (*size)     (struct av_list* self);

	/*! 
	* \brief List destructor
	*/
	void (*destroy)          (struct av_list*);
} av_list_t, *av_list_p;

/*!
* \brief Creates new list
* \param pplist result list object
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_list_create(av_list_p* pplist);

#ifdef __cplusplus
}
#endif

#endif /* __AV_LIST_H */
