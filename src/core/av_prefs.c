/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_prefs.c                                         */
/*                                                                   */
/*********************************************************************/

#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <av_prefs.h>
#include <av_tree.h>

#define MAX_INT_CHAR_SIZE 50
#define NOT_ALLOCATE 0
#define ALLOCATE 1

#define CONTEXT "prefs_ctx"
#define O_context(o) O_attr(o, CONTEXT)

typedef struct av_prefs_observe_info
{
	av_list_p path;
	av_prefs_observer_t observer;
	void* param;
} av_prefs_observe_info_t, *av_prefs_observe_info_p;

typedef struct av_prefs_context
{
	av_tree_node_p tree;
	av_list_p observers;
	av_bool_t notify_observers;
	char filename[MAX_FILENAME_SIZE];
} av_prefs_context_t, *av_prefs_context_p;

static void av_prefs_get_home_prefs(char home_prefs[MAX_FILENAME_SIZE])
{
	const char* home = getenv("HOME");

	if (home)
	{
		strcpy(home_prefs, getenv("HOME"));
	}
	else
	{
		strcpy(home_prefs, ".");
	}
	strcat(home_prefs, "/"AV_CONFIG_USER_PREFERENCES);
}

/* path of form <ident>{'.'<ident>} is decomposed to a list of string represented idents */
static av_result_t av_prefs_path_to_list(const char* path, av_list_p list)
{
	av_result_t rc;
	char* dot;
	av_assert(path, "path can't be null");
	av_assert(list, "list can't be null");

	dot = strchr(path, '.');
	if (dot)
	{
		int esize = dot - path;
		char* element = (char*)malloc(1+esize);
		strncpy(element, path, esize);
		element[esize] = '\0';
		if (AV_OK != (rc = list->push_last(list, element)))
		{
			free(element);
			return rc;
		}
		return av_prefs_path_to_list(dot+1, (void*)list);
	}
	return list->push_last(list, strdup(path));
}

static av_result_t av_prefs_get_path(av_prefs_p self, const char* path, av_tree_node_p* ppnode, int isalloc)
{
	av_result_t rc;
	av_list_p list;
	av_prefs_context_p ctx = (av_prefs_context_p)O_context(self);
	av_tree_node_p node = ctx->tree;
	if (AV_OK != (rc = av_list_create(&list)))
		return rc;

	if (AV_OK != (rc = av_prefs_path_to_list(path, list)))
	{
		list->destroy(list);
		return rc;
	}

	for (list->first(list); list->has_more(list); list->next(list))
	{
		char* element = (char*)list->get(list);
		av_bool_t found = AV_FALSE;
		av_prefs_value_p value;

		for (node->children->first(node->children);
			 node->children->has_more(node->children);
			 node->children->next(node->children))
		{
			av_tree_node_p cnode = (av_tree_node_p)node->children->get(node->children);
			value = (av_prefs_value_p)cnode->item;
			if (0 == strcmp(element, value->name))
			{
				/* element found */
				node = cnode;
				found = AV_TRUE;
				break;
			}
		}
		if (!found)
		{
			if (isalloc)
			{
				/* allocates new node */
				av_tree_node_p nnode;
				if (AV_OK != (rc = av_tree_node_create(node, &nnode)))
				{
					list->remove_all(list, free);
					list->destroy(list);
					return rc;
				}
				value = (av_prefs_value_p)malloc(sizeof(av_prefs_value_t));
				value->name = strdup(element);
				value->value_type = AV_PREFS_VALUE_UNDEF;
				value->nvalue = 0;
				value->svalue = AV_NULL;
				nnode->item = value;
				node = nnode;
			}
			else
			{
				list->remove_all(list, free);
				list->destroy(list);
				return AV_EFOUND;
			}
		}
	}
	list->remove_all(list, free);
	list->destroy(list);
	*ppnode = node;
	return AV_OK;
}

