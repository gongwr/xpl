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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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

/*
 * MT safe
 */

#include "config.h"

#include "gtree.h"

#include "gatomic.h"
#include "gtestutils.h"
#include "gslice.h"

/**
 * SECTION:trees-binary
 * @title: Balanced Binary Trees
 * @short_description: a sorted collection of key/value pairs optimized
 *                     for searching and traversing in order
 *
 * The #xtree_t structure and its associated functions provide a sorted
 * collection of key/value pairs optimized for searching and traversing
 * in order. This means that most of the operations  (access, search,
 * insertion, deletion, ...) on #xtree_t are O(log(n)) in average and O(n)
 * in worst case for time complexity. But, note that maintaining a
 * balanced sorted #xtree_t of n elements is done in time O(n log(n)).
 *
 * To create a new #xtree_t use xtree_new().
 *
 * To insert a key/value pair into a #xtree_t use xtree_insert()
 * (O(n log(n))).
 *
 * To remove a key/value pair use xtree_remove() (O(n log(n))).
 *
 * To look up the value corresponding to a given key, use
 * xtree_lookup() and xtree_lookup_extended().
 *
 * To find out the number of nodes in a #xtree_t, use xtree_nnodes(). To
 * get the height of a #xtree_t, use xtree_height().
 *
 * To traverse a #xtree_t, calling a function for each node visited in
 * the traversal, use xtree_foreach().
 *
 * To destroy a #xtree_t, use xtree_destroy().
 **/

#define MAX_GTREE_HEIGHT 40

/**
 * xtree_t:
 *
 * The xtree_t struct is an opaque data structure representing a
 * [balanced binary tree][glib-Balanced-Binary-Trees]. It should be
 * accessed only by using the following functions.
 */
struct _GTree
{
  GTreeNode        *root;
  GCompareDataFunc  key_compare;
  xdestroy_notify_t    key_destroy_func;
  xdestroy_notify_t    value_destroy_func;
  xpointer_t          key_compare_data;
  xuint_t             nnodes;
  xint_t              ref_count;
};

struct _GTreeNode
{
  xpointer_t   key;         /* key for this node */
  xpointer_t   value;       /* value stored at this node */
  GTreeNode *left;        /* left subtree */
  GTreeNode *right;       /* right subtree */
  gint8      balance;     /* height (right) - height (left) */
  xuint8_t     left_child;
  xuint8_t     right_child;
};


static GTreeNode* xtree_node_new                   (xpointer_t       key,
                                                     xpointer_t       value);
static GTreeNode *xtree_insert_internal (xtree_t *tree,
                                          xpointer_t key,
                                          xpointer_t value,
                                          xboolean_t replace);
static xboolean_t   xtree_remove_internal            (xtree_t         *tree,
                                                     xconstpointer  key,
                                                     xboolean_t       steal);
static GTreeNode* xtree_node_balance               (GTreeNode     *node);
static GTreeNode *xtree_find_node                  (xtree_t         *tree,
                                                     xconstpointer  key);
static xint_t       xtree_node_pre_order             (GTreeNode     *node,
                                                     GTraverseFunc  traverse_func,
                                                     xpointer_t       data);
static xint_t       xtree_node_in_order              (GTreeNode     *node,
                                                     GTraverseFunc  traverse_func,
                                                     xpointer_t       data);
static xint_t       xtree_node_post_order            (GTreeNode     *node,
                                                     GTraverseFunc  traverse_func,
                                                     xpointer_t       data);
static GTreeNode *xtree_node_search (GTreeNode *node,
                                      GCompareFunc search_func,
                                      xconstpointer data);
static GTreeNode* xtree_node_rotate_left           (GTreeNode     *node);
static GTreeNode* xtree_node_rotate_right          (GTreeNode     *node);
#ifdef G_TREE_DEBUG
static void       xtree_node_check                 (GTreeNode     *node);
#endif


static GTreeNode*
xtree_node_new (xpointer_t key,
                 xpointer_t value)
{
  GTreeNode *node = g_slice_new (GTreeNode);

  node->balance = 0;
  node->left = NULL;
  node->right = NULL;
  node->left_child = FALSE;
  node->right_child = FALSE;
  node->key = key;
  node->value = value;

  return node;
}

/**
 * xtree_new:
 * @key_compare_func: the function used to order the nodes in the #xtree_t.
 *   It should return values similar to the standard strcmp() function -
 *   0 if the two arguments are equal, a negative value if the first argument
 *   comes before the second, or a positive value if the first argument comes
 *   after the second.
 *
 * Creates a new #xtree_t.
 *
 * Returns: a newly allocated #xtree_t
 */
xtree_t *
xtree_new (GCompareFunc key_compare_func)
{
  xreturn_val_if_fail (key_compare_func != NULL, NULL);

  return xtree_new_full ((GCompareDataFunc) key_compare_func, NULL,
                          NULL, NULL);
}

/**
 * xtree_new_with_data:
 * @key_compare_func: qsort()-style comparison function
 * @key_compare_data: data to pass to comparison function
 *
 * Creates a new #xtree_t with a comparison function that accepts user data.
 * See xtree_new() for more details.
 *
 * Returns: a newly allocated #xtree_t
 */
xtree_t *
xtree_new_with_data (GCompareDataFunc key_compare_func,
                      xpointer_t         key_compare_data)
{
  xreturn_val_if_fail (key_compare_func != NULL, NULL);

  return xtree_new_full (key_compare_func, key_compare_data,
                          NULL, NULL);
}

