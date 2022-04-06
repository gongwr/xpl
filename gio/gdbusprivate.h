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

#ifndef __G_DBUS_PRIVATE_H__
#define __G_DBUS_PRIVATE_H__

#if !defined (GIO_COMPILATION)
#error "gdbusprivate.h is a private header file."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

/* ---------------------------------------------------------------------------------------------------- */

typedef struct GDBusWorker GDBusWorker;

typedef void (*GDBusWorkerMessageReceivedCallback) (GDBusWorker   *worker,
                                                    xdbus_message_t  *message,
                                                    xpointer_t       user_data);

typedef xdbus_message_t *(*GDBusWorkerMessageAboutToBeSentCallback) (GDBusWorker   *worker,
                                                                  xdbus_message_t  *message,
                                                                  xpointer_t       user_data);

typedef void (*GDBusWorkerDisconnectedCallback)    (GDBusWorker   *worker,
                                                    xboolean_t       remote_peer_vanished,
                                                    xerror_t        *error,
                                                    xpointer_t       user_data);

/* This function may be called from any thread - callbacks will be in the shared private message thread
 * and must not block.
 */
GDBusWorker *_g_dbus_worker_new          (xio_stream_t                          *stream,
                                          GDBusCapabilityFlags                capabilities,
                                          xboolean_t                            initially_frozen,
                                          GDBusWorkerMessageReceivedCallback  message_received_callback,
                                          GDBusWorkerMessageAboutToBeSentCallback message_about_to_be_sent_callback,
                                          GDBusWorkerDisconnectedCallback     disconnected_callback,
                                          xpointer_t                            user_data);

/* can be called from any thread - steals blob */
void         _g_dbus_worker_send_message (GDBusWorker    *worker,
                                          xdbus_message_t   *message,
                                          xchar_t          *blob,
                                          xsize_t           blob_len);

/* can be called from any thread */
void         _g_dbus_worker_stop         (GDBusWorker    *worker);

/* can be called from any thread */
void         _g_dbus_worker_unfreeze     (GDBusWorker    *worker);

/* can be called from any thread (except the worker thread) */
xboolean_t     _g_dbus_worker_flush_sync   (GDBusWorker    *worker,
                                          xcancellable_t   *cancellable,
                                          xerror_t        **error);

/* can be called from any thread */
void         _g_dbus_worker_close        (GDBusWorker         *worker,
                                          xtask_t               *task);

/* ---------------------------------------------------------------------------------------------------- */

void _g_dbus_initialize (void);
xboolean_t _g_dbus_debug_authentication (void);
xboolean_t _g_dbus_debug_transport (void);
xboolean_t _g_dbus_debug_message (void);
xboolean_t _g_dbus_debug_payload (void);
xboolean_t _g_dbus_debug_call    (void);
xboolean_t _g_dbus_debug_signal  (void);
xboolean_t _g_dbus_debug_incoming (void);
xboolean_t _g_dbus_debug_return (void);
xboolean_t _g_dbus_debug_emission (void);
xboolean_t _g_dbus_debug_address (void);
xboolean_t _g_dbus_debug_proxy (void);

void     _g_dbus_debug_print_lock (void);
void     _g_dbus_debug_print_unlock (void);

xboolean_t _g_dbus_address_parse_entry (const xchar_t  *address_entry,
                                      xchar_t       **out_transport_name,
                                      xhashtable_t  **out_key_value_pairs,
                                      xerror_t      **error);

xvariant_type_t * _g_dbus_compute_complete_signature (xdbus_arg_info_t **args);

xchar_t *_g_dbus_hexdump (const xchar_t *data, xsize_t len, xuint_t indent);

/* ---------------------------------------------------------------------------------------------------- */

#ifdef G_OS_WIN32
xchar_t *_g_dbus_win32_get_user_sid (void);

#define _GDBUS_ARG_WIN32_RUN_SESSION_BUS "_win32_run_session_bus"
/* The g_win32_run_session_bus is exported from libgio dll on win32,
 * but still is NOT part of API/ABI since it is declared in private header
 * and used only by tool built from same sources.
 * Initially this function was introduces for usage with rundll,
 * so the signature is kept rundll-compatible, though parameters aren't used.
 */
_XPL_EXTERN void __stdcall
g_win32_run_session_bus (void* hwnd, void* hinst, const char* cmdline, int cmdshow);
xchar_t *_g_dbus_win32_get_session_address_dbus_launch (xerror_t **error);
#endif

xchar_t *_g_dbus_get_machine_id (xerror_t **error);

xchar_t *_g_dbus_enum_to_string (xtype_t enum_type, xint_t value);

/* ---------------------------------------------------------------------------------------------------- */

xdbus_method_invocation_t *_xdbus_method_invocation_new (const xchar_t             *sender,
                                                      const xchar_t             *object_path,
                                                      const xchar_t             *interface_name,
                                                      const xchar_t             *method_name,
                                                      const xdbus_method_info_t   *method_info,
                                                      const xdbus_property_info_t *property_info,
                                                      xdbus_connection_t         *connection,
                                                      xdbus_message_t            *message,
                                                      xvariant_t                *parameters,
                                                      xpointer_t                 user_data);

/* ---------------------------------------------------------------------------------------------------- */

xboolean_t _g_signal_accumulator_false_handled (xsignal_invocation_hint_t *ihint,
                                              xvalue_t                *return_accu,
                                              const xvalue_t          *handler_return,
                                              xpointer_t               dummy);

xboolean_t _g_dbus_object_skeleton_has_authorize_method_handlers (xdbus_object_skeleton_t *object);

void _g_dbus_object_proxy_add_interface (xdbus_object_proxy_t *proxy,
                                         xdbus_proxy_t       *interface_proxy);
void _g_dbus_object_proxy_remove_interface (xdbus_object_proxy_t *proxy,
                                            const xchar_t      *interface_name);

xchar_t *_g_dbus_hexencode (const xchar_t *str,
                          xsize_t        str_len);

/* Implemented in gdbusconnection.c */
xdbus_connection_t *_g_bus_get_singleton_if_exists (GBusType bus_type);
void             _g_bus_forget_singleton        (GBusType bus_type);

G_END_DECLS

#endif /* __G_DBUS_PRIVATE_H__ */
