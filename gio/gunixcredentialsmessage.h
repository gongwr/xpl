/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Red Hat, Inc.
 * Copyright (C) 2009 Codethink Limited
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
 * Authors: David Zeuthen <davidz@redhat.com>
 */

#ifndef __G_UNIX_CREDENTIALS_MESSAGE_H__
#define __G_UNIX_CREDENTIALS_MESSAGE_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define XTYPE_UNIX_CREDENTIALS_MESSAGE         (g_unix_credentials_message_get_type ())
#define G_UNIX_CREDENTIALS_MESSAGE(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_UNIX_CREDENTIALS_MESSAGE, xunix_credentials_message))
#define G_UNIX_CREDENTIALS_MESSAGE_CLASS(c)     (XTYPE_CHECK_CLASS_CAST ((c), XTYPE_UNIX_CREDENTIALS_MESSAGE, GUnixCredentialsMessageClass))
#define X_IS_UNIX_CREDENTIALS_MESSAGE(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_UNIX_CREDENTIALS_MESSAGE))
#define X_IS_UNIX_CREDENTIALS_MESSAGE_CLASS(c)  (XTYPE_CHECK_CLASS_TYPE ((c), XTYPE_UNIX_CREDENTIALS_MESSAGE))
#define G_UNIX_CREDENTIALS_MESSAGE_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_UNIX_CREDENTIALS_MESSAGE, GUnixCredentialsMessageClass))

typedef struct _GUnixCredentialsMessagePrivate  GUnixCredentialsMessagePrivate;
typedef struct _GUnixCredentialsMessageClass    GUnixCredentialsMessageClass;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(xunix_credentials_message, xobject_unref)

/**
 * GUnixCredentialsMessageClass:
 *
 * Class structure for #xunix_credentials_message_t.
 *
 * Since: 2.26
 */
struct _GUnixCredentialsMessageClass
{
  xsocket_control_message_class_t parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
};

/**
 * xunix_credentials_message_t:
 *
 * The #xunix_credentials_message_t structure contains only private data
 * and should only be accessed using the provided API.
 *
 * Since: 2.26
 */
struct _GUnixCredentialsMessage
{
  xsocket_control_message_t parent_instance;
  GUnixCredentialsMessagePrivate *priv;
};

XPL_AVAILABLE_IN_ALL
xtype_t                  g_unix_credentials_message_get_type             (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xsocket_control_message_t *g_unix_credentials_message_new                  (void);
XPL_AVAILABLE_IN_ALL
xsocket_control_message_t *g_unix_credentials_message_new_with_credentials (xcredentials_t *credentials);
XPL_AVAILABLE_IN_ALL
xcredentials_t          *g_unix_credentials_message_get_credentials      (xunix_credentials_message_t *message);

XPL_AVAILABLE_IN_ALL
xboolean_t               g_unix_credentials_message_is_supported         (void);

G_END_DECLS

#endif /* __G_UNIX_CREDENTIALS_MESSAGE_H__ */
