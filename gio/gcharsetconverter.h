/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2009 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __XCHARSET_CONVERTER_H__
#define __XCHARSET_CONVERTER_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gconverter.h>

G_BEGIN_DECLS

#define XTYPE_CHARSET_CONVERTER         (xcharset_converter_get_type ())
#define XCHARSET_CONVERTER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_CHARSET_CONVERTER, xcharset_converter_t))
#define XCHARSET_CONVERTER_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_CHARSET_CONVERTER, xcharset_converter_class_t))
#define X_IS_CHARSET_CONVERTER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_CHARSET_CONVERTER))
#define X_IS_CHARSET_CONVERTER_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_CHARSET_CONVERTER))
#define XCHARSET_CONVERTER_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_CHARSET_CONVERTER, xcharset_converter_class_t))

typedef struct _xcharset_converter_class   xcharset_converter_class_t;

struct _xcharset_converter_class
{
  xobject_class_t parent_class;
};

XPL_AVAILABLE_IN_ALL
xtype_t              xcharset_converter_get_type      (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xcharset_converter_t *xcharset_converter_new            (const xchar_t  *to_charset,
						       const xchar_t  *from_charset,
						       xerror_t **error);
XPL_AVAILABLE_IN_ALL
void               xcharset_converter_set_use_fallback (xcharset_converter_t *converter,
							 xboolean_t use_fallback);
XPL_AVAILABLE_IN_ALL
xboolean_t           xcharset_converter_get_use_fallback (xcharset_converter_t *converter);
XPL_AVAILABLE_IN_ALL
xuint_t              xcharset_converter_get_num_fallbacks (xcharset_converter_t *converter);

G_END_DECLS

#endif /* __XCHARSET_CONVERTER_H__ */
