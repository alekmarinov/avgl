/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_tree.c                                          */
/* Description:   Tree                                               */
/*                                                                   */
/*********************************************************************/

#include <malloc.h>
#include <av_tree.h>

static void av_tree_traverse(av_tree_node_p node, void(*iterator)(void*))
{
	av_list_p children;
	if (node)
	{
		iterator(node->item);
		children = node->children;
		for (children->first(children); children->has_more(children); children->next(children))
			av_tree_traverse((av_tree_node_p)children->get(children), iterator);
	}
}

static void av_tree_node_detach(av_tree_node_p node)
{
	if (node && node->parent)
	{
		av_list_p children = node->parent->children;
		for (children->first(children); children->has_more(children); children->next(children))
		{
			if (node == (av_tree_node_p)children->get(children))
			{
				children->remove(children);
				return ;
			}
		}
	}
}

static void av_tree_node_destroy(av_tree_node_p node)
{
	if (node)
	{
		av_list_p children = node->children;
		for (children->first(children); children->has_more(children); children->next(children))
		{
			av_tree_node_p cnode = (av_tree_node_p)children->get(children);
			cnode->destroy(cnode);
		}

		/* back track */
		children->destroy(children);
		free(node);
	}
}

av_result_t av_tree_node_create(av_tree_node_p parent, av_tree_node_p *ppnode)
{
	av_result_t rc;
	av_tree_node_p node = (av_tree_node_p)malloc(sizeof(av_tree_node_t));
	if (!node)
		return AV_EMEM;

	if (AV_OK != (rc = av_list_create(&(node->children))))
	{
		free(node);
		return rc;
	}

	if (parent)
	{
		if (AV_OK != (rc = parent->children->push_last(parent->children, node)))
		{
			free(node);
			return rc;
		}
	}

	node->item     = AV_NULL;
	node->parent   = parent;
	node->traverse = av_tree_traverse;
	node->detach   = av_tree_node_detach;
	node->destroy  = av_tree_node_destroy;

	*ppnode = node;
	return AV_OK;
}

