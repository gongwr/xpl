/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"
#include <string.h>
#include "gvfs.h"
#include "glib-private.h"
#include "glocalvfs.h"
#include "gresourcefile.h"
#include "giomodule-priv.h"
#include "glibintl.h"


/**
 * SECTION:gvfs
 * @short_description: Virtual File System
 * @include: gio/gio.h
 *
 * Entry point for using GIO functionality.
 *
 */

static GRWLock additional_schemes_lock;

typedef struct _GVfsPrivate {
  xhashtable_t *additional_schemes;
  char const **supported_schemes;
} GVfsPrivate;

typedef struct {
  xvfs_file_lookup_func_t uri_func;
  xpointer_t uri_data;
  xdestroy_notify_t uri_destroy;

  xvfs_file_lookup_func_t parse_name_func;
  xpointer_t parse_name_data;
  xdestroy_notify_t parse_name_destroy;
} GVfsURISchemeData;

G_DEFINE_TYPE_WITH_PRIVATE (xvfs_t, g_vfs, XTYPE_OBJECT)

static void
xvfs_dispose (xobject_t *object)
{
  xvfs_t *vfs = XVFS (object);
  GVfsPrivate *priv = xvfs_get_instance_private (vfs);

  g_clear_pointer (&priv->additional_schemes, xhash_table_destroy);
  g_clear_pointer (&priv->supported_schemes, g_free);

  XOBJECT_CLASS (xvfs_parent_class)->dispose (object);
}

static void
xvfs_class_init (xvfs_class_t *klass)
{
  xobject_class_t *object_class = XOBJECT_CLASS (klass);
  object_class->dispose = xvfs_dispose;
}

static xfile_t *
resource_parse_name (xvfs_t       *vfs,
                     const char *parse_name,
                     xpointer_t    user_data)
{
  if (xstr_has_prefix (parse_name, "resource:"))
    return _xresource_file_new (parse_name);

  return NULL;
}

static xfile_t *
resource_get_file_for_uri (xvfs_t       *vfs,
                           const char *uri,
                           xpointer_t    user_data)
{
  return _xresource_file_new (uri);
}

static void
xvfs_uri_lookup_func_closure_free (xpointer_t data)
{
  GVfsURISchemeData *closure = data;

  if (closure->uri_destroy)
    closure->uri_destroy (closure->uri_data);
  if (closure->parse_name_destroy)
    closure->parse_name_destroy (closure->parse_name_data);

  g_free (closure);
}

static void
xvfs_init (xvfs_t *vfs)
{
  GVfsPrivate *priv = xvfs_get_instance_private (vfs);
  priv->additional_schemes =
    xhash_table_new_full (xstr_hash, xstr_equal,
                           g_free, xvfs_uri_lookup_func_closure_free);

  xvfs_register_uri_scheme (vfs, "resource",
                             resource_get_file_for_uri, NULL, NULL,
                             resource_parse_name, NULL, NULL);
}

/**
 * xvfs_is_active:
 * @vfs: a #xvfs_t.
 *
 * Checks if the VFS is active.
 *
 * Returns: %TRUE if construction of the @vfs was successful
 *     and it is now active.
 */
xboolean_t
xvfs_is_active (xvfs_t *vfs)
{
  xvfs_class_t *class;

  xreturn_val_if_fail (X_IS_VFS (vfs), FALSE);

  class = XVFS_GET_CLASS (vfs);

  return (* class->is_active) (vfs);
}


/**
 * xvfs_get_file_for_path:
 * @vfs: a #xvfs_t.
 * @path: a string containing a VFS path.
 *
 * Gets a #xfile_t for @path.
 *
 * Returns: (transfer full): a #xfile_t.
 *     Free the returned object with xobject_unref().
 */
xfile_t *
xvfs_get_file_for_path (xvfs_t       *vfs,
                         const char *path)
{
  xvfs_class_t *class;

  xreturn_val_if_fail (X_IS_VFS (vfs), NULL);
  xreturn_val_if_fail (path != NULL, NULL);

  class = XVFS_GET_CLASS (vfs);

  return (* class->get_file_for_path) (vfs, path);
}

