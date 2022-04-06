/* gkeyfile.h - desktop entry file parser
 *
 *  Copyright 2004 Red Hat, Inc.
 *
 *  Ray Strode <halfline@hawaii.rr.com>
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __G_KEY_FILE_H__
#define __G_KEY_FILE_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gbytes.h>
#include <glib/gerror.h>

G_BEGIN_DECLS

typedef enum
{
  G_KEY_FILE_ERROR_UNKNOWN_ENCODING,
  G_KEY_FILE_ERROR_PARSE,
  G_KEY_FILE_ERROR_NOT_FOUND,
  G_KEY_FILE_ERROR_KEY_NOT_FOUND,
  G_KEY_FILE_ERROR_GROUP_NOT_FOUND,
  G_KEY_FILE_ERROR_INVALID_VALUE
} GKeyFileError;

#define G_KEY_FILE_ERROR xkey_file_error_quark()

XPL_AVAILABLE_IN_ALL
xquark xkey_file_error_quark (void);

typedef struct _GKeyFile xkey_file_t;

typedef enum
{
  G_KEY_FILE_NONE              = 0,
  G_KEY_FILE_KEEP_COMMENTS     = 1 << 0,
  G_KEY_FILE_KEEP_TRANSLATIONS = 1 << 1
} GKeyFileFlags;

XPL_AVAILABLE_IN_ALL
xkey_file_t *xkey_file_new                    (void);
XPL_AVAILABLE_IN_ALL
xkey_file_t *xkey_file_ref                    (xkey_file_t             *key_file);
XPL_AVAILABLE_IN_ALL
void      xkey_file_unref                  (xkey_file_t             *key_file);
XPL_AVAILABLE_IN_ALL
void      xkey_file_free                   (xkey_file_t             *key_file);
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_list_separator     (xkey_file_t             *key_file,
					     xchar_t                 separator);
XPL_AVAILABLE_IN_ALL
xboolean_t  xkey_file_load_from_file         (xkey_file_t             *key_file,
					     const xchar_t          *file,
					     GKeyFileFlags         flags,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xboolean_t  xkey_file_load_from_data         (xkey_file_t             *key_file,
					     const xchar_t          *data,
					     xsize_t                 length,
					     GKeyFileFlags         flags,
					     xerror_t              **error);
XPL_AVAILABLE_IN_2_50
xboolean_t  xkey_file_load_from_bytes        (xkey_file_t             *key_file,
                                             xbytes_t               *bytes,
                                             GKeyFileFlags         flags,
                                             xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xboolean_t xkey_file_load_from_dirs          (xkey_file_t             *key_file,
					     const xchar_t	  *file,
					     const xchar_t	 **search_dirs,
					     xchar_t		 **full_path,
					     GKeyFileFlags         flags,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xboolean_t xkey_file_load_from_data_dirs     (xkey_file_t             *key_file,
					     const xchar_t          *file,
					     xchar_t               **full_path,
					     GKeyFileFlags         flags,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xchar_t    *xkey_file_to_data                (xkey_file_t             *key_file,
					     xsize_t                *length,
					     xerror_t              **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_2_40
xboolean_t  xkey_file_save_to_file           (xkey_file_t             *key_file,
                                             const xchar_t          *filename,
                                             xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xchar_t    *xkey_file_get_start_group        (xkey_file_t             *key_file) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t   **xkey_file_get_groups             (xkey_file_t             *key_file,
					     xsize_t                *length);
XPL_AVAILABLE_IN_ALL
xchar_t   **xkey_file_get_keys               (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     xsize_t                *length,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xboolean_t  xkey_file_has_group              (xkey_file_t             *key_file,
					     const xchar_t          *group_name);
XPL_AVAILABLE_IN_ALL
xboolean_t  xkey_file_has_key                (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xchar_t    *xkey_file_get_value              (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xerror_t              **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_value              (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     const xchar_t          *value);
XPL_AVAILABLE_IN_ALL
xchar_t    *xkey_file_get_string             (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xerror_t              **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_string             (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     const xchar_t          *string);
XPL_AVAILABLE_IN_ALL
xchar_t    *xkey_file_get_locale_string      (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     const xchar_t          *locale,
					     xerror_t              **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_2_56
xchar_t    *xkey_file_get_locale_for_key     (xkey_file_t             *key_file,
                                             const xchar_t          *group_name,
                                             const xchar_t          *key,
                                             const xchar_t          *locale) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_locale_string      (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     const xchar_t          *locale,
					     const xchar_t          *string);
XPL_AVAILABLE_IN_ALL
xboolean_t  xkey_file_get_boolean            (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_boolean            (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xboolean_t              value);
XPL_AVAILABLE_IN_ALL
xint_t      xkey_file_get_integer            (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_integer            (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xint_t                  value);
XPL_AVAILABLE_IN_ALL
sint64_t    xkey_file_get_int64              (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_int64              (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     sint64_t                value);
XPL_AVAILABLE_IN_ALL
xuint64_t   xkey_file_get_uint64             (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_uint64             (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xuint64_t               value);
XPL_AVAILABLE_IN_ALL
xdouble_t   xkey_file_get_double             (xkey_file_t             *key_file,
                                             const xchar_t          *group_name,
                                             const xchar_t          *key,
                                             xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_double             (xkey_file_t             *key_file,
                                             const xchar_t          *group_name,
                                             const xchar_t          *key,
                                             xdouble_t               value);
XPL_AVAILABLE_IN_ALL
xchar_t   **xkey_file_get_string_list        (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xsize_t                *length,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_string_list        (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     const xchar_t * const   list[],
					     xsize_t                 length);
XPL_AVAILABLE_IN_ALL
xchar_t   **xkey_file_get_locale_string_list (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     const xchar_t          *locale,
					     xsize_t                *length,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_locale_string_list (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     const xchar_t          *locale,
					     const xchar_t * const   list[],
					     xsize_t                 length);
XPL_AVAILABLE_IN_ALL
xboolean_t *xkey_file_get_boolean_list       (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xsize_t                *length,
					     xerror_t              **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_boolean_list       (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xboolean_t              list[],
					     xsize_t                 length);
XPL_AVAILABLE_IN_ALL
xint_t     *xkey_file_get_integer_list       (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xsize_t                *length,
					     xerror_t              **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_double_list        (xkey_file_t             *key_file,
                                             const xchar_t          *group_name,
                                             const xchar_t          *key,
                                             xdouble_t               list[],
                                             xsize_t                 length);
XPL_AVAILABLE_IN_ALL
xdouble_t  *xkey_file_get_double_list        (xkey_file_t             *key_file,
                                             const xchar_t          *group_name,
                                             const xchar_t          *key,
                                             xsize_t                *length,
                                             xerror_t              **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
void      xkey_file_set_integer_list       (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xint_t                  list[],
					     xsize_t                 length);
XPL_AVAILABLE_IN_ALL
xboolean_t  xkey_file_set_comment            (xkey_file_t             *key_file,
                                             const xchar_t          *group_name,
                                             const xchar_t          *key,
                                             const xchar_t          *comment,
                                             xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xchar_t    *xkey_file_get_comment            (xkey_file_t             *key_file,
                                             const xchar_t          *group_name,
                                             const xchar_t          *key,
                                             xerror_t              **error) G_GNUC_MALLOC;

XPL_AVAILABLE_IN_ALL
xboolean_t  xkey_file_remove_comment         (xkey_file_t             *key_file,
                                             const xchar_t          *group_name,
                                             const xchar_t          *key,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xboolean_t  xkey_file_remove_key             (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     const xchar_t          *key,
					     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xboolean_t  xkey_file_remove_group           (xkey_file_t             *key_file,
					     const xchar_t          *group_name,
					     xerror_t              **error);

/* Defines for handling freedesktop.org Desktop files */
#define G_KEY_FILE_DESKTOP_GROUP                "Desktop Entry"

