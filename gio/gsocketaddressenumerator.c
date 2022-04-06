/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 */

#include "config.h"
#include "gsocketaddressenumerator.h"
#include "glibintl.h"

#include "gtask.h"

/**
 * SECTION:gsocketaddressenumerator
 * @short_description: Enumerator for socket addresses
 * @include: gio/gio.h
 *
 * #xsocket_address_enumerator_t is an enumerator type for #xsocket_address_t
 * instances. It is returned by enumeration functions such as
 * xsocket_connectable_enumerate(), which returns a #xsocket_address_enumerator_t
 * to list each #xsocket_address_t which could be used to connect to that
 * #xsocket_connectable_t.
 *
 * Enumeration is typically a blocking operation, so the asynchronous methods
 * xsocket_address_enumerator_next_async() and
 * xsocket_address_enumerator_next_finish() should be used where possible.
 *
 * Each #xsocket_address_enumerator_t can only be enumerated once. Once
 * xsocket_address_enumerator_next() has returned %NULL, further
 * enumeration with that #xsocket_address_enumerator_t is not possible, and it can
 * be unreffed.
 */

G_DEFINE_ABSTRACT_TYPE (xsocket_address_enumerator_t, xsocket_address_enumerator, XTYPE_OBJECT)

static void            xsocket_address_enumerator_real_next_async  (xsocket_address_enumerator_t  *enumerator,
								     xcancellable_t              *cancellable,
								     xasync_ready_callback_t        callback,
								     xpointer_t                   user_data);
static xsocket_address_t *xsocket_address_enumerator_real_next_finish (xsocket_address_enumerator_t  *enumerator,
								     xasync_result_t              *result,
								     xerror_t                   **error);

static void
xsocket_address_enumerator_init (xsocket_address_enumerator_t *enumerator)
{
}

static void
xsocket_address_enumerator_class_init (GSocketAddressEnumeratorClass *enumerator_class)
{
  enumerator_class->next_async = xsocket_address_enumerator_real_next_async;
  enumerator_class->next_finish = xsocket_address_enumerator_real_next_finish;
}

/**
 * xsocket_address_enumerator_next:
 * @enumerator: a #xsocket_address_enumerator_t
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t.
 *
 * Retrieves the next #xsocket_address_t from @enumerator. Note that this
 * may block for some amount of time. (Eg, a #xnetwork_address_t may need
 * to do a DNS lookup before it can return an address.) Use
 * xsocket_address_enumerator_next_async() if you need to avoid
 * blocking.
 *
 * If @enumerator is expected to yield addresses, but for some reason
 * is unable to (eg, because of a DNS error), then the first call to
 * xsocket_address_enumerator_next() will return an appropriate error
 * in *@error. However, if the first call to
 * xsocket_address_enumerator_next() succeeds, then any further
 * internal errors (other than @cancellable being triggered) will be
 * ignored.
 *
 * Returns: (transfer full): a #xsocket_address_t (owned by the caller), or %NULL on
 *     error (in which case *@error will be set) or if there are no
 *     more addresses.
 */
xsocket_address_t *
xsocket_address_enumerator_next (xsocket_address_enumerator_t  *enumerator,
				  xcancellable_t              *cancellable,
				  xerror_t                   **error)
{
  GSocketAddressEnumeratorClass *klass;

  g_return_val_if_fail (X_IS_SOCKET_ADDRESS_ENUMERATOR (enumerator), NULL);

  klass = XSOCKET_ADDRESS_ENUMERATOR_GET_CLASS (enumerator);

  return (* klass->next) (enumerator, cancellable, error);
}

/* Default implementation just calls the synchronous method; this can
 * be used if the implementation already knows all of its addresses,
 * and so the synchronous method will never block.
 */
static void
xsocket_address_enumerator_real_next_async (xsocket_address_enumerator_t *enumerator,
					     xcancellable_t             *cancellable,
					     xasync_ready_callback_t       callback,
					     xpointer_t                  user_data)
{
  xtask_t *task;
  xsocket_address_t *address;
  xerror_t *error = NULL;

  task = xtask_new (enumerator, NULL, callback, user_data);
  xtask_set_source_tag (task, xsocket_address_enumerator_real_next_async);

  address = xsocket_address_enumerator_next (enumerator, cancellable, &error);
  if (error)
    xtask_return_error (task, error);
  else
    xtask_return_pointer (task, address, xobject_unref);

  xobject_unref (task);
}

/**
 * xsocket_address_enumerator_next_async:
 * @enumerator: a #xsocket_address_enumerator_t
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): a #xasync_ready_callback_t to call when the request
 *     is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously retrieves the next #xsocket_address_t from @enumerator
 * and then calls @callback, which must call
 * xsocket_address_enumerator_next_finish() to get the result.
 *
 * It is an error to call this multiple times before the previous callback has finished.
 */
void
xsocket_address_enumerator_next_async (xsocket_address_enumerator_t *enumerator,
					xcancellable_t             *cancellable,
					xasync_ready_callback_t       callback,
					xpointer_t                  user_data)
{
  GSocketAddressEnumeratorClass *klass;

  g_return_if_fail (X_IS_SOCKET_ADDRESS_ENUMERATOR (enumerator));

  klass = XSOCKET_ADDRESS_ENUMERATOR_GET_CLASS (enumerator);

  (* klass->next_async) (enumerator, cancellable, callback, user_data);
}

static xsocket_address_t *
xsocket_address_enumerator_real_next_finish (xsocket_address_enumerator_t  *enumerator,
					      xasync_result_t              *result,
					      xerror_t                   **error)
{
  g_return_val_if_fail (xtask_is_valid (result, enumerator), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}

/**
 * xsocket_address_enumerator_next_finish:
 * @enumerator: a #xsocket_address_enumerator_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t
 *
 * Retrieves the result of a completed call to
 * xsocket_address_enumerator_next_async(). See
 * xsocket_address_enumerator_next() for more information about
 * error handling.
 *
 * Returns: (transfer full): a #xsocket_address_t (owned by the caller), or %NULL on
 *     error (in which case *@error will be set) or if there are no
 *     more addresses.
 */
xsocket_address_t *
xsocket_address_enumerator_next_finish (xsocket_address_enumerator_t  *enumerator,
					 xasync_result_t              *result,
					 xerror_t                   **error)
{
  GSocketAddressEnumeratorClass *klass;

  g_return_val_if_fail (X_IS_SOCKET_ADDRESS_ENUMERATOR (enumerator), NULL);

  klass = XSOCKET_ADDRESS_ENUMERATOR_GET_CLASS (enumerator);

  return (* klass->next_finish) (enumerator, result, error);
}
