/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * xqueue_t: Double ended queue implementation, piggy backed on xlist_t.
 * Copyright (C) 1998 Tim Janik
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
 * MT safe
 */

/**
 * SECTION:queue
 * @Title: Double-ended Queues
 * @Short_description: double-ended queue data structure
 *
 * The #xqueue_t structure and its associated functions provide a standard
 * queue data structure. Internally, xqueue_t uses the same data structure
 * as #xlist_t to store elements with the same complexity over
 * insertion/deletion (O(1)) and access/search (O(n)) operations.
 *
 * The data contained in each element can be either integer values, by
 * using one of the [Type Conversion Macros][glib-Type-Conversion-Macros],
 * or simply pointers to any type of data.
 *
 * As with all other GLib data structures, #xqueue_t is not thread-safe.
 * For a thread-safe queue, use #xasync_queue_t.
 *
 * To create a new xqueue_t, use g_queue_new().
 *
 * To initialize a statically-allocated xqueue_t, use %G_QUEUE_INIT or
 * g_queue_init().
 *
 * To add elements, use g_queue_push_head(), g_queue_push_head_link(),
 * g_queue_push_tail() and g_queue_push_tail_link().
 *
 * To remove elements, use g_queue_pop_head() and g_queue_pop_tail().
 *
 * To free the entire queue, use g_queue_free().
 */
#include "config.h"

#include "gqueue.h"

#include "gtestutils.h"
#include "gslice.h"

/**
 * g_queue_new:
 *
 * Creates a new #xqueue_t.
 *
 * Returns: a newly allocated #xqueue_t
 **/
xqueue_t *
g_queue_new (void)
{
  return g_slice_new0 (xqueue_t);
}

/**
 * g_queue_free:
 * @queue: a #xqueue_t
 *
 * Frees the memory allocated for the #xqueue_t. Only call this function
 * if @queue was created with g_queue_new(). If queue elements contain
 * dynamically-allocated memory, they should be freed first.
 *
 * If queue elements contain dynamically-allocated memory, you should
 * either use g_queue_free_full() or free them manually first.
 **/
void
g_queue_free (xqueue_t *queue)
{
  g_return_if_fail (queue != NULL);

  xlist_free (queue->head);
  g_slice_free (xqueue_t, queue);
}

/**
 * g_queue_free_full:
 * @queue: a pointer to a #xqueue_t
 * @free_func: the function to be called to free each element's data
 *
 * Convenience method, which frees all the memory used by a #xqueue_t,
 * and calls the specified destroy function on every element's data.
 *
 * @free_func should not modify the queue (eg, by removing the freed
 * element from it).
 *
 * Since: 2.32
 */
void
g_queue_free_full (xqueue_t        *queue,
                  xdestroy_notify_t  free_func)
{
  g_queue_foreach (queue, (GFunc) free_func, NULL);
  g_queue_free (queue);
}

/**
 * g_queue_init:
 * @queue: an uninitialized #xqueue_t
 *
 * A statically-allocated #xqueue_t must be initialized with this function
 * before it can be used. Alternatively you can initialize it with
 * %G_QUEUE_INIT. It is not necessary to initialize queues created with
 * g_queue_new().
 *
 * Since: 2.14
 */
void
g_queue_init (xqueue_t *queue)
{
  g_return_if_fail (queue != NULL);

  queue->head = queue->tail = NULL;
  queue->length = 0;
}

/**
 * g_queue_clear:
 * @queue: a #xqueue_t
 *
 * Removes all the elements in @queue. If queue elements contain
 * dynamically-allocated memory, they should be freed first.
 *
 * Since: 2.14
 */
void
g_queue_clear (xqueue_t *queue)
{
  g_return_if_fail (queue != NULL);

  xlist_free (queue->head);
  g_queue_init (queue);
}

/**
 * g_queue_clear_full:
 * @queue: a pointer to a #xqueue_t
 * @free_func: (nullable): the function to be called to free memory allocated
 *
 * Convenience method, which frees all the memory used by a #xqueue_t,
 * and calls the provided @free_func on each item in the #xqueue_t.
 *
 * Since: 2.60
 */
