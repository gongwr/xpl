/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2014 Руслан Ижбулатов <lrn1986@gmail.com>
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef __G_WIN32_REGISTRY_KEY_H__
#define __G_WIN32_REGISTRY_KEY_H__

#include <gio/gio.h>

#ifdef XPLATFORM_WIN32

G_BEGIN_DECLS

#define XTYPE_WIN32_REGISTRY_KEY            (g_win32_registry_key_get_type ())
#define G_WIN32_REGISTRY_KEY(o)              (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_WIN32_REGISTRY_KEY, GWin32RegistryKey))
#define G_WIN32_REGISTRY_KEY_CLASS(klass)    (XTYPE_CHECK_CLASS_CAST ((klass), XTYPE_WIN32_REGISTRY_KEY, GWin32RegistryKeyClass))
#define X_IS_WIN32_REGISTRY_KEY(o)           (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_WIN32_REGISTRY_KEY))
#define X_IS_WIN32_REGISTRY_KEY_CLASS(klass) (XTYPE_CHECK_CLASS_TYPE ((klass), XTYPE_WIN32_REGISTRY_KEY))
#define G_WIN32_REGISTRY_KEY_GET_CLASS(o)    (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_WIN32_REGISTRY_KEY, GWin32RegistryKeyClass))

typedef enum {
  G_WIN32_REGISTRY_VALUE_NONE = 0,
  G_WIN32_REGISTRY_VALUE_BINARY = 1,
  G_WIN32_REGISTRY_VALUE_UINT32LE = 2,
  G_WIN32_REGISTRY_VALUE_UINT32BE = 3,
#if G_BYTE_ORDER == G_BIG_ENDIAN
  G_WIN32_REGISTRY_VALUE_UINT32 = G_WIN32_REGISTRY_VALUE_UINT32BE,
#else
  G_WIN32_REGISTRY_VALUE_UINT32 = G_WIN32_REGISTRY_VALUE_UINT32LE,
#endif
  G_WIN32_REGISTRY_VALUE_EXPAND_STR = 4,
  G_WIN32_REGISTRY_VALUE_LINK = 5,
  G_WIN32_REGISTRY_VALUE_MULTI_STR = 6,
  G_WIN32_REGISTRY_VALUE_UINT64LE = 7,
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  G_WIN32_REGISTRY_VALUE_UINT64 = G_WIN32_REGISTRY_VALUE_UINT64LE,
#endif
  G_WIN32_REGISTRY_VALUE_STR = 8
} GWin32RegistryValueType;

typedef enum {
  G_WIN32_REGISTRY_WATCH_NAME = 1 << 0,
  G_WIN32_REGISTRY_WATCH_ATTRIBUTES = 1 << 1,
  G_WIN32_REGISTRY_WATCH_VALUES = 1 << 2,
  G_WIN32_REGISTRY_WATCH_SECURITY = 1 << 3,
} GWin32RegistryKeyWatcherFlags;

typedef struct _GWin32RegistryKey GWin32RegistryKey;
typedef struct _GWin32RegistryKeyClass GWin32RegistryKeyClass;
typedef struct _GWin32RegistryKeyPrivate GWin32RegistryKeyPrivate;
typedef struct _GWin32RegistrySubkeyIter GWin32RegistrySubkeyIter;
typedef struct _GWin32RegistryValueIter GWin32RegistryValueIter;

struct _GWin32RegistryKey {
  xobject_t parent_instance;

  /*< private >*/
  GWin32RegistryKeyPrivate *priv;
};

struct _GWin32RegistryKeyClass {
  xobject_class_t parent_class;
};

/**
 * GWin32RegistryKeyWatchCallbackFunc:
 * @key: A #GWin32RegistryKey that was watched.
 * @user_data: The @user_data #xpointer_t passed to g_win32_registry_key_watch().
 *
 * The type of the callback passed to g_win32_registry_key_watch().
 *
 * The callback is invoked after a change matching the watch flags and arguments
 * occurs. If the children of the key were watched also, there is no way to know
 * which one of them triggered the callback.
 *
 * Since: 2.42
 */
typedef void (*GWin32RegistryKeyWatchCallbackFunc) (GWin32RegistryKey  *key,
                                                    xpointer_t            user_data);

#define XTYPE_WIN32_REGISTRY_SUBKEY_ITER (g_win32_registry_subkey_iter_get_type ())

