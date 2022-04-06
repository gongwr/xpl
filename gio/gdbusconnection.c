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

/*
 * TODO for GDBus:
 *
 * - would be nice to expose GDBusAuthMechanism and an extension point
 *
 * - Need to rewrite GDBusAuth and rework GDBusAuthMechanism. In particular
 *   the mechanism VFuncs need to be able to set an error.
 *
 * - Need to document other mechanisms/sources for determining the D-Bus
 *   address of a well-known bus.
 *
 *   - e.g. on Win32 we need code like from here
 *
 *     http://cgit.freedesktop.org/~david/gdbus-standalone/tree/gdbus/gdbusaddress.c#n900
 *
 *     that was never copied over here because it originally was copy-paste
 *     from the GPLv2 / AFL 2.1 libdbus sources.
 *
 *   - on OS X we need to look in launchd for the address
 *
 *     https://bugs.freedesktop.org/show_bug.cgi?id=14259
 *
 *   - on X11 we need to look in a X11 property on the X server
 *     - (we can also just use dbus-launch(1) from the D-Bus
 *        distribution)
 *
 *   - (ideally) this requires D-Bus spec work because none of
 *     this has never really been specced out properly (except
 *     the X11 bits)
 *
 * - Related to the above, we also need to be able to launch a message bus
 *   instance.... Since we don't want to write our own bus daemon we should
 *   launch dbus-daemon(1) (thus: Win32 and OS X need to bundle it)
 *
 * - probably want a G_DBUS_NONCE_TCP_TMPDIR environment variable
 *   to specify where the nonce is stored. This will allow people to use
 *   G_DBUS_NONCE_TCP_TMPDIR=/mnt/secure.company.server/dbus-nonce-dir
 *   to easily achieve secure RPC via nonce-tcp.
 *
 * - need to expose an extension point for resolving D-Bus address and
 *   turning them into xio_stream_t objects. This will allow us to implement
 *   e.g. X11 D-Bus transports without dlopen()'ing or linking against
 *   libX11 from libgio.
 *   - see g_dbus_address_connect() in gdbusaddress.c
 *
 * - would be cute to use kernel-specific APIs to resolve fds for
 *   debug output when using G_DBUS_DEBUG=message, e.g. in addition to
 *
 *     fd 21: dev=8:1,mode=0100644,ino=1171231,uid=0,gid=0,rdev=0:0,size=234,atime=1273070640,mtime=1267126160,ctime=1267126160
 *
 *   maybe we can show more information about what fd 21 really is.
 *   Ryan suggests looking in /proc/self/fd for clues / symlinks!
 *   Initial experiments on Linux 2.6 suggests that the symlink looks
 *   like this:
 *
 *    3 -> /proc/18068/fd
 *
 *   e.g. not of much use.
 *
 *  - GDBus High-Level docs
 *    - Proxy: properties, signals...
 *    - Connection: IOStream based, ::close, connection setup steps
 *                  mainloop integration, threading
 *    - Differences from libdbus (extend "Migrating from")
 *      - the message handling thread
 *      - Using xvariant_t instead of xvalue_t
 *    - Explain why the high-level API is a good thing and what
 *      kind of pitfalls it avoids
 *      - Export objects before claiming names
 *    - Talk about auto-starting services (cf. GBusNameWatcherFlags)
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "gdbusauth.h"
#include "gdbusutils.h"
#include "gdbusaddress.h"
#include "gdbusmessage.h"
#include "gdbusconnection.h"
#include "gdbuserror.h"
#include "gioenumtypes.h"
#include "gdbusintrospection.h"
#include "gdbusmethodinvocation.h"
#include "gdbusprivate.h"
#include "gdbusauthobserver.h"
#include "ginitable.h"
#include "gasyncinitable.h"
#include "giostream.h"
#include "gasyncresult.h"
#include "gtask.h"
#include "gmarshal-internal.h"

#ifdef G_OS_UNIX
#include "gunixconnection.h"
#include "gunixfdmessage.h"
#endif

#include "glibintl.h"

#define G_DBUS_CONNECTION_FLAGS_ALL \
  (G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | \
   G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER | \
   G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS | \
   G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION | \
   G_DBUS_CONNECTION_FLAGS_DELAY_MESSAGE_PROCESSING | \
   G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER)

/**
 * SECTION:gdbusconnection
 * @short_description: D-Bus Connections
 * @include: gio/gio.h
 *
 * The #xdbus_connection_t type is used for D-Bus connections to remote
 * peers such as a message buses. It is a low-level API that offers a
 * lot of flexibility. For instance, it lets you establish a connection
 * over any transport that can by represented as a #xio_stream_t.
 *
 * This class is rarely used directly in D-Bus clients. If you are writing
 * a D-Bus client, it is often easier to use the g_bus_own_name(),
 * g_bus_watch_name() or g_dbus_proxy_new_for_bus() APIs.
 *
 * As an exception to the usual GLib rule that a particular object must not
 * be used by two threads at the same time, #xdbus_connection_t's methods may be
 * called from any thread. This is so that g_bus_get() and g_bus_get_sync()
 * can safely return the same #xdbus_connection_t when called from any thread.
 *
 * Most of the ways to obtain a #xdbus_connection_t automatically initialize it
 * (i.e. connect to D-Bus): for instance, g_dbus_connection_new() and
 * g_bus_get(), and the synchronous versions of those methods, give you an
 * initialized connection. Language bindings for GIO should use
 * xinitable_new() or xasync_initable_new_async(), which also initialize the
 * connection.
 *
 * If you construct an uninitialized #xdbus_connection_t, such as via
 * xobject_new(), you must initialize it via xinitable_init() or
 * xasync_initable_init_async() before using its methods or properties.
 * Calling methods or accessing properties on a #xdbus_connection_t that has not
 * completed initialization successfully is considered to be invalid, and leads
 * to undefined behaviour. In particular, if initialization fails with a
 * #xerror_t, the only valid thing you can do with that #xdbus_connection_t is to
 * free it with xobject_unref().
 *
 * ## An example D-Bus server # {#gdbus-server}
 *
 * Here is an example for a D-Bus server:
 * [gdbus-example-server.c](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gdbus-example-server.c)
 *
 * ## An example for exporting a subtree # {#gdbus-subtree-server}
 *
 * Here is an example for exporting a subtree:
 * [gdbus-example-subtree.c](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gdbus-example-subtree.c)
 *
 * ## An example for file descriptor passing # {#gdbus-unix-fd-client}
 *
 * Here is an example for passing UNIX file descriptors:
 * [gdbus-unix-fd-client.c](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gdbus-example-unix-fd-client.c)
 *
 * ## An example for exporting a xobject_t # {#gdbus-export}
 *
 * Here is an example for exporting a #xobject_t:
 * [gdbus-example-export.c](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gdbus-example-export.c)
 */

/* ---------------------------------------------------------------------------------------------------- */

typedef struct _GDBusConnectionClass GDBusConnectionClass;

/**
 * GDBusConnectionClass:
 * @closed: Signal class handler for the #xdbus_connection_t::closed signal.
 *
 * Class structure for #xdbus_connection_t.
 *
 * Since: 2.26
 */
struct _GDBusConnectionClass
{
  /*< private >*/
  xobject_class_t parent_class;

  /*< public >*/
  /* Signals */
  void (*closed) (xdbus_connection_t *connection,
                  xboolean_t         remote_peer_vanished,
                  xerror_t          *error);
};

G_LOCK_DEFINE_STATIC (message_bus_lock);

static GWeakRef the_session_bus;
static GWeakRef the_system_bus;

/* Extra pseudo-member of GDBusSendMessageFlags.
 * Set by initable_init() to indicate that despite not being initialized yet,
 * enough of the only-valid-after-init members are set that we can send a
 * message, and we're being called from its thread, so no memory barrier is
 * required before accessing them.
 */
#define SEND_MESSAGE_FLAGS_INITIALIZING (1u << 31)

/* Same as SEND_MESSAGE_FLAGS_INITIALIZING, but in GDBusCallFlags */
#define CALL_FLAGS_INITIALIZING (1u << 31)

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xdestroy_notify_t              callback;
  xpointer_t                    user_data;
} CallDestroyNotifyData;

static xboolean_t
call_destroy_notify_data_in_idle (xpointer_t user_data)
{
  CallDestroyNotifyData *data = user_data;
  data->callback (data->user_data);
  return FALSE;
}

static void
call_destroy_notify_data_free (CallDestroyNotifyData *data)
{
  g_free (data);
}

/*
 * call_destroy_notify: <internal>
 * @context: (nullable): A #xmain_context_t or %NULL.
 * @callback: (nullable): A #xdestroy_notify_t or %NULL.
 * @user_data: Data to pass to @callback.
 *
 * Schedules @callback to run in @context.
 */
static void
call_destroy_notify (xmain_context_t  *context,
                     xdestroy_notify_t callback,
                     xpointer_t       user_data)
{
  xsource_t *idle_source;
  CallDestroyNotifyData *data;

  if (callback == NULL)
    return;

  data = g_new0 (CallDestroyNotifyData, 1);
  data->callback = callback;
  data->user_data = user_data;

  idle_source = g_idle_source_new ();
  xsource_set_priority (idle_source, G_PRIORITY_DEFAULT);
  xsource_set_callback (idle_source,
                         call_destroy_notify_data_in_idle,
                         data,
                         (xdestroy_notify_t) call_destroy_notify_data_free);
  xsource_set_static_name (idle_source, "[gio] call_destroy_notify_data_in_idle");
  xsource_attach (idle_source, context);
  xsource_unref (idle_source);
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
_xstrv_has_string (const xchar_t* const *haystack,
                    const xchar_t        *needle)
{
  xuint_t n;

  for (n = 0; haystack != NULL && haystack[n] != NULL; n++)
    {
      if (xstrcmp0 (haystack[n], needle) == 0)
        return TRUE;
    }
  return FALSE;
}

/* ---------------------------------------------------------------------------------------------------- */

#ifdef G_OS_WIN32
#define CONNECTION_ENSURE_LOCK(obj) do { ; } while (FALSE)
#else
// TODO: for some reason this doesn't work on Windows
#define CONNECTION_ENSURE_LOCK(obj) do {                                \
    if (G_UNLIKELY (g_mutex_trylock(&(obj)->lock)))                     \
      {                                                                 \
        g_assertion_message (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
                             "CONNECTION_ENSURE_LOCK: xdbus_connection_t object lock is not locked"); \
      }                                                                 \
  } while (FALSE)
#endif

#define CONNECTION_LOCK(obj) do {                                       \
    g_mutex_lock (&(obj)->lock);                                        \
  } while (FALSE)

#define CONNECTION_UNLOCK(obj) do {                                     \
    g_mutex_unlock (&(obj)->lock);                                      \
  } while (FALSE)

/* Flags in connection->atomic_flags */
enum {
    FLAG_INITIALIZED = 1 << 0,
    FLAG_EXIT_ON_CLOSE = 1 << 1,
    FLAG_CLOSED = 1 << 2
};

/**
 * xdbus_connection_t:
 *
 * The #xdbus_connection_t structure contains only private data and
 * should only be accessed using the provided API.
 *
 * Since: 2.26
 */
struct _GDBusConnection
{
  /*< private >*/
  xobject_t parent_instance;

  /* ------------------------------------------------------------------------ */
  /* -- General object state ------------------------------------------------ */
  /* ------------------------------------------------------------------------ */

  /* General-purpose lock for most fields */
  xmutex_t lock;

  /* A lock used in the init() method of the xinitable_t interface - see comments
   * in initable_init() for why a separate lock is needed.
   *
   * If you need both @lock and @init_lock, you must take @init_lock first.
   */
  xmutex_t init_lock;

  /* Set (by loading the contents of /var/lib/dbus/machine-id) the first time
   * someone calls org.freedesktop.DBus.Peer.GetMachineId(). Protected by @lock.
   */
  xchar_t *machine_id;

  /* The underlying stream used for communication
   * Read-only after initable_init(), so it may be read if you either
   * hold @init_lock or check for initialization first.
   */
  xio_stream_t *stream;

  /* The object used for authentication (if any).
   * Read-only after initable_init(), so it may be read if you either
   * hold @init_lock or check for initialization first.
   */
  GDBusAuth *auth;

  /* Last serial used. Protected by @lock. */
  xuint32_t last_serial;

  /* The object used to send/receive messages.
   * Read-only after initable_init(), so it may be read if you either
   * hold @init_lock or check for initialization first.
   */
  GDBusWorker *worker;

  /* If connected to a message bus, this contains the unique name assigned to
   * us by the bus (e.g. ":1.42").
   * Read-only after initable_init(), so it may be read if you either
   * hold @init_lock or check for initialization first.
   */
  xchar_t *bus_unique_name;

  /* The GUID returned by the other side if we authenticated as a client or
   * the GUID to use if authenticating as a server.
   * Read-only after initable_init(), so it may be read if you either
   * hold @init_lock or check for initialization first.
   */
  xchar_t *guid;

  /* FLAG_INITIALIZED is set exactly when initable_init() has finished running.
   * Inspect @initialization_error to see whether it succeeded or failed.
   *
   * FLAG_EXIT_ON_CLOSE is the exit-on-close property.
   *
   * FLAG_CLOSED is the closed property. It may be read at any time, but
   * may only be written while holding @lock.
   */
  xint_t atomic_flags;  /* (atomic) */

  /* If the connection could not be established during initable_init(),
   * this xerror_t will be set.
   * Read-only after initable_init(), so it may be read if you either
   * hold @init_lock or check for initialization first.
   */
  xerror_t *initialization_error;

  /* The result of xmain_context_ref_thread_default() when the object
   * was created (the xobject_t _init() function) - this is used for delivery
   * of the :closed xobject_t signal.
   *
   * Only set in the xobject_t init function, so no locks are needed.
   */
  xmain_context_t *main_context_at_construction;

  /* Read-only construct properties, no locks needed */
  xchar_t *address;
  GDBusConnectionFlags flags;

  /* Map used for managing method replies, protected by @lock */
  xhashtable_t *map_method_serial_to_task;  /* xuint32_t -> xtask_t* */

  /* Maps used for managing signal subscription, protected by @lock */
  xhashtable_t *map_rule_to_signal_data;                      /* match rule (xchar_t*)    -> SignalData */
  xhashtable_t *map_id_to_signal_data;                        /* id (xuint_t)             -> SignalData */
  xhashtable_t *map_sender_unique_name_to_signal_data_array;  /* unique sender (xchar_t*) -> xptr_array_t* of SignalData */

  /* Maps used for managing exported objects and subtrees,
   * protected by @lock
   */
  xhashtable_t *map_object_path_to_eo;  /* xchar_t* -> ExportedObject* */
  xhashtable_t *map_id_to_ei;           /* xuint_t  -> ExportedInterface* */
  xhashtable_t *map_object_path_to_es;  /* xchar_t* -> ExportedSubtree* */
  xhashtable_t *map_id_to_es;           /* xuint_t  -> ExportedSubtree* */

  /* Map used for storing last used serials for each thread, protected by @lock */
  xhashtable_t *map_thread_to_last_serial;

  /* Structure used for message filters, protected by @lock */
  xptr_array_t *filters;

  /* Capabilities negotiated during authentication
   * Read-only after initable_init(), so it may be read without holding a
   * lock, if you check for initialization first.
   */
  GDBusCapabilityFlags capabilities;

  /* Protected by @init_lock */
  xdbus_auth_observer_t *authentication_observer;

  /* Read-only after initable_init(), so it may be read if you either
   * hold @init_lock or check for initialization first.
   */
  xcredentials_t *credentials;

  /* set to TRUE when finalizing */
  xboolean_t finalizing;
};

typedef struct ExportedObject ExportedObject;
static void exported_object_free (ExportedObject *eo);

typedef struct ExportedSubtree ExportedSubtree;
static ExportedSubtree *exported_subtree_ref (ExportedSubtree *es);
static void exported_subtree_unref (ExportedSubtree *es);

enum
{
  CLOSED_SIGNAL,
  LAST_SIGNAL,
};

enum
{
  PROP_0,
  PROP_STREAM,
  PROP_ADDRESS,
  PROP_FLAGS,
  PROP_GUID,
  PROP_UNIQUE_NAME,
  PROP_CLOSED,
  PROP_EXIT_ON_CLOSE,
  PROP_CAPABILITY_FLAGS,
  PROP_AUTHENTICATION_OBSERVER,
};

static void distribute_signals (xdbus_connection_t  *connection,
                                xdbus_message_t     *message);

static void distribute_method_call (xdbus_connection_t  *connection,
                                    xdbus_message_t     *message);

static xboolean_t handle_generic_unlocked (xdbus_connection_t *connection,
                                         xdbus_message_t    *message);


static void purge_all_signal_subscriptions (xdbus_connection_t *connection);
static void purge_all_filters (xdbus_connection_t *connection);

static void schedule_method_call (xdbus_connection_t            *connection,
                                  xdbus_message_t               *message,
                                  xuint_t                       registration_id,
                                  xuint_t                       subtree_registration_id,
                                  const xdbus_interface_info_t   *interface_info,
                                  const xdbus_method_info_t      *method_info,
                                  const xdbus_property_info_t    *property_info,
                                  xvariant_t                   *parameters,
                                  const xdbus_interface_vtable_t *vtable,
                                  xmain_context_t               *main_context,
                                  xpointer_t                    user_data);

#define _G_ENSURE_LOCK(name) do {                                       \
    if (G_UNLIKELY (G_TRYLOCK(name)))                                   \
      {                                                                 \
        g_assertion_message (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
                             "_G_ENSURE_LOCK: Lock '" #name "' is not locked"); \
      }                                                                 \
  } while (FALSE)                                                       \

static xuint_t signals[LAST_SIGNAL] = { 0 };

static void initable_iface_init       (xinitable_iface_t      *initable_iface);
static void async_initable_iface_init (xasync_initable_iface_t *async_initable_iface);

G_DEFINE_TYPE_WITH_CODE (xdbus_connection, g_dbus_connection, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE, initable_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_ASYNC_INITABLE, async_initable_iface_init)
                         );

/*
 * Check that all members of @connection that can only be accessed after
 * the connection is initialized can safely be accessed. If not,
 * log a critical warning. This function is a memory barrier.
 *
 * Returns: %TRUE if initialized
 */
static xboolean_t
check_initialized (xdbus_connection_t *connection)
{
  /* The access to @atomic_flags isn't conditional, so that this function
   * provides a memory barrier for thread-safety even if checks are disabled.
   * (If you don't want this stricter guarantee, you can call
   * g_return_if_fail (check_initialized (c)).)
   *
   * This isn't strictly necessary now that we've decided use of an
   * uninitialized xdbus_connection_t is undefined behaviour, but it seems
   * better to be as deterministic as is feasible.
   *
   * (Anything that could suffer a crash from seeing undefined values
   * must have a race condition - thread A initializes the connection while
   * thread B calls a method without initialization, hoping that thread A will
   * win the race - so its behaviour is undefined anyway.)
   */
  xint_t flags = g_atomic_int_get (&connection->atomic_flags);

  g_return_val_if_fail (flags & FLAG_INITIALIZED, FALSE);

  /* We can safely access this, due to the memory barrier above */
  g_return_val_if_fail (connection->initialization_error == NULL, FALSE);

  return TRUE;
}

typedef enum {
    MAY_BE_UNINITIALIZED = (1<<1)
} CheckUnclosedFlags;

/*
 * Check the same thing as check_initialized(), and also that the
 * connection is not closed. If the connection is uninitialized,
 * raise a critical warning (it's programmer error); if it's closed,
 * raise a recoverable xerror_t (it's a runtime error).
 *
 * This function is a memory barrier.
 *
 * Returns: %TRUE if initialized and not closed
 */
static xboolean_t
check_unclosed (xdbus_connection_t     *connection,
                CheckUnclosedFlags   check,
                xerror_t             **error)
{
  /* check_initialized() is effectively inlined, so we don't waste time
   * doing two memory barriers
   */
  xint_t flags = g_atomic_int_get (&connection->atomic_flags);

  if (!(check & MAY_BE_UNINITIALIZED))
    {
      g_return_val_if_fail (flags & FLAG_INITIALIZED, FALSE);
      g_return_val_if_fail (connection->initialization_error == NULL, FALSE);
    }

  if (flags & FLAG_CLOSED)
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_CLOSED,
                           _("The connection is closed"));
      return FALSE;
    }

  return TRUE;
}

static xhashtable_t *alive_connections = NULL;

static void
g_dbus_connection_dispose (xobject_t *object)
{
  xdbus_connection_t *connection = G_DBUS_CONNECTION (object);

  G_LOCK (message_bus_lock);
  CONNECTION_LOCK (connection);
  if (connection->worker != NULL)
    {
      _g_dbus_worker_stop (connection->worker);
      connection->worker = NULL;
      if (alive_connections != NULL)
        g_warn_if_fail (xhash_table_remove (alive_connections, connection));
    }
  else
    {
      if (alive_connections != NULL)
        g_warn_if_fail (!xhash_table_contains (alive_connections, connection));
    }
  CONNECTION_UNLOCK (connection);
  G_UNLOCK (message_bus_lock);

  if (G_OBJECT_CLASS (g_dbus_connection_parent_class)->dispose != NULL)
    G_OBJECT_CLASS (g_dbus_connection_parent_class)->dispose (object);
}

static void
g_dbus_connection_finalize (xobject_t *object)
{
  xdbus_connection_t *connection = G_DBUS_CONNECTION (object);

  connection->finalizing = TRUE;

  purge_all_signal_subscriptions (connection);

  purge_all_filters (connection);
  xptr_array_unref (connection->filters);

  if (connection->authentication_observer != NULL)
    xobject_unref (connection->authentication_observer);

  if (connection->auth != NULL)
    xobject_unref (connection->auth);

  if (connection->credentials)
    xobject_unref (connection->credentials);

  if (connection->stream != NULL)
    {
      xobject_unref (connection->stream);
      connection->stream = NULL;
    }

  g_free (connection->address);

  g_free (connection->guid);
  g_free (connection->bus_unique_name);

  if (connection->initialization_error != NULL)
    xerror_free (connection->initialization_error);

  xhash_table_unref (connection->map_method_serial_to_task);

  xhash_table_unref (connection->map_rule_to_signal_data);
  xhash_table_unref (connection->map_id_to_signal_data);
  xhash_table_unref (connection->map_sender_unique_name_to_signal_data_array);

  xhash_table_unref (connection->map_id_to_ei);
  xhash_table_unref (connection->map_object_path_to_eo);
  xhash_table_unref (connection->map_id_to_es);
  xhash_table_unref (connection->map_object_path_to_es);

  xhash_table_unref (connection->map_thread_to_last_serial);

  xmain_context_unref (connection->main_context_at_construction);

  g_free (connection->machine_id);

  g_mutex_clear (&connection->init_lock);
  g_mutex_clear (&connection->lock);

  G_OBJECT_CLASS (g_dbus_connection_parent_class)->finalize (object);
}

/* called in any user thread, with the connection's lock not held */
static void
g_dbus_connection_get_property (xobject_t    *object,
                                xuint_t       prop_id,
                                xvalue_t     *value,
                                xparam_spec_t *pspec)
{
  xdbus_connection_t *connection = G_DBUS_CONNECTION (object);

  switch (prop_id)
    {
    case PROP_STREAM:
      xvalue_set_object (value, g_dbus_connection_get_stream (connection));
      break;

    case PROP_GUID:
      xvalue_set_string (value, g_dbus_connection_get_guid (connection));
      break;

    case PROP_UNIQUE_NAME:
      xvalue_set_string (value, g_dbus_connection_get_unique_name (connection));
      break;

    case PROP_CLOSED:
      xvalue_set_boolean (value, g_dbus_connection_is_closed (connection));
      break;

    case PROP_EXIT_ON_CLOSE:
      xvalue_set_boolean (value, g_dbus_connection_get_exit_on_close (connection));
      break;

    case PROP_CAPABILITY_FLAGS:
      xvalue_set_flags (value, g_dbus_connection_get_capabilities (connection));
      break;

    case PROP_FLAGS:
      xvalue_set_flags (value, g_dbus_connection_get_flags (connection));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* called in any user thread, with the connection's lock not held */
static void
g_dbus_connection_set_property (xobject_t      *object,
                                xuint_t         prop_id,
                                const xvalue_t *value,
                                xparam_spec_t   *pspec)
{
  xdbus_connection_t *connection = G_DBUS_CONNECTION (object);

  switch (prop_id)
    {
    case PROP_STREAM:
      connection->stream = xvalue_dup_object (value);
      break;

    case PROP_GUID:
      connection->guid = xvalue_dup_string (value);
      break;

    case PROP_ADDRESS:
      connection->address = xvalue_dup_string (value);
      break;

    case PROP_FLAGS:
      connection->flags = xvalue_get_flags (value);
      break;

    case PROP_EXIT_ON_CLOSE:
      g_dbus_connection_set_exit_on_close (connection, xvalue_get_boolean (value));
      break;

    case PROP_AUTHENTICATION_OBSERVER:
      connection->authentication_observer = xvalue_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Base-class implementation of xdbus_connection_t::closed.
 *
 * Called in a user thread, by the main context that was thread-default when
 * the object was constructed.
 */
static void
g_dbus_connection_real_closed (xdbus_connection_t *connection,
                               xboolean_t         remote_peer_vanished,
                               xerror_t          *error)
{
  xint_t flags = g_atomic_int_get (&connection->atomic_flags);

  /* Because atomic int access is a memory barrier, we can safely read
   * initialization_error without a lock, as long as we do it afterwards.
   */
  if (remote_peer_vanished &&
      (flags & FLAG_EXIT_ON_CLOSE) != 0 &&
      (flags & FLAG_INITIALIZED) != 0 &&
      connection->initialization_error == NULL)
    {
      raise (SIGTERM);
    }
}

static void
g_dbus_connection_class_init (GDBusConnectionClass *klass)
{
  xobject_class_t *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = g_dbus_connection_finalize;
  gobject_class->dispose      = g_dbus_connection_dispose;
  gobject_class->set_property = g_dbus_connection_set_property;
  gobject_class->get_property = g_dbus_connection_get_property;

  klass->closed = g_dbus_connection_real_closed;

  /**
   * xdbus_connection_t:stream:
   *
   * The underlying #xio_stream_t used for I/O.
   *
   * If this is passed on construction and is a #xsocket_connection_t,
   * then the corresponding #xsocket_t will be put into non-blocking mode.
   *
   * While the #xdbus_connection_t is active, it will interact with this
   * stream from a worker thread, so it is not safe to interact with
   * the stream directly.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_STREAM,
                                   g_param_spec_object ("stream",
                                                        P_("IO Stream"),
                                                        P_("The underlying streams used for I/O"),
                                                        XTYPE_IO_STREAM,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_connection_t:address:
   *
   * A D-Bus address specifying potential endpoints that can be used
   * when establishing the connection.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_ADDRESS,
                                   g_param_spec_string ("address",
                                                        P_("Address"),
                                                        P_("D-Bus address specifying potential socket endpoints"),
                                                        NULL,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_connection_t:flags:
   *
   * Flags from the #GDBusConnectionFlags enumeration.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_FLAGS,
                                   g_param_spec_flags ("flags",
                                                       P_("Flags"),
                                                       P_("Flags"),
                                                       XTYPE_DBUS_CONNECTION_FLAGS,
                                                       G_DBUS_CONNECTION_FLAGS_NONE,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_BLURB |
                                                       G_PARAM_STATIC_NICK));

  /**
   * xdbus_connection_t:guid:
   *
   * The GUID of the peer performing the role of server when
   * authenticating.
   *
   * If you are constructing a #xdbus_connection_t and pass
   * %G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER in the
   * #xdbus_connection_t:flags property then you **must** also set this
   * property to a valid guid.
   *
   * If you are constructing a #xdbus_connection_t and pass
   * %G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT in the
   * #xdbus_connection_t:flags property you will be able to read the GUID
   * of the other peer here after the connection has been successfully
   * initialized.
   *
   * Note that the
   * [D-Bus specification](https://dbus.freedesktop.org/doc/dbus-specification.html#addresses)
   * uses the term ‘UUID’ to refer to this, whereas GLib consistently uses the
   * term ‘GUID’ for historical reasons.
   *
   * Despite its name, the format of #xdbus_connection_t:guid does not follow
   * [RFC 4122](https://datatracker.ietf.org/doc/html/rfc4122) or the Microsoft
   * GUID format.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_GUID,
                                   g_param_spec_string ("guid",
                                                        P_("GUID"),
                                                        P_("GUID of the server peer"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_connection_t:unique-name:
   *
   * The unique name as assigned by the message bus or %NULL if the
   * connection is not open or not a message bus connection.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_UNIQUE_NAME,
                                   g_param_spec_string ("unique-name",
                                                        P_("unique-name"),
                                                        P_("Unique name of bus connection"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_connection_t:closed:
   *
   * A boolean specifying whether the connection has been closed.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_CLOSED,
                                   g_param_spec_boolean ("closed",
                                                         P_("Closed"),
                                                         P_("Whether the connection is closed"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_BLURB |
                                                         G_PARAM_STATIC_NICK));

  /**
   * xdbus_connection_t:exit-on-close:
   *
   * A boolean specifying whether the process will be terminated (by
   * calling `raise(SIGTERM)`) if the connection is closed by the
   * remote peer.
   *
   * Note that #xdbus_connection_t objects returned by g_bus_get_finish()
   * and g_bus_get_sync() will (usually) have this property set to %TRUE.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_EXIT_ON_CLOSE,
                                   g_param_spec_boolean ("exit-on-close",
                                                         P_("Exit on close"),
                                                         P_("Whether the process is terminated when the connection is closed"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_WRITABLE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_BLURB |
                                                         G_PARAM_STATIC_NICK));

  /**
   * xdbus_connection_t:capabilities:
   *
   * Flags from the #GDBusCapabilityFlags enumeration
   * representing connection features negotiated with the other peer.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_CAPABILITY_FLAGS,
                                   g_param_spec_flags ("capabilities",
                                                       P_("Capabilities"),
                                                       P_("Capabilities"),
                                                       XTYPE_DBUS_CAPABILITY_FLAGS,
                                                       G_DBUS_CAPABILITY_FLAGS_NONE,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_BLURB |
                                                       G_PARAM_STATIC_NICK));

  /**
   * xdbus_connection_t:authentication-observer:
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
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_connection_t::closed:
   * @connection: the #xdbus_connection_t emitting the signal
   * @remote_peer_vanished: %TRUE if @connection is closed because the
   *     remote peer closed its end of the connection
   * @error: (nullable): a #xerror_t with more details about the event or %NULL
   *
   * Emitted when the connection is closed.
   *
   * The cause of this event can be
   *
   * - If g_dbus_connection_close() is called. In this case
   *   @remote_peer_vanished is set to %FALSE and @error is %NULL.
   *
   * - If the remote peer closes the connection. In this case
   *   @remote_peer_vanished is set to %TRUE and @error is set.
   *
   * - If the remote peer sends invalid or malformed data. In this
   *   case @remote_peer_vanished is set to %FALSE and @error is set.
   *
   * Upon receiving this signal, you should give up your reference to
   * @connection. You are guaranteed that this signal is emitted only
   * once.
   *
   * Since: 2.26
   */
  signals[CLOSED_SIGNAL] = g_signal_new (I_("closed"),
                                         XTYPE_DBUS_CONNECTION,
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (GDBusConnectionClass, closed),
                                         NULL,
                                         NULL,
                                         _g_cclosure_marshal_VOID__BOOLEAN_BOXED,
                                         XTYPE_NONE,
                                         2,
                                         XTYPE_BOOLEAN,
                                         XTYPE_ERROR);
  g_signal_set_va_marshaller (signals[CLOSED_SIGNAL],
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_VOID__BOOLEAN_BOXEDv);
}

static void
g_dbus_connection_init (xdbus_connection_t *connection)
{
  g_mutex_init (&connection->lock);
  g_mutex_init (&connection->init_lock);

  connection->map_method_serial_to_task = xhash_table_new (g_direct_hash, g_direct_equal);

  connection->map_rule_to_signal_data = xhash_table_new (xstr_hash,
                                                          xstr_equal);
  connection->map_id_to_signal_data = xhash_table_new (g_direct_hash,
                                                        g_direct_equal);
  connection->map_sender_unique_name_to_signal_data_array = xhash_table_new_full (xstr_hash,
                                                                                   xstr_equal,
                                                                                   g_free,
                                                                                   (xdestroy_notify_t) xptr_array_unref);

  connection->map_object_path_to_eo = xhash_table_new_full (xstr_hash,
                                                             xstr_equal,
                                                             NULL,
                                                             (xdestroy_notify_t) exported_object_free);

  connection->map_id_to_ei = xhash_table_new (g_direct_hash,
                                               g_direct_equal);

  connection->map_object_path_to_es = xhash_table_new_full (xstr_hash,
                                                             xstr_equal,
                                                             NULL,
                                                             (xdestroy_notify_t) exported_subtree_unref);

  connection->map_id_to_es = xhash_table_new (g_direct_hash,
                                               g_direct_equal);

  connection->map_thread_to_last_serial = xhash_table_new (g_direct_hash,
                                                            g_direct_equal);

  connection->main_context_at_construction = xmain_context_ref_thread_default ();

  connection->filters = xptr_array_new ();
}

/**
 * g_dbus_connection_get_stream:
 * @connection: a #xdbus_connection_t
 *
 * Gets the underlying stream used for IO.
 *
 * While the #xdbus_connection_t is active, it will interact with this
 * stream from a worker thread, so it is not safe to interact with
 * the stream directly.
 *
 * Returns: (transfer none) (not nullable): the stream used for IO
 *
 * Since: 2.26
 */
xio_stream_t *
g_dbus_connection_get_stream (xdbus_connection_t *connection)
{
  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);

  /* do not use g_return_val_if_fail(), we want the memory barrier */
  if (!check_initialized (connection))
    return NULL;

  return connection->stream;
}

/**
 * g_dbus_connection_start_message_processing:
 * @connection: a #xdbus_connection_t
 *
 * If @connection was created with
 * %G_DBUS_CONNECTION_FLAGS_DELAY_MESSAGE_PROCESSING, this method
 * starts processing messages. Does nothing on if @connection wasn't
 * created with this flag or if the method has already been called.
 *
 * Since: 2.26
 */
void
g_dbus_connection_start_message_processing (xdbus_connection_t *connection)
{
  g_return_if_fail (X_IS_DBUS_CONNECTION (connection));

  /* do not use g_return_val_if_fail(), we want the memory barrier */
  if (!check_initialized (connection))
    return;

  g_assert (connection->worker != NULL);
  _g_dbus_worker_unfreeze (connection->worker);
}

/**
 * g_dbus_connection_is_closed:
 * @connection: a #xdbus_connection_t
 *
 * Gets whether @connection is closed.
 *
 * Returns: %TRUE if the connection is closed, %FALSE otherwise
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_connection_is_closed (xdbus_connection_t *connection)
{
  xint_t flags;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);

  flags = g_atomic_int_get (&connection->atomic_flags);

  return (flags & FLAG_CLOSED) ? TRUE : FALSE;
}

/**
 * g_dbus_connection_get_capabilities:
 * @connection: a #xdbus_connection_t
 *
 * Gets the capabilities negotiated with the remote peer
 *
 * Returns: zero or more flags from the #GDBusCapabilityFlags enumeration
 *
 * Since: 2.26
 */
GDBusCapabilityFlags
g_dbus_connection_get_capabilities (xdbus_connection_t *connection)
{
  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), G_DBUS_CAPABILITY_FLAGS_NONE);

  /* do not use g_return_val_if_fail(), we want the memory barrier */
  if (!check_initialized (connection))
    return G_DBUS_CAPABILITY_FLAGS_NONE;

  return connection->capabilities;
}

/**
 * g_dbus_connection_get_flags:
 * @connection: a #xdbus_connection_t
 *
 * Gets the flags used to construct this connection
 *
 * Returns: zero or more flags from the #GDBusConnectionFlags enumeration
 *
 * Since: 2.60
 */
GDBusConnectionFlags
g_dbus_connection_get_flags (xdbus_connection_t *connection)
{
  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), G_DBUS_CONNECTION_FLAGS_NONE);

  /* do not use g_return_val_if_fail(), we want the memory barrier */
  if (!check_initialized (connection))
    return G_DBUS_CONNECTION_FLAGS_NONE;

  return connection->flags;
}

