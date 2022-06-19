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

#ifndef __XDELAYED_SETTINGS_BACKEND_H__
#define __XDELAYED_SETTINGS_BACKEND_H__

#include <glib-object.h>

#include <gio/gsettingsbackend.h>

#define XTYPE_DELAYED_SETTINGS_BACKEND                     (xdelayed_settings_backend_get_type ())
#define XDELAYED_SETTINGS_BACKEND(inst)                    (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_DELAYED_SETTINGS_BACKEND,                        \
                                                             xdelayed_settings_backend_t))
#define XDELAYED_SETTINGS_BACKEND_CLASS(class)             (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_DELAYED_SETTINGS_BACKEND,                        \
                                                             xdelayed_settings_backend_class_t))
#define X_IS_DELAYED_SETTINGS_BACKEND(inst)                 (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_DELAYED_SETTINGS_BACKEND))
#define X_IS_DELAYED_SETTINGS_BACKEND_CLASS(class)          (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_DELAYED_SETTINGS_BACKEND))
#define XDELAYED_SETTINGS_BACKEND_GET_CLASS(inst)          (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_DELAYED_SETTINGS_BACKEND,                        \
                                                             xdelayed_settings_backend_class_t))

typedef struct _xdelayed_settings_backend_private              GDelayedSettingsBackendPrivate;
typedef struct _xdelayed_settings_backend_class                xdelayed_settings_backend_class_t;
typedef struct _xdelayed_settings_backend                     xdelayed_settings_backend_t;

struct _xdelayed_settings_backend_class
{
  xsettings_backend_class_t parent_class;
};

struct _xdelayed_settings_backend
{
  xsettings_backend_t parent_instance;
  GDelayedSettingsBackendPrivate *priv;
};

xtype_t                           xdelayed_settings_backend_get_type     (void);
xdelayed_settings_backend_t *       xdelayed_settings_backend_new          (xsettings_backend_t        *backend,
                                                                         xpointer_t                 owner,
                                                                         xmain_context_t            *owner_context);
void                            xdelayed_settings_backend_revert       (xdelayed_settings_backend_t *delayed);
void                            xdelayed_settings_backend_apply        (xdelayed_settings_backend_t *delayed);
xboolean_t                        xdelayed_settings_backend_get_has_unapplied (xdelayed_settings_backend_t *delayed);

#endif  /* __XDELAYED_SETTINGS_BACKEND_H__ */