void
g_queue_clear_full (xqueue_t          *queue,
                    xdestroy_notify_t  free_func)
{
  g_return_if_fail (queue != NULL);

  if (free_func != NULL)
    g_queue_foreach (queue, (GFunc) free_func, NULL);

  g_queue_clear (queue);
}

/**
 * g_queue_is_empty:
 * @queue: a #xqueue_t.
 *
 * Returns %TRUE if the queue is empty.
 *
 * Returns: %TRUE if the queue is empty
 */
xboolean_t
g_queue_is_empty (xqueue_t *queue)
{
  g_return_val_if_fail (queue != NULL, TRUE);

  return queue->head == NULL;
}

/**
 * g_queue_get_length:
 * @queue: a #xqueue_t
 *
 * Returns the number of items in @queue.
 *
 * Returns: the number of items in @queue
 *
 * Since: 2.4
 */
xuint_t
g_queue_get_length (xqueue_t *queue)
{
  g_return_val_if_fail (queue != NULL, 0);

  return queue->length;
}

/**
 * g_queue_reverse:
 * @queue: a #xqueue_t
 *
 * Reverses the order of the items in @queue.
 *
 * Since: 2.4
 */
void
g_queue_reverse (xqueue_t *queue)
{
  g_return_if_fail (queue != NULL);

  queue->tail = queue->head;
  queue->head = xlist_reverse (queue->head);
}

/**
 * g_queue_copy:
 * @queue: a #xqueue_t
 *
 * Copies a @queue. Note that is a shallow copy. If the elements in the
 * queue consist of pointers to data, the pointers are copied, but the
 * actual data is not.
 *
 * Returns: a copy of @queue
 *
 * Since: 2.4
 */
xqueue_t *
g_queue_copy (xqueue_t *queue)
{
  xqueue_t *result;
  xlist_t *list;

  g_return_val_if_fail (queue != NULL, NULL);

  result = g_queue_new ();

  for (list = queue->head; list != NULL; list = list->next)
    g_queue_push_tail (result, list->data);

  return result;
}

/**
 * g_queue_foreach:
 * @queue: a #xqueue_t
 * @func: the function to call for each element's data
 * @user_data: user data to pass to @func
 *
 * Calls @func for each element in the queue passing @user_data to the
 * function.
 *
 * It is safe for @func to remove the element from @queue, but it must
 * not modify any part of the queue after that element.
 *
 * Since: 2.4
 */
void
g_queue_foreach (xqueue_t   *queue,
                 GFunc     func,
                 xpointer_t  user_data)
{
  xlist_t *list;

  g_return_if_fail (queue != NULL);
  g_return_if_fail (func != NULL);

  list = queue->head;
  while (list)
    {
      xlist_t *next = list->next;
      func (list->data, user_data);
      list = next;
    }
}

/**
 * g_queue_find:
 * @queue: a #xqueue_t
 * @data: data to find
 *
 * Finds the first link in @queue which contains @data.
 *
 * Returns: the first link in @queue which contains @data
 *
 * Since: 2.4
 */
xlist_t *
g_queue_find (xqueue_t        *queue,
              xconstpointer  data)
{
  g_return_val_if_fail (queue != NULL, NULL);

  return xlist_find (queue->head, data);
}

/**
 * g_queue_find_custom:
 * @queue: a #xqueue_t
 * @data: user data passed to @func
 * @func: a #GCompareFunc to call for each element. It should return 0
 *     when the desired element is found
 *
 * Finds an element in a #xqueue_t, using a supplied function to find the
 * desired element. It iterates over the queue, calling the given function
 * which should return 0 when the desired element is found. The function
 * takes two xconstpointer arguments, the #xqueue_t element's data as the
 * first argument and the given user data as the second argument.
 *
 * Returns: the found link, or %NULL if it wasn't found
 *
 * Since: 2.4
 */
