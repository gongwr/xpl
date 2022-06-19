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

#include "gdbusobjectmanager.h"
#include "gdbusobjectmanagerserver.h"
#include "gdbusobject.h"
#include "gdbusobjectskeleton.h"
#include "gdbusinterfaceskeleton.h"
#include "gdbusconnection.h"
#include "gdbusintrospection.h"
#include "gdbusmethodinvocation.h"
#include "gdbuserror.h"

#include "gioerror.h"

#include "glibintl.h"

/**
 * SECTION:gdbusobjectmanagerserver
 * @short_description: Service-side object manager
 * @include: gio/gio.h
 *
 * #xdbus_object_manager_server_t is used to export #xdbus_object_t instances using
 * the standardized
 * [org.freedesktop.DBus.ObjectManager](http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager)
 * interface. For example, remote D-Bus clients can get all objects
 * and properties in a single call. Additionally, any change in the
 * object hierarchy is broadcast using signals. This means that D-Bus
 * clients can keep caches up to date by only listening to D-Bus
 * signals.
 *
 * The recommended path to export an object manager at is the path form of the
 * well-known name of a D-Bus service, or below. For example, if a D-Bus service
 * is available at the well-known name `net.example.ExampleService1`, the object
 * manager should typically be exported at `/net/example/ExampleService1`, or
 * below (to allow for multiple object managers in a service).
 *
 * It is supported, but not recommended, to export an object manager at the root
 * path, `/`.
 *
 * See #xdbus_object_manager_client_t for the client-side code that is
 * intended to be used with #xdbus_object_manager_server_t or any D-Bus
 * object implementing the org.freedesktop.DBus.ObjectManager
 * interface.
 */

typedef struct
{
  xdbus_object_skeleton_t *object;
  xdbus_object_manager_server_t *manager;
  xhashtable_t *map_iface_name_to_iface;
  xboolean_t exported;
} registration_data_t;

static void registration_data_free (registration_data_t *data);

static void export_all (xdbus_object_manager_server_t *manager);
static void unexport_all (xdbus_object_manager_server_t *manager, xboolean_t only_manager);

static void xdbus_object_manager_server_emit_interfaces_added (xdbus_object_manager_server_t *manager,
                                                         registration_data_t   *data,
                                                         const xchar_t *const *interfaces,
                                                         const xchar_t *object_path);

static void xdbus_object_manager_server_emit_interfaces_removed (xdbus_object_manager_server_t *manager,
                                                           registration_data_t   *data,
                                                           const xchar_t *const *interfaces);

static xboolean_t xdbus_object_manager_server_unexport_unlocked (xdbus_object_manager_server_t  *manager,
                                                                const xchar_t               *object_path);

struct _xdbus_object_manager_server_private
{
  xmutex_t lock;
  xdbus_connection_t *connection;
  xchar_t *object_path;
  xchar_t *object_path_ending_in_slash;
  xhashtable_t *map_object_path_to_data;
  xuint_t manager_reg_id;
};

enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_OBJECT_PATH
};

static void dbus_object_manager_interface_init (xdbus_object_manager_iface_t *iface);

G_DEFINE_TYPE_WITH_CODE (xdbus_object_manager_server, xdbus_object_manager_server, XTYPE_OBJECT,
                         G_ADD_PRIVATE (xdbus_object_manager_server)
                         G_IMPLEMENT_INTERFACE (XTYPE_DBUS_OBJECT_MANAGER, dbus_object_manager_interface_init))

static void xdbus_object_manager_server_constructed (xobject_t *object);

static void
xdbus_object_manager_server_finalize (xobject_t *object)
{
  xdbus_object_manager_server_t *manager = G_DBUS_OBJECT_MANAGER_SERVER (object);

  if (manager->priv->connection != NULL)
    {
      unexport_all (manager, TRUE);
      xobject_unref (manager->priv->connection);
    }
  xhash_table_unref (manager->priv->map_object_path_to_data);
  g_free (manager->priv->object_path);
  g_free (manager->priv->object_path_ending_in_slash);

  g_mutex_clear (&manager->priv->lock);

  if (XOBJECT_CLASS (xdbus_object_manager_server_parent_class)->finalize != NULL)
    XOBJECT_CLASS (xdbus_object_manager_server_parent_class)->finalize (object);
}