/* ---------------------------------------------------------------------------------------------------- */

/* Called in a temporary thread without holding locks. */
static void
flush_in_thread_func (xtask_t         *task,
                      xpointer_t       source_object,
                      xpointer_t       task_data,
                      xcancellable_t  *cancellable)
{
  xerror_t *error = NULL;

  if (g_dbus_connection_flush_sync (source_object,
                                    cancellable,
                                    &error))
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
}

/**
 * g_dbus_connection_flush:
 * @connection: a #xdbus_connection_t
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @callback: (nullable): a #xasync_ready_callback_t to call when the
 *     request is satisfied or %NULL if you don't care about the result
 * @user_data: The data to pass to @callback
 *
 * Asynchronously flushes @connection, that is, writes all queued
 * outgoing message to the transport and then flushes the transport
 * (using xoutput_stream_flush_async()). This is useful in programs
 * that wants to emit a D-Bus signal and then exit immediately. Without
 * flushing the connection, there is no guaranteed that the message has
 * been sent to the networking buffers in the OS kernel.
 *
 * This is an asynchronous method. When the operation is finished,
 * @callback will be invoked in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread you are calling this method from. You can
 * then call g_dbus_connection_flush_finish() to get the result of the
 * operation. See g_dbus_connection_flush_sync() for the synchronous
 * version.
 *
 * Since: 2.26
 */
void
g_dbus_connection_flush (xdbus_connection_t     *connection,
                         xcancellable_t        *cancellable,
                         xasync_ready_callback_t  callback,
                         xpointer_t             user_data)
{
  xtask_t *task;

  g_return_if_fail (X_IS_DBUS_CONNECTION (connection));

  task = xtask_new (connection, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_dbus_connection_flush);
  xtask_run_in_thread (task, flush_in_thread_func);
  xobject_unref (task);
}

/**
 * g_dbus_connection_flush_finish:
 * @connection: a #xdbus_connection_t
 * @res: a #xasync_result_t obtained from the #xasync_ready_callback_t passed
 *     to g_dbus_connection_flush()
 * @error: return location for error or %NULL
 *
 * Finishes an operation started with g_dbus_connection_flush().
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if @error is set
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_connection_flush_finish (xdbus_connection_t  *connection,
                                xasync_result_t     *res,
                                xerror_t          **error)
{
  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);
  g_return_val_if_fail (xtask_is_valid (res, connection), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return xtask_propagate_boolean (XTASK (res), error);
}

/**
 * g_dbus_connection_flush_sync:
 * @connection: a #xdbus_connection_t
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @error: return location for error or %NULL
 *
 * Synchronously flushes @connection. The calling thread is blocked
 * until this is done. See g_dbus_connection_flush() for the
 * asynchronous version of this method and more details about what it
 * does.
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if @error is set
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_connection_flush_sync (xdbus_connection_t  *connection,
                              xcancellable_t     *cancellable,
                              xerror_t          **error)
{
  xboolean_t ret;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ret = FALSE;

  /* This is only a best-effort attempt to see whether the connection is
   * closed, so it doesn't need the lock. If the connection closes just
   * after this check, but before scheduling the flush operation, the
   * result will be more or less the same as if the connection closed while
   * the flush operation was pending - it'll fail with either CLOSED or
   * CANCELLED.
   */
  if (!check_unclosed (connection, 0, error))
    goto out;

  g_assert (connection->worker != NULL);

  ret = _g_dbus_worker_flush_sync (connection->worker,
                                   cancellable,
                                   error);

 out:
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xdbus_connection_t *connection;
  xerror_t *error;
  xboolean_t remote_peer_vanished;
} EmitClosedData;

static void
emit_closed_data_free (EmitClosedData *data)
{
  xobject_unref (data->connection);
  if (data->error != NULL)
    xerror_free (data->error);
  g_free (data);
}

/* Called in a user thread that has acquired the main context that was
 * thread-default when the object was constructed
 */
static xboolean_t
emit_closed_in_idle (xpointer_t user_data)
{
  EmitClosedData *data = user_data;
  xboolean_t result;

  xobject_notify (G_OBJECT (data->connection), "closed");
  g_signal_emit (data->connection,
                 signals[CLOSED_SIGNAL],
                 0,
                 data->remote_peer_vanished,
                 data->error,
                 &result);
  return FALSE;
}

/* Can be called from any thread, must hold lock.
 * FLAG_CLOSED must already have been set.
 */
static void
schedule_closed_unlocked (xdbus_connection_t *connection,
                          xboolean_t         remote_peer_vanished,
                          xerror_t          *error)
{
  xsource_t *idle_source;
  EmitClosedData *data;

  CONNECTION_ENSURE_LOCK (connection);

  data = g_new0 (EmitClosedData, 1);
  data->connection = xobject_ref (connection);
  data->remote_peer_vanished = remote_peer_vanished;
  data->error = error != NULL ? xerror_copy (error) : NULL;

  idle_source = g_idle_source_new ();
  xsource_set_priority (idle_source, G_PRIORITY_DEFAULT);
  xsource_set_callback (idle_source,
                         emit_closed_in_idle,
                         data,
                         (xdestroy_notify_t) emit_closed_data_free);
  xsource_set_static_name (idle_source, "[gio] emit_closed_in_idle");
  xsource_attach (idle_source, connection->main_context_at_construction);
  xsource_unref (idle_source);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_connection_close:
 * @connection: a #xdbus_connection_t
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @callback: (nullable): a #xasync_ready_callback_t to call when the request is
 *     satisfied or %NULL if you don't care about the result
 * @user_data: The data to pass to @callback
 *
 * Closes @connection. Note that this never causes the process to
 * exit (this might only happen if the other end of a shared message
 * bus connection disconnects, see #xdbus_connection_t:exit-on-close).
 *
 * Once the connection is closed, operations such as sending a message
 * will return with the error %G_IO_ERROR_CLOSED. Closing a connection
 * will not automatically flush the connection so queued messages may
 * be lost. Use g_dbus_connection_flush() if you need such guarantees.
 *
 * If @connection is already closed, this method fails with
 * %G_IO_ERROR_CLOSED.
 *
 * When @connection has been closed, the #xdbus_connection_t::closed
 * signal is emitted in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread that @connection was constructed in.
 *
 * This is an asynchronous method. When the operation is finished,
 * @callback will be invoked in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread you are calling this method from. You can
 * then call g_dbus_connection_close_finish() to get the result of the
 * operation. See g_dbus_connection_close_sync() for the synchronous
 * version.
 *
 * Since: 2.26
 */
void
g_dbus_connection_close (xdbus_connection_t     *connection,
                         xcancellable_t        *cancellable,
                         xasync_ready_callback_t  callback,
                         xpointer_t             user_data)
{
  xtask_t *task;

  g_return_if_fail (X_IS_DBUS_CONNECTION (connection));

  /* do not use g_return_val_if_fail(), we want the memory barrier */
  if (!check_initialized (connection))
    return;

  g_assert (connection->worker != NULL);

  task = xtask_new (connection, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_dbus_connection_close);
  _g_dbus_worker_close (connection->worker, task);
  xobject_unref (task);
}

/**
 * g_dbus_connection_close_finish:
 * @connection: a #xdbus_connection_t
 * @res: a #xasync_result_t obtained from the #xasync_ready_callback_t passed
 *     to g_dbus_connection_close()
 * @error: return location for error or %NULL
 *
 * Finishes an operation started with g_dbus_connection_close().
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if @error is set
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_connection_close_finish (xdbus_connection_t  *connection,
                                xasync_result_t     *res,
                                xerror_t          **error)
{
  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);
  g_return_val_if_fail (xtask_is_valid (res, connection), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return xtask_propagate_boolean (XTASK (res), error);
}

typedef struct {
    xmain_loop_t *loop;
    xasync_result_t *result;
} SyncCloseData;

/* Can be called by any thread, without the connection lock */
static void
sync_close_cb (xobject_t *source_object,
               xasync_result_t *res,
               xpointer_t user_data)
{
  SyncCloseData *data = user_data;

  data->result = xobject_ref (res);
  xmain_loop_quit (data->loop);
}

/**
 * g_dbus_connection_close_sync:
 * @connection: a #xdbus_connection_t
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @error: return location for error or %NULL
 *
 * Synchronously closes @connection. The calling thread is blocked
 * until this is done. See g_dbus_connection_close() for the
 * asynchronous version of this method and more details about what it
 * does.
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if @error is set
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_connection_close_sync (xdbus_connection_t  *connection,
                              xcancellable_t     *cancellable,
                              xerror_t          **error)
{
  xboolean_t ret;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ret = FALSE;

  if (check_unclosed (connection, 0, error))
    {
      xmain_context_t *context;
      SyncCloseData data;

      context = xmain_context_new ();
      xmain_context_push_thread_default (context);
      data.loop = xmain_loop_new (context, TRUE);
      data.result = NULL;

      g_dbus_connection_close (connection, cancellable, sync_close_cb, &data);
      xmain_loop_run (data.loop);
      ret = g_dbus_connection_close_finish (connection, data.result, error);

      xobject_unref (data.result);
      xmain_loop_unref (data.loop);
      xmain_context_pop_thread_default (context);
      xmain_context_unref (context);
    }

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_connection_get_last_serial:
 * @connection: a #xdbus_connection_t
 *
 * Retrieves the last serial number assigned to a #xdbus_message_t on
 * the current thread. This includes messages sent via both low-level
 * API such as g_dbus_connection_send_message() as well as
 * high-level API such as g_dbus_connection_emit_signal(),
 * g_dbus_connection_call() or g_dbus_proxy_call().
 *
 * Returns: the last used serial or zero when no message has been sent
 *     within the current thread
 *
 * Since: 2.34
 */
xuint32_t
g_dbus_connection_get_last_serial (xdbus_connection_t *connection)
{
  xuint32_t ret;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), 0);

  CONNECTION_LOCK (connection);
  ret = GPOINTER_TO_UINT (xhash_table_lookup (connection->map_thread_to_last_serial,
                                               xthread_self ()));
  CONNECTION_UNLOCK (connection);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/* Can be called by any thread, with the connection lock held */
static xboolean_t
g_dbus_connection_send_message_unlocked (xdbus_connection_t   *connection,
                                         xdbus_message_t      *message,
                                         GDBusSendMessageFlags flags,
                                         xuint32_t           *out_serial,
                                         xerror_t           **error)
{
  guchar *blob;
  xsize_t blob_size;
  xuint32_t serial_to_use;

  CONNECTION_ENSURE_LOCK (connection);

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), FALSE);

  /* TODO: check all necessary headers are present */

  if (out_serial != NULL)
    *out_serial = 0;

  /* If we're in initable_init(), don't check for being initialized, to avoid
   * chicken-and-egg problems. initable_init() is responsible for setting up
   * our prerequisites (mainly connection->worker), and only calling us
   * from its own thread (so no memory barrier is needed).
   */
  if (!check_unclosed (connection,
                       (flags & SEND_MESSAGE_FLAGS_INITIALIZING) ? MAY_BE_UNINITIALIZED : 0,
                       error))
    return FALSE;

  blob = xdbus_message_to_blob (message,
                                 &blob_size,
                                 connection->capabilities,
                                 error);
  if (blob == NULL)
    return FALSE;

  if (flags & G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL)
    serial_to_use = xdbus_message_get_serial (message);
  else
    serial_to_use = ++connection->last_serial; /* TODO: handle overflow */

  switch (blob[0])
    {
    case 'l':
      ((xuint32_t *) blob)[2] = GUINT32_TO_LE (serial_to_use);
      break;
    case 'B':
      ((xuint32_t *) blob)[2] = GUINT32_TO_BE (serial_to_use);
      break;
    default:
      g_assert_not_reached ();
      break;
    }

#if 0
  g_printerr ("Writing message of %" G_GSIZE_FORMAT " bytes (serial %d) on %p:\n",
              blob_size, serial_to_use, connection);
  g_printerr ("----\n");
  hexdump (blob, blob_size);
  g_printerr ("----\n");
#endif

  /* TODO: use connection->auth to encode the blob */

  if (out_serial != NULL)
    *out_serial = serial_to_use;

  /* store used serial for the current thread */
  /* TODO: watch the thread disposal and remove associated record
   *       from hashtable
   *  - see https://bugzilla.gnome.org/show_bug.cgi?id=676825#c7
   */
  xhash_table_replace (connection->map_thread_to_last_serial,
                        xthread_self (),
                        GUINT_TO_POINTER (serial_to_use));

  if (!(flags & G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL))
    xdbus_message_set_serial (message, serial_to_use);

  xdbus_message_lock (message);

  _g_dbus_worker_send_message (connection->worker,
                               message,
                               (xchar_t*) blob, /* transfer ownership */
                               blob_size);

  return TRUE;
}

/**
 * g_dbus_connection_send_message:
 * @connection: a #xdbus_connection_t
 * @message: a #xdbus_message_t
 * @flags: flags affecting how the message is sent
 * @out_serial: (out) (optional): return location for serial number assigned
 *     to @message when sending it or %NULL
 * @error: Return location for error or %NULL
 *
 * Asynchronously sends @message to the peer represented by @connection.
 *
 * Unless @flags contain the
 * %G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL flag, the serial number
 * will be assigned by @connection and set on @message via
 * xdbus_message_set_serial(). If @out_serial is not %NULL, then the
 * serial number used will be written to this location prior to
 * submitting the message to the underlying transport. While it has a `volatile`
 * qualifier, this is a historical artifact and the argument passed to it should
 * not be `volatile`.
 *
 * If @connection is closed then the operation will fail with
 * %G_IO_ERROR_CLOSED. If @message is not well-formed,
 * the operation fails with %G_IO_ERROR_INVALID_ARGUMENT.
 *
 * See this [server][gdbus-server] and [client][gdbus-unix-fd-client]
 * for an example of how to use this low-level API to send and receive
 * UNIX file descriptors.
 *
 * Note that @message must be unlocked, unless @flags contain the
 * %G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL flag.
 *
 * Returns: %TRUE if the message was well-formed and queued for
 *     transmission, %FALSE if @error is set
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_connection_send_message (xdbus_connection_t        *connection,
                                xdbus_message_t           *message,
                                GDBusSendMessageFlags   flags,
                                volatile xuint32_t       *out_serial,
                                xerror_t                **error)
{
  xboolean_t ret;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), FALSE);
  g_return_val_if_fail ((flags & G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL) || !xdbus_message_get_locked (message), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  CONNECTION_LOCK (connection);
  ret = g_dbus_connection_send_message_unlocked (connection, message, flags, (xuint32_t *) out_serial, error);
  CONNECTION_UNLOCK (connection);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xuint32_t serial;

  gulong cancellable_handler_id;

  xsource_t *timeout_source;

  xboolean_t delivered;
} SendMessageData;

/* Can be called from any thread with or without lock held */
static void
send_message_data_free (SendMessageData *data)
{
  g_assert (data->timeout_source == NULL);
  g_assert (data->cancellable_handler_id == 0);

  g_slice_free (SendMessageData, data);
}

/* ---------------------------------------------------------------------------------------------------- */

/* can be called from any thread with lock held; @task is (transfer full) */
static void
send_message_with_reply_cleanup (xtask_t *task, xboolean_t remove)
{
  xdbus_connection_t *connection = xtask_get_source_object (task);
  SendMessageData *data = xtask_get_task_data (task);

  CONNECTION_ENSURE_LOCK (connection);

  g_assert (!data->delivered);

  data->delivered = TRUE;

  if (data->timeout_source != NULL)
    {
      xsource_destroy (data->timeout_source);
      data->timeout_source = NULL;
    }
  if (data->cancellable_handler_id > 0)
    {
      g_cancellable_disconnect (xtask_get_cancellable (task), data->cancellable_handler_id);
      data->cancellable_handler_id = 0;
    }

  if (remove)
    {
      xboolean_t removed = xhash_table_remove (connection->map_method_serial_to_task,
                                              GUINT_TO_POINTER (data->serial));
      g_warn_if_fail (removed);
    }

  xobject_unref (task);
}

/* ---------------------------------------------------------------------------------------------------- */

/* Called from GDBus worker thread with lock held; @task is (transfer full). */
static void
send_message_data_deliver_reply_unlocked (xtask_t           *task,
                                          xdbus_message_t    *reply)
{
  SendMessageData *data = xtask_get_task_data (task);

  if (data->delivered)
    goto out;

  xtask_return_pointer (task, xobject_ref (reply), xobject_unref);

  send_message_with_reply_cleanup (task, TRUE);

 out:
  ;
}

/* Called from a user thread, lock is not held */
static void
send_message_data_deliver_error (xtask_t      *task,
                                 xquark      domain,
                                 xint_t        code,
                                 const char *message)
{
  xdbus_connection_t *connection = xtask_get_source_object (task);
  SendMessageData *data = xtask_get_task_data (task);

  CONNECTION_LOCK (connection);
  if (data->delivered)
    {
      CONNECTION_UNLOCK (connection);
      return;
    }

  xobject_ref (task);
  send_message_with_reply_cleanup (task, TRUE);
  CONNECTION_UNLOCK (connection);

  xtask_return_new_error (task, domain, code, "%s", message);
  xobject_unref (task);
}

/* ---------------------------------------------------------------------------------------------------- */

/* Called from a user thread, lock is not held; @task is (transfer full) */
static xboolean_t
send_message_with_reply_cancelled_idle_cb (xpointer_t user_data)
{
  xtask_t *task = user_data;

  send_message_data_deliver_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED,
                                   _("Operation was cancelled"));
  return FALSE;
}

/* Can be called from any thread with or without lock held */
static void
send_message_with_reply_cancelled_cb (xcancellable_t *cancellable,
                                      xpointer_t      user_data)
{
  xtask_t *task = user_data;
  xsource_t *idle_source;

  /* postpone cancellation to idle handler since we may be called directly
   * via g_cancellable_connect() (e.g. holding lock)
   */
  idle_source = g_idle_source_new ();
  xsource_set_static_name (idle_source, "[gio] send_message_with_reply_cancelled_idle_cb");
  xtask_attach_source (task, idle_source, send_message_with_reply_cancelled_idle_cb);
  xsource_unref (idle_source);
}

/* ---------------------------------------------------------------------------------------------------- */

/* Called from a user thread, lock is not held; @task is (transfer full) */
static xboolean_t
send_message_with_reply_timeout_cb (xpointer_t user_data)
{
  xtask_t *task = user_data;

  send_message_data_deliver_error (task, G_IO_ERROR, G_IO_ERROR_TIMED_OUT,
                                   _("Timeout was reached"));
  return FALSE;
}

/* ---------------------------------------------------------------------------------------------------- */

/* Called from a user thread, connection's lock is held */
static void
g_dbus_connection_send_message_with_reply_unlocked (xdbus_connection_t     *connection,
                                                    xdbus_message_t        *message,
                                                    GDBusSendMessageFlags flags,
                                                    xint_t                 timeout_msec,
                                                    xuint32_t             *out_serial,
                                                    xcancellable_t        *cancellable,
                                                    xasync_ready_callback_t  callback,
                                                    xpointer_t             user_data)
{
  xtask_t *task;
  SendMessageData *data;
  xerror_t *error = NULL;
  xuint32_t serial;

  if (out_serial == NULL)
    out_serial = &serial;

  if (timeout_msec == -1)
    timeout_msec = 25 * 1000;

  data = g_slice_new0 (SendMessageData);
  task = xtask_new (connection, cancellable, callback, user_data);
  xtask_set_source_tag (task,
                         g_dbus_connection_send_message_with_reply_unlocked);
  xtask_set_task_data (task, data, (xdestroy_notify_t) send_message_data_free);

  if (xtask_return_error_if_cancelled (task))
    {
      xobject_unref (task);
      return;
    }

  if (!g_dbus_connection_send_message_unlocked (connection, message, flags, out_serial, &error))
    {
      xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }
  data->serial = *out_serial;

  if (cancellable != NULL)
    {
      data->cancellable_handler_id = g_cancellable_connect (cancellable,
                                                            G_CALLBACK (send_message_with_reply_cancelled_cb),
                                                            xobject_ref (task),
                                                            xobject_unref);
    }

  if (timeout_msec != G_MAXINT)
    {
      data->timeout_source = g_timeout_source_new (timeout_msec);
      xtask_attach_source (task, data->timeout_source,
                            (xsource_func_t) send_message_with_reply_timeout_cb);
      xsource_unref (data->timeout_source);
    }

  xhash_table_insert (connection->map_method_serial_to_task,
                       GUINT_TO_POINTER (*out_serial),
                       g_steal_pointer (&task));
}

/**
 * g_dbus_connection_send_message_with_reply:
 * @connection: a #xdbus_connection_t
 * @message: a #xdbus_message_t
 * @flags: flags affecting how the message is sent
 * @timeout_msec: the timeout in milliseconds, -1 to use the default
 *     timeout or %G_MAXINT for no timeout
 * @out_serial: (out) (optional): return location for serial number assigned
 *     to @message when sending it or %NULL
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @callback: (nullable): a #xasync_ready_callback_t to call when the request
 *     is satisfied or %NULL if you don't care about the result
 * @user_data: The data to pass to @callback
 *
 * Asynchronously sends @message to the peer represented by @connection.
 *
 * Unless @flags contain the
 * %G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL flag, the serial number
 * will be assigned by @connection and set on @message via
 * xdbus_message_set_serial(). If @out_serial is not %NULL, then the
 * serial number used will be written to this location prior to
 * submitting the message to the underlying transport. While it has a `volatile`
 * qualifier, this is a historical artifact and the argument passed to it should
 * not be `volatile`.
 *
 * If @connection is closed then the operation will fail with
 * %G_IO_ERROR_CLOSED. If @cancellable is canceled, the operation will
 * fail with %G_IO_ERROR_CANCELLED. If @message is not well-formed,
 * the operation fails with %G_IO_ERROR_INVALID_ARGUMENT.
 *
 * This is an asynchronous method. When the operation is finished, @callback
 * will be invoked in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread you are calling this method from. You can then call
 * g_dbus_connection_send_message_with_reply_finish() to get the result of the operation.
 * See g_dbus_connection_send_message_with_reply_sync() for the synchronous version.
 *
 * Note that @message must be unlocked, unless @flags contain the
 * %G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL flag.
 *
 * See this [server][gdbus-server] and [client][gdbus-unix-fd-client]
 * for an example of how to use this low-level API to send and receive
 * UNIX file descriptors.
 *
 * Since: 2.26
 */
void
g_dbus_connection_send_message_with_reply (xdbus_connection_t       *connection,
                                           xdbus_message_t          *message,
                                           GDBusSendMessageFlags  flags,
                                           xint_t                   timeout_msec,
                                           volatile xuint32_t      *out_serial,
                                           xcancellable_t          *cancellable,
                                           xasync_ready_callback_t    callback,
                                           xpointer_t               user_data)
{
  g_return_if_fail (X_IS_DBUS_CONNECTION (connection));
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail ((flags & G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL) || !xdbus_message_get_locked (message));
  g_return_if_fail (timeout_msec >= 0 || timeout_msec == -1);

  CONNECTION_LOCK (connection);
  g_dbus_connection_send_message_with_reply_unlocked (connection,
                                                      message,
                                                      flags,
                                                      timeout_msec,
                                                      (xuint32_t *) out_serial,
                                                      cancellable,
                                                      callback,
                                                      user_data);
  CONNECTION_UNLOCK (connection);
}