#define G_KEY_FILE_DESKTOP_KEY_TYPE             "Type"
#define G_KEY_FILE_DESKTOP_KEY_VERSION          "Version"
#define G_KEY_FILE_DESKTOP_KEY_NAME             "Name"
#define G_KEY_FILE_DESKTOP_KEY_GENERIC_NAME     "GenericName"
#define G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY       "NoDisplay"
#define G_KEY_FILE_DESKTOP_KEY_COMMENT          "Comment"
#define G_KEY_FILE_DESKTOP_KEY_ICON             "Icon"
#define G_KEY_FILE_DESKTOP_KEY_HIDDEN           "Hidden"
#define G_KEY_FILE_DESKTOP_KEY_ONLY_SHOW_IN     "OnlyShowIn"
#define G_KEY_FILE_DESKTOP_KEY_NOT_SHOW_IN      "NotShowIn"
#define G_KEY_FILE_DESKTOP_KEY_TRY_EXEC         "TryExec"
#define G_KEY_FILE_DESKTOP_KEY_EXEC             "Exec"
#define G_KEY_FILE_DESKTOP_KEY_PATH             "Path"
#define G_KEY_FILE_DESKTOP_KEY_TERMINAL         "Terminal"
#define G_KEY_FILE_DESKTOP_KEY_MIME_TYPE        "MimeType"
#define G_KEY_FILE_DESKTOP_KEY_CATEGORIES       "Categories"
#define G_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY   "StartupNotify"
#define G_KEY_FILE_DESKTOP_KEY_STARTUP_WM_CLASS "StartupWMClass"
#define G_KEY_FILE_DESKTOP_KEY_URL              "URL"
#define G_KEY_FILE_DESKTOP_KEY_DBUS_ACTIVATABLE "DBusActivatable"
#define G_KEY_FILE_DESKTOP_KEY_ACTIONS          "Actions"

#define G_KEY_FILE_DESKTOP_TYPE_APPLICATION     "Application"
#define G_KEY_FILE_DESKTOP_TYPE_LINK            "Link"
#define G_KEY_FILE_DESKTOP_TYPE_DIRECTORY       "Directory"

G_END_DECLS

#endif /* __G_KEY_FILE_H__ */
