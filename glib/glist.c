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

#include "glist.h"
#include "gslice.h"
#include "gmessages.h"

#include "gtestutils.h"

/**
 * SECTION:linked_lists_double
 * @title: Doubly-Linked Lists
 * @short_description: linked lists that can be iterated over in both directions
 *
 * The #xlist_t structure and its associated functions provide a standard
 * doubly-linked list data structure. The benefit of this data-structure
 * is to provide insertion/deletion operations in O(1) complexity where
 * access/search operations are in O(n). The benefit of #xlist_t over
 * #xslist_t (singly linked list) is that the worst case on access/search
 * operations is divided by two which comes at a cost in space as we need
 * to retain two pointers in place of one.
 *
 * Each element in the list contains a piece of data, together with
 * pointers which link to the previous and next elements in the list.
 * Using these pointers it is possible to move through the list in both
 * directions (unlike the singly-linked [xslist_t][glib-Singly-Linked-Lists],
 * which only allows movement through the list in the forward direction).
 *
 * The double linked list does not keep track of the number of items
 * and does not keep track of both the start and end of the list. If
 * you want fast access to both the start and the end of the list,
 * and/or the number of items in the list, use a
 * [xqueue_t][glib-Double-ended-Queues] instead.
 *
 * The data contained in each element can be either integer values, by
 * using one of the [Type Conversion Macros][glib-Type-Conversion-Macros],
 * or simply pointers to any type of data.
 *
 * List elements are allocated from the [slice allocator][glib-Memory-Slices],
 * which is more efficient than allocating elements individually.
 *
 * Note that most of the #xlist_t functions expect to be passed a pointer
 * to the first element in the list. The functions which insert
 * elements return the new start of the list, which may have changed.
 *
 * There is no function to create a #xlist_t. %NULL is considered to be
 * a valid, empty list so you simply set a #xlist_t* to %NULL to initialize
 * it.
 *
 * To add elements, use xlist_append(), xlist_prepend(),
 * xlist_insert() and xlist_insert_sorted().
 *
 * To visit all elements in the list, use a loop over the list:
 * |[<!-- language="C" -->
 * xlist_t *l;
 * for (l = list; l != NULL; l = l->next)
 *   {
 *     // do something with l->data
 *   }
 * ]|
 *
 * To call a function for each element in the list, use xlist_foreach().
 *
 * To loop over the list and modify it (e.g. remove a certain element)
 * a while loop is more appropriate, for example:
 * |[<!-- language="C" -->
 * xlist_t *l = list;
 * while (l != NULL)
 *   {
 *     xlist_t *next = l->next;
 *     if (should_be_removed (l))
 *       {
 *         // possibly free l->data
 *         list = xlist_delete_link (list, l);
 *       }
 *     l = next;
 *   }
 * ]|
 *
 * To remove elements, use xlist_remove().
 *
 * To navigate in a list, use xlist_first(), xlist_last(),
 * xlist_next(), xlist_previous().
 *
 * To find elements in the list use xlist_nth(), xlist_nth_data(),
 * xlist_find() and xlist_find_custom().
 *
 * To find the index of an element use xlist_position() and
 * xlist_index().
 *
 * To free the entire list, use xlist_free() or xlist_free_full().
 */

/**
 * xlist_t:
 * @data: holds the element's data, which can be a pointer to any kind
 *        of data, or any integer value using the
 *        [Type Conversion Macros][glib-Type-Conversion-Macros]
 * @next: contains the link to the next element in the list
 * @prev: contains the link to the previous element in the list
 *
 * The #xlist_t struct is used for each element in a doubly-linked list.
 **/

/**
 * xlist_previous:
 * @list: an element in a #xlist_t
 *
 * A convenience macro to get the previous element in a #xlist_t.
 * Note that it is considered perfectly acceptable to access
 * @list->prev directly.
 *
 * Returns: the previous element, or %NULL if there are no previous
 *          elements
 **/

/**
 * xlist_next:
 * @list: an element in a #xlist_t
 *
 * A convenience macro to get the next element in a #xlist_t.
 * Note that it is considered perfectly acceptable to access
 * @list->next directly.
 *
 * Returns: the next element, or %NULL if there are no more elements
 **/

#define _xlist_alloc()         g_slice_new (xlist_t)
#define _xlist_alloc0()        g_slice_new0 (xlist_t)
#define _xlist_free1(list)     g_slice_free (xlist_t, list)

