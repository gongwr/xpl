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
  xsettings_schema_t *schema;
  const xchar_t *name;

  xuint_t is_flags : 1;
  xuint_t is_enum  : 1;

  const xuint32_t *strinfo;
  xsize_t strinfo_length;

  const xchar_t *unparsed;
  xchar_t lc_char;

  const xvariant_type_t *type;
  xvariant_t *minimum, *maximum;
  xvariant_t *default_value;
  xvariant_t *desktop_overrides;

  xint_t ref_count;
};

const xchar_t *           g_settings_schema_get_gettext_domain            (xsettings_schema_t  *schema);
xvariant_iter_t *          g_settings_schema_get_value                     (xsettings_schema_t  *schema,
                                                                         const xchar_t      *key);
const xquark *          g_settings_schema_list                          (xsettings_schema_t  *schema,
                                                                         xint_t             *n_items);
const xchar_t *           g_settings_schema_get_string                    (xsettings_schema_t  *schema,
                                                                         const xchar_t      *key);

xsettings_schema_t *       g_settings_schema_get_child_schema              (xsettings_schema_t *schema,
                                                                         const xchar_t     *name);

void                    g_settings_schema_key_init                      (xsettings_schema_key_t *key,
                                                                         xsettings_schema_t    *schema,
                                                                         const xchar_t        *name);
void                    g_settings_schema_key_clear                     (xsettings_schema_key_t *key);
xboolean_t                g_settings_schema_key_type_check                (xsettings_schema_key_t *key,
                                                                         xvariant_t           *value);
xvariant_t *              g_settings_schema_key_range_fixup               (xsettings_schema_key_t *key,
                                                                         xvariant_t           *value);
xvariant_t *              g_settings_schema_key_get_translated_default    (xsettings_schema_key_t *key);
xvariant_t *              g_settings_schema_key_get_per_desktop_default   (xsettings_schema_key_t *key);

xint_t                    g_settings_schema_key_to_enum                   (xsettings_schema_key_t *key,
                                                                         xvariant_t           *value);
xvariant_t *              g_settings_schema_key_from_enum                 (xsettings_schema_key_t *key,
                                                                         xint_t                value);
xuint_t                   g_settings_schema_key_to_flags                  (xsettings_schema_key_t *key,
                                                                         xvariant_t           *value);
xvariant_t *              g_settings_schema_key_from_flags                (xsettings_schema_key_t *key,
                                                                         xuint_t               value);

#endif /* __G_SETTINGS_SCHEMA_INTERNAL_H__ */