/**
 * g_dbus_connection_send_message_with_reply_finish:
 * @connection: a #xdbus_connection_t
 * @res: a #xasync_result_t obtained from the #xasync_ready_callback_t passed to
 *     g_dbus_connection_send_message_with_reply()
 * @error: teturn location for error or %NULL
 *
 * Finishes an operation started with g_dbus_connection_send_message_with_reply().
 *
 * Note that @error is only set if a local in-process error
 * occurred. That is to say that the returned #xdbus_message_t object may
 * be of type %G_DBUS_MESSAGE_TYPE_ERROR. Use
 * xdbus_message_to_gerror() to transcode this to a #xerror_t.
 *
 * See this [server][gdbus-server] and [client][gdbus-unix-fd-client]
 * for an example of how to use this low-level API to send and receive
 * UNIX file descriptors.
 *
 * Returns: (transfer full): a locked #xdbus_message_t or %NULL if @error is set
 *
 * Since: 2.26
 */
xdbus_message_t *
g_dbus_connection_send_message_with_reply_finish (xdbus_connection_t  *connection,
                                                  xasync_result_t     *res,
                                                  xerror_t          **error)
{
  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (xtask_is_valid (res, connection), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xasync_result_t *res;
  xmain_context_t *context;
  xmain_loop_t *loop;
} SendMessageSyncData;

/* Called from a user thread, lock is not held */
static void
send_message_with_reply_sync_cb (xdbus_connection_t *connection,
                                 xasync_result_t    *res,
                                 xpointer_t         user_data)
{
  SendMessageSyncData *data = user_data;
  data->res = xobject_ref (res);
  xmain_loop_quit (data->loop);
}

/**
 * g_dbus_connection_send_message_with_reply_sync:
 * @connection: a #xdbus_connection_t
 * @message: a #xdbus_message_t
 * @flags: flags affecting how the message is sent.
 * @timeout_msec: the timeout in milliseconds, -1 to use the default
 *     timeout or %G_MAXINT for no timeout
 * @out_serial: (out) (optional): return location for serial number
 *     assigned to @message when sending it or %NULL
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @error: return location for error or %NULL
 *
 * Synchronously sends @message to the peer represented by @connection
 * and blocks the calling thread until a reply is received or the
 * timeout is reached. See g_dbus_connection_send_message_with_reply()
 * for the asynchronous version of this method.
 *
 * Unless @flags contain the
 * %G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL flag, the serial number
 * will be assigned by @connection and set on @message via
 * xdbus_message_set_serial(). If @out_serial is not %NULL, then the
 * serial number used will be written to this location prior to
 * submitting the message to the underlying transport. While it has a `volatile`
 * qualifier, this is a historical artifact and the argument passed to it should
 * not be `volatile`.
 *
 * If @connection is closed then the operation will fail with
 * %G_IO_ERROR_CLOSED. If @cancellable is canceled, the operation will
 * fail with %G_IO_ERROR_CANCELLED. If @message is not well-formed,
 * the operation fails with %G_IO_ERROR_INVALID_ARGUMENT.
 *
 * Note that @error is only set if a local in-process error
 * occurred. That is to say that the returned #xdbus_message_t object may
 * be of type %G_DBUS_MESSAGE_TYPE_ERROR. Use
 * xdbus_message_to_gerror() to transcode this to a #xerror_t.
 *
 * See this [server][gdbus-server] and [client][gdbus-unix-fd-client]
 * for an example of how to use this low-level API to send and receive
 * UNIX file descriptors.
 *
 * Note that @message must be unlocked, unless @flags contain the
 * %G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL flag.
 *
 * Returns: (transfer full): a locked #xdbus_message_t that is the reply
 *     to @message or %NULL if @error is set
 *
 * Since: 2.26
 */
xdbus_message_t *
g_dbus_connection_send_message_with_reply_sync (xdbus_connection_t        *connection,
                                                xdbus_message_t           *message,
                                                GDBusSendMessageFlags   flags,
                                                xint_t                    timeout_msec,
                                                volatile xuint32_t       *out_serial,
                                                xcancellable_t           *cancellable,
                                                xerror_t                **error)
{
  SendMessageSyncData data;
  xdbus_message_t *reply;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  g_return_val_if_fail ((flags & G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL) || !xdbus_message_get_locked (message), NULL);
  g_return_val_if_fail (timeout_msec >= 0 || timeout_msec == -1, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  data.res = NULL;
  data.context = xmain_context_new ();
  data.loop = xmain_loop_new (data.context, FALSE);

  xmain_context_push_thread_default (data.context);

  g_dbus_connection_send_message_with_reply (connection,
                                             message,
                                             flags,
                                             timeout_msec,
                                             out_serial,
                                             cancellable,
                                             (xasync_ready_callback_t) send_message_with_reply_sync_cb,
                                             &data);
  xmain_loop_run (data.loop);
  reply = g_dbus_connection_send_message_with_reply_finish (connection,
                                                            data.res,
                                                            error);

  xmain_context_pop_thread_default (data.context);

  xmain_context_unref (data.context);
  xmain_loop_unref (data.loop);
  if (data.res)
    xobject_unref (data.res);

  return reply;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xuint_t                       id;
  xuint_t                       ref_count;
  GDBusMessageFilterFunction  filter_function;
  xpointer_t                    user_data;
  xdestroy_notify_t              user_data_free_func;
  xmain_context_t               *context;
} FilterData;

static void
filter_data_destroy (FilterData *filter, xboolean_t notify_sync)
{
  if (notify_sync)
    {
      if (filter->user_data_free_func != NULL)
        filter->user_data_free_func (filter->user_data);
    }
  else
    {
      call_destroy_notify (filter->context,
                           filter->user_data_free_func,
                           filter->user_data);
    }
  xmain_context_unref (filter->context);
  g_free (filter);
}

/* requires CONNECTION_LOCK */
static FilterData **
copy_filter_list (xptr_array_t *filters)
{
  FilterData **copy;
  xuint_t n;

  copy = g_new (FilterData *, filters->len + 1);
  for (n = 0; n < filters->len; n++)
    {
      copy[n] = filters->pdata[n];
      copy[n]->ref_count++;
    }
  copy[n] = NULL;

  return copy;
}

/* requires CONNECTION_LOCK */
static void
free_filter_list (FilterData **filters)
{
  xuint_t n;

  for (n = 0; filters[n]; n++)
    {
      filters[n]->ref_count--;
      if (filters[n]->ref_count == 0)
        filter_data_destroy (filters[n], FALSE);
    }
  g_free (filters);
}

/* Called in GDBusWorker's thread - we must not block - with no lock held */
static void
on_worker_message_received (GDBusWorker  *worker,
                            xdbus_message_t *message,
                            xpointer_t      user_data)
{
  xdbus_connection_t *connection;
  FilterData **filters;
  xuint_t n;
  xboolean_t alive;

  G_LOCK (message_bus_lock);
  alive = xhash_table_contains (alive_connections, user_data);
  if (!alive)
    {
      G_UNLOCK (message_bus_lock);
      return;
    }
  connection = G_DBUS_CONNECTION (user_data);
  xobject_ref (connection);
  G_UNLOCK (message_bus_lock);

  //g_debug ("in on_worker_message_received");

  xobject_ref (message);
  xdbus_message_lock (message);

  //g_debug ("boo ref_count = %d %p %p", G_OBJECT (connection)->ref_count, connection, connection->worker);

  /* First collect the set of callback functions */
  CONNECTION_LOCK (connection);
  filters = copy_filter_list (connection->filters);
  CONNECTION_UNLOCK (connection);

  /* then call the filters in order (without holding the lock) */
  for (n = 0; filters[n]; n++)
    {
      message = filters[n]->filter_function (connection,
                                             message,
                                             TRUE,
                                             filters[n]->user_data);
      if (message == NULL)
        break;
      xdbus_message_lock (message);
    }

  CONNECTION_LOCK (connection);
  free_filter_list (filters);
  CONNECTION_UNLOCK (connection);

  /* Standard dispatch unless the filter ate the message - no need to
   * do anything if the message was altered
   */
  if (message != NULL)
    {
      GDBusMessageType message_type;

      message_type = xdbus_message_get_message_type (message);
      if (message_type == G_DBUS_MESSAGE_TYPE_METHOD_RETURN || message_type == G_DBUS_MESSAGE_TYPE_ERROR)
        {
          xuint32_t reply_serial;
          xtask_t *task;

          reply_serial = xdbus_message_get_reply_serial (message);
          CONNECTION_LOCK (connection);
          task = xhash_table_lookup (connection->map_method_serial_to_task,
                                      GUINT_TO_POINTER (reply_serial));
          if (task != NULL)
            {
              /* This removes @task from @map_method_serial_to_task. */
              //g_debug ("delivering reply/error for serial %d for %p", reply_serial, connection);
              send_message_data_deliver_reply_unlocked (task, message);
            }
          else
            {
              //g_debug ("message reply/error for serial %d but no SendMessageData found for %p", reply_serial, connection);
            }
          CONNECTION_UNLOCK (connection);
        }
      else if (message_type == G_DBUS_MESSAGE_TYPE_SIGNAL)
        {
          CONNECTION_LOCK (connection);
          distribute_signals (connection, message);
          CONNECTION_UNLOCK (connection);
        }
      else if (message_type == G_DBUS_MESSAGE_TYPE_METHOD_CALL)
        {
          CONNECTION_LOCK (connection);
          distribute_method_call (connection, message);
          CONNECTION_UNLOCK (connection);
        }
    }

  if (message != NULL)
    xobject_unref (message);
  xobject_unref (connection);
}

/* Called in GDBusWorker's thread, lock is not held */
static xdbus_message_t *
on_worker_message_about_to_be_sent (GDBusWorker  *worker,
                                    xdbus_message_t *message,
                                    xpointer_t      user_data)
{
  xdbus_connection_t *connection;
  FilterData **filters;
  xuint_t n;
  xboolean_t alive;

  G_LOCK (message_bus_lock);
  alive = xhash_table_contains (alive_connections, user_data);
  if (!alive)
    {
      G_UNLOCK (message_bus_lock);
      return message;
    }
  connection = G_DBUS_CONNECTION (user_data);
  xobject_ref (connection);
  G_UNLOCK (message_bus_lock);

  //g_debug ("in on_worker_message_about_to_be_sent");

  /* First collect the set of callback functions */
  CONNECTION_LOCK (connection);
  filters = copy_filter_list (connection->filters);
  CONNECTION_UNLOCK (connection);

  /* then call the filters in order (without holding the lock) */
  for (n = 0; filters[n]; n++)
    {
      xdbus_message_lock (message);
      message = filters[n]->filter_function (connection,
                                             message,
                                             FALSE,
                                             filters[n]->user_data);
      if (message == NULL)
        break;
    }

  CONNECTION_LOCK (connection);
  free_filter_list (filters);
  CONNECTION_UNLOCK (connection);

  xobject_unref (connection);

  return message;
}

/* called with connection lock held, in GDBusWorker thread */
static xboolean_t
cancel_method_on_close (xpointer_t key, xpointer_t value, xpointer_t user_data)
{
  xtask_t *task = value;
  SendMessageData *data = xtask_get_task_data (task);

  if (data->delivered)
    return FALSE;

  xtask_return_new_error (task,
                           G_IO_ERROR,
                           G_IO_ERROR_CLOSED,
                           _("The connection is closed"));

  /* Ask send_message_with_reply_cleanup not to remove the element from the
   * hash table - we're in the middle of a foreach; that would be unsafe.
   * Instead, return TRUE from this function so that it gets removed safely.
   */
  send_message_with_reply_cleanup (task, FALSE);
  return TRUE;
}

/* Called in GDBusWorker's thread - we must not block - without lock held */
static void
on_worker_closed (GDBusWorker *worker,
                  xboolean_t     remote_peer_vanished,
                  xerror_t      *error,
                  xpointer_t     user_data)
{
  xdbus_connection_t *connection;
  xboolean_t alive;
  xuint_t old_atomic_flags;

  G_LOCK (message_bus_lock);
  alive = xhash_table_contains (alive_connections, user_data);
  if (!alive)
    {
      G_UNLOCK (message_bus_lock);
      return;
    }
  connection = G_DBUS_CONNECTION (user_data);
  xobject_ref (connection);
  G_UNLOCK (message_bus_lock);

  //g_debug ("in on_worker_closed: %s", error->message);

  CONNECTION_LOCK (connection);
  /* Even though this is atomic, we do it inside the lock to avoid breaking
   * assumptions in remove_match_rule(). We'd need the lock in a moment
   * anyway, so, no loss.
   */
  old_atomic_flags = g_atomic_int_or (&connection->atomic_flags, FLAG_CLOSED);

  if (!(old_atomic_flags & FLAG_CLOSED))
    {
      xhash_table_foreach_remove (connection->map_method_serial_to_task, cancel_method_on_close, NULL);
      schedule_closed_unlocked (connection, remote_peer_vanished, error);
    }
  CONNECTION_UNLOCK (connection);

  xobject_unref (connection);
}

/* ---------------------------------------------------------------------------------------------------- */

/* Determines the biggest set of capabilities we can support on this
 * connection.
 *
 * Called with the init_lock held.
 */
static GDBusCapabilityFlags
get_offered_capabilities_max (xdbus_connection_t *connection)
{
      GDBusCapabilityFlags ret;
      ret = G_DBUS_CAPABILITY_FLAGS_NONE;
#ifdef G_OS_UNIX
      if (X_IS_UNIX_CONNECTION (connection->stream))
        ret |= G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING;
#endif
      return ret;
}

/* Called in a user thread, lock is not held */
static xboolean_t
initable_init (xinitable_t     *initable,
               xcancellable_t  *cancellable,
               xerror_t       **error)
{
  xdbus_connection_t *connection = G_DBUS_CONNECTION (initable);
  xboolean_t ret;

  /* This method needs to be idempotent to work with the singleton
   * pattern. See the docs for xinitable_init(). We implement this by
   * locking.
   *
   * Unfortunately we can't use the main lock since the on_worker_*()
   * callbacks above needs the lock during initialization (for message
   * bus connections we do a synchronous Hello() call on the bus).
   */
  g_mutex_lock (&connection->init_lock);

  ret = FALSE;

  /* Make this a no-op if we're already initialized (successfully or
   * unsuccessfully)
   */
  if ((g_atomic_int_get (&connection->atomic_flags) & FLAG_INITIALIZED))
    {
      ret = (connection->initialization_error == NULL);
      goto out;
    }

  /* Because of init_lock, we can't get here twice in different threads */
  g_assert (connection->initialization_error == NULL);

  /* The user can pass multiple (but mutally exclusive) construct
   * properties:
   *
   *  - stream (of type xio_stream_t)
   *  - address (of type xchar_t*)
   *
   * At the end of the day we end up with a non-NULL xio_stream_t
   * object in connection->stream.
   */
  if (connection->address != NULL)
    {
      g_assert (connection->stream == NULL);

      if ((connection->flags & G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER) ||
          (connection->flags & G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS) ||
          (connection->flags & G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER))
        {
          g_set_error_literal (&connection->initialization_error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("Unsupported flags encountered when constructing a client-side connection"));
          goto out;
        }

      connection->stream = g_dbus_address_get_stream_sync (connection->address,
                                                           NULL, /* TODO: out_guid */
                                                           cancellable,
                                                           &connection->initialization_error);
      if (connection->stream == NULL)
        goto out;
    }
  else if (connection->stream != NULL)
    {
      /* nothing to do */
    }
  else
    {
      g_assert_not_reached ();
    }

  /* Authenticate the connection */
  if (connection->flags & G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER)
    {
      g_assert (!(connection->flags & G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT));
      g_assert (connection->guid != NULL);
      connection->auth = _g_dbus_auth_new (connection->stream);
      if (!_g_dbus_auth_run_server (connection->auth,
                                    connection->authentication_observer,
                                    connection->guid,
                                    (connection->flags & G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS),
                                    (connection->flags & G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER),
                                    get_offered_capabilities_max (connection),
                                    &connection->capabilities,
                                    &connection->credentials,
                                    cancellable,
                                    &connection->initialization_error))
        goto out;
    }
  else if (connection->flags & G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT)
    {
      g_assert (!(connection->flags & G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER));
      g_assert (connection->guid == NULL);
      connection->auth = _g_dbus_auth_new (connection->stream);
      connection->guid = _g_dbus_auth_run_client (connection->auth,
                                                  connection->authentication_observer,
                                                  get_offered_capabilities_max (connection),
                                                  &connection->capabilities,
                                                  cancellable,
                                                  &connection->initialization_error);
      if (connection->guid == NULL)
        goto out;
    }

  if (connection->authentication_observer != NULL)
    {
      xobject_unref (connection->authentication_observer);
      connection->authentication_observer = NULL;
    }

  //xoutput_stream_flush (XSOCKET_CONNECTION (connection->stream)

  //g_debug ("haz unix fd passing powers: %d", connection->capabilities & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING);

#ifdef G_OS_UNIX
  /* We want all IO operations to be non-blocking since they happen in
   * the worker thread which is shared by _all_ connections.
   */
  if (X_IS_SOCKET_CONNECTION (connection->stream))
    {
      xsocket_set_blocking (xsocket_connection_get_socket (XSOCKET_CONNECTION (connection->stream)), FALSE);
    }
#endif

  G_LOCK (message_bus_lock);
  if (alive_connections == NULL)
    alive_connections = xhash_table_new (g_direct_hash, g_direct_equal);
  xhash_table_add (alive_connections, connection);
  G_UNLOCK (message_bus_lock);

  connection->worker = _g_dbus_worker_new (connection->stream,
                                           connection->capabilities,
                                           ((connection->flags & G_DBUS_CONNECTION_FLAGS_DELAY_MESSAGE_PROCESSING) != 0),
                                           on_worker_message_received,
                                           on_worker_message_about_to_be_sent,
                                           on_worker_closed,
                                           connection);

  /* if a bus connection, call org.freedesktop.DBus.Hello - this is how we're getting a name */
  if (connection->flags & G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION)
    {
      xvariant_t *hello_result;

      /* we could lift this restriction by adding code in gdbusprivate.c */
      if (connection->flags & G_DBUS_CONNECTION_FLAGS_DELAY_MESSAGE_PROCESSING)
        {
          g_set_error_literal (&connection->initialization_error,
                               G_IO_ERROR,
                               G_IO_ERROR_FAILED,
                               "Cannot use DELAY_MESSAGE_PROCESSING with MESSAGE_BUS_CONNECTION");
          goto out;
        }

      hello_result = g_dbus_connection_call_sync (connection,
                                                  "org.freedesktop.DBus", /* name */
                                                  "/org/freedesktop/DBus", /* path */
                                                  "org.freedesktop.DBus", /* interface */
                                                  "Hello",
                                                  NULL, /* parameters */
                                                  G_VARIANT_TYPE ("(s)"),
                                                  CALL_FLAGS_INITIALIZING,
                                                  -1,
                                                  NULL, /* TODO: cancellable */
                                                  &connection->initialization_error);
      if (hello_result == NULL)
        goto out;

      xvariant_get (hello_result, "(s)", &connection->bus_unique_name);
      xvariant_unref (hello_result);
      //g_debug ("unique name is '%s'", connection->bus_unique_name);
    }

  ret = TRUE;
 out:
  if (!ret)
    {
      g_assert (connection->initialization_error != NULL);
      g_propagate_error (error, xerror_copy (connection->initialization_error));
    }

  g_atomic_int_or (&connection->atomic_flags, FLAG_INITIALIZED);
  g_mutex_unlock (&connection->init_lock);

  return ret;
}

static void
initable_iface_init (xinitable_iface_t *initable_iface)
{
  initable_iface->init = initable_init;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
async_initable_iface_init (xasync_initable_iface_t *async_initable_iface)
{
  /* Use default */
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_connection_new:
 * @stream: a #xio_stream_t
 * @guid: (nullable): the GUID to use if authenticating as a server or %NULL
 * @flags: flags describing how to make the connection
 * @observer: (nullable): a #xdbus_auth_observer_t or %NULL
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @callback: a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: the data to pass to @callback
 *
 * Asynchronously sets up a D-Bus connection for exchanging D-Bus messages
 * with the end represented by @stream.
 *
 * If @stream is a #xsocket_connection_t, then the corresponding #xsocket_t
 * will be put into non-blocking mode.
 *
 * The D-Bus connection will interact with @stream from a worker thread.
 * As a result, the caller should not interact with @stream after this
 * method has been called, except by calling xobject_unref() on it.
 *
 * If @observer is not %NULL it may be used to control the
 * authentication process.
 *
 * When the operation is finished, @callback will be invoked. You can
 * then call g_dbus_connection_new_finish() to get the result of the
 * operation.
 *
 * This is an asynchronous failable constructor. See
 * g_dbus_connection_new_sync() for the synchronous
 * version.
 *
 * Since: 2.26
 */
void
g_dbus_connection_new (xio_stream_t            *stream,
                       const xchar_t          *guid,
                       GDBusConnectionFlags  flags,
                       xdbus_auth_observer_t    *observer,
                       xcancellable_t         *cancellable,
                       xasync_ready_callback_t   callback,
                       xpointer_t              user_data)
{
  _g_dbus_initialize ();

  g_return_if_fail (X_IS_IO_STREAM (stream));
  g_return_if_fail ((flags & ~G_DBUS_CONNECTION_FLAGS_ALL) == 0);

  xasync_initable_new_async (XTYPE_DBUS_CONNECTION,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "stream", stream,
                              "guid", guid,
                              "flags", flags,
                              "authentication-observer", observer,
                              NULL);
}

/**
 * g_dbus_connection_new_finish:
 * @res: a #xasync_result_t obtained from the #xasync_ready_callback_t
 *     passed to g_dbus_connection_new().
 * @error: return location for error or %NULL
 *
 * Finishes an operation started with g_dbus_connection_new().
 *
 * Returns: (transfer full): a #xdbus_connection_t or %NULL if @error is set. Free
 *     with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_connection_t *
g_dbus_connection_new_finish (xasync_result_t  *res,
                              xerror_t       **error)
{
  xobject_t *object;
  xobject_t *source_object;

  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  source_object = xasync_result_get_source_object (res);
  g_assert (source_object != NULL);
  object = xasync_initable_new_finish (XASYNC_INITABLE (source_object),
                                        res,
                                        error);
  xobject_unref (source_object);
  if (object != NULL)
    return G_DBUS_CONNECTION (object);
  else
    return NULL;
}

/**
 * g_dbus_connection_new_sync:
 * @stream: a #xio_stream_t
 * @guid: (nullable): the GUID to use if authenticating as a server or %NULL
 * @flags: flags describing how to make the connection
 * @observer: (nullable): a #xdbus_auth_observer_t or %NULL
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @error: return location for error or %NULL
 *
 * Synchronously sets up a D-Bus connection for exchanging D-Bus messages
 * with the end represented by @stream.
 *
 * If @stream is a #xsocket_connection_t, then the corresponding #xsocket_t
 * will be put into non-blocking mode.
 *
 * The D-Bus connection will interact with @stream from a worker thread.
 * As a result, the caller should not interact with @stream after this
 * method has been called, except by calling xobject_unref() on it.
 *
 * If @observer is not %NULL it may be used to control the
 * authentication process.
 *
 * This is a synchronous failable constructor. See
 * g_dbus_connection_new() for the asynchronous version.
 *
 * Returns: (transfer full): a #xdbus_connection_t or %NULL if @error is set.
 *     Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_connection_t *
g_dbus_connection_new_sync (xio_stream_t             *stream,
                            const xchar_t           *guid,
                            GDBusConnectionFlags   flags,
                            xdbus_auth_observer_t     *observer,
                            xcancellable_t          *cancellable,
                            xerror_t               **error)
{
  _g_dbus_initialize ();
  g_return_val_if_fail (X_IS_IO_STREAM (stream), NULL);
  g_return_val_if_fail ((flags & ~G_DBUS_CONNECTION_FLAGS_ALL) == 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  return xinitable_new (XTYPE_DBUS_CONNECTION,
                         cancellable,
                         error,
                         "stream", stream,
                         "guid", guid,
                         "flags", flags,
                         "authentication-observer", observer,
                         NULL);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_connection_new_for_address:
 * @address: a D-Bus address
 * @flags: flags describing how to make the connection
 * @observer: (nullable): a #xdbus_auth_observer_t or %NULL
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @callback: a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: the data to pass to @callback
 *
 * Asynchronously connects and sets up a D-Bus client connection for
 * exchanging D-Bus messages with an endpoint specified by @address
 * which must be in the
 * [D-Bus address format](https://dbus.freedesktop.org/doc/dbus-specification.html#addresses).
 *
 * This constructor can only be used to initiate client-side
 * connections - use g_dbus_connection_new() if you need to act as the
 * server. In particular, @flags cannot contain the
 * %G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER,
 * %G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS or
 * %G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER flags.
 *
 * When the operation is finished, @callback will be invoked. You can
 * then call g_dbus_connection_new_for_address_finish() to get the result of
 * the operation.
 *
 * If @observer is not %NULL it may be used to control the
 * authentication process.
 *
 * This is an asynchronous failable constructor. See
 * g_dbus_connection_new_for_address_sync() for the synchronous
 * version.
 *
 * Since: 2.26
 */
void
g_dbus_connection_new_for_address (const xchar_t          *address,
                                   GDBusConnectionFlags  flags,
                                   xdbus_auth_observer_t    *observer,
                                   xcancellable_t         *cancellable,
                                   xasync_ready_callback_t   callback,
                                   xpointer_t              user_data)
{
  _g_dbus_initialize ();

  g_return_if_fail (address != NULL);
  g_return_if_fail ((flags & ~G_DBUS_CONNECTION_FLAGS_ALL) == 0);

  xasync_initable_new_async (XTYPE_DBUS_CONNECTION,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "address", address,
                              "flags", flags,
                              "authentication-observer", observer,
                              NULL);
}

/**
 * g_dbus_connection_new_for_address_finish:
 * @res: a #xasync_result_t obtained from the #xasync_ready_callback_t passed
 *     to g_dbus_connection_new()
 * @error: return location for error or %NULL
 *
 * Finishes an operation started with g_dbus_connection_new_for_address().
 *
 * Returns: (transfer full): a #xdbus_connection_t or %NULL if @error is set.
 *     Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_connection_t *
g_dbus_connection_new_for_address_finish (xasync_result_t  *res,
                                          xerror_t       **error)
{
  xobject_t *object;
  xobject_t *source_object;

  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  source_object = xasync_result_get_source_object (res);
  g_assert (source_object != NULL);
  object = xasync_initable_new_finish (XASYNC_INITABLE (source_object),
                                        res,
                                        error);
  xobject_unref (source_object);
  if (object != NULL)
    return G_DBUS_CONNECTION (object);
  else
    return NULL;
}

/**
 * g_dbus_connection_new_for_address_sync:
 * @address: a D-Bus address
 * @flags: flags describing how to make the connection
 * @observer: (nullable): a #xdbus_auth_observer_t or %NULL
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @error: return location for error or %NULL
 *
 * Synchronously connects and sets up a D-Bus client connection for
 * exchanging D-Bus messages with an endpoint specified by @address
 * which must be in the
 * [D-Bus address format](https://dbus.freedesktop.org/doc/dbus-specification.html#addresses).
 *
 * This constructor can only be used to initiate client-side
 * connections - use g_dbus_connection_new_sync() if you need to act
 * as the server. In particular, @flags cannot contain the
 * %G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER,
 * %G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS or
 * %G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER flags.
 *
 * This is a synchronous failable constructor. See
 * g_dbus_connection_new_for_address() for the asynchronous version.
 *
 * If @observer is not %NULL it may be used to control the
 * authentication process.
 *
 * Returns: (transfer full): a #xdbus_connection_t or %NULL if @error is set.
 *     Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_connection_t *
g_dbus_connection_new_for_address_sync (const xchar_t           *address,
                                        GDBusConnectionFlags   flags,
                                        xdbus_auth_observer_t     *observer,
                                        xcancellable_t          *cancellable,
                                        xerror_t               **error)
{
  _g_dbus_initialize ();

  g_return_val_if_fail (address != NULL, NULL);
  g_return_val_if_fail ((flags & ~G_DBUS_CONNECTION_FLAGS_ALL) == 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  return xinitable_new (XTYPE_DBUS_CONNECTION,
                         cancellable,
                         error,
                         "address", address,
                         "flags", flags,
                         "authentication-observer", observer,
                         NULL);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_connection_set_exit_on_close:
 * @connection: a #xdbus_connection_t
 * @exit_on_close: whether the process should be terminated
 *     when @connection is closed by the remote peer
 *
 * Sets whether the process should be terminated when @connection is
 * closed by the remote peer. See #xdbus_connection_t:exit-on-close for
 * more details.
 *
 * Note that this function should be used with care. Most modern UNIX
 * desktops tie the notion of a user session with the session bus, and expect
 * all of a user's applications to quit when their bus connection goes away.
 * If you are setting @exit_on_close to %FALSE for the shared session
 * bus connection, you should make sure that your application exits
 * when the user session ends.
 *
 * Since: 2.26
 */
void
g_dbus_connection_set_exit_on_close (xdbus_connection_t *connection,
                                     xboolean_t         exit_on_close)
{
  g_return_if_fail (X_IS_DBUS_CONNECTION (connection));

  if (exit_on_close)
    g_atomic_int_or (&connection->atomic_flags, FLAG_EXIT_ON_CLOSE);
  else
    g_atomic_int_and (&connection->atomic_flags, ~FLAG_EXIT_ON_CLOSE);

}

/**
 * g_dbus_connection_get_exit_on_close:
 * @connection: a #xdbus_connection_t
 *
 * Gets whether the process is terminated when @connection is
 * closed by the remote peer. See
 * #xdbus_connection_t:exit-on-close for more details.
 *
 * Returns: whether the process is terminated when @connection is
 *     closed by the remote peer
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_connection_get_exit_on_close (xdbus_connection_t *connection)
{
  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);

  if (g_atomic_int_get (&connection->atomic_flags) & FLAG_EXIT_ON_CLOSE)
    return TRUE;
  else
    return FALSE;
}

/**
 * g_dbus_connection_get_guid:
 * @connection: a #xdbus_connection_t
 *
 * The GUID of the peer performing the role of server when
 * authenticating. See #xdbus_connection_t:guid for more details.
 *
 * Returns: (not nullable): The GUID. Do not free this string, it is owned by
 *     @connection.
 *
 * Since: 2.26
 */
const xchar_t *
g_dbus_connection_get_guid (xdbus_connection_t *connection)
{
  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);
  return connection->guid;
}

/**
 * g_dbus_connection_get_unique_name:
 * @connection: a #xdbus_connection_t
 *
 * Gets the unique name of @connection as assigned by the message
 * bus. This can also be used to figure out if @connection is a
 * message bus connection.
 *
 * Returns: (nullable): the unique name or %NULL if @connection is not a message
 *     bus connection. Do not free this string, it is owned by
 *     @connection.
 *
 * Since: 2.26
 */
const xchar_t *
g_dbus_connection_get_unique_name (xdbus_connection_t *connection)
{
  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);

  /* do not use g_return_val_if_fail(), we want the memory barrier */
  if (!check_initialized (connection))
    return NULL;

  return connection->bus_unique_name;
}

/**
 * g_dbus_connection_get_peer_credentials:
 * @connection: a #xdbus_connection_t
 *
 * Gets the credentials of the authenticated peer. This will always
 * return %NULL unless @connection acted as a server
 * (e.g. %G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER was passed)
 * when set up and the client passed credentials as part of the
 * authentication process.
 *
 * In a message bus setup, the message bus is always the server and
 * each application is a client. So this method will always return
 * %NULL for message bus clients.
 *
 * Returns: (transfer none) (nullable): a #xcredentials_t or %NULL if not
 *     available. Do not free this object, it is owned by @connection.
 *
 * Since: 2.26
 */
xcredentials_t *
g_dbus_connection_get_peer_credentials (xdbus_connection_t *connection)
{
  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);

  /* do not use g_return_val_if_fail(), we want the memory barrier */
  if (!check_initialized (connection))
    return NULL;

  return connection->credentials;
}

/* ---------------------------------------------------------------------------------------------------- */

static xuint_t _global_filter_id = 1;  /* (atomic) */

/**
 * g_dbus_connection_add_filter:
 * @connection: a #xdbus_connection_t
 * @filter_function: a filter function
 * @user_data: user data to pass to @filter_function
 * @user_data_free_func: function to free @user_data with when filter
 *     is removed or %NULL
 *
 * Adds a message filter. Filters are handlers that are run on all
 * incoming and outgoing messages, prior to standard dispatch. Filters
 * are run in the order that they were added.  The same handler can be
 * added as a filter more than once, in which case it will be run more
 * than once.  Filters added during a filter callback won't be run on
 * the message being processed. Filter functions are allowed to modify
 * and even drop messages.
 *
 * Note that filters are run in a dedicated message handling thread so
 * they can't block and, generally, can't do anything but signal a
 * worker thread. Also note that filters are rarely needed - use API
 * such as g_dbus_connection_send_message_with_reply(),
 * g_dbus_connection_signal_subscribe() or g_dbus_connection_call() instead.
 *
 * If a filter consumes an incoming message the message is not
 * dispatched anywhere else - not even the standard dispatch machinery
 * (that API such as g_dbus_connection_signal_subscribe() and
 * g_dbus_connection_send_message_with_reply() relies on) will see the
 * message. Similarly, if a filter consumes an outgoing message, the
 * message will not be sent to the other peer.
 *
 * If @user_data_free_func is non-%NULL, it will be called (in the
 * thread-default main context of the thread you are calling this
 * method from) at some point after @user_data is no longer
 * needed. (It is not guaranteed to be called synchronously when the
 * filter is removed, and may be called after @connection has been
 * destroyed.)
 *
 * Returns: a filter identifier that can be used with
 *     g_dbus_connection_remove_filter()
 *
 * Since: 2.26
 */
xuint_t
g_dbus_connection_add_filter (xdbus_connection_t            *connection,
                              GDBusMessageFilterFunction  filter_function,
                              xpointer_t                    user_data,
                              xdestroy_notify_t              user_data_free_func)
{
  FilterData *data;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), 0);
  g_return_val_if_fail (filter_function != NULL, 0);
  g_return_val_if_fail (check_initialized (connection), 0);

  CONNECTION_LOCK (connection);
  data = g_new0 (FilterData, 1);
  data->id = (xuint_t) g_atomic_int_add (&_global_filter_id, 1); /* TODO: overflow etc. */
  data->ref_count = 1;
  data->filter_function = filter_function;
  data->user_data = user_data;
  data->user_data_free_func = user_data_free_func;
  data->context = xmain_context_ref_thread_default ();
  xptr_array_add (connection->filters, data);
  CONNECTION_UNLOCK (connection);

  return data->id;
}

