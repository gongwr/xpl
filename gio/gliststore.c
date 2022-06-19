/*
 * Copyright 2015 Lars Uebernickel
 * Copyright 2015 Ryan Lortie
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Lars Uebernickel <lars@uebernic.de>
 *     Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gliststore.h"
#include "glistmodel.h"

/**
 * SECTION:gliststore
 * @title: xlist_store_t
 * @short_description: A simple implementation of #xlist_model_t
 * @include: gio/gio.h
 *
 * #xlist_store_t is a simple implementation of #xlist_model_t that stores all
 * items in memory.
 *
 * It provides insertions, deletions, and lookups in logarithmic time
 * with a fast path for the common case of iterating the list linearly.
 */

/**
 * xlist_store_t:
 *
 * #xlist_store_t is an opaque data structure and can only be accessed
 * using the following functions.
 **/

struct _GListStore
{
  xobject_t parent_instance;

  xtype_t item_type;
  xsequence_t *items;

  /* cache */
  xuint_t last_position;
  GSequenceIter *last_iter;
  xboolean_t last_position_valid;
};

enum
{
  PROP_0,
  PROP_ITEM_TYPE,
  N_PROPERTIES
};

static void xlist_store_iface_init (xlist_model_interface_t *iface);

G_DEFINE_TYPE_WITH_CODE (xlist_store, xlist_store, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_LIST_MODEL, xlist_store_iface_init));

static void
xlist_store_items_changed (xlist_store_t *store,
                            xuint_t       position,
                            xuint_t       removed,
                            xuint_t       added)
{
  /* check if the iter cache may have been invalidated */
  if (position <= store->last_position)
    {
      store->last_iter = NULL;
      store->last_position = 0;
      store->last_position_valid = FALSE;
    }

  xlist_model_items_changed (XLIST_MODEL (store), position, removed, added);
}

static void
xlist_store_dispose (xobject_t *object)
{
  xlist_store_t *store = XLIST_STORE (object);

  g_clear_pointer (&store->items, g_sequence_free);

  XOBJECT_CLASS (xlist_store_parent_class)->dispose (object);
}