static av_result_t av_prefs_notify_observers(av_prefs_p self, const char* name, av_tree_node_p node)
{
	av_prefs_context_p ctx = (av_prefs_context_p)O_context(self);
	av_result_t rc;
	av_list_p path;
	av_list_p observers = ctx->observers;
	av_tree_node_p pnode = node;

	if (!(ctx->notify_observers))
		return AV_OK;

	if (AV_OK != (rc = av_list_create(&path)))
		return rc;

	while (pnode && pnode->item)
	{
		av_prefs_value_p value = (av_prefs_value_p)pnode->item;
		if (AV_OK != (rc = path->push_first(path, value->name)))
		{
			path->destroy(path);
			return rc;
		}
		pnode = pnode->parent;
	}

	for (observers->first(observers); observers->has_more(observers); observers->next(observers))
	{
		av_prefs_observe_info_p observe_info = (av_prefs_observe_info_p)observers->get(observers);
		av_list_p opath = observe_info->path;
		av_bool_t match = AV_TRUE;

		/* compare node path with observer path */
		path->first(path);
		for (opath->first(opath); opath->has_more(opath); opath->next(opath))
		{
			const char* key = (const char*)path->get(path);
			const char* okey = (const char*)opath->get(opath);
			if (0 != strcmp(key, okey))
			{
				match = AV_FALSE;
				break;
			}

			if (path->has_more(path))
				path->next(path);
			else
				break;
		}

		if (match)
		{
			/* notify observer */
			observe_info->observer(observe_info->param, name, (av_prefs_value_p)node->item);
		}
	}
	path->destroy(path);
	return AV_OK;
}

static av_result_t av_prefs_set_int(av_prefs_p self, const char* name, int nvalue)
{
	av_result_t rc;
	av_tree_node_p node;
	av_prefs_value_p value;

	if (AV_OK != (rc = av_prefs_get_path(self, name, &node, ALLOCATE)))
		return rc;
	value = (av_prefs_value_p)node->item;
	if (value->svalue)
		free(value->svalue);

	value->value_type = AV_PREFS_VALUE_NUMBER;
	value->nvalue = nvalue;
	value->svalue = (char*)malloc(MAX_INT_CHAR_SIZE);
	snprintf(value->svalue, MAX_INT_CHAR_SIZE, "%d", nvalue);
	return av_prefs_notify_observers(self, name, node);
}

static av_result_t av_prefs_get_int(av_prefs_p self, const char* name, int ndefault, int* nvalue)
{
	av_result_t rc;
	av_tree_node_p node;
	av_prefs_value_p value;

	if (AV_OK != (rc = av_prefs_get_path(self, name, &node, NOT_ALLOCATE)))
	{
		if (AV_EFOUND == rc)
		{
			*nvalue = ndefault;
			return AV_OK;
		}
		return rc;
	}
	value = (av_prefs_value_p)node->item;
	*nvalue = value->nvalue;
	return AV_OK;
}

static av_result_t av_prefs_set_string(av_prefs_p self, const char* name, const char* svalue)
{
	av_result_t rc;
	av_tree_node_p node;
	av_prefs_value_p value;

	if (AV_OK != (rc = av_prefs_get_path(self, name, &node, ALLOCATE)))
		return rc;
	value = (av_prefs_value_p)node->item;
	if (value->svalue)
		free(value->svalue);

	value->value_type = AV_PREFS_VALUE_STRING;
	value->nvalue = atoi(svalue);
	value->svalue = strdup(svalue);
	return av_prefs_notify_observers(self, name, node);
}

static av_result_t av_prefs_get_string(av_prefs_p self, const char* name, const char* sdefault, const char** svalue)
{
	av_result_t rc;
	av_tree_node_p node;
	av_prefs_value_p value;

	if (AV_OK != (rc = av_prefs_get_path(self, name, &node, NOT_ALLOCATE)))
	{
		if (AV_EFOUND == rc)
		{
			*svalue = sdefault;
			return AV_OK;
		}
		return rc;
	}
	value = (av_prefs_value_p)node->item;

	if (value->svalue)
		*svalue = value->svalue;
	else
		*svalue = sdefault;
	return AV_OK;
}

