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

#include <config.h>
#include <glib.h>
#include <string.h>

#include "gnativesocketaddress.h"
#include "gnetworkingprivate.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:gnativesocketaddress
 * @short_description: Native xsocket_address_t
 * @include: gio/gio.h
 *
 * A socket address of some unknown native type.
 */

/**
 * xnative_socket_address_t:
 *
 * A socket address, corresponding to a general struct
 * sockadd address of a type not otherwise handled by glib.
 */

struct _GNativeSocketAddressPrivate
{
  struct sockaddr *sockaddr;
  union {
    struct sockaddr_storage storage;
    struct sockaddr sa;
  } storage;
  xsize_t sockaddr_len;
};

G_DEFINE_TYPE_WITH_PRIVATE (xnative_socket_address_t, g_native_socket_address, XTYPE_SOCKET_ADDRESS)

static void
g_native_socket_address_dispose (xobject_t *object)
{
  xnative_socket_address_t *address = G_NATIVE_SOCKET_ADDRESS (object);

  if (address->priv->sockaddr != &address->priv->storage.sa)
    g_free (address->priv->sockaddr);

  G_OBJECT_CLASS (g_native_socket_address_parent_class)->dispose (object);
}

static xsocket_family_t
g_native_socket_address_get_family (xsocket_address_t *address)
{
  xnative_socket_address_t *addr;

  g_return_val_if_fail (X_IS_NATIVE_SOCKET_ADDRESS (address), 0);

  addr = G_NATIVE_SOCKET_ADDRESS (address);

  return addr->priv->sockaddr->sa_family;
}

static xssize_t
g_native_socket_address_get_native_size (xsocket_address_t *address)
{
  xnative_socket_address_t *addr;

  g_return_val_if_fail (X_IS_NATIVE_SOCKET_ADDRESS (address), 0);

  addr = G_NATIVE_SOCKET_ADDRESS (address);

  return addr->priv->sockaddr_len;
}

static xboolean_t
g_native_socket_address_to_native (xsocket_address_t  *address,
                                   xpointer_t         dest,
                                   xsize_t            destlen,
                                   xerror_t         **error)
{
  xnative_socket_address_t *addr;

  g_return_val_if_fail (X_IS_NATIVE_SOCKET_ADDRESS (address), FALSE);

  addr = G_NATIVE_SOCKET_ADDRESS (address);

  if (destlen < addr->priv->sockaddr_len)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE,
                           _("Not enough space for socket address"));
      return FALSE;
    }

  memcpy (dest, addr->priv->sockaddr, addr->priv->sockaddr_len);
  return TRUE;
}

static void
g_native_socket_address_class_init (GNativeSocketAddressClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);
  GSocketAddressClass *gsocketaddress_class = XSOCKET_ADDRESS_CLASS (klass);

  gobject_class->dispose = g_native_socket_address_dispose;

  gsocketaddress_class->get_family = g_native_socket_address_get_family;
  gsocketaddress_class->to_native = g_native_socket_address_to_native;
  gsocketaddress_class->get_native_size = g_native_socket_address_get_native_size;
}

static void
g_native_socket_address_init (xnative_socket_address_t *address)
{
  address->priv = g_native_socket_address_get_instance_private (address);
}

/**
 * g_native_socket_address_new:
 * @native: a native address object
 * @len: the length of @native, in bytes
 *
 * Creates a new #xnative_socket_address_t for @native and @len.
 *
 * Returns: a new #xnative_socket_address_t
 *
 * Since: 2.46
 */
xsocket_address_t *
g_native_socket_address_new (xpointer_t        native,
                             xsize_t           len)
{
  xnative_socket_address_t *addr;

  addr = xobject_new (XTYPE_NATIVE_SOCKET_ADDRESS, NULL);

  if (len <= sizeof(addr->priv->storage))
    addr->priv->sockaddr = &addr->priv->storage.sa;
  else
    addr->priv->sockaddr = g_malloc (len);

  memcpy (addr->priv->sockaddr, native, len);
  addr->priv->sockaddr_len = len;
  return XSOCKET_ADDRESS (addr);
}