static xfile_t *
parse_name_internal (xvfs_t       *vfs,
                     const char *parse_name)
{
  GVfsPrivate *priv = xvfs_get_instance_private (vfs);
  xhash_table_iter_t iter;
  GVfsURISchemeData *closure;
  xfile_t *ret = NULL;

  g_rw_lock_reader_lock (&additional_schemes_lock);
  xhash_table_iter_init (&iter, priv->additional_schemes);

  while (xhash_table_iter_next (&iter, NULL, (xpointer_t *) &closure))
    {
      ret = closure->parse_name_func (vfs, parse_name,
                                      closure->parse_name_data);

      if (ret)
        break;
    }

  g_rw_lock_reader_unlock (&additional_schemes_lock);

  return ret;
}

static xfile_t *
get_file_for_uri_internal (xvfs_t       *vfs,
                           const char *uri)
{
  GVfsPrivate *priv = xvfs_get_instance_private (vfs);
  xfile_t *ret = NULL;
  char *scheme;
  GVfsURISchemeData *closure;

  scheme = xuri_parse_scheme (uri);
  if (scheme == NULL)
    return NULL;

  g_rw_lock_reader_lock (&additional_schemes_lock);
  closure = xhash_table_lookup (priv->additional_schemes, scheme);

  if (closure)
    ret = closure->uri_func (vfs, uri, closure->uri_data);

  g_rw_lock_reader_unlock (&additional_schemes_lock);

  g_free (scheme);
  return ret;
}

/**
 * xvfs_get_file_for_uri:
 * @vfs: a#xvfs_t.
 * @uri: a string containing a URI
 *
 * Gets a #xfile_t for @uri.
 *
 * This operation never fails, but the returned object
 * might not support any I/O operation if the URI
 * is malformed or if the URI scheme is not supported.
 *
 * Returns: (transfer full): a #xfile_t.
 *     Free the returned object with xobject_unref().
 */
xfile_t *
xvfs_get_file_for_uri (xvfs_t       *vfs,
                        const char *uri)
{
  xvfs_class_t *class;
  xfile_t *ret = NULL;

  xreturn_val_if_fail (X_IS_VFS (vfs), NULL);
  xreturn_val_if_fail (uri != NULL, NULL);

  class = XVFS_GET_CLASS (vfs);

  ret = get_file_for_uri_internal (vfs, uri);
  if (!ret)
    ret = (* class->get_file_for_uri) (vfs, uri);

  xassert (ret != NULL);

  return g_steal_pointer (&ret);
}

/**
 * xvfs_get_supported_uri_schemes:
 * @vfs: a #xvfs_t.
 *
 * Gets a list of URI schemes supported by @vfs.
 *
 * Returns: (transfer none): a %NULL-terminated array of strings.
 *     The returned array belongs to GIO and must
 *     not be freed or modified.
 */
const xchar_t * const *
xvfs_get_supported_uri_schemes (xvfs_t *vfs)
{
  GVfsPrivate *priv;

  xreturn_val_if_fail (X_IS_VFS (vfs), NULL);

  priv = xvfs_get_instance_private (vfs);

  if (!priv->supported_schemes)
    {
      xvfs_class_t *class;
      const char * const *default_schemes;
      const char *additional_scheme;
      xptr_array_t *supported_schemes;
      xhash_table_iter_t iter;

      class = XVFS_GET_CLASS (vfs);

      default_schemes = (* class->get_supported_uri_schemes) (vfs);
      supported_schemes = xptr_array_new ();

      for (; default_schemes && *default_schemes; default_schemes++)
        xptr_array_add (supported_schemes, (xpointer_t) *default_schemes);

      g_rw_lock_reader_lock (&additional_schemes_lock);
      xhash_table_iter_init (&iter, priv->additional_schemes);

      while (xhash_table_iter_next
             (&iter, (xpointer_t *) &additional_scheme, NULL))
        xptr_array_add (supported_schemes, (xpointer_t) additional_scheme);

      g_rw_lock_reader_unlock (&additional_schemes_lock);

      xptr_array_add (supported_schemes, NULL);

      g_free (priv->supported_schemes);
      priv->supported_schemes =
        (char const **) xptr_array_free (supported_schemes, FALSE);
    }

  return priv->supported_schemes;
}

