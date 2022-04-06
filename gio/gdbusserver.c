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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "giotypes.h"
#include "gioerror.h"
#include "gdbusaddress.h"
#include "gdbusutils.h"
#include "gdbusconnection.h"
#include "gdbusserver.h"
#include "gioenumtypes.h"
#include "gdbusprivate.h"
#include "gdbusauthobserver.h"
#include "ginitable.h"
#include "gsocketservice.h"
#include "gthreadedsocketservice.h"
#include "gresolver.h"
#include "glib/gstdio.h"
#include "ginetaddress.h"
#include "ginetsocketaddress.h"
#include "ginputstream.h"
#include "giostream.h"
#include "gmarshal-internal.h"

#ifdef G_OS_UNIX
#include <unistd.h>
#endif
#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifdef G_OS_UNIX
#include "gunixsocketaddress.h"
#endif

#include "glibintl.h"

#define G_DBUS_SERVER_FLAGS_ALL \
  (G_DBUS_SERVER_FLAGS_RUN_IN_THREAD | \
   G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS | \
   G_DBUS_SERVER_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER)

/**
 * SECTION:gdbusserver
 * @short_description: Helper for accepting connections
 * @include: gio/gio.h
 *
 * #xdbus_server_t is a helper for listening to and accepting D-Bus
 * connections. This can be used to create a new D-Bus server, allowing two
 * peers to use the D-Bus protocol for their own specialized communication.
 * A server instance provided in this way will not perform message routing or
 * implement the org.freedesktop.DBus interface.
 *
 * To just export an object on a well-known name on a message bus, such as the
 * session or system bus, you should instead use g_bus_own_name().
 *
 * An example of peer-to-peer communication with GDBus can be found
 * in [gdbus-example-peer.c](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gdbus-example-peer.c).
 *
 * Note that a minimal #xdbus_server_t will accept connections from any
 * peer. In many use-cases it will be necessary to add a #xdbus_auth_observer_t
 * that only accepts connections that have successfully authenticated
 * as the same user that is running the #xdbus_server_t. Since GLib 2.68 this can
 * be achieved more simply by passing the
 * %G_DBUS_SERVER_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER flag to the server.
 */

/**
 * xdbus_server_t:
 *
 * The #xdbus_server_t structure contains only private data and
 * should only be accessed using the provided API.
 *
 * Since: 2.26
 */
struct _GDBusServer
{
  /*< private >*/
  xobject_t parent_instance;

  GDBusServerFlags flags;
  xchar_t *address;
  xchar_t *guid;

  guchar *nonce;
  xchar_t *nonce_file;

  xchar_t *client_address;

  xchar_t *unix_socket_path;
  xsocket_listener_t *listener;
  xboolean_t is_using_listener;
  gulong run_signal_handler_id;

  /* The result of xmain_context_ref_thread_default() when the object
   * was created (the xobject_t _init() function) - this is used for delivery
   * of the :new-connection xobject_t signal.
   */
  xmain_context_t *main_context_at_construction;

  xboolean_t active;

  xdbus_auth_observer_t *authentication_observer;
};

typedef struct _GDBusServerClass GDBusServerClass;

/**
 * GDBusServerClass:
 * @new_connection: Signal class handler for the #xdbus_server_t::new-connection signal.
 *
 * Class structure for #xdbus_server_t.
 *
 * Since: 2.26
 */
struct _GDBusServerClass
{
  /*< private >*/
  xobject_class_t parent_class;

  /*< public >*/
  /* Signals */
  xboolean_t (*new_connection) (xdbus_server_t      *server,
                              xdbus_connection_t  *connection);
};

enum
{
  PROP_0,
  PROP_ADDRESS,
  PROP_CLIENT_ADDRESS,
  PROP_FLAGS,
  PROP_GUID,
  PROP_ACTIVE,
  PROP_AUTHENTICATION_OBSERVER,
};

enum
{
  NEW_CONNECTION_SIGNAL,
  LAST_SIGNAL,
};

static xuint_t _signals[LAST_SIGNAL] = {0};

