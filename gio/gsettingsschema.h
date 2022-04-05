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

typedef struct _GSettingsSchemaSource                       GSettingsSchemaSource;
typedef struct _GSettingsSchema                             GSettingsSchema;
typedef struct _GSettingsSchemaKey                          GSettingsSchemaKey;

#define                 XTYPE_SETTINGS_SCHEMA_SOURCE                   (g_settings_schema_source_get_type ())
XPL_AVAILABLE_IN_2_32
xtype_t                   g_settings_schema_source_get_type               (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_32
GSettingsSchemaSource * g_settings_schema_source_get_default            (void);
XPL_AVAILABLE_IN_2_32
GSettingsSchemaSource * g_settings_schema_source_ref                    (GSettingsSchemaSource  *source);
XPL_AVAILABLE_IN_2_32
void                    g_settings_schema_source_unref                  (GSettingsSchemaSource  *source);

XPL_AVAILABLE_IN_2_32
GSettingsSchemaSource * g_settings_schema_source_new_from_directory     (const xchar_t            *directory,
                                                                         GSettingsSchemaSource  *parent,
                                                                         xboolean_t                trusted,
                                                                         xerror_t                **error);

XPL_AVAILABLE_IN_2_32
GSettingsSchema *       g_settings_schema_source_lookup                 (GSettingsSchemaSource  *source,
                                                                         const xchar_t            *schema_id,
                                                                         xboolean_t                recursive);

XPL_AVAILABLE_IN_2_40
void                    g_settings_schema_source_list_schemas           (GSettingsSchemaSource   *source,
                                                                         xboolean_t                 recursive,
                                                                         xchar_t                 ***non_relocatable,
                                                                         xchar_t                 ***relocatable);

#define                 XTYPE_SETTINGS_SCHEMA                          (g_settings_schema_get_type ())
XPL_AVAILABLE_IN_2_32
xtype_t                   g_settings_schema_get_type                      (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_32
GSettingsSchema *       g_settings_schema_ref                           (GSettingsSchema        *schema);
XPL_AVAILABLE_IN_2_32
void                    g_settings_schema_unref                         (GSettingsSchema        *schema);

XPL_AVAILABLE_IN_2_32
const xchar_t *           g_settings_schema_get_id                        (GSettingsSchema        *schema);
XPL_AVAILABLE_IN_2_32
const xchar_t *           g_settings_schema_get_path                      (GSettingsSchema        *schema);
XPL_AVAILABLE_IN_2_40
GSettingsSchemaKey *    g_settings_schema_get_key                       (GSettingsSchema        *schema,
                                                                         const xchar_t            *name);
XPL_AVAILABLE_IN_2_40
xboolean_t                g_settings_schema_has_key                       (GSettingsSchema        *schema,
                                                                         const xchar_t            *name);
XPL_AVAILABLE_IN_2_46
xchar_t**                 g_settings_schema_list_keys                     (GSettingsSchema        *schema);


XPL_AVAILABLE_IN_2_44
xchar_t **                g_settings_schema_list_children                 (GSettingsSchema        *schema);

#define                 XTYPE_SETTINGS_SCHEMA_KEY                      (g_settings_schema_key_get_type ())
XPL_AVAILABLE_IN_2_40
xtype_t                   g_settings_schema_key_get_type                  (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_40
GSettingsSchemaKey *    g_settings_schema_key_ref                       (GSettingsSchemaKey     *key);
XPL_AVAILABLE_IN_2_40
void                    g_settings_schema_key_unref                     (GSettingsSchemaKey     *key);

XPL_AVAILABLE_IN_2_40
const xvariant_type_t *    g_settings_schema_key_get_value_type            (GSettingsSchemaKey     *key);
XPL_AVAILABLE_IN_2_40
xvariant_t *              g_settings_schema_key_get_default_value         (GSettingsSchemaKey     *key);
XPL_AVAILABLE_IN_2_40
xvariant_t *              g_settings_schema_key_get_range                 (GSettingsSchemaKey     *key);
XPL_AVAILABLE_IN_2_40
xboolean_t                g_settings_schema_key_range_check               (GSettingsSchemaKey     *key,
                                                                         xvariant_t               *value);

XPL_AVAILABLE_IN_2_44
const xchar_t *           g_settings_schema_key_get_name                  (GSettingsSchemaKey     *key);
XPL_AVAILABLE_IN_2_40
const xchar_t *           g_settings_schema_key_get_summary               (GSettingsSchemaKey     *key);
XPL_AVAILABLE_IN_2_40
const xchar_t *           g_settings_schema_key_get_description           (GSettingsSchemaKey     *key);

G_END_DECLS

#endif /* __G_SETTINGS_SCHEMA_H__ */
