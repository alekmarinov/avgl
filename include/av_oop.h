/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_oop.h                                           */
/*                                                                   */
/*********************************************************************/

/*! \file av_oop.h
*   \brief FIXME: Update documentation
*
* \page torba TORBA
* \section about_torba About TORBA
* TORB is the core of AVGL library which represents a bag for all defined
* AVGL classes and services. From here comes the name TORBA which read in bulgarian
* sounds like a bag in english. It aims to avoid global objects definitions
* used for shared access. Instead, a class is registered in TORBA under
* string represented name, so from any module an object from this class can be
* constructed by refering that string name.
*
* \subsection torba_class_object Classes and Objects
* All TORBA classes are derived from \c av_object class which is the super class
* in the AVGL class framework. The class \c av_object holds the \c classname
* which TORBA uses to provide Runtime Type Information (RTTI). This is achieved by
* the method 
* \code is_a(object, classname)
* \endcode defined in class \c av_object and thus for the whole AVGL framework.
* The method returns \c AV_TRUE if the object is instance of or derived from
* \c classname. Then it is save to typecast the object to that class named by \c classname.
*
* To register a class into TORBA use the function
* \code av_torb_register_class(classname, parentclassname, classsize, constructor, destructor)
* \endcode where \c classname is a string describing the name of the class, \c parentclassname
* if the class it inherites or AV_NULL defining that \c av_object is the parent.
* \c classsize is an integer defining the number of bytes required to allocate an object 
* of that class. Use sizeof() operator to set this value.
* Each class can be associated with constructor or destructor.
*
* To create an instance from class use the function:
* \code av_torb_create_object(classname, retobject)
* \endcode where \c classname is a string describing the name of the class we want to instantiate.
* And \c retobject is a pointer to \c av_object_p where the new created object will be
* returned.
* This method allocates a block of memory with size corresponding to the \c classsize given on
* class registration and then invokes the constructor sequence from top to bottom following
* the object class inheritance.
*
* To destroy an object use either the function
* \code object->destroy(object)
* \endcode or the shortcut macro O_destroy(object).
* This method destroys the object by invoking the destructor sequence from bottom to top following
* the object class inheritance. The allocated memory for the object is freed by \c av_object destructor.
*
* \subsection torba_service Services
* A service is a class derived from the class \c av_service. Such class is registered as a service
* under specified service name.
* 
* To register a service use the function
* \code av_torb_service_register(servicename, service) \endcode
*
* After a service is registered references to it can be obtained via the function
* \code av_torb_service_addref(servicename, retservice) \endcode
* where \c servicename is a service name and \c retservice is a pointer to \c av_service_p 
* where the service reference will be returned.
*
* After finish using the service reference call the function
* \code av_torb_service_release(servicename) \endcode
*
* When all references to a service are released the object associated with that service is destroyed.
*
*/

#ifndef __AV_OOP_H
#define __AV_OOP_H

#include <av.h>
#include <av_hash.h>
#include <av_list.h>

