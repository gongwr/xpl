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

#include "gslist.h"

#include "gtestutils.h"
#include "gslice.h"

/**
 * SECTION:linked_lists_single
 * @title: Singly-Linked Lists
 * @short_description: linked lists that can be iterated in one direction
 *
 * The #xslist_t structure and its associated functions provide a
 * standard singly-linked list data structure. The benefit of this
 * data-structure is to provide insertion/deletion operations in O(1)
 * complexity where access/search operations are in O(n). The benefit
 * of #xslist_t over #xlist_t (doubly linked list) is that they are lighter
 * in space as they only need to retain one pointer but it double the
 * cost of the worst case access/search operations.
 *
 * Each element in the list contains a piece of data, together with a
 * pointer which links to the next element in the list. Using this
 * pointer it is possible to move through the list in one direction
 * only (unlike the [double-linked lists][glib-Doubly-Linked-Lists],
 * which allow movement in both directions).
 *
 * The data contained in each element can be either integer values, by
 * using one of the [Type Conversion Macros][glib-Type-Conversion-Macros],
 * or simply pointers to any type of data.
 *
 * List elements are allocated from the [slice allocator][glib-Memory-Slices],
 * which is more efficient than allocating elements individually.
 *
 * Note that most of the #xslist_t functions expect to be passed a
 * pointer to the first element in the list. The functions which insert
 * elements return the new start of the list, which may have changed.
 *
 * There is no function to create a #xslist_t. %NULL is considered to be
 * the empty list so you simply set a #xslist_t* to %NULL.
 *
 * To add elements, use xslist_append(), xslist_prepend(),
 * xslist_insert() and xslist_insert_sorted().
 *
 * To remove elements, use xslist_remove().
 *
 * To find elements in the list use xslist_last(), xslist_next(),
 * xslist_nth(), xslist_nth_data(), xslist_find() and
 * xslist_find_custom().
 *
 * To find the index of an element use xslist_position() and
 * xslist_index().
 *
 * To call a function for each element in the list use
 * xslist_foreach().
 *
 * To free the entire list, use xslist_free().
 **/

/**
 * xslist_t:
 * @data: holds the element's data, which can be a pointer to any kind
 *        of data, or any integer value using the
 *        [Type Conversion Macros][glib-Type-Conversion-Macros]
 * @next: contains the link to the next element in the list.
 *
 * The #xslist_t struct is used for each element in the singly-linked
 * list.
 **/

/**
 * xslist_next:
 * @slist: an element in a #xslist_t.
 *
 * A convenience macro to get the next element in a #xslist_t.
 * Note that it is considered perfectly acceptable to access
 * @slist->next directly.
 *
 * Returns: the next element, or %NULL if there are no more elements.
 **/

#define _xslist_alloc0()       g_slice_new0 (xslist_t)
#define _xslist_alloc()        g_slice_new (xslist_t)
#define _xslist_free1(slist)   g_slice_free (xslist_t, slist)

/**
 * xslist_alloc:
 *
 * Allocates space for one #xslist_t element. It is called by the
 * xslist_append(), xslist_prepend(), xslist_insert() and
 * xslist_insert_sorted() functions and so is rarely used on its own.
 *
 * Returns: a pointer to the newly-allocated #xslist_t element.
 **/
xslist_t*
xslist_alloc (void)
{
  return _xslist_alloc0 ();
}

/**
 * xslist_free:
 * @list: the first link of a #xslist_t
 *
 * Frees all of the memory used by a #xslist_t.
 * The freed elements are returned to the slice allocator.
 *
 * If list elements contain dynamically-allocated memory,
 * you should either use xslist_free_full() or free them manually
 * first.
 *
 * It can be combined with g_steal_pointer() to ensure the list head pointer
 * is not left dangling:
 * |[<!-- language="C" -->
 * xslist_t *list_of_borrowed_things = …;  /<!-- -->* (transfer container) *<!-- -->/
 * xslist_free (g_steal_pointer (&list_of_borrowed_things));
 * ]|
 */
void
xslist_free (xslist_t *list)
{
  g_slice_free_chain (xslist_t, list, next);
}

/**
 * xslist_free_1:
 * @list: a #xslist_t element
 *
 * Frees one #xslist_t element.
 * It is usually used after xslist_remove_link().
 */
/**
 * xslist_free1:
 *
 * A macro which does the same as xslist_free_1().
 *
 * Since: 2.10
 **/
