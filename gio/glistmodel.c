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

#include "glistmodel.h"
#include "glibintl.h"
#include "gmarshal-internal.h"

G_DEFINE_INTERFACE (xlist_model_t, xlist_model, XTYPE_OBJECT)

/**
 * SECTION:glistmodel
 * @title: xlist_model_t
 * @short_description: An interface describing a dynamic list of objects
 * @include: gio/gio.h
 * @see_also: #xlist_store_t
 *
 * #xlist_model_t is an interface that represents a mutable list of
 * #GObjects. Its main intention is as a model for various widgets in
 * user interfaces, such as list views, but it can also be used as a
 * convenient method of returning lists of data, with support for
 * updates.
 *
 * Each object in the list may also report changes in itself via some
 * mechanism (normally the #xobject_t::notify signal).  Taken together
 * with the #xlist_model_t::items-changed signal, this provides for a list
 * that can change its membership, and in which the members can change
 * their individual properties.
 *
 * A good example would be the list of visible wireless network access
 * points, where each access point can report dynamic properties such as
 * signal strength.
 *
 * It is important to note that the #xlist_model_t itself does not report
 * changes to the individual items.  It only reports changes to the list
 * membership.  If you want to observe changes to the objects themselves
 * then you need to connect signals to the objects that you are
 * interested in.
 *
 * All items in a #xlist_model_t are of (or derived from) the same type.
 * xlist_model_get_item_type() returns that type.  The type may be an
 * interface, in which case all objects in the list must implement it.
 *
 * The semantics are close to that of an array:
 * xlist_model_get_n_items() returns the number of items in the list and
 * xlist_model_get_item() returns an item at a (0-based) position. In
 * order to allow implementations to calculate the list length lazily,
 * you can also iterate over items: starting from 0, repeatedly call
 * xlist_model_get_item() until it returns %NULL.
 *
 * An implementation may create objects lazily, but must take care to
 * return the same object for a given position until all references to
 * it are gone.
 *
 * On the other side, a consumer is expected only to hold references on
 * objects that are currently "user visible", in order to facilitate the
 * maximum level of laziness in the implementation of the list and to
 * reduce the required number of signal connections at a given time.
 *
 * This interface is intended only to be used from a single thread.  The
 * thread in which it is appropriate to use it depends on the particular
 * implementation, but typically it will be from the thread that owns
 * the [thread-default main context][g-main-context-push-thread-default]
 * in effect at the time that the model was created.
 */

/**
 * xlist_model_interface_t:
 * @x_iface: parent #xtype_interface_t
 * @get_item_type: the virtual function pointer for xlist_model_get_item_type()
 * @get_n_items: the virtual function pointer for xlist_model_get_n_items()
 * @get_item: the virtual function pointer for xlist_model_get_item()
 *
 * The virtual function table for #xlist_model_t.
 *
 * Since: 2.44
 */

/**
 * xlist_model_interface_t::get_item:
 * @list: a #xlist_model_t
 * @position: the position of the item to fetch
 *
 * Get the item at @position. If @position is greater than the number of
 * items in @list, %NULL is returned.
 *
 * %NULL is never returned for an index that is smaller than the length
 * of the list.  See xlist_model_get_n_items().
 *
 * Returns: (type xobject_t) (transfer full) (nullable): the object at @position.
 *
 * Since: 2.44
 */

/**
 * xlist_model_t:
 *
 * #xlist_model_t is an opaque data structure and can only be accessed
 * using the following functions.
 **/

static xuint_t xlist_model_changed_signal;

static void
xlist_model_default_init (xlist_model_interface_t *iface)
{
  /**
   * xlist_model_t::items-changed:
   * @list: the #xlist_model_t that changed
   * @position: the position at which @list changed
   * @removed: the number of items removed
   * @added: the number of items added
   *
   * This signal is emitted whenever items were added to or removed
   * from @list. At @position, @removed items were removed and @added
   * items were added in their place.
   *
   * Note: If `removed != added`, the positions of all later items
   * in the model change.
   *
   * Since: 2.44
   */
  xlist_model_changed_signal = xsignal_new (I_("items-changed"),
                                              XTYPE_LIST_MODEL,
                                              G_SIGNAL_RUN_LAST,
                                              0,
                                              NULL, NULL,
                                              _g_cclosure_marshal_VOID__UINT_UINT_UINT,
                                              XTYPE_NONE,
                                              3, XTYPE_UINT, XTYPE_UINT, XTYPE_UINT);
  xsignal_set_va_marshaller (xlist_model_changed_signal,
                              XTYPE_FROM_INTERFACE (iface),
                              _g_cclosure_marshal_VOID__UINT_UINT_UINTv);
}

