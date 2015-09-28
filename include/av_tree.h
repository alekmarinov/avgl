/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_tree.h                                          */
/*                                                                   */
/*********************************************************************/

/*! \file av_tree.h
*   \brief av_tree definition
*/

#ifndef __AV_TREE_H
#define __AV_TREE_H

#include <av_list.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! 
* \brief tree
*
* Data structure representing tree
*/
typedef struct av_tree_node
{
	void* item;
	struct av_tree_node* parent;
	av_list_p children;
	void (*traverse)(struct av_tree_node*, void(*iterator)(void*));
	void (*detach)(struct av_tree_node*);
	void (*destroy)(struct av_tree_node*);
} av_tree_node_t, *av_tree_node_p;

AV_API av_result_t av_tree_node_create(av_tree_node_p parent, av_tree_node_p *ppnode);

#ifdef __cplusplus
}
#endif

#endif /* __AV_TREE_H */
