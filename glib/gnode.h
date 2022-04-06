/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_NODE_H__
#define __G_NODE_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gmem.h>

G_BEGIN_DECLS

typedef struct _GNode		xnode_t;

/* Tree traverse flags */
typedef enum
{
  G_TRAVERSE_LEAVES     = 1 << 0,
  G_TRAVERSE_NON_LEAVES = 1 << 1,
  G_TRAVERSE_ALL        = G_TRAVERSE_LEAVES | G_TRAVERSE_NON_LEAVES,
  G_TRAVERSE_MASK       = 0x03,
  G_TRAVERSE_LEAFS      = G_TRAVERSE_LEAVES,
  G_TRAVERSE_NON_LEAFS  = G_TRAVERSE_NON_LEAVES
} GTraverseFlags;

/* Tree traverse orders */
typedef enum
{
  G_IN_ORDER,
  G_PRE_ORDER,
  G_POST_ORDER,
  G_LEVEL_ORDER
} GTraverseType;

typedef xboolean_t	(*GNodeTraverseFunc)	(xnode_t	       *node,
						 xpointer_t	data);
typedef void		(*GNodeForeachFunc)	(xnode_t	       *node,
						 xpointer_t	data);

/* N-way tree implementation
 */
struct _GNode
{
  xpointer_t data;
  xnode_t	  *next;
  xnode_t	  *prev;
  xnode_t	  *parent;
  xnode_t	  *children;
};

/**
 * G_NODE_IS_ROOT:
 * @node: a #xnode_t
 *
 * Returns %TRUE if a #xnode_t is the root of a tree.
 *
 * Returns: %TRUE if the #xnode_t is the root of a tree
 *     (i.e. it has no parent or siblings)
 */
#define	 G_NODE_IS_ROOT(node)	(((xnode_t*) (node))->parent == NULL && \
				 ((xnode_t*) (node))->prev == NULL && \
				 ((xnode_t*) (node))->next == NULL)

/**
 * G_NODE_IS_LEAF:
 * @node: a #xnode_t
 *
 * Returns %TRUE if a #xnode_t is a leaf node.
 *
 * Returns: %TRUE if the #xnode_t is a leaf node
 *     (i.e. it has no children)
 */
#define	 G_NODE_IS_LEAF(node)	(((xnode_t*) (node))->children == NULL)

XPL_AVAILABLE_IN_ALL
xnode_t*	 g_node_new		(xpointer_t	   data);
XPL_AVAILABLE_IN_ALL
void	 g_node_destroy		(xnode_t		  *root);
XPL_AVAILABLE_IN_ALL
void	 g_node_unlink		(xnode_t		  *node);
XPL_AVAILABLE_IN_ALL
xnode_t*   g_node_copy_deep       (xnode_t            *node,
				 GCopyFunc         copy_func,
				 xpointer_t          data);
XPL_AVAILABLE_IN_ALL
xnode_t*   g_node_copy            (xnode_t            *node);
XPL_AVAILABLE_IN_ALL
xnode_t*	 g_node_insert		(xnode_t		  *parent,
				 xint_t		   position,
				 xnode_t		  *node);
XPL_AVAILABLE_IN_ALL
xnode_t*	 g_node_insert_before	(xnode_t		  *parent,
				 xnode_t		  *sibling,
				 xnode_t		  *node);
XPL_AVAILABLE_IN_ALL
xnode_t*   g_node_insert_after    (xnode_t            *parent,
				 xnode_t            *sibling,
				 xnode_t            *node);
XPL_AVAILABLE_IN_ALL
xnode_t*	 g_node_prepend		(xnode_t		  *parent,
				 xnode_t		  *node);
XPL_AVAILABLE_IN_ALL
xuint_t	 g_node_n_nodes		(xnode_t		  *root,
				 GTraverseFlags	   flags);
XPL_AVAILABLE_IN_ALL
xnode_t*	 g_node_get_root	(xnode_t		  *node);
XPL_AVAILABLE_IN_ALL
xboolean_t g_node_is_ancestor	(xnode_t		  *node,
				 xnode_t		  *descendant);
XPL_AVAILABLE_IN_ALL
xuint_t	 g_node_depth		(xnode_t		  *node);
XPL_AVAILABLE_IN_ALL
xnode_t*	 g_node_find		(xnode_t		  *root,
				 GTraverseType	   order,
				 GTraverseFlags	   flags,
				 xpointer_t	   data);

/* convenience macros */
/**
 * g_node_append:
 * @parent: the #xnode_t to place the new #xnode_t under
 * @node: the #xnode_t to insert
 *
 * Inserts a #xnode_t as the last child of the given parent.
 *
 * Returns: the inserted #xnode_t
 */
#define g_node_append(parent, node)				\
     g_node_insert_before ((parent), NULL, (node))

/**
 * g_node_insert_data:
 * @parent: the #xnode_t to place the new #xnode_t under
 * @position: the position to place the new #xnode_t at. If position is -1,
 *     the new #xnode_t is inserted as the last child of @parent
 * @data: the data for the new #xnode_t
 *
 * Inserts a new #xnode_t at the given position.
 *
 * Returns: the new #xnode_t
 */