void
xslist_free_1 (xslist_t *list)
{
  _xslist_free1 (list);
}

/**
 * xslist_free_full:
 * @list: the first link of a #xslist_t
 * @free_func: the function to be called to free each element's data
 *
 * Convenience method, which frees all the memory used by a #xslist_t, and
 * calls the specified destroy function on every element's data.
 *
 * @free_func must not modify the list (eg, by removing the freed
 * element from it).
 *
 * It can be combined with g_steal_pointer() to ensure the list head pointer
 * is not left dangling ­— this also has the nice property that the head pointer
 * is cleared before any of the list elements are freed, to prevent double frees
 * from @free_func:
 * |[<!-- language="C" -->
 * xslist_t *list_of_owned_things = …;  /<!-- -->* (transfer full) (element-type xobject_t) *<!-- -->/
 * xslist_free_full (g_steal_pointer (&list_of_owned_things), xobject_unref);
 * ]|
 *
 * Since: 2.28
 **/
void
xslist_free_full (xslist_t         *list,
		   xdestroy_notify_t  free_func)
{
  xslist_foreach (list, (GFunc) free_func, NULL);
  xslist_free (list);
}

/**
 * xslist_append:
 * @list: a #xslist_t
 * @data: the data for the new element
 *
 * Adds a new element on to the end of the list.
 *
 * The return value is the new start of the list, which may
 * have changed, so make sure you store the new value.
 *
 * Note that xslist_append() has to traverse the entire list
 * to find the end, which is inefficient when adding multiple
 * elements. A common idiom to avoid the inefficiency is to prepend
 * the elements and reverse the list when all elements have been added.
 *
 * |[<!-- language="C" -->
 * // Notice that these are initialized to the empty list.
 * xslist_t *list = NULL, *number_list = NULL;
 *
 * // This is a list of strings.
 * list = xslist_append (list, "first");
 * list = xslist_append (list, "second");
 *
 * // This is a list of integers.
 * number_list = xslist_append (number_list, GINT_TO_POINTER (27));
 * number_list = xslist_append (number_list, GINT_TO_POINTER (14));
 * ]|
 *
 * Returns: the new start of the #xslist_t
 */
xslist_t*
xslist_append (xslist_t   *list,
                xpointer_t  data)
{
  xslist_t *new_list;
  xslist_t *last;

  new_list = _xslist_alloc ();
  new_list->data = data;
  new_list->next = NULL;

  if (list)
    {
      last = xslist_last (list);
      /* g_assert (last != NULL); */
      last->next = new_list;

      return list;
    }
  else
    return new_list;
}

/**
 * xslist_prepend:
 * @list: a #xslist_t
 * @data: the data for the new element
 *
 * Adds a new element on to the start of the list.
 *
 * The return value is the new start of the list, which
 * may have changed, so make sure you store the new value.
 *
 * |[<!-- language="C" -->
 * // Notice that it is initialized to the empty list.
 * xslist_t *list = NULL;
 * list = xslist_prepend (list, "last");
 * list = xslist_prepend (list, "first");
 * ]|
 *
 * Returns: the new start of the #xslist_t
 */
xslist_t*
xslist_prepend (xslist_t   *list,
                 xpointer_t  data)
{
  xslist_t *new_list;

  new_list = _xslist_alloc ();
  new_list->data = data;
  new_list->next = list;

  return new_list;
}

/**
 * xslist_insert:
 * @list: a #xslist_t
 * @data: the data for the new element
 * @position: the position to insert the element.
 *     If this is negative, or is larger than the number
 *     of elements in the list, the new element is added on
 *     to the end of the list.
 *
 * Inserts a new element into the list at the given position.
 *
 * Returns: the new start of the #xslist_t
 */
xslist_t*
xslist_insert (xslist_t   *list,
                xpointer_t  data,
                xint_t      position)
{
  xslist_t *prev_list;
  xslist_t *tmp_list;
  xslist_t *new_list;

  if (position < 0)
    return xslist_append (list, data);
  else if (position == 0)
    return xslist_prepend (list, data);

  new_list = _xslist_alloc ();
  new_list->data = data;

  if (!list)
    {
      new_list->next = NULL;
      return new_list;
    }

  prev_list = NULL;
  tmp_list = list;

  while ((position-- > 0) && tmp_list)
    {
      prev_list = tmp_list;
      tmp_list = tmp_list->next;
    }

  new_list->next = prev_list->next;
  prev_list->next = new_list;

  return list;
}