xlist_t *
g_queue_find_custom (xqueue_t        *queue,
                     xconstpointer  data,
                     GCompareFunc   func)
{
  g_return_val_if_fail (queue != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  return xlist_find_custom (queue->head, data, func);
}

/**
 * g_queue_sort:
 * @queue: a #xqueue_t
 * @compare_func: the #GCompareDataFunc used to sort @queue. This function
 *     is passed two elements of the queue and should return 0 if they are
 *     equal, a negative value if the first comes before the second, and
 *     a positive value if the second comes before the first.
 * @user_data: user data passed to @compare_func
 *
 * Sorts @queue using @compare_func.
 *
 * Since: 2.4
 */
void
g_queue_sort (xqueue_t           *queue,
              GCompareDataFunc  compare_func,
              xpointer_t          user_data)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (compare_func != NULL);

  queue->head = xlist_sort_with_data (queue->head, compare_func, user_data);
  queue->tail = xlist_last (queue->head);
}

/**
 * g_queue_push_head:
 * @queue: a #xqueue_t.
 * @data: the data for the new element.
 *
 * Adds a new element at the head of the queue.
 */
void
g_queue_push_head (xqueue_t   *queue,
                   xpointer_t  data)
{
  g_return_if_fail (queue != NULL);

  queue->head = xlist_prepend (queue->head, data);
  if (!queue->tail)
    queue->tail = queue->head;
  queue->length++;
}

/**
 * g_queue_push_nth:
 * @queue: a #xqueue_t
 * @data: the data for the new element
 * @n: the position to insert the new element. If @n is negative or
 *     larger than the number of elements in the @queue, the element is
 *     added to the end of the queue.
 *
 * Inserts a new element into @queue at the given position.
 *
 * Since: 2.4
 */
void
g_queue_push_nth (xqueue_t   *queue,
                  xpointer_t  data,
                  xint_t      n)
{
  g_return_if_fail (queue != NULL);

  if (n < 0 || (xuint_t) n >= queue->length)
    {
      g_queue_push_tail (queue, data);
      return;
    }

  g_queue_insert_before (queue, g_queue_peek_nth_link (queue, n), data);
}

/**
 * g_queue_push_head_link:
 * @queue: a #xqueue_t
 * @link_: a single #xlist_t element, not a list with more than one element
 *
 * Adds a new element at the head of the queue.
 */
void
g_queue_push_head_link (xqueue_t *queue,
                        xlist_t  *link)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (link != NULL);
  g_return_if_fail (link->prev == NULL);
  g_return_if_fail (link->next == NULL);

  link->next = queue->head;
  if (queue->head)
    queue->head->prev = link;
  else
    queue->tail = link;
  queue->head = link;
  queue->length++;
}

/**
 * g_queue_push_tail:
 * @queue: a #xqueue_t
 * @data: the data for the new element
 *
 * Adds a new element at the tail of the queue.
 */
void
g_queue_push_tail (xqueue_t   *queue,
                   xpointer_t  data)
{
  g_return_if_fail (queue != NULL);

  queue->tail = xlist_append (queue->tail, data);
  if (queue->tail->next)
    queue->tail = queue->tail->next;
  else
    queue->head = queue->tail;
  queue->length++;
}

/**
 * g_queue_push_tail_link:
 * @queue: a #xqueue_t
 * @link_: a single #xlist_t element, not a list with more than one element
 *
 * Adds a new element at the tail of the queue.
 */
void
g_queue_push_tail_link (xqueue_t *queue,
                        xlist_t  *link)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (link != NULL);
  g_return_if_fail (link->prev == NULL);
  g_return_if_fail (link->next == NULL);

  link->prev = queue->tail;
  if (queue->tail)
    queue->tail->next = link;
  else
    queue->head = link;
  queue->tail = link;
  queue->length++;
}

