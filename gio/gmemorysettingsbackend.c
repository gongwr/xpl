/*
 * Copyright © 2010 Codethink Limited
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
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gsimplepermission.h"
#include "gsettingsbackendinternal.h"
#include "giomodule.h"


#define XTYPE_MEMORY_SETTINGS_BACKEND  (xmemory_settings_backend_get_type())
#define XMEMORY_SETTINGS_BACKEND(inst) (XTYPE_CHECK_INSTANCE_CAST ((inst), \
                                         XTYPE_MEMORY_SETTINGS_BACKEND,     \
                                         xmemory_settings_backend_t))

typedef xsettings_backend_class_t xmemory_settings_backend_class_t;
typedef struct
{
  xsettings_backend_t parent_instance;
  xhashtable_t *table;
} xmemory_settings_backend_t;

G_DEFINE_TYPE_WITH_CODE (xmemory_settings_backend,
                         xmemory_settings_backend,
                         XTYPE_SETTINGS_BACKEND,
                         g_io_extension_point_implement (G_SETTINGS_BACKEND_EXTENSION_POINT_NAME,
                                                         g_define_type_id, "memory", 10))

static xvariant_t *
xmemory_settings_backend_read (xsettings_backend_t   *backend,
                                const xchar_t        *key,
                                const xvariant_type_t *expected_type,
                                xboolean_t            default_value)
{
  xmemory_settings_backend_t *memory = XMEMORY_SETTINGS_BACKEND (backend);
  xvariant_t *value;

  if (default_value)
    return NULL;

  value = xhash_table_lookup (memory->table, key);

  if (value != NULL)
    xvariant_ref (value);

  return value;
}

static xboolean_t
xmemory_settings_backend_write (xsettings_backend_t *backend,
                                 const xchar_t      *key,
                                 xvariant_t         *value,
                                 xpointer_t          origin_tag)
{
  xmemory_settings_backend_t *memory = XMEMORY_SETTINGS_BACKEND (backend);
  xvariant_t *old_value;

  old_value = xhash_table_lookup (memory->table, key);
  xvariant_ref_sink (value);

  if (old_value == NULL || !xvariant_equal (value, old_value))
    {
      xhash_table_insert (memory->table, xstrdup (key), value);
      g_settings_backend_changed (backend, key, origin_tag);
    }
  else
    xvariant_unref (value);

  return TRUE;
}

static xboolean_t
xmemory_settings_backend_write_one (xpointer_t key,
                                     xpointer_t value,
                                     xpointer_t data)
{
  xmemory_settings_backend_t *memory = data;

  if (value != NULL)
    xhash_table_insert (memory->table, xstrdup (key), xvariant_ref (value));
  else
    xhash_table_remove (memory->table, key);

  return FALSE;
}

static xboolean_t
xmemory_settings_backend_write_tree (xsettings_backend_t *backend,
                                      xtree_t            *tree,
                                      xpointer_t          origin_tag)
{
  xtree_foreach (tree, xmemory_settings_backend_write_one, backend);
  g_settings_backend_changed_tree (backend, tree, origin_tag);

  return TRUE;
}

static void
xmemory_settings_backend_reset (xsettings_backend_t *backend,
                                 const xchar_t      *key,
                                 xpointer_t          origin_tag)
{
  xmemory_settings_backend_t *memory = XMEMORY_SETTINGS_BACKEND (backend);

  if (xhash_table_lookup (memory->table, key))
    {
      xhash_table_remove (memory->table, key);
      g_settings_backend_changed (backend, key, origin_tag);
    }
}

static xboolean_t
xmemory_settings_backend_get_writable (xsettings_backend_t *backend,
                                        const xchar_t      *name)
{
  return TRUE;
}

static xpermission_t *
xmemory_settings_backend_get_permission (xsettings_backend_t *backend,
                                          const xchar_t      *path)
{
  return g_simple_permission_new (TRUE);
}

static void
xmemory_settings_backend_finalize (xobject_t *object)
{
  xmemory_settings_backend_t *memory = XMEMORY_SETTINGS_BACKEND (object);

  xhash_table_unref (memory->table);

  XOBJECT_CLASS (xmemory_settings_backend_parent_class)
    ->finalize (object);
}

static void
xmemory_settings_backend_init (xmemory_settings_backend_t *memory)
{
  memory->table = xhash_table_new_full (xstr_hash, xstr_equal, g_free,
                                         (xdestroy_notify_t) xvariant_unref);
}

static void
xmemory_settings_backend_class_init (xmemory_settings_backend_class_t *class)
{
  xsettings_backend_class_t *backend_class = G_SETTINGS_BACKEND_CLASS (class);
  xobject_class_t *object_class = XOBJECT_CLASS (class);

  backend_class->read = xmemory_settings_backend_read;
  backend_class->write = xmemory_settings_backend_write;
  backend_class->write_tree = xmemory_settings_backend_write_tree;
  backend_class->reset = xmemory_settings_backend_reset;
  backend_class->get_writable = xmemory_settings_backend_get_writable;
  backend_class->get_permission = xmemory_settings_backend_get_permission;
  object_class->finalize = xmemory_settings_backend_finalize;
}

/**
 * xmemory_settings_backend_new:
 *
 * Creates a memory-backed #xsettings_backend_t.
 *
 * This backend allows changes to settings, but does not write them
 * to any backing storage, so the next time you run your application,
 * the memory backend will start out with the default values again.
 *
 * Returns: (transfer full): a newly created #xsettings_backend_t
 *
 * Since: 2.28
 */
xsettings_backend_t *
xmemory_settings_backend_new (void)
{
  return xobject_new (XTYPE_MEMORY_SETTINGS_BACKEND, NULL);
}