/**
 * xslist_insert_before:
 * @slist: a #xslist_t
 * @sibling: node to insert @data before
 * @data: data to put in the newly-inserted node
 *
 * Inserts a node before @sibling containing @data.
 *
 * Returns: the new head of the list.
 */
xslist_t*
xslist_insert_before (xslist_t  *slist,
                       xslist_t  *sibling,
                       xpointer_t data)
{
  if (!slist)
    {
      slist = _xslist_alloc ();
      slist->data = data;
      slist->next = NULL;
      g_return_val_if_fail (sibling == NULL, slist);
      return slist;
    }
  else
    {
      xslist_t *node, *last = NULL;

      for (node = slist; node; last = node, node = last->next)
        if (node == sibling)
          break;
      if (!last)
        {
          node = _xslist_alloc ();
          node->data = data;
          node->next = slist;

          return node;
        }
      else
        {
          node = _xslist_alloc ();
          node->data = data;
          node->next = last->next;
          last->next = node;

          return slist;
        }
    }
}

/**
 * xslist_concat:
 * @list1: a #xslist_t
 * @list2: the #xslist_t to add to the end of the first #xslist_t
 *
 * Adds the second #xslist_t onto the end of the first #xslist_t.
 * Note that the elements of the second #xslist_t are not copied.
 * They are used directly.
 *
 * Returns: the start of the new #xslist_t
 */
xslist_t *
xslist_concat (xslist_t *list1, xslist_t *list2)
{
  if (list2)
    {
      if (list1)
        xslist_last (list1)->next = list2;
      else
        list1 = list2;
    }

  return list1;
}

static xslist_t*
_xslist_remove_data (xslist_t        *list,
                      xconstpointer  data,
                      xboolean_t       all)
{
  xslist_t *tmp = NULL;
  xslist_t **previous_ptr = &list;

  while (*previous_ptr)
    {
      tmp = *previous_ptr;
      if (tmp->data == data)
        {
          *previous_ptr = tmp->next;
          xslist_free_1 (tmp);
          if (!all)
            break;
        }
      else
        {
          previous_ptr = &tmp->next;
        }
    }

  return list;
}
/**
 * xslist_remove:
 * @list: a #xslist_t
 * @data: the data of the element to remove
 *
 * Removes an element from a #xslist_t.
 * If two elements contain the same data, only the first is removed.
 * If none of the elements contain the data, the #xslist_t is unchanged.
 *
 * Returns: the new start of the #xslist_t
 */
xslist_t*
xslist_remove (xslist_t        *list,
                xconstpointer  data)
{
  return _xslist_remove_data (list, data, FALSE);
}

/**
 * xslist_remove_all:
 * @list: a #xslist_t
 * @data: data to remove
 *
 * Removes all list nodes with data equal to @data.
 * Returns the new head of the list. Contrast with
 * xslist_remove() which removes only the first node
 * matching the given data.
 *
 * Returns: new head of @list
 */
xslist_t*
xslist_remove_all (xslist_t        *list,
                    xconstpointer  data)
{
  return _xslist_remove_data (list, data, TRUE);
}

static inline xslist_t*
_xslist_remove_link (xslist_t *list,
                      xslist_t *link)
{
  xslist_t *tmp = NULL;
  xslist_t **previous_ptr = &list;

  while (*previous_ptr)
    {
      tmp = *previous_ptr;
      if (tmp == link)
        {
          *previous_ptr = tmp->next;
          tmp->next = NULL;
          break;
        }

      previous_ptr = &tmp->next;
    }

  return list;
}

/**
 * xslist_remove_link:
 * @list: a #xslist_t
 * @link_: an element in the #xslist_t
 *
 * Removes an element from a #xslist_t, without
 * freeing the element. The removed element's next
 * link is set to %NULL, so that it becomes a
 * self-contained list with one element.
 *
 * Removing arbitrary nodes from a singly-linked list
 * requires time that is proportional to the length of the list
 * (ie. O(n)). If you find yourself using xslist_remove_link()
 * frequently, you should consider a different data structure,
 * such as the doubly-linked #xlist_t.
 *
 * Returns: the new start of the #xslist_t, without the element
 */
