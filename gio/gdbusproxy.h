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

#ifndef __G_DBUS_PROXY_H__
#define __G_DBUS_PROXY_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>
#include <gio/gdbusintrospection.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_PROXY         (g_dbus_proxy_get_type ())
#define G_DBUS_PROXY(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_PROXY, xdbus_proxy))
#define G_DBUS_PROXY_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DBUS_PROXY, GDBusProxyClass))
#define G_DBUS_PROXY_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DBUS_PROXY, GDBusProxyClass))
#define X_IS_DBUS_PROXY(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_PROXY))
#define X_IS_DBUS_PROXY_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DBUS_PROXY))

typedef struct _GDBusProxyClass   GDBusProxyClass;
typedef struct _GDBusProxyPrivate GDBusProxyPrivate;

/**
 * xdbus_proxy_t:
 *
 * The #xdbus_proxy_t structure contains only private data and
 * should only be accessed using the provided API.
 *
 * Since: 2.26
 */
struct _GDBusProxy
{
  /*< private >*/
  xobject_t parent_instance;
  GDBusProxyPrivate *priv;
};

/**
 * GDBusProxyClass:
 * @g_properties_changed: Signal class handler for the #xdbus_proxy_t::g-properties-changed signal.
 * @g_signal: Signal class handler for the #xdbus_proxy_t::g-signal signal.
 *
 * Class structure for #xdbus_proxy_t.
 *
 * Since: 2.26
 */
struct _GDBusProxyClass
{
  /*< private >*/
  xobject_class_t parent_class;

  /*< public >*/
  /* Signals */
  void (*g_properties_changed) (xdbus_proxy_t          *proxy,
                                xvariant_t            *changed_properties,
                                const xchar_t* const  *invalidated_properties);
  void (*g_signal)             (xdbus_proxy_t          *proxy,
                                const xchar_t         *sender_name,
                                const xchar_t         *signal_name,
                                xvariant_t            *parameters);

  /*< private >*/
  /* Padding for future expansion */
  xpointer_t padding[32];
};