static av_result_t av_prefs_register_observer(av_prefs_p self,
											  const char* name,
											  av_prefs_observer_t observer,
											  void* param)
{
	av_result_t rc;
	av_prefs_context_p ctx = (av_prefs_context_p)O_context(self);
	av_prefs_observe_info_p	observe_info = (av_prefs_observe_info_p)malloc(sizeof(av_prefs_observe_info_t));
	if (!observe_info)
		return AV_EMEM;

	if (AV_OK != (rc = av_list_create(&(observe_info->path))))
	{
		free(observe_info);
		return rc;
	}

	observe_info->observer = observer;
	observe_info->param    = param;
	if (AV_OK != (rc = av_prefs_path_to_list(name, observe_info->path)))
	{
		observe_info->path->destroy(observe_info->path);
		free(observe_info);
		return rc;
	}

	if (AV_OK != (rc = ctx->observers->push_last(ctx->observers, observe_info)))
	{
		observe_info->path->remove_all(observe_info->path, free);
		observe_info->path->destroy(observe_info->path);
		free(observe_info);
		return rc;
	}

	return AV_OK;
}

static av_result_t av_prefs_unregister_observer(av_prefs_p self, av_prefs_observer_t observer)
{
	av_prefs_context_p ctx = (av_prefs_context_p)O_context(self);
	for (ctx->observers->first(ctx->observers);
		 ctx->observers->has_more(ctx->observers);
		 ctx->observers->next(ctx->observers))
	{
		av_prefs_observe_info_p	observe_info = (av_prefs_observe_info_p)ctx->observers->get(ctx->observers);
		if (observer == observe_info->observer)
		{
			observe_info->path->remove_all(observe_info->path, free);
			observe_info->path->destroy(observe_info->path);
			free(observe_info);
			ctx->observers->remove(ctx->observers);
			return AV_OK;
		}
	}
	return AV_EFOUND;
}

static av_result_t av_prefs_load(av_prefs_p self, const char* filename)
{
	av_prefs_context_p ctx = (av_prefs_context_p)O_context(self);
	av_bool_t keep_notify_observers = ctx->notify_observers;
	char line[255];
	FILE* file = fopen(filename, "r");
	if (!file)
		return AV_EFOUND;

    printf("Loading preferences from %s\n", filename);
	strncpy(ctx->filename, filename, MAX_FILENAME_SIZE);
	ctx->notify_observers = AV_FALSE;
	while (fgets(line, 255, file))
	{
		char name[255];
		char* pline = line;
		char* value = strchr(line, '=');
		if (value)
		{
			int size;

			/* trim left name */
			while (isspace(*pline++));
			size = value - --pline;
			strncpy(name, pline, size);

			if ('#' != name[0])
			{
				/* trim right name */
				while (isspace(name[--size]));
				name[1+size] = '\0';

				value++; /* skip `=' */
				/* trim left value */
				while (isspace(*value)) value++;

				/* trim right value */
				size = strlen(value);
				while (isspace(value[--size]));
				value[1+size] = '\0';

				self->set_string(self, name, value);
			}
		}
	}
	fclose(file);
	ctx->notify_observers = keep_notify_observers;
	return AV_OK;
}

static void av_prefs_save_recurse(av_tree_node_p node, char* name, FILE* file)
{
	if (node)
	{
		av_prefs_value_p value;
		char* p = name;
		av_list_p children = node->children;
		p += strlen(name);
		if (children->size(children) > 0)
		{
			for (children->first(children); children->has_more(children); children->next(children))
			{
				node = (av_tree_node_p)children->get(children);
				value = (av_prefs_value_p)node->item;
				if (strlen(name)>0)
					strcat(name, ".");
				strcat(name, value->name);
				av_prefs_save_recurse(node, name, file);
				*p = 0;
			}
		}
		else
		{
			/* save current prefs element */
			value = (av_prefs_value_p)node->item;
			fprintf(file, "%s = %s\n", name, value->svalue);
		}
	}
}

