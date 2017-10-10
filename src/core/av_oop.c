/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_oop.c                                           */
/* Description:   OOP for C implementation                           */
/*                                                                   */
/*                                                                   */
/*********************************************************************/

#include <av_oop.h>
#include "av_stdc.h"
#include "av_debug.h"

/* the class names of avgl base classes */
static const char* object_class_name = "object";
static const char* service_class_name = "service";

// FiXME: shut dbg messages
#define av_dbg(x, ...)

/* av_object_t implementation */
/* --------------------------- */

/* Returns AV_TRUE if the object is derived from a class given by the \c class_identifier */
static av_bool_t av_object_is_a(av_object_p self, const char* classname)
{
	av_class_p classref;

	av_assert(classname, "NULL class name is not allowed");

	classref = self->classref;
	while (classref)
	{
		if (0 == (av_strcmp(classref->classname, classname)))
		{
			return AV_TRUE;
		}
		classref = classref->parent;
	}
	return AV_FALSE;
}

/*
* Sets custom attribute to that object
*/
static av_result_t av_object_set_attribute(av_object_p self, const char* name, void* value)
{
	av_result_t rc;
	if (!self->attributes)
	{
		if (AV_OK != (rc = av_hash_create(AV_HASH_CAPACITY_SMALL, &self->attributes)))
			return rc;
	}

	if (value)
	{
		self->attributes->remove(self->attributes, name);
		rc = self->attributes->add(self->attributes, name, value);
	}
	else
	{
		self->attributes->remove(self->attributes, name);
		rc = AV_OK;
	}
	return rc;
}

/*
* Gets custom attribute from the object
*/
static void* av_object_get_attribute(av_object_p self, const char* name)
{
	void* value = AV_NULL;
	if (self->attributes)
	{
		value = self->attributes->get(self->attributes, name);
	}
	return value;
}

/*
* Dumps attributes in stdout
*/
static void av_object_dump_attributes(av_object_p self)
{
	const char* key;
	void* value;
	char classname[MAX_NAME_SIZE];
	self->tostring(self, classname, MAX_NAME_SIZE);
	if (self->attributes)
	{
		self->attributes->first(self->attributes);
		while (self->attributes->next(self->attributes, &key, &value))
		{
			printf("%s: %s = %p\n", classname, key, value);
		}
	}
}

/*
* Copy object string representation into the given buffer without exceeding the specified size
*/
static char* av_object_tostring(av_object_p self, char* buf, int bufsize)
{
	snprintf(buf, bufsize, "[%s %p]", self->classref->classname, (void*)self);
	return buf;
}

/* Object destructor */
static void av_object_destructor(av_object_p self)
{
	av_class_p classref;
	av_dbg("av_object_destructor: %p %s\n", self, self->classref->classname);

	classref = self->classref;
	// call all parent destructors but without object
	while (classref->parent)
	{
		if (classref->destructor)
			classref->destructor(self);
		classref = classref->parent;
	}

	if (self->attributes)
	{
		self->attributes->destroy(self->attributes);
	}
	free(self);
}

/* Object constructor */
static av_result_t av_object_constructor(av_object_p self)
{
	av_dbg("av_object_constructor %p\n", self);
	self->attributes = AV_NULL;
	self->is_a = av_object_is_a;
	self->tostring = av_object_tostring;
	self->set_attribute = av_object_set_attribute;
	self->get_attribute = av_object_get_attribute;
	self->dump_attributes = av_object_dump_attributes;
	self->destroy = av_object_destructor;
	return AV_OK;
}

/* Service class implementation */
/* --------------------------- */

/* Increment reference counter */
static void av_service_addref(av_service_p self)
{
	self->refcnt++;
}

/* Decrement reference counter and destroy the object when reaches 0 */
static void av_service_release(av_service_p self)
{
	if (0 == --(self->refcnt))
	{
		O_destroy(self);
	}
}

/* Initializes memory given by the object argument with a service class information */
static av_result_t av_service_constructor(av_object_p object)
{
	av_dbg("av_service_constructor: %p\n", object);
	av_service_p self = (av_service_p)object;
	self->name[0] = '\0';
	self->refcnt = 0;
	self->addref = av_service_addref;
	self->release = av_service_release;
	return AV_OK;
}

/* av_oop_t implementation */
/* ----------------------- */

/* Defines new class into class repository */
static av_result_t av_oop_define_class(av_oop_p self,
	const char* classname,
	const char* parentclassname,
	int classsize,
	av_constructor_t constructor,
	av_destructor_t destructor)
{
	av_result_t rc;
	av_class_p clazz;
	av_class_p parent = AV_NULL;

	av_assert(classname, "NULL class name is not allowed");

	/* if parent class is not given set object as a parent unless the class is also the object */
	if (!parentclassname && (0 != av_strcmp(classname, object_class_name)))
	{
		parentclassname = object_class_name;
	}

	av_dbg("av_oop_define_class: %s->%s %d\n", classname, parentclassname, classsize);

	/* checks if the class name is not already defined */
	if (AV_NULL != self->classmap->get(self->classmap, classname))
	{
		av_dbg("av_oop_define_class: %s is already defined\n", classname);

		/* already defined */
		return AV_OK;
	}

	/* gets class parent */
	if (parentclassname)
	{
		parent = (av_class_p)self->classmap->get(self->classmap, parentclassname);
		if (!parent)
		{
			av_dbg("Parent class %s is not found\n", parentclassname);
			return AV_EFOUND;
		}
	}

	/* creates new class descriptor entry */
	clazz = (av_class_p)av_malloc(sizeof(av_class_t));
	if (!clazz)
		return AV_EMEM;

	av_strncpy(clazz->classname, classname, av_strlen(classname));
	clazz->oop = self;
	clazz->parent = parent;
	clazz->classsize = classsize;
	clazz->constructor = constructor;
	clazz->destructor = destructor;

	/* adds the new class entry to repository hashtable */
	if (AV_OK != (rc = self->classmap->add(self->classmap, classname, clazz)))
	{
		free(clazz);
		return rc;
	}

	/* adds the new class name to repository list*/
	if (AV_OK != (rc = self->classes->push_last(self->classes, clazz)))
	{
		free(clazz);
		return rc;
	}

	return AV_OK;
}