/**
 * xvfs_parse_name:
 * @vfs: a #xvfs_t.
 * @parse_name: a string to be parsed by the VFS module.
 *
 * This operation never fails, but the returned object might
 * not support any I/O operations if the @parse_name cannot
 * be parsed by the #xvfs_t module.
 *
 * Returns: (transfer full): a #xfile_t for the given @parse_name.
 *     Free the returned object with xobject_unref().
 */
xfile_t *
xvfs_parse_name (xvfs_t       *vfs,
                  const char *parse_name)
{
  xvfs_class_t *class;
  xfile_t *ret;

  xreturn_val_if_fail (X_IS_VFS (vfs), NULL);
  xreturn_val_if_fail (parse_name != NULL, NULL);

  class = XVFS_GET_CLASS (vfs);

  ret = parse_name_internal (vfs, parse_name);
  if (ret)
    return ret;

  return (* class->parse_name) (vfs, parse_name);
}

static xvfs_t *vfs_default_singleton = NULL;  /* (owned) (atomic) */

/**
 * xvfs_get_default:
 *
 * Gets the default #xvfs_t for the system.
 *
 * Returns: (not nullable) (transfer none): a #xvfs_t, which will be the local
 *     file system #xvfs_t if no other implementation is available.
 */
xvfs_t *
xvfs_get_default (void)
{
  if (XPL_PRIVATE_CALL (g_check_setuid) ())
    return xvfs_get_local ();

  if (g_once_init_enter (&vfs_default_singleton))
    {
      xvfs_t *singleton;

      singleton = _xio_module_get_default (XVFS_EXTENSION_POINT_NAME,
                                            "GIO_USE_VFS",
                                            (xio_module_verify_func_t) xvfs_is_active);

      g_once_init_leave (&vfs_default_singleton, singleton);
    }

  return vfs_default_singleton;
}

/**
 * xvfs_get_local:
 *
 * Gets the local #xvfs_t for the system.
 *
 * Returns: (transfer none): a #xvfs_t.
 */
xvfs_t *
xvfs_get_local (void)
{
  static xsize_t vfs = 0;

  if (g_once_init_enter (&vfs))
    g_once_init_leave (&vfs, (xsize_t)_g_local_vfs_new ());

  return XVFS (vfs);
}

/**
 * xvfs_register_uri_scheme:
 * @vfs: a #xvfs_t
 * @scheme: an URI scheme, e.g. "http"
 * @uri_func: (scope notified) (nullable): a #xvfs_file_lookup_func_t
 * @uri_data: (nullable): custom data passed to be passed to @uri_func, or %NULL
 * @uri_destroy: (nullable): function to be called when unregistering the
 *     URI scheme, or when @vfs is disposed, to free the resources used
 *     by the URI lookup function
 * @parse_name_func: (scope notified) (nullable): a #xvfs_file_lookup_func_t
 * @parse_name_data: (nullable): custom data passed to be passed to
 *     @parse_name_func, or %NULL
 * @parse_name_destroy: (nullable): function to be called when unregistering the
 *     URI scheme, or when @vfs is disposed, to free the resources used
 *     by the parse name lookup function
 *
 * Registers @uri_func and @parse_name_func as the #xfile_t URI and parse name
 * lookup functions for URIs with a scheme matching @scheme.
 * Note that @scheme is registered only within the running application, as
 * opposed to desktop-wide as it happens with xvfs_t backends.
 *
 * When a #xfile_t is requested with an URI containing @scheme (e.g. through
 * xfile_new_for_uri()), @uri_func will be called to allow a custom
 * constructor. The implementation of @uri_func should not be blocking, and
 * must not call xvfs_register_uri_scheme() or xvfs_unregister_uri_scheme().
 *
 * When xfile_parse_name() is called with a parse name obtained from such file,
 * @parse_name_func will be called to allow the #xfile_t to be created again. In
 * that case, it's responsibility of @parse_name_func to make sure the parse
 * name matches what the custom #xfile_t implementation returned when
 * xfile_get_parse_name() was previously called. The implementation of
 * @parse_name_func should not be blocking, and must not call
 * xvfs_register_uri_scheme() or xvfs_unregister_uri_scheme().
 *
 * It's an error to call this function twice with the same scheme. To unregister
 * a custom URI scheme, use xvfs_unregister_uri_scheme().
 *
 * Returns: %TRUE if @scheme was successfully registered, or %FALSE if a handler
 *     for @scheme already exists.
 *
 * Since: 2.50
 */