#ifdef __cplusplus
extern "C" {
#endif

/* forward declarations */
struct _av_oop_t;
struct _av_object_t;

/*!
* \brief Defines type for object constructor assigned to each registered class
* @param self is pointing an allocated object to be constructed by this function
* \return av_result_t
*         - AV_OK on success
*         - != AV_OK on failure
*/
typedef av_result_t(*av_constructor_t)(struct _av_object_t* self);

/*!
* \brief Defines type for object destructor assigned to each registered class
* @param self is pointing an allocated object to be destructed by this function
*/
typedef void(*av_destructor_t)(struct _av_object_t*);

/* class descriptor */
typedef struct _av_class_t
{
	/*!
	* Reference to OOP container
	*/
	struct _av_oop_t* oop;

	/*!
	* Class name
	*/
	char classname[MAX_NAME_SIZE];

	/*!
	* Memory size occuppied by the class
	*/
	int classsize;

	/*!
	* Parent class
	*/
	struct _av_class_t* parent;

	/*!
	* Class constructor
	*/
	av_constructor_t constructor;

	/*!
	* Class destructor
	*/
	av_destructor_t destructor;
} av_class_t, *av_class_p;

/*!
* \brief The super class
*/
typedef struct _av_object_t
{
	/*!
	* object class referrence
	*/
	av_class_p classref;

	/*!
	* Holds object attributes
	*/
	av_hash_p attributes;

	/*!
	* Reference counter
	*/
	unsigned int refcnt;

	/*!
    * \brief Tells if this object is instance of or derived from a specified class
	* \param self is a reference to this object
	* \param classname a string describing TORBA class
	* \return AV_TRUE if \c self is instance of or derived from the class \c classname,
	*         AV_FALSE otherwise
	*/
	av_bool_t (*is_a) (struct _av_object_t* self, const char* classname);

	/*!
    * \brief Gets object string representation into the given buffer without exceeding the specified size
	* \param self is a reference to this object
	* \param buf is a string buffer where the object description will be received
	* \param bufsize is the string buffer size to not be exceeded from the \c tostring method
	* \return a pointer to the string buffer \c buf
	*/
	char* (*tostring)(struct _av_object_t* self, char* buf, int bufsize);
	
	/*!
    * \brief Sets custom attribute to that object
	* \param self is a reference to this object
	* \param name is the attribute name
	* \param value is the attribute value
    */
	av_result_t (*set_attribute)(struct _av_object_t* self, const char* name, void* value);
	
	/*!
    * \brief Gets custom attribute from the object
	* \param self is a reference to this object
	* \param name is the attribute name
	* \return the value associated with the attribute name
    */
	void* (*get_attribute)(struct _av_object_t* self, const char* name);
	
	/*!
    * \brief Dumps attributes in stdout
	* \param self is a reference to this object
	* \param name is the attribute name
	* \return the value associated with the attribute name
    */
	void (*dump_attributes)(struct _av_object_t* self);

	/*! 
    * \brief Default object destructor
	* \param self is a void* pointer to this object
	*/
	void (*destroy)(void* self);

	/*!
	* \brief Increment reference counter
	* \param self is a reference to this object
	*/
	struct _av_object_t* (*addref)(struct _av_object_t* self);

	/*!
	* \brief Decrement reference counter and destroy the object when reaches 0
	* \param self is a reference to this object
	*/
	void(*release)(struct _av_object_t* self);

} av_object_t, *av_object_p;

/*! A shortcut to \c context field of an object */
#define O_attr(o, name)            ((av_object_p)(o))->get_attribute((av_object_p)(o), name)
#define O_set_attr(o, name, value) ((av_object_p)(o))->set_attribute((av_object_p)(o), name, value)

/*! A shortcut to \c is_a method of an object */
#define O_is_a(o, classname) (((av_object_p)(o))->is_a((av_object_p)(o), classname))

/*! A shortcut to \c tostring method of an object */
#define O_tostring(o, b, c)  ((av_object_p)(o))->tostring((av_object_p)(o), b, c)

/*! A shortcut to \c destroy method of an object */
#define O_destroy(o)         { ((av_object_p)(o))->destroy((av_object_p)(o)); o = AV_NULL; }

/*! A shortcut to \c release method of an object */
#define O_addref(o)         ((av_object_p)(o))->addref((av_object_p)(o))

/*! A shortcut to \c release method of an object */
#define O_release(o)         { ((av_object_p)(o))->release((av_object_p)(o)); }

/*! Refers oop from object */
#define O_oop(o)             ((av_object_p)o)->classref->oop

/*! Dump oop attributes */
#define O_dump(o)            ((av_object_p)o)->dump_attributes((av_object_p)o)

/*!
* \brief The super class of all avgl services
*/
typedef struct av_service
{
	/*! Parent class object */
	av_object_t object;

	/*! Service name */
	char name[MAX_NAME_SIZE];

} av_service_t, *av_service_p;

typedef struct _av_oop_t {
	/* maps class name to object */
	av_hash_p classmap;

	/* list with registered class names */
	av_list_p classes;

	/* map service name to service object */
	av_hash_p servicemap;

	/* list of registered service names */
	av_list_p services;

	/* destroys oop container */
	void (*destroy)(struct _av_oop_t* self);

	/*!
	* \brief Defines new class
	* \param pservice a memory with size more or equal of sizeof(av_service_t)
	* \param classname the class name to be registered
	* \param parentclassname the parent class name which this class inherites.
	*                        It can be AV_NULL which defaults to av_object to be the parent class
	* \param classsize the size of class in bytes, e.g. sizeof(av_foo_t)
	* \param constructor the class constructor or AV_NULL if not present for this class
	* \param destructor the class destructor or AV_NULL if not present for this class
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EFOUND if the \c parentclassname is not registered
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*define_class)(struct _av_oop_t* self,
		const char* classname,
		const char* parentclassname,
		int classsize,
		av_constructor_t constructor,
		av_destructor_t destructor);

	/*!
	* \brief Creates new object from the given class name.
	*
	* This function can be also considered as a factory method to all TORBA registered classes
	* \param classname the class name to be instantiated
	* \param ppobject the result object of class named \c classname
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EFOUND if the \c classname is not registered
	*         - AV_ESUPPORTED if the \c classname is abstract
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*new)(struct _av_oop_t* self, 
		const char* classname,
		av_object_p* ppobject);

	/*!
	* \brief Registers new service
	* \param servicename the service name under which this service will be refered
	* \param service an object deriving from \c av_service class
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*register_service)(struct _av_oop_t* self, 
		const char* servicename,
		av_service_p service);

	/*!
	* \brief Creates new reference to a service corresponding to a given name and increment service reference count
	* \param servicename the service name from which to obtain reference
	* \param ppservice is where the service object will be returned
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EFOUND if service named \c servicename is not registered
	*/
	av_result_t (*get_service)(struct _av_oop_t* self, 
		const char* servicename,
		av_service_p* ppservice);
} av_oop_t, *av_oop_p;

/*!
* \brief Creates new OOP container
*/
AV_API av_result_t av_oop_create(av_oop_p* pself);

#ifdef __cplusplus
}
#endif

#endif /* __AV_OOP_H */