static void initable_iface_init       (xinitable_iface_t *initable_iface);

G_DEFINE_TYPE_WITH_CODE (xdbus_server, g_dbus_server, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE, initable_iface_init))

static void
g_dbus_server_dispose (xobject_t *object)
{
  xdbus_server_t *server = G_DBUS_SERVER (object);

  if (server->active)
    g_dbus_server_stop (server);

  G_OBJECT_CLASS (g_dbus_server_parent_class)->dispose (object);
}

static void
g_dbus_server_finalize (xobject_t *object)
{
  xdbus_server_t *server = G_DBUS_SERVER (object);

  g_assert (!server->active);

  if (server->authentication_observer != NULL)
    xobject_unref (server->authentication_observer);

  if (server->run_signal_handler_id > 0)
    g_signal_handler_disconnect (server->listener, server->run_signal_handler_id);

  if (server->listener != NULL)
    xobject_unref (server->listener);

  g_free (server->address);
  g_free (server->guid);
  g_free (server->client_address);
  if (server->nonce != NULL)
    {
      memset (server->nonce, '\0', 16);
      g_free (server->nonce);
    }

  g_free (server->unix_socket_path);
  g_free (server->nonce_file);

  xmain_context_unref (server->main_context_at_construction);

  G_OBJECT_CLASS (g_dbus_server_parent_class)->finalize (object);
}