/**
 * xlist_alloc:
 *
 * Allocates space for one #xlist_t element. It is called by
 * xlist_append(), xlist_prepend(), xlist_insert() and
 * xlist_insert_sorted() and so is rarely used on its own.
 *
 * Returns: a pointer to the newly-allocated #xlist_t element
 **/
xlist_t *
xlist_alloc (void)
{
  return _xlist_alloc0 ();
}

/**
 * xlist_free:
 * @list: the first link of a #xlist_t
 *
 * Frees all of the memory used by a #xlist_t.
 * The freed elements are returned to the slice allocator.
 *
 * If list elements contain dynamically-allocated memory, you should
 * either use xlist_free_full() or free them manually first.
 *
 * It can be combined with g_steal_pointer() to ensure the list head pointer
 * is not left dangling:
 * |[<!-- language="C" -->
 * xlist_t *list_of_borrowed_things = ???;  /<!-- -->* (transfer container) *<!-- -->/
 * xlist_free (g_steal_pointer (&list_of_borrowed_things));
 * ]|
 */
void
xlist_free (xlist_t *list)
{
  g_slice_free_chain (xlist_t, list, next);
}

/**
 * xlist_free_1:
 * @list: a #xlist_t element
 *
 * Frees one #xlist_t element, but does not update links from the next and
 * previous elements in the list, so you should not call this function on an
 * element that is currently part of a list.
 *
 * It is usually used after xlist_remove_link().
 */
/**
 * xlist_free1:
 *
 * Another name for xlist_free_1().
 **/
void
xlist_free_1 (xlist_t *list)
{
  _xlist_free1 (list);
}

/**
 * xlist_free_full:
 * @list: the first link of a #xlist_t
 * @free_func: the function to be called to free each element's data
 *
 * Convenience method, which frees all the memory used by a #xlist_t,
 * and calls @free_func on every element's data.
 *
 * @free_func must not modify the list (eg, by removing the freed
 * element from it).
 *
 * It can be combined with g_steal_pointer() to ensure the list head pointer
 * is not left dangling ????? this also has the nice property that the head pointer
 * is cleared before any of the list elements are freed, to prevent double frees
 * from @free_func:
 * |[<!-- language="C" -->
 * xlist_t *list_of_owned_things = ???;  /<!-- -->* (transfer full) (element-type xobject_t) *<!-- -->/
 * xlist_free_full (g_steal_pointer (&list_of_owned_things), xobject_unref);
 * ]|
 *
 * Since: 2.28
 */
void
xlist_free_full (xlist_t          *list,
                  xdestroy_notify_t  free_func)
{
  xlist_foreach (list, (GFunc) free_func, NULL);
  xlist_free (list);
}

/**
 * xlist_append:
 * @list: a pointer to a #xlist_t
 * @data: the data for the new element
 *
 * Adds a new element on to the end of the list.
 *
 * Note that the return value is the new start of the list,
 * if @list was empty; make sure you store the new value.
 *
 * xlist_append() has to traverse the entire list to find the end,
 * which is inefficient when adding multiple elements. A common idiom
 * to avoid the inefficiency is to use xlist_prepend() and reverse
 * the list with xlist_reverse() when all elements have been added.
 *
 * |[<!-- language="C" -->
 * // Notice that these are initialized to the empty list.
 * xlist_t *string_list = NULL, *number_list = NULL;
 *
 * // This is a list of strings.
 * string_list = xlist_append (string_list, "first");
 * string_list = xlist_append (string_list, "second");
 *
 * // This is a list of integers.
 * number_list = xlist_append (number_list, GINT_TO_POINTER (27));
 * number_list = xlist_append (number_list, GINT_TO_POINTER (14));
 * ]|
 *
 * Returns: either @list or the new start of the #xlist_t if @list was %NULL
 */
xlist_t *
xlist_append (xlist_t    *list,
               xpointer_t  data)
{
  xlist_t *new_list;
  xlist_t *last;

  new_list = _xlist_alloc ();
  new_list->data = data;
  new_list->next = NULL;

  if (list)
    {
      last = xlist_last (list);
      /* xassert (last != NULL); */
      last->next = new_list;
      new_list->prev = last;

      return list;
    }
  else
    {
      new_list->prev = NULL;
      return new_list;
    }
}