/**
 * xtree_new_full:
 * @key_compare_func: qsort()-style comparison function
 * @key_compare_data: data to pass to comparison function
 * @key_destroy_func: a function to free the memory allocated for the key
 *   used when removing the entry from the #xtree_t or %NULL if you don't
 *   want to supply such a function
 * @value_destroy_func: a function to free the memory allocated for the
 *   value used when removing the entry from the #xtree_t or %NULL if you
 *   don't want to supply such a function
 *
 * Creates a new #xtree_t like xtree_new() and allows to specify functions
 * to free the memory allocated for the key and value that get called when
 * removing the entry from the #xtree_t.
 *
 * Returns: a newly allocated #xtree_t
 */
xtree_t *
xtree_new_full (GCompareDataFunc key_compare_func,
                 xpointer_t         key_compare_data,
                 xdestroy_notify_t   key_destroy_func,
                 xdestroy_notify_t   value_destroy_func)
{
  xtree_t *tree;

  xreturn_val_if_fail (key_compare_func != NULL, NULL);

  tree = g_slice_new (xtree_t);
  tree->root               = NULL;
  tree->key_compare        = key_compare_func;
  tree->key_destroy_func   = key_destroy_func;
  tree->value_destroy_func = value_destroy_func;
  tree->key_compare_data   = key_compare_data;
  tree->nnodes             = 0;
  tree->ref_count          = 1;

  return tree;
}

/**
 * xtree_node_first:
 * @tree: a #xtree_t
 *
 * Returns the first in-order node of the tree, or %NULL
 * for an empty tree.
 *
 * Returns: (nullable) (transfer none): the first node in the tree
 *
 * Since: 2.68
 */
GTreeNode *
xtree_node_first (xtree_t *tree)
{
  GTreeNode *tmp;

  xreturn_val_if_fail (tree != NULL, NULL);

  if (!tree->root)
    return NULL;

  tmp = tree->root;

  while (tmp->left_child)
    tmp = tmp->left;

  return tmp;
}

/**
 * xtree_node_last:
 * @tree: a #xtree_t
 *
 * Returns the last in-order node of the tree, or %NULL
 * for an empty tree.
 *
 * Returns: (nullable) (transfer none): the last node in the tree
 *
 * Since: 2.68
 */
GTreeNode *
xtree_node_last (xtree_t *tree)
{
  GTreeNode *tmp;

  xreturn_val_if_fail (tree != NULL, NULL);

  if (!tree->root)
    return NULL;

  tmp = tree->root;

  while (tmp->right_child)
    tmp = tmp->right;

  return tmp;
}

/**
 * xtree_node_previous
 * @node: a #xtree_t node
 *
 * Returns the previous in-order node of the tree, or %NULL
 * if the passed node was already the first one.
 *
 * Returns: (nullable) (transfer none): the previous node in the tree
 *
 * Since: 2.68
 */
GTreeNode *
xtree_node_previous (GTreeNode *node)
{
  GTreeNode *tmp;

  xreturn_val_if_fail (node != NULL, NULL);

  tmp = node->left;

  if (node->left_child)
    while (tmp->right_child)
      tmp = tmp->right;

  return tmp;
}

/**
 * xtree_node_next
 * @node: a #xtree_t node
 *
 * Returns the next in-order node of the tree, or %NULL
 * if the passed node was already the last one.
 *
 * Returns: (nullable) (transfer none): the next node in the tree
 *
 * Since: 2.68
 */
GTreeNode *
xtree_node_next (GTreeNode *node)
{
  GTreeNode *tmp;

  xreturn_val_if_fail (node != NULL, NULL);

  tmp = node->right;

  if (node->right_child)
    while (tmp->left_child)
      tmp = tmp->left;

  return tmp;
}

/**
 * xtree_remove_all:
 * @tree: a #xtree_t
 *
 * Removes all nodes from a #xtree_t and destroys their keys and values,
 * then resets the #xtree_t’s root to %NULL.
 *
 * Since: 2.70
 */
void
xtree_remove_all (xtree_t *tree)
{
  GTreeNode *node;
  GTreeNode *next;

  g_return_if_fail (tree != NULL);

  node = xtree_node_first (tree);

  while (node)
    {
      next = xtree_node_next (node);

      if (tree->key_destroy_func)
        tree->key_destroy_func (node->key);
      if (tree->value_destroy_func)
        tree->value_destroy_func (node->value);
      g_slice_free (GTreeNode, node);

#ifdef G_TREE_DEBUG
      xassert (tree->nnodes > 0);
      tree->nnodes--;
#endif

      node = next;
    }

#ifdef G_TREE_DEBUG
  xassert (tree->nnodes == 0);
#endif

  tree->root = NULL;
#ifndef G_TREE_DEBUG
  tree->nnodes = 0;
#endif
}

/**
 * xtree_ref:
 * @tree: a #xtree_t
 *
 * Increments the reference count of @tree by one.
 *
 * It is safe to call this function from any thread.
 *
 * Returns: the passed in #xtree_t
 *
 * Since: 2.22
 */
xtree_t *
xtree_ref (xtree_t *tree)
{
  xreturn_val_if_fail (tree != NULL, NULL);

  g_atomic_int_inc (&tree->ref_count);

  return tree;
}

