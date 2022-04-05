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

#ifndef __G_TREE_H__
#define __G_TREE_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gnode.h>

G_BEGIN_DECLS

#undef G_TREE_DEBUG

typedef struct _GTree  GTree;

/**
 * GTreeNode:
 *
 * An opaque type which identifies a specific node in a #GTree.
 *
 * Since: 2.68
 */
typedef struct _GTreeNode GTreeNode;

typedef xboolean_t (*GTraverseFunc) (xpointer_t  key,
                                   xpointer_t  value,
                                   xpointer_t  data);

/**
 * GTraverseNodeFunc:
 * @node: a #GTreeNode
 * @data: user data passed to g_tree_foreach_node()
 *
 * Specifies the type of function passed to g_tree_foreach_node(). It is
 * passed each node, together with the @user_data parameter passed to
 * g_tree_foreach_node(). If the function returns %TRUE, the traversal is
 * stopped.
 *
 * Returns: %TRUE to stop the traversal
 * Since: 2.68
 */
typedef xboolean_t (*GTraverseNodeFunc) (GTreeNode *node,
                                       xpointer_t   data);

/* Balanced binary trees
 */
XPL_AVAILABLE_IN_ALL
GTree*   g_tree_new             (GCompareFunc      key_compare_func);
XPL_AVAILABLE_IN_ALL
GTree*   g_tree_new_with_data   (GCompareDataFunc  key_compare_func,
                                 xpointer_t          key_compare_data);
XPL_AVAILABLE_IN_ALL
GTree*   g_tree_new_full        (GCompareDataFunc  key_compare_func,
                                 xpointer_t          key_compare_data,
                                 GDestroyNotify    key_destroy_func,
                                 GDestroyNotify    value_destroy_func);
XPL_AVAILABLE_IN_2_68
GTreeNode *g_tree_node_first (GTree *tree);
XPL_AVAILABLE_IN_2_68
GTreeNode *g_tree_node_last (GTree *tree);
XPL_AVAILABLE_IN_2_68
GTreeNode *g_tree_node_previous (GTreeNode *node);
XPL_AVAILABLE_IN_2_68
GTreeNode *g_tree_node_next (GTreeNode *node);
XPL_AVAILABLE_IN_ALL
GTree*   g_tree_ref             (GTree            *tree);
XPL_AVAILABLE_IN_ALL
void     g_tree_unref           (GTree            *tree);
XPL_AVAILABLE_IN_ALL
void     g_tree_destroy         (GTree            *tree);
XPL_AVAILABLE_IN_2_68
GTreeNode *g_tree_insert_node (GTree *tree,
                               xpointer_t key,
                               xpointer_t value);
XPL_AVAILABLE_IN_ALL
void     g_tree_insert          (GTree            *tree,
                                 xpointer_t          key,
                                 xpointer_t          value);
XPL_AVAILABLE_IN_2_68
GTreeNode *g_tree_replace_node (GTree *tree,
                                xpointer_t key,
                                xpointer_t value);
XPL_AVAILABLE_IN_ALL
void     g_tree_replace         (GTree            *tree,
                                 xpointer_t          key,
                                 xpointer_t          value);
XPL_AVAILABLE_IN_ALL
xboolean_t g_tree_remove          (GTree            *tree,
                                 gconstpointer     key);

XPL_AVAILABLE_IN_2_70
void     g_tree_remove_all      (GTree            *tree);

XPL_AVAILABLE_IN_ALL
xboolean_t g_tree_steal           (GTree            *tree,
                                 gconstpointer     key);
XPL_AVAILABLE_IN_2_68
xpointer_t g_tree_node_key (GTreeNode *node);
XPL_AVAILABLE_IN_2_68
xpointer_t g_tree_node_value (GTreeNode *node);
XPL_AVAILABLE_IN_2_68
GTreeNode *g_tree_lookup_node (GTree *tree,
                               gconstpointer key);
XPL_AVAILABLE_IN_ALL
xpointer_t g_tree_lookup          (GTree            *tree,
                                 gconstpointer     key);
XPL_AVAILABLE_IN_ALL
xboolean_t g_tree_lookup_extended (GTree            *tree,
                                 gconstpointer     lookup_key,
                                 xpointer_t         *orig_key,
                                 xpointer_t         *value);
XPL_AVAILABLE_IN_ALL
void     g_tree_foreach         (GTree            *tree,
                                 GTraverseFunc	   func,
                                 xpointer_t	   user_data);
XPL_AVAILABLE_IN_2_68
void g_tree_foreach_node (GTree *tree,
                          GTraverseNodeFunc func,
                          xpointer_t user_data);

XPL_DEPRECATED
void     g_tree_traverse        (GTree            *tree,
                                 GTraverseFunc     traverse_func,
                                 GTraverseType     traverse_type,
                                 xpointer_t          user_data);

XPL_AVAILABLE_IN_2_68
GTreeNode *g_tree_search_node (GTree *tree,
                               GCompareFunc search_func,
                               gconstpointer user_data);
XPL_AVAILABLE_IN_ALL
xpointer_t g_tree_search          (GTree            *tree,
                                 GCompareFunc      search_func,
                                 gconstpointer     user_data);
XPL_AVAILABLE_IN_2_68
GTreeNode *g_tree_lower_bound (GTree *tree,
                               gconstpointer key);
XPL_AVAILABLE_IN_2_68
GTreeNode *g_tree_upper_bound (GTree *tree,
                               gconstpointer key);
XPL_AVAILABLE_IN_ALL
xint_t     g_tree_height          (GTree            *tree);
XPL_AVAILABLE_IN_ALL
xint_t     g_tree_nnodes          (GTree            *tree);

#ifdef G_TREE_DEBUG
/*< private >*/
#ifndef __GTK_DOC_IGNORE__
void g_tree_dump (GTree *tree);
#endif  /* !__GTK_DOC_IGNORE__ */
#endif  /* G_TREE_DEBUG */

G_END_DECLS

#endif /* __G_TREE_H__ */