xslist_t*
xslist_remove_link (xslist_t *list,
                     xslist_t *link_)
{
  return _xslist_remove_link (list, link_);
}

/**
 * xslist_delete_link:
 * @list: a #xslist_t
 * @link_: node to delete
 *
 * Removes the node link_ from the list and frees it.
 * Compare this to xslist_remove_link() which removes the node
 * without freeing it.
 *
 * Removing arbitrary nodes from a singly-linked list requires time
 * that is proportional to the length of the list (ie. O(n)). If you
 * find yourself using xslist_delete_link() frequently, you should
 * consider a different data structure, such as the doubly-linked
 * #xlist_t.
 *
 * Returns: the new head of @list
 */
xslist_t*
xslist_delete_link (xslist_t *list,
                     xslist_t *link_)
{
  list = _xslist_remove_link (list, link_);
  _xslist_free1 (link_);

  return list;
}

/**
 * xslist_copy:
 * @list: a #xslist_t
 *
 * Copies a #xslist_t.
 *
 * Note that this is a "shallow" copy. If the list elements
 * consist of pointers to data, the pointers are copied but
 * the actual data isn't. See xslist_copy_deep() if you need
 * to copy the data as well.
 *
 * Returns: a copy of @list
 */
xslist_t*
xslist_copy (xslist_t *list)
{
  return xslist_copy_deep (list, NULL, NULL);
}

/**
 * xslist_copy_deep:
 * @list: a #xslist_t
 * @func: a copy function used to copy every element in the list
 * @user_data: user data passed to the copy function @func, or #NULL
 *
 * Makes a full (deep) copy of a #xslist_t.
 *
 * In contrast with xslist_copy(), this function uses @func to make a copy of
 * each list element, in addition to copying the list container itself.
 *
 * @func, as a #GCopyFunc, takes two arguments, the data to be copied
 * and a @user_data pointer. On common processor architectures, it's safe to
 * pass %NULL as @user_data if the copy function takes only one argument. You
 * may get compiler warnings from this though if compiling with GCC’s
 * `-Wcast-function-type` warning.
 *
 * For instance, if @list holds a list of GObjects, you can do:
 * |[<!-- language="C" -->
 * another_list = xslist_copy_deep (list, (GCopyFunc) xobject_ref, NULL);
 * ]|
 *
 * And, to entirely free the new list, you could do:
 * |[<!-- language="C" -->
 * xslist_free_full (another_list, xobject_unref);
 * ]|
 *
 * Returns: a full copy of @list, use xslist_free_full() to free it
 *
 * Since: 2.34
 */