/**
 * xtree_unref:
 * @tree: a #xtree_t
 *
 * Decrements the reference count of @tree by one.
 * If the reference count drops to 0, all keys and values will
 * be destroyed (if destroy functions were specified) and all
 * memory allocated by @tree will be released.
 *
 * It is safe to call this function from any thread.
 *
 * Since: 2.22
 */
void
xtree_unref (xtree_t *tree)
{
  g_return_if_fail (tree != NULL);

  if (g_atomic_int_dec_and_test (&tree->ref_count))
    {
      xtree_remove_all (tree);
      g_slice_free (xtree_t, tree);
    }
}

/**
 * xtree_destroy:
 * @tree: a #xtree_t
 *
 * Removes all keys and values from the #xtree_t and decreases its
 * reference count by one. If keys and/or values are dynamically
 * allocated, you should either free them first or create the #xtree_t
 * using xtree_new_full(). In the latter case the destroy functions
 * you supplied will be called on all keys and values before destroying
 * the #xtree_t.
 */
void
xtree_destroy (xtree_t *tree)
{
  g_return_if_fail (tree != NULL);

  xtree_remove_all (tree);
  xtree_unref (tree);
}

/**
 * xtree_insert_node:
 * @tree: a #xtree_t
 * @key: the key to insert
 * @value: the value corresponding to the key
 *
 * Inserts a key/value pair into a #xtree_t.
 *
 * If the given key already exists in the #xtree_t its corresponding value
 * is set to the new value. If you supplied a @value_destroy_func when
 * creating the #xtree_t, the old value is freed using that function. If
 * you supplied a @key_destroy_func when creating the #xtree_t, the passed
 * key is freed using that function.
 *
 * The tree is automatically 'balanced' as new key/value pairs are added,
 * so that the distance from the root to every leaf is as small as possible.
 * The cost of maintaining a balanced tree while inserting new key/value
 * result in a O(n log(n)) operation where most of the other operations
 * are O(log(n)).
 *
 * Returns: (transfer none): the inserted (or set) node.
 *
 * Since: 2.68
 */
GTreeNode *
xtree_insert_node (xtree_t    *tree,
                    xpointer_t  key,
                    xpointer_t  value)
{
  GTreeNode *node;

  xreturn_val_if_fail (tree != NULL, NULL);

  node = xtree_insert_internal (tree, key, value, FALSE);

#ifdef G_TREE_DEBUG
  xtree_node_check (tree->root);
#endif

  return node;
}

/**
 * xtree_insert:
 * @tree: a #xtree_t
 * @key: the key to insert
 * @value: the value corresponding to the key
 *
 * Inserts a key/value pair into a #xtree_t.
 *
 * Inserts a new key and value into a #xtree_t as xtree_insert_node() does,
 * only this function does not return the inserted or set node.
 */
void
xtree_insert (xtree_t    *tree,
               xpointer_t  key,
               xpointer_t  value)
{
  xtree_insert_node (tree, key, value);
}

/**
 * xtree_replace_node:
 * @tree: a #xtree_t
 * @key: the key to insert
 * @value: the value corresponding to the key
 *
 * Inserts a new key and value into a #xtree_t similar to xtree_insert_node().
 * The difference is that if the key already exists in the #xtree_t, it gets
 * replaced by the new key. If you supplied a @value_destroy_func when
 * creating the #xtree_t, the old value is freed using that function. If you
 * supplied a @key_destroy_func when creating the #xtree_t, the old key is
 * freed using that function.
 *
 * The tree is automatically 'balanced' as new key/value pairs are added,
 * so that the distance from the root to every leaf is as small as possible.
 *
 * Returns: (transfer none): the inserted (or set) node.
 *
 * Since: 2.68
 */
GTreeNode *
xtree_replace_node (xtree_t    *tree,
                     xpointer_t  key,
                     xpointer_t  value)
{
  GTreeNode *node;

  xreturn_val_if_fail (tree != NULL, NULL);

  node = xtree_insert_internal (tree, key, value, TRUE);

#ifdef G_TREE_DEBUG
  xtree_node_check (tree->root);
#endif

  return node;
}

/**
 * xtree_replace:
 * @tree: a #xtree_t
 * @key: the key to insert
 * @value: the value corresponding to the key
 *
 * Inserts a new key and value into a #xtree_t as xtree_replace_node() does,
 * only this function does not return the inserted or set node.
 */
void
xtree_replace (xtree_t    *tree,
                xpointer_t  key,
                xpointer_t  value)
{
  xtree_replace_node (tree, key, value);
}

