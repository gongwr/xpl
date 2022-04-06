/*
 * Copyright © 2010 Codethink Limited
 * Copyright © 2011 Canonical Limited
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
 */

#ifndef __G_SETTINGS_SCHEMA_H__
#define __G_SETTINGS_SCHEMA_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GSettingsSchemaSource                       xsettings_schema_source_t;
typedef struct _GSettingsSchema                             xsettings_schema_t;
typedef struct _GSettingsSchemaKey                          xsettings_schema_key_t;

#define                 XTYPE_SETTINGS_SCHEMA_SOURCE                   (g_settings_schema_source_get_type ())
XPL_AVAILABLE_IN_2_32
xtype_t                   g_settings_schema_source_get_type               (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_32
xsettings_schema_source_t * g_settings_schema_source_get_default            (void);
XPL_AVAILABLE_IN_2_32
xsettings_schema_source_t * g_settings_schema_source_ref                    (xsettings_schema_source_t  *source);
XPL_AVAILABLE_IN_2_32
void                    g_settings_schema_source_unref                  (xsettings_schema_source_t  *source);

XPL_AVAILABLE_IN_2_32
xsettings_schema_source_t * g_settings_schema_source_new_from_directory     (const xchar_t            *directory,
                                                                         xsettings_schema_source_t  *parent,
                                                                         xboolean_t                trusted,
                                                                         xerror_t                **error);

XPL_AVAILABLE_IN_2_32
xsettings_schema_t *       g_settings_schema_source_lookup                 (xsettings_schema_source_t  *source,
                                                                         const xchar_t            *schema_id,
                                                                         xboolean_t                recursive);

XPL_AVAILABLE_IN_2_40
void                    g_settings_schema_source_list_schemas           (xsettings_schema_source_t   *source,
                                                                         xboolean_t                 recursive,
                                                                         xchar_t                 ***non_relocatable,
                                                                         xchar_t                 ***relocatable);

#define                 XTYPE_SETTINGS_SCHEMA                          (g_settings_schema_get_type ())
XPL_AVAILABLE_IN_2_32
xtype_t                   g_settings_schema_get_type                      (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_32
xsettings_schema_t *       g_settings_schema_ref                           (xsettings_schema_t        *schema);
XPL_AVAILABLE_IN_2_32
void                    g_settings_schema_unref                         (xsettings_schema_t        *schema);

XPL_AVAILABLE_IN_2_32
const xchar_t *           g_settings_schema_get_id                        (xsettings_schema_t        *schema);
XPL_AVAILABLE_IN_2_32
const xchar_t *           g_settings_schema_get_path                      (xsettings_schema_t        *schema);
XPL_AVAILABLE_IN_2_40
xsettings_schema_key_t *    g_settings_schema_get_key                       (xsettings_schema_t        *schema,
                                                                         const xchar_t            *name);
XPL_AVAILABLE_IN_2_40
xboolean_t                g_settings_schema_has_key                       (xsettings_schema_t        *schema,
                                                                         const xchar_t            *name);
XPL_AVAILABLE_IN_2_46
xchar_t**                 g_settings_schema_list_keys                     (xsettings_schema_t        *schema);


XPL_AVAILABLE_IN_2_44
xchar_t **                g_settings_schema_list_children                 (xsettings_schema_t        *schema);

#define                 XTYPE_SETTINGS_SCHEMA_KEY                      (g_settings_schema_key_get_type ())
XPL_AVAILABLE_IN_2_40
xtype_t                   g_settings_schema_key_get_type                  (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_40
xsettings_schema_key_t *    g_settings_schema_key_ref                       (xsettings_schema_key_t     *key);
XPL_AVAILABLE_IN_2_40
void                    g_settings_schema_key_unref                     (xsettings_schema_key_t     *key);

XPL_AVAILABLE_IN_2_40
const xvariant_type_t *    g_settings_schema_key_get_value_type            (xsettings_schema_key_t     *key);
XPL_AVAILABLE_IN_2_40
xvariant_t *              g_settings_schema_key_get_default_value         (xsettings_schema_key_t     *key);
XPL_AVAILABLE_IN_2_40
xvariant_t *              g_settings_schema_key_get_range                 (xsettings_schema_key_t     *key);
XPL_AVAILABLE_IN_2_40
xboolean_t                g_settings_schema_key_range_check               (xsettings_schema_key_t     *key,
                                                                         xvariant_t               *value);

XPL_AVAILABLE_IN_2_44
const xchar_t *           g_settings_schema_key_get_name                  (xsettings_schema_key_t     *key);
XPL_AVAILABLE_IN_2_40
const xchar_t *           g_settings_schema_key_get_summary               (xsettings_schema_key_t     *key);
XPL_AVAILABLE_IN_2_40
const xchar_t *           g_settings_schema_key_get_description           (xsettings_schema_key_t     *key);

G_END_DECLS

#endif /* __G_SETTINGS_SCHEMA_H__ */
