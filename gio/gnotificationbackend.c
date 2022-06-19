/*
 * Copyright © 2013 Lars Uebernickel
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
 * Authors: Lars Uebernickel <lars@uebernic.de>
 */

#include "gnotificationbackend.h"

#include "gnotification.h"
#include "gapplication.h"
#include "gactiongroup.h"
#include "giomodule-priv.h"

XDEFINE_TYPE (xnotification_backend, xnotification_backend, XTYPE_OBJECT)

static void
xnotification_backend_class_init (xnotification_backend_class_t *class)
{
}

static void
xnotification_backend_init (xnotification_backend_t *backend)
{
}

xnotification_backend_t *
xnotification_backend_new_default (xapplication_t *application)
{
  xtype_t backend_type;
  xnotification_backend_t *backend;

  xreturn_val_if_fail (X_IS_APPLICATION (application), NULL);

  backend_type = _xio_module_get_default_type (G_NOTIFICATION_BACKEND_EXTENSION_POINT_NAME,
                                                "GNOTIFICATION_BACKEND",
                                                G_STRUCT_OFFSET (xnotification_backend_class_t, is_supported));

  backend = xobject_new (backend_type, NULL);

  /* Avoid ref cycle by not taking a ref to the application at all. The
   * backend only lives as long as the application does.
   */
  backend->application = application;

  backend->dbus_connection = xapplication_get_dbus_connection (application);
  if (backend->dbus_connection)
    xobject_ref (backend->dbus_connection);

  return backend;
}

void
xnotification_backend_send_notification (xnotification_backend_t *backend,
                                          const xchar_t          *id,
                                          xnotification_t        *notification)
{
  g_return_if_fail (X_IS_NOTIFICATION_BACKEND (backend));
  g_return_if_fail (X_IS_NOTIFICATION (notification));

  G_NOTIFICATION_BACKEND_GET_CLASS (backend)->send_notification (backend, id, notification);
}

void
xnotification_backend_withdraw_notification (xnotification_backend_t *backend,
                                              const xchar_t          *id)
{
  g_return_if_fail (X_IS_NOTIFICATION_BACKEND (backend));
  g_return_if_fail (id != NULL);

  G_NOTIFICATION_BACKEND_GET_CLASS (backend)->withdraw_notification (backend, id);
}
