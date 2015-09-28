/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_torb.c                                          */
/* Description:   TORBA implementation.                              */
/*                Trivial Object-Request-Broker Architecture         */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <av_torb.h>
#include <av_list.h>
#include <av_hash.h>
#include <av_log.h>
#include <av_prefs.h>
#include <av_system.h>
#include <av_player.h>
#include <av_graphics.h>

#ifdef WITH_SYSTEM_SDL
int av_system_sdl_register_torba(void);
#endif

#ifdef WITH_SYSTEM_DIRECTFB
int av_system_dfb_register_torba(void);
#endif

#ifdef WITH_GRAPHICS_CAIRO
int av_graphics_cairo_register_torba(void);
#endif

/* Default preferences in case not defined in the configuration file */

/* default logging preferences */
static int _logfileenable            = 0;
static const char* _logfilename      = "avgl.log";
static const char* _logfileverbosity = "debug";
static int _logconenable             = 1;
static const char* _logconverbosity  = "info";

/* default system preferences */
static const char* _systembackend    = "sdl";

/* class descriptor */
typedef struct _av_class_tree_t
{
	char classname[MAX_NAME_SIZE];
	int classsize;
	struct _av_class_tree_t* parent;
	av_torb_constructor_t constructor;
	av_torb_destructor_t destructor;
} av_class_tree_t, *av_class_tree_p;

/* the class name of the base class for all TORBA classes */
const char* superclassname = "object";

/* TORBA initialization status */
static av_bool_t _torba_initialized = AV_FALSE;

/* hashtable class name to object */
static av_hash_p _class_repository_ht = AV_NULL;

/* list with registered class names */
static av_list_p _class_repository_ls = AV_NULL;

/* hashtable service name to service object */
static av_hash_p _service_repository_ht = AV_NULL;

/* list of registered service names */
static av_list_p _service_repository_ls = AV_NULL;

/* reference to log service */
static av_log_p _log = AV_NULL;

/* reference to system service */
static av_system_p _system = AV_NULL;

/* reference to prefs service */
static av_prefs_p _prefs = AV_NULL;

