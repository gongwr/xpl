/* gbookmarkfile.h: parsing and building desktop bookmarks
 *
 * Copyright (C) 2005-2006 Emmanuele Bassi
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

#ifndef __G_BOOKMARK_FILE_H__
#define __G_BOOKMARK_FILE_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gdatetime.h>
#include <glib/gerror.h>
#include <time.h>

G_BEGIN_DECLS

/**
 * G_BOOKMARK_FILE_ERROR:
 *
 * Error domain for bookmark file parsing.
 *
 * Errors in this domain will be from the #GBookmarkFileError
 * enumeration. See #xerror_t for information on error domains.
 */
#define G_BOOKMARK_FILE_ERROR	(g_bookmark_file_error_quark ())


/**
 * GBookmarkFileError:
 * @G_BOOKMARK_FILE_ERROR_INVALID_URI: URI was ill-formed
 * @G_BOOKMARK_FILE_ERROR_INVALID_VALUE: a requested field was not found
 * @G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED: a requested application did
 *     not register a bookmark
 * @G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND: a requested URI was not found
 * @G_BOOKMARK_FILE_ERROR_READ: document was ill formed
 * @G_BOOKMARK_FILE_ERROR_UNKNOWN_ENCODING: the text being parsed was
 *     in an unknown encoding
 * @G_BOOKMARK_FILE_ERROR_WRITE: an error occurred while writing
 * @G_BOOKMARK_FILE_ERROR_FILE_NOT_FOUND: requested file was not found
 *
 * Error codes returned by bookmark file parsing.
 */
typedef enum
{
  G_BOOKMARK_FILE_ERROR_INVALID_URI,
  G_BOOKMARK_FILE_ERROR_INVALID_VALUE,
  G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED,
  G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
  G_BOOKMARK_FILE_ERROR_READ,
  G_BOOKMARK_FILE_ERROR_UNKNOWN_ENCODING,
  G_BOOKMARK_FILE_ERROR_WRITE,
  G_BOOKMARK_FILE_ERROR_FILE_NOT_FOUND
} GBookmarkFileError;

XPL_AVAILABLE_IN_ALL
xquark g_bookmark_file_error_quark (void);

/**
 * xbookmark_file_t:
 *
 * An opaque data structure representing a set of bookmarks.
 */
typedef struct _GBookmarkFile xbookmark_file_t;

XPL_AVAILABLE_IN_ALL
xbookmark_file_t *g_bookmark_file_new                 (void);
XPL_AVAILABLE_IN_ALL
void           g_bookmark_file_free                (xbookmark_file_t  *bookmark);

XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_load_from_file      (xbookmark_file_t  *bookmark,
						    const xchar_t    *filename,
						    xerror_t        **error);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_load_from_data      (xbookmark_file_t  *bookmark,
						    const xchar_t    *data,
						    xsize_t           length,
						    xerror_t        **error);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_load_from_data_dirs (xbookmark_file_t  *bookmark,
						    const xchar_t    *file,
						    xchar_t         **full_path,
						    xerror_t        **error);
