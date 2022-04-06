/*
 * Copyright © 2009, 2010 Codethink Limited
 * Copyright © 2010 Red Hat, Inc.
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Matthias Clasen <mclasen@redhat.com>
 */

#ifndef __G_SETTINGS_BACKEND_INTERNAL_H__
#define __G_SETTINGS_BACKEND_INTERNAL_H__

#include "gsettingsbackend.h"

typedef struct
{
  void (* changed)               (xobject_t             *target,
                                  xsettings_backend_t    *backend,
                                  const xchar_t         *key,
                                  xpointer_t             origin_tag);
  void (* path_changed)          (xobject_t             *target,
                                  xsettings_backend_t    *backend,
                                  const xchar_t         *path,
                                  xpointer_t             origin_tag);
  void (* keys_changed)          (xobject_t             *target,
                                  xsettings_backend_t    *backend,
                                  const xchar_t         *prefix,
                                  xpointer_t             origin_tag,
                                  const xchar_t * const *names);
  void (* writable_changed)      (xobject_t             *target,
                                  xsettings_backend_t    *backend,
                                  const xchar_t         *key);
  void (* path_writable_changed) (xobject_t             *target,
                                  xsettings_backend_t    *backend,
                                  const xchar_t         *path);
} GSettingsListenerVTable;

void                    g_settings_backend_watch                        (xsettings_backend_t               *backend,
                                                                         const GSettingsListenerVTable  *vtable,
                                                                         xobject_t                        *target,
                                                                         xmain_context_t                   *context);
void                    g_settings_backend_unwatch                      (xsettings_backend_t               *backend,
                                                                         xobject_t                        *target);

xtree_t *                 g_settings_backend_create_tree                  (void);

xvariant_t *              g_settings_backend_read                         (xsettings_backend_t               *backend,
                                                                         const xchar_t                    *key,
                                                                         const xvariant_type_t             *expected_type,
                                                                         xboolean_t                        default_value);
xvariant_t *              g_settings_backend_read_user_value              (xsettings_backend_t               *backend,
                                                                         const xchar_t                    *key,
                                                                         const xvariant_type_t             *expected_type);
xboolean_t                g_settings_backend_write                        (xsettings_backend_t               *backend,
                                                                         const xchar_t                    *key,
                                                                         xvariant_t                       *value,
                                                                         xpointer_t                        origin_tag);
xboolean_t                g_settings_backend_write_tree                   (xsettings_backend_t               *backend,
                                                                         xtree_t                          *tree,
                                                                         xpointer_t                        origin_tag);
void                    g_settings_backend_reset                        (xsettings_backend_t               *backend,
                                                                         const xchar_t                    *key,
                                                                         xpointer_t                        origin_tag);
xboolean_t                g_settings_backend_get_writable                 (xsettings_backend_t               *backend,
                                                                         const char                     *key);
void                    g_settings_backend_unsubscribe                  (xsettings_backend_t               *backend,
                                                                         const char                     *name);
void                    g_settings_backend_subscribe                    (xsettings_backend_t               *backend,
                                                                         const char                     *name);
xpermission_t *           g_settings_backend_get_permission               (xsettings_backend_t               *backend,
                                                                         const xchar_t                    *path);
void                    g_settings_backend_sync_default                 (void);

xtype_t                   g_null_settings_backend_get_type                (void);

xtype_t                   xmemory_settings_backend_get_type              (void);

xtype_t                   g_keyfile_settings_backend_get_type             (void);

#ifdef HAVE_COCOA
xtype_t                   g_nextstep_settings_backend_get_type            (void);
#endif

#endif  /* __G_SETTINGS_BACKEND_INTERNAL_H__ */