/* only called from finalize(), removes all filters */
static void
purge_all_filters (xdbus_connection_t *connection)
{
  xuint_t n;

  for (n = 0; n < connection->filters->len; n++)
    filter_data_destroy (connection->filters->pdata[n], FALSE);
}

/**
 * g_dbus_connection_remove_filter:
 * @connection: a #xdbus_connection_t
 * @filter_id: an identifier obtained from g_dbus_connection_add_filter()
 *
 * Removes a filter.
 *
 * Note that since filters run in a different thread, there is a race
 * condition where it is possible that the filter will be running even
 * after calling g_dbus_connection_remove_filter(), so you cannot just
 * free data that the filter might be using. Instead, you should pass
 * a #xdestroy_notify_t to g_dbus_connection_add_filter(), which will be
 * called when it is guaranteed that the data is no longer needed.
 *
 * Since: 2.26
 */
void
g_dbus_connection_remove_filter (xdbus_connection_t *connection,
                                 xuint_t            filter_id)
{
  xuint_t n;
  xboolean_t found;
  FilterData *to_destroy;

  g_return_if_fail (X_IS_DBUS_CONNECTION (connection));
  g_return_if_fail (check_initialized (connection));

  CONNECTION_LOCK (connection);
  found = FALSE;
  to_destroy = NULL;
  for (n = 0; n < connection->filters->len; n++)
    {
      FilterData *data = connection->filters->pdata[n];
      if (data->id == filter_id)
        {
          found = TRUE;
          xptr_array_remove_index (connection->filters, n);
          data->ref_count--;
          if (data->ref_count == 0)
            to_destroy = data;
          break;
        }
    }
  CONNECTION_UNLOCK (connection);

  /* do free without holding lock */
  if (to_destroy != NULL)
    filter_data_destroy (to_destroy, TRUE);
  else if (!found)
    {
      g_warning ("g_dbus_connection_remove_filter: No filter found for filter_id %d", filter_id);
    }
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xchar_t *rule;
  xchar_t *sender;
  xchar_t *sender_unique_name; /* if sender is unique or org.freedesktop.DBus, then that name... otherwise blank */
  xchar_t *interface_name;
  xchar_t *member;
  xchar_t *object_path;
  xchar_t *arg0;
  GDBusSignalFlags flags;
  xptr_array_t *subscribers;  /* (owned) (element-type SignalSubscriber) */
} SignalData;

static void
signal_data_free (SignalData *signal_data)
{
  g_free (signal_data->rule);
  g_free (signal_data->sender);
  g_free (signal_data->sender_unique_name);
  g_free (signal_data->interface_name);
  g_free (signal_data->member);
  g_free (signal_data->object_path);
  g_free (signal_data->arg0);
  xptr_array_unref (signal_data->subscribers);
  g_free (signal_data);
}

typedef struct
{
  /* All fields are immutable after construction. */
  gatomicrefcount ref_count;
  GDBusSignalCallback callback;
  xpointer_t user_data;
  xdestroy_notify_t user_data_free_func;
  xuint_t id;
  xmain_context_t *context;
} SignalSubscriber;

static SignalSubscriber *
signal_subscriber_ref (SignalSubscriber *subscriber)
{
  g_atomic_ref_count_inc (&subscriber->ref_count);
  return subscriber;
}

static void
signal_subscriber_unref (SignalSubscriber *subscriber)
{
  if (g_atomic_ref_count_dec (&subscriber->ref_count))
    {
      /* Destroy the user data. It doesn’t matter which thread
       * signal_subscriber_unref() is called in (or whether it’s called with a
       * lock held), as call_destroy_notify() always defers to the next
       * #xmain_context_t iteration. */
      call_destroy_notify (subscriber->context,
                           subscriber->user_data_free_func,
                           subscriber->user_data);

      xmain_context_unref (subscriber->context);
      g_free (subscriber);
    }
}

static xchar_t *
args_to_rule (const xchar_t      *sender,
              const xchar_t      *interface_name,
              const xchar_t      *member,
              const xchar_t      *object_path,
              const xchar_t      *arg0,
              GDBusSignalFlags  flags)
{
  xstring_t *rule;

  rule = xstring_new ("type='signal'");
  if (flags & G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE)
    xstring_prepend_c (rule, '-');
  if (sender != NULL)
    xstring_append_printf (rule, ",sender='%s'", sender);
  if (interface_name != NULL)
    xstring_append_printf (rule, ",interface='%s'", interface_name);
  if (member != NULL)
    xstring_append_printf (rule, ",member='%s'", member);
  if (object_path != NULL)
    xstring_append_printf (rule, ",path='%s'", object_path);

  if (arg0 != NULL)
    {
      if (flags & G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH)
        xstring_append_printf (rule, ",arg0path='%s'", arg0);
      else if (flags & G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE)
        xstring_append_printf (rule, ",arg0namespace='%s'", arg0);
      else
        xstring_append_printf (rule, ",arg0='%s'", arg0);
    }

  return xstring_free (rule, FALSE);
}

static xuint_t _global_subscriber_id = 1;  /* (atomic) */
static xuint_t _global_registration_id = 1;  /* (atomic) */
static xuint_t _global_subtree_registration_id = 1;  /* (atomic) */

/* ---------------------------------------------------------------------------------------------------- */

/* Called in a user thread, lock is held */
static void
add_match_rule (xdbus_connection_t *connection,
                const xchar_t     *match_rule)
{
  xerror_t *error;
  xdbus_message_t *message;

  if (match_rule[0] == '-')
    return;

  message = xdbus_message_new_method_call ("org.freedesktop.DBus", /* name */
                                            "/org/freedesktop/DBus", /* path */
                                            "org.freedesktop.DBus", /* interface */
                                            "AddMatch");
  xdbus_message_set_body (message, xvariant_new ("(s)", match_rule));
  error = NULL;
  if (!g_dbus_connection_send_message_unlocked (connection,
                                                message,
                                                G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                                NULL,
                                                &error))
    {
      g_critical ("Error while sending AddMatch() message: %s", error->message);
      xerror_free (error);
    }
  xobject_unref (message);
}

/* ---------------------------------------------------------------------------------------------------- */

/* Called in a user thread, lock is held */
static void
remove_match_rule (xdbus_connection_t *connection,
                   const xchar_t     *match_rule)
{
  xerror_t *error;
  xdbus_message_t *message;

  if (match_rule[0] == '-')
    return;

  message = xdbus_message_new_method_call ("org.freedesktop.DBus", /* name */
                                            "/org/freedesktop/DBus", /* path */
                                            "org.freedesktop.DBus", /* interface */
                                            "RemoveMatch");
  xdbus_message_set_body (message, xvariant_new ("(s)", match_rule));

  error = NULL;
  if (!g_dbus_connection_send_message_unlocked (connection,
                                                message,
                                                G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                                NULL,
                                                &error))
    {
      /* If we could get G_IO_ERROR_CLOSED here, it wouldn't be reasonable to
       * critical; but we're holding the lock, and our caller checked whether
       * we were already closed, so we can't get that error.
       */
      g_critical ("Error while sending RemoveMatch() message: %s", error->message);
      xerror_free (error);
    }
  xobject_unref (message);
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
is_signal_data_for_name_lost_or_acquired (SignalData *signal_data)
{
  return xstrcmp0 (signal_data->sender_unique_name, "org.freedesktop.DBus") == 0 &&
         xstrcmp0 (signal_data->interface_name, "org.freedesktop.DBus") == 0 &&
         xstrcmp0 (signal_data->object_path, "/org/freedesktop/DBus") == 0 &&
         (xstrcmp0 (signal_data->member, "NameLost") == 0 ||
          xstrcmp0 (signal_data->member, "NameAcquired") == 0);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_connection_signal_subscribe:
 * @connection: a #xdbus_connection_t
 * @sender: (nullable): sender name to match on (unique or well-known name)
 *     or %NULL to listen from all senders
 * @interface_name: (nullable): D-Bus interface name to match on or %NULL to
 *     match on all interfaces
 * @member: (nullable): D-Bus signal name to match on or %NULL to match on
 *     all signals
 * @object_path: (nullable): object path to match on or %NULL to match on
 *     all object paths
 * @arg0: (nullable): contents of first string argument to match on or %NULL
 *     to match on all kinds of arguments
 * @flags: #GDBusSignalFlags describing how arg0 is used in subscribing to the
 *     signal
 * @callback: callback to invoke when there is a signal matching the requested data
 * @user_data: user data to pass to @callback
 * @user_data_free_func: (nullable): function to free @user_data with when
 *     subscription is removed or %NULL
 *
 * Subscribes to signals on @connection and invokes @callback with a whenever
 * the signal is received. Note that @callback will be invoked in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread you are calling this method from.
 *
 * If @connection is not a message bus connection, @sender must be
 * %NULL.
 *
 * If @sender is a well-known name note that @callback is invoked with
 * the unique name for the owner of @sender, not the well-known name
 * as one would expect. This is because the message bus rewrites the
 * name. As such, to avoid certain race conditions, users should be
 * tracking the name owner of the well-known name and use that when
 * processing the received signal.
 *
 * If one of %G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE or
 * %G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH are given, @arg0 is
 * interpreted as part of a namespace or path.  The first argument
 * of a signal is matched against that part as specified by D-Bus.
 *
 * If @user_data_free_func is non-%NULL, it will be called (in the
 * thread-default main context of the thread you are calling this
 * method from) at some point after @user_data is no longer
 * needed. (It is not guaranteed to be called synchronously when the
 * signal is unsubscribed from, and may be called after @connection
 * has been destroyed.)
 *
 * As @callback is potentially invoked in a different thread from where it’s
 * emitted, it’s possible for this to happen after
 * g_dbus_connection_signal_unsubscribe() has been called in another thread.
 * Due to this, @user_data should have a strong reference which is freed with
 * @user_data_free_func, rather than pointing to data whose lifecycle is tied
 * to the signal subscription. For example, if a #xobject_t is used to store the
 * subscription ID from g_dbus_connection_signal_subscribe(), a strong reference
 * to that #xobject_t must be passed to @user_data, and xobject_unref() passed to
 * @user_data_free_func. You are responsible for breaking the resulting
 * reference count cycle by explicitly unsubscribing from the signal when
 * dropping the last external reference to the #xobject_t. Alternatively, a weak
 * reference may be used.
 *
 * It is guaranteed that if you unsubscribe from a signal using
 * g_dbus_connection_signal_unsubscribe() from the same thread which made the
 * corresponding g_dbus_connection_signal_subscribe() call, @callback will not
 * be invoked after g_dbus_connection_signal_unsubscribe() returns.
 *
 * The returned subscription identifier is an opaque value which is guaranteed
 * to never be zero.
 *
 * This function can never fail.
 *
 * Returns: a subscription identifier that can be used with g_dbus_connection_signal_unsubscribe()
 *
 * Since: 2.26
 */
xuint_t
g_dbus_connection_signal_subscribe (xdbus_connection_t     *connection,
                                    const xchar_t         *sender,
                                    const xchar_t         *interface_name,
                                    const xchar_t         *member,
                                    const xchar_t         *object_path,
                                    const xchar_t         *arg0,
                                    GDBusSignalFlags     flags,
                                    GDBusSignalCallback  callback,
                                    xpointer_t             user_data,
                                    xdestroy_notify_t       user_data_free_func)
{
  xchar_t *rule;
  SignalData *signal_data;
  SignalSubscriber *subscriber;
  xptr_array_t *signal_data_array;
  const xchar_t *sender_unique_name;

  /* Right now we abort if AddMatch() fails since it can only fail with the bus being in
   * an OOM condition. We might want to change that but that would involve making
   * g_dbus_connection_signal_subscribe() asynchronous and having the call sites
   * handle that. And there's really no sensible way of handling this short of retrying
   * to add the match rule... and then there's the little thing that, hey, maybe there's
   * a reason the bus in an OOM condition.
   *
   * Doable, but not really sure it's worth it...
   */

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), 0);
  g_return_val_if_fail (sender == NULL || (g_dbus_is_name (sender) && (connection->flags & G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION)), 0);
  g_return_val_if_fail (interface_name == NULL || g_dbus_is_interface_name (interface_name), 0);
  g_return_val_if_fail (member == NULL || g_dbus_is_member_name (member), 0);
  g_return_val_if_fail (object_path == NULL || xvariant_is_object_path (object_path), 0);
  g_return_val_if_fail (callback != NULL, 0);
  g_return_val_if_fail (check_initialized (connection), 0);
  g_return_val_if_fail (!((flags & G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH) && (flags & G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE)), 0);
  g_return_val_if_fail (!(arg0 == NULL && (flags & (G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH | G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE))), 0);

  CONNECTION_LOCK (connection);

  /* If G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE was specified, we will end up
   * with a '-' character to prefix the rule (which will otherwise be
   * normal).
   *
   * This allows us to hash the rule and do our lifecycle tracking in
   * the usual way, but the '-' prevents the match rule from ever
   * actually being send to the bus (either for add or remove).
   */
  rule = args_to_rule (sender, interface_name, member, object_path, arg0, flags);

  if (sender != NULL && (g_dbus_is_unique_name (sender) || xstrcmp0 (sender, "org.freedesktop.DBus") == 0))
    sender_unique_name = sender;
  else
    sender_unique_name = "";

  subscriber = g_new0 (SignalSubscriber, 1);
  subscriber->ref_count = 1;
  subscriber->callback = callback;
  subscriber->user_data = user_data;
  subscriber->user_data_free_func = user_data_free_func;
  subscriber->id = (xuint_t) g_atomic_int_add (&_global_subscriber_id, 1); /* TODO: overflow etc. */
  subscriber->context = xmain_context_ref_thread_default ();

  /* see if we've already have this rule */
  signal_data = xhash_table_lookup (connection->map_rule_to_signal_data, rule);
  if (signal_data != NULL)
    {
      xptr_array_add (signal_data->subscribers, subscriber);
      g_free (rule);
      goto out;
    }

  signal_data = g_new0 (SignalData, 1);
  signal_data->rule                  = rule;
  signal_data->sender                = xstrdup (sender);
  signal_data->sender_unique_name    = xstrdup (sender_unique_name);
  signal_data->interface_name        = xstrdup (interface_name);
  signal_data->member                = xstrdup (member);
  signal_data->object_path           = xstrdup (object_path);
  signal_data->arg0                  = xstrdup (arg0);
  signal_data->flags                 = flags;
  signal_data->subscribers           = xptr_array_new_with_free_func ((xdestroy_notify_t) signal_subscriber_unref);
  xptr_array_add (signal_data->subscribers, subscriber);

  xhash_table_insert (connection->map_rule_to_signal_data,
                       signal_data->rule,
                       signal_data);

  /* Add the match rule to the bus...
   *
   * Avoid adding match rules for NameLost and NameAcquired messages - the bus will
   * always send such messages to us.
   */
  if (connection->flags & G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION)
    {
      if (!is_signal_data_for_name_lost_or_acquired (signal_data))
        add_match_rule (connection, signal_data->rule);
    }

  signal_data_array = xhash_table_lookup (connection->map_sender_unique_name_to_signal_data_array,
                                           signal_data->sender_unique_name);
  if (signal_data_array == NULL)
    {
      signal_data_array = xptr_array_new ();
      xhash_table_insert (connection->map_sender_unique_name_to_signal_data_array,
                           xstrdup (signal_data->sender_unique_name),
                           signal_data_array);
    }
  xptr_array_add (signal_data_array, signal_data);

 out:
  xhash_table_insert (connection->map_id_to_signal_data,
                       GUINT_TO_POINTER (subscriber->id),
                       signal_data);

  CONNECTION_UNLOCK (connection);

  return subscriber->id;
}

/* ---------------------------------------------------------------------------------------------------- */

/* called in any thread */
/* must hold lock when calling this (except if connection->finalizing is TRUE)
 * returns the number of removed subscribers */
static xuint_t
unsubscribe_id_internal (xdbus_connection_t *connection,
                         xuint_t            subscription_id)
{
  SignalData *signal_data;
  xptr_array_t *signal_data_array;
  xuint_t n;
  xuint_t n_removed = 0;

  signal_data = xhash_table_lookup (connection->map_id_to_signal_data,
                                     GUINT_TO_POINTER (subscription_id));
  if (signal_data == NULL)
    {
      /* Don't warn here, we may have thrown all subscriptions out when the connection was closed */
      goto out;
    }

  for (n = 0; n < signal_data->subscribers->len; n++)
    {
      SignalSubscriber *subscriber = signal_data->subscribers->pdata[n];

      if (subscriber->id != subscription_id)
        continue;

      /* It’s OK to rearrange the array order using the ‘fast’ #xptr_array_t
       * removal functions, since we’re going to exit the loop below anyway — we
       * never move on to the next element. Secondly, subscription IDs are
       * guaranteed to be unique. */
      g_warn_if_fail (xhash_table_remove (connection->map_id_to_signal_data,
                                           GUINT_TO_POINTER (subscription_id)));
      n_removed++;
      xptr_array_remove_index_fast (signal_data->subscribers, n);

      if (signal_data->subscribers->len == 0)
        {
          g_warn_if_fail (xhash_table_remove (connection->map_rule_to_signal_data, signal_data->rule));

          signal_data_array = xhash_table_lookup (connection->map_sender_unique_name_to_signal_data_array,
                                                   signal_data->sender_unique_name);
          g_warn_if_fail (signal_data_array != NULL);
          g_warn_if_fail (xptr_array_remove (signal_data_array, signal_data));

          if (signal_data_array->len == 0)
            {
              g_warn_if_fail (xhash_table_remove (connection->map_sender_unique_name_to_signal_data_array,
                                                   signal_data->sender_unique_name));
            }

          /* remove the match rule from the bus unless NameLost or NameAcquired (see subscribe()) */
          if ((connection->flags & G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION) &&
              !is_signal_data_for_name_lost_or_acquired (signal_data) &&
              !g_dbus_connection_is_closed (connection) &&
              !connection->finalizing)
            {
              /* The check for g_dbus_connection_is_closed() means that
               * sending the RemoveMatch message can't fail with
               * G_IO_ERROR_CLOSED, because we're holding the lock,
               * so on_worker_closed() can't happen between the check we just
               * did, and releasing the lock later.
               */
              remove_match_rule (connection, signal_data->rule);
            }

          signal_data_free (signal_data);
        }

      goto out;
    }

  g_assert_not_reached ();

 out:
  return n_removed;
}

/**
 * g_dbus_connection_signal_unsubscribe:
 * @connection: a #xdbus_connection_t
 * @subscription_id: a subscription id obtained from
 *     g_dbus_connection_signal_subscribe()
 *
 * Unsubscribes from signals.
 *
 * Note that there may still be D-Bus traffic to process (relating to this
 * signal subscription) in the current thread-default #xmain_context_t after this
 * function has returned. You should continue to iterate the #xmain_context_t
 * until the #xdestroy_notify_t function passed to
 * g_dbus_connection_signal_subscribe() is called, in order to avoid memory
 * leaks through callbacks queued on the #xmain_context_t after it’s stopped being
 * iterated.
 * Alternatively, any idle source with a priority lower than %G_PRIORITY_DEFAULT
 * that was scheduled after unsubscription, also indicates that all resources
 * of this subscription are released.
 *
 * Since: 2.26
 */
void
g_dbus_connection_signal_unsubscribe (xdbus_connection_t *connection,
                                      xuint_t            subscription_id)
{
  xuint_t n_subscribers_removed G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;

  g_return_if_fail (X_IS_DBUS_CONNECTION (connection));
  g_return_if_fail (check_initialized (connection));

  CONNECTION_LOCK (connection);
  n_subscribers_removed = unsubscribe_id_internal (connection, subscription_id);
  CONNECTION_UNLOCK (connection);

  /* invariant */
  g_assert (n_subscribers_removed == 0 || n_subscribers_removed == 1);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  SignalSubscriber    *subscriber;  /* (owned) */
  xdbus_message_t        *message;
  xdbus_connection_t     *connection;
  const xchar_t         *sender;  /* (nullable) for peer-to-peer connections */
  const xchar_t         *path;
  const xchar_t         *interface;
  const xchar_t         *member;
} SignalInstance;

/* called on delivery thread (e.g. where g_dbus_connection_signal_subscribe() was called) with
 * no locks held
 */
static xboolean_t
emit_signal_instance_in_idle_cb (xpointer_t data)
{
  SignalInstance *signal_instance = data;
  xvariant_t *parameters;
  xboolean_t has_subscription;

  parameters = xdbus_message_get_body (signal_instance->message);
  if (parameters == NULL)
    {
      parameters = xvariant_new ("()");
      xvariant_ref_sink (parameters);
    }
  else
    {
      xvariant_ref_sink (parameters);
    }

#if 0
  g_print ("in emit_signal_instance_in_idle_cb (id=%d sender=%s path=%s interface=%s member=%s params=%s)\n",
           signal_instance->subscriber->id,
           signal_instance->sender,
           signal_instance->path,
           signal_instance->interface,
           signal_instance->member,
           xvariant_print (parameters, TRUE));
#endif

  /* Careful here, don't do the callback if we no longer has the subscription */
  CONNECTION_LOCK (signal_instance->connection);
  has_subscription = FALSE;
  if (xhash_table_lookup (signal_instance->connection->map_id_to_signal_data,
                           GUINT_TO_POINTER (signal_instance->subscriber->id)) != NULL)
    has_subscription = TRUE;
  CONNECTION_UNLOCK (signal_instance->connection);

  if (has_subscription)
    signal_instance->subscriber->callback (signal_instance->connection,
                                           signal_instance->sender,
                                           signal_instance->path,
                                           signal_instance->interface,
                                           signal_instance->member,
                                           parameters,
                                           signal_instance->subscriber->user_data);

  xvariant_unref (parameters);

  return FALSE;
}

static void
signal_instance_free (SignalInstance *signal_instance)
{
  xobject_unref (signal_instance->message);
  xobject_unref (signal_instance->connection);
  signal_subscriber_unref (signal_instance->subscriber);
  g_free (signal_instance);
}

static xboolean_t
namespace_rule_matches (const xchar_t *namespace,
                        const xchar_t *name)
{
  xint_t len_namespace;
  xint_t len_name;

  len_namespace = strlen (namespace);
  len_name = strlen (name);

  if (len_name < len_namespace)
    return FALSE;

  if (memcmp (namespace, name, len_namespace) != 0)
    return FALSE;

  return len_namespace == len_name || name[len_namespace] == '.';
}

static xboolean_t
path_rule_matches (const xchar_t *path_a,
                   const xchar_t *path_b)
{
  xint_t len_a, len_b;

  len_a = strlen (path_a);
  len_b = strlen (path_b);

  if (len_a < len_b && (len_a == 0 || path_a[len_a - 1] != '/'))
    return FALSE;

  if (len_b < len_a && (len_b == 0 || path_b[len_b - 1] != '/'))
    return FALSE;

  return memcmp (path_a, path_b, MIN (len_a, len_b)) == 0;
}

/* called in GDBusWorker thread WITH lock held
 *
 * @sender is (nullable) for peer-to-peer connections */