struct _GWin32RegistrySubkeyIter {
  /*< private >*/
  GWin32RegistryKey *key;
  xint_t               counter;
  xint_t               subkey_count;

  xunichar2_t         *subkey_name;
  xsize_t              subkey_name_size;
  xsize_t              subkey_name_len;

  xchar_t             *subkey_name_u8;
};

#define XTYPE_WIN32_REGISTRY_VALUE_ITER (g_win32_registry_value_iter_get_type ())

struct _GWin32RegistryValueIter {
  /*< private >*/
  GWin32RegistryKey       *key;
  xint_t                     counter;
  xint_t                     value_count;

  xunichar2_t               *value_name;
  xsize_t                    value_name_size;
  xsize_t                    value_name_len;
  GWin32RegistryValueType  value_type;
  xuint8_t                  *value_data;
  xsize_t                    value_data_size;
  xsize_t                    value_actual_data_size;
  GWin32RegistryValueType  value_expanded_type;
  xunichar2_t               *value_data_expanded;
  xsize_t                    value_data_expanded_charsize;

  xchar_t                   *value_name_u8;
  xsize_t                    value_name_u8_len;
  xchar_t                   *value_data_u8;
  xsize_t                    value_data_u8_size;
  xchar_t                   *value_data_expanded_u8;
  xsize_t                    value_data_expanded_u8_size;
};

XPL_AVAILABLE_IN_2_46
GWin32RegistrySubkeyIter *g_win32_registry_subkey_iter_copy     (const GWin32RegistrySubkeyIter *iter);
XPL_AVAILABLE_IN_2_46
void                      g_win32_registry_subkey_iter_free     (GWin32RegistrySubkeyIter       *iter);
XPL_AVAILABLE_IN_2_46
void                      g_win32_registry_subkey_iter_assign   (GWin32RegistrySubkeyIter       *iter,
                                                                 const GWin32RegistrySubkeyIter *other);
XPL_AVAILABLE_IN_2_46
xtype_t                     g_win32_registry_subkey_iter_get_type (void) G_GNUC_CONST;


XPL_AVAILABLE_IN_2_46
GWin32RegistryValueIter  *g_win32_registry_value_iter_copy      (const GWin32RegistryValueIter *iter);
XPL_AVAILABLE_IN_2_46
void                      g_win32_registry_value_iter_free      (GWin32RegistryValueIter       *iter);
XPL_AVAILABLE_IN_2_46
void                      g_win32_registry_value_iter_assign    (GWin32RegistryValueIter       *iter,
                                                                 const GWin32RegistryValueIter *other);
XPL_AVAILABLE_IN_2_46
xtype_t                     g_win32_registry_value_iter_get_type  (void) G_GNUC_CONST;


XPL_AVAILABLE_IN_2_46
xtype_t              g_win32_registry_key_get_type             (void);

XPL_AVAILABLE_IN_2_46
GWin32RegistryKey *g_win32_registry_key_new                  (const xchar_t                    *path,
                                                              xerror_t                        **error);

XPL_AVAILABLE_IN_2_46
GWin32RegistryKey *g_win32_registry_key_new_w                (const xunichar2_t                *path,
                                                              xerror_t                        **error);

XPL_AVAILABLE_IN_2_46
GWin32RegistryKey *g_win32_registry_key_get_child            (GWin32RegistryKey              *key,
                                                              const xchar_t                    *subkey,
                                                              xerror_t                        **error);

XPL_AVAILABLE_IN_2_46
GWin32RegistryKey *g_win32_registry_key_get_child_w          (GWin32RegistryKey              *key,
                                                              const xunichar2_t                *subkey,
                                                              xerror_t                        **error);

XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_subkey_iter_init           (GWin32RegistrySubkeyIter       *iter,
                                                              GWin32RegistryKey              *key,
                                                              xerror_t                        **error);
XPL_AVAILABLE_IN_2_46
void             g_win32_registry_subkey_iter_clear          (GWin32RegistrySubkeyIter       *iter);
XPL_AVAILABLE_IN_2_46
xsize_t            g_win32_registry_subkey_iter_n_subkeys      (GWin32RegistrySubkeyIter       *iter);
XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_subkey_iter_next           (GWin32RegistrySubkeyIter       *iter,
                                                              xboolean_t                        skip_errors,
                                                              xerror_t                        **error);
XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_subkey_iter_get_name       (GWin32RegistrySubkeyIter        *iter,
                                                              const xchar_t                    **subkey_name,
                                                              xsize_t                           *subkey_name_len,
                                                              xerror_t                         **error);
XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_subkey_iter_get_name_w     (GWin32RegistrySubkeyIter        *iter,
                                                              const xunichar2_t                **subkey_name,
                                                              xsize_t                           *subkey_name_len,
                                                              xerror_t                         **error);

XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_value_iter_init            (GWin32RegistryValueIter         *iter,
                                                              GWin32RegistryKey               *key,
                                                              xerror_t                         **error);
XPL_AVAILABLE_IN_2_46
void             g_win32_registry_value_iter_clear           (GWin32RegistryValueIter         *iter);
XPL_AVAILABLE_IN_2_46
xsize_t            g_win32_registry_value_iter_n_values        (GWin32RegistryValueIter         *iter);
XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_value_iter_next            (GWin32RegistryValueIter         *iter,
                                                              xboolean_t                         skip_errors,
                                                              xerror_t                         **error);
XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_value_iter_get_value_type  (GWin32RegistryValueIter         *iter,
                                                              GWin32RegistryValueType         *value_type,
                                                              xerror_t                         **error);
XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_value_iter_get_name        (GWin32RegistryValueIter         *iter,
                                                              xchar_t                          **value_name,
                                                              xsize_t                           *value_name_len,
                                                              xerror_t                         **error);
XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_value_iter_get_name_w      (GWin32RegistryValueIter         *iter,
                                                              xunichar2_t                      **value_name,
                                                              xsize_t                           *value_name_len,
                                                              xerror_t                         **error);
XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_value_iter_get_data        (GWin32RegistryValueIter         *iter,
                                                              xboolean_t                         auto_expand,
                                                              xpointer_t                        *value_data,
                                                              xsize_t                           *value_data_size,
                                                              xerror_t                         **error);
XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_value_iter_get_data_w      (GWin32RegistryValueIter         *iter,
                                                              xboolean_t                         auto_expand,
                                                              xpointer_t                        *value_data,
                                                              xsize_t                           *value_data_size,
                                                              xerror_t                         **error);

XPL_AVAILABLE_IN_2_66
xboolean_t         g_win32_registry_key_get_value              (GWin32RegistryKey               *key,
                                                              const xchar_t * const             *mui_dll_dirs,
                                                              xboolean_t                         auto_expand,
                                                              const xchar_t                     *value_name,
                                                              GWin32RegistryValueType         *value_type,
                                                              xpointer_t                        *value_data,
                                                              xsize_t                           *value_data_size,
                                                              xerror_t                         **error);

XPL_AVAILABLE_IN_2_66
xboolean_t         g_win32_registry_key_get_value_w            (GWin32RegistryKey               *key,
                                                              const xunichar2_t * const         *mui_dll_dirs,
                                                              xboolean_t                         auto_expand,
                                                              const xunichar2_t                 *value_name,
                                                              GWin32RegistryValueType         *value_type,
                                                              xpointer_t                        *value_data,
                                                              xsize_t                           *value_data_size,
                                                              xerror_t                         **error);

XPL_AVAILABLE_IN_2_46
const xchar_t     *g_win32_registry_key_get_path               (GWin32RegistryKey               *key);

XPL_AVAILABLE_IN_2_46
const xunichar2_t *g_win32_registry_key_get_path_w             (GWin32RegistryKey               *key);

XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_key_watch                  (GWin32RegistryKey               *key,
                                                              xboolean_t                         watch_children,
                                                              GWin32RegistryKeyWatcherFlags    watch_flags,
                                                              GWin32RegistryKeyWatchCallbackFunc callback,
                                                              xpointer_t                         user_data,
                                                              xerror_t                         **error);
XPL_AVAILABLE_IN_2_46
xboolean_t         g_win32_registry_key_has_changed            (GWin32RegistryKey               *key);

XPL_AVAILABLE_IN_2_46
void             g_win32_registry_key_erase_change_indicator (GWin32RegistryKey               *key);

XPL_AVAILABLE_IN_2_66
const xunichar2_t * const *g_win32_registry_get_os_dirs_w (void);

XPL_AVAILABLE_IN_2_66
const xchar_t * const     *g_win32_registry_get_os_dirs   (void);

G_END_DECLS

#endif /* XPLATFORM_WIN32 */

#endif /* __G_WIN32_REGISTRY_KEY_H__ */