xslist_t*
xslist_copy_deep (xslist_t *list, GCopyFunc func, xpointer_t user_data)
{
  xslist_t *new_list = NULL;

  if (list)
    {
      xslist_t *last;

      new_list = _xslist_alloc ();
      if (func)
        new_list->data = func (list->data, user_data);
      else
        new_list->data = list->data;
      last = new_list;
      list = list->next;
      while (list)
        {
          last->next = _xslist_alloc ();
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
 * xslist_reverse:
 * @list: a #xslist_t
 *
 * Reverses a #xslist_t.
 *
 * Returns: the start of the reversed #xslist_t
 */
xslist_t*
xslist_reverse (xslist_t *list)
{
  xslist_t *prev = NULL;

  while (list)
    {
      xslist_t *next = list->next;

      list->next = prev;

      prev = list;
      list = next;
    }

  return prev;
}

/**
 * xslist_nth:
 * @list: a #xslist_t
 * @n: the position of the element, counting from 0
 *
 * Gets the element at the given position in a #xslist_t.
 *
 * Returns: the element, or %NULL if the position is off
 *     the end of the #xslist_t
 */
xslist_t*
xslist_nth (xslist_t *list,
             xuint_t   n)
{
  while (n-- > 0 && list)
    list = list->next;

  return list;
}

/**
 * xslist_nth_data:
 * @list: a #xslist_t
 * @n: the position of the element
 *
 * Gets the data of the element at the given position.
 *
 * Returns: the element's data, or %NULL if the position
 *     is off the end of the #xslist_t
 */
xpointer_t
xslist_nth_data (xslist_t   *list,
                  xuint_t     n)
{
  while (n-- > 0 && list)
    list = list->next;

  return list ? list->data : NULL;
}

/**
 * xslist_find:
 * @list: a #xslist_t
 * @data: the element data to find
 *
 * Finds the element in a #xslist_t which
 * contains the given data.
 *
 * Returns: the found #xslist_t element,
 *     or %NULL if it is not found
 */
xslist_t*
xslist_find (xslist_t        *list,
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
 * xslist_find_custom:
 * @list: a #xslist_t
 * @data: user data passed to the function
 * @func: the function to call for each element.
 *     It should return 0 when the desired element is found
 *
 * Finds an element in a #xslist_t, using a supplied function to
 * find the desired element. It iterates over the list, calling
 * the given function which should return 0 when the desired
 * element is found. The function takes two #xconstpointer arguments,
 * the #xslist_t element's data as the first argument and the
 * given user data.
 *
 * Returns: the found #xslist_t element, or %NULL if it is not found
 */
xslist_t*
xslist_find_custom (xslist_t        *list,
                     xconstpointer  data,
                     GCompareFunc   func)
{
  g_return_val_if_fail (func != NULL, list);

  while (list)
    {
      if (! func (list->data, data))
        return list;
      list = list->next;
    }

  return NULL;
}

/**
 * xslist_position:
 * @list: a #xslist_t
 * @llink: an element in the #xslist_t
 *
 * Gets the position of the given element
 * in the #xslist_t (starting from 0).
 *
 * Returns: the position of the element in the #xslist_t,
 *     or -1 if the element is not found
 */
xint_t
xslist_position (xslist_t *list,
                  xslist_t *llink)
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
 * xslist_index:
 * @list: a #xslist_t
 * @data: the data to find
 *
 * Gets the position of the element containing
 * the given data (starting from 0).
 *
 * Returns: the index of the element containing the data,
 *     or -1 if the data is not found
 */
xint_t
xslist_index (xslist_t        *list,
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
 * xslist_last:
 * @list: a #xslist_t
 *
 * Gets the last element in a #xslist_t.
 *
 * This function iterates over the whole list.
 *
 * Returns: the last element in the #xslist_t,
 *     or %NULL if the #xslist_t has no elements
 */
xslist_t*
xslist_last (xslist_t *list)
{
  if (list)
    {
      while (list->next)
        list = list->next;
    }

  return list;
}

/**
 * xslist_length:
 * @list: a #xslist_t
 *
 * Gets the number of elements in a #xslist_t.
 *
 * This function iterates over the whole list to
 * count its elements. To check whether the list is non-empty, it is faster to
 * check @list against %NULL.
 *
 * Returns: the number of elements in the #xslist_t
 */
xuint_t
xslist_length (xslist_t *list)
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
 * xslist_foreach:
 * @list: a #xslist_t
 * @func: the function to call with each element's data
 * @user_data: user data to pass to the function
 *
 * Calls a function for each element of a #xslist_t.
 *
 * It is safe for @func to remove the element from @list, but it must
 * not modify any part of the list after that element.
 */
void
xslist_foreach (xslist_t   *list,
                 GFunc     func,
                 xpointer_t  user_data)
{
  while (list)
    {
      xslist_t *next = list->next;
      (*func) (list->data, user_data);
      list = next;
    }
}

static xslist_t*
xslist_insert_sorted_real (xslist_t   *list,
                            xpointer_t  data,
                            GFunc     func,
                            xpointer_t  user_data)
{
  xslist_t *tmp_list = list;
  xslist_t *prev_list = NULL;
  xslist_t *new_list;
  xint_t cmp;

  g_return_val_if_fail (func != NULL, list);

  if (!list)
    {
      new_list = _xslist_alloc ();
      new_list->data = data;
      new_list->next = NULL;
      return new_list;
    }

  cmp = ((GCompareDataFunc) func) (data, tmp_list->data, user_data);

  while ((tmp_list->next) && (cmp > 0))
    {
      prev_list = tmp_list;
      tmp_list = tmp_list->next;

      cmp = ((GCompareDataFunc) func) (data, tmp_list->data, user_data);
    }

  new_list = _xslist_alloc ();
  new_list->data = data;

  if ((!tmp_list->next) && (cmp > 0))
    {
      tmp_list->next = new_list;
      new_list->next = NULL;
      return list;
    }

  if (prev_list)
    {
      prev_list->next = new_list;
      new_list->next = tmp_list;
      return list;
    }
  else
    {
      new_list->next = list;
      return new_list;
    }
}

/**
 * xslist_insert_sorted:
 * @list: a #xslist_t
 * @data: the data for the new element
 * @func: the function to compare elements in the list.
 *     It should return a number > 0 if the first parameter
 *     comes after the second parameter in the sort order.
 *
 * Inserts a new element into the list, using the given
 * comparison function to determine its position.
 *
 * Returns: the new start of the #xslist_t
 */
xslist_t*
xslist_insert_sorted (xslist_t       *list,
                       xpointer_t      data,
                       GCompareFunc  func)
{
  return xslist_insert_sorted_real (list, data, (GFunc) func, NULL);
}

/**
 * xslist_insert_sorted_with_data:
 * @list: a #xslist_t
 * @data: the data for the new element
 * @func: the function to compare elements in the list.
 *     It should return a number > 0 if the first parameter
 *     comes after the second parameter in the sort order.
 * @user_data: data to pass to comparison function
 *
 * Inserts a new element into the list, using the given
 * comparison function to determine its position.
 *
 * Returns: the new start of the #xslist_t
 *
 * Since: 2.10
 */
xslist_t*
xslist_insert_sorted_with_data (xslist_t           *list,
                                 xpointer_t          data,
                                 GCompareDataFunc  func,
                                 xpointer_t          user_data)
{
  return xslist_insert_sorted_real (list, data, (GFunc) func, user_data);
}

static xslist_t *
xslist_sort_merge (xslist_t   *l1,
                    xslist_t   *l2,
                    GFunc     compare_func,
                    xpointer_t  user_data)
{
  xslist_t list, *l;
  xint_t cmp;

  l=&list;

  while (l1 && l2)
    {
      cmp = ((GCompareDataFunc) compare_func) (l1->data, l2->data, user_data);

      if (cmp <= 0)
        {
          l=l->next=l1;
          l1=l1->next;
        }
      else
        {
          l=l->next=l2;
          l2=l2->next;
        }
    }
  l->next= l1 ? l1 : l2;

  return list.next;
}

static xslist_t *
xslist_sort_real (xslist_t   *list,
                   GFunc     compare_func,
                   xpointer_t  user_data)
{
  xslist_t *l1, *l2;

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
      l1=l1->next;
    }
  l2 = l1->next;
  l1->next = NULL;

  return xslist_sort_merge (xslist_sort_real (list, compare_func, user_data),
                             xslist_sort_real (l2, compare_func, user_data),
                             compare_func,
                             user_data);
}

/**
 * xslist_sort:
 * @list: a #xslist_t
 * @compare_func: the comparison function used to sort the #xslist_t.
 *     This function is passed the data from 2 elements of the #xslist_t
 *     and should return 0 if they are equal, a negative value if the
 *     first element comes before the second, or a positive value if
 *     the first element comes after the second.
 *
 * Sorts a #xslist_t using the given comparison function. The algorithm
 * used is a stable sort.
 *
 * Returns: the start of the sorted #xslist_t
 */
xslist_t *
xslist_sort (xslist_t       *list,
              GCompareFunc  compare_func)
{
  return xslist_sort_real (list, (GFunc) compare_func, NULL);
}

/**
 * xslist_sort_with_data:
 * @list: a #xslist_t
 * @compare_func: comparison function
 * @user_data: data to pass to comparison function
 *
 * Like xslist_sort(), but the sort function accepts a user data argument.
 *
 * Returns: new head of the list
 */
xslist_t *
xslist_sort_with_data (xslist_t           *list,
                        GCompareDataFunc  compare_func,
                        xpointer_t          user_data)
{
  return xslist_sort_real (list, (GFunc) compare_func, user_data);
}

/**
 * g_clear_slist: (skip)
 * @slist_ptr: (not nullable): a #xslist_t return location
 * @destroy: (nullable): the function to pass to xslist_free_full() or %NULL to not free elements
 *
 * Clears a pointer to a #xslist_t, freeing it and, optionally, freeing its elements using @destroy.
 *
 * @slist_ptr must be a valid pointer. If @slist_ptr points to a null #xslist_t, this does nothing.
 *
 * Since: 2.64
 */
void
(g_clear_slist) (xslist_t         **slist_ptr,
                 xdestroy_notify_t   destroy)
{
  xslist_t *slist;

  slist = *slist_ptr;
  if (slist)
    {
      *slist_ptr = NULL;

      if (destroy)
        xslist_free_full (slist, destroy);
      else
        xslist_free (slist);
    }
}