/**
 * xlist_prepend:
 * @list: a pointer to a #xlist_t, this must point to the top of the list
 * @data: the data for the new element
 *
 * Prepends a new element on to the start of the list.
 *
 * Note that the return value is the new start of the list,
 * which will have changed, so make sure you store the new value.
 *
 * |[<!-- language="C" -->
 * // Notice that it is initialized to the empty list.
 * xlist_t *list = NULL;
 *
 * list = xlist_prepend (list, "last");
 * list = xlist_prepend (list, "first");
 * ]|
 *
 * Do not use this function to prepend a new element to a different
 * element than the start of the list. Use xlist_insert_before() instead.
 *
 * Returns: a pointer to the newly prepended element, which is the new
 *     start of the #xlist_t
 */
xlist_t *
xlist_prepend (xlist_t    *list,
                xpointer_t  data)
{
  xlist_t *new_list;

  new_list = _xlist_alloc ();
  new_list->data = data;
  new_list->next = list;

  if (list)
    {
      new_list->prev = list->prev;
      if (list->prev)
        list->prev->next = new_list;
      list->prev = new_list;
    }
  else
    new_list->prev = NULL;

  return new_list;
}

/**
 * xlist_insert:
 * @list: a pointer to a #xlist_t, this must point to the top of the list
 * @data: the data for the new element
 * @position: the position to insert the element. If this is
 *     negative, or is larger than the number of elements in the
 *     list, the new element is added on to the end of the list.
 *
 * Inserts a new element into the list at the given position.
 *
 * Returns: the (possibly changed) start of the #xlist_t
 */
xlist_t *
xlist_insert (xlist_t    *list,
               xpointer_t  data,
               xint_t      position)
{
  xlist_t *new_list;
  xlist_t *tmp_list;

  if (position < 0)
    return xlist_append (list, data);
  else if (position == 0)
    return xlist_prepend (list, data);

  tmp_list = xlist_nth (list, position);
  if (!tmp_list)
    return xlist_append (list, data);

  new_list = _xlist_alloc ();
  new_list->data = data;
  new_list->prev = tmp_list->prev;
  tmp_list->prev->next = new_list;
  new_list->next = tmp_list;
  tmp_list->prev = new_list;

  return list;
}

/**
 * xlist_insert_before_link:
 * @list: a pointer to a #xlist_t, this must point to the top of the list
 * @sibling: (nullable): the list element before which the new element
 *     is inserted or %NULL to insert at the end of the list
 * @link_: the list element to be added, which must not be part of
 *     any other list
 *
 * Inserts @link_ into the list before the given position.
 *
 * Returns: the (possibly changed) start of the #xlist_t
 *
 * Since: 2.62
 */
xlist_t *
xlist_insert_before_link (xlist_t *list,
                           xlist_t *sibling,
                           xlist_t *link_)
{
  xreturn_val_if_fail (link_ != NULL, list);
  xreturn_val_if_fail (link_->prev == NULL, list);
  xreturn_val_if_fail (link_->next == NULL, list);

  if (list == NULL)
    {
      xreturn_val_if_fail (sibling == NULL, list);
      return link_;
    }
  else if (sibling != NULL)
    {
      link_->prev = sibling->prev;
      link_->next = sibling;
      sibling->prev = link_;
      if (link_->prev != NULL)
        {
          link_->prev->next = link_;
          return list;
        }
      else
        {
          xreturn_val_if_fail (sibling == list, link_);
          return link_;
        }
    }
  else
    {
      xlist_t *last;

      for (last = list; last->next != NULL; last = last->next) {}

      last->next = link_;
      last->next->prev = last;
      last->next->next = NULL;

      return list;
    }
}

/**
 * xlist_insert_before:
 * @list: a pointer to a #xlist_t, this must point to the top of the list
 * @sibling: the list element before which the new element
 *     is inserted or %NULL to insert at the end of the list
 * @data: the data for the new element
 *
 * Inserts a new element into the list before the given position.
 *
 * Returns: the (possibly changed) start of the #xlist_t
 */
