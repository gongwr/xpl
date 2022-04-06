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

#ifndef __G_DBUS_AUTH_MECHANISM_H__
#define __G_DBUS_AUTH_MECHANISM_H__

#if !defined (GIO_COMPILATION)
#error "gdbusauthmechanism.h is a private header file."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_AUTH_MECHANISM         (_g_dbus_auth_mechanism_get_type ())
#define G_DBUS_AUTH_MECHANISM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_AUTH_MECHANISM, GDBusAuthMechanism))
#define G_DBUS_AUTH_MECHANISM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DBUS_AUTH_MECHANISM, GDBusAuthMechanismClass))
#define G_DBUS_AUTH_MECHANISM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DBUS_AUTH_MECHANISM, GDBusAuthMechanismClass))
#define X_IS_DBUS_AUTH_MECHANISM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_AUTH_MECHANISM))
#define X_IS_DBUS_AUTH_MECHANISM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DBUS_AUTH_MECHANISM))

typedef struct _GDBusAuthMechanism        GDBusAuthMechanism;
typedef struct _GDBusAuthMechanismClass   GDBusAuthMechanismClass;
typedef struct _GDBusAuthMechanismPrivate GDBusAuthMechanismPrivate;

typedef enum {
  G_DBUS_AUTH_MECHANISM_STATE_INVALID,
  G_DBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA,
  G_DBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND,
  G_DBUS_AUTH_MECHANISM_STATE_REJECTED,
  G_DBUS_AUTH_MECHANISM_STATE_ACCEPTED,
} GDBusAuthMechanismState;

struct _GDBusAuthMechanismClass
{
  /*< private >*/
  xobject_class_t parent_class;

  /*< public >*/

  /* VTable */

  /* TODO: server_initiate and client_initiate probably needs to have a
   * xcredentials_t parameter...
   */

  xint_t                      (*get_priority)             (void);
  const xchar_t              *(*get_name)                 (void);

  /* functions shared by server/client */
  xboolean_t                  (*is_supported)             (GDBusAuthMechanism   *mechanism);
  xchar_t                    *(*encode_data)              (GDBusAuthMechanism   *mechanism,
                                                         const xchar_t          *data,
                                                         xsize_t                 data_len,
                                                         xsize_t                *out_data_len);
  xchar_t                    *(*decode_data)              (GDBusAuthMechanism   *mechanism,
                                                         const xchar_t          *data,
                                                         xsize_t                 data_len,
                                                         xsize_t                *out_data_len);

  /* functions for server-side authentication */
  GDBusAuthMechanismState   (*server_get_state)         (GDBusAuthMechanism   *mechanism);
  void                      (*server_initiate)          (GDBusAuthMechanism   *mechanism,
                                                         const xchar_t          *initial_response,
                                                         xsize_t                 initial_response_len);
  void                      (*server_data_receive)      (GDBusAuthMechanism   *mechanism,
                                                         const xchar_t          *data,
                                                         xsize_t                 data_len);
  xchar_t                    *(*server_data_send)         (GDBusAuthMechanism   *mechanism,
                                                         xsize_t                *out_data_len);
  xchar_t                    *(*server_get_reject_reason) (GDBusAuthMechanism   *mechanism);
  void                      (*server_shutdown)          (GDBusAuthMechanism   *mechanism);

  /* functions for client-side authentication */
  GDBusAuthMechanismState   (*client_get_state)         (GDBusAuthMechanism   *mechanism);
  xchar_t                    *(*client_initiate)          (GDBusAuthMechanism   *mechanism,
                                                         xsize_t                *out_initial_response_len);
  void                      (*client_data_receive)      (GDBusAuthMechanism   *mechanism,
                                                         const xchar_t          *data,
                                                         xsize_t                 data_len);
  xchar_t                    *(*client_data_send)         (GDBusAuthMechanism   *mechanism,
                                                         xsize_t                *out_data_len);
  void                      (*client_shutdown)          (GDBusAuthMechanism   *mechanism);
};

struct _GDBusAuthMechanism
{
  xobject_t parent_instance;
  GDBusAuthMechanismPrivate *priv;
};

xtype_t                     _g_dbus_auth_mechanism_get_type                 (void) G_GNUC_CONST;

xint_t                      _g_dbus_auth_mechanism_get_priority             (xtype_t                 mechanism_type);
const xchar_t              *_g_dbus_auth_mechanism_get_name                 (xtype_t                 mechanism_type);

xio_stream_t                *_g_dbus_auth_mechanism_get_stream               (GDBusAuthMechanism   *mechanism);
xcredentials_t             *_g_dbus_auth_mechanism_get_credentials          (GDBusAuthMechanism   *mechanism);

xboolean_t                  _g_dbus_auth_mechanism_is_supported             (GDBusAuthMechanism   *mechanism);
xchar_t                    *_g_dbus_auth_mechanism_encode_data              (GDBusAuthMechanism   *mechanism,
                                                                           const xchar_t          *data,
                                                                           xsize_t                 data_len,
                                                                           xsize_t                *out_data_len);
xchar_t                    *_g_dbus_auth_mechanism_decode_data              (GDBusAuthMechanism   *mechanism,
                                                                           const xchar_t          *data,
                                                                           xsize_t                 data_len,
                                                                           xsize_t                *out_data_len);

GDBusAuthMechanismState   _g_dbus_auth_mechanism_server_get_state         (GDBusAuthMechanism   *mechanism);
void                      _g_dbus_auth_mechanism_server_initiate          (GDBusAuthMechanism   *mechanism,
                                                                           const xchar_t          *initial_response,
                                                                           xsize_t                 initial_response_len);
void                      _g_dbus_auth_mechanism_server_data_receive      (GDBusAuthMechanism   *mechanism,
                                                                           const xchar_t          *data,
                                                                           xsize_t                 data_len);
xchar_t                    *_g_dbus_auth_mechanism_server_data_send         (GDBusAuthMechanism   *mechanism,
                                                                           xsize_t                *out_data_len);
xchar_t                    *_g_dbus_auth_mechanism_server_get_reject_reason (GDBusAuthMechanism   *mechanism);
void                      _g_dbus_auth_mechanism_server_shutdown          (GDBusAuthMechanism   *mechanism);

GDBusAuthMechanismState   _g_dbus_auth_mechanism_client_get_state         (GDBusAuthMechanism   *mechanism);
xchar_t                    *_g_dbus_auth_mechanism_client_initiate          (GDBusAuthMechanism   *mechanism,
                                                                           xsize_t                *out_initial_response_len);
void                      _g_dbus_auth_mechanism_client_data_receive      (GDBusAuthMechanism   *mechanism,
                                                                           const xchar_t          *data,
                                                                           xsize_t                 data_len);
xchar_t                    *_g_dbus_auth_mechanism_client_data_send         (GDBusAuthMechanism   *mechanism,
                                                                          xsize_t                *out_data_len);
void                      _g_dbus_auth_mechanism_client_shutdown          (GDBusAuthMechanism   *mechanism);


G_END_DECLS

#endif /* __G_DBUS_AUTH_MECHANISM_H__ */
