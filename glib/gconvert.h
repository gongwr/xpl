/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_CONVERT_H__
#define __G_CONVERT_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gerror.h>

G_BEGIN_DECLS

/**
 * GConvertError:
 * @G_CONVERT_ERROR_NO_CONVERSION: Conversion between the requested character
 *     sets is not supported.
 * @G_CONVERT_ERROR_ILLEGAL_SEQUENCE: Invalid byte sequence in conversion input;
 *    or the character sequence could not be represented in the target
 *    character set.
 * @G_CONVERT_ERROR_FAILED: Conversion failed for some reason.
 * @G_CONVERT_ERROR_PARTIAL_INPUT: Partial character sequence at end of input.
 * @G_CONVERT_ERROR_BAD_URI: URI is invalid.
 * @G_CONVERT_ERROR_NOT_ABSOLUTE_PATH: Pathname is not an absolute path.
 * @G_CONVERT_ERROR_NO_MEMORY: No memory available. Since: 2.40
 * @G_CONVERT_ERROR_EMBEDDED_NUL: An embedded NUL character is present in
 *     conversion output where a NUL-terminated string is expected.
 *     Since: 2.56
 *
 * Error codes returned by character set conversion routines.
 */
typedef enum
{
  G_CONVERT_ERROR_NO_CONVERSION,
  G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
  G_CONVERT_ERROR_FAILED,
  G_CONVERT_ERROR_PARTIAL_INPUT,
  G_CONVERT_ERROR_BAD_URI,
  G_CONVERT_ERROR_NOT_ABSOLUTE_PATH,
  G_CONVERT_ERROR_NO_MEMORY,
  G_CONVERT_ERROR_EMBEDDED_NUL
} GConvertError;

/**
 * G_CONVERT_ERROR:
 *
 * Error domain for character set conversions. Errors in this domain will
 * be from the #GConvertError enumeration. See #xerror_t for information on
 * error domains.
 */
#define G_CONVERT_ERROR g_convert_error_quark()
XPL_AVAILABLE_IN_ALL
xquark g_convert_error_quark (void);

/**
 * GIConv: (skip)
 *
 * The GIConv struct wraps an iconv() conversion descriptor. It contains
 * private data and should only be accessed using the following functions.
 */
typedef struct _GIConv *GIConv;

XPL_AVAILABLE_IN_ALL
GIConv g_iconv_open   (const xchar_t  *to_codeset,
		       const xchar_t  *from_codeset);
XPL_AVAILABLE_IN_ALL
xsize_t  g_iconv        (GIConv        converter,
		       xchar_t       **inbuf,
		       xsize_t        *inbytes_left,
		       xchar_t       **outbuf,
		       xsize_t        *outbytes_left);
XPL_AVAILABLE_IN_ALL
xint_t   g_iconv_close  (GIConv        converter);


XPL_AVAILABLE_IN_ALL
xchar_t* g_convert               (const xchar_t  *str,
				xssize_t        len,
				const xchar_t  *to_codeset,
				const xchar_t  *from_codeset,
				xsize_t        *bytes_read,
				xsize_t        *bytes_written,
				xerror_t      **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t* g_convert_with_iconv    (const xchar_t  *str,
				xssize_t        len,
				GIConv        converter,
				xsize_t        *bytes_read,
				xsize_t        *bytes_written,
				xerror_t      **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t* g_convert_with_fallback (const xchar_t  *str,
				xssize_t        len,
				const xchar_t  *to_codeset,
				const xchar_t  *from_codeset,
				const xchar_t  *fallback,
				xsize_t        *bytes_read,
				xsize_t        *bytes_written,
				xerror_t      **error) G_GNUC_MALLOC;


/* Convert between libc's idea of strings and UTF-8.
 */
XPL_AVAILABLE_IN_ALL
xchar_t* g_locale_to_utf8   (const xchar_t  *opsysstring,
			   xssize_t        len,
			   xsize_t        *bytes_read,
			   xsize_t        *bytes_written,
			   xerror_t      **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t* g_locale_from_utf8 (const xchar_t  *utf8string,
			   xssize_t        len,
			   xsize_t        *bytes_read,
			   xsize_t        *bytes_written,
			   xerror_t      **error) G_GNUC_MALLOC;

/* Convert between the operating system (or C runtime)
 * representation of file names and UTF-8.
 */
XPL_AVAILABLE_IN_ALL
xchar_t* xfilename_to_utf8   (const xchar_t  *opsysstring,
			     xssize_t        len,
			     xsize_t        *bytes_read,
			     xsize_t        *bytes_written,
			     xerror_t      **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t* xfilename_from_utf8 (const xchar_t  *utf8string,
			     xssize_t        len,
			     xsize_t        *bytes_read,
			     xsize_t        *bytes_written,
			     xerror_t      **error) G_GNUC_MALLOC;

XPL_AVAILABLE_IN_ALL
xchar_t *xfilename_from_uri (const xchar_t *uri,
			    xchar_t      **hostname,
			    xerror_t     **error) G_GNUC_MALLOC;

XPL_AVAILABLE_IN_ALL
xchar_t *xfilename_to_uri   (const xchar_t *filename,
			    const xchar_t *hostname,
			    xerror_t     **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t *xfilename_display_name (const xchar_t *filename) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xboolean_t g_get_filename_charsets (const xchar_t ***filename_charsets);

XPL_AVAILABLE_IN_ALL
xchar_t *xfilename_display_basename (const xchar_t *filename) G_GNUC_MALLOC;

XPL_AVAILABLE_IN_ALL
xchar_t **xuri_list_extract_uris (const xchar_t *uri_list);

G_END_DECLS

#endif /* __G_CONVERT_H__ */
