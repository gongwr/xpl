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

#ifndef __G_SETTINGS_H__
#define __G_SETTINGS_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gsettingsschema.h>
#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SETTINGS                                     (g_settings_get_type ())
#define G_SETTINGS(inst)                                    (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_SETTINGS, xsettings))
#define G_SETTINGS_CLASS(class)                             (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_SETTINGS, GSettingsClass))
#define X_IS_SETTINGS(inst)                                 (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_SETTINGS))
#define X_IS_SETTINGS_CLASS(class)                          (XTYPE_CHECK_CLASS_TYPE ((class), XTYPE_SETTINGS))
#define G_SETTINGS_GET_CLASS(inst)                          (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_SETTINGS, GSettingsClass))

typedef struct _GSettingsPrivate                            GSettingsPrivate;
typedef struct _GSettingsClass                              GSettingsClass;

struct _GSettingsClass
{
  xobject_class_t parent_class;

  /* Signals */
  void        (*writable_changed)      (xsettings_t    *settings,
                                        const xchar_t  *key);
  void        (*changed)               (xsettings_t    *settings,
                                        const xchar_t  *key);
  xboolean_t    (*writable_change_event) (xsettings_t    *settings,
                                        xquark        key);
  xboolean_t    (*change_event)          (xsettings_t    *settings,
                                        const xquark *keys,
                                        xint_t          n_keys);

  xpointer_t padding[20];
};

struct _GSettings
{
  xobject_t parent_instance;
  GSettingsPrivate *priv;
};


XPL_AVAILABLE_IN_ALL
xtype_t                   g_settings_get_type                             (void);

XPL_DEPRECATED_IN_2_40_FOR(g_settings_schema_source_list_schemas)
const xchar_t * const *   g_settings_list_schemas                         (void);
XPL_DEPRECATED_IN_2_40_FOR(g_settings_schema_source_list_schemas)
const xchar_t * const *   g_settings_list_relocatable_schemas             (void);
XPL_AVAILABLE_IN_ALL
xsettings_t *             g_settings_new                                  (const xchar_t        *schema_id);
XPL_AVAILABLE_IN_ALL
xsettings_t *             g_settings_new_with_path                        (const xchar_t        *schema_id,
                                                                         const xchar_t        *path);
XPL_AVAILABLE_IN_ALL
xsettings_t *             g_settings_new_with_backend                     (const xchar_t        *schema_id,
                                                                         xsettings_backend_t   *backend);
XPL_AVAILABLE_IN_ALL
xsettings_t *             g_settings_new_with_backend_and_path            (const xchar_t        *schema_id,
                                                                         xsettings_backend_t   *backend,
                                                                         const xchar_t        *path);
XPL_AVAILABLE_IN_2_32
xsettings_t *             g_settings_new_full                             (xsettings_schema_t    *schema,
                                                                         xsettings_backend_t   *backend,
                                                                         const xchar_t        *path);
XPL_AVAILABLE_IN_ALL
xchar_t **                g_settings_list_children                        (xsettings_t          *settings);
XPL_DEPRECATED_IN_2_46_FOR(g_settings_schema_list_keys)
xchar_t **                g_settings_list_keys                            (xsettings_t          *settings);
XPL_DEPRECATED_IN_2_40_FOR(g_settings_schema_key_get_range)
xvariant_t *              g_settings_get_range                            (xsettings_t          *settings,
                                                                         const xchar_t        *key);
XPL_DEPRECATED_IN_2_40_FOR(g_settings_schema_key_range_check)
xboolean_t                g_settings_range_check                          (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         xvariant_t           *value);

