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

#include "gsettingsbackendinternal.h"
#include "giomodule.h"
#include "gsimplepermission.h"


#define XTYPE_NULL_SETTINGS_BACKEND    (g_null_settings_backend_get_type ())
#define G_NULL_SETTINGS_BACKEND(inst)   (XTYPE_CHECK_INSTANCE_CAST ((inst), \
                                         XTYPE_NULL_SETTINGS_BACKEND,       \
                                         GNullSettingsBackend))


typedef GSettingsBackendClass GNullSettingsBackendClass;
typedef xsettings_backend_t      GNullSettingsBackend;

G_DEFINE_TYPE_WITH_CODE (GNullSettingsBackend,
                         g_null_settings_backend,
                         XTYPE_SETTINGS_BACKEND,
                         g_io_extension_point_implement (G_SETTINGS_BACKEND_EXTENSION_POINT_NAME,
                                                         g_define_type_id, "null", 10))

static xvariant_t *
g_null_settings_backend_read (xsettings_backend_t   *backend,
                              const xchar_t        *key,
                              const xvariant_type_t *expected_type,
                              xboolean_t            default_value)
{
  return NULL;
}

static xboolean_t
g_null_settings_backend_write (xsettings_backend_t *backend,
                               const xchar_t      *key,
                               xvariant_t         *value,
                               xpointer_t          origin_tag)
{
  if (value)
    xvariant_unref (xvariant_ref_sink (value));
  return FALSE;
}

static xboolean_t
g_null_settings_backend_write_one (xpointer_t key,
                                   xpointer_t value,
                                   xpointer_t data)
{
  if (value)
    xvariant_unref (xvariant_ref_sink (value));
  return FALSE;
}

static xboolean_t
g_null_settings_backend_write_tree (xsettings_backend_t *backend,
                                    xtree_t            *tree,
                                    xpointer_t          origin_tag)
{
  xtree_foreach (tree, g_null_settings_backend_write_one, backend);
  return FALSE;
}

static void
g_null_settings_backend_reset (xsettings_backend_t *backend,
                               const xchar_t      *key,
                               xpointer_t          origin_tag)
{
}

static xboolean_t
g_null_settings_backend_get_writable (xsettings_backend_t *backend,
                                      const xchar_t      *name)
{
  return FALSE;
}

static xpermission_t *
g_null_settings_backend_get_permission (xsettings_backend_t *backend,
                                        const xchar_t      *path)
{
  return g_simple_permission_new (FALSE);
}

static void
g_null_settings_backend_init (GNullSettingsBackend *memory)
{
}

static void
g_null_settings_backend_class_init (GNullSettingsBackendClass *class)
{
  GSettingsBackendClass *backend_class = G_SETTINGS_BACKEND_CLASS (class);

  backend_class->read = g_null_settings_backend_read;
  backend_class->write = g_null_settings_backend_write;
  backend_class->write_tree = g_null_settings_backend_write_tree;
  backend_class->reset = g_null_settings_backend_reset;
  backend_class->get_writable = g_null_settings_backend_get_writable;
  backend_class->get_permission = g_null_settings_backend_get_permission;
}

/**
 * g_null_settings_backend_new:
 *
 *
 * Creates a readonly #xsettings_backend_t.
 *
 * This backend does not allow changes to settings, so all settings
 * will always have their default values.
 *
 * Returns: (transfer full): a newly created #xsettings_backend_t
 *
 * Since: 2.28
 */
xsettings_backend_t *
g_null_settings_backend_new (void)
{
  return xobject_new (XTYPE_NULL_SETTINGS_BACKEND, NULL);
}
