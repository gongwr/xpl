/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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

#ifndef __XICON_H__
#define __XICON_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_ICON            (xicon_get_type ())
#define XICON(obj)            (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_ICON, xicon))
#define X_IS_ICON(obj)	       (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_ICON))
#define XICON_GET_IFACE(obj)  (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_ICON, xicon_iface_t))

/**
 * xicon_t:
 *
 * An abstract type that specifies an icon.
 **/
typedef struct _GIconIface xicon_iface_t;

/**
 * xicon_iface_t:
 * @x_iface: The parent interface.
 * @hash: A hash for a given #xicon_t.
 * @equal: Checks if two #GIcons are equal.
 * @to_tokens: Serializes a #xicon_t into tokens. The tokens must not
 * contain any whitespace. Don't implement if the #xicon_t can't be
 * serialized (Since 2.20).
 * @from_tokens: Constructs a #xicon_t from tokens. Set the #xerror_t if
 * the tokens are malformed. Don't implement if the #xicon_t can't be
 * serialized (Since 2.20).
 * @serialize: Serializes a #xicon_t into a #xvariant_t. Since: 2.38
 *
 * xicon_iface_t is used to implement xicon_t types for various
 * different systems. See #xthemed_icon_t and #xloadable_icon_t for
 * examples of how to implement this interface.
 */
struct _GIconIface
{
  xtype_interface_t x_iface;

  /* Virtual Table */

  xuint_t       (* hash)        (xicon_t   *icon);
  xboolean_t    (* equal)       (xicon_t   *icon1,
                               xicon_t   *icon2);
  xboolean_t    (* to_tokens)   (xicon_t   *icon,
			       xptr_array_t *tokens,
                               xint_t    *out_version);
  xicon_t *     (* from_tokens) (xchar_t  **tokens,
                               xint_t     num_tokens,
                               xint_t     version,
                               xerror_t **error);

  xvariant_t *  (* serialize)   (xicon_t   *icon);
};

XPL_AVAILABLE_IN_ALL
xtype_t    xicon_get_type  (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xuint_t    xicon_hash            (xconstpointer  icon);
XPL_AVAILABLE_IN_ALL
xboolean_t xicon_equal           (xicon_t         *icon1,
                                 xicon_t         *icon2);
XPL_AVAILABLE_IN_ALL
xchar_t   *xicon_to_string       (xicon_t         *icon);
XPL_AVAILABLE_IN_ALL
xicon_t   *xicon_new_for_string  (const xchar_t   *str,
                                 xerror_t       **error);

XPL_AVAILABLE_IN_2_38
xvariant_t * xicon_serialize     (xicon_t         *icon);
XPL_AVAILABLE_IN_2_38
xicon_t *    xicon_deserialize   (xvariant_t      *value);

G_END_DECLS

#endif /* __XICON_H__ */