static void
xlist_store_get_property (xobject_t    *object,
                           xuint_t       property_id,
                           xvalue_t     *value,
                           xparam_spec_t *pspec)
{
  xlist_store_t *store = XLIST_STORE (object);

  switch (property_id)
    {
    case PROP_ITEM_TYPE:
      xvalue_set_gtype (value, store->item_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
xlist_store_set_property (xobject_t      *object,
                           xuint_t         property_id,
                           const xvalue_t *value,
                           xparam_spec_t   *pspec)
{
  xlist_store_t *store = XLIST_STORE (object);

  switch (property_id)
    {
    case PROP_ITEM_TYPE: /* construct-only */
      xassert (xtype_is_a (xvalue_get_gtype (value), XTYPE_OBJECT));
      store->item_type = xvalue_get_gtype (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
xlist_store_class_init (GListStoreClass *klass)
{
  xobject_class_t *object_class = XOBJECT_CLASS (klass);

  object_class->dispose = xlist_store_dispose;
  object_class->get_property = xlist_store_get_property;
  object_class->set_property = xlist_store_set_property;

  /**
   * xlist_store_t:item-type:
   *
   * The type of items contained in this list store. Items must be
   * subclasses of #xobject_t.
   *
   * Since: 2.44
   **/
  xobject_class_install_property (object_class, PROP_ITEM_TYPE,
    xparam_spec_gtype ("item-type", "", "", XTYPE_OBJECT,
                        XPARAM_CONSTRUCT_ONLY | XPARAM_READWRITE | XPARAM_STATIC_STRINGS));
}

static xtype_t
xlist_store_get_item_type (xlist_model_t *list)
{
  xlist_store_t *store = XLIST_STORE (list);

  return store->item_type;
}

static xuint_t
xlist_store_get_n_items (xlist_model_t *list)
{
  xlist_store_t *store = XLIST_STORE (list);

  return g_sequence_get_length (store->items);
}

static xpointer_t
xlist_store_get_item (xlist_model_t *list,
                       xuint_t       position)
{
  xlist_store_t *store = XLIST_STORE (list);
  GSequenceIter *it = NULL;

  if (store->last_position_valid)
    {
      if (position < G_MAXUINT && store->last_position == position + 1)
        it = g_sequence_iter_prev (store->last_iter);
      else if (position > 0 && store->last_position == position - 1)
        it = g_sequence_iter_next (store->last_iter);
      else if (store->last_position == position)
        it = store->last_iter;
    }

  if (it == NULL)
    it = g_sequence_get_iter_at_pos (store->items, position);

  store->last_iter = it;
  store->last_position = position;
  store->last_position_valid = TRUE;

  if (g_sequence_iter_is_end (it))
    return NULL;
  else
    return xobject_ref (g_sequence_get (it));
}

static void
xlist_store_iface_init (xlist_model_interface_t *iface)
{
  iface->get_item_type = xlist_store_get_item_type;
  iface->get_n_items = xlist_store_get_n_items;
  iface->get_item = xlist_store_get_item;
}

static void
xlist_store_init (xlist_store_t *store)
{
  store->items = g_sequence_new (xobject_unref);
  store->last_position = 0;
  store->last_position_valid = FALSE;
}

/**
 * xlist_store_new:
 * @item_type: the #xtype_t of items in the list
 *
 * Creates a new #xlist_store_t with items of type @item_type. @item_type
 * must be a subclass of #xobject_t.
 *
 * Returns: a new #xlist_store_t
 * Since: 2.44
 */
xlist_store_t *
xlist_store_new (xtype_t item_type)
{
  /* We only allow GObjects as item types right now. This might change
   * in the future.
   */
  xreturn_val_if_fail (xtype_is_a (item_type, XTYPE_OBJECT), NULL);

  return xobject_new (XTYPE_LIST_STORE,
                       "item-type", item_type,
                       NULL);
}

/**
 * xlist_store_insert:
 * @store: a #xlist_store_t
 * @position: the position at which to insert the new item
 * @item: (type xobject_t): the new item
 *
 * Inserts @item into @store at @position. @item must be of type
 * #xlist_store_t:item-type or derived from it. @position must be smaller
 * than the length of the list, or equal to it to append.
 *
 * This function takes a ref on @item.
 *
 * Use xlist_store_splice() to insert multiple items at the same time
 * efficiently.
 *
 * Since: 2.44
 */
void
xlist_store_insert (xlist_store_t *store,
                     xuint_t       position,
                     xpointer_t    item)
{
  GSequenceIter *it;

  g_return_if_fail (X_IS_LIST_STORE (store));
  g_return_if_fail (xtype_is_a (G_OBJECT_TYPE (item), store->item_type));
  g_return_if_fail (position <= (xuint_t) g_sequence_get_length (store->items));

  it = g_sequence_get_iter_at_pos (store->items, position);
  g_sequence_insert_before (it, xobject_ref (item));

  xlist_store_items_changed (store, position, 0, 1);
}

/**
 * xlist_store_insert_sorted:
 * @store: a #xlist_store_t
 * @item: (type xobject_t): the new item
 * @compare_func: (scope call): pairwise comparison function for sorting
 * @user_data: (closure): user data for @compare_func
 *
 * Inserts @item into @store at a position to be determined by the
 * @compare_func.
 *
 * The list must already be sorted before calling this function or the
 * result is undefined.  Usually you would approach this by only ever
 * inserting items by way of this function.
 *
 * This function takes a ref on @item.
 *
 * Returns: the position at which @item was inserted
 *
 * Since: 2.44
 */
xuint_t
xlist_store_insert_sorted (xlist_store_t       *store,
                            xpointer_t          item,
                            GCompareDataFunc  compare_func,
                            xpointer_t          user_data)
{
  GSequenceIter *it;
  xuint_t position;

  xreturn_val_if_fail (X_IS_LIST_STORE (store), 0);
  xreturn_val_if_fail (xtype_is_a (G_OBJECT_TYPE (item), store->item_type), 0);
  xreturn_val_if_fail (compare_func != NULL, 0);

  it = g_sequence_insert_sorted (store->items, xobject_ref (item), compare_func, user_data);
  position = g_sequence_iter_get_position (it);

  xlist_store_items_changed (store, position, 0, 1);

  return position;
}

/**
 * xlist_store_sort:
 * @store: a #xlist_store_t
 * @compare_func: (scope call): pairwise comparison function for sorting
 * @user_data: (closure): user data for @compare_func
 *
 * Sort the items in @store according to @compare_func.
 *
 * Since: 2.46
 */
void
xlist_store_sort (xlist_store_t       *store,
                   GCompareDataFunc  compare_func,
                   xpointer_t          user_data)
{
  xint_t n_items;

  g_return_if_fail (X_IS_LIST_STORE (store));
  g_return_if_fail (compare_func != NULL);

  g_sequence_sort (store->items, compare_func, user_data);

  n_items = g_sequence_get_length (store->items);
  xlist_store_items_changed (store, 0, n_items, n_items);
}

/**
 * xlist_store_append:
 * @store: a #xlist_store_t
 * @item: (type xobject_t): the new item
 *
 * Appends @item to @store. @item must be of type #xlist_store_t:item-type.
 *
 * This function takes a ref on @item.
 *
 * Use xlist_store_splice() to append multiple items at the same time
 * efficiently.
 *
 * Since: 2.44
 */
void
xlist_store_append (xlist_store_t *store,
                     xpointer_t    item)
{
  xuint_t n_items;

  g_return_if_fail (X_IS_LIST_STORE (store));
  g_return_if_fail (xtype_is_a (G_OBJECT_TYPE (item), store->item_type));

  n_items = g_sequence_get_length (store->items);
  g_sequence_append (store->items, xobject_ref (item));

  xlist_store_items_changed (store, n_items, 0, 1);
}

/**
 * xlist_store_remove:
 * @store: a #xlist_store_t
 * @position: the position of the item that is to be removed
 *
 * Removes the item from @store that is at @position. @position must be
 * smaller than the current length of the list.
 *
 * Use xlist_store_splice() to remove multiple items at the same time
 * efficiently.
 *
 * Since: 2.44
 */
void
xlist_store_remove (xlist_store_t *store,
                     xuint_t       position)
{
  GSequenceIter *it;

  g_return_if_fail (X_IS_LIST_STORE (store));

  it = g_sequence_get_iter_at_pos (store->items, position);
  g_return_if_fail (!g_sequence_iter_is_end (it));

  g_sequence_remove (it);
  xlist_store_items_changed (store, position, 1, 0);
}

/**
 * xlist_store_remove_all:
 * @store: a #xlist_store_t
 *
 * Removes all items from @store.
 *
 * Since: 2.44
 */
void
xlist_store_remove_all (xlist_store_t *store)
{
  xuint_t n_items;

  g_return_if_fail (X_IS_LIST_STORE (store));

  n_items = g_sequence_get_length (store->items);
  g_sequence_remove_range (g_sequence_get_begin_iter (store->items),
                           g_sequence_get_end_iter (store->items));

  xlist_store_items_changed (store, 0, n_items, 0);
}

/**
 * xlist_store_splice:
 * @store: a #xlist_store_t
 * @position: the position at which to make the change
 * @n_removals: the number of items to remove
 * @additions: (array length=n_additions) (element-type xobject_t): the items to add
 * @n_additions: the number of items to add
 *
 * Changes @store by removing @n_removals items and adding @n_additions
 * items to it. @additions must contain @n_additions items of type
 * #xlist_store_t:item-type.  %NULL is not permitted.
 *
 * This function is more efficient than xlist_store_insert() and
 * xlist_store_remove(), because it only emits
 * #xlist_model_t::items-changed once for the change.
 *
 * This function takes a ref on each item in @additions.
 *
 * The parameters @position and @n_removals must be correct (ie:
 * @position + @n_removals must be less than or equal to the length of
 * the list at the time this function is called).
 *
 * Since: 2.44
 */
void
xlist_store_splice (xlist_store_t *store,
                     xuint_t       position,
                     xuint_t       n_removals,
                     xpointer_t   *additions,
                     xuint_t       n_additions)
{
  GSequenceIter *it;
  xuint_t n_items;

  g_return_if_fail (X_IS_LIST_STORE (store));
  g_return_if_fail (position + n_removals >= position); /* overflow */

  n_items = g_sequence_get_length (store->items);
  g_return_if_fail (position + n_removals <= n_items);

  it = g_sequence_get_iter_at_pos (store->items, position);

  if (n_removals)
    {
      GSequenceIter *end;

      end = g_sequence_iter_move (it, n_removals);
      g_sequence_remove_range (it, end);

      it = end;
    }

  if (n_additions)
    {
      xuint_t i;

      for (i = 0; i < n_additions; i++)
        {
          if G_UNLIKELY (!xtype_is_a (G_OBJECT_TYPE (additions[i]), store->item_type))
            {
              g_critical ("%s: item %d is a %s instead of a %s.  xlist_store_t is now in an undefined state.",
                          G_STRFUNC, i, G_OBJECT_TYPE_NAME (additions[i]), xtype_name (store->item_type));
              return;
            }

          g_sequence_insert_before (it, xobject_ref (additions[i]));
        }
    }

  xlist_store_items_changed (store, position, n_removals, n_additions);
}

/**
 * xlist_store_find_with_equal_func:
 * @store: a #xlist_store_t
 * @item: (type xobject_t): an item
 * @equal_func: (scope call): A custom equality check function
 * @position: (out) (optional): the first position of @item, if it was found.
 *
 * Looks up the given @item in the list store by looping over the items and
 * comparing them with @compare_func until the first occurrence of @item which
 * matches. If @item was not found, then @position will not be set, and this
 * method will return %FALSE.
 *
 * Returns: Whether @store contains @item. If it was found, @position will be
 * set to the position where @item occurred for the first time.
 *
 * Since: 2.64
 */
xboolean_t
xlist_store_find_with_equal_func (xlist_store_t *store,
                                   xpointer_t    item,
                                   GEqualFunc  equal_func,
                                   xuint_t      *position)
{
  GSequenceIter *iter, *begin, *end;

  xreturn_val_if_fail (X_IS_LIST_STORE (store), FALSE);
  xreturn_val_if_fail (xtype_is_a (G_OBJECT_TYPE (item), store->item_type),
                        FALSE);
  xreturn_val_if_fail (equal_func != NULL, FALSE);

  /* NOTE: We can't use g_sequence_lookup() or g_sequence_search(), because we
   * can't assume the sequence is sorted. */
  begin = g_sequence_get_begin_iter (store->items);
  end = g_sequence_get_end_iter (store->items);

  iter = begin;
  while (iter != end)
    {
      xpointer_t iter_item;

      iter_item = g_sequence_get (iter);
      if (equal_func (iter_item, item))
        {
          if (position)
            *position = g_sequence_iter_get_position (iter);
          return TRUE;
        }

      iter = g_sequence_iter_next (iter);
    }

  return FALSE;
}

/**
 * xlist_store_find:
 * @store: a #xlist_store_t
 * @item: (type xobject_t): an item
 * @position: (out) (optional): the first position of @item, if it was found.
 *
 * Looks up the given @item in the list store by looping over the items until
 * the first occurrence of @item. If @item was not found, then @position will
 * not be set, and this method will return %FALSE.
 *
 * If you need to compare the two items with a custom comparison function, use
 * xlist_store_find_with_equal_func() with a custom #GEqualFunc instead.
 *
 * Returns: Whether @store contains @item. If it was found, @position will be
 * set to the position where @item occurred for the first time.
 *
 * Since: 2.64
 */
xboolean_t
xlist_store_find (xlist_store_t *store,
                   xpointer_t    item,
                   xuint_t      *position)
{
  return xlist_store_find_with_equal_func (store,
                                            item,
                                            g_direct_equal,
                                            position);
}