xlist_t *
xlist_insert_before (xlist_t    *list,
                      xlist_t    *sibling,
                      xpointer_t  data)
{
  if (list == NULL)
    {
      list = xlist_alloc ();
      list->data = data;
      xreturn_val_if_fail (sibling == NULL, list);
      return list;
    }
  else if (sibling != NULL)
    {
      xlist_t *node;

      node = _xlist_alloc ();
      node->data = data;
      node->prev = sibling->prev;
      node->next = sibling;
      sibling->prev = node;
      if (node->prev != NULL)
        {
          node->prev->next = node;
          return list;
        }
      else
        {
          xreturn_val_if_fail (sibling == list, node);
          return node;
        }
    }
  else
    {
      xlist_t *last;

      for (last = list; last->next != NULL; last = last->next) {}

      last->next = _xlist_alloc ();
      last->next->data = data;
      last->next->prev = last;
      last->next->next = NULL;

      return list;
    }
}

/**
 * xlist_concat:
 * @list1: a #xlist_t, this must point to the top of the list
 * @list2: the #xlist_t to add to the end of the first #xlist_t,
 *     this must point  to the top of the list
 *
 * Adds the second #xlist_t onto the end of the first #xlist_t.
 * Note that the elements of the second #xlist_t are not copied.
 * They are used directly.
 *
 * This function is for example used to move an element in the list.
 * The following example moves an element to the top of the list:
 * |[<!-- language="C" -->
 * list = xlist_remove_link (list, llink);
 * list = xlist_concat (llink, list);
 * ]|
 *
 * Returns: the start of the new #xlist_t, which equals @list1 if not %NULL
 */
xlist_t *
xlist_concat (xlist_t *list1,
               xlist_t *list2)
{
  xlist_t *tmp_list;

  if (list2)
    {
      tmp_list = xlist_last (list1);
      if (tmp_list)
        tmp_list->next = list2;
      else
        list1 = list2;
      list2->prev = tmp_list;
    }

  return list1;
}

static inline xlist_t *
_xlist_remove_link (xlist_t *list,
                     xlist_t *link)
{
  if (link == NULL)
    return list;

  if (link->prev)
    {
      if (link->prev->next == link)
        link->prev->next = link->next;
      else
        g_warning ("corrupted double-linked list detected");
    }
  if (link->next)
    {
      if (link->next->prev == link)
        link->next->prev = link->prev;
      else
        g_warning ("corrupted double-linked list detected");
    }

  if (link == list)
    list = list->next;

  link->next = NULL;
  link->prev = NULL;

  return list;
}

/**
 * xlist_remove:
 * @list: a #xlist_t, this must point to the top of the list
 * @data: the data of the element to remove
 *
 * Removes an element from a #xlist_t.
 * If two elements contain the same data, only the first is removed.
 * If none of the elements contain the data, the #xlist_t is unchanged.
 *
 * Returns: the (possibly changed) start of the #xlist_t
 */
xlist_t *
xlist_remove (xlist_t         *list,
               xconstpointer  data)
{
  xlist_t *tmp;

  tmp = list;
  while (tmp)
    {
      if (tmp->data != data)
        tmp = tmp->next;
      else
        {
          list = _xlist_remove_link (list, tmp);
          _xlist_free1 (tmp);

          break;
        }
    }
  return list;
}

/**
 * xlist_remove_all:
 * @list: a #xlist_t, this must point to the top of the list
 * @data: data to remove
 *
 * Removes all list nodes with data equal to @data.
 * Returns the new head of the list. Contrast with
 * xlist_remove() which removes only the first node
 * matching the given data.
 *
 * Returns: the (possibly changed) start of the #xlist_t
 */
xlist_t *
xlist_remove_all (xlist_t         *list,
                   xconstpointer  data)
{
  xlist_t *tmp = list;

  while (tmp)
    {
      if (tmp->data != data)
        tmp = tmp->next;
      else
        {
          xlist_t *next = tmp->next;

          if (tmp->prev)
            tmp->prev->next = next;
          else
            list = next;
          if (next)
            next->prev = tmp->prev;

          _xlist_free1 (tmp);
          tmp = next;
        }
    }
  return list;
}

/**
 * xlist_remove_link:
 * @list: a #xlist_t, this must point to the top of the list
 * @llink: an element in the #xlist_t
 *
 * Removes an element from a #xlist_t, without freeing the element.
 * The removed element's prev and next links are set to %NULL, so
 * that it becomes a self-contained list with one element.
 *
 * This function is for example used to move an element in the list
 * (see the example for xlist_concat()) or to remove an element in
 * the list before freeing its data:
 * |[<!-- language="C" -->
 * list = xlist_remove_link (list, llink);
 * free_some_data_that_may_access_the_list_again (llink->data);
 * xlist_free (llink);
 * ]|
 *
 * Returns: the (possibly changed) start of the #xlist_t
 */
