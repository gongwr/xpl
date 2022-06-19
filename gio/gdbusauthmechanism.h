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

#ifndef __XDBUS_AUTH_MECHANISM_H__
#define __XDBUS_AUTH_MECHANISM_H__

#if !defined (GIO_COMPILATION)
#error "gdbusauthmechanism.h is a private header file."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_AUTH_MECHANISM         (_xdbus_auth_mechanism_get_type ())
#define XDBUS_AUTH_MECHANISM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_AUTH_MECHANISM, xdbus_auth_mechanism_t))
#define XDBUS_AUTH_MECHANISM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DBUS_AUTH_MECHANISM, xdbus_auth_mechanism_class_t))
#define XDBUS_AUTH_MECHANISM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DBUS_AUTH_MECHANISM, xdbus_auth_mechanism_class_t))
#define X_IS_DBUS_AUTH_MECHANISM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_AUTH_MECHANISM))
#define X_IS_DBUS_AUTH_MECHANISM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DBUS_AUTH_MECHANISM))

typedef struct _xdbus_auth_mechanism        xdbus_auth_mechanism_t;
typedef struct _xdbus_auth_mechanism_class   xdbus_auth_mechanism_class_t;
typedef struct _xdbus_auth_mechanism_private xdbus_auth_mechanism_private_t;

typedef enum {
  XDBUS_AUTH_MECHANISM_STATE_INVALID,
  XDBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA,
  XDBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND,
  XDBUS_AUTH_MECHANISM_STATE_REJECTED,
  XDBUS_AUTH_MECHANISM_STATE_ACCEPTED,
} xdbus_auth_mechanism_state_t;

struct _xdbus_auth_mechanism_class
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
  xboolean_t                  (*is_supported)             (xdbus_auth_mechanism_t   *mechanism);
  xchar_t                    *(*encode_data)              (xdbus_auth_mechanism_t   *mechanism,
                                                         const xchar_t          *data,
                                                         xsize_t                 data_len,
                                                         xsize_t                *out_data_len);
  xchar_t                    *(*decode_data)              (xdbus_auth_mechanism_t   *mechanism,
                                                         const xchar_t          *data,
                                                         xsize_t                 data_len,
                                                         xsize_t                *out_data_len);

  /* functions for server-side authentication */
  xdbus_auth_mechanism_state_t
   (*server_get_state)         (xdbus_auth_mechanism_t   *mechanism);
  void                      (*server_initiate)          (xdbus_auth_mechanism_t   *mechanism,
                                                         const xchar_t          *initial_response,
                                                         xsize_t                 initial_response_len);
  void                      (*server_data_receive)      (xdbus_auth_mechanism_t   *mechanism,
                                                         const xchar_t          *data,
                                                         xsize_t                 data_len);
  xchar_t                    *(*server_data_send)         (xdbus_auth_mechanism_t   *mechanism,
                                                         xsize_t                *out_data_len);
  xchar_t                    *(*server_get_reject_reason) (xdbus_auth_mechanism_t   *mechanism);
  void                      (*server_shutdown)          (xdbus_auth_mechanism_t   *mechanism);

  /* functions for client-side authentication */
  xdbus_auth_mechanism_state_t
   (*client_get_state)         (xdbus_auth_mechanism_t   *mechanism);
  xchar_t                    *(*client_initiate)          (xdbus_auth_mechanism_t   *mechanism,
                                                         xsize_t                *out_initial_response_len);
  void                      (*client_data_receive)      (xdbus_auth_mechanism_t   *mechanism,
                                                         const xchar_t          *data,
                                                         xsize_t                 data_len);
  xchar_t                    *(*client_data_send)         (xdbus_auth_mechanism_t   *mechanism,
                                                         xsize_t                *out_data_len);
  void                      (*client_shutdown)          (xdbus_auth_mechanism_t   *mechanism);
};

struct _xdbus_auth_mechanism
{
  xobject_t parent_instance;
  xdbus_auth_mechanism_private_t
 *priv;
};

xtype_t                     _xdbus_auth_mechanism_get_type                 (void) G_GNUC_CONST;

xint_t                      _xdbus_auth_mechanism_get_priority             (xtype_t                 mechanism_type);
const xchar_t              *_xdbus_auth_mechanism_get_name                 (xtype_t                 mechanism_type);

xio_stream_t                *_xdbus_auth_mechanism_get_stream               (xdbus_auth_mechanism_t   *mechanism);
xcredentials_t             *_xdbus_auth_mechanism_get_credentials          (xdbus_auth_mechanism_t   *mechanism);

xboolean_t                  _xdbus_auth_mechanism_is_supported             (xdbus_auth_mechanism_t   *mechanism);
xchar_t                    *_xdbus_auth_mechanism_encode_data              (xdbus_auth_mechanism_t   *mechanism,
                                                                           const xchar_t          *data,
                                                                           xsize_t                 data_len,
                                                                           xsize_t                *out_data_len);
xchar_t                    *_xdbus_auth_mechanism_decode_data              (xdbus_auth_mechanism_t   *mechanism,
                                                                           const xchar_t          *data,
                                                                           xsize_t                 data_len,
                                                                           xsize_t                *out_data_len);

xdbus_auth_mechanism_state_t
   _xdbus_auth_mechanism_server_get_state         (xdbus_auth_mechanism_t   *mechanism);
void                      _xdbus_auth_mechanism_server_initiate          (xdbus_auth_mechanism_t   *mechanism,
                                                                           const xchar_t          *initial_response,
                                                                           xsize_t                 initial_response_len);
void                      _xdbus_auth_mechanism_server_data_receive      (xdbus_auth_mechanism_t   *mechanism,
                                                                           const xchar_t          *data,
                                                                           xsize_t                 data_len);
xchar_t                    *_xdbus_auth_mechanism_server_data_send         (xdbus_auth_mechanism_t   *mechanism,
                                                                           xsize_t                *out_data_len);
xchar_t                    *_xdbus_auth_mechanism_server_get_reject_reason (xdbus_auth_mechanism_t   *mechanism);
void                      _xdbus_auth_mechanism_server_shutdown          (xdbus_auth_mechanism_t   *mechanism);

xdbus_auth_mechanism_state_t
   _xdbus_auth_mechanism_client_get_state         (xdbus_auth_mechanism_t   *mechanism);
xchar_t                    *_xdbus_auth_mechanism_client_initiate          (xdbus_auth_mechanism_t   *mechanism,
                                                                           xsize_t                *out_initial_response_len);
void                      _xdbus_auth_mechanism_client_data_receive      (xdbus_auth_mechanism_t   *mechanism,
                                                                           const xchar_t          *data,
                                                                           xsize_t                 data_len);
xchar_t                    *_xdbus_auth_mechanism_client_data_send         (xdbus_auth_mechanism_t   *mechanism,
                                                                          xsize_t                *out_data_len);
void                      _xdbus_auth_mechanism_client_shutdown          (xdbus_auth_mechanism_t   *mechanism);


G_END_DECLS

#endif /* __XDBUS_AUTH_MECHANISM_H__ */