static void
schedule_callbacks (xdbus_connection_t *connection,
                    xptr_array_t       *signal_data_array,
                    xdbus_message_t    *message,
                    const xchar_t     *sender)
{
  xuint_t n, m;
  const xchar_t *interface;
  const xchar_t *member;
  const xchar_t *path;
  const xchar_t *arg0;

  interface = NULL;
  member = NULL;
  path = NULL;
  arg0 = NULL;

  interface = xdbus_message_get_interface (message);
  member = xdbus_message_get_member (message);
  path = xdbus_message_get_path (message);
  arg0 = xdbus_message_get_arg0 (message);

#if 0
  g_print ("In schedule_callbacks:\n"
           "  sender    = '%s'\n"
           "  interface = '%s'\n"
           "  member    = '%s'\n"
           "  path      = '%s'\n"
           "  arg0      = '%s'\n",
           sender,
           interface,
           member,
           path,
           arg0);
#endif

  /* TODO: if this is slow, then we can change signal_data_array into
   *       map_object_path_to_signal_data_array or something.
   */
  for (n = 0; n < signal_data_array->len; n++)
    {
      SignalData *signal_data = signal_data_array->pdata[n];

      if (signal_data->interface_name != NULL && xstrcmp0 (signal_data->interface_name, interface) != 0)
        continue;

      if (signal_data->member != NULL && xstrcmp0 (signal_data->member, member) != 0)
        continue;

      if (signal_data->object_path != NULL && xstrcmp0 (signal_data->object_path, path) != 0)
        continue;

      if (signal_data->arg0 != NULL)
        {
          if (arg0 == NULL)
            continue;

          if (signal_data->flags & G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE)
            {
              if (!namespace_rule_matches (signal_data->arg0, arg0))
                continue;
            }
          else if (signal_data->flags & G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH)
            {
              if (!path_rule_matches (signal_data->arg0, arg0))
                continue;
            }
          else if (!xstr_equal (signal_data->arg0, arg0))
            continue;
        }

      for (m = 0; m < signal_data->subscribers->len; m++)
        {
          SignalSubscriber *subscriber = signal_data->subscribers->pdata[m];
          xsource_t *idle_source;
          SignalInstance *signal_instance;

          signal_instance = g_new0 (SignalInstance, 1);
          signal_instance->subscriber = signal_subscriber_ref (subscriber);
          signal_instance->message = xobject_ref (message);
          signal_instance->connection = xobject_ref (connection);
          signal_instance->sender = sender;
          signal_instance->path = path;
          signal_instance->interface = interface;
          signal_instance->member = member;

          idle_source = g_idle_source_new ();
          xsource_set_priority (idle_source, G_PRIORITY_DEFAULT);
          xsource_set_callback (idle_source,
                                 emit_signal_instance_in_idle_cb,
                                 signal_instance,
                                 (xdestroy_notify_t) signal_instance_free);
          xsource_set_static_name (idle_source, "[gio] emit_signal_instance_in_idle_cb");
          xsource_attach (idle_source, subscriber->context);
          xsource_unref (idle_source);
        }
    }
}

/* called in GDBusWorker thread with lock held */
static void
distribute_signals (xdbus_connection_t *connection,
                    xdbus_message_t    *message)
{
  xptr_array_t *signal_data_array;
  const xchar_t *sender;

  sender = xdbus_message_get_sender (message);

  if (G_UNLIKELY (_g_dbus_debug_signal ()))
    {
      _g_dbus_debug_print_lock ();
      g_print ("========================================================================\n"
               "GDBus-debug:Signal:\n"
               " <<<< RECEIVED SIGNAL %s.%s\n"
               "      on object %s\n"
               "      sent by name %s\n",
               xdbus_message_get_interface (message),
               xdbus_message_get_member (message),
               xdbus_message_get_path (message),
               sender != NULL ? sender : "(none)");
      _g_dbus_debug_print_unlock ();
    }

  /* collect subscribers that match on sender */
  if (sender != NULL)
    {
      signal_data_array = xhash_table_lookup (connection->map_sender_unique_name_to_signal_data_array, sender);
      if (signal_data_array != NULL)
        schedule_callbacks (connection, signal_data_array, message, sender);
    }

  /* collect subscribers not matching on sender */
  signal_data_array = xhash_table_lookup (connection->map_sender_unique_name_to_signal_data_array, "");
  if (signal_data_array != NULL)
    schedule_callbacks (connection, signal_data_array, message, sender);
}

/* ---------------------------------------------------------------------------------------------------- */

/* only called from finalize(), removes all subscriptions */
static void
purge_all_signal_subscriptions (xdbus_connection_t *connection)
{
  xhash_table_iter_t iter;
  xpointer_t key;
  xarray_t *ids;
  xuint_t n;

  ids = g_array_new (FALSE, FALSE, sizeof (xuint_t));
  xhash_table_iter_init (&iter, connection->map_id_to_signal_data);
  while (xhash_table_iter_next (&iter, &key, NULL))
    {
      xuint_t subscription_id = GPOINTER_TO_UINT (key);
      g_array_append_val (ids, subscription_id);
    }

  for (n = 0; n < ids->len; n++)
    {
      xuint_t subscription_id = g_array_index (ids, xuint_t, n);
      unsubscribe_id_internal (connection, subscription_id);
    }
  g_array_free (ids, TRUE);
}

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_interface_vtable_t *
_g_dbus_interface_vtable_copy (const xdbus_interface_vtable_t *vtable)
{
  /* Don't waste memory by copying padding - remember to update this
   * when changing struct _GDBusInterfaceVTable in gdbusconnection.h
   */
  return g_memdup2 ((xconstpointer) vtable, 3 * sizeof (xpointer_t));
}

static void
_g_dbus_interface_vtable_free (xdbus_interface_vtable_t *vtable)
{
  g_free (vtable);
}

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_subtree_vtable_t *
_g_dbus_subtree_vtable_copy (const xdbus_subtree_vtable_t *vtable)
{
  /* Don't waste memory by copying padding - remember to update this
   * when changing struct _GDBusSubtreeVTable in gdbusconnection.h
   */
  return g_memdup2 ((xconstpointer) vtable, 3 * sizeof (xpointer_t));
}

static void
_g_dbus_subtree_vtable_free (xdbus_subtree_vtable_t *vtable)
{
  g_free (vtable);
}

/* ---------------------------------------------------------------------------------------------------- */

struct ExportedObject
{
  xchar_t *object_path;
  xdbus_connection_t *connection;

  /* maps xchar_t* -> ExportedInterface* */
  xhashtable_t *map_if_name_to_ei;
};

/* only called with lock held */
static void
exported_object_free (ExportedObject *eo)
{
  g_free (eo->object_path);
  xhash_table_unref (eo->map_if_name_to_ei);
  g_free (eo);
}

typedef struct
{
  ExportedObject *eo;

  xint_t                        refcount;  /* (atomic) */

  xuint_t                       id;
  xchar_t                      *interface_name;  /* (owned) */
  xdbus_interface_vtable_t       *vtable;  /* (owned) */
  xdbus_interface_info_t         *interface_info;  /* (owned) */

  xmain_context_t               *context;  /* (owned) */
  xpointer_t                    user_data;
  xdestroy_notify_t              user_data_free_func;
} ExportedInterface;

static ExportedInterface *
exported_interface_ref (ExportedInterface *ei)
{
  g_atomic_int_inc (&ei->refcount);

  return ei;
}

/* May be called with lock held */
static void
exported_interface_unref (ExportedInterface *ei)
{
  if (!g_atomic_int_dec_and_test (&ei->refcount))
    return;

  g_dbus_interface_info_cache_release (ei->interface_info);
  g_dbus_interface_info_unref ((xdbus_interface_info_t *) ei->interface_info);

  /* All uses of ei->vtable from callbacks scheduled in idle functions must
   * have completed by this call_destroy_notify() call, as language bindings
   * may destroy function closures in this callback. */
  call_destroy_notify (ei->context,
                       ei->user_data_free_func,
                       ei->user_data);

  xmain_context_unref (ei->context);

  g_free (ei->interface_name);
  _g_dbus_interface_vtable_free (ei->vtable);
  g_free (ei);
}

struct ExportedSubtree
{
  xint_t                      refcount;  /* (atomic) */

  xuint_t                     id;
  xchar_t                    *object_path;  /* (owned) */
  xdbus_connection_t          *connection;  /* (unowned) */
  xdbus_subtree_vtable_t       *vtable;  /* (owned) */
  GDBusSubtreeFlags         flags;

  xmain_context_t             *context;  /* (owned) */
  xpointer_t                  user_data;
  xdestroy_notify_t            user_data_free_func;
};

static ExportedSubtree *
exported_subtree_ref (ExportedSubtree *es)
{
  g_atomic_int_inc (&es->refcount);

  return es;
}

/* May be called with lock held */
static void
exported_subtree_unref (ExportedSubtree *es)
{
  if (!g_atomic_int_dec_and_test (&es->refcount))
    return;

  /* All uses of es->vtable from callbacks scheduled in idle functions must
   * have completed by this call_destroy_notify() call, as language bindings
   * may destroy function closures in this callback. */
  call_destroy_notify (es->context,
                       es->user_data_free_func,
                       es->user_data);

  xmain_context_unref (es->context);

  _g_dbus_subtree_vtable_free (es->vtable);
  g_free (es->object_path);
  g_free (es);
}

/* ---------------------------------------------------------------------------------------------------- */

/* Convenience function to check if @registration_id (if not zero) or
 * @subtree_registration_id (if not zero) has been unregistered. If
 * so, returns %TRUE.
 *
 * If not, sets @out_ei and/or @out_es to a strong reference to the relevant
 * #ExportedInterface/#ExportedSubtree and returns %FALSE.
 *
 * May be called by any thread. Caller must *not* hold lock.
 */
static xboolean_t
has_object_been_unregistered (xdbus_connection_t    *connection,
                              xuint_t               registration_id,
                              ExportedInterface **out_ei,
                              xuint_t               subtree_registration_id,
                              ExportedSubtree   **out_es)
{
  xboolean_t ret;
  ExportedInterface *ei = NULL;
  xpointer_t es = NULL;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);

  ret = FALSE;

  CONNECTION_LOCK (connection);

  if (registration_id != 0)
    {
      ei = xhash_table_lookup (connection->map_id_to_ei, GUINT_TO_POINTER (registration_id));
      if (ei == NULL)
        ret = TRUE;
      else if (out_ei != NULL)
        *out_ei = exported_interface_ref (ei);
    }
  if (subtree_registration_id != 0)
    {
      es = xhash_table_lookup (connection->map_id_to_es, GUINT_TO_POINTER (subtree_registration_id));
      if (es == NULL)
        ret = TRUE;
      else if (out_es != NULL)
        *out_es = exported_subtree_ref (es);
    }

  CONNECTION_UNLOCK (connection);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xdbus_connection_t *connection;
  xdbus_message_t *message;
  xpointer_t user_data;
  const xchar_t *property_name;
  const xdbus_interface_vtable_t *vtable;
  xdbus_interface_info_t *interface_info;
  const xdbus_property_info_t *property_info;
  xuint_t registration_id;
  xuint_t subtree_registration_id;
} PropertyData;

static void
property_data_free (PropertyData *data)
{
  xobject_unref (data->connection);
  xobject_unref (data->message);
  g_free (data);
}