/* Returns AV_TRUE if the object is derived from a class given by the \c class_identifier */
static av_bool_t av_object_is_a(av_object_p self, const char* classname)
{
	av_class_tree_p classref;

	av_assert(_class_repository_ht, "TORBA is not initialized");
	av_assert(classname, "NULL class name is not allowed");

	classref = (av_class_tree_p)_class_repository_ht->get(_class_repository_ht, self->classname);
	while (classref)
	{
		if (0 == (strcmp(classref->classname, classname)))
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
	self->attributes->first(self->attributes);
	while (self->attributes->next(self->attributes, &key, &value))
	{
		printf("%s: %s = %p\n", classname, key, value);
	}
}

/*
* Copy object string representation into the given buffer without exceeding the specified size
*/
static char* av_object_tostring(av_object_p self, char* buf, int bufsize)
{
	snprintf(buf, bufsize, "[%s %p]", self->classname, (void*)self);
	return buf;
}

/* Object destructor */
static void av_object_destructor(void* pobject)
{
	av_object_p self = (av_object_p)pobject;
	if (self->attributes)
	{
		self->attributes->destroy(self->attributes);
	}
	free(self);
}

/* Object constructor */
static av_result_t av_object_constructor(av_object_p self)
{
	self->attributes      = AV_NULL;
	self->is_a            = av_object_is_a;
	self->tostring        = av_object_tostring;
	self->set_attribute   = av_object_set_attribute;
	self->get_attribute   = av_object_get_attribute;
	self->dump_attributes = av_object_dump_attributes;
	self->destroy         = av_object_destructor;
	return AV_OK;
}

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

/* registers core TORBA classes and services */
static av_result_t av_torb_register_core( void )
{
	av_result_t rc;

	/* registers prefs module */
	if (AV_OK != (rc = av_prefs_register_torba()))
		return rc;

	/* registers log module */
	if (AV_OK != (rc = av_log_register_torba()))
		return rc;

	return AV_OK;
}

/* registers system TORBA classes and services */
static av_result_t av_torb_register_system( void )
{
	av_result_t rc;

	/* registers window module */
	if (AV_OK != (rc = av_window_register_torba()))
		return rc;

	/* registers system module */
	if (AV_OK != (rc = av_system_register_torba()))
		return rc;

#ifdef WITH_SYSTEM_SDL
	/* registers system module */
	if (AV_OK != (rc = av_system_sdl_register_torba()))
		return rc;
#endif

#ifdef WITH_SYSTEM_DIRECTFB
	/* registers system module */
	if (AV_OK != (rc = av_system_dfb_register_torba()))
		return rc;
#endif

	/* registers graphics */
	if (AV_OK != (rc = av_graphics_register_torba()))
		return rc;

#ifdef WITH_GRAPHICS_CAIRO
	/* registers graphics cairo */
	if (AV_OK != (rc = av_graphics_cairo_register_torba()))
		return rc;
#endif

	/* registers player object */
	if (AV_OK != (rc = av_player_register_torba()))
		return rc;

	return AV_OK;
}

/* Initializes memory given by the input pointer with a service class information */
static av_result_t av_service_constructor(av_object_p object)
{
	av_service_p self = (av_service_p)object;
	self->name[0] = '\0';
	self->refcnt  = 0;
	self->addref  = av_service_addref;
	self->release = av_service_release;
	return AV_OK;
}

/* Setup logger according the preferences */
static av_log_verbosity_t av_torb_parse_verbosity(const char* verbosity, av_log_verbosity_t default_verbosity)
{
	if (0 == strcasecmp("DEBUG", verbosity))
		return LOG_VERBOSITY_DEBUG;
	else if (0 == strcasecmp("INFO", verbosity))
		return LOG_VERBOSITY_INFO;
	else if (0 == strcasecmp("WARN", verbosity))
		return LOG_VERBOSITY_WARN;
	else if (0 == strcasecmp("ERROR", verbosity))
		return LOG_VERBOSITY_ERROR;
	else if (0 == strcasecmp("SILENT", verbosity))
		return LOG_VERBOSITY_SILENT;
	else return default_verbosity;
}

static av_result_t av_torb_setup_log(void)
{
	int islogconsole;
	int islogfile;

	_prefs->get_int(_prefs, "log.console.enabled", _logconenable, &islogconsole);
	if (islogconsole)
	{
		const char* logconverbosity;
		_prefs->get_string(_prefs, "log.console.verbosity", _logconverbosity, &logconverbosity);
		_log->add_console_logger(_log, av_torb_parse_verbosity(logconverbosity, LOG_VERBOSITY_INFO),
								 "console");
	}

	_prefs->get_int(_prefs, "log.file.enabled", _logfileenable, &islogfile);
	if (islogfile)
	{
		const char* logfilename;
		const char* logfileverbosity;
		_prefs->get_string(_prefs, "log.file.name", _logfilename, &logfilename);
		_prefs->get_string(_prefs, "log.file.verbosity", _logfileverbosity, &logfileverbosity);
		_log->add_file_logger(_log, av_torb_parse_verbosity(logfileverbosity, LOG_VERBOSITY_DEBUG),
							  "file", logfilename);
	}

	return AV_OK;
}

static av_result_t av_torb_setup_system(void)
{
	av_result_t rc;
	const char* systembackend;
	char system_class_name[MAX_NAME_SIZE];
	_prefs->get_string(_prefs, "system.backend", _systembackend, &systembackend);

	strcpy(system_class_name, "system_");
	strncat(system_class_name, systembackend, MAX_NAME_SIZE-7);

	if (AV_OK != (rc = av_torb_create_object(system_class_name, (av_object_p*)&_system)))
		return rc;

	if (AV_OK != (rc = av_torb_service_register("system", (av_service_p)_system)))
	{
		O_destroy(_system);
		return rc;
	}

	/* Add new reference to system service */
	if (AV_OK != (rc = av_torb_service_addref("system", (av_service_p*)&_system)))
	{
		O_destroy(_system);
		return rc;
	}

	return rc;
}

/* Initializes TORBA */
av_result_t av_torb_init()
{
	av_result_t rc;

	if (AV_FALSE == _torba_initialized) /* not already initialized? */
	{
		av_class_tree_p objectclass;

		/* creates class repository hashtable */
		if (AV_OK != (rc = av_hash_create(AV_HASH_CAPACITY_MEDIUM, &_class_repository_ht)))
			return rc;

		/* creates class repository list */
		if (AV_OK != (rc = av_list_create(&_class_repository_ls)))
		{
			av_torb_done();
			return AV_EMEM;
		}

		/* creates service repository hashtable */
		if (AV_OK != (rc = av_hash_create(AV_HASH_CAPACITY_MEDIUM, &_service_repository_ht)))
		{
			av_torb_done();
			return rc;
		}

		/* creates service repository list */
		if (AV_OK != (rc = av_list_create(&_service_repository_ls)))
		{
			av_torb_done();
			return AV_EMEM;
		}

		/* creates object class */
		objectclass = (av_class_tree_p)malloc(sizeof(av_class_tree_t));
		if (!objectclass)
		{
			av_torb_done();
			return AV_EMEM;
		}
		strcpy(objectclass->classname, superclassname);

		objectclass->parent        = AV_NULL;
		objectclass->constructor   = av_object_constructor;
		objectclass->destructor    = av_object_destructor;

		/* registers object to class repository hashtable */
		if (AV_OK != (rc = _class_repository_ht->add(_class_repository_ht, superclassname, objectclass)))
		{
			av_torb_done();
			free(objectclass);
			return rc;
		}

		/* registers object to class repository list */
		if (AV_OK != (rc = _class_repository_ls->push_last(_class_repository_ls, objectclass)))
		{
			av_torb_done();
			free(objectclass);
			return rc;
		}

		_torba_initialized = AV_TRUE;

		/* register class service */
		av_torb_register_class("service", AV_NULL, sizeof(av_service_t), av_service_constructor, AV_NULL);

		/* register core classes and services */
		if (AV_OK != (rc = av_torb_register_core()))
		{
			av_torb_done();
			free(objectclass);
			return rc;
		}

		/* initializes prefs service */
		if (AV_OK != av_torb_service_addref("prefs", (av_service_p*)&_prefs))
		{
			av_torb_done();
			free(objectclass);
			return rc;
		}

		/* initializes log service */
		if (AV_OK != av_torb_service_addref("log", (av_service_p*)&_log))
		{
			av_torb_done();
			free(objectclass);
			return rc;
		}

		av_torb_setup_log();

		/* register system classes and services */
		if (AV_OK != (rc = av_torb_register_system()))
		{
			av_torb_done();
			free(objectclass);
			return rc;
		}

		av_torb_setup_system();

		_log->info(_log, "TORBA initialized");
	}
	return AV_OK;
}

/* Deinitializes TORBA */
void av_torb_done()
{
	av_assert(_torba_initialized, "TORBA is not initialized");
	if (_log)
	{
		_log->info(_log, "Finalizing TORBA");
	}

	if (_system)
	{
		av_assert(((av_service_p)_system)->refcnt == 1,
				  "Unreleased references to system service");
		av_torb_service_release("system");
		_system = AV_NULL;
	}

	if (_prefs)
	{
		av_assert(((av_service_p)_prefs)->refcnt == 1,
				  "Unreleased references to prefs service");
		av_torb_service_release("prefs");
		_prefs = AV_NULL;
	}

	if (_log)
	{
		av_assert(((av_service_p)_log)->refcnt == 1,
				  "Unreleased references to log service");
		av_torb_service_release("log");
		_log = AV_NULL;
	}

	/* destroys service repository */
	if (_service_repository_ls)
	{
		for (_service_repository_ls->last(_service_repository_ls);
			 _service_repository_ls->has_more(_service_repository_ls);
			 _service_repository_ls->prev(_service_repository_ls))
		{
			char* servicename = (char*)_service_repository_ls->get(_service_repository_ls);
			free(servicename);
		}
		_service_repository_ls->destroy(_service_repository_ls);
		_service_repository_ls = AV_NULL;
		_service_repository_ht->destroy(_service_repository_ht);
		_service_repository_ht = AV_NULL;
	}

	/* destroys class repository list */
	if (_class_repository_ls)
	{
		/* free class descriptors in reverse order */
		_class_repository_ls->iterate_all(_class_repository_ls, free, AV_FALSE);
		_class_repository_ls->destroy(_class_repository_ls);
		_class_repository_ls = AV_NULL;
	}

	/* destroys class repository hashtable */
	if (_class_repository_ht)
	{
		_class_repository_ht->destroy(_class_repository_ht);
		_class_repository_ht = AV_NULL;
	}

	_torba_initialized = AV_FALSE;
}

/* Registers new class into class repository */
av_result_t av_torb_register_class(const char* classname,
								   const char* parentclassname,
								   int classsize,
								   av_torb_constructor_t constructor,
								   av_torb_destructor_t destructor)
{
	av_result_t rc;
	av_class_tree_p self;
	av_class_tree_p parent;

	av_assert(_torba_initialized, "TORBA is not initialized");
	av_assert(classname, "NULL class name is not allowed");

	/* if parent class is not given it is assumed to be an object */
	if (!parentclassname) parentclassname = superclassname;

	/* checks if the class name is not already registered */
	if (AV_NULL != (self = _class_repository_ht->get(_class_repository_ht, classname)))
	{
		/* already registered */
		return AV_OK;
	}

	/* gets class parent */
	parent = (av_class_tree_p)_class_repository_ht->get(_class_repository_ht, parentclassname);
	if (!parent)
		return AV_EFOUND;

	/* creates new class descriptor entry */
	self = (av_class_tree_p)malloc(sizeof(av_class_tree_t));
	if (!self)
		return AV_EMEM;

	strncpy(self->classname, classname, MAX_NAME_SIZE);
	self->parent      = parent;
	self->classsize   = classsize;
	self->constructor = constructor;
	self->destructor  = destructor;

	/* adds the new class entry to repository hashtable */
	if (AV_OK != (rc = _class_repository_ht->add(_class_repository_ht, classname, self)))
	{
		free(self);
		return rc;
	}

	/* adds the new class name to repository list*/
	if (AV_OK != (rc = _class_repository_ls->push_last(_class_repository_ls, self)))
	{
		free(self);
		return rc;
	}

	return AV_OK;
}

/* Registers new service into service repository */
av_result_t av_torb_service_register(const char* servicename, av_service_p service)
{
	av_result_t rc;
	av_assert(_torba_initialized, "TORBA is not initialized");
	av_assert(servicename && service,
			 "NULL service name or service object is not allowed");

	if (AV_NULL != _service_repository_ht->get(_service_repository_ht, servicename))
	{
		/* already registered */
		return AV_OK;
	}

	/* add the new service to service repository hashtable */
	if (AV_OK != (rc = _service_repository_ht->add(_service_repository_ht, servicename, service)))
	{
		return rc;
	}

	/* add the new service name to service repository list */
	if (AV_OK != (rc = _service_repository_ls->push_last(_service_repository_ls, strdup(servicename))))
	{
		_service_repository_ht->remove(_service_repository_ht, servicename);
		return rc;
	}

	return AV_OK;
}

/*
* Calls the destructor associated with the given class against the object.
* If the classname is AV_NULL it is assumed the \c object class
*/
static void av_torb_destroy_object(void* pobject)
{
	av_object_p self = (av_object_p)pobject;
	av_class_tree_p classref;

	if (_log)
		_log->debug(_log, "torba: destroy object %s (%p)", self->classname, self);

	av_assert(_torba_initialized, "TORBA is not initialized");

	classref = _class_repository_ht->get(_class_repository_ht, self->classname);
	av_assert(classref, "object is not properly initialized");
	while (classref)
	{
		if (classref->destructor)
			classref->destructor(self);
		classref = classref->parent;
	}
}

/* Creates new object from the given class */
av_result_t av_torb_create_object(const char* classname, av_object_p* ppobject)
{
	av_result_t rc;
	av_class_tree_p classref;
	av_object_p self;
	av_list_p parentslist;

	av_assert(_torba_initialized, "TORBA is not initialized");
	av_assert(classname, "NULL class name is not allowed");

	classref = (av_class_tree_p)_class_repository_ht->get(_class_repository_ht, classname);
	if (AV_NULL == classref)
		return AV_EFOUND;

	self = (av_object_p)malloc(classref->classsize);
	if (!self)
		return AV_EMEM;

	if (_log)
		_log->debug(_log, "torba: create object %s (%p)", classname, self);

	self->classname = classref->classname;

	if (AV_OK != (rc = av_list_create(&parentslist)))
	{
		free(self);
		return rc;
	}
	while (classref)
	{
		parentslist->push_last(parentslist, classref);
		classref = classref->parent;
	}

	for (parentslist->last(parentslist); parentslist->has_more(parentslist); parentslist->prev(parentslist))
	{
		classref = (av_class_tree_p)parentslist->get(parentslist);
		if (classref->constructor)
			if (AV_OK != (rc = classref->constructor(self)))
			{
				parentslist->destroy(parentslist);
				free(self);
				return rc;
			}
	}

	self->destroy = av_torb_destroy_object;
	*ppobject = self;
	parentslist->destroy(parentslist);
	return AV_OK;
}

/* Creates new reference to a service corresponding to a given name */
av_result_t av_torb_service_addref(const char* servicename, av_service_p* ppservice)
{
	av_service_p service;
	av_assert(_torba_initialized, "TORBA is not initialized");
	av_assert(servicename, "NULL service name is not allowed");

	if (_log)
		_log->debug(_log, "Refering service `%s'", servicename);

	if (AV_NULL == (service = _service_repository_ht->get(_service_repository_ht, servicename)))
	{
		/* service not found */
		return AV_EFOUND;
	}

	*ppservice = service;
	service->addref(service);
	return AV_OK;
}

/*
*	Decrements the reference counter to a service and destroy it when it reaches 0
*	by calling the destructor associated with its class
*/
av_result_t av_torb_service_release(const char* servicename)
{
	av_service_p service;
	av_assert(_torba_initialized, "TORBA is not initialized");

	if (_log)
		_log->debug(_log, "Releasing service `%s'", servicename);

	if (AV_NULL == (service = _service_repository_ht->get(_service_repository_ht, servicename)))
	{
		_log->warn(_log, "Service %s not found", servicename);
		return AV_EFOUND;
	}
	else
	{
		service->release(service);
	}
	return AV_OK;
}
