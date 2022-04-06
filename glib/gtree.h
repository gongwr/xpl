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

typedef struct _GTree  xtree_t;

/**
 * GTreeNode:
 *
 * An opaque type which identifies a specific node in a #xtree_t.
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
 * @data: user data passed to xtree_foreach_node()
 *
 * Specifies the type of function passed to xtree_foreach_node(). It is
 * passed each node, together with the @user_data parameter passed to
 * xtree_foreach_node(). If the function returns %TRUE, the traversal is
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
xtree_t*   xtree_new             (GCompareFunc      key_compare_func);
XPL_AVAILABLE_IN_ALL
xtree_t*   xtree_new_with_data   (GCompareDataFunc  key_compare_func,
                                 xpointer_t          key_compare_data);
XPL_AVAILABLE_IN_ALL
xtree_t*   xtree_new_full        (GCompareDataFunc  key_compare_func,
                                 xpointer_t          key_compare_data,
                                 xdestroy_notify_t    key_destroy_func,
                                 xdestroy_notify_t    value_destroy_func);
XPL_AVAILABLE_IN_2_68
GTreeNode *xtree_node_first (xtree_t *tree);
XPL_AVAILABLE_IN_2_68
GTreeNode *xtree_node_last (xtree_t *tree);
XPL_AVAILABLE_IN_2_68
GTreeNode *xtree_node_previous (GTreeNode *node);
XPL_AVAILABLE_IN_2_68
GTreeNode *xtree_node_next (GTreeNode *node);
XPL_AVAILABLE_IN_ALL
xtree_t*   xtree_ref             (xtree_t            *tree);
XPL_AVAILABLE_IN_ALL
void     xtree_unref           (xtree_t            *tree);
XPL_AVAILABLE_IN_ALL
void     xtree_destroy         (xtree_t            *tree);
XPL_AVAILABLE_IN_2_68
GTreeNode *xtree_insert_node (xtree_t *tree,
                               xpointer_t key,
                               xpointer_t value);
XPL_AVAILABLE_IN_ALL
void     xtree_insert          (xtree_t            *tree,
                                 xpointer_t          key,
                                 xpointer_t          value);
XPL_AVAILABLE_IN_2_68
GTreeNode *xtree_replace_node (xtree_t *tree,
                                xpointer_t key,
                                xpointer_t value);
XPL_AVAILABLE_IN_ALL
void     xtree_replace         (xtree_t            *tree,
                                 xpointer_t          key,
                                 xpointer_t          value);
XPL_AVAILABLE_IN_ALL
xboolean_t xtree_remove          (xtree_t            *tree,
                                 xconstpointer     key);

XPL_AVAILABLE_IN_2_70
void     xtree_remove_all      (xtree_t            *tree);

XPL_AVAILABLE_IN_ALL
xboolean_t xtree_steal           (xtree_t            *tree,
                                 xconstpointer     key);
XPL_AVAILABLE_IN_2_68
xpointer_t xtree_node_key (GTreeNode *node);
XPL_AVAILABLE_IN_2_68
xpointer_t xtree_node_value (GTreeNode *node);
XPL_AVAILABLE_IN_2_68
GTreeNode *xtree_lookup_node (xtree_t *tree,
                               xconstpointer key);
XPL_AVAILABLE_IN_ALL
xpointer_t xtree_lookup          (xtree_t            *tree,
                                 xconstpointer     key);
XPL_AVAILABLE_IN_ALL
xboolean_t xtree_lookup_extended (xtree_t            *tree,
                                 xconstpointer     lookup_key,
                                 xpointer_t         *orig_key,
                                 xpointer_t         *value);
XPL_AVAILABLE_IN_ALL
void     xtree_foreach         (xtree_t            *tree,
                                 GTraverseFunc	   func,
                                 xpointer_t	   user_data);
XPL_AVAILABLE_IN_2_68
void xtree_foreach_node (xtree_t *tree,
                          GTraverseNodeFunc func,
                          xpointer_t user_data);

XPL_DEPRECATED
void     xtree_traverse        (xtree_t            *tree,
                                 GTraverseFunc     traverse_func,
                                 GTraverseType     traverse_type,
                                 xpointer_t          user_data);

XPL_AVAILABLE_IN_2_68
GTreeNode *xtree_search_node (xtree_t *tree,
                               GCompareFunc search_func,
                               xconstpointer user_data);
XPL_AVAILABLE_IN_ALL
xpointer_t xtree_search          (xtree_t            *tree,
                                 GCompareFunc      search_func,
                                 xconstpointer     user_data);
XPL_AVAILABLE_IN_2_68
GTreeNode *xtree_lower_bound (xtree_t *tree,
                               xconstpointer key);
XPL_AVAILABLE_IN_2_68
GTreeNode *xtree_upper_bound (xtree_t *tree,
                               xconstpointer key);
XPL_AVAILABLE_IN_ALL
xint_t     xtree_height          (xtree_t            *tree);
XPL_AVAILABLE_IN_ALL
xint_t     xtree_nnodes          (xtree_t            *tree);

#ifdef G_TREE_DEBUG
/*< private >*/
#ifndef __GTK_DOC_IGNORE__
void xtree_dump (xtree_t *tree);
#endif  /* !__GTK_DOC_IGNORE__ */
#endif  /* G_TREE_DEBUG */

G_END_DECLS

#endif /* __G_TREE_H__ */