XPL_AVAILABLE_IN_ALL
xboolean_t                g_settings_set_value                            (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         xvariant_t           *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *              g_settings_get_value                            (xsettings_t          *settings,
                                                                         const xchar_t        *key);

XPL_AVAILABLE_IN_2_40
xvariant_t *              g_settings_get_user_value                       (xsettings_t          *settings,
                                                                         const xchar_t        *key);
XPL_AVAILABLE_IN_2_40
xvariant_t *              g_settings_get_default_value                    (xsettings_t          *settings,
                                                                         const xchar_t        *key);

XPL_AVAILABLE_IN_ALL
xboolean_t                g_settings_set                                  (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         const xchar_t        *format,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
void                    g_settings_get                                  (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         const xchar_t        *format,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
void                    g_settings_reset                                (xsettings_t          *settings,
                                                                         const xchar_t        *key);

XPL_AVAILABLE_IN_ALL
xint_t                    g_settings_get_int                              (xsettings_t          *settings,
                                                                         const xchar_t        *key);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_settings_set_int                              (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         xint_t                value);
XPL_AVAILABLE_IN_2_50
gint64                  g_settings_get_int64                            (xsettings_t          *settings,
                                                                         const xchar_t        *key);
XPL_AVAILABLE_IN_2_50
xboolean_t                g_settings_set_int64                            (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         gint64              value);
XPL_AVAILABLE_IN_2_32
xuint_t                   g_settings_get_uint                             (xsettings_t          *settings,
                                                                         const xchar_t        *key);
XPL_AVAILABLE_IN_2_32
xboolean_t                g_settings_set_uint                             (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         xuint_t               value);
XPL_AVAILABLE_IN_2_50
xuint64_t                 g_settings_get_uint64                           (xsettings_t          *settings,
                                                                         const xchar_t        *key);
XPL_AVAILABLE_IN_2_50
xboolean_t                g_settings_set_uint64                           (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         xuint64_t             value);
XPL_AVAILABLE_IN_ALL
xchar_t *                 g_settings_get_string                           (xsettings_t          *settings,
                                                                         const xchar_t        *key);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_settings_set_string                           (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         const xchar_t        *value);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_settings_get_boolean                          (xsettings_t          *settings,
                                                                         const xchar_t        *key);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_settings_set_boolean                          (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         xboolean_t            value);
XPL_AVAILABLE_IN_ALL
xdouble_t                 g_settings_get_double                           (xsettings_t          *settings,
                                                                         const xchar_t        *key);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_settings_set_double                           (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         xdouble_t             value);
XPL_AVAILABLE_IN_ALL
xchar_t **                g_settings_get_strv                             (xsettings_t          *settings,
                                                                         const xchar_t        *key);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_settings_set_strv                             (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         const xchar_t *const *value);
XPL_AVAILABLE_IN_ALL
xint_t                    g_settings_get_enum                             (xsettings_t          *settings,
                                                                         const xchar_t        *key);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_settings_set_enum                             (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         xint_t                value);
XPL_AVAILABLE_IN_ALL
xuint_t                   g_settings_get_flags                            (xsettings_t          *settings,
                                                                         const xchar_t        *key);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_settings_set_flags                            (xsettings_t          *settings,
                                                                         const xchar_t        *key,
                                                                         xuint_t               value);
XPL_AVAILABLE_IN_ALL
xsettings_t *             g_settings_get_child                            (xsettings_t          *settings,
                                                                         const xchar_t        *name);

XPL_AVAILABLE_IN_ALL
xboolean_t                g_settings_is_writable                          (xsettings_t          *settings,
                                                                         const xchar_t        *name);

XPL_AVAILABLE_IN_ALL
void                    g_settings_delay                                (xsettings_t          *settings);
XPL_AVAILABLE_IN_ALL
void                    g_settings_apply                                (xsettings_t          *settings);
XPL_AVAILABLE_IN_ALL
void                    g_settings_revert                               (xsettings_t          *settings);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_settings_get_has_unapplied                    (xsettings_t          *settings);
XPL_AVAILABLE_IN_ALL
void                    g_settings_sync                                 (void);

/**
 * GSettingsBindSetMapping:
 * @value: a #xvalue_t containing the property value to map
 * @expected_type: the #xvariant_type_t to create
 * @user_data: user data that was specified when the binding was created
 *
 * The type for the function that is used to convert an object property
 * value to a #xvariant_t for storing it in #xsettings_t.
 *
 * Returns: a new #xvariant_t holding the data from @value,
 *     or %NULL in case of an error
 */
typedef xvariant_t *    (*GSettingsBindSetMapping)                        (const xvalue_t       *value,
                                                                         const xvariant_type_t *expected_type,
                                                                         xpointer_t            user_data);

/**
 * GSettingsBindGetMapping:
 * @value: return location for the property value
 * @variant: the #xvariant_t
 * @user_data: user data that was specified when the binding was created
 *
 * The type for the function that is used to convert from #xsettings_t to
 * an object property. The @value is already initialized to hold values
 * of the appropriate type.
 *
 * Returns: %TRUE if the conversion succeeded, %FALSE in case of an error
 */
typedef xboolean_t      (*GSettingsBindGetMapping)                        (xvalue_t             *value,
                                                                         xvariant_t           *variant,
                                                                         xpointer_t            user_data);

/**
 * GSettingsGetMapping:
 * @value: the #xvariant_t to map, or %NULL
 * @result: (out): the result of the mapping
 * @user_data: (closure): the user data that was passed to
 * g_settings_get_mapped()
 *
 * The type of the function that is used to convert from a value stored
 * in a #xsettings_t to a value that is useful to the application.
 *
 * If the value is successfully mapped, the result should be stored at
 * @result and %TRUE returned.  If mapping fails (for example, if @value
 * is not in the right format) then %FALSE should be returned.
 *
 * If @value is %NULL then it means that the mapping function is being
 * given a "last chance" to successfully return a valid value.  %TRUE
 * must be returned in this case.
 *
 * Returns: %TRUE if the conversion succeeded, %FALSE in case of an error
 **/
typedef xboolean_t      (*GSettingsGetMapping)                            (xvariant_t           *value,
                                                                         xpointer_t           *result,
                                                                         xpointer_t            user_data);

/**
 * GSettingsBindFlags:
 * @G_SETTINGS_BIND_DEFAULT: Equivalent to `G_SETTINGS_BIND_GET|G_SETTINGS_BIND_SET`
 * @G_SETTINGS_BIND_GET: Update the #xobject_t property when the setting changes.
 *     It is an error to use this flag if the property is not writable.
 * @G_SETTINGS_BIND_SET: Update the setting when the #xobject_t property changes.
 *     It is an error to use this flag if the property is not readable.
 * @G_SETTINGS_BIND_NO_SENSITIVITY: Do not try to bind a "sensitivity" property to the writability of the setting
 * @G_SETTINGS_BIND_GET_NO_CHANGES: When set in addition to %G_SETTINGS_BIND_GET, set the #xobject_t property
 *     value initially from the setting, but do not listen for changes of the setting
 * @G_SETTINGS_BIND_INVERT_BOOLEAN: When passed to g_settings_bind(), uses a pair of mapping functions that invert
 *     the boolean value when mapping between the setting and the property.  The setting and property must both
 *     be booleans.  You cannot pass this flag to g_settings_bind_with_mapping().
 *
 * Flags used when creating a binding. These flags determine in which
 * direction the binding works. The default is to synchronize in both
 * directions.
 */
typedef enum
{
  G_SETTINGS_BIND_DEFAULT,
  G_SETTINGS_BIND_GET            = (1<<0),
  G_SETTINGS_BIND_SET            = (1<<1),
  G_SETTINGS_BIND_NO_SENSITIVITY = (1<<2),
  G_SETTINGS_BIND_GET_NO_CHANGES = (1<<3),
  G_SETTINGS_BIND_INVERT_BOOLEAN = (1<<4)
} GSettingsBindFlags;

XPL_AVAILABLE_IN_ALL
void                    g_settings_bind                                 (xsettings_t               *settings,
                                                                         const xchar_t             *key,
                                                                         xpointer_t                 object,
                                                                         const xchar_t             *property,
                                                                         GSettingsBindFlags       flags);
XPL_AVAILABLE_IN_ALL
void                    g_settings_bind_with_mapping                    (xsettings_t               *settings,
                                                                         const xchar_t             *key,
                                                                         xpointer_t                 object,
                                                                         const xchar_t             *property,
                                                                         GSettingsBindFlags       flags,
                                                                         GSettingsBindGetMapping  get_mapping,
                                                                         GSettingsBindSetMapping  set_mapping,
                                                                         xpointer_t                 user_data,
                                                                         xdestroy_notify_t           destroy);
XPL_AVAILABLE_IN_ALL
void                    g_settings_bind_writable                        (xsettings_t               *settings,
                                                                         const xchar_t             *key,
                                                                         xpointer_t                 object,
                                                                         const xchar_t             *property,
                                                                         xboolean_t                 inverted);
XPL_AVAILABLE_IN_ALL
void                    g_settings_unbind                               (xpointer_t                 object,
                                                                         const xchar_t             *property);

XPL_AVAILABLE_IN_2_32
xaction_t *               g_settings_create_action                        (xsettings_t               *settings,
                                                                         const xchar_t             *key);

XPL_AVAILABLE_IN_ALL
xpointer_t                g_settings_get_mapped                           (xsettings_t               *settings,
                                                                         const xchar_t             *key,
                                                                         GSettingsGetMapping      mapping,
                                                                         xpointer_t                 user_data);

G_END_DECLS

#endif  /* __G_SETTINGS_H__ */