/**
 * g_queue_push_nth_link:
 * @queue: a #xqueue_t
 * @n: the position to insert the link. If this is negative or larger than
 *     the number of elements in @queue, the link is added to the end of
 *     @queue.
 * @link_: the link to add to @queue
 *
 * Inserts @link into @queue at the given position.
 *
 * Since: 2.4
 */
void
g_queue_push_nth_link (xqueue_t *queue,
                       xint_t    n,
                       xlist_t  *link_)
{
  xlist_t *next;
  xlist_t *prev;

  g_return_if_fail (queue != NULL);
  g_return_if_fail (link_ != NULL);

  if (n < 0 || (xuint_t) n >= queue->length)
    {
      g_queue_push_tail_link (queue, link_);
      return;
    }

  g_assert (queue->head);
  g_assert (queue->tail);

  next = g_queue_peek_nth_link (queue, n);
  prev = next->prev;

  if (prev)
    prev->next = link_;
  next->prev = link_;

  link_->next = next;
  link_->prev = prev;

  if (queue->head->prev)
    queue->head = queue->head->prev;

  /* The case where we’re pushing @link_ at the end of @queue is handled above
   * using g_queue_push_tail_link(), so we should never have to manually adjust
   * queue->tail. */
  g_assert (queue->tail->next == NULL);

  queue->length++;
}

/**
 * g_queue_pop_head:
 * @queue: a #xqueue_t
 *
 * Removes the first element of the queue and returns its data.
 *
 * Returns: the data of the first element in the queue, or %NULL
 *     if the queue is empty
 */
xpointer_t
g_queue_pop_head (xqueue_t *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  if (queue->head)
    {
      xlist_t *node = queue->head;
      xpointer_t data = node->data;

      queue->head = node->next;
      if (queue->head)
        queue->head->prev = NULL;
      else
        queue->tail = NULL;
      xlist_free_1 (node);
      queue->length--;

      return data;
    }

  return NULL;
}

/**
 * g_queue_pop_head_link:
 * @queue: a #xqueue_t
 *
 * Removes and returns the first element of the queue.
 *
 * Returns: the #xlist_t element at the head of the queue, or %NULL
 *     if the queue is empty
 */
xlist_t *
g_queue_pop_head_link (xqueue_t *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  if (queue->head)
    {
      xlist_t *node = queue->head;

      queue->head = node->next;
      if (queue->head)
        {
          queue->head->prev = NULL;
          node->next = NULL;
        }
      else
        queue->tail = NULL;
      queue->length--;

      return node;
    }

  return NULL;
}

/**
 * g_queue_peek_head_link:
 * @queue: a #xqueue_t
 *
 * Returns the first link in @queue.
 *
 * Returns: the first link in @queue, or %NULL if @queue is empty
 *
 * Since: 2.4
 */
xlist_t *
g_queue_peek_head_link (xqueue_t *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  return queue->head;
}

/**
 * g_queue_peek_tail_link:
 * @queue: a #xqueue_t
 *
 * Returns the last link in @queue.
 *
 * Returns: the last link in @queue, or %NULL if @queue is empty
 *
 * Since: 2.4
 */
xlist_t *
g_queue_peek_tail_link (xqueue_t *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  return queue->tail;
}

/**
 * g_queue_pop_tail:
 * @queue: a #xqueue_t
 *
 * Removes the last element of the queue and returns its data.
 *
 * Returns: the data of the last element in the queue, or %NULL
 *     if the queue is empty
 */
xpointer_t
g_queue_pop_tail (xqueue_t *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  if (queue->tail)
    {
      xlist_t *node = queue->tail;
      xpointer_t data = node->data;

      queue->tail = node->prev;
      if (queue->tail)
        queue->tail->next = NULL;
      else
        queue->head = NULL;
      queue->length--;
      xlist_free_1 (node);

      return data;
    }

  return NULL;
}

/**
 * g_queue_pop_nth:
 * @queue: a #xqueue_t
 * @n: the position of the element
 *
 * Removes the @n'th element of @queue and returns its data.
 *
 * Returns: the element's data, or %NULL if @n is off the end of @queue
 *
 * Since: 2.4
 */