XPL_AVAILABLE_IN_ALL
xchar_t *        g_bookmark_file_to_data             (xbookmark_file_t  *bookmark,
						    xsize_t          *length,
						    xerror_t        **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_to_file             (xbookmark_file_t  *bookmark,
						    const xchar_t    *filename,
						    xerror_t        **error);

XPL_AVAILABLE_IN_ALL
void           g_bookmark_file_set_title           (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t    *title);
XPL_AVAILABLE_IN_ALL
xchar_t *        g_bookmark_file_get_title           (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    xerror_t        **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
void           g_bookmark_file_set_description     (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t    *description);
XPL_AVAILABLE_IN_ALL
xchar_t *        g_bookmark_file_get_description     (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    xerror_t        **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
void           g_bookmark_file_set_mime_type       (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t    *mime_type);
XPL_AVAILABLE_IN_ALL
xchar_t *        g_bookmark_file_get_mime_type       (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    xerror_t        **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
void           g_bookmark_file_set_groups          (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t   **groups,
						    xsize_t           length);
XPL_AVAILABLE_IN_ALL
void           g_bookmark_file_add_group           (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t    *group);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_has_group           (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t    *group,
						    xerror_t        **error);
XPL_AVAILABLE_IN_ALL
xchar_t **       g_bookmark_file_get_groups          (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    xsize_t          *length,
						    xerror_t        **error);
XPL_AVAILABLE_IN_ALL
void           g_bookmark_file_add_application     (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t    *name,
						    const xchar_t    *exec);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_has_application     (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t    *name,
						    xerror_t        **error);
XPL_AVAILABLE_IN_ALL
xchar_t **       g_bookmark_file_get_applications    (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    xsize_t          *length,
						    xerror_t        **error);
XPL_DEPRECATED_IN_2_66_FOR(g_bookmark_file_set_application_info)
xboolean_t       g_bookmark_file_set_app_info        (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t    *name,
						    const xchar_t    *exec,
						    xint_t            count,
						    time_t          stamp,
						    xerror_t        **error);
XPL_AVAILABLE_IN_2_66
xboolean_t       g_bookmark_file_set_application_info (xbookmark_file_t  *bookmark,
                                                     const char     *uri,
                                                     const char     *name,
                                                     const char     *exec,
                                                     int             count,
                                                     xdatetime_t      *stamp,
                                                     xerror_t        **error);
XPL_DEPRECATED_IN_2_66_FOR(g_bookmark_file_get_application_info)
xboolean_t       g_bookmark_file_get_app_info        (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t    *name,
						    xchar_t         **exec,
						    xuint_t          *count,
						    time_t         *stamp,
						    xerror_t        **error);
XPL_AVAILABLE_IN_2_66
xboolean_t       g_bookmark_file_get_application_info (xbookmark_file_t  *bookmark,
                                                     const char     *uri,
                                                     const char     *name,
                                                     char          **exec,
                                                     unsigned int   *count,
                                                     xdatetime_t     **stamp,
                                                     xerror_t        **error);
XPL_AVAILABLE_IN_ALL
void           g_bookmark_file_set_is_private      (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    xboolean_t        is_private);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_get_is_private      (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    xerror_t        **error);
XPL_AVAILABLE_IN_ALL
void           g_bookmark_file_set_icon            (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t    *href,
						    const xchar_t    *mime_type);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_get_icon            (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    xchar_t         **href,
						    xchar_t         **mime_type,
						    xerror_t        **error);
XPL_DEPRECATED_IN_2_66_FOR(g_bookmark_file_set_added_date_time)
void           g_bookmark_file_set_added           (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    time_t          added);
XPL_AVAILABLE_IN_2_66
void           g_bookmark_file_set_added_date_time (xbookmark_file_t  *bookmark,
                                                    const char     *uri,
                                                    xdatetime_t      *added);
XPL_DEPRECATED_IN_2_66_FOR(g_bookmark_file_get_added_date_time)
time_t         g_bookmark_file_get_added           (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    xerror_t        **error);
XPL_AVAILABLE_IN_2_66
xdatetime_t     *g_bookmark_file_get_added_date_time (xbookmark_file_t  *bookmark,
                                                    const char     *uri,
                                                    xerror_t        **error);
XPL_DEPRECATED_IN_2_66_FOR(g_bookmark_file_set_modified_date_time)
void           g_bookmark_file_set_modified        (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    time_t          modified);
XPL_AVAILABLE_IN_2_66
void           g_bookmark_file_set_modified_date_time (xbookmark_file_t  *bookmark,
                                                       const char     *uri,
                                                       xdatetime_t      *modified);
XPL_DEPRECATED_IN_2_66_FOR(g_bookmark_file_get_modified_date_time)
time_t         g_bookmark_file_get_modified        (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    xerror_t        **error);
XPL_AVAILABLE_IN_2_66
xdatetime_t     *g_bookmark_file_get_modified_date_time (xbookmark_file_t  *bookmark,
                                                       const char     *uri,
                                                       xerror_t        **error);
XPL_DEPRECATED_IN_2_66_FOR(g_bookmark_file_set_visited_date_time)
void           g_bookmark_file_set_visited         (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    time_t          visited);
XPL_AVAILABLE_IN_2_66
void           g_bookmark_file_set_visited_date_time (xbookmark_file_t  *bookmark,
                                                      const char     *uri,
                                                      xdatetime_t      *visited);
XPL_DEPRECATED_IN_2_66_FOR(g_bookmark_file_get_visited_date_time)
time_t         g_bookmark_file_get_visited         (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    xerror_t        **error);
XPL_AVAILABLE_IN_2_66
xdatetime_t     *g_bookmark_file_get_visited_date_time (xbookmark_file_t  *bookmark,
                                                      const char     *uri,
                                                      xerror_t        **error);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_has_item            (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri);
XPL_AVAILABLE_IN_ALL
xint_t           g_bookmark_file_get_size            (xbookmark_file_t  *bookmark);
XPL_AVAILABLE_IN_ALL
xchar_t **       g_bookmark_file_get_uris            (xbookmark_file_t  *bookmark,
						    xsize_t          *length);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_remove_group        (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t    *group,
						    xerror_t        **error);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_remove_application  (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    const xchar_t    *name,
						    xerror_t        **error);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_remove_item         (xbookmark_file_t  *bookmark,
						    const xchar_t    *uri,
						    xerror_t        **error);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_bookmark_file_move_item           (xbookmark_file_t  *bookmark,
						    const xchar_t    *old_uri,
						    const xchar_t    *new_uri,
						    xerror_t        **error);

G_END_DECLS

#endif /* __G_BOOKMARK_FILE_H__ */