/**
 * xlist_model_get_item_type:
 * @list: a #xlist_model_t
 *
 * Gets the type of the items in @list.
 *
 * All items returned from xlist_model_get_item() are of the type
 * returned by this function, or a subtype, or if the type is an
 * interface, they are an implementation of that interface.
 *
 * The item type of a #xlist_model_t can not change during the life of the
 * model.
 *
 * Returns: the #xtype_t of the items contained in @list.
 *
 * Since: 2.44
 */
xtype_t
xlist_model_get_item_type (xlist_model_t *list)
{
  g_return_val_if_fail (X_IS_LIST_MODEL (list), XTYPE_NONE);

  return XLIST_MODEL_GET_IFACE (list)->get_item_type (list);
}

/**
 * xlist_model_get_n_items:
 * @list: a #xlist_model_t
 *
 * Gets the number of items in @list.
 *
 * Depending on the model implementation, calling this function may be
 * less efficient than iterating the list with increasing values for
 * @position until xlist_model_get_item() returns %NULL.
 *
 * Returns: the number of items in @list.
 *
 * Since: 2.44
 */
xuint_t
xlist_model_get_n_items (xlist_model_t *list)
{
  g_return_val_if_fail (X_IS_LIST_MODEL (list), 0);

  return XLIST_MODEL_GET_IFACE (list)->get_n_items (list);
}

/**
 * xlist_model_get_item: (skip)
 * @list: a #xlist_model_t
 * @position: the position of the item to fetch
 *
 * Get the item at @position.
 *
 * If @position is greater than the number of items in @list, %NULL is
 * returned.
 *
 * %NULL is never returned for an index that is smaller than the length
 * of the list.
 *
 * See also: xlist_model_get_n_items()
 *
 * Returns: (transfer full) (nullable): the item at @position.
 *
 * Since: 2.44
 */
xpointer_t
xlist_model_get_item (xlist_model_t *list,
                       xuint_t       position)
{
  g_return_val_if_fail (X_IS_LIST_MODEL (list), NULL);

  return XLIST_MODEL_GET_IFACE (list)->get_item (list, position);
}

/**
 * xlist_model_get_object: (rename-to xlist_model_get_item)
 * @list: a #xlist_model_t
 * @position: the position of the item to fetch
 *
 * Get the item at @position.
 *
 * If @position is greater than the number of items in @list, %NULL is
 * returned.
 *
 * %NULL is never returned for an index that is smaller than the length
 * of the list.
 *
 * This function is meant to be used by language bindings in place
 * of xlist_model_get_item().
 *
 * See also: xlist_model_get_n_items()
 *
 * Returns: (transfer full) (nullable): the object at @position.
 *
 * Since: 2.44
 */
xobject_t *
xlist_model_get_object (xlist_model_t *list,
                         xuint_t       position)
{
  xpointer_t item;

  g_return_val_if_fail (X_IS_LIST_MODEL (list), NULL);

  item = xlist_model_get_item (list, position);

  return G_OBJECT (item);
}

/**
 * xlist_model_items_changed:
 * @list: a #xlist_model_t
 * @position: the position at which @list changed
 * @removed: the number of items removed
 * @added: the number of items added
 *
 * Emits the #xlist_model_t::items-changed signal on @list.
 *
 * This function should only be called by classes implementing
 * #xlist_model_t. It has to be called after the internal representation
 * of @list has been updated, because handlers connected to this signal
 * might query the new state of the list.
 *
 * Implementations must only make changes to the model (as visible to
 * its consumer) in places that will not cause problems for that
 * consumer.  For models that are driven directly by a write API (such
 * as #xlist_store_t), changes can be reported in response to uses of that
 * API.  For models that represent remote data, changes should only be
 * made from a fresh mainloop dispatch.  It is particularly not
 * permitted to make changes in response to a call to the #xlist_model_t
 * consumer API.
 *
 * Stated another way: in general, it is assumed that code making a
 * series of accesses to the model via the API, without returning to the
 * mainloop, and without calling other code, will continue to view the
 * same contents of the model.
 *
 * Since: 2.44
 */
void
xlist_model_items_changed (xlist_model_t *list,
                            xuint_t       position,
                            xuint_t       removed,
                            xuint_t       added)
{
  g_return_if_fail (X_IS_LIST_MODEL (list));

  xsignal_emit (list, xlist_model_changed_signal, 0, position, removed, added);
}
