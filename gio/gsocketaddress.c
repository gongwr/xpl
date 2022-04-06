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

#include "gsocketaddress.h"
#include "ginetaddress.h"
#include "ginetsocketaddress.h"
#include "gnativesocketaddress.h"
#include "gnetworkingprivate.h"
#include "gproxyaddress.h"
#include "gproxyaddressenumerator.h"
#include "gsocketaddressenumerator.h"
#include "gsocketconnectable.h"
#include "glibintl.h"
#include "gioenumtypes.h"

#include "gunixsocketaddress.h"

#ifdef G_OS_WIN32
#include "giowin32-afunix.h"
#endif


/**
 * SECTION:gsocketaddress
 * @short_description: Abstract base class representing endpoints
 *     for socket communication
 * @include: gio/gio.h
 *
 * #xsocket_address_t is the equivalent of struct sockaddr in the BSD
 * sockets API. This is an abstract class; use #xinet_socket_address_t
 * for internet sockets, or #GUnixSocketAddress for UNIX domain sockets.
 */

/**
 * xsocket_address_t:
 *
 * A socket endpoint address, corresponding to struct sockaddr
 * or one of its subtypes.
 */

enum
{
  PROP_NONE,
  PROP_FAMILY
};

static void                      xsocket_address_connectable_iface_init       (xsocket_connectable_iface_t *iface);
static xsocket_address_enumerator_t *xsocket_address_connectable_enumerate	       (xsocket_connectable_t      *connectable);
static xsocket_address_enumerator_t *xsocket_address_connectable_proxy_enumerate  (xsocket_connectable_t      *connectable);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (xsocket_address_t, xsocket_address, XTYPE_OBJECT,
				  G_IMPLEMENT_INTERFACE (XTYPE_SOCKET_CONNECTABLE,
							 xsocket_address_connectable_iface_init))

/**
 * xsocket_address_get_family:
 * @address: a #xsocket_address_t
 *
 * Gets the socket family type of @address.
 *
 * Returns: the socket family type of @address
 *
 * Since: 2.22
 */
xsocket_family_t
xsocket_address_get_family (xsocket_address_t *address)
{
  g_return_val_if_fail (X_IS_SOCKET_ADDRESS (address), 0);

  return XSOCKET_ADDRESS_GET_CLASS (address)->get_family (address);
}