xlist_t *
xlist_remove_link (xlist_t *list,
                    xlist_t *llink)
{
  return _xlist_remove_link (list, llink);
}

/**
 * xlist_delete_link:
 * @list: a #xlist_t, this must point to the top of the list
 * @link_: node to delete from @list
 *
 * Removes the node link_ from the list and frees it.
 * Compare this to xlist_remove_link() which removes the node
 * without freeing it.
 *
 * Returns: the (possibly changed) start of the #xlist_t
 */
xlist_t *
xlist_delete_link (xlist_t *list,
                    xlist_t *link_)
{
  list = _xlist_remove_link (list, link_);
  _xlist_free1 (link_);

  return list;
}

/**
 * xlist_copy:
 * @list: a #xlist_t, this must point to the top of the list
 *
 * Copies a #xlist_t.
 *
 * Note that this is a "shallow" copy. If the list elements
 * consist of pointers to data, the pointers are copied but
 * the actual data is not. See xlist_copy_deep() if you need
 * to copy the data as well.
 *
 * Returns: the start of the new list that holds the same data as @list
 */
xlist_t *
xlist_copy (xlist_t *list)
{
  return xlist_copy_deep (list, NULL, NULL);
}

/**
 * xlist_copy_deep:
 * @list: a #xlist_t, this must point to the top of the list
 * @func: a copy function used to copy every element in the list
 * @user_data: user data passed to the copy function @func, or %NULL
 *
 * Makes a full (deep) copy of a #xlist_t.
 *
 * In contrast with xlist_copy(), this function uses @func to make
 * a copy of each list element, in addition to copying the list
 * container itself.
 *
 * @func, as a #GCopyFunc, takes two arguments, the data to be copied
 * and a @user_data pointer. On common processor architectures, it's safe to
 * pass %NULL as @user_data if the copy function takes only one argument. You
 * may get compiler warnings from this though if compiling with GCC???s
 * `-Wcast-function-type` warning.
 *
 * For instance, if @list holds a list of GObjects, you can do:
 * |[<!-- language="C" -->
 * another_list = xlist_copy_deep (list, (GCopyFunc) xobject_ref, NULL);
 * ]|
 *
 * And, to entirely free the new list, you could do:
 * |[<!-- language="C" -->
 * xlist_free_full (another_list, xobject_unref);
 * ]|
 *
 * Returns: the start of the new list that holds a full copy of @list,
 *     use xlist_free_full() to free it
 *
 * Since: 2.34
 */
xlist_t *
xlist_copy_deep (xlist_t     *list,
                  GCopyFunc  func,
                  xpointer_t   user_data)
{
  xlist_t *new_list = NULL;

  if (list)
    {
      xlist_t *last;

      new_list = _xlist_alloc ();
      if (func)
        new_list->data = func (list->data, user_data);
      else
        new_list->data = list->data;
      new_list->prev = NULL;
      last = new_list;
      list = list->next;
      while (list)
        {
          last->next = _xlist_alloc ();
          last->next->prev = last;
          last = last->next;
          if (func)
            last->data = func (list->data, user_data);
          else
            last->data = list->data;
          list = list->next;
        }
      last->next = NULL;
    }

  return new_list;
}

/**
 * xlist_reverse:
 * @list: a #xlist_t, this must point to the top of the list
 *
 * Reverses a #xlist_t.
 * It simply switches the next and prev pointers of each element.
 *
 * Returns: the start of the reversed #xlist_t
 */
xlist_t *
xlist_reverse (xlist_t *list)
{
  xlist_t *last;

  last = NULL;
  while (list)
    {
      last = list;
      list = last->next;
      last->next = last->prev;
      last->prev = list;
    }

  return last;
}

/**
 * xlist_nth:
 * @list: a #xlist_t, this must point to the top of the list
 * @n: the position of the element, counting from 0
 *
 * Gets the element at the given position in a #xlist_t.
 *
 * This iterates over the list until it reaches the @n-th position. If you
 * intend to iterate over every element, it is better to use a for-loop as
 * described in the #xlist_t introduction.
 *
 * Returns: the element, or %NULL if the position is off
 *     the end of the #xlist_t
 */
xlist_t *
xlist_nth (xlist_t *list,
            xuint_t  n)
{
  while ((n-- > 0) && list)
    list = list->next;

  return list;
}