/* Registers new service into service repository */
static av_result_t av_oop_register_service(av_oop_p self, const char* servicename, av_service_p service)
{
	av_result_t rc;
	av_assert(servicename && service,
		"NULL service name or service object is not allowed");

	av_dbg("av_oop_register_service: %p registered as service %s\n", service, servicename);
	if (AV_NULL != self->servicemap->get(self->servicemap, servicename))
	{
		av_dbg("av_oop_register_service: %s is already registered\n", servicename);

		/* already registered */
		return AV_OK;
	}

	/* add the new service to service repository hashtable */
	if (AV_OK != (rc = self->servicemap->add(self->servicemap, servicename, service)))
	{
		return rc;
	}

	/* add the new service name to service repository list */
	if (AV_OK != (rc = self->services->push_last(self->services, av_strdup(servicename))))
	{
		self->servicemap->remove(self->servicemap, servicename);
		return rc;
	}

	return AV_OK;
}

/* Creates new reference to a service with a given name */
static av_result_t av_oop_service_ref(av_oop_p self, const char* servicename, av_service_p* ppservice)
{
	av_service_p service;
	av_assert(servicename, "NULL service name is not allowed");

	if (AV_NULL == (service = self->servicemap->get(self->servicemap, servicename)))
	{
		av_dbg("av_oop_service_ref: Service %s not found\n", servicename);
		/* service not found */
		return AV_EFOUND;
	}

	*ppservice = service;
	service->addref(service);
	return AV_OK;
}

/* Creates new object from the given class */
static av_result_t av_oop_new(av_oop_p self, const char* classname, av_object_p* ppobject)
{
	av_result_t rc;
	av_class_p classref;
	av_object_p object;
	av_list_p parentslist;

	av_assert(classname, "NULL class name is not allowed");

	classref = (av_class_p)self->classmap->get(self->classmap, classname);
	if (AV_NULL == classref)
	{
		av_dbg("av_oop_new: Class %s not found\n", classname);
		return AV_EFOUND;
	}

	object = (av_object_p)calloc(1, classref->classsize);
	if (!object)
		return AV_EMEM;

	object->classref = classref;

	if (AV_OK != (rc = av_list_create(&parentslist)))
	{
		free(object);
		return rc;
	}
	while (classref)
	{
		parentslist->push_last(parentslist, classref);
		classref = classref->parent;
	}

	for (parentslist->last(parentslist); parentslist->has_more(parentslist); parentslist->prev(parentslist))
	{
		classref = (av_class_p)parentslist->get(parentslist);
		if (classref->constructor)
			if (AV_OK != (rc = classref->constructor(object)))
			{
				parentslist->destroy(parentslist);
				free(object);
				return rc;
			}
	}
	*ppobject = object;
	av_dbg("av_oop_new: New object %p of class %s\n", object, classname);
	parentslist->destroy(parentslist);
	return AV_OK;
}

static void av_oop_destroy(av_oop_p self)
{
	av_dbg("av_oop_destroy\n");
	self->services->iterate_all(self->services, free, AV_FALSE);
	self->classes->iterate_all(self->classes, free, AV_FALSE);
	self->services->destroy(self->services);
	self->classes->destroy(self->classes);
	self->servicemap->destroy(self->servicemap);
	self->classmap->destroy(self->classmap);
	free(self);
}

av_result_t av_oop_create(av_oop_p* pself)
{
	av_result_t rc;
	av_oop_p self = (av_oop_p)av_malloc(sizeof(av_oop_t));
	av_dbg("av_oop_create\n");
	if (!self)
		return AV_EMEM;

	/* creates class repository hashtable */
	if (AV_OK != (rc = av_hash_create(AV_HASH_CAPACITY_MEDIUM, &self->classmap)))
	{
		free(self);
		return rc;
	}

	/* creates class repository list */
	if (AV_OK != (rc = av_list_create(&self->classes)))
	{
		O_destroy(self->classmap);
		free(self);
		return rc;
	}

	/* creates service repository hashtable */
	if (AV_OK != (rc = av_hash_create(AV_HASH_CAPACITY_MEDIUM, &self->servicemap)))
	{
		O_destroy(self->classmap);
		O_destroy(self->classes);
		free(self);
		return rc;
	}

	/* creates service repository list */
	if (AV_OK != (rc = av_list_create(&self->services)))
	{
		O_destroy(self->servicemap);
		O_destroy(self->classmap);
		O_destroy(self->classes);
		free(self);
		return AV_EMEM;
	}

	self->define_class     = av_oop_define_class;
	self->new              = av_oop_new;
	self->register_service = av_oop_register_service;
	self->service_ref      = av_oop_service_ref;
	self->destroy          = av_oop_destroy;

	/* register base classes */
	self->define_class(self, object_class_name, NULL, sizeof(av_object_t), av_object_constructor, av_object_destructor);
	self->define_class(self, service_class_name, object_class_name, sizeof(av_object_t), av_service_constructor, AV_NULL);

	*pself = self;
	return AV_OK;
}