static void
xsocket_address_get_property (xobject_t *object, xuint_t prop_id,
			       xvalue_t *value, xparam_spec_t *pspec)
{
  xsocket_address_t *address = XSOCKET_ADDRESS (object);

  switch (prop_id)
    {
     case PROP_FAMILY:
      xvalue_set_enum (value, xsocket_address_get_family (address));
      break;

     default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xsocket_address_class_init (GSocketAddressClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = xsocket_address_get_property;

  xobject_class_install_property (gobject_class, PROP_FAMILY,
                                   g_param_spec_enum ("family",
						      P_("Address family"),
						      P_("The family of the socket address"),
						      XTYPE_SOCKET_FAMILY,
						      XSOCKET_FAMILY_INVALID,
						      G_PARAM_READABLE |
                                                      G_PARAM_STATIC_STRINGS));
}

static void
xsocket_address_connectable_iface_init (xsocket_connectable_iface_t *connectable_iface)
{
  connectable_iface->enumerate  = xsocket_address_connectable_enumerate;
  connectable_iface->proxy_enumerate  = xsocket_address_connectable_proxy_enumerate;
  /* to_string() is implemented by subclasses */
}

static void
xsocket_address_init (xsocket_address_t *address)
{

}

/**
 * xsocket_address_get_native_size:
 * @address: a #xsocket_address_t
 *
 * Gets the size of @address's native struct sockaddr.
 * You can use this to allocate memory to pass to
 * xsocket_address_to_native().
 *
 * Returns: the size of the native struct sockaddr that
 *     @address represents
 *
 * Since: 2.22
 */
xssize_t
xsocket_address_get_native_size (xsocket_address_t *address)
{
  g_return_val_if_fail (X_IS_SOCKET_ADDRESS (address), -1);

  return XSOCKET_ADDRESS_GET_CLASS (address)->get_native_size (address);
}

/**
 * xsocket_address_to_native:
 * @address: a #xsocket_address_t
 * @dest: a pointer to a memory location that will contain the native
 * struct sockaddr
 * @destlen: the size of @dest. Must be at least as large as
 *     xsocket_address_get_native_size()
 * @error: #xerror_t for error reporting, or %NULL to ignore
 *
 * Converts a #xsocket_address_t to a native struct sockaddr, which can
 * be passed to low-level functions like connect() or bind().
 *
 * If not enough space is available, a %G_IO_ERROR_NO_SPACE error
 * is returned. If the address type is not known on the system
 * then a %G_IO_ERROR_NOT_SUPPORTED error is returned.
 *
 * Returns: %TRUE if @dest was filled in, %FALSE on error
 *
 * Since: 2.22
 */
xboolean_t
xsocket_address_to_native (xsocket_address_t  *address,
			    xpointer_t         dest,
			    xsize_t            destlen,
			    xerror_t         **error)
{
  g_return_val_if_fail (X_IS_SOCKET_ADDRESS (address), FALSE);

  return XSOCKET_ADDRESS_GET_CLASS (address)->to_native (address, dest, destlen, error);
}

/**
 * xsocket_address_new_from_native:
 * @native: (not nullable): a pointer to a struct sockaddr
 * @len: the size of the memory location pointed to by @native
 *
 * Creates a #xsocket_address_t subclass corresponding to the native
 * struct sockaddr @native.
 *
 * Returns: a new #xsocket_address_t if @native could successfully
 *     be converted, otherwise %NULL
 *
 * Since: 2.22
 */
xsocket_address_t *
xsocket_address_new_from_native (xpointer_t native,
				  xsize_t    len)
{
  gshort family;

  if (len < sizeof (gshort))
    return NULL;

  family = ((struct sockaddr *) native)->sa_family;

  if (family == AF_UNSPEC)
    return NULL;

  if (family == AF_INET)
    {
      struct sockaddr_in *addr = (struct sockaddr_in *) native;
      xinet_address_t *iaddr;
      xsocket_address_t *sockaddr;

      if (len < sizeof (*addr))
	return NULL;

      iaddr = xinet_address_new_from_bytes ((xuint8_t *) &(addr->sin_addr), AF_INET);
      sockaddr = g_inet_socket_address_new (iaddr, g_ntohs (addr->sin_port));
      xobject_unref (iaddr);
      return sockaddr;
    }

  if (family == AF_INET6)
    {
      struct sockaddr_in6 *addr = (struct sockaddr_in6 *) native;
      xinet_address_t *iaddr;
      xsocket_address_t *sockaddr;

      if (len < sizeof (*addr))
	return NULL;

      if (IN6_IS_ADDR_V4MAPPED (&(addr->sin6_addr)))
	{
	  struct sockaddr_in sin_addr;

	  sin_addr.sin_family = AF_INET;
	  sin_addr.sin_port = addr->sin6_port;
	  memcpy (&(sin_addr.sin_addr.s_addr), addr->sin6_addr.s6_addr + 12, 4);
	  iaddr = xinet_address_new_from_bytes ((xuint8_t *) &(sin_addr.sin_addr), AF_INET);
	}
      else
	{
	  iaddr = xinet_address_new_from_bytes ((xuint8_t *) &(addr->sin6_addr), AF_INET6);
	}

      sockaddr = xobject_new (XTYPE_INET_SOCKET_ADDRESS,
			       "address", iaddr,
			       "port", g_ntohs (addr->sin6_port),
			       "flowinfo", addr->sin6_flowinfo,
			       "scope_id", addr->sin6_scope_id,
			       NULL);
      xobject_unref (iaddr);
      return sockaddr;
    }

  if (family == AF_UNIX)
    {
      struct sockaddr_un *addr = (struct sockaddr_un *) native;
      xint_t path_len = len - G_STRUCT_OFFSET (struct sockaddr_un, sun_path);

      if (path_len == 0)
	{
	  return g_unix_socket_address_new_with_type ("", 0,
						      G_UNIX_SOCKET_ADDRESS_ANONYMOUS);
	}
      else if (addr->sun_path[0] == 0)
	{
	  if (!g_unix_socket_address_abstract_names_supported ())
	    {
	      return g_unix_socket_address_new_with_type ("", 0,
							  G_UNIX_SOCKET_ADDRESS_ANONYMOUS);
	    }
	  else if (len < sizeof (*addr))
	    {
	      return g_unix_socket_address_new_with_type (addr->sun_path + 1,
							  path_len - 1,
							  G_UNIX_SOCKET_ADDRESS_ABSTRACT);
	    }
	  else
	    {
	      return g_unix_socket_address_new_with_type (addr->sun_path + 1,
							  path_len - 1,
							  G_UNIX_SOCKET_ADDRESS_ABSTRACT_PADDED);
	    }
	}
      else
	return g_unix_socket_address_new (addr->sun_path);
    }

  return g_native_socket_address_new (native, len);
}


#define XTYPE_SOCKET_ADDRESS_ADDRESS_ENUMERATOR (_xsocket_address_address_enumerator_get_type ())
#define XSOCKET_ADDRESS_ADDRESS_ENUMERATOR(obj) (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_SOCKET_ADDRESS_ADDRESS_ENUMERATOR, GSocketAddressAddressEnumerator))

typedef struct {
  xsocket_address_enumerator_t parent_instance;

  xsocket_address_t *sockaddr;
} GSocketAddressAddressEnumerator;

typedef struct {
  GSocketAddressEnumeratorClass parent_class;

} GSocketAddressAddressEnumeratorClass;

static xtype_t _xsocket_address_address_enumerator_get_type (void);
G_DEFINE_TYPE (GSocketAddressAddressEnumerator, _xsocket_address_address_enumerator, XTYPE_SOCKET_ADDRESS_ENUMERATOR)

static void
xsocket_address_address_enumerator_finalize (xobject_t *object)
{
  GSocketAddressAddressEnumerator *sockaddr_enum =
    XSOCKET_ADDRESS_ADDRESS_ENUMERATOR (object);

  if (sockaddr_enum->sockaddr)
    xobject_unref (sockaddr_enum->sockaddr);

  G_OBJECT_CLASS (_xsocket_address_address_enumerator_parent_class)->finalize (object);
}

static xsocket_address_t *
xsocket_address_address_enumerator_next (xsocket_address_enumerator_t  *enumerator,
					  xcancellable_t              *cancellable,
					  xerror_t                   **error)
{
  GSocketAddressAddressEnumerator *sockaddr_enum =
    XSOCKET_ADDRESS_ADDRESS_ENUMERATOR (enumerator);

  if (sockaddr_enum->sockaddr)
    {
      xsocket_address_t *ret = sockaddr_enum->sockaddr;

      sockaddr_enum->sockaddr = NULL;
      return ret;
    }
  else
    return NULL;
}

static void
_xsocket_address_address_enumerator_init (GSocketAddressAddressEnumerator *enumerator)
{
}

static void
_xsocket_address_address_enumerator_class_init (GSocketAddressAddressEnumeratorClass *sockaddrenum_class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (sockaddrenum_class);
  GSocketAddressEnumeratorClass *enumerator_class =
    XSOCKET_ADDRESS_ENUMERATOR_CLASS (sockaddrenum_class);

  enumerator_class->next = xsocket_address_address_enumerator_next;
  object_class->finalize = xsocket_address_address_enumerator_finalize;
}

static xsocket_address_enumerator_t *
xsocket_address_connectable_enumerate (xsocket_connectable_t *connectable)
{
  GSocketAddressAddressEnumerator *sockaddr_enum;

  sockaddr_enum = xobject_new (XTYPE_SOCKET_ADDRESS_ADDRESS_ENUMERATOR, NULL);
  sockaddr_enum->sockaddr = xobject_ref (XSOCKET_ADDRESS (connectable));

  return (xsocket_address_enumerator_t *)sockaddr_enum;
}

static xsocket_address_enumerator_t *
xsocket_address_connectable_proxy_enumerate (xsocket_connectable_t *connectable)
{
  xsocket_address_enumerator_t *addr_enum = NULL;

  g_assert (connectable != NULL);

  if (X_IS_INET_SOCKET_ADDRESS (connectable) &&
      !X_IS_PROXY_ADDRESS (connectable))
    {
      xinet_address_t *addr;
      xuint_t port;
      xchar_t *uri;
      xchar_t *ip;

      xobject_get (connectable, "address", &addr, "port", &port, NULL);

      ip = xinet_address_to_string (addr);
      uri = xuri_join (XURI_FLAGS_NONE, "none", NULL, ip, port, "", NULL, NULL);

      addr_enum = xobject_new (XTYPE_PROXY_ADDRESS_ENUMERATOR,
      	       	       	       	"connectable", connectable,
      	       	       	       	"uri", uri,
      	       	       	       	NULL);

      xobject_unref (addr);
      g_free (ip);
      g_free (uri);
    }
  else
    {
      addr_enum = xsocket_address_connectable_enumerate (connectable);
    }

  return addr_enum;
}