/**
 * xlist_nth_prev:
 * @list: a #xlist_t
 * @n: the position of the element, counting from 0
 *
 * Gets the element @n places before @list.
 *
 * Returns: the element, or %NULL if the position is
 *     off the end of the #xlist_t
 */
xlist_t *
xlist_nth_prev (xlist_t *list,
                 xuint_t  n)
{
  while ((n-- > 0) && list)
    list = list->prev;

  return list;
}

/**
 * xlist_nth_data:
 * @list: a #xlist_t, this must point to the top of the list
 * @n: the position of the element
 *
 * Gets the data of the element at the given position.
 *
 * This iterates over the list until it reaches the @n-th position. If you
 * intend to iterate over every element, it is better to use a for-loop as
 * described in the #xlist_t introduction.
 *
 * Returns: the element's data, or %NULL if the position
 *     is off the end of the #xlist_t
 */
xpointer_t
xlist_nth_data (xlist_t *list,
                 xuint_t  n)
{
  while ((n-- > 0) && list)
    list = list->next;

  return list ? list->data : NULL;
}

/**
 * xlist_find:
 * @list: a #xlist_t, this must point to the top of the list
 * @data: the element data to find
 *
 * Finds the element in a #xlist_t which contains the given data.
 *
 * Returns: the found #xlist_t element, or %NULL if it is not found
 */
xlist_t *
xlist_find (xlist_t         *list,
             xconstpointer  data)
{
  while (list)
    {
      if (list->data == data)
        break;
      list = list->next;
    }

  return list;
}

/**
 * xlist_find_custom:
 * @list: a #xlist_t, this must point to the top of the list
 * @data: user data passed to the function
 * @func: the function to call for each element.
 *     It should return 0 when the desired element is found
 *
 * Finds an element in a #xlist_t, using a supplied function to
 * find the desired element. It iterates over the list, calling
 * the given function which should return 0 when the desired
 * element is found. The function takes two #xconstpointer arguments,
 * the #xlist_t element's data as the first argument and the
 * given user data.
 *
 * Returns: the found #xlist_t element, or %NULL if it is not found
 */
xlist_t *
xlist_find_custom (xlist_t         *list,
                    xconstpointer  data,
                    GCompareFunc   func)
{
  xreturn_val_if_fail (func != NULL, list);

  while (list)
    {
      if (! func (list->data, data))
        return list;
      list = list->next;
    }

  return NULL;
}

/**
 * xlist_position:
 * @list: a #xlist_t, this must point to the top of the list
 * @llink: an element in the #xlist_t
 *
 * Gets the position of the given element
 * in the #xlist_t (starting from 0).
 *
 * Returns: the position of the element in the #xlist_t,
 *     or -1 if the element is not found
 */
xint_t
xlist_position (xlist_t *list,
                 xlist_t *llink)
{
  xint_t i;

  i = 0;
  while (list)
    {
      if (list == llink)
        return i;
      i++;
      list = list->next;
    }

  return -1;
}

/**
 * xlist_index:
 * @list: a #xlist_t, this must point to the top of the list
 * @data: the data to find
 *
 * Gets the position of the element containing
 * the given data (starting from 0).
 *
 * Returns: the index of the element containing the data,
 *     or -1 if the data is not found
 */
xint_t
xlist_index (xlist_t         *list,
              xconstpointer  data)
{
  xint_t i;

  i = 0;
  while (list)
    {
      if (list->data == data)
        return i;
      i++;
      list = list->next;
    }

  return -1;
}

/**
 * xlist_last:
 * @list: any #xlist_t element
 *
 * Gets the last element in a #xlist_t.
 *
 * Returns: the last element in the #xlist_t,
 *     or %NULL if the #xlist_t has no elements
 */
xlist_t *
xlist_last (xlist_t *list)
{
  if (list)
    {
      while (list->next)
        list = list->next;
    }

  return list;
}

/**
 * xlist_first:
 * @list: any #xlist_t element
 *
 * Gets the first element in a #xlist_t.
 *
 * Returns: the first element in the #xlist_t,
 *     or %NULL if the #xlist_t has no elements
 */
xlist_t *
xlist_first (xlist_t *list)
{
  if (list)
    {
      while (list->prev)
        list = list->prev;
    }

  return list;
}

