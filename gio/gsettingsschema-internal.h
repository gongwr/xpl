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
 */

#ifndef __G_SETTINGS_SCHEMA_INTERNAL_H__
#define __G_SETTINGS_SCHEMA_INTERNAL_H__

#include "gsettingsschema.h"

struct _GSettingsSchemaKey
{
  GSettingsSchema *schema;
  const xchar_t *name;

  xuint_t is_flags : 1;
  xuint_t is_enum  : 1;

  const guint32 *strinfo;
  xsize_t strinfo_length;

  const xchar_t *unparsed;
  xchar_t lc_char;

  const xvariant_type_t *type;
  xvariant_t *minimum, *maximum;
  xvariant_t *default_value;
  xvariant_t *desktop_overrides;

  xint_t ref_count;
};

const xchar_t *           g_settings_schema_get_gettext_domain            (GSettingsSchema  *schema);
GVariantIter *          g_settings_schema_get_value                     (GSettingsSchema  *schema,
                                                                         const xchar_t      *key);
const GQuark *          g_settings_schema_list                          (GSettingsSchema  *schema,
                                                                         xint_t             *n_items);
const xchar_t *           g_settings_schema_get_string                    (GSettingsSchema  *schema,
                                                                         const xchar_t      *key);

GSettingsSchema *       g_settings_schema_get_child_schema              (GSettingsSchema *schema,
                                                                         const xchar_t     *name);

void                    g_settings_schema_key_init                      (GSettingsSchemaKey *key,
                                                                         GSettingsSchema    *schema,
                                                                         const xchar_t        *name);
void                    g_settings_schema_key_clear                     (GSettingsSchemaKey *key);
xboolean_t                g_settings_schema_key_type_check                (GSettingsSchemaKey *key,
                                                                         xvariant_t           *value);
xvariant_t *              g_settings_schema_key_range_fixup               (GSettingsSchemaKey *key,
                                                                         xvariant_t           *value);
xvariant_t *              g_settings_schema_key_get_translated_default    (GSettingsSchemaKey *key);
xvariant_t *              g_settings_schema_key_get_per_desktop_default   (GSettingsSchemaKey *key);

xint_t                    g_settings_schema_key_to_enum                   (GSettingsSchemaKey *key,
                                                                         xvariant_t           *value);
xvariant_t *              g_settings_schema_key_from_enum                 (GSettingsSchemaKey *key,
                                                                         xint_t                value);
xuint_t                   g_settings_schema_key_to_flags                  (GSettingsSchemaKey *key,
                                                                         xvariant_t           *value);
xvariant_t *              g_settings_schema_key_from_flags                (GSettingsSchemaKey *key,
                                                                         xuint_t               value);

#endif /* __G_SETTINGS_SCHEMA_INTERNAL_H__ */
