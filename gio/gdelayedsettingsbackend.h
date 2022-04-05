/*
 * Copyright © 2009, 2010 Codethink Limited
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

#ifndef __G_DELAYED_SETTINGS_BACKEND_H__
#define __G_DELAYED_SETTINGS_BACKEND_H__

#include <glib-object.h>

#include <gio/gsettingsbackend.h>

#define XTYPE_DELAYED_SETTINGS_BACKEND                     (g_delayed_settings_backend_get_type ())
#define G_DELAYED_SETTINGS_BACKEND(inst)                    (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_DELAYED_SETTINGS_BACKEND,                        \
                                                             GDelayedSettingsBackend))
#define G_DELAYED_SETTINGS_BACKEND_CLASS(class)             (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_DELAYED_SETTINGS_BACKEND,                        \
                                                             GDelayedSettingsBackendClass))
#define X_IS_DELAYED_SETTINGS_BACKEND(inst)                 (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_DELAYED_SETTINGS_BACKEND))
#define X_IS_DELAYED_SETTINGS_BACKEND_CLASS(class)          (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_DELAYED_SETTINGS_BACKEND))
#define G_DELAYED_SETTINGS_BACKEND_GET_CLASS(inst)          (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_DELAYED_SETTINGS_BACKEND,                        \
                                                             GDelayedSettingsBackendClass))

typedef struct _GDelayedSettingsBackendPrivate              GDelayedSettingsBackendPrivate;
typedef struct _GDelayedSettingsBackendClass                GDelayedSettingsBackendClass;
typedef struct _GDelayedSettingsBackend                     GDelayedSettingsBackend;

struct _GDelayedSettingsBackendClass
{
  GSettingsBackendClass parent_class;
};

struct _GDelayedSettingsBackend
{
  GSettingsBackend parent_instance;
  GDelayedSettingsBackendPrivate *priv;
};

xtype_t                           g_delayed_settings_backend_get_type     (void);
GDelayedSettingsBackend *       g_delayed_settings_backend_new          (GSettingsBackend        *backend,
                                                                         xpointer_t                 owner,
                                                                         GMainContext            *owner_context);
void                            g_delayed_settings_backend_revert       (GDelayedSettingsBackend *delayed);
void                            g_delayed_settings_backend_apply        (GDelayedSettingsBackend *delayed);
xboolean_t                        g_delayed_settings_backend_get_has_unapplied (GDelayedSettingsBackend *delayed);

#endif  /* __G_DELAYED_SETTINGS_BACKEND_H__ */