/* internal insert routine */
static GTreeNode *
xtree_insert_internal (xtree_t    *tree,
                        xpointer_t  key,
                        xpointer_t  value,
                        xboolean_t  replace)
{
  GTreeNode *node, *retnode;
  GTreeNode *path[MAX_GTREE_HEIGHT];
  int idx;

  xreturn_val_if_fail (tree != NULL, NULL);

  if (!tree->root)
    {
      tree->root = xtree_node_new (key, value);
      tree->nnodes++;
      return tree->root;
    }

  idx = 0;
  path[idx++] = NULL;
  node = tree->root;

  while (1)
    {
      int cmp = tree->key_compare (key, node->key, tree->key_compare_data);

      if (cmp == 0)
        {
          if (tree->value_destroy_func)
            tree->value_destroy_func (node->value);

          node->value = value;

          if (replace)
            {
              if (tree->key_destroy_func)
                tree->key_destroy_func (node->key);

              node->key = key;
            }
          else
            {
              /* free the passed key */
              if (tree->key_destroy_func)
                tree->key_destroy_func (key);
            }

          return node;
        }
      else if (cmp < 0)
        {
          if (node->left_child)
            {
              path[idx++] = node;
              node = node->left;
            }
          else
            {
              GTreeNode *child = xtree_node_new (key, value);

              child->left = node->left;
              child->right = node;
              node->left = child;
              node->left_child = TRUE;
              node->balance -= 1;

              tree->nnodes++;

              retnode = child;
              break;
            }
        }
      else
        {
          if (node->right_child)
            {
              path[idx++] = node;
              node = node->right;
            }
          else
            {
              GTreeNode *child = xtree_node_new (key, value);

              child->right = node->right;
              child->left = node;
              node->right = child;
              node->right_child = TRUE;
              node->balance += 1;

              tree->nnodes++;

              retnode = child;
              break;
            }
        }
    }

  /* Restore balance. This is the goodness of a non-recursive
   * implementation, when we are done with balancing we 'break'
   * the loop and we are done.
   */
  while (1)
    {
      GTreeNode *bparent = path[--idx];
      xboolean_t left_node = (bparent && node == bparent->left);
      xassert (!bparent || bparent->left == node || bparent->right == node);

      if (node->balance < -1 || node->balance > 1)
        {
          node = xtree_node_balance (node);
          if (bparent == NULL)
            tree->root = node;
          else if (left_node)
            bparent->left = node;
          else
            bparent->right = node;
        }

      if (node->balance == 0 || bparent == NULL)
        break;

      if (left_node)
        bparent->balance -= 1;
      else
        bparent->balance += 1;

      node = bparent;
    }

  return retnode;
}

/**
 * xtree_remove:
 * @tree: a #xtree_t
 * @key: the key to remove
 *
 * Removes a key/value pair from a #xtree_t.
 *
 * If the #xtree_t was created using xtree_new_full(), the key and value
 * are freed using the supplied destroy functions, otherwise you have to
 * make sure that any dynamically allocated values are freed yourself.
 * If the key does not exist in the #xtree_t, the function does nothing.
 *
 * The cost of maintaining a balanced tree while removing a key/value
 * result in a O(n log(n)) operation where most of the other operations
 * are O(log(n)).
 *
 * Returns: %TRUE if the key was found (prior to 2.8, this function
 *     returned nothing)
 */
xboolean_t
xtree_remove (xtree_t         *tree,
               xconstpointer  key)
{
  xboolean_t removed;

  xreturn_val_if_fail (tree != NULL, FALSE);

  removed = xtree_remove_internal (tree, key, FALSE);

#ifdef G_TREE_DEBUG
  xtree_node_check (tree->root);
#endif

  return removed;
}

/**
 * xtree_steal:
 * @tree: a #xtree_t
 * @key: the key to remove
 *
 * Removes a key and its associated value from a #xtree_t without calling
 * the key and value destroy functions.
 *
 * If the key does not exist in the #xtree_t, the function does nothing.
 *
 * Returns: %TRUE if the key was found (prior to 2.8, this function
 *     returned nothing)
 */
xboolean_t
xtree_steal (xtree_t         *tree,
              xconstpointer  key)
{
  xboolean_t removed;

  xreturn_val_if_fail (tree != NULL, FALSE);

  removed = xtree_remove_internal (tree, key, TRUE);

#ifdef G_TREE_DEBUG
  xtree_node_check (tree->root);
#endif

  return removed;
}

