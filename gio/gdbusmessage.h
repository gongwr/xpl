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

#define XTYPE_DBUS_MESSAGE         (g_dbus_message_get_type ())
#define G_DBUS_MESSAGE(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_MESSAGE, GDBusMessage))
#define X_IS_DBUS_MESSAGE(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_MESSAGE))

XPL_AVAILABLE_IN_ALL
xtype_t                     g_dbus_message_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
GDBusMessage             *g_dbus_message_new                (void);
XPL_AVAILABLE_IN_ALL
GDBusMessage             *g_dbus_message_new_signal         (const xchar_t              *path,
                                                             const xchar_t              *interface_,
                                                             const xchar_t              *signal);
XPL_AVAILABLE_IN_ALL
GDBusMessage             *g_dbus_message_new_method_call    (const xchar_t              *name,
                                                             const xchar_t              *path,
                                                             const xchar_t              *interface_,
                                                             const xchar_t              *method);
XPL_AVAILABLE_IN_ALL
GDBusMessage             *g_dbus_message_new_method_reply   (GDBusMessage             *method_call_message);
XPL_AVAILABLE_IN_ALL
GDBusMessage             *g_dbus_message_new_method_error   (GDBusMessage             *method_call_message,
                                                             const xchar_t              *error_name,
                                                             const xchar_t              *error_message_format,
                                                             ...) G_GNUC_PRINTF(3, 4);
XPL_AVAILABLE_IN_ALL
GDBusMessage             *g_dbus_message_new_method_error_valist (GDBusMessage             *method_call_message,
                                                                  const xchar_t              *error_name,
                                                                  const xchar_t              *error_message_format,
                                                                  va_list                   var_args);
XPL_AVAILABLE_IN_ALL
GDBusMessage             *g_dbus_message_new_method_error_literal (GDBusMessage             *method_call_message,
                                                                   const xchar_t              *error_name,
                                                                   const xchar_t              *error_message);
XPL_AVAILABLE_IN_ALL
xchar_t                    *g_dbus_message_print              (GDBusMessage             *message,
                                                             xuint_t                     indent);
XPL_AVAILABLE_IN_ALL
xboolean_t                  g_dbus_message_get_locked         (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_lock               (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
GDBusMessage             *g_dbus_message_copy               (GDBusMessage             *message,
                                                             xerror_t                  **error);
XPL_AVAILABLE_IN_ALL
GDBusMessageByteOrder     g_dbus_message_get_byte_order     (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_byte_order     (GDBusMessage             *message,
                                                             GDBusMessageByteOrder     byte_order);

XPL_AVAILABLE_IN_ALL
GDBusMessageType          g_dbus_message_get_message_type   (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_message_type   (GDBusMessage             *message,
                                                             GDBusMessageType          type);
XPL_AVAILABLE_IN_ALL
GDBusMessageFlags         g_dbus_message_get_flags          (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_flags          (GDBusMessage             *message,
                                                             GDBusMessageFlags         flags);
XPL_AVAILABLE_IN_ALL
guint32                   g_dbus_message_get_serial         (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_serial         (GDBusMessage             *message,
                                                             guint32                   serial);
XPL_AVAILABLE_IN_ALL
xvariant_t                 *g_dbus_message_get_header         (GDBusMessage             *message,
                                                             GDBusMessageHeaderField   header_field);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_header         (GDBusMessage             *message,
                                                             GDBusMessageHeaderField   header_field,
                                                             xvariant_t                 *value);
XPL_AVAILABLE_IN_ALL
guchar                   *g_dbus_message_get_header_fields  (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
xvariant_t                 *g_dbus_message_get_body           (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_body           (GDBusMessage             *message,
                                                             xvariant_t                 *body);
XPL_AVAILABLE_IN_ALL
GUnixFDList              *g_dbus_message_get_unix_fd_list   (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_unix_fd_list   (GDBusMessage             *message,
                                                             GUnixFDList              *fd_list);

XPL_AVAILABLE_IN_ALL
guint32                   g_dbus_message_get_reply_serial   (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_reply_serial   (GDBusMessage             *message,
                                                             guint32                   value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *g_dbus_message_get_interface      (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_interface      (GDBusMessage             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *g_dbus_message_get_member         (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_member         (GDBusMessage             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *g_dbus_message_get_path           (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_path           (GDBusMessage             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *g_dbus_message_get_sender         (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_sender         (GDBusMessage             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *g_dbus_message_get_destination    (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_destination    (GDBusMessage             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *g_dbus_message_get_error_name     (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_error_name     (GDBusMessage             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *g_dbus_message_get_signature      (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_signature      (GDBusMessage             *message,
                                                             const xchar_t              *value);

XPL_AVAILABLE_IN_ALL
guint32                   g_dbus_message_get_num_unix_fds   (GDBusMessage             *message);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_message_set_num_unix_fds   (GDBusMessage             *message,
                                                             guint32                   value);

XPL_AVAILABLE_IN_ALL
const xchar_t              *g_dbus_message_get_arg0           (GDBusMessage             *message);


XPL_AVAILABLE_IN_ALL
GDBusMessage             *g_dbus_message_new_from_blob      (guchar                   *blob,
                                                             xsize_t                     blob_len,
                                                             GDBusCapabilityFlags      capabilities,
                                                             xerror_t                  **error);

XPL_AVAILABLE_IN_ALL
gssize                    g_dbus_message_bytes_needed       (guchar                   *blob,
                                                             xsize_t                     blob_len,
                                                             xerror_t                  **error);

XPL_AVAILABLE_IN_ALL
guchar                   *g_dbus_message_to_blob            (GDBusMessage             *message,
                                                             xsize_t                    *out_size,
                                                             GDBusCapabilityFlags      capabilities,
                                                             xerror_t                  **error);

XPL_AVAILABLE_IN_ALL
xboolean_t                  g_dbus_message_to_gerror          (GDBusMessage             *message,
                                                             xerror_t                  **error);

G_END_DECLS

#endif /* __G_DBUS_MESSAGE_H__ */