xpointer_t
g_queue_pop_nth (xqueue_t *queue,
                 xuint_t   n)
{
  xlist_t *nth_link;
  xpointer_t result;

  g_return_val_if_fail (queue != NULL, NULL);

  if (n >= queue->length)
    return NULL;

  nth_link = g_queue_peek_nth_link (queue, n);
  result = nth_link->data;

  g_queue_delete_link (queue, nth_link);

  return result;
}

/**
 * g_queue_pop_tail_link:
 * @queue: a #xqueue_t
 *
 * Removes and returns the last element of the queue.
 *
 * Returns: the #xlist_t element at the tail of the queue, or %NULL
 *     if the queue is empty
 */
xlist_t *
g_queue_pop_tail_link (xqueue_t *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  if (queue->tail)
    {
      xlist_t *node = queue->tail;

      queue->tail = node->prev;
      if (queue->tail)
        {
          queue->tail->next = NULL;
          node->prev = NULL;
        }
      else
        queue->head = NULL;
      queue->length--;

      return node;
    }

  return NULL;
}

/**
 * g_queue_pop_nth_link:
 * @queue: a #xqueue_t
 * @n: the link's position
 *
 * Removes and returns the link at the given position.
 *
 * Returns: the @n'th link, or %NULL if @n is off the end of @queue
 *
 * Since: 2.4
 */
xlist_t*
g_queue_pop_nth_link (xqueue_t *queue,
                      xuint_t   n)
{
  xlist_t *link;

  g_return_val_if_fail (queue != NULL, NULL);

  if (n >= queue->length)
    return NULL;

  link = g_queue_peek_nth_link (queue, n);
  g_queue_unlink (queue, link);

  return link;
}

/**
 * g_queue_peek_nth_link:
 * @queue: a #xqueue_t
 * @n: the position of the link
 *
 * Returns the link at the given position
 *
 * Returns: the link at the @n'th position, or %NULL
 *     if @n is off the end of the list
 *
 * Since: 2.4
 */
xlist_t *
g_queue_peek_nth_link (xqueue_t *queue,
                       xuint_t   n)
{
  xlist_t *link;
  xuint_t i;

  g_return_val_if_fail (queue != NULL, NULL);

  if (n >= queue->length)
    return NULL;

  if (n > queue->length / 2)
    {
      n = queue->length - n - 1;

      link = queue->tail;
      for (i = 0; i < n; ++i)
        link = link->prev;
    }
  else
    {
      link = queue->head;
      for (i = 0; i < n; ++i)
        link = link->next;
    }

  return link;
}

/**
 * g_queue_link_index:
 * @queue: a #xqueue_t
 * @link_: a #xlist_t link
 *
 * Returns the position of @link_ in @queue.
 *
 * Returns: the position of @link_, or -1 if the link is
 *     not part of @queue
 *
 * Since: 2.4
 */
xint_t
g_queue_link_index (xqueue_t *queue,
                    xlist_t  *link_)
{
  g_return_val_if_fail (queue != NULL, -1);

  return xlist_position (queue->head, link_);
}

/**
 * g_queue_unlink:
 * @queue: a #xqueue_t
 * @link_: a #xlist_t link that must be part of @queue
 *
 * Unlinks @link_ so that it will no longer be part of @queue.
 * The link is not freed.
 *
 * @link_ must be part of @queue.
 *
 * Since: 2.4
 */
void
g_queue_unlink (xqueue_t *queue,
                xlist_t  *link_)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (link_ != NULL);

  if (link_ == queue->tail)
    queue->tail = queue->tail->prev;

  queue->head = xlist_remove_link (queue->head, link_);
  queue->length--;
}

/**
 * g_queue_delete_link:
 * @queue: a #xqueue_t
 * @link_: a #xlist_t link that must be part of @queue
 *
 * Removes @link_ from @queue and frees it.
 *
 * @link_ must be part of @queue.
 *
 * Since: 2.4
 */