static void
xdbus_object_manager_server_get_property (xobject_t    *object,
                                           xuint_t       prop_id,
                                           xvalue_t     *value,
                                           xparam_spec_t *pspec)
{
  xdbus_object_manager_server_t *manager = G_DBUS_OBJECT_MANAGER_SERVER (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_mutex_lock (&manager->priv->lock);
      xvalue_set_object (value, manager->priv->connection);
      g_mutex_unlock (&manager->priv->lock);
      break;

    case PROP_OBJECT_PATH:
      xvalue_set_string (value, g_dbus_object_manager_get_object_path (G_DBUS_OBJECT_MANAGER (manager)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xdbus_object_manager_server_set_property (xobject_t       *object,
                                           xuint_t          prop_id,
                                           const xvalue_t  *value,
                                           xparam_spec_t    *pspec)
{
  xdbus_object_manager_server_t *manager = G_DBUS_OBJECT_MANAGER_SERVER (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      xdbus_object_manager_server_set_connection (manager, xvalue_get_object (value));
      break;

    case PROP_OBJECT_PATH:
      xassert (manager->priv->object_path == NULL);
      xassert (xvariant_is_object_path (xvalue_get_string (value)));
      manager->priv->object_path = xvalue_dup_string (value);
      if (xstr_equal (manager->priv->object_path, "/"))
        manager->priv->object_path_ending_in_slash = xstrdup (manager->priv->object_path);
      else
        manager->priv->object_path_ending_in_slash = xstrdup_printf ("%s/", manager->priv->object_path);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xdbus_object_manager_server_class_init (GDBusObjectManagerServerClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->finalize     = xdbus_object_manager_server_finalize;
  xobject_class->constructed  = xdbus_object_manager_server_constructed;
  xobject_class->set_property = xdbus_object_manager_server_set_property;
  xobject_class->get_property = xdbus_object_manager_server_get_property;

  /**
   * xdbus_object_manager_server_t:connection:
   *
   * The #xdbus_connection_t to export objects on.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_CONNECTION,
                                   xparam_spec_object ("connection",
                                                        "Connection",
                                                        "The connection to export objects on",
                                                        XTYPE_DBUS_CONNECTION,
                                                        XPARAM_READABLE |
                                                        XPARAM_WRITABLE |
                                                        XPARAM_STATIC_STRINGS));

  /**
   * xdbus_object_manager_server_t:object-path:
   *
   * The object path to register the manager object at.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_OBJECT_PATH,
                                   xparam_spec_string ("object-path",
                                                        "Object Path",
                                                        "The object path to register the manager object at",
                                                        NULL,
                                                        XPARAM_READABLE |
                                                        XPARAM_WRITABLE |
                                                        XPARAM_CONSTRUCT_ONLY |
                                                        XPARAM_STATIC_STRINGS));
}

static void
xdbus_object_manager_server_init (xdbus_object_manager_server_t *manager)
{
  manager->priv = xdbus_object_manager_server_get_instance_private (manager);
  g_mutex_init (&manager->priv->lock);
  manager->priv->map_object_path_to_data = xhash_table_new_full (xstr_hash,
                                                                  xstr_equal,
                                                                  g_free,
                                                                  (xdestroy_notify_t) registration_data_free);
}

/**
 * xdbus_object_manager_server_new:
 * @object_path: The object path to export the manager object at.
 *
 * Creates a new #xdbus_object_manager_server_t object.
 *
 * The returned server isn't yet exported on any connection. To do so,
 * use xdbus_object_manager_server_set_connection(). Normally you
 * want to export all of your objects before doing so to avoid
 * [InterfacesAdded](http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager)
 * signals being emitted.
 *
 * Returns: A #xdbus_object_manager_server_t object. Free with xobject_unref().
 *
 * Since: 2.30
 */
xdbus_object_manager_server_t *
xdbus_object_manager_server_new (const xchar_t     *object_path)
{
  xreturn_val_if_fail (xvariant_is_object_path (object_path), NULL);
  return G_DBUS_OBJECT_MANAGER_SERVER (xobject_new (XTYPE_DBUS_OBJECT_MANAGER_SERVER,
                                                     "object-path", object_path,
                                                     NULL));
}

/**
 * xdbus_object_manager_server_set_connection:
 * @manager: A #xdbus_object_manager_server_t.
 * @connection: (nullable): A #xdbus_connection_t or %NULL.
 *
 * Exports all objects managed by @manager on @connection. If
 * @connection is %NULL, stops exporting objects.
 */
void
xdbus_object_manager_server_set_connection (xdbus_object_manager_server_t  *manager,
                                             xdbus_connection_t           *connection)
{
  g_return_if_fail (X_IS_DBUS_OBJECT_MANAGER_SERVER (manager));
  g_return_if_fail (connection == NULL || X_IS_DBUS_CONNECTION (connection));

  g_mutex_lock (&manager->priv->lock);

  if (manager->priv->connection == connection)
    {
      g_mutex_unlock (&manager->priv->lock);
      goto out;
    }

  if (manager->priv->connection != NULL)
    {
      unexport_all (manager, FALSE);
      xobject_unref (manager->priv->connection);
      manager->priv->connection = NULL;
    }

  manager->priv->connection = connection != NULL ? xobject_ref (connection) : NULL;
  if (manager->priv->connection != NULL)
    export_all (manager);

  g_mutex_unlock (&manager->priv->lock);

  xobject_notify (G_OBJECT (manager), "connection");
 out:
  ;
}

/**
 * xdbus_object_manager_server_get_connection:
 * @manager: A #xdbus_object_manager_server_t
 *
 * Gets the #xdbus_connection_t used by @manager.
 *
 * Returns: (transfer full) (nullable): A #xdbus_connection_t object or %NULL if
 *   @manager isn't exported on a connection. The returned object should
 *   be freed with xobject_unref().
 *
 * Since: 2.30
 */
xdbus_connection_t *
xdbus_object_manager_server_get_connection (xdbus_object_manager_server_t *manager)
{
  xdbus_connection_t *ret;
  xreturn_val_if_fail (X_IS_DBUS_OBJECT_MANAGER_SERVER (manager), NULL);
  g_mutex_lock (&manager->priv->lock);
  ret = manager->priv->connection != NULL ? xobject_ref (manager->priv->connection) : NULL;
  g_mutex_unlock (&manager->priv->lock);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
registration_data_export_interface (registration_data_t        *data,
                                    xdbus_interface_skeleton_t  *interface_skeleton,
                                    const xchar_t             *object_path)
{
  xdbus_interface_info_t *info;
  xerror_t *error;

  info = g_dbus_interface_skeleton_get_info (interface_skeleton);
  error = NULL;
  if (data->manager->priv->connection != NULL)
    {
      if (!g_dbus_interface_skeleton_export (interface_skeleton,
                                             data->manager->priv->connection,
                                             object_path,
                                             &error))
        {
          g_warning ("%s: Error registering object at %s with interface %s: %s",
                     G_STRLOC,
                     object_path,
                     info->name,
                     error->message);
          xerror_free (error);
        }
    }

  xassert (xhash_table_lookup (data->map_iface_name_to_iface, info->name) == NULL);
  xhash_table_insert (data->map_iface_name_to_iface,
                       info->name,
                       xobject_ref (interface_skeleton));

  /* if we are already exported, then... */
  if (data->exported)
    {
      const xchar_t *interfaces[2];
      /* emit InterfacesAdded on the ObjectManager object */
      interfaces[0] = info->name;
      interfaces[1] = NULL;
      xdbus_object_manager_server_emit_interfaces_added (data->manager, data, interfaces, object_path);
    }
}

static void
registration_data_unexport_interface (registration_data_t       *data,
                                      xdbus_interface_skeleton_t *interface_skeleton)
{
  xdbus_interface_info_t *info;
  xdbus_interface_skeleton_t *iface;

  info = g_dbus_interface_skeleton_get_info (interface_skeleton);
  iface = xhash_table_lookup (data->map_iface_name_to_iface, info->name);
  xassert (iface != NULL);

  if (data->manager->priv->connection != NULL)
    g_dbus_interface_skeleton_unexport (iface);

  g_warn_if_fail (xhash_table_remove (data->map_iface_name_to_iface, info->name));

  /* if we are already exported, then... */
  if (data->exported)
    {
      const xchar_t *interfaces[2];
      /* emit InterfacesRemoved on the ObjectManager object */
      interfaces[0] = info->name;
      interfaces[1] = NULL;
      xdbus_object_manager_server_emit_interfaces_removed (data->manager, data, interfaces);
    }
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_interface_added (xdbus_object_t    *object,
                    xdbus_interface_t *interface,
                    xpointer_t        user_data)
{
  registration_data_t *data = user_data;
  const xchar_t *object_path;
  g_mutex_lock (&data->manager->priv->lock);
  object_path = g_dbus_object_get_object_path (G_DBUS_OBJECT (data->object));
  registration_data_export_interface (data, G_DBUS_INTERFACE_SKELETON (interface), object_path);
  g_mutex_unlock (&data->manager->priv->lock);
}

static void
on_interface_removed (xdbus_object_t    *object,
                      xdbus_interface_t *interface,
                      xpointer_t        user_data)
{
  registration_data_t *data = user_data;
  g_mutex_lock (&data->manager->priv->lock);
  registration_data_unexport_interface (data, G_DBUS_INTERFACE_SKELETON (interface));
  g_mutex_unlock (&data->manager->priv->lock);
}

/* ---------------------------------------------------------------------------------------------------- */


static void
registration_data_free (registration_data_t *data)
{
  xhash_table_iter_t iter;
  xdbus_interface_skeleton_t *iface;

  data->exported = FALSE;

  xhash_table_iter_init (&iter, data->map_iface_name_to_iface);
  while (xhash_table_iter_next (&iter, NULL, (xpointer_t) &iface))
    {
      if (data->manager->priv->connection != NULL)
        g_dbus_interface_skeleton_unexport (iface);
    }

  xsignal_handlers_disconnect_by_func (data->object, G_CALLBACK (on_interface_added), data);
  xsignal_handlers_disconnect_by_func (data->object, G_CALLBACK (on_interface_removed), data);
  xobject_unref (data->object);
  xhash_table_destroy (data->map_iface_name_to_iface);
  g_free (data);
}

/* Validate whether an object path is valid as a child of the manager. According
 * to the specification:
 * https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager
 * this means that:
 * > All returned object paths are children of the object path implementing this
 * > interface, i.e. their object paths start with the ObjectManager's object
 * > path plus '/'
 *
 * For example, if the manager is at `/org/gnome/Example`, children will be
 * `/org/gnome/Example/(.+)`.
 *
 * It is permissible (but not encouraged) for the manager to be at `/`. If so,
 * children will be `/(.+)`.
 */
static xboolean_t
is_valid_child_object_path (xdbus_object_manager_server_t *manager,
                            const xchar_t              *child_object_path)
{
  /* Historically GDBus accepted @child_object_paths at `/` if the @manager
   * itself is also at `/". This is not spec-compliant, but making GDBus enforce
   * the spec more strictly would be an incompatible change.
   *
   * See https://gitlab.gnome.org/GNOME/glib/-/issues/2500 */
  g_warn_if_fail (!xstr_equal (child_object_path, manager->priv->object_path_ending_in_slash));

  return xstr_has_prefix (child_object_path, manager->priv->object_path_ending_in_slash);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
xdbus_object_manager_server_export_unlocked (xdbus_object_manager_server_t  *manager,
                                              xdbus_object_skeleton_t       *object,
                                              const xchar_t               *object_path)
{
  registration_data_t *data;
  xlist_t *existing_interfaces;
  xlist_t *l;
  xptr_array_t *interface_names;

  g_return_if_fail (X_IS_DBUS_OBJECT_MANAGER_SERVER (manager));
  g_return_if_fail (X_IS_DBUS_OBJECT (object));
  g_return_if_fail (is_valid_child_object_path (manager, object_path));

  interface_names = xptr_array_new ();

  data = xhash_table_lookup (manager->priv->map_object_path_to_data, object_path);
  if (data != NULL)
    xdbus_object_manager_server_unexport_unlocked (manager, object_path);

  data = g_new0 (registration_data_t, 1);
  data->object = xobject_ref (object);
  data->manager = manager;
  data->map_iface_name_to_iface = xhash_table_new_full (xstr_hash,
                                                         xstr_equal,
                                                         NULL,
                                                         (xdestroy_notify_t) xobject_unref);

  xsignal_connect (object,
                    "interface-added",
                    G_CALLBACK (on_interface_added),
                    data);
  xsignal_connect (object,
                    "interface-removed",
                    G_CALLBACK (on_interface_removed),
                    data);

  /* Register all known interfaces - note that data->exported is FALSE so
   * we don't emit any InterfacesAdded signals.
   */
  existing_interfaces = g_dbus_object_get_interfaces (G_DBUS_OBJECT (object));
  for (l = existing_interfaces; l != NULL; l = l->next)
    {
      xdbus_interface_skeleton_t *interface_skeleton = G_DBUS_INTERFACE_SKELETON (l->data);
      registration_data_export_interface (data, interface_skeleton, object_path);
      xptr_array_add (interface_names, g_dbus_interface_skeleton_get_info (interface_skeleton)->name);
    }
  xlist_free_full (existing_interfaces, xobject_unref);
  xptr_array_add (interface_names, NULL);

  data->exported = TRUE;

  /* now emit InterfacesAdded() for all the interfaces */
  xdbus_object_manager_server_emit_interfaces_added (manager, data, (const xchar_t *const *) interface_names->pdata, object_path);
  xptr_array_unref (interface_names);

  xhash_table_insert (manager->priv->map_object_path_to_data,
                       xstrdup (object_path),
                       data);
}

/**
 * xdbus_object_manager_server_export:
 * @manager: A #xdbus_object_manager_server_t.
 * @object: A #xdbus_object_skeleton_t.
 *
 * Exports @object on @manager.
 *
 * If there is already a #xdbus_object_t exported at the object path,
 * then the old object is removed.
 *
 * The object path for @object must be in the hierarchy rooted by the
 * object path for @manager.
 *
 * Note that @manager will take a reference on @object for as long as
 * it is exported.
 *
 * Since: 2.30
 */
void
xdbus_object_manager_server_export (xdbus_object_manager_server_t  *manager,
                                     xdbus_object_skeleton_t       *object)
{
  g_return_if_fail (X_IS_DBUS_OBJECT_MANAGER_SERVER (manager));
  g_mutex_lock (&manager->priv->lock);
  xdbus_object_manager_server_export_unlocked (manager, object,
                                                g_dbus_object_get_object_path (G_DBUS_OBJECT (object)));
  g_mutex_unlock (&manager->priv->lock);
}

/**
 * xdbus_object_manager_server_export_uniquely:
 * @manager: A #xdbus_object_manager_server_t.
 * @object: An object.
 *
 * Like xdbus_object_manager_server_export() but appends a string of
 * the form _N (with N being a natural number) to @object's object path
 * if an object with the given path already exists. As such, the
 * #xdbus_object_proxy_t:g-object-path property of @object may be modified.
 *
 * Since: 2.30
 */
void
xdbus_object_manager_server_export_uniquely (xdbus_object_manager_server_t *manager,
                                              xdbus_object_skeleton_t      *object)
{
  const xchar_t *orixobject_path;
  xchar_t *object_path;
  xuint_t count;
  xboolean_t modified;

  orixobject_path = g_dbus_object_get_object_path (G_DBUS_OBJECT (object));

  g_return_if_fail (X_IS_DBUS_OBJECT_MANAGER_SERVER (manager));
  g_return_if_fail (X_IS_DBUS_OBJECT (object));
  g_return_if_fail (is_valid_child_object_path (manager, orixobject_path));

  g_mutex_lock (&manager->priv->lock);

  object_path = xstrdup (orixobject_path);
  count = 1;
  modified = FALSE;
  while (TRUE)
    {
      registration_data_t *data;
      data = xhash_table_lookup (manager->priv->map_object_path_to_data, object_path);
      if (data == NULL)
        {
          break;
        }
      g_free (object_path);
      object_path = xstrdup_printf ("%s_%d", orixobject_path, count++);
      modified = TRUE;
    }

  xdbus_object_manager_server_export_unlocked (manager, object, object_path);

  g_mutex_unlock (&manager->priv->lock);

  if (modified)
    xdbus_object_skeleton_set_object_path (G_DBUS_OBJECT_SKELETON (object), object_path);

  g_free (object_path);

}

/**
 * xdbus_object_manager_server_is_exported:
 * @manager: A #xdbus_object_manager_server_t.
 * @object: An object.
 *
 * Returns whether @object is currently exported on @manager.
 *
 * Returns: %TRUE if @object is exported
 *
 * Since: 2.34
 **/
xboolean_t
xdbus_object_manager_server_is_exported (xdbus_object_manager_server_t *manager,
                                          xdbus_object_skeleton_t      *object)
{
  registration_data_t *data = NULL;
  const xchar_t *object_path;
  xboolean_t object_is_exported;

  xreturn_val_if_fail (X_IS_DBUS_OBJECT_MANAGER_SERVER (manager), FALSE);
  xreturn_val_if_fail (X_IS_DBUS_OBJECT (object), FALSE);

  g_mutex_lock (&manager->priv->lock);

  object_path = g_dbus_object_get_object_path (G_DBUS_OBJECT (object));
  if (object_path != NULL)
    data = xhash_table_lookup (manager->priv->map_object_path_to_data, object_path);
  object_is_exported = (data != NULL);

  g_mutex_unlock (&manager->priv->lock);

  return object_is_exported;
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
xdbus_object_manager_server_unexport_unlocked (xdbus_object_manager_server_t  *manager,
                                                const xchar_t               *object_path)
{
  registration_data_t *data;
  xboolean_t ret;

  xreturn_val_if_fail (X_IS_DBUS_OBJECT_MANAGER_SERVER (manager), FALSE);
  xreturn_val_if_fail (xvariant_is_object_path (object_path), FALSE);
  xreturn_val_if_fail (is_valid_child_object_path (manager, object_path), FALSE);

  ret = FALSE;

  data = xhash_table_lookup (manager->priv->map_object_path_to_data, object_path);
  if (data != NULL)
    {
      xptr_array_t *interface_names;
      xhash_table_iter_t iter;
      const xchar_t *iface_name;

      interface_names = xptr_array_new ();
      xhash_table_iter_init (&iter, data->map_iface_name_to_iface);
      while (xhash_table_iter_next (&iter, (xpointer_t) &iface_name, NULL))
        xptr_array_add (interface_names, (xpointer_t) iface_name);
      xptr_array_add (interface_names, NULL);
      /* now emit InterfacesRemoved() for all the interfaces */
      xdbus_object_manager_server_emit_interfaces_removed (manager, data, (const xchar_t *const *) interface_names->pdata);
      xptr_array_unref (interface_names);

      xhash_table_remove (manager->priv->map_object_path_to_data, object_path);
      ret = TRUE;
    }

  return ret;
}

/**
 * xdbus_object_manager_server_unexport:
 * @manager: A #xdbus_object_manager_server_t.
 * @object_path: An object path.
 *
 * If @manager has an object at @path, removes the object. Otherwise
 * does nothing.
 *
 * Note that @object_path must be in the hierarchy rooted by the
 * object path for @manager.
 *
 * Returns: %TRUE if object at @object_path was removed, %FALSE otherwise.
 *
 * Since: 2.30
 */
xboolean_t
xdbus_object_manager_server_unexport (xdbus_object_manager_server_t  *manager,
                                       const xchar_t         *object_path)
{
  xboolean_t ret;
  xreturn_val_if_fail (X_IS_DBUS_OBJECT_MANAGER_SERVER (manager), FALSE);
  g_mutex_lock (&manager->priv->lock);
  ret = xdbus_object_manager_server_unexport_unlocked (manager, object_path);
  g_mutex_unlock (&manager->priv->lock);
  return ret;
}


/* ---------------------------------------------------------------------------------------------------- */

static const xdbus_arg_info_t manager_interfaces_added_signal_info_arg0 =
{
  -1,
  "object_path",
  "o",
  (xdbus_annotation_info_t**) NULL,
};

static const xdbus_arg_info_t manager_interfaces_added_signal_info_arg1 =
{
  -1,
  "interfaces_and_properties",
  "a{sa{sv}}",
  (xdbus_annotation_info_t**) NULL,
};

static const xdbus_arg_info_t * const manager_interfaces_added_signal_info_arg_pointers[] =
{
  &manager_interfaces_added_signal_info_arg0,
  &manager_interfaces_added_signal_info_arg1,
  NULL
};

static const xdbus_signalInfo_t manager_interfaces_added_signal_info =
{
  -1,
  "InterfacesAdded",
  (xdbus_arg_info_t**) &manager_interfaces_added_signal_info_arg_pointers,
  (xdbus_annotation_info_t**) NULL
};

/* ---------- */

static const xdbus_arg_info_t manager_interfaces_removed_signal_info_arg0 =
{
  -1,
  "object_path",
  "o",
  (xdbus_annotation_info_t**) NULL,
};

static const xdbus_arg_info_t manager_interfaces_removed_signal_info_arg1 =
{
  -1,
  "interfaces",
  "as",
  (xdbus_annotation_info_t**) NULL,
};

static const xdbus_arg_info_t * const manager_interfaces_removed_signal_info_arg_pointers[] =
{
  &manager_interfaces_removed_signal_info_arg0,
  &manager_interfaces_removed_signal_info_arg1,
  NULL
};

static const xdbus_signalInfo_t manager_interfaces_removed_signal_info =
{
  -1,
  "InterfacesRemoved",
  (xdbus_arg_info_t**) &manager_interfaces_removed_signal_info_arg_pointers,
  (xdbus_annotation_info_t**) NULL
};

/* ---------- */

static const xdbus_signalInfo_t * const manager_signal_info_pointers[] =
{
  &manager_interfaces_added_signal_info,
  &manager_interfaces_removed_signal_info,
  NULL
};

/* ---------- */

static const xdbus_arg_info_t manager_get_all_method_info_out_arg0 =
{
  -1,
  "object_paths_interfaces_and_properties",
  "a{oa{sa{sv}}}",
  (xdbus_annotation_info_t**) NULL,
};

static const xdbus_arg_info_t * const manager_get_all_method_info_out_arg_pointers[] =
{
  &manager_get_all_method_info_out_arg0,
  NULL
};

static const xdbus_method_info_t manager_get_all_method_info =
{
  -1,
  "GetManagedObjects",
  (xdbus_arg_info_t**) NULL,
  (xdbus_arg_info_t**) &manager_get_all_method_info_out_arg_pointers,
  (xdbus_annotation_info_t**) NULL
};

static const xdbus_method_info_t * const manager_method_info_pointers[] =
{
  &manager_get_all_method_info,
  NULL
};

/* ---------- */

static const xdbus_interface_info_t manager_interface_info =
{
  -1,
  "org.freedesktop.DBus.ObjectManager",
  (xdbus_method_info_t **) manager_method_info_pointers,
  (xdbus_signalInfo_t **) manager_signal_info_pointers,
  (xdbus_property_info_t **) NULL,
  (xdbus_annotation_info_t **) NULL
};

static void
manager_method_call (xdbus_connection_t       *connection,
                     const xchar_t           *sender,
                     const xchar_t           *object_path,
                     const xchar_t           *interface_name,
                     const xchar_t           *method_name,
                     xvariant_t              *parameters,
                     xdbus_method_invocation_t *invocation,
                     xpointer_t               user_data)
{
  xdbus_object_manager_server_t *manager = G_DBUS_OBJECT_MANAGER_SERVER (user_data);
  xvariant_builder_t array_builder;
  xhash_table_iter_t object_iter;
  registration_data_t *data;

  g_mutex_lock (&manager->priv->lock);

  if (xstrcmp0 (method_name, "GetManagedObjects") == 0)
    {
      xvariant_builder_init (&array_builder, G_VARIANT_TYPE ("a{oa{sa{sv}}}"));
      xhash_table_iter_init (&object_iter, manager->priv->map_object_path_to_data);
      while (xhash_table_iter_next (&object_iter, NULL, (xpointer_t) &data))
        {
          xvariant_builder_t interfaces_builder;
          xhash_table_iter_t interface_iter;
          xdbus_interface_skeleton_t *iface;
          const xchar_t *iter_object_path;

          xvariant_builder_init (&interfaces_builder, G_VARIANT_TYPE ("a{sa{sv}}"));
          xhash_table_iter_init (&interface_iter, data->map_iface_name_to_iface);
          while (xhash_table_iter_next (&interface_iter, NULL, (xpointer_t) &iface))
            {
              xvariant_t *properties = g_dbus_interface_skeleton_get_properties (iface);
              xvariant_builder_add (&interfaces_builder, "{s@a{sv}}",
                                     g_dbus_interface_skeleton_get_info (iface)->name,
                                     properties);
              xvariant_unref (properties);
            }
          iter_object_path = g_dbus_object_get_object_path (G_DBUS_OBJECT (data->object));
          xvariant_builder_add (&array_builder,
                                 "{oa{sa{sv}}}",
                                 iter_object_path,
                                 &interfaces_builder);
        }

      xdbus_method_invocation_return_value (invocation,
                                             xvariant_new ("(a{oa{sa{sv}}})",
                                                            &array_builder));
    }
  else
    {
      xdbus_method_invocation_return_error (invocation,
                                             G_DBUS_ERROR,
                                             G_DBUS_ERROR_UNKNOWN_METHOD,
                                             "Unknown method %s - only GetManagedObjects() is supported",
                                             method_name);
    }
  g_mutex_unlock (&manager->priv->lock);
}

static const xdbus_interface_vtable_t manager_interface_vtable =
{
  manager_method_call, /* handle_method_call */
  NULL, /* get_property */
  NULL, /* set_property */
  { 0 }
};

/* ---------------------------------------------------------------------------------------------------- */

static void
xdbus_object_manager_server_constructed (xobject_t *object)
{
  xdbus_object_manager_server_t *manager = G_DBUS_OBJECT_MANAGER_SERVER (object);

  if (manager->priv->connection != NULL)
    export_all (manager);

  if (XOBJECT_CLASS (xdbus_object_manager_server_parent_class)->constructed != NULL)
    XOBJECT_CLASS (xdbus_object_manager_server_parent_class)->constructed (object);
}

static void
xdbus_object_manager_server_emit_interfaces_added (xdbus_object_manager_server_t *manager,
                                                    registration_data_t   *data,
                                                    const xchar_t *const *interfaces,
                                                    const xchar_t *object_path)
{
  xvariant_builder_t array_builder;
  xerror_t *error;
  xuint_t n;

  if (data->manager->priv->connection == NULL)
    goto out;

  xvariant_builder_init (&array_builder, G_VARIANT_TYPE ("a{sa{sv}}"));
  for (n = 0; interfaces[n] != NULL; n++)
    {
      xdbus_interface_skeleton_t *iface;
      xvariant_t *properties;

      iface = xhash_table_lookup (data->map_iface_name_to_iface, interfaces[n]);
      xassert (iface != NULL);
      properties = g_dbus_interface_skeleton_get_properties (iface);
      xvariant_builder_add (&array_builder, "{s@a{sv}}", interfaces[n], properties);
      xvariant_unref (properties);
    }

  error = NULL;
  xdbus_connection_emit_signal (data->manager->priv->connection,
                                 NULL, /* destination_bus_name */
                                 manager->priv->object_path,
                                 manager_interface_info.name,
                                 "InterfacesAdded",
                                 xvariant_new ("(oa{sa{sv}})",
                                                object_path,
                                                &array_builder),
                                 &error);
  if (error)
    {
      if (!xerror_matches (error, G_IO_ERROR, G_IO_ERROR_CLOSED))
        g_warning ("Couldn't emit InterfacesAdded signal: %s", error->message);
      xerror_free (error);
    }
 out:
  ;
}

static void
xdbus_object_manager_server_emit_interfaces_removed (xdbus_object_manager_server_t *manager,
                                                      registration_data_t   *data,
                                                      const xchar_t *const *interfaces)
{
  xvariant_builder_t array_builder;
  xerror_t *error;
  xuint_t n;
  const xchar_t *object_path;

  if (data->manager->priv->connection == NULL)
    goto out;

  xvariant_builder_init (&array_builder, G_VARIANT_TYPE ("as"));
  for (n = 0; interfaces[n] != NULL; n++)
    xvariant_builder_add (&array_builder, "s", interfaces[n]);

  error = NULL;
  object_path = g_dbus_object_get_object_path (G_DBUS_OBJECT (data->object));
  xdbus_connection_emit_signal (data->manager->priv->connection,
                                 NULL, /* destination_bus_name */
                                 manager->priv->object_path,
                                 manager_interface_info.name,
                                 "InterfacesRemoved",
                                 xvariant_new ("(oas)",
                                                object_path,
                                                &array_builder),
                                 &error);
  if (error)
    {
      if (!xerror_matches (error, G_IO_ERROR, G_IO_ERROR_CLOSED))
        g_warning ("Couldn't emit InterfacesRemoved signal: %s", error->message);
      xerror_free (error);
    }
 out:
  ;
}

/* ---------------------------------------------------------------------------------------------------- */

static xlist_t *
xdbus_object_manager_server_get_objects (xdbus_object_manager_t  *_manager)
{
  xdbus_object_manager_server_t *manager = G_DBUS_OBJECT_MANAGER_SERVER (_manager);
  xlist_t *ret;
  xhash_table_iter_t iter;
  registration_data_t *data;

  g_mutex_lock (&manager->priv->lock);

  ret = NULL;
  xhash_table_iter_init (&iter, manager->priv->map_object_path_to_data);
  while (xhash_table_iter_next (&iter, NULL, (xpointer_t) &data))
    {
      ret = xlist_prepend (ret, xobject_ref (data->object));
    }

  g_mutex_unlock (&manager->priv->lock);

  return ret;
}

static const xchar_t *
xdbus_object_manager_server_get_object_path (xdbus_object_manager_t *_manager)
{
  xdbus_object_manager_server_t *manager = G_DBUS_OBJECT_MANAGER_SERVER (_manager);
  return manager->priv->object_path;
}

static xdbus_object_t *
xdbus_object_manager_server_get_object (xdbus_object_manager_t *_manager,
                                         const xchar_t        *object_path)
{
  xdbus_object_manager_server_t *manager = G_DBUS_OBJECT_MANAGER_SERVER (_manager);
  xdbus_object_t *ret;
  registration_data_t *data;

  ret = NULL;

  g_mutex_lock (&manager->priv->lock);
  data = xhash_table_lookup (manager->priv->map_object_path_to_data, object_path);
  if (data != NULL)
    ret = xobject_ref (G_DBUS_OBJECT (data->object));
  g_mutex_unlock (&manager->priv->lock);

  return ret;
}

static xdbus_interface_t *
xdbus_object_manager_server_get_interface  (xdbus_object_manager_t  *_manager,
                                             const xchar_t         *object_path,
                                             const xchar_t         *interface_name)
{
  xdbus_interface_t *ret;
  xdbus_object_t *object;

  ret = NULL;

  object = g_dbus_object_manager_get_object (_manager, object_path);
  if (object == NULL)
    goto out;

  ret = g_dbus_object_get_interface (object, interface_name);
  xobject_unref (object);

 out:
  return ret;
}

static void
dbus_object_manager_interface_init (xdbus_object_manager_iface_t *iface)
{
  iface->get_object_path = xdbus_object_manager_server_get_object_path;
  iface->get_objects     = xdbus_object_manager_server_get_objects;
  iface->get_object      = xdbus_object_manager_server_get_object;
  iface->get_interface   = xdbus_object_manager_server_get_interface;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
export_all (xdbus_object_manager_server_t *manager)
{
  xhash_table_iter_t iter;
  const xchar_t *object_path;
  registration_data_t *data;
  xhash_table_iter_t iface_iter;
  xdbus_interface_skeleton_t *iface;
  xerror_t *error;

  g_return_if_fail (manager->priv->connection != NULL);

  error = NULL;
  g_warn_if_fail (manager->priv->manager_reg_id == 0);
  manager->priv->manager_reg_id = xdbus_connection_register_object (manager->priv->connection,
                                                                     manager->priv->object_path,
                                                                     (xdbus_interface_info_t *) &manager_interface_info,
                                                                     &manager_interface_vtable,
                                                                     manager,
                                                                     NULL, /* user_data_free_func */
                                                                     &error);
  if (manager->priv->manager_reg_id == 0)
    {
      g_warning ("%s: Error registering manager at %s: %s",
                 G_STRLOC,
                 manager->priv->object_path,
                 error->message);
      xerror_free (error);
    }

  xhash_table_iter_init (&iter, manager->priv->map_object_path_to_data);
  while (xhash_table_iter_next (&iter, (xpointer_t) &object_path, (xpointer_t) &data))
    {
      xhash_table_iter_init (&iface_iter, data->map_iface_name_to_iface);
      while (xhash_table_iter_next (&iface_iter, NULL, (xpointer_t) &iface))
        {
          g_warn_if_fail (g_dbus_interface_skeleton_get_connection (iface) == NULL);
          error = NULL;
          if (!g_dbus_interface_skeleton_export (iface,
                                                 manager->priv->connection,
                                                 object_path,
                                                 &error))
            {
              g_warning ("%s: Error registering object at %s with interface %s: %s",
                         G_STRLOC,
                         object_path,
                         g_dbus_interface_skeleton_get_info (iface)->name,
                         error->message);
              xerror_free (error);
            }
        }
    }
}

static void
unexport_all (xdbus_object_manager_server_t *manager, xboolean_t only_manager)
{
  xhash_table_iter_t iter;
  registration_data_t *data;
  xhash_table_iter_t iface_iter;
  xdbus_interface_skeleton_t *iface;

  g_return_if_fail (manager->priv->connection != NULL);

  g_warn_if_fail (manager->priv->manager_reg_id > 0);
  if (manager->priv->manager_reg_id > 0)
    {
      g_warn_if_fail (xdbus_connection_unregister_object (manager->priv->connection,
                                                           manager->priv->manager_reg_id));
      manager->priv->manager_reg_id = 0;
    }
  if (only_manager)
    goto out;

  xhash_table_iter_init (&iter, manager->priv->map_object_path_to_data);
  while (xhash_table_iter_next (&iter, NULL, (xpointer_t) &data))
    {
      xhash_table_iter_init (&iface_iter, data->map_iface_name_to_iface);
      while (xhash_table_iter_next (&iface_iter, NULL, (xpointer_t) &iface))
        {
          g_warn_if_fail (g_dbus_interface_skeleton_get_connection (iface) != NULL);
          g_dbus_interface_skeleton_unexport (iface);
        }
    }
 out:
  ;
}

/* ---------------------------------------------------------------------------------------------------- */