/**
 * xlist_length:
 * @list: a #xlist_t, this must point to the top of the list
 *
 * Gets the number of elements in a #xlist_t.
 *
 * This function iterates over the whole list to count its elements.
 * Use a #xqueue_t instead of a xlist_t if you regularly need the number
 * of items. To check whether the list is non-empty, it is faster to check
 * @list against %NULL.
 *
 * Returns: the number of elements in the #xlist_t
 */
xuint_t
xlist_length (xlist_t *list)
{
  xuint_t length;

  length = 0;
  while (list)
    {
      length++;
      list = list->next;
    }

  return length;
}

/**
 * xlist_foreach:
 * @list: a #xlist_t, this must point to the top of the list
 * @func: the function to call with each element's data
 * @user_data: user data to pass to the function
 *
 * Calls a function for each element of a #xlist_t.
 *
 * It is safe for @func to remove the element from @list, but it must
 * not modify any part of the list after that element.
 */
/**
 * GFunc:
 * @data: the element's data
 * @user_data: user data passed to xlist_foreach() or xslist_foreach()
 *
 * Specifies the type of functions passed to xlist_foreach() and
 * xslist_foreach().
 */
void
xlist_foreach (xlist_t    *list,
                GFunc     func,
                xpointer_t  user_data)
{
  while (list)
    {
      xlist_t *next = list->next;
      (*func) (list->data, user_data);
      list = next;
    }
}

static xlist_t*
xlist_insert_sorted_real (xlist_t    *list,
                           xpointer_t  data,
                           GFunc     func,
                           xpointer_t  user_data)
{
  xlist_t *tmp_list = list;
  xlist_t *new_list;
  xint_t cmp;

  xreturn_val_if_fail (func != NULL, list);

  if (!list)
    {
      new_list = _xlist_alloc0 ();
      new_list->data = data;
      return new_list;
    }

  cmp = ((GCompareDataFunc) func) (data, tmp_list->data, user_data);

  while ((tmp_list->next) && (cmp > 0))
    {
      tmp_list = tmp_list->next;

      cmp = ((GCompareDataFunc) func) (data, tmp_list->data, user_data);
    }

  new_list = _xlist_alloc0 ();
  new_list->data = data;

  if ((!tmp_list->next) && (cmp > 0))
    {
      tmp_list->next = new_list;
      new_list->prev = tmp_list;
      return list;
    }

  if (tmp_list->prev)
    {
      tmp_list->prev->next = new_list;
      new_list->prev = tmp_list->prev;
    }
  new_list->next = tmp_list;
  tmp_list->prev = new_list;

  if (tmp_list == list)
    return new_list;
  else
    return list;
}

/**
 * xlist_insert_sorted:
 * @list: a pointer to a #xlist_t, this must point to the top of the
 *     already sorted list
 * @data: the data for the new element
 * @func: the function to compare elements in the list. It should
 *     return a number > 0 if the first parameter comes after the
 *     second parameter in the sort order.
 *
 * Inserts a new element into the list, using the given comparison
 * function to determine its position.
 *
 * If you are adding many new elements to a list, and the number of
 * new elements is much larger than the length of the list, use
 * xlist_prepend() to add the new items and sort the list afterwards
 * with xlist_sort().
 *
 * Returns: the (possibly changed) start of the #xlist_t
 */
xlist_t *
xlist_insert_sorted (xlist_t        *list,
                      xpointer_t      data,
                      GCompareFunc  func)
{
  return xlist_insert_sorted_real (list, data, (GFunc) func, NULL);
}

/**
 * xlist_insert_sorted_with_data:
 * @list: a pointer to a #xlist_t, this must point to the top of the
 *     already sorted list
 * @data: the data for the new element
 * @func: the function to compare elements in the list. It should
 *     return a number > 0 if the first parameter  comes after the
 *     second parameter in the sort order.
 * @user_data: user data to pass to comparison function
 *
 * Inserts a new element into the list, using the given comparison
 * function to determine its position.
 *
 * If you are adding many new elements to a list, and the number of
 * new elements is much larger than the length of the list, use
 * xlist_prepend() to add the new items and sort the list afterwards
 * with xlist_sort().
 *
 * Returns: the (possibly changed) start of the #xlist_t
 *
 * Since: 2.10
 */
xlist_t *
xlist_insert_sorted_with_data (xlist_t            *list,
                                xpointer_t          data,
                                GCompareDataFunc  func,
                                xpointer_t          user_data)
{
  return xlist_insert_sorted_real (list, data, (GFunc) func, user_data);
}