static void
g_dbus_server_get_property (xobject_t    *object,
                            xuint_t       prop_id,
                            xvalue_t     *value,
                            xparam_spec_t *pspec)
{
  xdbus_server_t *server = G_DBUS_SERVER (object);

  switch (prop_id)
    {
    case PROP_FLAGS:
      xvalue_set_flags (value, server->flags);
      break;

    case PROP_GUID:
      xvalue_set_string (value, server->guid);
      break;

    case PROP_ADDRESS:
      xvalue_set_string (value, server->address);
      break;

    case PROP_CLIENT_ADDRESS:
      xvalue_set_string (value, server->client_address);
      break;

    case PROP_ACTIVE:
      xvalue_set_boolean (value, server->active);
      break;

    case PROP_AUTHENTICATION_OBSERVER:
      xvalue_set_object (value, server->authentication_observer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_dbus_server_set_property (xobject_t      *object,
                            xuint_t         prop_id,
                            const xvalue_t *value,
                            xparam_spec_t   *pspec)
{
  xdbus_server_t *server = G_DBUS_SERVER (object);

  switch (prop_id)
    {
    case PROP_FLAGS:
      server->flags = xvalue_get_flags (value);
      break;

    case PROP_GUID:
      server->guid = xvalue_dup_string (value);
      break;

    case PROP_ADDRESS:
      server->address = xvalue_dup_string (value);
      break;

    case PROP_AUTHENTICATION_OBSERVER:
      server->authentication_observer = xvalue_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_dbus_server_class_init (GDBusServerClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose      = g_dbus_server_dispose;
  gobject_class->finalize     = g_dbus_server_finalize;
  gobject_class->set_property = g_dbus_server_set_property;
  gobject_class->get_property = g_dbus_server_get_property;

  /**
   * xdbus_server_t:flags:
   *
   * Flags from the #GDBusServerFlags enumeration.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_FLAGS,
                                   g_param_spec_flags ("flags",
                                                       P_("Flags"),
                                                       P_("Flags for the server"),
                                                       XTYPE_DBUS_SERVER_FLAGS,
                                                       G_DBUS_SERVER_FLAGS_NONE,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_BLURB |
                                                       G_PARAM_STATIC_NICK));

  /**
   * xdbus_server_t:guid:
   *
   * The GUID of the server.
   *
   * See #xdbus_connection_t:guid for more details.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_GUID,
                                   g_param_spec_string ("guid",
                                                        P_("GUID"),
                                                        P_("The guid of the server"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_server_t:address:
   *
   * The D-Bus address to listen on.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_ADDRESS,
                                   g_param_spec_string ("address",
                                                        P_("Address"),
                                                        P_("The address to listen on"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_server_t:client-address:
   *
   * The D-Bus address that clients can use.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_CLIENT_ADDRESS,
                                   g_param_spec_string ("client-address",
                                                        P_("Client Address"),
                                                        P_("The address clients can use"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_server_t:active:
   *
   * Whether the server is currently active.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         P_("Active"),
                                                         P_("Whether the server is currently active"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_BLURB |
                                                         G_PARAM_STATIC_NICK));

  /**
   * xdbus_server_t:authentication-observer:
   *
   * A #xdbus_auth_observer_t object to assist in the authentication process or %NULL.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_AUTHENTICATION_OBSERVER,
                                   g_param_spec_object ("authentication-observer",
                                                        P_("Authentication Observer"),
                                                        P_("Object used to assist in the authentication process"),
                                                        XTYPE_DBUS_AUTH_OBSERVER,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_server_t::new-connection:
   * @server: The #xdbus_server_t emitting the signal.
   * @connection: A #xdbus_connection_t for the new connection.
   *
   * Emitted when a new authenticated connection has been made. Use
   * g_dbus_connection_get_peer_credentials() to figure out what
   * identity (if any), was authenticated.
   *
   * If you want to accept the connection, take a reference to the
   * @connection object and return %TRUE. When you are done with the
   * connection call g_dbus_connection_close() and give up your
   * reference. Note that the other peer may disconnect at any time -
   * a typical thing to do when accepting a connection is to listen to
   * the #xdbus_connection_t::closed signal.
   *
   * If #xdbus_server_t:flags contains %G_DBUS_SERVER_FLAGS_RUN_IN_THREAD
   * then the signal is emitted in a new thread dedicated to the
   * connection. Otherwise the signal is emitted in the
   * [thread-default main context][g-main-context-push-thread-default]
   * of the thread that @server was constructed in.
   *
   * You are guaranteed that signal handlers for this signal runs
   * before incoming messages on @connection are processed. This means
   * that it's suitable to call g_dbus_connection_register_object() or
   * similar from the signal handler.
   *
   * Returns: %TRUE to claim @connection, %FALSE to let other handlers
   * run.
   *
   * Since: 2.26
   */
  _signals[NEW_CONNECTION_SIGNAL] = g_signal_new (I_("new-connection"),
                                                  XTYPE_DBUS_SERVER,
                                                  G_SIGNAL_RUN_LAST,
                                                  G_STRUCT_OFFSET (GDBusServerClass, new_connection),
                                                  g_signal_accumulator_true_handled,
                                                  NULL, /* accu_data */
                                                  _g_cclosure_marshal_BOOLEAN__OBJECT,
                                                  XTYPE_BOOLEAN,
                                                  1,
                                                  XTYPE_DBUS_CONNECTION);
  g_signal_set_va_marshaller (_signals[NEW_CONNECTION_SIGNAL],
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_BOOLEAN__OBJECTv);
}

static void
g_dbus_server_init (xdbus_server_t *server)
{
  server->main_context_at_construction = xmain_context_ref_thread_default ();
}

static xboolean_t
on_run (xsocket_service_t    *service,
        xsocket_connection_t *socket_connection,
        xobject_t           *source_object,
        xpointer_t           user_data);

/**
 * g_dbus_server_new_sync:
 * @address: A D-Bus address.
 * @flags: Flags from the #GDBusServerFlags enumeration.
 * @guid: A D-Bus GUID.
 * @observer: (nullable): A #xdbus_auth_observer_t or %NULL.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @error: Return location for server or %NULL.
 *
 * Creates a new D-Bus server that listens on the first address in
 * @address that works.
 *
 * Once constructed, you can use g_dbus_server_get_client_address() to
 * get a D-Bus address string that clients can use to connect.
 *
 * To have control over the available authentication mechanisms and
 * the users that are authorized to connect, it is strongly recommended
 * to provide a non-%NULL #xdbus_auth_observer_t.
 *
 * Connect to the #xdbus_server_t::new-connection signal to handle
 * incoming connections.
 *
 * The returned #xdbus_server_t isn't active - you have to start it with
 * g_dbus_server_start().
 *
 * #xdbus_server_t is used in this [example][gdbus-peer-to-peer].
 *
 * This is a synchronous failable constructor. There is currently no
 * asynchronous version.
 *
 * Returns: A #xdbus_server_t or %NULL if @error is set. Free with
 * xobject_unref().
 *
 * Since: 2.26
 */
xdbus_server_t *
g_dbus_server_new_sync (const xchar_t        *address,
                        GDBusServerFlags    flags,
                        const xchar_t        *guid,
                        xdbus_auth_observer_t  *observer,
                        xcancellable_t       *cancellable,
                        xerror_t            **error)
{
  xdbus_server_t *server;

  g_return_val_if_fail (address != NULL, NULL);
  g_return_val_if_fail (g_dbus_is_guid (guid), NULL);
  g_return_val_if_fail ((flags & ~G_DBUS_SERVER_FLAGS_ALL) == 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  server = xinitable_new (XTYPE_DBUS_SERVER,
                           cancellable,
                           error,
                           "address", address,
                           "flags", flags,
                           "guid", guid,
                           "authentication-observer", observer,
                           NULL);

  return server;
}

/**
 * g_dbus_server_get_client_address:
 * @server: A #xdbus_server_t.
 *
 * Gets a
 * [D-Bus address](https://dbus.freedesktop.org/doc/dbus-specification.html#addresses)
 * string that can be used by clients to connect to @server.
 *
 * This is valid and non-empty if initializing the #xdbus_server_t succeeded.
 *
 * Returns: (not nullable): A D-Bus address string. Do not free, the string is owned
 * by @server.
 *
 * Since: 2.26
 */
const xchar_t *
g_dbus_server_get_client_address (xdbus_server_t *server)
{
  g_return_val_if_fail (X_IS_DBUS_SERVER (server), NULL);
  return server->client_address;
}

/**
 * g_dbus_server_get_guid:
 * @server: A #xdbus_server_t.
 *
 * Gets the GUID for @server, as provided to g_dbus_server_new_sync().
 *
 * Returns: (not nullable): A D-Bus GUID. Do not free this string, it is owned by @server.
 *
 * Since: 2.26
 */
const xchar_t *
g_dbus_server_get_guid (xdbus_server_t *server)
{
  g_return_val_if_fail (X_IS_DBUS_SERVER (server), NULL);
  return server->guid;
}

/**
 * g_dbus_server_get_flags:
 * @server: A #xdbus_server_t.
 *
 * Gets the flags for @server.
 *
 * Returns: A set of flags from the #GDBusServerFlags enumeration.
 *
 * Since: 2.26
 */
GDBusServerFlags
g_dbus_server_get_flags (xdbus_server_t *server)
{
  g_return_val_if_fail (X_IS_DBUS_SERVER (server), G_DBUS_SERVER_FLAGS_NONE);
  return server->flags;
}

/**
 * g_dbus_server_is_active:
 * @server: A #xdbus_server_t.
 *
 * Gets whether @server is active.
 *
 * Returns: %TRUE if server is active, %FALSE otherwise.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_server_is_active (xdbus_server_t *server)
{
  g_return_val_if_fail (X_IS_DBUS_SERVER (server), G_DBUS_SERVER_FLAGS_NONE);
  return server->active;
}

/**
 * g_dbus_server_start:
 * @server: A #xdbus_server_t.
 *
 * Starts @server.
 *
 * Since: 2.26
 */
void
g_dbus_server_start (xdbus_server_t *server)
{
  g_return_if_fail (X_IS_DBUS_SERVER (server));
  if (server->active)
    return;
  /* Right now we don't have any transport not using the listener... */
  g_assert (server->is_using_listener);
  server->run_signal_handler_id = g_signal_connect_data (XSOCKET_SERVICE (server->listener),
                                                         "run",
                                                         G_CALLBACK (on_run),
                                                         xobject_ref (server),
                                                         (xclosure_notify_t) xobject_unref,
                                                         0  /* flags */);
  xsocket_service_start (XSOCKET_SERVICE (server->listener));
  server->active = TRUE;
  xobject_notify (G_OBJECT (server), "active");
}

/**
 * g_dbus_server_stop:
 * @server: A #xdbus_server_t.
 *
 * Stops @server.
 *
 * Since: 2.26
 */
void
g_dbus_server_stop (xdbus_server_t *server)
{
  g_return_if_fail (X_IS_DBUS_SERVER (server));
  if (!server->active)
    return;
  /* Right now we don't have any transport not using the listener... */
  g_assert (server->is_using_listener);
  g_assert (server->run_signal_handler_id > 0);
  g_clear_signal_handler (&server->run_signal_handler_id, server->listener);
  xsocket_service_stop (XSOCKET_SERVICE (server->listener));
  server->active = FALSE;
  xobject_notify (G_OBJECT (server), "active");

  if (server->unix_socket_path)
    {
      if (g_unlink (server->unix_socket_path) != 0)
        g_warning ("Failed to delete %s: %s", server->unix_socket_path, xstrerror (errno));
    }

  if (server->nonce_file)
    {
      if (g_unlink (server->nonce_file) != 0)
        g_warning ("Failed to delete %s: %s", server->nonce_file, xstrerror (errno));
    }
}

/* ---------------------------------------------------------------------------------------------------- */

#ifdef G_OS_UNIX

static xint_t
random_ascii (void)
{
  xint_t ret;
  ret = g_random_int_range (0, 60);
  if (ret < 25)
    ret += 'A';
  else if (ret < 50)
    ret += 'a' - 25;
  else
    ret += '0' - 50;
  return ret;
}

/* note that address_entry has already been validated => exactly one of path, dir, tmpdir, or abstract keys are set */
static xboolean_t
try_unix (xdbus_server_t  *server,
          const xchar_t  *address_entry,
          xhashtable_t   *key_value_pairs,
          xerror_t      **error)
{
  xboolean_t ret;
  const xchar_t *path;
  const xchar_t *dir;
  const xchar_t *tmpdir;
  const xchar_t *abstract;
  xsocket_address_t *address;

  ret = FALSE;
  address = NULL;

  path = xhash_table_lookup (key_value_pairs, "path");
  dir = xhash_table_lookup (key_value_pairs, "dir");
  tmpdir = xhash_table_lookup (key_value_pairs, "tmpdir");
  abstract = xhash_table_lookup (key_value_pairs, "abstract");

  if (path != NULL)
    {
      address = g_unix_socket_address_new (path);
    }
  else if (dir != NULL || tmpdir != NULL)
    {
      xint_t n;
      xstring_t *s;
      xerror_t *local_error;

    retry:
      s = xstring_new (tmpdir != NULL ? tmpdir : dir);
      xstring_append (s, "/dbus-");
      for (n = 0; n < 8; n++)
        xstring_append_c (s, random_ascii ());

      /* prefer abstract namespace if available for tmpdir: addresses
       * abstract namespace is disallowed for dir: addresses */
      if (tmpdir != NULL && g_unix_socket_address_abstract_names_supported ())
        address = g_unix_socket_address_new_with_type (s->str,
                                                       -1,
                                                       G_UNIX_SOCKET_ADDRESS_ABSTRACT);
      else
        address = g_unix_socket_address_new (s->str);
      xstring_free (s, TRUE);

      local_error = NULL;
      if (!xsocket_listener_add_address (server->listener,
                                          address,
                                          XSOCKET_TYPE_STREAM,
                                          XSOCKET_PROTOCOL_DEFAULT,
                                          NULL, /* source_object */
                                          NULL, /* effective_address */
                                          &local_error))
        {
          if (local_error->domain == G_IO_ERROR && local_error->code == G_IO_ERROR_ADDRESS_IN_USE)
            {
              xerror_free (local_error);
              goto retry;
            }
          g_propagate_error (error, local_error);
          goto out;
        }
      ret = TRUE;
      goto out;
    }
  else if (abstract != NULL)
    {
      if (!g_unix_socket_address_abstract_names_supported ())
        {
          g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_NOT_SUPPORTED,
                               _("Abstract namespace not supported"));
          goto out;
        }
      address = g_unix_socket_address_new_with_type (abstract,
                                                     -1,
                                                     G_UNIX_SOCKET_ADDRESS_ABSTRACT);
    }
  else
    {
      g_assert_not_reached ();
    }

  if (!xsocket_listener_add_address (server->listener,
                                      address,
                                      XSOCKET_TYPE_STREAM,
                                      XSOCKET_PROTOCOL_DEFAULT,
                                      NULL, /* source_object */
                                      NULL, /* effective_address */
                                      error))
    goto out;

  ret = TRUE;

 out:

  if (address != NULL)
    {
      /* Fill out client_address if the connection attempt worked */
      if (ret)
        {
          const char *address_path;
          char *escaped_path;

          server->is_using_listener = TRUE;
          address_path = g_unix_socket_address_get_path (G_UNIX_SOCKET_ADDRESS (address));
          escaped_path = g_dbus_address_escape_value (address_path);

          switch (g_unix_socket_address_get_address_type (G_UNIX_SOCKET_ADDRESS (address)))
            {
            case G_UNIX_SOCKET_ADDRESS_ABSTRACT:
              server->client_address = xstrdup_printf ("unix:abstract=%s", escaped_path);
              break;

            case G_UNIX_SOCKET_ADDRESS_PATH:
              server->client_address = xstrdup_printf ("unix:path=%s", escaped_path);
              server->unix_socket_path = xstrdup (address_path);
              break;

            default:
              g_assert_not_reached ();
              break;
            }

          g_free (escaped_path);
        }
      xobject_unref (address);
    }
  return ret;
}
#endif

/* ---------------------------------------------------------------------------------------------------- */

/* note that address_entry has already been validated =>
 *  both host and port (guaranteed to be a number in [0, 65535]) are set (family is optional)
 */
static xboolean_t
try_tcp (xdbus_server_t  *server,
         const xchar_t  *address_entry,
         xhashtable_t   *key_value_pairs,
         xboolean_t      do_nonce,
         xerror_t      **error)
{
  xboolean_t ret;
  const xchar_t *host;
  const xchar_t *port;
  xint_t port_num;
  xresolver_t *resolver;
  xlist_t *resolved_addresses;
  xlist_t *l;

  ret = FALSE;
  resolver = NULL;
  resolved_addresses = NULL;

  host = xhash_table_lookup (key_value_pairs, "host");
  port = xhash_table_lookup (key_value_pairs, "port");
  /* family = xhash_table_lookup (key_value_pairs, "family"); */
  if (xhash_table_lookup (key_value_pairs, "noncefile") != NULL)
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Cannot specify nonce file when creating a server"));
      goto out;
    }

  if (host == NULL)
    host = "localhost";
  if (port == NULL)
    port = "0";
  port_num = strtol (port, NULL, 10);

  resolver = g_resolver_get_default ();
  resolved_addresses = g_resolver_lookup_by_name (resolver,
                                                  host,
                                                  NULL,
                                                  error);
  if (resolved_addresses == NULL)
    goto out;

  /* TODO: handle family */
  for (l = resolved_addresses; l != NULL; l = l->next)
    {
      xinet_address_t *address = G_INET_ADDRESS (l->data);
      xsocket_address_t *socket_address;
      xsocket_address_t *effective_address;

      socket_address = g_inet_socket_address_new (address, port_num);
      if (!xsocket_listener_add_address (server->listener,
                                          socket_address,
                                          XSOCKET_TYPE_STREAM,
                                          XSOCKET_PROTOCOL_TCP,
                                          NULL, /* xobject_t *source_object */
                                          &effective_address,
                                          error))
        {
          xobject_unref (socket_address);
          goto out;
        }
      if (port_num == 0)
        /* make sure we allocate the same port number for other listeners */
        port_num = g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (effective_address));

      xobject_unref (effective_address);
      xobject_unref (socket_address);
    }

  if (do_nonce)
    {
      xint_t fd;
      xuint_t n;
      xsize_t bytes_written;
      xsize_t bytes_remaining;
      char *file_escaped;
      char *host_escaped;

      server->nonce = g_new0 (guchar, 16);
      for (n = 0; n < 16; n++)
        server->nonce[n] = g_random_int_range (0, 256);
      fd = xfile_open_tmp ("gdbus-nonce-file-XXXXXX",
                            &server->nonce_file,
                            error);
      if (fd == -1)
        {
          xsocket_listener_close (server->listener);
          goto out;
        }
    again:
      bytes_written = 0;
      bytes_remaining = 16;
      while (bytes_remaining > 0)
        {
          xssize_t ret;
          int errsv;

          ret = write (fd, server->nonce + bytes_written, bytes_remaining);
          errsv = errno;
          if (ret == -1)
            {
              if (errsv == EINTR)
                goto again;
              g_set_error (error,
                           G_IO_ERROR,
                           g_io_error_from_errno (errsv),
                           _("Error writing nonce file at “%s”: %s"),
                           server->nonce_file,
                           xstrerror (errsv));
              goto out;
            }
          bytes_written += ret;
          bytes_remaining -= ret;
        }
      if (!g_close (fd, error))
        goto out;
      host_escaped = g_dbus_address_escape_value (host);
      file_escaped = g_dbus_address_escape_value (server->nonce_file);
      server->client_address = xstrdup_printf ("nonce-tcp:host=%s,port=%d,noncefile=%s",
                                                host_escaped,
                                                port_num,
                                                file_escaped);
      g_free (host_escaped);
      g_free (file_escaped);
    }
  else
    {
      server->client_address = xstrdup_printf ("tcp:host=%s,port=%d", host, port_num);
    }
  server->is_using_listener = TRUE;
  ret = TRUE;

 out:
  xlist_free_full (resolved_addresses, xobject_unref);
  if (resolver)
    xobject_unref (resolver);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xdbus_server_t *server;
  xdbus_connection_t *connection;
} EmitIdleData;

static void
emit_idle_data_free (EmitIdleData *data)
{
  xobject_unref (data->server);
  xobject_unref (data->connection);
  g_free (data);
}

static xboolean_t
emit_new_connection_in_idle (xpointer_t user_data)
{
  EmitIdleData *data = user_data;
  xboolean_t claimed;

  claimed = FALSE;
  g_signal_emit (data->server,
                 _signals[NEW_CONNECTION_SIGNAL],
                 0,
                 data->connection,
                 &claimed);

  if (claimed)
    g_dbus_connection_start_message_processing (data->connection);
  xobject_unref (data->connection);

  return FALSE;
}

/* Called in new thread */
static xboolean_t
on_run (xsocket_service_t    *service,
        xsocket_connection_t *socket_connection,
        xobject_t           *source_object,
        xpointer_t           user_data)
{
  xdbus_server_t *server = G_DBUS_SERVER (user_data);
  xdbus_connection_t *connection;
  GDBusConnectionFlags connection_flags;

  if (server->nonce != NULL)
    {
      xchar_t buf[16];
      xsize_t bytes_read;

      if (!xinput_stream_read_all (g_io_stream_get_input_stream (XIO_STREAM (socket_connection)),
                                    buf,
                                    16,
                                    &bytes_read,
                                    NULL,  /* xcancellable_t */
                                    NULL)) /* xerror_t */
        goto out;

      if (bytes_read != 16)
        goto out;

      if (memcmp (buf, server->nonce, 16) != 0)
        goto out;
    }

  connection_flags =
    G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER |
    G_DBUS_CONNECTION_FLAGS_DELAY_MESSAGE_PROCESSING;
  if (server->flags & G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS)
    connection_flags |= G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;
  if (server->flags & G_DBUS_SERVER_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER)
    connection_flags |= G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER;

  connection = g_dbus_connection_new_sync (XIO_STREAM (socket_connection),
                                           server->guid,
                                           connection_flags,
                                           server->authentication_observer,
                                           NULL,  /* xcancellable_t */
                                           NULL); /* xerror_t */
  if (connection == NULL)
      goto out;

  if (server->flags & G_DBUS_SERVER_FLAGS_RUN_IN_THREAD)
    {
      xboolean_t claimed;

      claimed = FALSE;
      g_signal_emit (server,
                     _signals[NEW_CONNECTION_SIGNAL],
                     0,
                     connection,
                     &claimed);
      if (claimed)
        g_dbus_connection_start_message_processing (connection);
      xobject_unref (connection);
    }
  else
    {
      xsource_t *idle_source;
      EmitIdleData *data;

      data = g_new0 (EmitIdleData, 1);
      data->server = xobject_ref (server);
      data->connection = xobject_ref (connection);

      idle_source = g_idle_source_new ();
      xsource_set_priority (idle_source, G_PRIORITY_DEFAULT);
      xsource_set_callback (idle_source,
                             emit_new_connection_in_idle,
                             data,
                             (xdestroy_notify_t) emit_idle_data_free);
      xsource_set_static_name (idle_source, "[gio] emit_new_connection_in_idle");
      xsource_attach (idle_source, server->main_context_at_construction);
      xsource_unref (idle_source);
    }

 out:
  return TRUE;
}

static xboolean_t
initable_init (xinitable_t     *initable,
               xcancellable_t  *cancellable,
               xerror_t       **error)
{
  xdbus_server_t *server = G_DBUS_SERVER (initable);
  xboolean_t ret;
  xuint_t n;
  xchar_t **addr_array;
  xerror_t *last_error;

  ret = FALSE;
  addr_array = NULL;
  last_error = NULL;

  if (!g_dbus_is_guid (server->guid))
    {
      g_set_error (&last_error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("The string “%s” is not a valid D-Bus GUID"),
                   server->guid);
      goto out;
    }

  server->listener = XSOCKET_LISTENER (xthreaded_socket_service_new (-1));

  addr_array = xstrsplit (server->address, ";", 0);
  last_error = NULL;
  for (n = 0; addr_array != NULL && addr_array[n] != NULL; n++)
    {
      const xchar_t *address_entry = addr_array[n];
      xhashtable_t *key_value_pairs;
      xchar_t *transport_name;
      xerror_t *this_error;

      this_error = NULL;
      if (g_dbus_is_supported_address (address_entry,
                                       &this_error) &&
          _g_dbus_address_parse_entry (address_entry,
                                       &transport_name,
                                       &key_value_pairs,
                                       &this_error))
        {

          if (FALSE)
            {
            }
#ifdef G_OS_UNIX
          else if (xstrcmp0 (transport_name, "unix") == 0)
            ret = try_unix (server, address_entry, key_value_pairs, &this_error);
#endif
          else if (xstrcmp0 (transport_name, "tcp") == 0)
            ret = try_tcp (server, address_entry, key_value_pairs, FALSE, &this_error);
          else if (xstrcmp0 (transport_name, "nonce-tcp") == 0)
            ret = try_tcp (server, address_entry, key_value_pairs, TRUE, &this_error);
          else
            g_set_error (&this_error,
                         G_IO_ERROR,
                         G_IO_ERROR_INVALID_ARGUMENT,
                         _("Cannot listen on unsupported transport “%s”"),
                         transport_name);

          g_free (transport_name);
          if (key_value_pairs != NULL)
            xhash_table_unref (key_value_pairs);

          if (ret)
            {
              g_assert (this_error == NULL);
              goto out;
            }
        }

      if (this_error != NULL)
        {
          if (last_error != NULL)
            xerror_free (last_error);
          last_error = this_error;
        }
    }

 out:

  xstrfreev (addr_array);

  if (ret)
    {
      g_clear_error (&last_error);
    }
  else
    {
      g_assert (last_error != NULL);
      g_propagate_error (error, last_error);
    }
  return ret;
}


static void
initable_iface_init (xinitable_iface_t *initable_iface)
{
  initable_iface->init = initable_init;
}

/* ---------------------------------------------------------------------------------------------------- */