void
g_queue_delete_link (xqueue_t *queue,
                     xlist_t  *link_)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (link_ != NULL);

  g_queue_unlink (queue, link_);
  xlist_free (link_);
}

/**
 * g_queue_peek_head:
 * @queue: a #xqueue_t
 *
 * Returns the first element of the queue.
 *
 * Returns: the data of the first element in the queue, or %NULL
 *     if the queue is empty
 */
xpointer_t
g_queue_peek_head (xqueue_t *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  return queue->head ? queue->head->data : NULL;
}

/**
 * g_queue_peek_tail:
 * @queue: a #xqueue_t
 *
 * Returns the last element of the queue.
 *
 * Returns: the data of the last element in the queue, or %NULL
 *     if the queue is empty
 */
xpointer_t
g_queue_peek_tail (xqueue_t *queue)
{
  g_return_val_if_fail (queue != NULL, NULL);

  return queue->tail ? queue->tail->data : NULL;
}

/**
 * g_queue_peek_nth:
 * @queue: a #xqueue_t
 * @n: the position of the element
 *
 * Returns the @n'th element of @queue.
 *
 * Returns: the data for the @n'th element of @queue,
 *     or %NULL if @n is off the end of @queue
 *
 * Since: 2.4
 */
xpointer_t
g_queue_peek_nth (xqueue_t *queue,
                  xuint_t   n)
{
  xlist_t *link;

  g_return_val_if_fail (queue != NULL, NULL);

  link = g_queue_peek_nth_link (queue, n);

  if (link)
    return link->data;

  return NULL;
}

/**
 * g_queue_index:
 * @queue: a #xqueue_t
 * @data: the data to find
 *
 * Returns the position of the first element in @queue which contains @data.
 *
 * Returns: the position of the first element in @queue which
 *     contains @data, or -1 if no element in @queue contains @data
 *
 * Since: 2.4
 */
xint_t
g_queue_index (xqueue_t        *queue,
               xconstpointer  data)
{
  g_return_val_if_fail (queue != NULL, -1);

  return xlist_index (queue->head, data);
}

/**
 * g_queue_remove:
 * @queue: a #xqueue_t
 * @data: the data to remove
 *
 * Removes the first element in @queue that contains @data.
 *
 * Returns: %TRUE if @data was found and removed from @queue
 *
 * Since: 2.4
 */
xboolean_t
g_queue_remove (xqueue_t        *queue,
                xconstpointer  data)
{
  xlist_t *link;

  g_return_val_if_fail (queue != NULL, FALSE);

  link = xlist_find (queue->head, data);

  if (link)
    g_queue_delete_link (queue, link);

  return (link != NULL);
}

/**
 * g_queue_remove_all:
 * @queue: a #xqueue_t
 * @data: the data to remove
 *
 * Remove all elements whose data equals @data from @queue.
 *
 * Returns: the number of elements removed from @queue
 *
 * Since: 2.4
 */
xuint_t
g_queue_remove_all (xqueue_t        *queue,
                    xconstpointer  data)
{
  xlist_t *list;
  xuint_t old_length;

  g_return_val_if_fail (queue != NULL, 0);

  old_length = queue->length;

  list = queue->head;
  while (list)
    {
      xlist_t *next = list->next;

      if (list->data == data)
        g_queue_delete_link (queue, list);

      list = next;
    }

  return (old_length - queue->length);
}

/**
 * g_queue_insert_before:
 * @queue: a #xqueue_t
 * @sibling: (nullable): a #xlist_t link that must be part of @queue, or %NULL to
 *   push at the tail of the queue.
 * @data: the data to insert
 *
 * Inserts @data into @queue before @sibling.
 *
 * @sibling must be part of @queue. Since GLib 2.44 a %NULL sibling pushes the
 * data at the tail of the queue.
 *
 * Since: 2.4
 */