static xlist_t *
xlist_sort_merge (xlist_t     *l1,
                   xlist_t     *l2,
                   GFunc     compare_func,
                   xpointer_t  user_data)
{
  xlist_t list, *l, *lprev;
  xint_t cmp;

  l = &list;
  lprev = NULL;

  while (l1 && l2)
    {
      cmp = ((GCompareDataFunc) compare_func) (l1->data, l2->data, user_data);

      if (cmp <= 0)
        {
          l->next = l1;
          l1 = l1->next;
        }
      else
        {
          l->next = l2;
          l2 = l2->next;
        }
      l = l->next;
      l->prev = lprev;
      lprev = l;
    }
  l->next = l1 ? l1 : l2;
  l->next->prev = l;

  return list.next;
}

static xlist_t *
xlist_sort_real (xlist_t    *list,
                  GFunc     compare_func,
                  xpointer_t  user_data)
{
  xlist_t *l1, *l2;

  if (!list)
    return NULL;
  if (!list->next)
    return list;

  l1 = list;
  l2 = list->next;

  while ((l2 = l2->next) != NULL)
    {
      if ((l2 = l2->next) == NULL)
        break;
      l1 = l1->next;
    }
  l2 = l1->next;
  l1->next = NULL;

  return xlist_sort_merge (xlist_sort_real (list, compare_func, user_data),
                            xlist_sort_real (l2, compare_func, user_data),
                            compare_func,
                            user_data);
}

/**
 * xlist_sort:
 * @list: a #xlist_t, this must point to the top of the list
 * @compare_func: the comparison function used to sort the #xlist_t.
 *     This function is passed the data from 2 elements of the #xlist_t
 *     and should return 0 if they are equal, a negative value if the
 *     first element comes before the second, or a positive value if
 *     the first element comes after the second.
 *
 * Sorts a #xlist_t using the given comparison function. The algorithm
 * used is a stable sort.
 *
 * Returns: the (possibly changed) start of the #xlist_t
 */
/**
 * GCompareFunc:
 * @a: a value
 * @b: a value to compare with
 *
 * Specifies the type of a comparison function used to compare two
 * values.  The function should return a negative integer if the first
 * value comes before the second, 0 if they are equal, or a positive
 * integer if the first value comes after the second.
 *
 * Returns: negative value if @a < @b; zero if @a = @b; positive
 *          value if @a > @b
 */
xlist_t *
xlist_sort (xlist_t        *list,
             GCompareFunc  compare_func)
{
  return xlist_sort_real (list, (GFunc) compare_func, NULL);
}

/**
 * xlist_sort_with_data:
 * @list: a #xlist_t, this must point to the top of the list
 * @compare_func: comparison function
 * @user_data: user data to pass to comparison function
 *
 * Like xlist_sort(), but the comparison function accepts
 * a user data argument.
 *
 * Returns: the (possibly changed) start of the #xlist_t
 */
/**
 * GCompareDataFunc:
 * @a: a value
 * @b: a value to compare with
 * @user_data: user data
 *
 * Specifies the type of a comparison function used to compare two
 * values.  The function should return a negative integer if the first
 * value comes before the second, 0 if they are equal, or a positive
 * integer if the first value comes after the second.
 *
 * Returns: negative value if @a < @b; zero if @a = @b; positive
 *          value if @a > @b
 */
xlist_t *
xlist_sort_with_data (xlist_t            *list,
                       GCompareDataFunc  compare_func,
                       xpointer_t          user_data)
{
  return xlist_sort_real (list, (GFunc) compare_func, user_data);
}

/**
 * g_clear_list: (skip)
 * @list_ptr: (not nullable): a #xlist_t return location
 * @destroy: (nullable): the function to pass to xlist_free_full() or %NULL to not free elements
 *
 * Clears a pointer to a #xlist_t, freeing it and, optionally, freeing its elements using @destroy.
 *
 * @list_ptr must be a valid pointer. If @list_ptr points to a null #xlist_t, this does nothing.
 *
 * Since: 2.64
 */
void
(g_clear_list) (xlist_t          **list_ptr,
                xdestroy_notify_t   destroy)
{
  xlist_t *list;

  list = *list_ptr;
  if (list)
    {
      *list_ptr = NULL;

      if (destroy)
        xlist_free_full (list, destroy);
      else
        xlist_free (list);
    }
}
