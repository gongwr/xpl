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

#ifndef __G_STRING_H__
#define __G_STRING_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>
#include <glib/gunicode.h>
#include <glib/gbytes.h>
#include <glib/gutils.h>  /* for G_CAN_INLINE */

G_BEGIN_DECLS

typedef struct _GString         xstring_t;

struct _GString
{
  xchar_t  *str;
  xsize_t len;
  xsize_t allocated_len;
};

XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_new               (const xchar_t     *init);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_new_len           (const xchar_t     *init,
                                         xssize_t           len);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_sized_new         (xsize_t            dfl_size);
XPL_AVAILABLE_IN_ALL
xchar_t*       xstring_free              (xstring_t         *string,
                                         xboolean_t         free_segment);
XPL_AVAILABLE_IN_2_34
xbytes_t*      xstring_free_to_bytes     (xstring_t         *string);
XPL_AVAILABLE_IN_ALL
xboolean_t     xstring_equal             (const xstring_t   *v,
                                         const xstring_t   *v2);
XPL_AVAILABLE_IN_ALL
xuint_t        xstring_hash              (const xstring_t   *str);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_assign            (xstring_t         *string,
                                         const xchar_t     *rval);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_truncate          (xstring_t         *string,
                                         xsize_t            len);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_set_size          (xstring_t         *string,
                                         xsize_t            len);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_insert_len        (xstring_t         *string,
                                         xssize_t           pos,
                                         const xchar_t     *val,
                                         xssize_t           len);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_append            (xstring_t         *string,
                                         const xchar_t     *val);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_append_len        (xstring_t         *string,
                                         const xchar_t     *val,
                                         xssize_t           len);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_append_c          (xstring_t         *string,
                                         xchar_t            c);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_append_unichar    (xstring_t         *string,
                                         xunichar_t         wc);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_prepend           (xstring_t         *string,
                                         const xchar_t     *val);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_prepend_c         (xstring_t         *string,
                                         xchar_t            c);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_prepend_unichar   (xstring_t         *string,
                                         xunichar_t         wc);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_prepend_len       (xstring_t         *string,
                                         const xchar_t     *val,
                                         xssize_t           len);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_insert            (xstring_t         *string,
                                         xssize_t           pos,
                                         const xchar_t     *val);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_insert_c          (xstring_t         *string,
                                         xssize_t           pos,
                                         xchar_t            c);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_insert_unichar    (xstring_t         *string,
                                         xssize_t           pos,
                                         xunichar_t         wc);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_overwrite         (xstring_t         *string,
                                         xsize_t            pos,
                                         const xchar_t     *val);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_overwrite_len     (xstring_t         *string,
                                         xsize_t            pos,
                                         const xchar_t     *val,
                                         xssize_t           len);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_erase             (xstring_t         *string,
                                         xssize_t           pos,
                                         xssize_t           len);
XPL_AVAILABLE_IN_2_68
xuint_t         xstring_replace          (xstring_t         *string,
                                         const xchar_t     *find,
                                         const xchar_t     *replace,
                                         xuint_t            limit);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_ascii_down        (xstring_t         *string);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_ascii_up          (xstring_t         *string);
XPL_AVAILABLE_IN_ALL
void         xstring_vprintf           (xstring_t         *string,
                                         const xchar_t     *format,
                                         va_list          args)
                                         G_GNUC_PRINTF(2, 0);
XPL_AVAILABLE_IN_ALL
void         xstring_printf            (xstring_t         *string,
                                         const xchar_t     *format,
                                         ...) G_GNUC_PRINTF (2, 3);
XPL_AVAILABLE_IN_ALL
void         xstring_append_vprintf    (xstring_t         *string,
                                         const xchar_t     *format,
                                         va_list          args)
                                         G_GNUC_PRINTF(2, 0);
XPL_AVAILABLE_IN_ALL
void         xstring_append_printf     (xstring_t         *string,
                                         const xchar_t     *format,
                                         ...) G_GNUC_PRINTF (2, 3);
XPL_AVAILABLE_IN_ALL
xstring_t*     xstring_append_uri_escaped (xstring_t         *string,
                                          const xchar_t     *unescaped,
                                          const xchar_t     *reserved_chars_allowed,
                                          xboolean_t         allow_utf8);

/* -- optimize xstrig_append_c --- */
#ifdef G_CAN_INLINE
static inline xstring_t*
xstring_append_c_inline (xstring_t *xstring,
                          xchar_t    c)
{
  if (xstring->len + 1 < xstring->allocated_len)
    {
      xstring->str[xstring->len++] = c;
      xstring->str[xstring->len] = 0;
    }
  else
    xstring_insert_c (xstring, -1, c);
  return xstring;
}
#define xstring_append_c(gstr,c)       xstring_append_c_inline (gstr, c)
#endif /* G_CAN_INLINE */


XPL_DEPRECATED
xstring_t *xstring_down (xstring_t *string);
XPL_DEPRECATED
xstring_t *xstring_up   (xstring_t *string);

#define  xstring_sprintf  xstring_printf XPL_DEPRECATED_MACRO_IN_2_26_FOR(xstring_printf)
#define  xstring_sprintfa xstring_append_printf XPL_DEPRECATED_MACRO_IN_2_26_FOR(xstring_append_printf)

G_END_DECLS

#endif /* __G_STRING_H__ */
