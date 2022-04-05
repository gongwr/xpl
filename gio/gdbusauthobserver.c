/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include "config.h"

#include "gdbusauthobserver.h"
#include "gcredentials.h"
#include "gioenumtypes.h"
#include "giostream.h"
#include "gdbusprivate.h"

#include "glibintl.h"
#include "gmarshal-internal.h"

/**
 * SECTION:gdbusauthobserver
 * @short_description: Object used for authenticating connections
 * @include: gio/gio.h
 *
 * The #GDBusAuthObserver type provides a mechanism for participating
 * in how a #GDBusServer (or a #GDBusConnection) authenticates remote
 * peers. Simply instantiate a #GDBusAuthObserver and connect to the
 * signals you are interested in. Note that new signals may be added
 * in the future
 *
 * ## Controlling Authentication Mechanisms
 *
 * By default, a #GDBusServer or server-side #GDBusConnection will allow
 * any authentication mechanism to be used. If you only
 * want to allow D-Bus connections with the `EXTERNAL` mechanism,
 * which makes use of credentials passing and is the recommended
 * mechanism for modern Unix platforms such as Linux and the BSD family,
 * you would use a signal handler like this:
 *
 * |[<!-- language="C" -->
 * static xboolean_t
 * on_allow_mechanism (GDBusAuthObserver *observer,
 *                     const xchar_t       *mechanism,
 *                     xpointer_t           user_data)
 * {
 *   if (g_strcmp0 (mechanism, "EXTERNAL") == 0)
 *     {
 *       return TRUE;
 *     }
 *
 *   return FALSE;
 * }
 * ]|
 *
 * ## Controlling Authorization # {#auth-observer}
 *
 * By default, a #GDBusServer or server-side #GDBusConnection will accept
 * connections from any successfully authenticated user (but not from
 * anonymous connections using the `ANONYMOUS` mechanism). If you only
 * want to allow D-Bus connections from processes owned by the same uid
 * as the server, since GLib 2.68, you should use the
 * %G_DBUS_SERVER_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER flag. It’s equivalent
 * to the following signal handler:
 *
 * |[<!-- language="C" -->
 * static xboolean_t
 * on_authorize_authenticated_peer (GDBusAuthObserver *observer,
 *                                  xio_stream_t         *stream,
 *                                  GCredentials      *credentials,
 *                                  xpointer_t           user_data)
 * {
 *   xboolean_t authorized;
 *
 *   authorized = FALSE;
 *   if (credentials != NULL)
 *     {
 *       GCredentials *own_credentials;
 *       own_credentials = g_credentials_new ();
 *       if (g_credentials_is_same_user (credentials, own_credentials, NULL))
 *         authorized = TRUE;
 *       g_object_unref (own_credentials);
 *     }
 *
 *   return authorized;
 * }
 * ]|
 */

typedef struct _GDBusAuthObserverClass GDBusAuthObserverClass;

/**
 * GDBusAuthObserverClass:
 * @authorize_authenticated_peer: Signal class handler for the #GDBusAuthObserver::authorize-authenticated-peer signal.
 *
 * Class structure for #GDBusAuthObserverClass.
 *
 * Since: 2.26
 */
struct _GDBusAuthObserverClass
{
  /*< private >*/
  xobject_class_t parent_class;

  /*< public >*/

  /* Signals */
  xboolean_t (*authorize_authenticated_peer) (GDBusAuthObserver  *observer,
                                            xio_stream_t          *stream,
                                            GCredentials       *credentials);

  xboolean_t (*allow_mechanism) (GDBusAuthObserver  *observer,
                               const xchar_t        *mechanism);
};

/**
 * GDBusAuthObserver:
 *
 * The #GDBusAuthObserver structure contains only private data and
 * should only be accessed using the provided API.
 *
 * Since: 2.26
 */
struct _GDBusAuthObserver
{
  xobject_t parent_instance;
};

enum
{
  AUTHORIZE_AUTHENTICATED_PEER_SIGNAL,
  ALLOW_MECHANISM_SIGNAL,
  LAST_SIGNAL,
};

static xuint_t signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GDBusAuthObserver, g_dbus_auth_observer, XTYPE_OBJECT)

/* ---------------------------------------------------------------------------------------------------- */

static void
g_dbus_auth_observer_finalize (xobject_t *object)
{
  G_OBJECT_CLASS (g_dbus_auth_observer_parent_class)->finalize (object);
}

static xboolean_t
g_dbus_auth_observer_authorize_authenticated_peer_real (GDBusAuthObserver  *observer,
                                                        xio_stream_t          *stream,
                                                        GCredentials       *credentials)
{
  return TRUE;
}

static xboolean_t
g_dbus_auth_observer_allow_mechanism_real (GDBusAuthObserver  *observer,
                                           const xchar_t        *mechanism)
{
  return TRUE;
}