/* called in thread where object was registered - no locks held */
static xboolean_t
invoke_get_property_in_idle_cb (xpointer_t _data)
{
  PropertyData *data = _data;
  xvariant_t *value;
  xerror_t *error;
  xdbus_message_t *reply;
  ExportedInterface *ei = NULL;
  ExportedSubtree *es = NULL;

  if (has_object_been_unregistered (data->connection,
                                    data->registration_id,
                                    &ei,
                                    data->subtree_registration_id,
                                    &es))
    {
      reply = xdbus_message_new_method_error (data->message,
                                               "org.freedesktop.DBus.Error.UnknownMethod",
                                               _("No such interface “org.freedesktop.DBus.Properties” on object at path %s"),
                                               xdbus_message_get_path (data->message));
      g_dbus_connection_send_message (data->connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      xobject_unref (reply);
      goto out;
    }

  error = NULL;
  value = data->vtable->get_property (data->connection,
                                      xdbus_message_get_sender (data->message),
                                      xdbus_message_get_path (data->message),
                                      data->interface_info->name,
                                      data->property_name,
                                      &error,
                                      data->user_data);


  if (value != NULL)
    {
      g_assert_no_error (error);

      xvariant_take_ref (value);
      reply = xdbus_message_new_method_reply (data->message);
      xdbus_message_set_body (reply, xvariant_new ("(v)", value));
      g_dbus_connection_send_message (data->connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      xvariant_unref (value);
      xobject_unref (reply);
    }
  else
    {
      xchar_t *dbus_error_name;
      g_assert (error != NULL);
      dbus_error_name = g_dbus_error_encode_gerror (error);
      reply = xdbus_message_new_method_error_literal (data->message,
                                                       dbus_error_name,
                                                       error->message);
      g_dbus_connection_send_message (data->connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      g_free (dbus_error_name);
      xerror_free (error);
      xobject_unref (reply);
    }

 out:
  g_clear_pointer (&ei, exported_interface_unref);
  g_clear_pointer (&es, exported_subtree_unref);

  return FALSE;
}

/* called in thread where object was registered - no locks held */
static xboolean_t
invoke_set_property_in_idle_cb (xpointer_t _data)
{
  PropertyData *data = _data;
  xerror_t *error;
  xdbus_message_t *reply;
  xvariant_t *value;

  error = NULL;
  value = NULL;

  xvariant_get (xdbus_message_get_body (data->message),
                 "(ssv)",
                 NULL,
                 NULL,
                 &value);

  if (!data->vtable->set_property (data->connection,
                                   xdbus_message_get_sender (data->message),
                                   xdbus_message_get_path (data->message),
                                   data->interface_info->name,
                                   data->property_name,
                                   value,
                                   &error,
                                   data->user_data))
    {
      xchar_t *dbus_error_name;
      g_assert (error != NULL);
      dbus_error_name = g_dbus_error_encode_gerror (error);
      reply = xdbus_message_new_method_error_literal (data->message,
                                                       dbus_error_name,
                                                       error->message);
      g_free (dbus_error_name);
      xerror_free (error);
    }
  else
    {
      reply = xdbus_message_new_method_reply (data->message);
    }

  g_assert (reply != NULL);
  g_dbus_connection_send_message (data->connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
  xobject_unref (reply);
  xvariant_unref (value);

  return FALSE;
}

/* called in any thread with connection's lock held */
static xboolean_t
validate_and_maybe_schedule_property_getset (xdbus_connection_t            *connection,
                                             xdbus_message_t               *message,
                                             xuint_t                       registration_id,
                                             xuint_t                       subtree_registration_id,
                                             xboolean_t                    is_get,
                                             xdbus_interface_info_t         *interface_info,
                                             const xdbus_interface_vtable_t *vtable,
                                             xmain_context_t               *main_context,
                                             xpointer_t                    user_data)
{
  xboolean_t handled;
  const char *interface_name;
  const char *property_name;
  const xdbus_property_info_t *property_info;
  xsource_t *idle_source;
  PropertyData *property_data;
  xdbus_message_t *reply;

  handled = FALSE;

  if (is_get)
    xvariant_get (xdbus_message_get_body (message),
                   "(&s&s)",
                   &interface_name,
                   &property_name);
  else
    xvariant_get (xdbus_message_get_body (message),
                   "(&s&sv)",
                   &interface_name,
                   &property_name,
                   NULL);

  if (vtable == NULL)
    goto out;

  /* Check that the property exists - if not fail with org.freedesktop.DBus.Error.InvalidArgs
   */
  property_info = NULL;

  /* TODO: the cost of this is O(n) - it might be worth caching the result */
  property_info = g_dbus_interface_info_lookup_property (interface_info, property_name);
  if (property_info == NULL)
    {
      reply = xdbus_message_new_method_error (message,
                                               "org.freedesktop.DBus.Error.InvalidArgs",
                                               _("No such property “%s”"),
                                               property_name);
      g_dbus_connection_send_message_unlocked (connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      xobject_unref (reply);
      handled = TRUE;
      goto out;
    }

  if (is_get && !(property_info->flags & G_DBUS_PROPERTY_INFO_FLAGS_READABLE))
    {
      reply = xdbus_message_new_method_error (message,
                                               "org.freedesktop.DBus.Error.InvalidArgs",
                                               _("Property “%s” is not readable"),
                                               property_name);
      g_dbus_connection_send_message_unlocked (connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      xobject_unref (reply);
      handled = TRUE;
      goto out;
    }
  else if (!is_get && !(property_info->flags & G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE))
    {
      reply = xdbus_message_new_method_error (message,
                                               "org.freedesktop.DBus.Error.InvalidArgs",
                                               _("Property “%s” is not writable"),
                                               property_name);
      g_dbus_connection_send_message_unlocked (connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      xobject_unref (reply);
      handled = TRUE;
      goto out;
    }

  if (!is_get)
    {
      xvariant_t *value;

      /* Fail with org.freedesktop.DBus.Error.InvalidArgs if the type
       * of the given value is wrong
       */
      xvariant_get_child (xdbus_message_get_body (message), 2, "v", &value);
      if (xstrcmp0 (xvariant_get_type_string (value), property_info->signature) != 0)
        {
          reply = xdbus_message_new_method_error (message,
                                                   "org.freedesktop.DBus.Error.InvalidArgs",
                                                   _("Error setting property “%s”: Expected type “%s” but got “%s”"),
                                                   property_name, property_info->signature,
                                                   xvariant_get_type_string (value));
          g_dbus_connection_send_message_unlocked (connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
          xvariant_unref (value);
          xobject_unref (reply);
          handled = TRUE;
          goto out;
        }

      xvariant_unref (value);
    }

  /* If the vtable pointer for get_property() resp. set_property() is
   * NULL then dispatch the call via the method_call() handler.
   */
  if (is_get)
    {
      if (vtable->get_property == NULL)
        {
          schedule_method_call (connection, message, registration_id, subtree_registration_id,
                                interface_info, NULL, property_info, xdbus_message_get_body (message),
                                vtable, main_context, user_data);
          handled = TRUE;
          goto out;
        }
    }
  else
    {
      if (vtable->set_property == NULL)
        {
          schedule_method_call (connection, message, registration_id, subtree_registration_id,
                                interface_info, NULL, property_info, xdbus_message_get_body (message),
                                vtable, main_context, user_data);
          handled = TRUE;
          goto out;
        }
    }

  /* ok, got the property info - call user code in an idle handler */
  property_data = g_new0 (PropertyData, 1);
  property_data->connection = xobject_ref (connection);
  property_data->message = xobject_ref (message);
  property_data->user_data = user_data;
  property_data->property_name = property_name;
  property_data->vtable = vtable;
  property_data->interface_info = interface_info;
  property_data->property_info = property_info;
  property_data->registration_id = registration_id;
  property_data->subtree_registration_id = subtree_registration_id;

  idle_source = g_idle_source_new ();
  xsource_set_priority (idle_source, G_PRIORITY_DEFAULT);
  xsource_set_callback (idle_source,
                         is_get ? invoke_get_property_in_idle_cb : invoke_set_property_in_idle_cb,
                         property_data,
                         (xdestroy_notify_t) property_data_free);
  if (is_get)
    xsource_set_static_name (idle_source, "[gio] invoke_get_property_in_idle_cb");
  else
    xsource_set_static_name (idle_source, "[gio] invoke_set_property_in_idle_cb");
  xsource_attach (idle_source, main_context);
  xsource_unref (idle_source);

  handled = TRUE;

 out:
  return handled;
}

/* called in GDBusWorker thread with connection's lock held */
static xboolean_t
handle_getset_property (xdbus_connection_t *connection,
                        ExportedObject  *eo,
                        xdbus_message_t    *message,
                        xboolean_t         is_get)
{
  ExportedInterface *ei;
  xboolean_t handled;
  const char *interface_name;
  const char *property_name;

  handled = FALSE;

  if (is_get)
    xvariant_get (xdbus_message_get_body (message),
                   "(&s&s)",
                   &interface_name,
                   &property_name);
  else
    xvariant_get (xdbus_message_get_body (message),
                   "(&s&sv)",
                   &interface_name,
                   &property_name,
                   NULL);

  /* Fail with org.freedesktop.DBus.Error.InvalidArgs if there is
   * no such interface registered
   */
  ei = xhash_table_lookup (eo->map_if_name_to_ei, interface_name);
  if (ei == NULL)
    {
      xdbus_message_t *reply;
      reply = xdbus_message_new_method_error (message,
                                               "org.freedesktop.DBus.Error.InvalidArgs",
                                               _("No such interface “%s”"),
                                               interface_name);
      g_dbus_connection_send_message_unlocked (eo->connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      xobject_unref (reply);
      handled = TRUE;
      goto out;
    }

  handled = validate_and_maybe_schedule_property_getset (eo->connection,
                                                         message,
                                                         ei->id,
                                                         0,
                                                         is_get,
                                                         ei->interface_info,
                                                         ei->vtable,
                                                         ei->context,
                                                         ei->user_data);
 out:
  return handled;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xdbus_connection_t *connection;
  xdbus_message_t *message;
  xpointer_t user_data;
  const xdbus_interface_vtable_t *vtable;
  xdbus_interface_info_t *interface_info;
  xuint_t registration_id;
  xuint_t subtree_registration_id;
} PropertyGetAllData;

static void
property_get_all_data_free (PropertyData *data)
{
  xobject_unref (data->connection);
  xobject_unref (data->message);
  g_free (data);
}

/* called in thread where object was registered - no locks held */
static xboolean_t
invoke_get_all_properties_in_idle_cb (xpointer_t _data)
{
  PropertyGetAllData *data = _data;
  xvariant_builder_t builder;
  xdbus_message_t *reply;
  xuint_t n;
  ExportedInterface *ei = NULL;
  ExportedSubtree *es = NULL;

  if (has_object_been_unregistered (data->connection,
                                    data->registration_id,
                                    &ei,
                                    data->subtree_registration_id,
                                    &es))
    {
      reply = xdbus_message_new_method_error (data->message,
                                               "org.freedesktop.DBus.Error.UnknownMethod",
                                               _("No such interface “org.freedesktop.DBus.Properties” on object at path %s"),
                                               xdbus_message_get_path (data->message));
      g_dbus_connection_send_message (data->connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      xobject_unref (reply);
      goto out;
    }

  /* TODO: Right now we never fail this call - we just omit values if
   *       a get_property() call is failing.
   *
   *       We could fail the whole call if just a single get_property() call
   *       returns an error. We need clarification in the D-Bus spec about this.
   */
  xvariant_builder_init (&builder, G_VARIANT_TYPE ("(a{sv})"));
  xvariant_builder_open (&builder, G_VARIANT_TYPE ("a{sv}"));
  for (n = 0; data->interface_info->properties != NULL && data->interface_info->properties[n] != NULL; n++)
    {
      const xdbus_property_info_t *property_info = data->interface_info->properties[n];
      xvariant_t *value;

      if (!(property_info->flags & G_DBUS_PROPERTY_INFO_FLAGS_READABLE))
        continue;

      value = data->vtable->get_property (data->connection,
                                          xdbus_message_get_sender (data->message),
                                          xdbus_message_get_path (data->message),
                                          data->interface_info->name,
                                          property_info->name,
                                          NULL,
                                          data->user_data);

      if (value == NULL)
        continue;

      xvariant_take_ref (value);
      xvariant_builder_add (&builder,
                             "{sv}",
                             property_info->name,
                             value);
      xvariant_unref (value);
    }
  xvariant_builder_close (&builder);

  reply = xdbus_message_new_method_reply (data->message);
  xdbus_message_set_body (reply, xvariant_builder_end (&builder));
  g_dbus_connection_send_message (data->connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
  xobject_unref (reply);

 out:
  g_clear_pointer (&ei, exported_interface_unref);
  g_clear_pointer (&es, exported_subtree_unref);

  return FALSE;
}

static xboolean_t
interface_has_readable_properties (xdbus_interface_info_t *interface_info)
{
  xint_t i;

  if (!interface_info->properties)
    return FALSE;

  for (i = 0; interface_info->properties[i]; i++)
    if (interface_info->properties[i]->flags & G_DBUS_PROPERTY_INFO_FLAGS_READABLE)
      return TRUE;

  return FALSE;
}

/* called in any thread with connection's lock held */
static xboolean_t
validate_and_maybe_schedule_property_get_all (xdbus_connection_t            *connection,
                                              xdbus_message_t               *message,
                                              xuint_t                       registration_id,
                                              xuint_t                       subtree_registration_id,
                                              xdbus_interface_info_t         *interface_info,
                                              const xdbus_interface_vtable_t *vtable,
                                              xmain_context_t               *main_context,
                                              xpointer_t                    user_data)
{
  xboolean_t handled;
  xsource_t *idle_source;
  PropertyGetAllData *property_get_all_data;

  handled = FALSE;

  if (vtable == NULL)
    goto out;

  /* If the vtable pointer for get_property() is NULL but we have a
   * non-zero number of readable properties, then dispatch the call via
   * the method_call() handler.
   */
  if (vtable->get_property == NULL && interface_has_readable_properties (interface_info))
    {
      schedule_method_call (connection, message, registration_id, subtree_registration_id,
                            interface_info, NULL, NULL, xdbus_message_get_body (message),
                            vtable, main_context, user_data);
      handled = TRUE;
      goto out;
    }

  /* ok, got the property info - call user in an idle handler */
  property_get_all_data = g_new0 (PropertyGetAllData, 1);
  property_get_all_data->connection = xobject_ref (connection);
  property_get_all_data->message = xobject_ref (message);
  property_get_all_data->user_data = user_data;
  property_get_all_data->vtable = vtable;
  property_get_all_data->interface_info = interface_info;
  property_get_all_data->registration_id = registration_id;
  property_get_all_data->subtree_registration_id = subtree_registration_id;

  idle_source = g_idle_source_new ();
  xsource_set_priority (idle_source, G_PRIORITY_DEFAULT);
  xsource_set_callback (idle_source,
                         invoke_get_all_properties_in_idle_cb,
                         property_get_all_data,
                         (xdestroy_notify_t) property_get_all_data_free);
  xsource_set_static_name (idle_source, "[gio] invoke_get_all_properties_in_idle_cb");
  xsource_attach (idle_source, main_context);
  xsource_unref (idle_source);

  handled = TRUE;

 out:
  return handled;
}

/* called in GDBusWorker thread with connection's lock held */
static xboolean_t
handle_get_all_properties (xdbus_connection_t *connection,
                           ExportedObject  *eo,
                           xdbus_message_t    *message)
{
  ExportedInterface *ei;
  xboolean_t handled;
  const char *interface_name;

  handled = FALSE;

  xvariant_get (xdbus_message_get_body (message),
                 "(&s)",
                 &interface_name);

  /* Fail with org.freedesktop.DBus.Error.InvalidArgs if there is
   * no such interface registered
   */
  ei = xhash_table_lookup (eo->map_if_name_to_ei, interface_name);
  if (ei == NULL)
    {
      xdbus_message_t *reply;
      reply = xdbus_message_new_method_error (message,
                                               "org.freedesktop.DBus.Error.InvalidArgs",
                                               _("No such interface “%s”"),
                                               interface_name);
      g_dbus_connection_send_message_unlocked (eo->connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      xobject_unref (reply);
      handled = TRUE;
      goto out;
    }

  handled = validate_and_maybe_schedule_property_get_all (eo->connection,
                                                          message,
                                                          ei->id,
                                                          0,
                                                          ei->interface_info,
                                                          ei->vtable,
                                                          ei->context,
                                                          ei->user_data);
 out:
  return handled;
}

/* ---------------------------------------------------------------------------------------------------- */

static const xchar_t introspect_header[] =
  "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
  "                      \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
  "<!-- GDBus " PACKAGE_VERSION " -->\n"
  "<node>\n";

static const xchar_t introspect_tail[] =
  "</node>\n";

static const xchar_t introspect_properties_interface[] =
  "  <interface name=\"org.freedesktop.DBus.Properties\">\n"
  "    <method name=\"Get\">\n"
  "      <arg type=\"s\" name=\"interface_name\" direction=\"in\"/>\n"
  "      <arg type=\"s\" name=\"property_name\" direction=\"in\"/>\n"
  "      <arg type=\"v\" name=\"value\" direction=\"out\"/>\n"
  "    </method>\n"
  "    <method name=\"GetAll\">\n"
  "      <arg type=\"s\" name=\"interface_name\" direction=\"in\"/>\n"
  "      <arg type=\"a{sv}\" name=\"properties\" direction=\"out\"/>\n"
  "    </method>\n"
  "    <method name=\"Set\">\n"
  "      <arg type=\"s\" name=\"interface_name\" direction=\"in\"/>\n"
  "      <arg type=\"s\" name=\"property_name\" direction=\"in\"/>\n"
  "      <arg type=\"v\" name=\"value\" direction=\"in\"/>\n"
  "    </method>\n"
  "    <signal name=\"PropertiesChanged\">\n"
  "      <arg type=\"s\" name=\"interface_name\"/>\n"
  "      <arg type=\"a{sv}\" name=\"changed_properties\"/>\n"
  "      <arg type=\"as\" name=\"invalidated_properties\"/>\n"
  "    </signal>\n"
  "  </interface>\n";

static const xchar_t introspect_introspectable_interface[] =
  "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
  "    <method name=\"Introspect\">\n"
  "      <arg type=\"s\" name=\"xml_data\" direction=\"out\"/>\n"
  "    </method>\n"
  "  </interface>\n"
  "  <interface name=\"org.freedesktop.DBus.Peer\">\n"
  "    <method name=\"Ping\"/>\n"
  "    <method name=\"GetMachineId\">\n"
  "      <arg type=\"s\" name=\"machine_uuid\" direction=\"out\"/>\n"
  "    </method>\n"
  "  </interface>\n";

static void
introspect_append_header (xstring_t *s)
{
  xstring_append (s, introspect_header);
}

static void
maybe_add_path (const xchar_t *path, xsize_t path_len, const xchar_t *object_path, xhashtable_t *set)
{
  if (xstr_has_prefix (object_path, path) && strlen (object_path) > path_len && object_path[path_len-1] == '/')
    {
      const xchar_t *begin;
      const xchar_t *end;
      xchar_t *s;

      begin = object_path + path_len;
      end = strchr (begin, '/');
      if (end != NULL)
        s = xstrndup (begin, end - begin);
      else
        s = xstrdup (begin);

      if (!xhash_table_contains (set, s))
        xhash_table_add (set, s);
      else
        g_free (s);
    }
}

/* TODO: we want a nicer public interface for this */
/* called in any thread with connection's lock held */
static xchar_t **
g_dbus_connection_list_registered_unlocked (xdbus_connection_t *connection,
                                            const xchar_t     *path)
{
  xptr_array_t *p;
  xchar_t **ret;
  xhash_table_iter_t hash_iter;
  const xchar_t *object_path;
  xsize_t path_len;
  xhashtable_t *set;
  xlist_t *keys;
  xlist_t *l;

  CONNECTION_ENSURE_LOCK (connection);

  path_len = strlen (path);
  if (path_len > 1)
    path_len++;

  set = xhash_table_new (xstr_hash, xstr_equal);

  xhash_table_iter_init (&hash_iter, connection->map_object_path_to_eo);
  while (xhash_table_iter_next (&hash_iter, (xpointer_t) &object_path, NULL))
    maybe_add_path (path, path_len, object_path, set);

  xhash_table_iter_init (&hash_iter, connection->map_object_path_to_es);
  while (xhash_table_iter_next (&hash_iter, (xpointer_t) &object_path, NULL))
    maybe_add_path (path, path_len, object_path, set);

  p = xptr_array_new ();
  keys = xhash_table_get_keys (set);
  for (l = keys; l != NULL; l = l->next)
    xptr_array_add (p, l->data);
  xhash_table_unref (set);
  xlist_free (keys);

  xptr_array_add (p, NULL);
  ret = (xchar_t **) xptr_array_free (p, FALSE);
  return ret;
}

/* called in any thread with connection's lock not held */
static xchar_t **
g_dbus_connection_list_registered (xdbus_connection_t *connection,
                                   const xchar_t     *path)
{
  xchar_t **ret;
  CONNECTION_LOCK (connection);
  ret = g_dbus_connection_list_registered_unlocked (connection, path);
  CONNECTION_UNLOCK (connection);
  return ret;
}

/* called in GDBusWorker thread with connection's lock held */
static xboolean_t
handle_introspect (xdbus_connection_t *connection,
                   ExportedObject  *eo,
                   xdbus_message_t    *message)
{
  xuint_t n;
  xstring_t *s;
  xdbus_message_t *reply;
  xhash_table_iter_t hash_iter;
  ExportedInterface *ei;
  xchar_t **registered;

  /* first the header with the standard interfaces */
  s = xstring_sized_new (sizeof (introspect_header) +
                          sizeof (introspect_properties_interface) +
                          sizeof (introspect_introspectable_interface) +
                          sizeof (introspect_tail));
  introspect_append_header (s);
  if (!xhash_table_lookup (eo->map_if_name_to_ei,
                            "org.freedesktop.DBus.Properties"))
    xstring_append (s, introspect_properties_interface);

  if (!xhash_table_lookup (eo->map_if_name_to_ei,
                            "org.freedesktop.DBus.Introspectable"))
    xstring_append (s, introspect_introspectable_interface);

  /* then include the registered interfaces */
  xhash_table_iter_init (&hash_iter, eo->map_if_name_to_ei);
  while (xhash_table_iter_next (&hash_iter, NULL, (xpointer_t) &ei))
    g_dbus_interface_info_generate_xml (ei->interface_info, 2, s);

  /* finally include nodes registered below us */
  registered = g_dbus_connection_list_registered_unlocked (connection, eo->object_path);
  for (n = 0; registered != NULL && registered[n] != NULL; n++)
    xstring_append_printf (s, "  <node name=\"%s\"/>\n", registered[n]);
  xstrfreev (registered);
  xstring_append (s, introspect_tail);

  reply = xdbus_message_new_method_reply (message);
  xdbus_message_set_body (reply, xvariant_new ("(s)", s->str));
  g_dbus_connection_send_message_unlocked (connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
  xobject_unref (reply);
  xstring_free (s, TRUE);

  return TRUE;
}

/* called in thread where object was registered - no locks held */
static xboolean_t
call_in_idle_cb (xpointer_t user_data)
{
  xdbus_method_invocation_t *invocation = G_DBUS_METHOD_INVOCATION (user_data);
  xdbus_interface_vtable_t *vtable;
  xuint_t registration_id;
  xuint_t subtree_registration_id;
  ExportedInterface *ei = NULL;
  ExportedSubtree *es = NULL;

  registration_id = GPOINTER_TO_UINT (xobject_get_data (G_OBJECT (invocation), "g-dbus-registration-id"));
  subtree_registration_id = GPOINTER_TO_UINT (xobject_get_data (G_OBJECT (invocation), "g-dbus-subtree-registration-id"));

  if (has_object_been_unregistered (xdbus_method_invocation_get_connection (invocation),
                                    registration_id,
                                    &ei,
                                    subtree_registration_id,
                                    &es))
    {
      xdbus_message_t *reply;
      reply = xdbus_message_new_method_error (xdbus_method_invocation_get_message (invocation),
                                               "org.freedesktop.DBus.Error.UnknownMethod",
                                               _("No such interface “%s” on object at path %s"),
                                               xdbus_method_invocation_get_interface_name (invocation),
                                               xdbus_method_invocation_get_object_path (invocation));
      g_dbus_connection_send_message (xdbus_method_invocation_get_connection (invocation), reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      xobject_unref (reply);
      goto out;
    }

  vtable = xobject_get_data (G_OBJECT (invocation), "g-dbus-interface-vtable");
  g_assert (vtable != NULL && vtable->method_call != NULL);

  vtable->method_call (xdbus_method_invocation_get_connection (invocation),
                       xdbus_method_invocation_get_sender (invocation),
                       xdbus_method_invocation_get_object_path (invocation),
                       xdbus_method_invocation_get_interface_name (invocation),
                       xdbus_method_invocation_get_method_name (invocation),
                       xdbus_method_invocation_get_parameters (invocation),
                       xobject_ref (invocation),
                       xdbus_method_invocation_get_user_data (invocation));

 out:
  g_clear_pointer (&ei, exported_interface_unref);
  g_clear_pointer (&es, exported_subtree_unref);

  return FALSE;
}

/* called in GDBusWorker thread with connection's lock held */
static void
schedule_method_call (xdbus_connection_t            *connection,
                      xdbus_message_t               *message,
                      xuint_t                       registration_id,
                      xuint_t                       subtree_registration_id,
                      const xdbus_interface_info_t   *interface_info,
                      const xdbus_method_info_t      *method_info,
                      const xdbus_property_info_t    *property_info,
                      xvariant_t                   *parameters,
                      const xdbus_interface_vtable_t *vtable,
                      xmain_context_t               *main_context,
                      xpointer_t                    user_data)
{
  xdbus_method_invocation_t *invocation;
  xsource_t *idle_source;

  invocation = _xdbus_method_invocation_new (xdbus_message_get_sender (message),
                                              xdbus_message_get_path (message),
                                              xdbus_message_get_interface (message),
                                              xdbus_message_get_member (message),
                                              method_info,
                                              property_info,
                                              connection,
                                              message,
                                              parameters,
                                              user_data);

  /* TODO: would be nicer with a real MethodData like we already
   * have PropertyData and PropertyGetAllData... */
  xobject_set_data (G_OBJECT (invocation), "g-dbus-interface-vtable", (xpointer_t) vtable);
  xobject_set_data (G_OBJECT (invocation), "g-dbus-registration-id", GUINT_TO_POINTER (registration_id));
  xobject_set_data (G_OBJECT (invocation), "g-dbus-subtree-registration-id", GUINT_TO_POINTER (subtree_registration_id));

  idle_source = g_idle_source_new ();
  xsource_set_priority (idle_source, G_PRIORITY_DEFAULT);
  xsource_set_callback (idle_source,
                         call_in_idle_cb,
                         invocation,
                         xobject_unref);
  xsource_set_static_name (idle_source, "[gio, " __FILE__ "] call_in_idle_cb");
  xsource_attach (idle_source, main_context);
  xsource_unref (idle_source);
}

/* called in GDBusWorker thread with connection's lock held */
static xboolean_t
validate_and_maybe_schedule_method_call (xdbus_connection_t            *connection,
                                         xdbus_message_t               *message,
                                         xuint_t                       registration_id,
                                         xuint_t                       subtree_registration_id,
                                         xdbus_interface_info_t         *interface_info,
                                         const xdbus_interface_vtable_t *vtable,
                                         xmain_context_t               *main_context,
                                         xpointer_t                    user_data)
{
  xdbus_method_info_t *method_info;
  xdbus_message_t *reply;
  xvariant_t *parameters;
  xboolean_t handled;
  xvariant_type_t *in_type;

  handled = FALSE;

  /* TODO: the cost of this is O(n) - it might be worth caching the result */
  method_info = g_dbus_interface_info_lookup_method (interface_info, xdbus_message_get_member (message));

  /* if the method doesn't exist, return the org.freedesktop.DBus.Error.UnknownMethod
   * error to the caller
   */
  if (method_info == NULL)
    {
      reply = xdbus_message_new_method_error (message,
                                               "org.freedesktop.DBus.Error.UnknownMethod",
                                               _("No such method “%s”"),
                                               xdbus_message_get_member (message));
      g_dbus_connection_send_message_unlocked (connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      xobject_unref (reply);
      handled = TRUE;
      goto out;
    }

  parameters = xdbus_message_get_body (message);
  if (parameters == NULL)
    {
      parameters = xvariant_new ("()");
      xvariant_ref_sink (parameters);
    }
  else
    {
      xvariant_ref (parameters);
    }

  /* Check that the incoming args are of the right type - if they are not, return
   * the org.freedesktop.DBus.Error.InvalidArgs error to the caller
   */
  in_type = _g_dbus_compute_complete_signature (method_info->in_args);
  if (!xvariant_is_of_type (parameters, in_type))
    {
      xchar_t *type_string;

      type_string = xvariant_type_dup_string (in_type);

      reply = xdbus_message_new_method_error (message,
                                               "org.freedesktop.DBus.Error.InvalidArgs",
                                               _("Type of message, “%s”, does not match expected type “%s”"),
                                               xvariant_get_type_string (parameters),
                                               type_string);
      g_dbus_connection_send_message_unlocked (connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      xvariant_type_free (in_type);
      xvariant_unref (parameters);
      xobject_unref (reply);
      g_free (type_string);
      handled = TRUE;
      goto out;
    }
  xvariant_type_free (in_type);

  /* schedule the call in idle */
  schedule_method_call (connection, message, registration_id, subtree_registration_id,
                        interface_info, method_info, NULL, parameters,
                        vtable, main_context, user_data);
  xvariant_unref (parameters);
  handled = TRUE;

 out:
  return handled;
}

/* ---------------------------------------------------------------------------------------------------- */

/* called in GDBusWorker thread with connection's lock held */
static xboolean_t
obj_message_func (xdbus_connection_t *connection,
                  ExportedObject  *eo,
                  xdbus_message_t    *message,
                  xboolean_t        *object_found)
{
  const xchar_t *interface_name;
  const xchar_t *member;
  const xchar_t *signature;
  xboolean_t handled;

  handled = FALSE;

  interface_name = xdbus_message_get_interface (message);
  member = xdbus_message_get_member (message);
  signature = xdbus_message_get_signature (message);

  /* see if we have an interface for handling this call */
  if (interface_name != NULL)
    {
      ExportedInterface *ei;
      ei = xhash_table_lookup (eo->map_if_name_to_ei, interface_name);
      if (ei != NULL)
        {
          /* we do - invoke the handler in idle in the right thread */

          /* handle no vtable or handler being present */
          if (ei->vtable == NULL || ei->vtable->method_call == NULL)
            goto out;

          handled = validate_and_maybe_schedule_method_call (connection,
                                                             message,
                                                             ei->id,
                                                             0,
                                                             ei->interface_info,
                                                             ei->vtable,
                                                             ei->context,
                                                             ei->user_data);
          goto out;
        }
      else
        {
          *object_found = TRUE;
        }
    }

  if (xstrcmp0 (interface_name, "org.freedesktop.DBus.Introspectable") == 0 &&
      xstrcmp0 (member, "Introspect") == 0 &&
      xstrcmp0 (signature, "") == 0)
    {
      handled = handle_introspect (connection, eo, message);
      goto out;
    }
  else if (xstrcmp0 (interface_name, "org.freedesktop.DBus.Properties") == 0 &&
           xstrcmp0 (member, "Get") == 0 &&
           xstrcmp0 (signature, "ss") == 0)
    {
      handled = handle_getset_property (connection, eo, message, TRUE);
      goto out;
    }
  else if (xstrcmp0 (interface_name, "org.freedesktop.DBus.Properties") == 0 &&
           xstrcmp0 (member, "Set") == 0 &&
           xstrcmp0 (signature, "ssv") == 0)
    {
      handled = handle_getset_property (connection, eo, message, FALSE);
      goto out;
    }
  else if (xstrcmp0 (interface_name, "org.freedesktop.DBus.Properties") == 0 &&
           xstrcmp0 (member, "GetAll") == 0 &&
           xstrcmp0 (signature, "s") == 0)
    {
      handled = handle_get_all_properties (connection, eo, message);
      goto out;
    }

 out:
  return handled;
}

/**
 * g_dbus_connection_register_object:
 * @connection: a #xdbus_connection_t
 * @object_path: the object path to register at
 * @interface_info: introspection data for the interface
 * @vtable: (nullable): a #xdbus_interface_vtable_t to call into or %NULL
 * @user_data: (nullable): data to pass to functions in @vtable
 * @user_data_free_func: function to call when the object path is unregistered
 * @error: return location for error or %NULL
 *
 * Registers callbacks for exported objects at @object_path with the
 * D-Bus interface that is described in @interface_info.
 *
 * Calls to functions in @vtable (and @user_data_free_func) will happen
 * in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread you are calling this method from.
 *
 * Note that all #xvariant_t values passed to functions in @vtable will match
 * the signature given in @interface_info - if a remote caller passes
 * incorrect values, the `org.freedesktop.DBus.Error.InvalidArgs`
 * is returned to the remote caller.
 *
 * Additionally, if the remote caller attempts to invoke methods or
 * access properties not mentioned in @interface_info the
 * `org.freedesktop.DBus.Error.UnknownMethod` resp.
 * `org.freedesktop.DBus.Error.InvalidArgs` errors
 * are returned to the caller.
 *
 * It is considered a programming error if the
 * #GDBusInterfaceGetPropertyFunc function in @vtable returns a
 * #xvariant_t of incorrect type.
 *
 * If an existing callback is already registered at @object_path and
 * @interface_name, then @error is set to %G_IO_ERROR_EXISTS.
 *
 * GDBus automatically implements the standard D-Bus interfaces
 * org.freedesktop.DBus.Properties, org.freedesktop.DBus.Introspectable
 * and org.freedesktop.Peer, so you don't have to implement those for the
 * objects you export. You can implement org.freedesktop.DBus.Properties
 * yourself, e.g. to handle getting and setting of properties asynchronously.
 *
 * Note that the reference count on @interface_info will be
 * incremented by 1 (unless allocated statically, e.g. if the
 * reference count is -1, see g_dbus_interface_info_ref()) for as long
 * as the object is exported. Also note that @vtable will be copied.
 *
 * See this [server][gdbus-server] for an example of how to use this method.
 *
 * Returns: 0 if @error is set, otherwise a registration id (never 0)
 *     that can be used with g_dbus_connection_unregister_object()
 *
 * Since: 2.26
 */
xuint_t
g_dbus_connection_register_object (xdbus_connection_t             *connection,
                                   const xchar_t                 *object_path,
                                   xdbus_interface_info_t          *interface_info,
                                   const xdbus_interface_vtable_t  *vtable,
                                   xpointer_t                     user_data,
                                   xdestroy_notify_t               user_data_free_func,
                                   xerror_t                     **error)
{
  ExportedObject *eo;
  ExportedInterface *ei;
  xuint_t ret;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), 0);
  g_return_val_if_fail (object_path != NULL && xvariant_is_object_path (object_path), 0);
  g_return_val_if_fail (interface_info != NULL, 0);
  g_return_val_if_fail (g_dbus_is_interface_name (interface_info->name), 0);
  g_return_val_if_fail (error == NULL || *error == NULL, 0);
  g_return_val_if_fail (check_initialized (connection), 0);

  ret = 0;

  CONNECTION_LOCK (connection);

  eo = xhash_table_lookup (connection->map_object_path_to_eo, object_path);
  if (eo == NULL)
    {
      eo = g_new0 (ExportedObject, 1);
      eo->object_path = xstrdup (object_path);
      eo->connection = connection;
      eo->map_if_name_to_ei = xhash_table_new_full (xstr_hash,
                                                     xstr_equal,
                                                     NULL,
                                                     (xdestroy_notify_t) exported_interface_unref);
      xhash_table_insert (connection->map_object_path_to_eo, eo->object_path, eo);
    }

  ei = xhash_table_lookup (eo->map_if_name_to_ei, interface_info->name);
  if (ei != NULL)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_EXISTS,
                   _("An object is already exported for the interface %s at %s"),
                   interface_info->name,
                   object_path);
      goto out;
    }

  ei = g_new0 (ExportedInterface, 1);
  ei->refcount = 1;
  ei->id = (xuint_t) g_atomic_int_add (&_global_registration_id, 1); /* TODO: overflow etc. */
  ei->eo = eo;
  ei->user_data = user_data;
  ei->user_data_free_func = user_data_free_func;
  ei->vtable = _g_dbus_interface_vtable_copy (vtable);
  ei->interface_info = g_dbus_interface_info_ref (interface_info);
  g_dbus_interface_info_cache_build (ei->interface_info);
  ei->interface_name = xstrdup (interface_info->name);
  ei->context = xmain_context_ref_thread_default ();

  xhash_table_insert (eo->map_if_name_to_ei,
                       (xpointer_t) ei->interface_name,
                       ei);
  xhash_table_insert (connection->map_id_to_ei,
                       GUINT_TO_POINTER (ei->id),
                       ei);

  ret = ei->id;

 out:
  CONNECTION_UNLOCK (connection);

  return ret;
}

/**
 * g_dbus_connection_unregister_object:
 * @connection: a #xdbus_connection_t
 * @registration_id: a registration id obtained from
 *     g_dbus_connection_register_object()
 *
 * Unregisters an object.
 *
 * Returns: %TRUE if the object was unregistered, %FALSE otherwise
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_connection_unregister_object (xdbus_connection_t *connection,
                                     xuint_t            registration_id)
{
  ExportedInterface *ei;
  ExportedObject *eo;
  xboolean_t ret;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);
  g_return_val_if_fail (check_initialized (connection), FALSE);

  ret = FALSE;

  CONNECTION_LOCK (connection);

  ei = xhash_table_lookup (connection->map_id_to_ei,
                            GUINT_TO_POINTER (registration_id));
  if (ei == NULL)
    goto out;

  eo = ei->eo;

  g_warn_if_fail (xhash_table_remove (connection->map_id_to_ei, GUINT_TO_POINTER (ei->id)));
  g_warn_if_fail (xhash_table_remove (eo->map_if_name_to_ei, ei->interface_name));
  /* unregister object path if we have no more exported interfaces */
  if (xhash_table_size (eo->map_if_name_to_ei) == 0)
    g_warn_if_fail (xhash_table_remove (connection->map_object_path_to_eo,
                                         eo->object_path));

  ret = TRUE;

 out:
  CONNECTION_UNLOCK (connection);

  return ret;
}

typedef struct {
  xclosure_t *method_call_closure;
  xclosure_t *get_property_closure;
  xclosure_t *set_property_closure;
} RegisterObjectData;

static RegisterObjectData *
register_object_data_new (xclosure_t *method_call_closure,
                          xclosure_t *get_property_closure,
                          xclosure_t *set_property_closure)
{
  RegisterObjectData *data;

  data = g_new0 (RegisterObjectData, 1);

  if (method_call_closure != NULL)
    {
      data->method_call_closure = xclosure_ref (method_call_closure);
      xclosure_sink (method_call_closure);
      if (G_CLOSURE_NEEDS_MARSHAL (method_call_closure))
        xclosure_set_marshal (method_call_closure, g_cclosure_marshal_generic);
    }

  if (get_property_closure != NULL)
    {
      data->get_property_closure = xclosure_ref (get_property_closure);
      xclosure_sink (get_property_closure);
      if (G_CLOSURE_NEEDS_MARSHAL (get_property_closure))
        xclosure_set_marshal (get_property_closure, g_cclosure_marshal_generic);
    }

  if (set_property_closure != NULL)
    {
      data->set_property_closure = xclosure_ref (set_property_closure);
      xclosure_sink (set_property_closure);
      if (G_CLOSURE_NEEDS_MARSHAL (set_property_closure))
        xclosure_set_marshal (set_property_closure, g_cclosure_marshal_generic);
    }

  return data;
}

static void
register_object_free_func (xpointer_t user_data)
{
  RegisterObjectData *data = user_data;

  g_clear_pointer (&data->method_call_closure, xclosure_unref);
  g_clear_pointer (&data->get_property_closure, xclosure_unref);
  g_clear_pointer (&data->set_property_closure, xclosure_unref);

  g_free (data);
}

static void
register_with_closures_on_method_call (xdbus_connection_t       *connection,
                                       const xchar_t           *sender,
                                       const xchar_t           *object_path,
                                       const xchar_t           *interface_name,
                                       const xchar_t           *method_name,
                                       xvariant_t              *parameters,
                                       xdbus_method_invocation_t *invocation,
                                       xpointer_t               user_data)
{
  RegisterObjectData *data = user_data;
  xvalue_t params[] = { G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT };

  xvalue_init (&params[0], XTYPE_DBUS_CONNECTION);
  xvalue_set_object (&params[0], connection);

  xvalue_init (&params[1], XTYPE_STRING);
  xvalue_set_string (&params[1], sender);

  xvalue_init (&params[2], XTYPE_STRING);
  xvalue_set_string (&params[2], object_path);

  xvalue_init (&params[3], XTYPE_STRING);
  xvalue_set_string (&params[3], interface_name);

  xvalue_init (&params[4], XTYPE_STRING);
  xvalue_set_string (&params[4], method_name);

  xvalue_init (&params[5], XTYPE_VARIANT);
  xvalue_set_variant (&params[5], parameters);

  xvalue_init (&params[6], XTYPE_DBUS_METHOD_INVOCATION);
  xvalue_set_object (&params[6], invocation);

  xclosure_invoke (data->method_call_closure, NULL, G_N_ELEMENTS (params), params, NULL);

  xvalue_unset (params + 0);
  xvalue_unset (params + 1);
  xvalue_unset (params + 2);
  xvalue_unset (params + 3);
  xvalue_unset (params + 4);
  xvalue_unset (params + 5);
  xvalue_unset (params + 6);
}

static xvariant_t *
register_with_closures_on_get_property (xdbus_connection_t *connection,
                                        const xchar_t     *sender,
                                        const xchar_t     *object_path,
                                        const xchar_t     *interface_name,
                                        const xchar_t     *property_name,
                                        xerror_t         **error,
                                        xpointer_t         user_data)
{
  RegisterObjectData *data = user_data;
  xvalue_t params[] = { G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT };
  xvalue_t result_value = G_VALUE_INIT;
  xvariant_t *result;

  xvalue_init (&params[0], XTYPE_DBUS_CONNECTION);
  xvalue_set_object (&params[0], connection);

  xvalue_init (&params[1], XTYPE_STRING);
  xvalue_set_string (&params[1], sender);

  xvalue_init (&params[2], XTYPE_STRING);
  xvalue_set_string (&params[2], object_path);

  xvalue_init (&params[3], XTYPE_STRING);
  xvalue_set_string (&params[3], interface_name);

  xvalue_init (&params[4], XTYPE_STRING);
  xvalue_set_string (&params[4], property_name);

  xvalue_init (&result_value, XTYPE_VARIANT);

  xclosure_invoke (data->get_property_closure, &result_value, G_N_ELEMENTS (params), params, NULL);

  result = xvalue_get_variant (&result_value);
  if (result)
    xvariant_ref (result);

  xvalue_unset (params + 0);
  xvalue_unset (params + 1);
  xvalue_unset (params + 2);
  xvalue_unset (params + 3);
  xvalue_unset (params + 4);
  xvalue_unset (&result_value);

  if (!result)
    g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                 _("Unable to retrieve property %s.%s"),
                 interface_name, property_name);

  return result;
}

static xboolean_t
register_with_closures_on_set_property (xdbus_connection_t *connection,
                                        const xchar_t     *sender,
                                        const xchar_t     *object_path,
                                        const xchar_t     *interface_name,
                                        const xchar_t     *property_name,
                                        xvariant_t        *value,
                                        xerror_t         **error,
                                        xpointer_t         user_data)
{
  RegisterObjectData *data = user_data;
  xvalue_t params[] = { G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT };
  xvalue_t result_value = G_VALUE_INIT;
  xboolean_t result;

  xvalue_init (&params[0], XTYPE_DBUS_CONNECTION);
  xvalue_set_object (&params[0], connection);

  xvalue_init (&params[1], XTYPE_STRING);
  xvalue_set_string (&params[1], sender);

  xvalue_init (&params[2], XTYPE_STRING);
  xvalue_set_string (&params[2], object_path);

  xvalue_init (&params[3], XTYPE_STRING);
  xvalue_set_string (&params[3], interface_name);

  xvalue_init (&params[4], XTYPE_STRING);
  xvalue_set_string (&params[4], property_name);

  xvalue_init (&params[5], XTYPE_VARIANT);
  xvalue_set_variant (&params[5], value);

  xvalue_init (&result_value, XTYPE_BOOLEAN);

  xclosure_invoke (data->set_property_closure, &result_value, G_N_ELEMENTS (params), params, NULL);

  result = xvalue_get_boolean (&result_value);

  xvalue_unset (params + 0);
  xvalue_unset (params + 1);
  xvalue_unset (params + 2);
  xvalue_unset (params + 3);
  xvalue_unset (params + 4);
  xvalue_unset (params + 5);
  xvalue_unset (&result_value);

  if (!result)
    g_set_error (error,
                 G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                 _("Unable to set property %s.%s"),
                 interface_name, property_name);

  return result;
}

/**
 * g_dbus_connection_register_object_with_closures: (rename-to g_dbus_connection_register_object)
 * @connection: A #xdbus_connection_t.
 * @object_path: The object path to register at.
 * @interface_info: Introspection data for the interface.
 * @method_call_closure: (nullable): #xclosure_t for handling incoming method calls.
 * @get_property_closure: (nullable): #xclosure_t for getting a property.
 * @set_property_closure: (nullable): #xclosure_t for setting a property.
 * @error: Return location for error or %NULL.
 *
 * Version of g_dbus_connection_register_object() using closures instead of a
 * #xdbus_interface_vtable_t for easier binding in other languages.
 *
 * Returns: 0 if @error is set, otherwise a registration ID (never 0)
 * that can be used with g_dbus_connection_unregister_object() .
 *
 * Since: 2.46
 */
xuint_t
g_dbus_connection_register_object_with_closures (xdbus_connection_t     *connection,
                                                 const xchar_t         *object_path,
                                                 xdbus_interface_info_t  *interface_info,
                                                 xclosure_t            *method_call_closure,
                                                 xclosure_t            *get_property_closure,
                                                 xclosure_t            *set_property_closure,
                                                 xerror_t             **error)
{
  RegisterObjectData *data;
  xdbus_interface_vtable_t vtable =
    {
      method_call_closure != NULL  ? register_with_closures_on_method_call  : NULL,
      get_property_closure != NULL ? register_with_closures_on_get_property : NULL,
      set_property_closure != NULL ? register_with_closures_on_set_property : NULL,
      { 0 }
    };

  data = register_object_data_new (method_call_closure, get_property_closure, set_property_closure);

  return g_dbus_connection_register_object (connection,
                                            object_path,
                                            interface_info,
                                            &vtable,
                                            data,
                                            register_object_free_func,
                                            error);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_connection_emit_signal:
 * @connection: a #xdbus_connection_t
 * @destination_bus_name: (nullable): the unique bus name for the destination
 *     for the signal or %NULL to emit to all listeners
 * @object_path: path of remote object
 * @interface_name: D-Bus interface to emit a signal on
 * @signal_name: the name of the signal to emit
 * @parameters: (nullable): a #xvariant_t tuple with parameters for the signal
 *              or %NULL if not passing parameters
 * @error: Return location for error or %NULL
 *
 * Emits a signal.
 *
 * If the parameters xvariant_t is floating, it is consumed.
 *
 * This can only fail if @parameters is not compatible with the D-Bus protocol
 * (%G_IO_ERROR_INVALID_ARGUMENT), or if @connection has been closed
 * (%G_IO_ERROR_CLOSED).
 *
 * Returns: %TRUE unless @error is set
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_connection_emit_signal (xdbus_connection_t  *connection,
                               const xchar_t      *destination_bus_name,
                               const xchar_t      *object_path,
                               const xchar_t      *interface_name,
                               const xchar_t      *signal_name,
                               xvariant_t         *parameters,
                               xerror_t          **error)
{
  xdbus_message_t *message;
  xboolean_t ret;

  message = NULL;
  ret = FALSE;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);
  g_return_val_if_fail (destination_bus_name == NULL || g_dbus_is_name (destination_bus_name), FALSE);
  g_return_val_if_fail (object_path != NULL && xvariant_is_object_path (object_path), FALSE);
  g_return_val_if_fail (interface_name != NULL && g_dbus_is_interface_name (interface_name), FALSE);
  g_return_val_if_fail (signal_name != NULL && g_dbus_is_member_name (signal_name), FALSE);
  g_return_val_if_fail (parameters == NULL || xvariant_is_of_type (parameters, G_VARIANT_TYPE_TUPLE), FALSE);
  g_return_val_if_fail (check_initialized (connection), FALSE);

  if (G_UNLIKELY (_g_dbus_debug_emission ()))
    {
      _g_dbus_debug_print_lock ();
      g_print ("========================================================================\n"
               "GDBus-debug:Emission:\n"
               " >>>> SIGNAL EMISSION %s.%s()\n"
               "      on object %s\n"
               "      destination %s\n",
               interface_name, signal_name,
               object_path,
               destination_bus_name != NULL ? destination_bus_name : "(none)");
      _g_dbus_debug_print_unlock ();
    }

  message = xdbus_message_new_signal (object_path,
                                       interface_name,
                                       signal_name);

  if (destination_bus_name != NULL)
    xdbus_message_set_header (message,
                               G_DBUS_MESSAGE_HEADER_FIELD_DESTINATION,
                               xvariant_new_string (destination_bus_name));

  if (parameters != NULL)
    xdbus_message_set_body (message, parameters);

  ret = g_dbus_connection_send_message (connection, message, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, error);
  xobject_unref (message);

  return ret;
}

static void
add_call_flags (xdbus_message_t           *message,
                         GDBusCallFlags  flags)
{
  GDBusMessageFlags msg_flags = 0;

  if (flags & G_DBUS_CALL_FLAGS_NO_AUTO_START)
    msg_flags |= G_DBUS_MESSAGE_FLAGS_NO_AUTO_START;
  if (flags & G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION)
    msg_flags |= G_DBUS_MESSAGE_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION;
  if (msg_flags)
    xdbus_message_set_flags (message, msg_flags);
}

static xvariant_t *
decode_method_reply (xdbus_message_t        *reply,
                     const xchar_t         *method_name,
                     const xvariant_type_t  *reply_type,
                     xunix_fd_list_t        **out_fd_list,
                     xerror_t             **error)
{
  xvariant_t *result;

  result = NULL;
  switch (xdbus_message_get_message_type (reply))
    {
    case G_DBUS_MESSAGE_TYPE_METHOD_RETURN:
      result = xdbus_message_get_body (reply);
      if (result == NULL)
        {
          result = xvariant_new ("()");
          xvariant_ref_sink (result);
        }
      else
        {
          xvariant_ref (result);
        }

      if (!xvariant_is_of_type (result, reply_type))
        {
          xchar_t *type_string = xvariant_type_dup_string (reply_type);

          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Method “%s” returned type “%s”, but expected “%s”"),
                       method_name, xvariant_get_type_string (result), type_string);

          xvariant_unref (result);
          g_free (type_string);
          result = NULL;
        }

#ifdef G_OS_UNIX
      if (result != NULL)
        {
          if (out_fd_list != NULL)
            {
              *out_fd_list = xdbus_message_get_unix_fd_list (reply);
              if (*out_fd_list != NULL)
                xobject_ref (*out_fd_list);
            }
        }
#endif
      break;

    case G_DBUS_MESSAGE_TYPE_ERROR:
      xdbus_message_to_gerror (reply, error);
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  return result;
}


typedef struct
{
  xvariant_type_t *reply_type;
  xchar_t *method_name; /* for error message */

  xunix_fd_list_t *fd_list;
} CallState;

static void
call_state_free (CallState *state)
{
  xvariant_type_free (state->reply_type);
  g_free (state->method_name);

  if (state->fd_list != NULL)
    xobject_unref (state->fd_list);
  g_slice_free (CallState, state);
}

/* called in any thread, with the connection's lock not held */
static void
g_dbus_connection_call_done (xobject_t      *source,
                             xasync_result_t *result,
                             xpointer_t      user_data)
{
  xdbus_connection_t *connection = G_DBUS_CONNECTION (source);
  xtask_t *task = user_data;
  CallState *state = xtask_get_task_data (task);
  xerror_t *error = NULL;
  xdbus_message_t *reply;
  xvariant_t *value = NULL;

  reply = g_dbus_connection_send_message_with_reply_finish (connection,
                                                            result,
                                                            &error);

  if (G_UNLIKELY (_g_dbus_debug_call ()))
    {
      _g_dbus_debug_print_lock ();
      g_print ("========================================================================\n"
               "GDBus-debug:Call:\n"
               " <<<< ASYNC COMPLETE %s()",
               state->method_name);

      if (reply != NULL)
        {
          g_print (" (serial %d)\n"
                   "      SUCCESS\n",
                   xdbus_message_get_reply_serial (reply));
        }
      else
        {
          g_print ("\n"
                   "      FAILED: %s\n",
                   error->message);
        }
      _g_dbus_debug_print_unlock ();
    }

  if (reply != NULL)
    value = decode_method_reply (reply, state->method_name, state->reply_type, &state->fd_list, &error);

  if (error != NULL)
    xtask_return_error (task, error);
  else
    xtask_return_pointer (task, value, (xdestroy_notify_t) xvariant_unref);

  g_clear_object (&reply);
  xobject_unref (task);
}

/* called in any thread, with the connection's lock not held */
static void
g_dbus_connection_call_internal (xdbus_connection_t        *connection,
                                 const xchar_t            *bus_name,
                                 const xchar_t            *object_path,
                                 const xchar_t            *interface_name,
                                 const xchar_t            *method_name,
                                 xvariant_t               *parameters,
                                 const xvariant_type_t     *reply_type,
                                 GDBusCallFlags          flags,
                                 xint_t                    timeout_msec,
                                 xunix_fd_list_t            *fd_list,
                                 xcancellable_t           *cancellable,
                                 xasync_ready_callback_t     callback,
                                 xpointer_t                user_data)
{
  xdbus_message_t *message;
  xuint32_t serial;

  g_return_if_fail (X_IS_DBUS_CONNECTION (connection));
  g_return_if_fail (bus_name == NULL || g_dbus_is_name (bus_name));
  g_return_if_fail (object_path != NULL && xvariant_is_object_path (object_path));
  g_return_if_fail (interface_name != NULL && g_dbus_is_interface_name (interface_name));
  g_return_if_fail (method_name != NULL && g_dbus_is_member_name (method_name));
  g_return_if_fail (timeout_msec >= 0 || timeout_msec == -1);
  g_return_if_fail ((parameters == NULL) || xvariant_is_of_type (parameters, G_VARIANT_TYPE_TUPLE));
  g_return_if_fail (check_initialized (connection));
#ifdef G_OS_UNIX
  g_return_if_fail (fd_list == NULL || X_IS_UNIX_FD_LIST (fd_list));
#else
  g_return_if_fail (fd_list == NULL);
#endif

  message = xdbus_message_new_method_call (bus_name,
                                            object_path,
                                            interface_name,
                                            method_name);
  add_call_flags (message, flags);
  if (parameters != NULL)
    xdbus_message_set_body (message, parameters);

#ifdef G_OS_UNIX
  if (fd_list != NULL)
    xdbus_message_set_unix_fd_list (message, fd_list);
#endif

  /* If the user has no callback then we can just send the message with
   * the G_DBUS_MESSAGE_FLAGS_NO_REPLY_EXPECTED flag set and skip all
   * the logic for processing the reply.  If the service sends the reply
   * anyway then it will just be ignored.
   */
  if (callback != NULL)
    {
      CallState *state;
      xtask_t *task;

      state = g_slice_new0 (CallState);
      state->method_name = xstrjoin (".", interface_name, method_name, NULL);

      if (reply_type == NULL)
        reply_type = G_VARIANT_TYPE_ANY;

      state->reply_type = xvariant_type_copy (reply_type);

      task = xtask_new (connection, cancellable, callback, user_data);
      xtask_set_source_tag (task, g_dbus_connection_call_internal);
      xtask_set_task_data (task, state, (xdestroy_notify_t) call_state_free);

      g_dbus_connection_send_message_with_reply (connection,
                                                 message,
                                                 G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                                 timeout_msec,
                                                 &serial,
                                                 cancellable,
                                                 g_dbus_connection_call_done,
                                                 task);
    }
  else
    {
      GDBusMessageFlags flags;

      flags = xdbus_message_get_flags (message);
      flags |= G_DBUS_MESSAGE_FLAGS_NO_REPLY_EXPECTED;
      xdbus_message_set_flags (message, flags);

      g_dbus_connection_send_message (connection,
                                      message,
                                      G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                      &serial, NULL);
    }

  if (G_UNLIKELY (_g_dbus_debug_call ()))
    {
      _g_dbus_debug_print_lock ();
      g_print ("========================================================================\n"
               "GDBus-debug:Call:\n"
               " >>>> ASYNC %s.%s()\n"
               "      on object %s\n"
               "      owned by name %s (serial %d)\n",
               interface_name,
               method_name,
               object_path,
               bus_name != NULL ? bus_name : "(none)",
               serial);
      _g_dbus_debug_print_unlock ();
    }

  if (message != NULL)
    xobject_unref (message);
}

/* called in any thread, with the connection's lock not held */
static xvariant_t *
g_dbus_connection_call_finish_internal (xdbus_connection_t  *connection,
                                        xunix_fd_list_t     **out_fd_list,
                                        xasync_result_t     *res,
                                        xerror_t          **error)
{
  xtask_t *task;
  CallState *state;
  xvariant_t *ret;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (xtask_is_valid (res, connection), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  task = XTASK (res);
  state = xtask_get_task_data (task);

  ret = xtask_propagate_pointer (task, error);
  if (!ret)
    return NULL;

  if (out_fd_list != NULL)
    *out_fd_list = state->fd_list != NULL ? xobject_ref (state->fd_list) : NULL;
  return ret;
}

/* called in any user thread, with the connection's lock not held */
static xvariant_t *
g_dbus_connection_call_sync_internal (xdbus_connection_t         *connection,
                                      const xchar_t             *bus_name,
                                      const xchar_t             *object_path,
                                      const xchar_t             *interface_name,
                                      const xchar_t             *method_name,
                                      xvariant_t                *parameters,
                                      const xvariant_type_t      *reply_type,
                                      GDBusCallFlags           flags,
                                      xint_t                     timeout_msec,
                                      xunix_fd_list_t             *fd_list,
                                      xunix_fd_list_t            **out_fd_list,
                                      xcancellable_t            *cancellable,
                                      xerror_t                 **error)
{
  xdbus_message_t *message;
  xdbus_message_t *reply;
  xvariant_t *result;
  xerror_t *local_error;
  GDBusSendMessageFlags send_flags;

  message = NULL;
  reply = NULL;
  result = NULL;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (bus_name == NULL || g_dbus_is_name (bus_name), NULL);
  g_return_val_if_fail (object_path != NULL && xvariant_is_object_path (object_path), NULL);
  g_return_val_if_fail (interface_name != NULL && g_dbus_is_interface_name (interface_name), NULL);
  g_return_val_if_fail (method_name != NULL && g_dbus_is_member_name (method_name), NULL);
  g_return_val_if_fail (timeout_msec >= 0 || timeout_msec == -1, NULL);
  g_return_val_if_fail ((parameters == NULL) || xvariant_is_of_type (parameters, G_VARIANT_TYPE_TUPLE), NULL);
#ifdef G_OS_UNIX
  g_return_val_if_fail (fd_list == NULL || X_IS_UNIX_FD_LIST (fd_list), NULL);
#else
  g_return_val_if_fail (fd_list == NULL, NULL);
#endif
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (!(flags & CALL_FLAGS_INITIALIZING))
    g_return_val_if_fail (check_initialized (connection), FALSE);

  if (reply_type == NULL)
    reply_type = G_VARIANT_TYPE_ANY;

  message = xdbus_message_new_method_call (bus_name,
                                            object_path,
                                            interface_name,
                                            method_name);
  add_call_flags (message, flags);
  if (parameters != NULL)
    xdbus_message_set_body (message, parameters);

#ifdef G_OS_UNIX
  if (fd_list != NULL)
    xdbus_message_set_unix_fd_list (message, fd_list);
#endif

  if (G_UNLIKELY (_g_dbus_debug_call ()))
    {
      _g_dbus_debug_print_lock ();
      g_print ("========================================================================\n"
               "GDBus-debug:Call:\n"
               " >>>> SYNC %s.%s()\n"
               "      on object %s\n"
               "      owned by name %s\n",
               interface_name,
               method_name,
               object_path,
               bus_name != NULL ? bus_name : "(none)");
      _g_dbus_debug_print_unlock ();
    }

  local_error = NULL;

  send_flags = G_DBUS_SEND_MESSAGE_FLAGS_NONE;

  /* translate from one flavour of flags to another... */
  if (flags & CALL_FLAGS_INITIALIZING)
    send_flags |= SEND_MESSAGE_FLAGS_INITIALIZING;

  reply = g_dbus_connection_send_message_with_reply_sync (connection,
                                                          message,
                                                          send_flags,
                                                          timeout_msec,
                                                          NULL, /* xuint32_t *out_serial */
                                                          cancellable,
                                                          &local_error);

  if (G_UNLIKELY (_g_dbus_debug_call ()))
    {
      _g_dbus_debug_print_lock ();
      g_print ("========================================================================\n"
               "GDBus-debug:Call:\n"
               " <<<< SYNC COMPLETE %s.%s()\n"
               "      ",
               interface_name,
               method_name);
      if (reply != NULL)
        {
          g_print ("SUCCESS\n");
        }
      else
        {
          g_print ("FAILED: %s\n",
                   local_error->message);
        }
      _g_dbus_debug_print_unlock ();
    }

  if (reply == NULL)
    {
      if (error != NULL)
        *error = local_error;
      else
        xerror_free (local_error);
      goto out;
    }

  result = decode_method_reply (reply, method_name, reply_type, out_fd_list, error);

 out:
  if (message != NULL)
    xobject_unref (message);
  if (reply != NULL)
    xobject_unref (reply);

  return result;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_connection_call:
 * @connection: a #xdbus_connection_t
 * @bus_name: (nullable): a unique or well-known bus name or %NULL if
 *     @connection is not a message bus connection
 * @object_path: path of remote object
 * @interface_name: D-Bus interface to invoke method on
 * @method_name: the name of the method to invoke
 * @parameters: (nullable): a #xvariant_t tuple with parameters for the method
 *     or %NULL if not passing parameters
 * @reply_type: (nullable): the expected type of the reply (which will be a
 *     tuple), or %NULL
 * @flags: flags from the #GDBusCallFlags enumeration
 * @timeout_msec: the timeout in milliseconds, -1 to use the default
 *     timeout or %G_MAXINT for no timeout
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @callback: (nullable): a #xasync_ready_callback_t to call when the request
 *     is satisfied or %NULL if you don't care about the result of the
 *     method invocation
 * @user_data: the data to pass to @callback
 *
 * Asynchronously invokes the @method_name method on the
 * @interface_name D-Bus interface on the remote object at
 * @object_path owned by @bus_name.
 *
 * If @connection is closed then the operation will fail with
 * %G_IO_ERROR_CLOSED. If @cancellable is canceled, the operation will
 * fail with %G_IO_ERROR_CANCELLED. If @parameters contains a value
 * not compatible with the D-Bus protocol, the operation fails with
 * %G_IO_ERROR_INVALID_ARGUMENT.
 *
 * If @reply_type is non-%NULL then the reply will be checked for having this type and an
 * error will be raised if it does not match.  Said another way, if you give a @reply_type
 * then any non-%NULL return value will be of this type. Unless it’s
 * %G_VARIANT_TYPE_UNIT, the @reply_type will be a tuple containing one or more
 * values.
 *
 * If the @parameters #xvariant_t is floating, it is consumed. This allows
 * convenient 'inline' use of xvariant_new(), e.g.:
 * |[<!-- language="C" -->
 *  g_dbus_connection_call (connection,
 *                          "org.freedesktop.StringThings",
 *                          "/org/freedesktop/StringThings",
 *                          "org.freedesktop.StringThings",
 *                          "TwoStrings",
 *                          xvariant_new ("(ss)",
 *                                         "Thing One",
 *                                         "Thing Two"),
 *                          NULL,
 *                          G_DBUS_CALL_FLAGS_NONE,
 *                          -1,
 *                          NULL,
 *                          (xasync_ready_callback_t) two_strings_done,
 *                          NULL);
 * ]|
 *
 * This is an asynchronous method. When the operation is finished,
 * @callback will be invoked in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread you are calling this method from. You can then call
 * g_dbus_connection_call_finish() to get the result of the operation.
 * See g_dbus_connection_call_sync() for the synchronous version of this
 * function.
 *
 * If @callback is %NULL then the D-Bus method call message will be sent with
 * the %G_DBUS_MESSAGE_FLAGS_NO_REPLY_EXPECTED flag set.
 *
 * Since: 2.26
 */
void
g_dbus_connection_call (xdbus_connection_t     *connection,
                        const xchar_t         *bus_name,
                        const xchar_t         *object_path,
                        const xchar_t         *interface_name,
                        const xchar_t         *method_name,
                        xvariant_t            *parameters,
                        const xvariant_type_t  *reply_type,
                        GDBusCallFlags       flags,
                        xint_t                 timeout_msec,
                        xcancellable_t        *cancellable,
                        xasync_ready_callback_t  callback,
                        xpointer_t             user_data)
{
  g_dbus_connection_call_internal (connection, bus_name, object_path, interface_name, method_name, parameters, reply_type, flags, timeout_msec, NULL, cancellable, callback, user_data);
}

/**
 * g_dbus_connection_call_finish:
 * @connection: a #xdbus_connection_t
 * @res: a #xasync_result_t obtained from the #xasync_ready_callback_t passed to g_dbus_connection_call()
 * @error: return location for error or %NULL
 *
 * Finishes an operation started with g_dbus_connection_call().
 *
 * Returns: (transfer full): %NULL if @error is set. Otherwise a non-floating
 *     #xvariant_t tuple with return values. Free with xvariant_unref().
 *
 * Since: 2.26
 */
xvariant_t *
g_dbus_connection_call_finish (xdbus_connection_t  *connection,
                               xasync_result_t     *res,
                               xerror_t          **error)
{
  return g_dbus_connection_call_finish_internal (connection, NULL, res, error);
}

/**
 * g_dbus_connection_call_sync:
 * @connection: a #xdbus_connection_t
 * @bus_name: (nullable): a unique or well-known bus name or %NULL if
 *     @connection is not a message bus connection
 * @object_path: path of remote object
 * @interface_name: D-Bus interface to invoke method on
 * @method_name: the name of the method to invoke
 * @parameters: (nullable): a #xvariant_t tuple with parameters for the method
 *     or %NULL if not passing parameters
 * @reply_type: (nullable): the expected type of the reply, or %NULL
 * @flags: flags from the #GDBusCallFlags enumeration
 * @timeout_msec: the timeout in milliseconds, -1 to use the default
 *     timeout or %G_MAXINT for no timeout
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @error: return location for error or %NULL
 *
 * Synchronously invokes the @method_name method on the
 * @interface_name D-Bus interface on the remote object at
 * @object_path owned by @bus_name.
 *
 * If @connection is closed then the operation will fail with
 * %G_IO_ERROR_CLOSED. If @cancellable is canceled, the
 * operation will fail with %G_IO_ERROR_CANCELLED. If @parameters
 * contains a value not compatible with the D-Bus protocol, the operation
 * fails with %G_IO_ERROR_INVALID_ARGUMENT.
 *
 * If @reply_type is non-%NULL then the reply will be checked for having
 * this type and an error will be raised if it does not match.  Said
 * another way, if you give a @reply_type then any non-%NULL return
 * value will be of this type.
 *
 * If the @parameters #xvariant_t is floating, it is consumed.
 * This allows convenient 'inline' use of xvariant_new(), e.g.:
 * |[<!-- language="C" -->
 *  g_dbus_connection_call_sync (connection,
 *                               "org.freedesktop.StringThings",
 *                               "/org/freedesktop/StringThings",
 *                               "org.freedesktop.StringThings",
 *                               "TwoStrings",
 *                               xvariant_new ("(ss)",
 *                                              "Thing One",
 *                                              "Thing Two"),
 *                               NULL,
 *                               G_DBUS_CALL_FLAGS_NONE,
 *                               -1,
 *                               NULL,
 *                               &error);
 * ]|
 *
 * The calling thread is blocked until a reply is received. See
 * g_dbus_connection_call() for the asynchronous version of
 * this method.
 *
 * Returns: (transfer full): %NULL if @error is set. Otherwise a non-floating
 *     #xvariant_t tuple with return values. Free with xvariant_unref().
 *
 * Since: 2.26
 */
xvariant_t *
g_dbus_connection_call_sync (xdbus_connection_t     *connection,
                             const xchar_t         *bus_name,
                             const xchar_t         *object_path,
                             const xchar_t         *interface_name,
                             const xchar_t         *method_name,
                             xvariant_t            *parameters,
                             const xvariant_type_t  *reply_type,
                             GDBusCallFlags       flags,
                             xint_t                 timeout_msec,
                             xcancellable_t        *cancellable,
                             xerror_t             **error)
{
  return g_dbus_connection_call_sync_internal (connection, bus_name, object_path, interface_name, method_name, parameters, reply_type, flags, timeout_msec, NULL, NULL, cancellable, error);
}

/* ---------------------------------------------------------------------------------------------------- */

#ifdef G_OS_UNIX

/**
 * g_dbus_connection_call_with_unix_fd_list:
 * @connection: a #xdbus_connection_t
 * @bus_name: (nullable): a unique or well-known bus name or %NULL if
 *     @connection is not a message bus connection
 * @object_path: path of remote object
 * @interface_name: D-Bus interface to invoke method on
 * @method_name: the name of the method to invoke
 * @parameters: (nullable): a #xvariant_t tuple with parameters for the method
 *     or %NULL if not passing parameters
 * @reply_type: (nullable): the expected type of the reply, or %NULL
 * @flags: flags from the #GDBusCallFlags enumeration
 * @timeout_msec: the timeout in milliseconds, -1 to use the default
 *     timeout or %G_MAXINT for no timeout
 * @fd_list: (nullable): a #xunix_fd_list_t or %NULL
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @callback: (nullable): a #xasync_ready_callback_t to call when the request is
 *     satisfied or %NULL if you don't * care about the result of the
 *     method invocation
 * @user_data: The data to pass to @callback.
 *
 * Like g_dbus_connection_call() but also takes a #xunix_fd_list_t object.
 *
 * The file descriptors normally correspond to %G_VARIANT_TYPE_HANDLE
 * values in the body of the message. For example, if a message contains
 * two file descriptors, @fd_list would have length 2, and
 * `xvariant_new_handle (0)` and `xvariant_new_handle (1)` would appear
 * somewhere in the body of the message (not necessarily in that order!)
 * to represent the file descriptors at indexes 0 and 1 respectively.
 *
 * When designing D-Bus APIs that are intended to be interoperable,
 * please note that non-GDBus implementations of D-Bus can usually only
 * access file descriptors if they are referenced in this way by a
 * value of type %G_VARIANT_TYPE_HANDLE in the body of the message.
 *
 * This method is only available on UNIX.
 *
 * Since: 2.30
 */
void
g_dbus_connection_call_with_unix_fd_list (xdbus_connection_t     *connection,
                                          const xchar_t         *bus_name,
                                          const xchar_t         *object_path,
                                          const xchar_t         *interface_name,
                                          const xchar_t         *method_name,
                                          xvariant_t            *parameters,
                                          const xvariant_type_t  *reply_type,
                                          GDBusCallFlags       flags,
                                          xint_t                 timeout_msec,
                                          xunix_fd_list_t         *fd_list,
                                          xcancellable_t        *cancellable,
                                          xasync_ready_callback_t  callback,
                                          xpointer_t             user_data)
{
  g_dbus_connection_call_internal (connection, bus_name, object_path, interface_name, method_name, parameters, reply_type, flags, timeout_msec, fd_list, cancellable, callback, user_data);
}

/**
 * g_dbus_connection_call_with_unix_fd_list_finish:
 * @connection: a #xdbus_connection_t
 * @out_fd_list: (out) (optional): return location for a #xunix_fd_list_t or %NULL
 * @res: a #xasync_result_t obtained from the #xasync_ready_callback_t passed to
 *     g_dbus_connection_call_with_unix_fd_list()
 * @error: return location for error or %NULL
 *
 * Finishes an operation started with g_dbus_connection_call_with_unix_fd_list().
 *
 * The file descriptors normally correspond to %G_VARIANT_TYPE_HANDLE
 * values in the body of the message. For example,
 * if xvariant_get_handle() returns 5, that is intended to be a reference
 * to the file descriptor that can be accessed by
 * `g_unix_fd_list_get (*out_fd_list, 5, ...)`.
 *
 * When designing D-Bus APIs that are intended to be interoperable,
 * please note that non-GDBus implementations of D-Bus can usually only
 * access file descriptors if they are referenced in this way by a
 * value of type %G_VARIANT_TYPE_HANDLE in the body of the message.
 *
 * Returns: (transfer full): %NULL if @error is set. Otherwise a non-floating
 *     #xvariant_t tuple with return values. Free with xvariant_unref().
 *
 * Since: 2.30
 */
xvariant_t *
g_dbus_connection_call_with_unix_fd_list_finish (xdbus_connection_t  *connection,
                                                 xunix_fd_list_t     **out_fd_list,
                                                 xasync_result_t     *res,
                                                 xerror_t          **error)
{
  return g_dbus_connection_call_finish_internal (connection, out_fd_list, res, error);
}

/**
 * g_dbus_connection_call_with_unix_fd_list_sync:
 * @connection: a #xdbus_connection_t
 * @bus_name: (nullable): a unique or well-known bus name or %NULL
 *     if @connection is not a message bus connection
 * @object_path: path of remote object
 * @interface_name: D-Bus interface to invoke method on
 * @method_name: the name of the method to invoke
 * @parameters: (nullable): a #xvariant_t tuple with parameters for
 *     the method or %NULL if not passing parameters
 * @reply_type: (nullable): the expected type of the reply, or %NULL
 * @flags: flags from the #GDBusCallFlags enumeration
 * @timeout_msec: the timeout in milliseconds, -1 to use the default
 *     timeout or %G_MAXINT for no timeout
 * @fd_list: (nullable): a #xunix_fd_list_t or %NULL
 * @out_fd_list: (out) (optional): return location for a #xunix_fd_list_t or %NULL
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @error: return location for error or %NULL
 *
 * Like g_dbus_connection_call_sync() but also takes and returns #xunix_fd_list_t objects.
 * See g_dbus_connection_call_with_unix_fd_list() and
 * g_dbus_connection_call_with_unix_fd_list_finish() for more details.
 *
 * This method is only available on UNIX.
 *
 * Returns: (transfer full): %NULL if @error is set. Otherwise a non-floating
 *     #xvariant_t tuple with return values. Free with xvariant_unref().
 *
 * Since: 2.30
 */
xvariant_t *
g_dbus_connection_call_with_unix_fd_list_sync (xdbus_connection_t     *connection,
                                               const xchar_t         *bus_name,
                                               const xchar_t         *object_path,
                                               const xchar_t         *interface_name,
                                               const xchar_t         *method_name,
                                               xvariant_t            *parameters,
                                               const xvariant_type_t  *reply_type,
                                               GDBusCallFlags       flags,
                                               xint_t                 timeout_msec,
                                               xunix_fd_list_t         *fd_list,
                                               xunix_fd_list_t        **out_fd_list,
                                               xcancellable_t        *cancellable,
                                               xerror_t             **error)
{
  return g_dbus_connection_call_sync_internal (connection, bus_name, object_path, interface_name, method_name, parameters, reply_type, flags, timeout_msec, fd_list, out_fd_list, cancellable, error);
}

#endif /* G_OS_UNIX */

/* ---------------------------------------------------------------------------------------------------- */

/* called without lock held in the thread where the caller registered
 * the subtree
 */
static xboolean_t
handle_subtree_introspect (xdbus_connection_t *connection,
                           ExportedSubtree *es,
                           xdbus_message_t    *message)
{
  xstring_t *s;
  xboolean_t handled;
  xdbus_message_t *reply;
  xchar_t **children;
  xboolean_t is_root;
  const xchar_t *sender;
  const xchar_t *requested_object_path;
  const xchar_t *requested_node;
  xdbus_interface_info_t **interfaces;
  xuint_t n;
  xchar_t **subnode_paths;
  xboolean_t has_properties_interface;
  xboolean_t has_introspectable_interface;

  handled = FALSE;

  requested_object_path = xdbus_message_get_path (message);
  sender = xdbus_message_get_sender (message);
  is_root = (xstrcmp0 (requested_object_path, es->object_path) == 0);

  s = xstring_new (NULL);
  introspect_append_header (s);

  /* Strictly we don't need the children in dynamic mode, but we avoid the
   * conditionals to preserve code clarity
   */
  children = es->vtable->enumerate (es->connection,
                                    sender,
                                    es->object_path,
                                    es->user_data);

  if (!is_root)
    {
      requested_node = strrchr (requested_object_path, '/') + 1;

      /* Assert existence of object if we are not dynamic */
      if (!(es->flags & G_DBUS_SUBTREE_FLAGS_DISPATCH_TO_UNENUMERATED_NODES) &&
          !_xstrv_has_string ((const xchar_t * const *) children, requested_node))
        goto out;
    }
  else
    {
      requested_node = NULL;
    }

  interfaces = es->vtable->introspect (es->connection,
                                       sender,
                                       es->object_path,
                                       requested_node,
                                       es->user_data);
  if (interfaces != NULL)
    {
      has_properties_interface = FALSE;
      has_introspectable_interface = FALSE;

      for (n = 0; interfaces[n] != NULL; n++)
        {
          if (strcmp (interfaces[n]->name, "org.freedesktop.DBus.Properties") == 0)
            has_properties_interface = TRUE;
          else if (strcmp (interfaces[n]->name, "org.freedesktop.DBus.Introspectable") == 0)
            has_introspectable_interface = TRUE;
        }
      if (!has_properties_interface)
        xstring_append (s, introspect_properties_interface);
      if (!has_introspectable_interface)
        xstring_append (s, introspect_introspectable_interface);

      for (n = 0; interfaces[n] != NULL; n++)
        {
          g_dbus_interface_info_generate_xml (interfaces[n], 2, s);
          g_dbus_interface_info_unref (interfaces[n]);
        }
      g_free (interfaces);
    }

  /* then include <node> entries from the Subtree for the root */
  if (is_root)
    {
      for (n = 0; children != NULL && children[n] != NULL; n++)
        xstring_append_printf (s, "  <node name=\"%s\"/>\n", children[n]);
    }

  /* finally include nodes registered below us */
  subnode_paths = g_dbus_connection_list_registered (es->connection, requested_object_path);
  for (n = 0; subnode_paths != NULL && subnode_paths[n] != NULL; n++)
    xstring_append_printf (s, "  <node name=\"%s\"/>\n", subnode_paths[n]);
  xstrfreev (subnode_paths);

  xstring_append (s, "</node>\n");

  reply = xdbus_message_new_method_reply (message);
  xdbus_message_set_body (reply, xvariant_new ("(s)", s->str));
  g_dbus_connection_send_message (connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
  xobject_unref (reply);

  handled = TRUE;

 out:
  xstring_free (s, TRUE);
  xstrfreev (children);
  return handled;
}

/* called without lock held in the thread where the caller registered
 * the subtree
 */
static xboolean_t
handle_subtree_method_invocation (xdbus_connection_t *connection,
                                  ExportedSubtree *es,
                                  xdbus_message_t    *message)
{
  xboolean_t handled;
  const xchar_t *sender;
  const xchar_t *interface_name;
  const xchar_t *member;
  const xchar_t *signature;
  const xchar_t *requested_object_path;
  const xchar_t *requested_node;
  xboolean_t is_root;
  xdbus_interface_info_t *interface_info;
  const xdbus_interface_vtable_t *interface_vtable;
  xpointer_t interface_user_data;
  xuint_t n;
  xdbus_interface_info_t **interfaces;
  xboolean_t is_property_get;
  xboolean_t is_property_set;
  xboolean_t is_property_get_all;

  handled = FALSE;
  interfaces = NULL;

  requested_object_path = xdbus_message_get_path (message);
  sender = xdbus_message_get_sender (message);
  interface_name = xdbus_message_get_interface (message);
  member = xdbus_message_get_member (message);
  signature = xdbus_message_get_signature (message);
  is_root = (xstrcmp0 (requested_object_path, es->object_path) == 0);

  is_property_get = FALSE;
  is_property_set = FALSE;
  is_property_get_all = FALSE;
  if (xstrcmp0 (interface_name, "org.freedesktop.DBus.Properties") == 0)
    {
      if (xstrcmp0 (member, "Get") == 0 && xstrcmp0 (signature, "ss") == 0)
        is_property_get = TRUE;
      else if (xstrcmp0 (member, "Set") == 0 && xstrcmp0 (signature, "ssv") == 0)
        is_property_set = TRUE;
      else if (xstrcmp0 (member, "GetAll") == 0 && xstrcmp0 (signature, "s") == 0)
        is_property_get_all = TRUE;
    }

  if (!is_root)
    {
      requested_node = strrchr (requested_object_path, '/') + 1;

      if (~es->flags & G_DBUS_SUBTREE_FLAGS_DISPATCH_TO_UNENUMERATED_NODES)
        {
          /* We don't want to dispatch to unenumerated
           * nodes, so ensure that the child exists.
           */
          xchar_t **children;
          xboolean_t exists;

          children = es->vtable->enumerate (es->connection,
                                            sender,
                                            es->object_path,
                                            es->user_data);

          exists = _xstrv_has_string ((const xchar_t * const *) children, requested_node);
          xstrfreev (children);

          if (!exists)
            goto out;
        }
    }
  else
    {
      requested_node = NULL;
    }

  /* get introspection data for the node */
  interfaces = es->vtable->introspect (es->connection,
                                       sender,
                                       requested_object_path,
                                       requested_node,
                                       es->user_data);

  if (interfaces == NULL)
    goto out;

  interface_info = NULL;
  for (n = 0; interfaces[n] != NULL; n++)
    {
      if (xstrcmp0 (interfaces[n]->name, interface_name) == 0)
        interface_info = interfaces[n];
    }

  /* dispatch the call if the user wants to handle it */
  if (interface_info != NULL)
    {
      /* figure out where to dispatch the method call */
      interface_user_data = NULL;
      interface_vtable = es->vtable->dispatch (es->connection,
                                               sender,
                                               es->object_path,
                                               interface_name,
                                               requested_node,
                                               &interface_user_data,
                                               es->user_data);
      if (interface_vtable == NULL)
        goto out;

      CONNECTION_LOCK (connection);
      handled = validate_and_maybe_schedule_method_call (es->connection,
                                                         message,
                                                         0,
                                                         es->id,
                                                         interface_info,
                                                         interface_vtable,
                                                         es->context,
                                                         interface_user_data);
      CONNECTION_UNLOCK (connection);
    }
  /* handle org.freedesktop.DBus.Properties interface if not explicitly handled */
  else if (is_property_get || is_property_set || is_property_get_all)
    {
      if (is_property_get)
        xvariant_get (xdbus_message_get_body (message), "(&s&s)", &interface_name, NULL);
      else if (is_property_set)
        xvariant_get (xdbus_message_get_body (message), "(&s&sv)", &interface_name, NULL, NULL);
      else if (is_property_get_all)
        xvariant_get (xdbus_message_get_body (message), "(&s)", &interface_name, NULL, NULL);
      else
        g_assert_not_reached ();

      /* see if the object supports this interface at all */
      for (n = 0; interfaces[n] != NULL; n++)
        {
          if (xstrcmp0 (interfaces[n]->name, interface_name) == 0)
            interface_info = interfaces[n];
        }

      /* Fail with org.freedesktop.DBus.Error.InvalidArgs if the user-code
       * claims it won't support the interface
       */
      if (interface_info == NULL)
        {
          xdbus_message_t *reply;
          reply = xdbus_message_new_method_error (message,
                                                   "org.freedesktop.DBus.Error.InvalidArgs",
                                                   _("No such interface “%s”"),
                                                   interface_name);
          g_dbus_connection_send_message (es->connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
          xobject_unref (reply);
          handled = TRUE;
          goto out;
        }

      /* figure out where to dispatch the property get/set/getall calls */
      interface_user_data = NULL;
      interface_vtable = es->vtable->dispatch (es->connection,
                                               sender,
                                               es->object_path,
                                               interface_name,
                                               requested_node,
                                               &interface_user_data,
                                               es->user_data);
      if (interface_vtable == NULL)
        {
          g_warning ("The subtree introspection function indicates that '%s' "
                     "is a valid interface name, but calling the dispatch "
                     "function on that interface gave us NULL", interface_name);
          goto out;
        }

      if (is_property_get || is_property_set)
        {
          CONNECTION_LOCK (connection);
          handled = validate_and_maybe_schedule_property_getset (es->connection,
                                                                 message,
                                                                 0,
                                                                 es->id,
                                                                 is_property_get,
                                                                 interface_info,
                                                                 interface_vtable,
                                                                 es->context,
                                                                 interface_user_data);
          CONNECTION_UNLOCK (connection);
        }
      else if (is_property_get_all)
        {
          CONNECTION_LOCK (connection);
          handled = validate_and_maybe_schedule_property_get_all (es->connection,
                                                                  message,
                                                                  0,
                                                                  es->id,
                                                                  interface_info,
                                                                  interface_vtable,
                                                                  es->context,
                                                                  interface_user_data);
          CONNECTION_UNLOCK (connection);
        }
    }

 out:
  if (interfaces != NULL)
    {
      for (n = 0; interfaces[n] != NULL; n++)
        g_dbus_interface_info_unref (interfaces[n]);
      g_free (interfaces);
    }

  return handled;
}

typedef struct
{
  xdbus_message_t *message;  /* (owned) */
  ExportedSubtree *es;  /* (owned) */
} SubtreeDeferredData;

static void
subtree_deferred_data_free (SubtreeDeferredData *data)
{
  xobject_unref (data->message);
  exported_subtree_unref (data->es);
  g_free (data);
}

/* called without lock held in the thread where the caller registered the subtree */
static xboolean_t
process_subtree_vtable_message_in_idle_cb (xpointer_t _data)
{
  SubtreeDeferredData *data = _data;
  xboolean_t handled;

  handled = FALSE;

  if (xstrcmp0 (xdbus_message_get_interface (data->message), "org.freedesktop.DBus.Introspectable") == 0 &&
      xstrcmp0 (xdbus_message_get_member (data->message), "Introspect") == 0 &&
      xstrcmp0 (xdbus_message_get_signature (data->message), "") == 0)
    handled = handle_subtree_introspect (data->es->connection,
                                         data->es,
                                         data->message);
  else
    handled = handle_subtree_method_invocation (data->es->connection,
                                                data->es,
                                                data->message);

  if (!handled)
    {
      CONNECTION_LOCK (data->es->connection);
      handled = handle_generic_unlocked (data->es->connection, data->message);
      CONNECTION_UNLOCK (data->es->connection);
    }

  /* if we couldn't handle the request, just bail with the UnknownMethod error */
  if (!handled)
    {
      xdbus_message_t *reply;
      reply = xdbus_message_new_method_error (data->message,
                                               "org.freedesktop.DBus.Error.UnknownMethod",
                                               _("Method “%s” on interface “%s” with signature “%s” does not exist"),
                                               xdbus_message_get_member (data->message),
                                               xdbus_message_get_interface (data->message),
                                               xdbus_message_get_signature (data->message));
      g_dbus_connection_send_message (data->es->connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
      xobject_unref (reply);
    }

  return FALSE;
}

/* called in GDBusWorker thread with connection's lock held */
static xboolean_t
subtree_message_func (xdbus_connection_t *connection,
                      ExportedSubtree *es,
                      xdbus_message_t    *message)
{
  xsource_t *idle_source;
  SubtreeDeferredData *data;

  data = g_new0 (SubtreeDeferredData, 1);
  data->message = xobject_ref (message);
  data->es = exported_subtree_ref (es);

  /* defer this call to an idle handler in the right thread */
  idle_source = g_idle_source_new ();
  xsource_set_priority (idle_source, G_PRIORITY_HIGH);
  xsource_set_callback (idle_source,
                         process_subtree_vtable_message_in_idle_cb,
                         data,
                         (xdestroy_notify_t) subtree_deferred_data_free);
  xsource_set_static_name (idle_source, "[gio] process_subtree_vtable_message_in_idle_cb");
  xsource_attach (idle_source, es->context);
  xsource_unref (idle_source);

  /* since we own the entire subtree, handlers for objects not in the subtree have been
   * tried already by libdbus-1 - so we just need to ensure that we're always going
   * to reply to the message
   */
  return TRUE;
}

/**
 * g_dbus_connection_register_subtree:
 * @connection: a #xdbus_connection_t
 * @object_path: the object path to register the subtree at
 * @vtable: a #xdbus_subtree_vtable_t to enumerate, introspect and
 *     dispatch nodes in the subtree
 * @flags: flags used to fine tune the behavior of the subtree
 * @user_data: data to pass to functions in @vtable
 * @user_data_free_func: function to call when the subtree is unregistered
 * @error: return location for error or %NULL
 *
 * Registers a whole subtree of dynamic objects.
 *
 * The @enumerate and @introspection functions in @vtable are used to
 * convey, to remote callers, what nodes exist in the subtree rooted
 * by @object_path.
 *
 * When handling remote calls into any node in the subtree, first the
 * @enumerate function is used to check if the node exists. If the node exists
 * or the %G_DBUS_SUBTREE_FLAGS_DISPATCH_TO_UNENUMERATED_NODES flag is set
 * the @introspection function is used to check if the node supports the
 * requested method. If so, the @dispatch function is used to determine
 * where to dispatch the call. The collected #xdbus_interface_vtable_t and
 * #xpointer_t will be used to call into the interface vtable for processing
 * the request.
 *
 * All calls into user-provided code will be invoked in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread you are calling this method from.
 *
 * If an existing subtree is already registered at @object_path or
 * then @error is set to %G_IO_ERROR_EXISTS.
 *
 * Note that it is valid to register regular objects (using
 * g_dbus_connection_register_object()) in a subtree registered with
 * g_dbus_connection_register_subtree() - if so, the subtree handler
 * is tried as the last resort. One way to think about a subtree
 * handler is to consider it a fallback handler for object paths not
 * registered via g_dbus_connection_register_object() or other bindings.
 *
 * Note that @vtable will be copied so you cannot change it after
 * registration.
 *
 * See this [server][gdbus-subtree-server] for an example of how to use
 * this method.
 *
 * Returns: 0 if @error is set, otherwise a subtree registration ID (never 0)
 * that can be used with g_dbus_connection_unregister_subtree()
 *
 * Since: 2.26
 */
xuint_t
g_dbus_connection_register_subtree (xdbus_connection_t           *connection,
                                    const xchar_t               *object_path,
                                    const xdbus_subtree_vtable_t  *vtable,
                                    GDBusSubtreeFlags          flags,
                                    xpointer_t                   user_data,
                                    xdestroy_notify_t             user_data_free_func,
                                    xerror_t                   **error)
{
  xuint_t ret;
  ExportedSubtree *es;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), 0);
  g_return_val_if_fail (object_path != NULL && xvariant_is_object_path (object_path), 0);
  g_return_val_if_fail (vtable != NULL, 0);
  g_return_val_if_fail (error == NULL || *error == NULL, 0);
  g_return_val_if_fail (check_initialized (connection), 0);

  ret = 0;

  CONNECTION_LOCK (connection);

  es = xhash_table_lookup (connection->map_object_path_to_es, object_path);
  if (es != NULL)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_EXISTS,
                   _("A subtree is already exported for %s"),
                   object_path);
      goto out;
    }

  es = g_new0 (ExportedSubtree, 1);
  es->refcount = 1;
  es->object_path = xstrdup (object_path);
  es->connection = connection;

  es->vtable = _g_dbus_subtree_vtable_copy (vtable);
  es->flags = flags;
  es->id = (xuint_t) g_atomic_int_add (&_global_subtree_registration_id, 1); /* TODO: overflow etc. */
  es->user_data = user_data;
  es->user_data_free_func = user_data_free_func;
  es->context = xmain_context_ref_thread_default ();

  xhash_table_insert (connection->map_object_path_to_es, es->object_path, es);
  xhash_table_insert (connection->map_id_to_es,
                       GUINT_TO_POINTER (es->id),
                       es);

  ret = es->id;

 out:
  CONNECTION_UNLOCK (connection);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_connection_unregister_subtree:
 * @connection: a #xdbus_connection_t
 * @registration_id: a subtree registration id obtained from
 *     g_dbus_connection_register_subtree()
 *
 * Unregisters a subtree.
 *
 * Returns: %TRUE if the subtree was unregistered, %FALSE otherwise
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_connection_unregister_subtree (xdbus_connection_t *connection,
                                      xuint_t            registration_id)
{
  ExportedSubtree *es;
  xboolean_t ret;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);
  g_return_val_if_fail (check_initialized (connection), FALSE);

  ret = FALSE;

  CONNECTION_LOCK (connection);

  es = xhash_table_lookup (connection->map_id_to_es,
                            GUINT_TO_POINTER (registration_id));
  if (es == NULL)
    goto out;

  g_warn_if_fail (xhash_table_remove (connection->map_id_to_es, GUINT_TO_POINTER (es->id)));
  g_warn_if_fail (xhash_table_remove (connection->map_object_path_to_es, es->object_path));

  ret = TRUE;

 out:
  CONNECTION_UNLOCK (connection);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/* may be called in any thread, with connection's lock held */
static void
handle_generic_ping_unlocked (xdbus_connection_t *connection,
                              const xchar_t     *object_path,
                              xdbus_message_t    *message)
{
  xdbus_message_t *reply;
  reply = xdbus_message_new_method_reply (message);
  g_dbus_connection_send_message_unlocked (connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
  xobject_unref (reply);
}

/* may be called in any thread, with connection's lock held */
static void
handle_generic_get_machine_id_unlocked (xdbus_connection_t *connection,
                                        const xchar_t     *object_path,
                                        xdbus_message_t    *message)
{
  xdbus_message_t *reply;

  reply = NULL;
  if (connection->machine_id == NULL)
    {
      xerror_t *error;

      error = NULL;
      connection->machine_id = _g_dbus_get_machine_id (&error);
      if (connection->machine_id == NULL)
        {
          reply = xdbus_message_new_method_error_literal (message,
                                                           "org.freedesktop.DBus.Error.Failed",
                                                           error->message);
          xerror_free (error);
        }
    }

  if (reply == NULL)
    {
      reply = xdbus_message_new_method_reply (message);
      xdbus_message_set_body (reply, xvariant_new ("(s)", connection->machine_id));
    }
  g_dbus_connection_send_message_unlocked (connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
  xobject_unref (reply);
}

/* may be called in any thread, with connection's lock held */
static void
handle_generic_introspect_unlocked (xdbus_connection_t *connection,
                                    const xchar_t     *object_path,
                                    xdbus_message_t    *message)
{
  xuint_t n;
  xstring_t *s;
  xchar_t **registered;
  xdbus_message_t *reply;

  /* first the header */
  s = xstring_new (NULL);
  introspect_append_header (s);

  registered = g_dbus_connection_list_registered_unlocked (connection, object_path);
  for (n = 0; registered != NULL && registered[n] != NULL; n++)
      xstring_append_printf (s, "  <node name=\"%s\"/>\n", registered[n]);
  xstrfreev (registered);
  xstring_append (s, "</node>\n");

  reply = xdbus_message_new_method_reply (message);
  xdbus_message_set_body (reply, xvariant_new ("(s)", s->str));
  g_dbus_connection_send_message_unlocked (connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
  xobject_unref (reply);
  xstring_free (s, TRUE);
}

/* may be called in any thread, with connection's lock held */
static xboolean_t
handle_generic_unlocked (xdbus_connection_t *connection,
                         xdbus_message_t    *message)
{
  xboolean_t handled;
  const xchar_t *interface_name;
  const xchar_t *member;
  const xchar_t *signature;
  const xchar_t *path;

  CONNECTION_ENSURE_LOCK (connection);

  handled = FALSE;

  interface_name = xdbus_message_get_interface (message);
  member = xdbus_message_get_member (message);
  signature = xdbus_message_get_signature (message);
  path = xdbus_message_get_path (message);

  if (xstrcmp0 (interface_name, "org.freedesktop.DBus.Introspectable") == 0 &&
      xstrcmp0 (member, "Introspect") == 0 &&
      xstrcmp0 (signature, "") == 0)
    {
      handle_generic_introspect_unlocked (connection, path, message);
      handled = TRUE;
    }
  else if (xstrcmp0 (interface_name, "org.freedesktop.DBus.Peer") == 0 &&
           xstrcmp0 (member, "Ping") == 0 &&
           xstrcmp0 (signature, "") == 0)
    {
      handle_generic_ping_unlocked (connection, path, message);
      handled = TRUE;
    }
  else if (xstrcmp0 (interface_name, "org.freedesktop.DBus.Peer") == 0 &&
           xstrcmp0 (member, "GetMachineId") == 0 &&
           xstrcmp0 (signature, "") == 0)
    {
      handle_generic_get_machine_id_unlocked (connection, path, message);
      handled = TRUE;
    }

  return handled;
}

/* ---------------------------------------------------------------------------------------------------- */

/* called in GDBusWorker thread with connection's lock held */
static void
distribute_method_call (xdbus_connection_t *connection,
                        xdbus_message_t    *message)
{
  xdbus_message_t *reply;
  ExportedObject *eo;
  ExportedSubtree *es;
  const xchar_t *object_path;
  const xchar_t *interface_name;
  const xchar_t *member;
  const xchar_t *path;
  xchar_t *subtree_path;
  xchar_t *needle;
  xboolean_t object_found = FALSE;

  g_assert (xdbus_message_get_message_type (message) == G_DBUS_MESSAGE_TYPE_METHOD_CALL);

  interface_name = xdbus_message_get_interface (message);
  member = xdbus_message_get_member (message);
  path = xdbus_message_get_path (message);
  subtree_path = xstrdup (path);
  needle = strrchr (subtree_path, '/');
  if (needle != NULL && needle != subtree_path)
    {
      *needle = '\0';
    }
  else
    {
      g_free (subtree_path);
      subtree_path = NULL;
    }


  if (G_UNLIKELY (_g_dbus_debug_incoming ()))
    {
      _g_dbus_debug_print_lock ();
      g_print ("========================================================================\n"
               "GDBus-debug:Incoming:\n"
               " <<<< METHOD INVOCATION %s.%s()\n"
               "      on object %s\n"
               "      invoked by name %s\n"
               "      serial %d\n",
               interface_name, member,
               path,
               xdbus_message_get_sender (message) != NULL ? xdbus_message_get_sender (message) : "(none)",
               xdbus_message_get_serial (message));
      _g_dbus_debug_print_unlock ();
    }

  object_path = xdbus_message_get_path (message);
  g_assert (object_path != NULL);

  eo = xhash_table_lookup (connection->map_object_path_to_eo, object_path);
  if (eo != NULL)
    {
      if (obj_message_func (connection, eo, message, &object_found))
        goto out;
    }

  es = xhash_table_lookup (connection->map_object_path_to_es, object_path);
  if (es != NULL)
    {
      if (subtree_message_func (connection, es, message))
        goto out;
    }

  if (subtree_path != NULL)
    {
      es = xhash_table_lookup (connection->map_object_path_to_es, subtree_path);
      if (es != NULL)
        {
          if (subtree_message_func (connection, es, message))
            goto out;
        }
    }

  if (handle_generic_unlocked (connection, message))
    goto out;

  /* if we end up here, the message has not been not handled - so return an error saying this */
  if (object_found == TRUE)
    {
      reply = xdbus_message_new_method_error (message,
                                               "org.freedesktop.DBus.Error.UnknownMethod",
                                               _("No such interface “%s” on object at path %s"),
                                               interface_name,
                                               object_path);
    }
  else
    {
      reply = xdbus_message_new_method_error (message,
                                           "org.freedesktop.DBus.Error.UnknownMethod",
                                           _("Object does not exist at path “%s”"),
                                           object_path);
    }

  g_dbus_connection_send_message_unlocked (connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
  xobject_unref (reply);

 out:
  g_free (subtree_path);
}

/* ---------------------------------------------------------------------------------------------------- */

/* Called in any user thread, with the message_bus_lock held. */
static GWeakRef *
message_bus_get_singleton (GBusType   bus_type,
                           xerror_t   **error)
{
  GWeakRef *ret;
  const xchar_t *starter_bus;

  ret = NULL;

  switch (bus_type)
    {
    case G_BUS_TYPE_SESSION:
      ret = &the_session_bus;
      break;

    case G_BUS_TYPE_SYSTEM:
      ret = &the_system_bus;
      break;

    case G_BUS_TYPE_STARTER:
      starter_bus = g_getenv ("DBUS_STARTER_BUS_TYPE");
      if (xstrcmp0 (starter_bus, "session") == 0)
        {
          ret = message_bus_get_singleton (G_BUS_TYPE_SESSION, error);
          goto out;
        }
      else if (xstrcmp0 (starter_bus, "system") == 0)
        {
          ret = message_bus_get_singleton (G_BUS_TYPE_SYSTEM, error);
          goto out;
        }
      else
        {
          if (starter_bus != NULL)
            {
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Cannot determine bus address from DBUS_STARTER_BUS_TYPE environment variable"
                             " — unknown value “%s”"),
                           starter_bus);
            }
          else
            {
              g_set_error_literal (error,
                                   G_IO_ERROR,
                                   G_IO_ERROR_INVALID_ARGUMENT,
                                   _("Cannot determine bus address because the DBUS_STARTER_BUS_TYPE environment "
                                     "variable is not set"));
            }
        }
      break;

    default:
      g_assert_not_reached ();
      break;
    }

 out:
  return ret;
}

/* Called in any user thread, without holding locks. */
static xdbus_connection_t *
get_uninitialized_connection (GBusType       bus_type,
                              xcancellable_t  *cancellable,
                              xerror_t       **error)
{
  GWeakRef *singleton;
  xdbus_connection_t *ret;

  ret = NULL;

  G_LOCK (message_bus_lock);
  singleton = message_bus_get_singleton (bus_type, error);
  if (singleton == NULL)
    goto out;

  ret = g_weak_ref_get (singleton);

  if (ret == NULL)
    {
      xchar_t *address;
      address = g_dbus_address_get_for_bus_sync (bus_type, cancellable, error);
      if (address == NULL)
        goto out;
      ret = xobject_new (XTYPE_DBUS_CONNECTION,
                          "address", address,
                          "flags", G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                   G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                          "exit-on-close", TRUE,
                          NULL);

      g_weak_ref_set (singleton, ret);
      g_free (address);
    }

  g_assert (ret != NULL);

 out:
  G_UNLOCK (message_bus_lock);
  return ret;
}

/* May be called from any thread. Must not hold message_bus_lock. */
xdbus_connection_t *
_g_bus_get_singleton_if_exists (GBusType bus_type)
{
  GWeakRef *singleton;
  xdbus_connection_t *ret = NULL;

  G_LOCK (message_bus_lock);
  singleton = message_bus_get_singleton (bus_type, NULL);
  if (singleton == NULL)
    goto out;

  ret = g_weak_ref_get (singleton);

 out:
  G_UNLOCK (message_bus_lock);
  return ret;
}

/* May be called from any thread. Must not hold message_bus_lock. */
void
_g_bus_forget_singleton (GBusType bus_type)
{
  GWeakRef *singleton;

  G_LOCK (message_bus_lock);

  singleton = message_bus_get_singleton (bus_type, NULL);

  if (singleton != NULL)
    g_weak_ref_set (singleton, NULL);

  G_UNLOCK (message_bus_lock);
}

/**
 * g_bus_get_sync:
 * @bus_type: a #GBusType
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @error: return location for error or %NULL
 *
 * Synchronously connects to the message bus specified by @bus_type.
 * Note that the returned object may shared with other callers,
 * e.g. if two separate parts of a process calls this function with
 * the same @bus_type, they will share the same object.
 *
 * This is a synchronous failable function. See g_bus_get() and
 * g_bus_get_finish() for the asynchronous version.
 *
 * The returned object is a singleton, that is, shared with other
 * callers of g_bus_get() and g_bus_get_sync() for @bus_type. In the
 * event that you need a private message bus connection, use
 * g_dbus_address_get_for_bus_sync() and
 * g_dbus_connection_new_for_address() with
 * G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT and
 * G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION flags.
 *
 * Note that the returned #xdbus_connection_t object will (usually) have
 * the #xdbus_connection_t:exit-on-close property set to %TRUE.
 *
 * Returns: (transfer full): a #xdbus_connection_t or %NULL if @error is set.
 *     Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_connection_t *
g_bus_get_sync (GBusType       bus_type,
                xcancellable_t  *cancellable,
                xerror_t       **error)
{
  xdbus_connection_t *connection;

  _g_dbus_initialize ();

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  connection = get_uninitialized_connection (bus_type, cancellable, error);
  if (connection == NULL)
    goto out;

  if (!xinitable_init (XINITABLE (connection), cancellable, error))
    {
      xobject_unref (connection);
      connection = NULL;
    }

 out:
  return connection;
}

static void
bus_get_async_initable_cb (xobject_t      *source_object,
                           xasync_result_t *res,
                           xpointer_t      user_data)
{
  xtask_t *task = user_data;
  xerror_t *error = NULL;

  if (!xasync_initable_init_finish (XASYNC_INITABLE (source_object),
                                     res,
                                     &error))
    {
      g_assert (error != NULL);
      xtask_return_error (task, error);
      xobject_unref (source_object);
    }
  else
    {
      xtask_return_pointer (task, source_object, xobject_unref);
    }
  xobject_unref (task);
}

/**
 * g_bus_get:
 * @bus_type: a #GBusType
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @callback: a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: the data to pass to @callback
 *
 * Asynchronously connects to the message bus specified by @bus_type.
 *
 * When the operation is finished, @callback will be invoked. You can
 * then call g_bus_get_finish() to get the result of the operation.
 *
 * This is an asynchronous failable function. See g_bus_get_sync() for
 * the synchronous version.
 *
 * Since: 2.26
 */
void
g_bus_get (GBusType             bus_type,
           xcancellable_t        *cancellable,
           xasync_ready_callback_t  callback,
           xpointer_t             user_data)
{
  xdbus_connection_t *connection;
  xtask_t *task;
  xerror_t *error = NULL;

  _g_dbus_initialize ();

  task = xtask_new (NULL, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_bus_get);

  connection = get_uninitialized_connection (bus_type, cancellable, &error);
  if (connection == NULL)
    {
      g_assert (error != NULL);
      xtask_return_error (task, error);
      xobject_unref (task);
    }
  else
    {
      xasync_initable_init_async (XASYNC_INITABLE (connection),
                                   G_PRIORITY_DEFAULT,
                                   cancellable,
                                   bus_get_async_initable_cb,
                                   task);
    }
}

/**
 * g_bus_get_finish:
 * @res: a #xasync_result_t obtained from the #xasync_ready_callback_t passed
 *     to g_bus_get()
 * @error: return location for error or %NULL
 *
 * Finishes an operation started with g_bus_get().
 *
 * The returned object is a singleton, that is, shared with other
 * callers of g_bus_get() and g_bus_get_sync() for @bus_type. In the
 * event that you need a private message bus connection, use
 * g_dbus_address_get_for_bus_sync() and
 * g_dbus_connection_new_for_address() with
 * G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT and
 * G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION flags.
 *
 * Note that the returned #xdbus_connection_t object will (usually) have
 * the #xdbus_connection_t:exit-on-close property set to %TRUE.
 *
 * Returns: (transfer full): a #xdbus_connection_t or %NULL if @error is set.
 *     Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_connection_t *
g_bus_get_finish (xasync_result_t  *res,
                  xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, NULL), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

/* ---------------------------------------------------------------------------------------------------- */