#define	g_node_insert_data(parent, position, data)		\
     g_node_insert ((parent), (position), g_node_new (data))

/**
 * g_node_insert_data_after:
 * @parent: the #xnode_t to place the new #xnode_t under
 * @sibling: the sibling #xnode_t to place the new #xnode_t after
 * @data: the data for the new #xnode_t
 *
 * Inserts a new #xnode_t after the given sibling.
 *
 * Returns: the new #xnode_t
 */

#define	g_node_insert_data_after(parent, sibling, data)	\
     g_node_insert_after ((parent), (sibling), g_node_new (data))
/**
 * g_node_insert_data_before:
 * @parent: the #xnode_t to place the new #xnode_t under
 * @sibling: the sibling #xnode_t to place the new #xnode_t before
 * @data: the data for the new #xnode_t
 *
 * Inserts a new #xnode_t before the given sibling.
 *
 * Returns: the new #xnode_t
 */
#define	g_node_insert_data_before(parent, sibling, data)	\
     g_node_insert_before ((parent), (sibling), g_node_new (data))

/**
 * g_node_prepend_data:
 * @parent: the #xnode_t to place the new #xnode_t under
 * @data: the data for the new #xnode_t
 *
 * Inserts a new #xnode_t as the first child of the given parent.
 *
 * Returns: the new #xnode_t
 */
#define	g_node_prepend_data(parent, data)			\
     g_node_prepend ((parent), g_node_new (data))

/**
 * g_node_append_data:
 * @parent: the #xnode_t to place the new #xnode_t under
 * @data: the data for the new #xnode_t
 *
 * Inserts a new #xnode_t as the last child of the given parent.
 *
 * Returns: the new #xnode_t
 */
#define	g_node_append_data(parent, data)			\
     g_node_insert_before ((parent), NULL, g_node_new (data))

/* traversal function, assumes that 'node' is root
 * (only traverses 'node' and its subtree).
 * this function is just a high level interface to
 * low level traversal functions, optimized for speed.
 */
XPL_AVAILABLE_IN_ALL
void	 g_node_traverse	(xnode_t		  *root,
				 GTraverseType	   order,
				 GTraverseFlags	   flags,
				 xint_t		   max_depth,
				 GNodeTraverseFunc func,
				 xpointer_t	   data);

/* return the maximum tree height starting with 'node', this is an expensive
 * operation, since we need to visit all nodes. this could be shortened by
 * adding 'xuint_t height' to struct _GNode, but then again, this is not very
 * often needed, and would make g_node_insert() more time consuming.
 */
XPL_AVAILABLE_IN_ALL
xuint_t	 g_node_max_height	 (xnode_t *root);

XPL_AVAILABLE_IN_ALL
void	 g_node_children_foreach (xnode_t		  *node,
				  GTraverseFlags   flags,
				  GNodeForeachFunc func,
				  xpointer_t	   data);
XPL_AVAILABLE_IN_ALL
void	 g_node_reverse_children (xnode_t		  *node);
XPL_AVAILABLE_IN_ALL
xuint_t	 g_node_n_children	 (xnode_t		  *node);
XPL_AVAILABLE_IN_ALL
xnode_t*	 g_node_nth_child	 (xnode_t		  *node,
				  xuint_t		   n);
XPL_AVAILABLE_IN_ALL
xnode_t*	 g_node_last_child	 (xnode_t		  *node);
XPL_AVAILABLE_IN_ALL
xnode_t*	 g_node_find_child	 (xnode_t		  *node,
				  GTraverseFlags   flags,
				  xpointer_t	   data);
XPL_AVAILABLE_IN_ALL
xint_t	 g_node_child_position	 (xnode_t		  *node,
				  xnode_t		  *child);
XPL_AVAILABLE_IN_ALL
xint_t	 g_node_child_index	 (xnode_t		  *node,
				  xpointer_t	   data);

XPL_AVAILABLE_IN_ALL
xnode_t*	 g_node_first_sibling	 (xnode_t		  *node);
XPL_AVAILABLE_IN_ALL
xnode_t*	 g_node_last_sibling	 (xnode_t		  *node);

/**
 * g_node_prev_sibling:
 * @node: a #xnode_t
 *
 * Gets the previous sibling of a #xnode_t.
 *
 * Returns: the previous sibling of @node, or %NULL if @node is the first
 *     node or %NULL
 */
#define	 g_node_prev_sibling(node)	((node) ? \
					 ((xnode_t*) (node))->prev : NULL)

/**
 * g_node_next_sibling:
 * @node: a #xnode_t
 *
 * Gets the next sibling of a #xnode_t.
 *
 * Returns: the next sibling of @node, or %NULL if @node is the last node
 *     or %NULL
 */
#define	 g_node_next_sibling(node)	((node) ? \
					 ((xnode_t*) (node))->next : NULL)

/**
 * g_node_first_child:
 * @node: a #xnode_t
 *
 * Gets the first child of a #xnode_t.
 *
 * Returns: the first child of @node, or %NULL if @node is %NULL
 *     or has no children
 */
#define	 g_node_first_child(node)	((node) ? \
					 ((xnode_t*) (node))->children : NULL)

G_END_DECLS

#endif /* __G_NODE_H__ */