void
g_queue_insert_before (xqueue_t   *queue,
                       xlist_t    *sibling,
                       xpointer_t  data)
{
  g_return_if_fail (queue != NULL);

  if (sibling == NULL)
    {
      /* We don't use xlist_insert_before() with a NULL sibling because it
       * would be a O(n) operation and we would need to update manually the tail
       * pointer.
       */
      g_queue_push_tail (queue, data);
    }
  else
    {
      queue->head = xlist_insert_before (queue->head, sibling, data);
      queue->length++;
    }
}

/**
 * g_queue_insert_before_link:
 * @queue: a #xqueue_t
 * @sibling: (nullable): a #xlist_t link that must be part of @queue, or %NULL to
 *   push at the tail of the queue.
 * @link_: a #xlist_t link to insert which must not be part of any other list.
 *
 * Inserts @link_ into @queue before @sibling.
 *
 * @sibling must be part of @queue.
 *
 * Since: 2.62
 */
void
g_queue_insert_before_link (xqueue_t   *queue,
                            xlist_t    *sibling,
                            xlist_t    *link_)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (link_ != NULL);
  g_return_if_fail (link_->prev == NULL);
  g_return_if_fail (link_->next == NULL);

  if G_UNLIKELY (sibling == NULL)
    {
      /* We don't use xlist_insert_before_link() with a NULL sibling because it
       * would be a O(n) operation and we would need to update manually the tail
       * pointer.
       */
      g_queue_push_tail_link (queue, link_);
    }
  else
    {
      queue->head = xlist_insert_before_link (queue->head, sibling, link_);
      queue->length++;
    }
}

/**
 * g_queue_insert_after:
 * @queue: a #xqueue_t
 * @sibling: (nullable): a #xlist_t link that must be part of @queue, or %NULL to
 *   push at the head of the queue.
 * @data: the data to insert
 *
 * Inserts @data into @queue after @sibling.
 *
 * @sibling must be part of @queue. Since GLib 2.44 a %NULL sibling pushes the
 * data at the head of the queue.
 *
 * Since: 2.4
 */
void
g_queue_insert_after (xqueue_t   *queue,
                      xlist_t    *sibling,
                      xpointer_t  data)
{
  g_return_if_fail (queue != NULL);

  if (sibling == NULL)
    g_queue_push_head (queue, data);
  else
    g_queue_insert_before (queue, sibling->next, data);
}

/**
 * g_queue_insert_after_link:
 * @queue: a #xqueue_t
 * @sibling: (nullable): a #xlist_t link that must be part of @queue, or %NULL to
 *   push at the head of the queue.
 * @link_: a #xlist_t link to insert which must not be part of any other list.
 *
 * Inserts @link_ into @queue after @sibling.
 *
 * @sibling must be part of @queue.
 *
 * Since: 2.62
 */
void
g_queue_insert_after_link (xqueue_t   *queue,
                           xlist_t    *sibling,
                           xlist_t    *link_)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (link_ != NULL);
  g_return_if_fail (link_->prev == NULL);
  g_return_if_fail (link_->next == NULL);

  if G_UNLIKELY (sibling == NULL)
    g_queue_push_head_link (queue, link_);
  else
    g_queue_insert_before_link (queue, sibling->next, link_);
}

/**
 * g_queue_insert_sorted:
 * @queue: a #xqueue_t
 * @data: the data to insert
 * @func: the #GCompareDataFunc used to compare elements in the queue. It is
 *     called with two elements of the @queue and @user_data. It should
 *     return 0 if the elements are equal, a negative value if the first
 *     element comes before the second, and a positive value if the second
 *     element comes before the first.
 * @user_data: user data passed to @func
 *
 * Inserts @data into @queue using @func to determine the new position.
 *
 * Since: 2.4
 */
void
g_queue_insert_sorted (xqueue_t           *queue,
                       xpointer_t          data,
                       GCompareDataFunc  func,
                       xpointer_t          user_data)
{
  xlist_t *list;

  g_return_if_fail (queue != NULL);

  list = queue->head;
  while (list && func (list->data, data, user_data) < 0)
    list = list->next;

  g_queue_insert_before (queue, list, data);
}