static void
g_dbus_auth_observer_class_init (GDBusAuthObserverClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_dbus_auth_observer_finalize;

  klass->authorize_authenticated_peer = g_dbus_auth_observer_authorize_authenticated_peer_real;
  klass->allow_mechanism = g_dbus_auth_observer_allow_mechanism_real;

  /**
   * GDBusAuthObserver::authorize-authenticated-peer:
   * @observer: The #GDBusAuthObserver emitting the signal.
   * @stream: A #xio_stream_t for the #GDBusConnection.
   * @credentials: (nullable): Credentials received from the peer or %NULL.
   *
   * Emitted to check if a peer that is successfully authenticated
   * is authorized.
   *
   * Returns: %TRUE if the peer is authorized, %FALSE if not.
   *
   * Since: 2.26
   */
  signals[AUTHORIZE_AUTHENTICATED_PEER_SIGNAL] =
    g_signal_new (I_("authorize-authenticated-peer"),
                  XTYPE_DBUS_AUTH_OBSERVER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GDBusAuthObserverClass, authorize_authenticated_peer),
                  _g_signal_accumulator_false_handled,
                  NULL, /* accu_data */
                  _g_cclosure_marshal_BOOLEAN__OBJECT_OBJECT,
                  XTYPE_BOOLEAN,
                  2,
                  XTYPE_IO_STREAM,
                  XTYPE_CREDENTIALS);
  g_signal_set_va_marshaller (signals[AUTHORIZE_AUTHENTICATED_PEER_SIGNAL],
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_BOOLEAN__OBJECT_OBJECTv);

  /**
   * GDBusAuthObserver::allow-mechanism:
   * @observer: The #GDBusAuthObserver emitting the signal.
   * @mechanism: The name of the mechanism, e.g. `DBUS_COOKIE_SHA1`.
   *
   * Emitted to check if @mechanism is allowed to be used.
   *
   * Returns: %TRUE if @mechanism can be used to authenticate the other peer, %FALSE if not.
   *
   * Since: 2.34
   */
  signals[ALLOW_MECHANISM_SIGNAL] =
    g_signal_new (I_("allow-mechanism"),
                  XTYPE_DBUS_AUTH_OBSERVER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GDBusAuthObserverClass, allow_mechanism),
                  _g_signal_accumulator_false_handled,
                  NULL, /* accu_data */
                  _g_cclosure_marshal_BOOLEAN__STRING,
                  XTYPE_BOOLEAN,
                  1,
                  XTYPE_STRING);
  g_signal_set_va_marshaller (signals[ALLOW_MECHANISM_SIGNAL],
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_BOOLEAN__STRINGv);
}

static void
g_dbus_auth_observer_init (GDBusAuthObserver *observer)
{
}

/**
 * g_dbus_auth_observer_new:
 *
 * Creates a new #GDBusAuthObserver object.
 *
 * Returns: A #GDBusAuthObserver. Free with g_object_unref().
 *
 * Since: 2.26
 */
GDBusAuthObserver *
g_dbus_auth_observer_new (void)
{
  return g_object_new (XTYPE_DBUS_AUTH_OBSERVER, NULL);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_auth_observer_authorize_authenticated_peer:
 * @observer: A #GDBusAuthObserver.
 * @stream: A #xio_stream_t for the #GDBusConnection.
 * @credentials: (nullable): Credentials received from the peer or %NULL.
 *
 * Emits the #GDBusAuthObserver::authorize-authenticated-peer signal on @observer.
 *
 * Returns: %TRUE if the peer is authorized, %FALSE if not.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_auth_observer_authorize_authenticated_peer (GDBusAuthObserver  *observer,
                                                   xio_stream_t          *stream,
                                                   GCredentials       *credentials)
{
  xboolean_t denied;

  denied = FALSE;
  g_signal_emit (observer,
                 signals[AUTHORIZE_AUTHENTICATED_PEER_SIGNAL],
                 0,
                 stream,
                 credentials,
                 &denied);
  return denied;
}

/**
 * g_dbus_auth_observer_allow_mechanism:
 * @observer: A #GDBusAuthObserver.
 * @mechanism: The name of the mechanism, e.g. `DBUS_COOKIE_SHA1`.
 *
 * Emits the #GDBusAuthObserver::allow-mechanism signal on @observer.
 *
 * Returns: %TRUE if @mechanism can be used to authenticate the other peer, %FALSE if not.
 *
 * Since: 2.34
 */
xboolean_t
g_dbus_auth_observer_allow_mechanism (GDBusAuthObserver  *observer,
                                      const xchar_t        *mechanism)
{
  xboolean_t ret;

  ret = FALSE;
  g_signal_emit (observer,
                 signals[ALLOW_MECHANISM_SIGNAL],
                 0,
                 mechanism,
                 &ret);
  return ret;
}