XPL_AVAILABLE_IN_ALL
xtype_t            g_dbus_proxy_get_type                  (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
void             g_dbus_proxy_new                       (xdbus_connection_t     *connection,
                                                         GDBusProxyFlags      flags,
                                                         xdbus_interface_info_t *info,
                                                         const xchar_t         *name,
                                                         const xchar_t         *object_path,
                                                         const xchar_t         *interface_name,
                                                         xcancellable_t        *cancellable,
                                                         xasync_ready_callback_t  callback,
                                                         xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xdbus_proxy_t      *g_dbus_proxy_new_finish                (xasync_result_t        *res,
                                                         xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xdbus_proxy_t      *g_dbus_proxy_new_sync                  (xdbus_connection_t     *connection,
                                                         GDBusProxyFlags      flags,
                                                         xdbus_interface_info_t *info,
                                                         const xchar_t         *name,
                                                         const xchar_t         *object_path,
                                                         const xchar_t         *interface_name,
                                                         xcancellable_t        *cancellable,
                                                         xerror_t             **error);
XPL_AVAILABLE_IN_ALL
void             g_dbus_proxy_new_for_bus               (GBusType             bus_type,
                                                         GDBusProxyFlags      flags,
                                                         xdbus_interface_info_t *info,
                                                         const xchar_t         *name,
                                                         const xchar_t         *object_path,
                                                         const xchar_t         *interface_name,
                                                         xcancellable_t        *cancellable,
                                                         xasync_ready_callback_t  callback,
                                                         xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xdbus_proxy_t      *g_dbus_proxy_new_for_bus_finish        (xasync_result_t        *res,
                                                         xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xdbus_proxy_t      *g_dbus_proxy_new_for_bus_sync          (GBusType             bus_type,
                                                         GDBusProxyFlags      flags,
                                                         xdbus_interface_info_t *info,
                                                         const xchar_t         *name,
                                                         const xchar_t         *object_path,
                                                         const xchar_t         *interface_name,
                                                         xcancellable_t        *cancellable,
                                                         xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xdbus_connection_t *g_dbus_proxy_get_connection            (xdbus_proxy_t          *proxy);
XPL_AVAILABLE_IN_ALL
GDBusProxyFlags  g_dbus_proxy_get_flags                 (xdbus_proxy_t          *proxy);
XPL_AVAILABLE_IN_ALL
const xchar_t     *g_dbus_proxy_get_name                  (xdbus_proxy_t          *proxy);
XPL_AVAILABLE_IN_ALL
xchar_t           *g_dbus_proxy_get_name_owner            (xdbus_proxy_t          *proxy);
XPL_AVAILABLE_IN_ALL
const xchar_t     *g_dbus_proxy_get_object_path           (xdbus_proxy_t          *proxy);
XPL_AVAILABLE_IN_ALL
const xchar_t     *g_dbus_proxy_get_interface_name        (xdbus_proxy_t          *proxy);
XPL_AVAILABLE_IN_ALL
xint_t             g_dbus_proxy_get_default_timeout       (xdbus_proxy_t          *proxy);
XPL_AVAILABLE_IN_ALL
void             g_dbus_proxy_set_default_timeout       (xdbus_proxy_t          *proxy,
                                                         xint_t                 timeout_msec);
XPL_AVAILABLE_IN_ALL
xdbus_interface_info_t *g_dbus_proxy_get_interface_info     (xdbus_proxy_t          *proxy);
XPL_AVAILABLE_IN_ALL
void             g_dbus_proxy_set_interface_info        (xdbus_proxy_t           *proxy,
                                                         xdbus_interface_info_t   *info);
XPL_AVAILABLE_IN_ALL
xvariant_t        *g_dbus_proxy_get_cached_property       (xdbus_proxy_t          *proxy,
                                                         const xchar_t         *property_name);
XPL_AVAILABLE_IN_ALL
void             g_dbus_proxy_set_cached_property       (xdbus_proxy_t          *proxy,
                                                         const xchar_t         *property_name,
                                                         xvariant_t            *value);
XPL_AVAILABLE_IN_ALL
xchar_t          **g_dbus_proxy_get_cached_property_names (xdbus_proxy_t          *proxy);
XPL_AVAILABLE_IN_ALL
void             g_dbus_proxy_call                      (xdbus_proxy_t          *proxy,
                                                         const xchar_t         *method_name,
                                                         xvariant_t            *parameters,
                                                         GDBusCallFlags       flags,
                                                         xint_t                 timeout_msec,
                                                         xcancellable_t        *cancellable,
                                                         xasync_ready_callback_t  callback,
                                                         xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xvariant_t        *g_dbus_proxy_call_finish               (xdbus_proxy_t          *proxy,
                                                         xasync_result_t        *res,
                                                         xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xvariant_t        *g_dbus_proxy_call_sync                 (xdbus_proxy_t          *proxy,
                                                         const xchar_t         *method_name,
                                                         xvariant_t            *parameters,
                                                         GDBusCallFlags       flags,
                                                         xint_t                 timeout_msec,
                                                         xcancellable_t        *cancellable,
                                                         xerror_t             **error);

XPL_AVAILABLE_IN_ALL
void             g_dbus_proxy_call_with_unix_fd_list        (xdbus_proxy_t          *proxy,
                                                             const xchar_t         *method_name,
                                                             xvariant_t            *parameters,
                                                             GDBusCallFlags       flags,
                                                             xint_t                 timeout_msec,
                                                             xunix_fd_list_t         *fd_list,
                                                             xcancellable_t        *cancellable,
                                                             xasync_ready_callback_t  callback,
                                                             xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xvariant_t        *g_dbus_proxy_call_with_unix_fd_list_finish (xdbus_proxy_t          *proxy,
                                                             xunix_fd_list_t        **out_fd_list,
                                                             xasync_result_t        *res,
                                                             xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xvariant_t        *g_dbus_proxy_call_with_unix_fd_list_sync   (xdbus_proxy_t          *proxy,
                                                             const xchar_t         *method_name,
                                                             xvariant_t            *parameters,
                                                             GDBusCallFlags       flags,
                                                             xint_t                 timeout_msec,
                                                             xunix_fd_list_t         *fd_list,
                                                             xunix_fd_list_t        **out_fd_list,
                                                             xcancellable_t        *cancellable,
                                                             xerror_t             **error);

G_END_DECLS

#endif /* __G_DBUS_PROXY_H__ */
