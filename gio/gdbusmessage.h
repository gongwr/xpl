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

#ifndef __G_DBUS_MESSAGE_H__
#define __G_DBUS_MESSAGE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_MESSAGE         (xdbus_message_get_type ())
#define G_DBUS_MESSAGE(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_MESSAGE, xdbus_message))
#define X_IS_DBUS_MESSAGE(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_MESSAGE))

XPL_AVAILABLE_IN_ALL
xtype_t                     xdbus_message_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xdbus_message_t             *xdbus_message_new                (void);
XPL_AVAILABLE_IN_ALL
xdbus_message_t             *xdbus_message_new_signal         (const xchar_t              *path,
                                                             const xchar_t              *interface_,
                                                             const xchar_t              *signal);
XPL_AVAILABLE_IN_ALL
xdbus_message_t             *xdbus_message_new_method_call    (const xchar_t              *name,
                                                             const xchar_t              *path,
                                                             const xchar_t              *interface_,
                                                             const xchar_t              *method);
XPL_AVAILABLE_IN_ALL
xdbus_message_t             *xdbus_message_new_method_reply   (xdbus_message_t             *method_call_message);
XPL_AVAILABLE_IN_ALL
xdbus_message_t             *xdbus_message_new_method_error   (xdbus_message_t             *method_call_message,
                                                             const xchar_t              *error_name,
                                                             const xchar_t              *error_message_format,
                                                             ...) G_GNUC_PRINTF(3, 4);
XPL_AVAILABLE_IN_ALL
xdbus_message_t             *xdbus_message_new_method_error_valist (xdbus_message_t             *method_call_message,
                                                                  const xchar_t              *error_name,
                                                                  const xchar_t              *error_message_format,
                                                                  va_list                   var_args);
XPL_AVAILABLE_IN_ALL
xdbus_message_t             *xdbus_message_new_method_error_literal (xdbus_message_t             *method_call_message,
                                                                   const xchar_t              *error_name,
                                                                   const xchar_t              *error_message);
XPL_AVAILABLE_IN_ALL
xchar_t                    *xdbus_message_print              (xdbus_message_t             *message,
                                                             xuint_t                     indent);
XPL_AVAILABLE_IN_ALL
xboolean_t                  xdbus_message_get_locked         (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_lock               (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
xdbus_message_t             *xdbus_message_copy               (xdbus_message_t             *message,
                                                             xerror_t                  **error);
XPL_AVAILABLE_IN_ALL
GDBusMessageByteOrder     xdbus_message_get_byte_order     (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_byte_order     (xdbus_message_t             *message,
                                                             GDBusMessageByteOrder     byte_order);

XPL_AVAILABLE_IN_ALL
GDBusMessageType          xdbus_message_get_message_type   (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_message_type   (xdbus_message_t             *message,
                                                             GDBusMessageType          type);
XPL_AVAILABLE_IN_ALL
GDBusMessageFlags         xdbus_message_get_flags          (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_flags          (xdbus_message_t             *message,
                                                             GDBusMessageFlags         flags);
XPL_AVAILABLE_IN_ALL
xuint32_t                   xdbus_message_get_serial         (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_serial         (xdbus_message_t             *message,
                                                             xuint32_t                   serial);
XPL_AVAILABLE_IN_ALL
xvariant_t                 *xdbus_message_get_header         (xdbus_message_t             *message,
                                                             GDBusMessageHeaderField   header_field);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_header         (xdbus_message_t             *message,
                                                             GDBusMessageHeaderField   header_field,
                                                             xvariant_t                 *value);
XPL_AVAILABLE_IN_ALL
xuchar_t                   *xdbus_message_get_header_fields  (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
xvariant_t                 *xdbus_message_get_body           (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_body           (xdbus_message_t             *message,
                                                             xvariant_t                 *body);
XPL_AVAILABLE_IN_ALL
xunix_fd_list_t              *xdbus_message_get_unix_fd_list   (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_unix_fd_list   (xdbus_message_t             *message,
                                                             xunix_fd_list_t              *fd_list);

XPL_AVAILABLE_IN_ALL
xuint32_t                   xdbus_message_get_reply_serial   (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_reply_serial   (xdbus_message_t             *message,
                                                             xuint32_t                   value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *xdbus_message_get_interface      (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_interface      (xdbus_message_t             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *xdbus_message_get_member         (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_member         (xdbus_message_t             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *xdbus_message_get_path           (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_path           (xdbus_message_t             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *xdbus_message_get_sender         (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_sender         (xdbus_message_t             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *xdbus_message_get_destination    (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_destination    (xdbus_message_t             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *xdbus_message_get_error_name     (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_error_name     (xdbus_message_t             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *xdbus_message_get_signature      (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_signature      (xdbus_message_t             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
xuint32_t                   xdbus_message_get_num_unix_fds   (xdbus_message_t             *message);
XPL_AVAILABLE_IN_ALL
void                      xdbus_message_set_num_unix_fds   (xdbus_message_t             *message,
                                                             xuint32_t                   value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *xdbus_message_get_arg0           (xdbus_message_t             *message);


XPL_AVAILABLE_IN_ALL
xdbus_message_t             *xdbus_message_new_from_blob      (xuchar_t                   *blob,
                                                             xsize_t                     blob_len,
                                                             xdbus_capability_flags_t      capabilities,
                                                             xerror_t                  **error);

XPL_AVAILABLE_IN_ALL
xssize_t                    xdbus_message_bytes_needed       (xuchar_t                   *blob,
                                                             xsize_t                     blob_len,
                                                             xerror_t                  **error);

XPL_AVAILABLE_IN_ALL
xuchar_t                   *xdbus_message_to_blob            (xdbus_message_t             *message,
                                                             xsize_t                    *out_size,
                                                             xdbus_capability_flags_t      capabilities,
                                                             xerror_t                  **error);

XPL_AVAILABLE_IN_ALL
xboolean_t                  xdbus_message_to_gerror          (xdbus_message_t             *message,
                                                             xerror_t                  **error);

G_END_DECLS

#endif /* __G_DBUS_MESSAGE_H__ */
