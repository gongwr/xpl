/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __G_DBUS_OBJECT_PROXY_H__
#define __G_DBUS_OBJECT_PROXY_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_OBJECT_PROXY         (xdbus_object_proxy_get_type ())
#define G_DBUS_OBJECT_PROXY(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_OBJECT_PROXY, xdbus_object_proxy))
#define G_DBUS_OBJECT_PROXY_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DBUS_OBJECT_PROXY, xdbus_object_proxy_class_t))
#define G_DBUS_OBJECT_PROXY_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DBUS_OBJECT_PROXY, xdbus_object_proxy_class_t))
#define X_IS_DBUS_OBJECT_PROXY(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_OBJECT_PROXY))
#define X_IS_DBUS_OBJECT_PROXY_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DBUS_OBJECT_PROXY))

typedef struct _xdbus_object_proxy_class   xdbus_object_proxy_class_t;
typedef struct _xdbus_object_proxy_private xdbus_object_proxy_private;

/**
 * xdbus_object_proxy_t:
 *
 * The #xdbus_object_proxy_t structure contains private data and should
 * only be accessed using the provided API.
 *
 * Since: 2.30
 */
struct _GDBusObjectProxy
{
  /*< private >*/
  xobject_t parent_instance;
  xdbus_object_proxy_private *priv;
};

/**
 * xdbus_object_proxy_class_t:
 * @parent_class: The parent class.
 *
 * Class structure for #xdbus_object_proxy_t.
 *
 * Since: 2.30
 */
struct _xdbus_object_proxy_class
{
  xobject_class_t parent_class;

  /*< private >*/
  xpointer_t padding[8];
};

XPL_AVAILABLE_IN_ALL
xtype_t             xdbus_object_proxy_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xdbus_object_proxy_t *xdbus_object_proxy_new            (xdbus_connection_t   *connection,
                                                      const xchar_t       *object_path);
XPL_AVAILABLE_IN_ALL
xdbus_connection_t  *xdbus_object_proxy_get_connection (xdbus_object_proxy_t  *proxy);

G_END_DECLS

#endif /* __G_DBUS_OBJECT_PROXY_H */