/* internal remove routine */
static xboolean_t
xtree_remove_internal (xtree_t         *tree,
                        xconstpointer  key,
                        xboolean_t       steal)
{
  GTreeNode *node, *parent, *balance;
  GTreeNode *path[MAX_GTREE_HEIGHT];
  int idx;
  xboolean_t left_node;

  xreturn_val_if_fail (tree != NULL, FALSE);

  if (!tree->root)
    return FALSE;

  idx = 0;
  path[idx++] = NULL;
  node = tree->root;

  while (1)
    {
      int cmp = tree->key_compare (key, node->key, tree->key_compare_data);

      if (cmp == 0)
        break;
      else if (cmp < 0)
        {
          if (!node->left_child)
            return FALSE;

          path[idx++] = node;
          node = node->left;
        }
      else
        {
          if (!node->right_child)
            return FALSE;

          path[idx++] = node;
          node = node->right;
        }
    }

  /* The following code is almost equal to xtree_remove_node,
   * except that we do not have to call xtree_node_parent.
   */
  balance = parent = path[--idx];
  xassert (!parent || parent->left == node || parent->right == node);
  left_node = (parent && node == parent->left);

  if (!node->left_child)
    {
      if (!node->right_child)
        {
          if (!parent)
            tree->root = NULL;
          else if (left_node)
            {
              parent->left_child = FALSE;
              parent->left = node->left;
              parent->balance += 1;
            }
          else
            {
              parent->right_child = FALSE;
              parent->right = node->right;
              parent->balance -= 1;
            }
        }
      else /* node has a right child */
        {
          GTreeNode *tmp = xtree_node_next (node);
          tmp->left = node->left;

          if (!parent)
            tree->root = node->right;
          else if (left_node)
            {
              parent->left = node->right;
              parent->balance += 1;
            }
          else
            {
              parent->right = node->right;
              parent->balance -= 1;
            }
        }
    }
  else /* node has a left child */
    {
      if (!node->right_child)
        {
          GTreeNode *tmp = xtree_node_previous (node);
          tmp->right = node->right;

          if (parent == NULL)
            tree->root = node->left;
          else if (left_node)
            {
              parent->left = node->left;
              parent->balance += 1;
            }
          else
            {
              parent->right = node->left;
              parent->balance -= 1;
            }
        }
      else /* node has a both children (pant, pant!) */
        {
          GTreeNode *prev = node->left;
          GTreeNode *next = node->right;
          GTreeNode *nextp = node;
          int old_idx = idx + 1;
          idx++;

          /* path[idx] == parent */
          /* find the immediately next node (and its parent) */
          while (next->left_child)
            {
              path[++idx] = nextp = next;
              next = next->left;
            }

          path[old_idx] = next;
          balance = path[idx];

          /* remove 'next' from the tree */
          if (nextp != node)
            {
              if (next->right_child)
                nextp->left = next->right;
              else
                nextp->left_child = FALSE;
              nextp->balance += 1;

              next->right_child = TRUE;
              next->right = node->right;
            }
          else
            node->balance -= 1;

          /* set the prev to point to the right place */
          while (prev->right_child)
            prev = prev->right;
          prev->right = next;

          /* prepare 'next' to replace 'node' */
          next->left_child = TRUE;
          next->left = node->left;
          next->balance = node->balance;

          if (!parent)
            tree->root = next;
          else if (left_node)
            parent->left = next;
          else
            parent->right = next;
        }
    }

  /* restore balance */
  if (balance)
    while (1)
      {
        GTreeNode *bparent = path[--idx];
        xassert (!bparent || bparent->left == balance || bparent->right == balance);
        left_node = (bparent && balance == bparent->left);

        if(balance->balance < -1 || balance->balance > 1)
          {
            balance = xtree_node_balance (balance);
            if (!bparent)
              tree->root = balance;
            else if (left_node)
              bparent->left = balance;
            else
              bparent->right = balance;
          }

        if (balance->balance != 0 || !bparent)
          break;

        if (left_node)
          bparent->balance += 1;
        else
          bparent->balance -= 1;

        balance = bparent;
      }

  if (!steal)
    {
      if (tree->key_destroy_func)
        tree->key_destroy_func (node->key);
      if (tree->value_destroy_func)
        tree->value_destroy_func (node->value);
    }

  g_slice_free (GTreeNode, node);

  tree->nnodes--;

  return TRUE;
}

/**
 * xtree_node_key:
 * @node: a #xtree_t node
 *
 * Gets the key stored at a particular tree node.
 *
 * Returns: (nullable) (transfer none): the key at the node.
 *
 * Since: 2.68
 */
xpointer_t
xtree_node_key (GTreeNode *node)
{
  xreturn_val_if_fail (node != NULL, NULL);

  return node->key;
}

/**
 * xtree_node_value:
 * @node: a #xtree_t node
 *
 * Gets the value stored at a particular tree node.
 *
 * Returns: (nullable) (transfer none): the value at the node.
 *
 * Since: 2.68
 */
xpointer_t
xtree_node_value (GTreeNode *node)
{
  xreturn_val_if_fail (node != NULL, NULL);

  return node->value;
}

/**
 * xtree_lookup_node:
 * @tree: a #xtree_t
 * @key: the key to look up
 *
 * Gets the tree node corresponding to the given key. Since a #xtree_t is
 * automatically balanced as key/value pairs are added, key lookup
 * is O(log n) (where n is the number of key/value pairs in the tree).
 *
 * Returns: (nullable) (transfer none): the tree node corresponding to
 *          the key, or %NULL if the key was not found
 *
 * Since: 2.68
 */
GTreeNode *
xtree_lookup_node (xtree_t         *tree,
                    xconstpointer  key)
{
  xreturn_val_if_fail (tree != NULL, NULL);

  return xtree_find_node (tree, key);
}

/**
 * xtree_lookup:
 * @tree: a #xtree_t
 * @key: the key to look up
 *
 * Gets the value corresponding to the given key. Since a #xtree_t is
 * automatically balanced as key/value pairs are added, key lookup
 * is O(log n) (where n is the number of key/value pairs in the tree).
 *
 * Returns: the value corresponding to the key, or %NULL
 *     if the key was not found
 */
xpointer_t
xtree_lookup (xtree_t         *tree,
               xconstpointer  key)
{
  GTreeNode *node;

  node = xtree_lookup_node (tree, key);

  return node ? node->value : NULL;
}

/**
 * xtree_lookup_extended:
 * @tree: a #xtree_t
 * @lookup_key: the key to look up
 * @orig_key: (out) (optional) (nullable): returns the original key
 * @value: (out) (optional) (nullable): returns the value associated with the key
 *
 * Looks up a key in the #xtree_t, returning the original key and the
 * associated value. This is useful if you need to free the memory
 * allocated for the original key, for example before calling
 * xtree_remove().
 *
 * Returns: %TRUE if the key was found in the #xtree_t
 */
