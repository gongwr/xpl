/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Christian Kellner, Samuel Cormier-Iijima
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
 * Authors: Christian Kellner <gicmo@gnome.org>
 *          Samuel Cormier-Iijima <sciyoshi@gmail.com>
 */

#ifndef __G_UNIX_SOCKET_ADDRESS_H__
#define __G_UNIX_SOCKET_ADDRESS_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define XTYPE_UNIX_SOCKET_ADDRESS         (g_unix_socket_address_get_type ())
#define G_UNIX_SOCKET_ADDRESS(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_UNIX_SOCKET_ADDRESS, GUnixSocketAddress))
#define G_UNIX_SOCKET_ADDRESS_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_UNIX_SOCKET_ADDRESS, GUnixSocketAddressClass))
#define X_IS_UNIX_SOCKET_ADDRESS(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_UNIX_SOCKET_ADDRESS))
#define X_IS_UNIX_SOCKET_ADDRESS_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_UNIX_SOCKET_ADDRESS))
#define G_UNIX_SOCKET_ADDRESS_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_UNIX_SOCKET_ADDRESS, GUnixSocketAddressClass))

typedef struct _GUnixSocketAddress        GUnixSocketAddress;
typedef struct _GUnixSocketAddressClass   GUnixSocketAddressClass;
typedef struct _GUnixSocketAddressPrivate GUnixSocketAddressPrivate;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GUnixSocketAddress, g_object_unref)

struct _GUnixSocketAddress
{
  xsocket_address_t parent_instance;

  /*< private >*/
  GUnixSocketAddressPrivate *priv;
};

struct _GUnixSocketAddressClass
{
  GSocketAddressClass parent_class;
};

XPL_AVAILABLE_IN_ALL
xtype_t           g_unix_socket_address_get_type    (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xsocket_address_t *g_unix_socket_address_new             (const xchar_t        *path);
XPL_DEPRECATED_FOR(g_unix_socket_address_new_with_type)
xsocket_address_t *g_unix_socket_address_new_abstract    (const xchar_t        *path,
                                                       xint_t                path_len);
XPL_AVAILABLE_IN_ALL
xsocket_address_t *g_unix_socket_address_new_with_type   (const xchar_t            *path,
                                                       xint_t                    path_len,
                                                       GUnixSocketAddressType  type);
XPL_AVAILABLE_IN_ALL
const char *    g_unix_socket_address_get_path        (GUnixSocketAddress *address);
XPL_AVAILABLE_IN_ALL
xsize_t           g_unix_socket_address_get_path_len    (GUnixSocketAddress *address);
XPL_AVAILABLE_IN_ALL
GUnixSocketAddressType g_unix_socket_address_get_address_type (GUnixSocketAddress *address);
XPL_DEPRECATED
xboolean_t        g_unix_socket_address_get_is_abstract (GUnixSocketAddress *address);

XPL_AVAILABLE_IN_ALL
xboolean_t        g_unix_socket_address_abstract_names_supported (void);

G_END_DECLS

#endif /* __G_UNIX_SOCKET_ADDRESS_H__ */