static av_result_t av_prefs_save(av_prefs_p self, const char* filename)
{
	char home_prefs[MAX_FILENAME_SIZE];
	av_prefs_context_p ctx = (av_prefs_context_p)O_context(self);
	av_tree_node_p node = ctx->tree;
	av_list_p children;
	FILE* file;
	if (!node)
		return AV_ESTATE;

	if (!filename)
	{
		av_prefs_get_home_prefs(home_prefs);
		filename = home_prefs;
	}
	printf("Saving %s\n", filename);

	file = fopen(filename, "w");
	if (!file)
		return AV_EWRITE;

	children = node->children;
	for (children->first(children); children->has_more(children); children->next(children))
	{
		av_prefs_value_p value;
		char keyname[MAX_NAME_SIZE];
		node = (av_tree_node_p)children->get(children);
		value = (av_prefs_value_p)node->item;
		strcpy(keyname, value->name);
		av_prefs_save_recurse(node, keyname, file);
	}
	fclose(file);
	return AV_OK;
}

static const char* av_prefs_get_prefs_file(av_prefs_p self)
{
	av_prefs_context_p ctx = (av_prefs_context_p)O_context(self);
	if (strlen(ctx->filename)>0)
	{
		return ctx->filename;
	}
	return AV_NULL;
}

static void av_prefs_load_preferences(av_prefs_p self)
{
	char home_prefs[MAX_FILENAME_SIZE];
	av_prefs_get_home_prefs(home_prefs);

	self->load(self, AV_CONFIG_INSTALL_PREFERENCES);
	self->load(self, home_prefs);
}

static void av_prefs_destroy_iterator(void* item)
{
	av_prefs_value_p value = (av_prefs_value_p)item;
	if (value)
	{
		free(value->name);
		if (value->svalue)
			free(value->svalue);
		free(value);
	}
}

static void av_prefs_destructor(void* pprefs)
{
	av_prefs_p self        = (av_prefs_p)pprefs;
	av_prefs_context_p ctx = (av_prefs_context_p)O_context(self);
	av_list_p observers = ctx->observers;

	while (observers->size(observers) > 0)
	{
		av_prefs_observe_info_p observer_info;
		observers->first(observers);
		observer_info = (av_prefs_observe_info_p)observers->get(observers);
		self->unregister_observer(self, observer_info->observer);
	}
	observers->destroy(observers);

	ctx->tree->traverse(ctx->tree, av_prefs_destroy_iterator);
	ctx->tree->destroy(ctx->tree);

	free(ctx);
}

static av_result_t av_prefs_constructor(av_object_p object)
{
	av_result_t rc;
	av_prefs_p self           = (av_prefs_p)object;
	av_prefs_context_p ctx    = (av_prefs_context_p)malloc(sizeof(av_prefs_context_t));
	if (!ctx)
		return AV_EMEM;

	ctx->filename[0] = '\0';

	if (AV_OK != (rc = av_tree_node_create(AV_NULL, &(ctx->tree))))
	{
		free(ctx);
		return rc;
	}
	ctx->tree->item           = AV_NULL;

	if (AV_OK != (rc = av_list_create(&(ctx->observers))))
	{
		ctx->tree->destroy(ctx->tree);
		free(ctx);
		return rc;
	}
	ctx->notify_observers     = AV_TRUE;
	O_set_attr(self, CONTEXT, ctx);
	self->set_int             = av_prefs_set_int;
	self->get_int             = av_prefs_get_int;
	self->set_string          = av_prefs_set_string;
	self->get_string          = av_prefs_get_string;
	self->register_observer   = av_prefs_register_observer;
	self->unregister_observer = av_prefs_unregister_observer;
	self->load                = av_prefs_load;
	self->save                = av_prefs_save;
	self->get_prefs_file      = av_prefs_get_prefs_file;

	av_prefs_load_preferences(self);
	return AV_OK;
}

/* Registers prefs class into TORBA */
av_result_t av_prefs_register_torba(void)
{
	av_service_p prefs;
	av_result_t rc;

	if (AV_OK != (rc = av_torb_register_class("prefs", "service",
											  sizeof(av_prefs_t),
											  av_prefs_constructor,
											  av_prefs_destructor)))
		return rc;

	if (AV_OK != (rc = av_torb_create_object("prefs", (av_object_p*)&prefs)))
		return rc;

	return av_torb_service_register("prefs", prefs);
}

