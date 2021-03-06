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

#ifndef __G_DBUS_METHOD_INVOCATION_H__
#define __G_DBUS_METHOD_INVOCATION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_METHOD_INVOCATION         (xdbus_method_invocation_get_type ())
#define G_DBUS_METHOD_INVOCATION(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_METHOD_INVOCATION, xdbus_method_invocation))
#define X_IS_DBUS_METHOD_INVOCATION(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_METHOD_INVOCATION))

/**
 * G_DBUS_METHOD_INVOCATION_HANDLED:
 *
 * The value returned by handlers of the signals generated by
 * the `gdbus-codegen` tool to indicate that a method call has been
 * handled by an implementation. It is equal to %TRUE, but using
 * this macro is sometimes more readable.
 *
 * In code that needs to be backwards-compatible with older GLib,
 * use %TRUE instead, often written like this:
 *
 * |[
 *   xdbus_method_invocation_return_error (invocation, ...);
 *   return TRUE;    // handled
 * ]|
 *
 * Since: 2.68
 */
#define G_DBUS_METHOD_INVOCATION_HANDLED TRUE XPL_AVAILABLE_MACRO_IN_2_68

/**
 * G_DBUS_METHOD_INVOCATION_UNHANDLED:
 *
 * The value returned by handlers of the signals generated by
 * the `gdbus-codegen` tool to indicate that a method call has not been
 * handled by an implementation. It is equal to %FALSE, but using
 * this macro is sometimes more readable.
 *
 * In code that needs to be backwards-compatible with older GLib,
 * use %FALSE instead.
 *
 * Since: 2.68
 */
#define G_DBUS_METHOD_INVOCATION_UNHANDLED FALSE XPL_AVAILABLE_MACRO_IN_2_68

XPL_AVAILABLE_IN_ALL
xtype_t                  xdbus_method_invocation_get_type             (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
const xchar_t           *xdbus_method_invocation_get_sender           (xdbus_method_invocation_t *invocation);
XPL_AVAILABLE_IN_ALL
const xchar_t           *xdbus_method_invocation_get_object_path      (xdbus_method_invocation_t *invocation);
XPL_AVAILABLE_IN_ALL
const xchar_t           *xdbus_method_invocation_get_interface_name   (xdbus_method_invocation_t *invocation);
XPL_AVAILABLE_IN_ALL
const xchar_t           *xdbus_method_invocation_get_method_name      (xdbus_method_invocation_t *invocation);
XPL_AVAILABLE_IN_ALL
const xdbus_method_info_t *xdbus_method_invocation_get_method_info      (xdbus_method_invocation_t *invocation);
XPL_AVAILABLE_IN_2_38
const xdbus_property_info_t *xdbus_method_invocation_get_property_info  (xdbus_method_invocation_t *invocation);
XPL_AVAILABLE_IN_ALL
xdbus_connection_t       *xdbus_method_invocation_get_connection       (xdbus_method_invocation_t *invocation);
XPL_AVAILABLE_IN_ALL
xdbus_message_t          *xdbus_method_invocation_get_message          (xdbus_method_invocation_t *invocation);
XPL_AVAILABLE_IN_ALL
xvariant_t              *xdbus_method_invocation_get_parameters       (xdbus_method_invocation_t *invocation);
XPL_AVAILABLE_IN_ALL
xpointer_t               xdbus_method_invocation_get_user_data        (xdbus_method_invocation_t *invocation);

XPL_AVAILABLE_IN_ALL
void                   xdbus_method_invocation_return_value         (xdbus_method_invocation_t *invocation,
                                                                      xvariant_t              *parameters);
XPL_AVAILABLE_IN_ALL
void                   xdbus_method_invocation_return_value_with_unix_fd_list (xdbus_method_invocation_t *invocation,
                                                                                xvariant_t              *parameters,
                                                                                xunix_fd_list_t           *fd_list);
XPL_AVAILABLE_IN_ALL
void                   xdbus_method_invocation_return_error         (xdbus_method_invocation_t *invocation,
                                                                      xquark                 domain,
                                                                      xint_t                   code,
                                                                      const xchar_t           *format,
                                                                      ...) G_GNUC_PRINTF(4, 5);
XPL_AVAILABLE_IN_ALL
void                   xdbus_method_invocation_return_error_valist  (xdbus_method_invocation_t *invocation,
                                                                      xquark                 domain,
                                                                      xint_t                   code,
                                                                      const xchar_t           *format,
                                                                      va_list                var_args)
                                                                      G_GNUC_PRINTF(4, 0);
XPL_AVAILABLE_IN_ALL
void                   xdbus_method_invocation_return_error_literal (xdbus_method_invocation_t *invocation,
                                                                      xquark                 domain,
                                                                      xint_t                   code,
                                                                      const xchar_t           *message);
XPL_AVAILABLE_IN_ALL
void                   xdbus_method_invocation_return_gerror        (xdbus_method_invocation_t *invocation,
                                                                      const xerror_t          *error);
XPL_AVAILABLE_IN_ALL
void                   xdbus_method_invocation_take_error           (xdbus_method_invocation_t *invocation,
                                                                      xerror_t                *error);
XPL_AVAILABLE_IN_ALL
void                   xdbus_method_invocation_return_dbus_error    (xdbus_method_invocation_t *invocation,
                                                                      const xchar_t           *error_name,
                                                                      const xchar_t           *error_message);

G_END_DECLS

#endif /* __G_DBUS_METHOD_INVOCATION_H__ */