xboolean_t
xtree_lookup_extended (xtree_t         *tree,
                        xconstpointer  lookup_key,
                        xpointer_t      *orig_key,
                        xpointer_t      *value)
{
  GTreeNode *node;

  xreturn_val_if_fail (tree != NULL, FALSE);

  node = xtree_find_node (tree, lookup_key);

  if (node)
    {
      if (orig_key)
        *orig_key = node->key;
      if (value)
        *value = node->value;
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * xtree_foreach:
 * @tree: a #xtree_t
 * @func: the function to call for each node visited.
 *     If this function returns %TRUE, the traversal is stopped.
 * @user_data: user data to pass to the function
 *
 * Calls the given function for each of the key/value pairs in the #xtree_t.
 * The function is passed the key and value of each pair, and the given
 * @data parameter. The tree is traversed in sorted order.
 *
 * The tree may not be modified while iterating over it (you can't
 * add/remove items). To remove all items matching a predicate, you need
 * to add each item to a list in your #GTraverseFunc as you walk over
 * the tree, then walk the list and remove each item.
 */
void
xtree_foreach (xtree_t         *tree,
                GTraverseFunc  func,
                xpointer_t       user_data)
{
  GTreeNode *node;

  g_return_if_fail (tree != NULL);

  if (!tree->root)
    return;

  node = xtree_node_first (tree);

  while (node)
    {
      if ((*func) (node->key, node->value, user_data))
        break;

      node = xtree_node_next (node);
    }
}

/**
 * xtree_foreach_node:
 * @tree: a #xtree_t
 * @func: the function to call for each node visited.
 *     If this function returns %TRUE, the traversal is stopped.
 * @user_data: user data to pass to the function
 *
 * Calls the given function for each of the nodes in the #xtree_t.
 * The function is passed the pointer to the particular node, and the given
 * @data parameter. The tree traversal happens in-order.
 *
 * The tree may not be modified while iterating over it (you can't
 * add/remove items). To remove all items matching a predicate, you need
 * to add each item to a list in your #GTraverseFunc as you walk over
 * the tree, then walk the list and remove each item.
 *
 * Since: 2.68
 */
void
xtree_foreach_node (xtree_t             *tree,
                     GTraverseNodeFunc  func,
                     xpointer_t           user_data)
{
  GTreeNode *node;

  g_return_if_fail (tree != NULL);

  if (!tree->root)
    return;

  node = xtree_node_first (tree);

  while (node)
    {
      if ((*func) (node, user_data))
        break;

      node = xtree_node_next (node);
    }
}

/**
 * xtree_traverse:
 * @tree: a #xtree_t
 * @traverse_func: the function to call for each node visited. If this
 *   function returns %TRUE, the traversal is stopped.
 * @traverse_type: the order in which nodes are visited, one of %G_IN_ORDER,
 *   %G_PRE_ORDER and %G_POST_ORDER
 * @user_data: user data to pass to the function
 *
 * Calls the given function for each node in the #xtree_t.
 *
 * Deprecated:2.2: The order of a balanced tree is somewhat arbitrary.
 *     If you just want to visit all nodes in sorted order, use
 *     xtree_foreach() instead. If you really need to visit nodes in
 *     a different order, consider using an [n-ary tree][glib-N-ary-Trees].
 */
/**
 * GTraverseFunc:
 * @key: a key of a #xtree_t node
 * @value: the value corresponding to the key
 * @data: user data passed to xtree_traverse()
 *
 * Specifies the type of function passed to xtree_traverse(). It is
 * passed the key and value of each node, together with the @user_data
 * parameter passed to xtree_traverse(). If the function returns
 * %TRUE, the traversal is stopped.
 *
 * Returns: %TRUE to stop the traversal
 */
void
xtree_traverse (xtree_t         *tree,
                 GTraverseFunc  traverse_func,
                 GTraverseType  traverse_type,
                 xpointer_t       user_data)
{
  g_return_if_fail (tree != NULL);

  if (!tree->root)
    return;

  switch (traverse_type)
    {
    case G_PRE_ORDER:
      xtree_node_pre_order (tree->root, traverse_func, user_data);
      break;

    case G_IN_ORDER:
      xtree_node_in_order (tree->root, traverse_func, user_data);
      break;

    case G_POST_ORDER:
      xtree_node_post_order (tree->root, traverse_func, user_data);
      break;

    case G_LEVEL_ORDER:
      g_warning ("xtree_traverse(): traverse type G_LEVEL_ORDER isn't implemented.");
      break;
    }
}

/**
 * xtree_search_node:
 * @tree: a #xtree_t
 * @search_func: a function used to search the #xtree_t
 * @user_data: the data passed as the second argument to @search_func
 *
 * Searches a #xtree_t using @search_func.
 *
 * The @search_func is called with a pointer to the key of a key/value
 * pair in the tree, and the passed in @user_data. If @search_func returns
 * 0 for a key/value pair, then the corresponding node is returned as
 * the result of xtree_search(). If @search_func returns -1, searching
 * will proceed among the key/value pairs that have a smaller key; if
 * @search_func returns 1, searching will proceed among the key/value
 * pairs that have a larger key.
 *
 * Returns: (nullable) (transfer none): the node corresponding to the
 *          found key, or %NULL if the key was not found
 *
 * Since: 2.68
 */
GTreeNode *
xtree_search_node (xtree_t         *tree,
                    GCompareFunc   search_func,
                    xconstpointer  user_data)
{
  xreturn_val_if_fail (tree != NULL, NULL);

  if (!tree->root)
    return NULL;

  return xtree_node_search (tree->root, search_func, user_data);
}

/**
 * xtree_search:
 * @tree: a #xtree_t
 * @search_func: a function used to search the #xtree_t
 * @user_data: the data passed as the second argument to @search_func
 *
 * Searches a #xtree_t using @search_func.
 *
 * The @search_func is called with a pointer to the key of a key/value
 * pair in the tree, and the passed in @user_data. If @search_func returns
 * 0 for a key/value pair, then the corresponding value is returned as
 * the result of xtree_search(). If @search_func returns -1, searching
 * will proceed among the key/value pairs that have a smaller key; if
 * @search_func returns 1, searching will proceed among the key/value
 * pairs that have a larger key.
 *
 * Returns: the value corresponding to the found key, or %NULL
 *     if the key was not found
 */
xpointer_t
xtree_search (xtree_t         *tree,
               GCompareFunc   search_func,
               xconstpointer  user_data)
{
  GTreeNode *node;

  node = xtree_search_node (tree, search_func, user_data);

  return node ? node->value : NULL;
}

/**
 * xtree_lower_bound:
 * @tree: a #xtree_t
 * @key: the key to calculate the lower bound for
 *
 * Gets the lower bound node corresponding to the given key,
 * or %NULL if the tree is empty or all the nodes in the tree
 * have keys that are strictly lower than the searched key.
 *
 * The lower bound is the first node that has its key greater
 * than or equal to the searched key.
 *
 * Returns: (nullable) (transfer none): the tree node corresponding to
 *          the lower bound, or %NULL if the tree is empty or has only
 *          keys strictly lower than the searched key.
 *
 * Since: 2.68
 */
GTreeNode *
xtree_lower_bound (xtree_t         *tree,
                    xconstpointer  key)
{
  GTreeNode *node, *result;
  xint_t cmp;

  xreturn_val_if_fail (tree != NULL, NULL);

  node = tree->root;
  if (!node)
    return NULL;

  result = NULL;
  while (1)
    {
      cmp = tree->key_compare (key, node->key, tree->key_compare_data);
      if (cmp <= 0)
        {
          result = node;

          if (!node->left_child)
            return result;

          node = node->left;
        }
      else
        {
          if (!node->right_child)
            return result;

          node = node->right;
        }
    }
}

/**
 * xtree_upper_bound:
 * @tree: a #xtree_t
 * @key: the key to calculate the upper bound for
 *
 * Gets the upper bound node corresponding to the given key,
 * or %NULL if the tree is empty or all the nodes in the tree
 * have keys that are lower than or equal to the searched key.
 *
 * The upper bound is the first node that has its key strictly greater
 * than the searched key.
 *
 * Returns: (nullable) (transfer none): the tree node corresponding to the
 *          upper bound, or %NULL if the tree is empty or has only keys
 *          lower than or equal to the searched key.
 *
 * Since: 2.68
 */
GTreeNode *
xtree_upper_bound (xtree_t         *tree,
                    xconstpointer  key)
{
  GTreeNode *node, *result;
  xint_t cmp;

  xreturn_val_if_fail (tree != NULL, NULL);

  node = tree->root;
  if (!node)
    return NULL;

  result = NULL;
  while (1)
    {
      cmp = tree->key_compare (key, node->key, tree->key_compare_data);
      if (cmp < 0)
        {
          result = node;

          if (!node->left_child)
            return result;

          node = node->left;
        }
      else
        {
          if (!node->right_child)
            return result;

          node = node->right;
        }
    }
}

/**
 * xtree_height:
 * @tree: a #xtree_t
 *
 * Gets the height of a #xtree_t.
 *
 * If the #xtree_t contains no nodes, the height is 0.
 * If the #xtree_t contains only one root node the height is 1.
 * If the root node has children the height is 2, etc.
 *
 * Returns: the height of @tree
 */
xint_t
xtree_height (xtree_t *tree)
{
  GTreeNode *node;
  xint_t height;

  xreturn_val_if_fail (tree != NULL, 0);

  if (!tree->root)
    return 0;

  height = 0;
  node = tree->root;

  while (1)
    {
      height += 1 + MAX(node->balance, 0);

      if (!node->left_child)
        return height;

      node = node->left;
    }
}

/**
 * xtree_nnodes:
 * @tree: a #xtree_t
 *
 * Gets the number of nodes in a #xtree_t.
 *
 * Returns: the number of nodes in @tree
 */
xint_t
xtree_nnodes (xtree_t *tree)
{
  xreturn_val_if_fail (tree != NULL, 0);

  return tree->nnodes;
}

static GTreeNode *
xtree_node_balance (GTreeNode *node)
{
  if (node->balance < -1)
    {
      if (node->left->balance > 0)
        node->left = xtree_node_rotate_left (node->left);
      node = xtree_node_rotate_right (node);
    }
  else if (node->balance > 1)
    {
      if (node->right->balance < 0)
        node->right = xtree_node_rotate_right (node->right);
      node = xtree_node_rotate_left (node);
    }

  return node;
}

static GTreeNode *
xtree_find_node (xtree_t        *tree,
                  xconstpointer key)
{
  GTreeNode *node;
  xint_t cmp;

  node = tree->root;
  if (!node)
    return NULL;

  while (1)
    {
      cmp = tree->key_compare (key, node->key, tree->key_compare_data);
      if (cmp == 0)
        return node;
      else if (cmp < 0)
        {
          if (!node->left_child)
            return NULL;

          node = node->left;
        }
      else
        {
          if (!node->right_child)
            return NULL;

          node = node->right;
        }
    }
}

static xint_t
xtree_node_pre_order (GTreeNode     *node,
                       GTraverseFunc  traverse_func,
                       xpointer_t       data)
{
  if ((*traverse_func) (node->key, node->value, data))
    return TRUE;

  if (node->left_child)
    {
      if (xtree_node_pre_order (node->left, traverse_func, data))
        return TRUE;
    }

  if (node->right_child)
    {
      if (xtree_node_pre_order (node->right, traverse_func, data))
        return TRUE;
    }

  return FALSE;
}

static xint_t
xtree_node_in_order (GTreeNode     *node,
                      GTraverseFunc  traverse_func,
                      xpointer_t       data)
{
  if (node->left_child)
    {
      if (xtree_node_in_order (node->left, traverse_func, data))
        return TRUE;
    }

  if ((*traverse_func) (node->key, node->value, data))
    return TRUE;

  if (node->right_child)
    {
      if (xtree_node_in_order (node->right, traverse_func, data))
        return TRUE;
    }

  return FALSE;
}

static xint_t
xtree_node_post_order (GTreeNode     *node,
                        GTraverseFunc  traverse_func,
                        xpointer_t       data)
{
  if (node->left_child)
    {
      if (xtree_node_post_order (node->left, traverse_func, data))
        return TRUE;
    }

  if (node->right_child)
    {
      if (xtree_node_post_order (node->right, traverse_func, data))
        return TRUE;
    }

  if ((*traverse_func) (node->key, node->value, data))
    return TRUE;

  return FALSE;
}

static GTreeNode *
xtree_node_search (GTreeNode     *node,
                    GCompareFunc   search_func,
                    xconstpointer  data)
{
  xint_t dir;

  if (!node)
    return NULL;

  while (1)
    {
      dir = (* search_func) (node->key, data);
      if (dir == 0)
        return node;
      else if (dir < 0)
        {
          if (!node->left_child)
            return NULL;

          node = node->left;
        }
      else
        {
          if (!node->right_child)
            return NULL;

          node = node->right;
        }
    }
}

static GTreeNode *
xtree_node_rotate_left (GTreeNode *node)
{
  GTreeNode *right;
  xint_t a_bal;
  xint_t b_bal;

  right = node->right;

  if (right->left_child)
    node->right = right->left;
  else
    {
      node->right_child = FALSE;
      right->left_child = TRUE;
    }
  right->left = node;

  a_bal = node->balance;
  b_bal = right->balance;

  if (b_bal <= 0)
    {
      if (a_bal >= 1)
        right->balance = b_bal - 1;
      else
        right->balance = a_bal + b_bal - 2;
      node->balance = a_bal - 1;
    }
  else
    {
      if (a_bal <= b_bal)
        right->balance = a_bal - 2;
      else
        right->balance = b_bal - 1;
      node->balance = a_bal - b_bal - 1;
    }

  return right;
}

static GTreeNode *
xtree_node_rotate_right (GTreeNode *node)
{
  GTreeNode *left;
  xint_t a_bal;
  xint_t b_bal;

  left = node->left;

  if (left->right_child)
    node->left = left->right;
  else
    {
      node->left_child = FALSE;
      left->right_child = TRUE;
    }
  left->right = node;

  a_bal = node->balance;
  b_bal = left->balance;

  if (b_bal <= 0)
    {
      if (b_bal > a_bal)
        left->balance = b_bal + 1;
      else
        left->balance = a_bal + 2;
      node->balance = a_bal - b_bal + 1;
    }
  else
    {
      if (a_bal <= -1)
        left->balance = b_bal + 1;
      else
        left->balance = a_bal + b_bal + 2;
      node->balance = a_bal + 1;
    }

  return left;
}

#ifdef G_TREE_DEBUG
static xint_t
xtree_node_height (GTreeNode *node)
{
  xint_t left_height;
  xint_t right_height;

  if (node)
    {
      left_height = 0;
      right_height = 0;

      if (node->left_child)
        left_height = xtree_node_height (node->left);

      if (node->right_child)
        right_height = xtree_node_height (node->right);

      return MAX (left_height, right_height) + 1;
    }

  return 0;
}

static void
xtree_node_check (GTreeNode *node)
{
  xint_t left_height;
  xint_t right_height;
  xint_t balance;
  GTreeNode *tmp;

  if (node)
    {
      if (node->left_child)
        {
          tmp = xtree_node_previous (node);
          xassert (tmp->right == node);
        }

      if (node->right_child)
        {
          tmp = xtree_node_next (node);
          xassert (tmp->left == node);
        }

      left_height = 0;
      right_height = 0;

      if (node->left_child)
        left_height = xtree_node_height (node->left);
      if (node->right_child)
        right_height = xtree_node_height (node->right);

      balance = right_height - left_height;
      xassert (balance == node->balance);

      if (node->left_child)
        xtree_node_check (node->left);
      if (node->right_child)
        xtree_node_check (node->right);
    }
}

static void
xtree_node_dump (GTreeNode *node,
                  xint_t       indent)
{
  g_print ("%*s%c\n", indent, "", *(char *)node->key);

  if (node->left_child)
    {
      g_print ("%*sLEFT\n", indent, "");
      xtree_node_dump (node->left, indent + 2);
    }
  else if (node->left)
    g_print ("%*s<%c\n", indent + 2, "", *(char *)node->left->key);

  if (node->right_child)
    {
      g_print ("%*sRIGHT\n", indent, "");
      xtree_node_dump (node->right, indent + 2);
    }
  else if (node->right)
    g_print ("%*s>%c\n", indent + 2, "", *(char *)node->right->key);
}

void
xtree_dump (xtree_t *tree)
{
  if (tree->root)
    xtree_node_dump (tree->root, 0);
}
#endif