xboolean_t
xvfs_register_uri_scheme (xvfs_t              *vfs,
                           const char        *scheme,
                           xvfs_file_lookup_func_t uri_func,
                           xpointer_t           uri_data,
                           xdestroy_notify_t     uri_destroy,
                           xvfs_file_lookup_func_t parse_name_func,
                           xpointer_t           parse_name_data,
                           xdestroy_notify_t     parse_name_destroy)
{
  GVfsPrivate *priv;
  GVfsURISchemeData *closure;

  xreturn_val_if_fail (X_IS_VFS (vfs), FALSE);
  xreturn_val_if_fail (scheme != NULL, FALSE);

  priv = xvfs_get_instance_private (vfs);

  g_rw_lock_reader_lock (&additional_schemes_lock);
  closure = xhash_table_lookup (priv->additional_schemes, scheme);
  g_rw_lock_reader_unlock (&additional_schemes_lock);

  if (closure != NULL)
    return FALSE;

  closure = g_new0 (GVfsURISchemeData, 1);
  closure->uri_func = uri_func;
  closure->uri_data = uri_data;
  closure->uri_destroy = uri_destroy;
  closure->parse_name_func = parse_name_func;
  closure->parse_name_data = parse_name_data;
  closure->parse_name_destroy = parse_name_destroy;

  g_rw_lock_writer_lock (&additional_schemes_lock);
  xhash_table_insert (priv->additional_schemes, xstrdup (scheme), closure);
  g_rw_lock_writer_unlock (&additional_schemes_lock);

  /* Invalidate supported schemes */
  g_clear_pointer (&priv->supported_schemes, g_free);

  return TRUE;
}

/**
 * xvfs_unregister_uri_scheme:
 * @vfs: a #xvfs_t
 * @scheme: an URI scheme, e.g. "http"
 *
 * Unregisters the URI handler for @scheme previously registered with
 * xvfs_register_uri_scheme().
 *
 * Returns: %TRUE if @scheme was successfully unregistered, or %FALSE if a
 *     handler for @scheme does not exist.
 *
 * Since: 2.50
 */
xboolean_t
xvfs_unregister_uri_scheme (xvfs_t       *vfs,
                             const char *scheme)
{
  GVfsPrivate *priv;
  xboolean_t res;

  xreturn_val_if_fail (X_IS_VFS (vfs), FALSE);
  xreturn_val_if_fail (scheme != NULL, FALSE);

  priv = xvfs_get_instance_private (vfs);

  g_rw_lock_writer_lock (&additional_schemes_lock);
  res = xhash_table_remove (priv->additional_schemes, scheme);
  g_rw_lock_writer_unlock (&additional_schemes_lock);

  if (res)
    {
      /* Invalidate supported schemes */
      g_clear_pointer (&priv->supported_schemes, g_free);

      return TRUE;
    }

  return FALSE;
}
