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

#ifndef __XDBUS_AUTH_MECHANISM_ANON_H__
#define __XDBUS_AUTH_MECHANISM_ANON_H__

#if !defined (GIO_COMPILATION)
#error "gdbusauthmechanismanon.h is a private header file."
#endif

#include <gio/giotypes.h>
#include <gio/gdbusauthmechanism.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_AUTH_MECHANISM_ANON         (_xdbus_auth_mechanism_anon_get_type ())
#define XDBUS_AUTH_MECHANISM_ANON(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_AUTH_MECHANISM_ANON, GDBusAuthMechanismAnon))
#define XDBUS_AUTH_MECHANISM_ANON_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DBUS_AUTH_MECHANISM_ANON, GDBusAuthMechanismAnonClass))
#define XDBUS_AUTH_MECHANISM_ANON_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DBUS_AUTH_MECHANISM_ANON, GDBusAuthMechanismAnonClass))
#define X_IS_DBUS_AUTH_MECHANISM_ANON(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_AUTH_MECHANISM_ANON))
#define X_IS_DBUS_AUTH_MECHANISM_ANON_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DBUS_AUTH_MECHANISM_ANON))

typedef struct _GDBusAuthMechanismAnon        GDBusAuthMechanismAnon;
typedef struct _GDBusAuthMechanismAnonClass   GDBusAuthMechanismAnonClass;
typedef struct _GDBusAuthMechanismAnonPrivate GDBusAuthMechanismAnonPrivate;

struct _GDBusAuthMechanismAnonClass
{
  /*< private >*/
  xdbus_auth_mechanism_class_t parent_class;
};

struct _GDBusAuthMechanismAnon
{
  xdbus_auth_mechanism_t parent_instance;
  GDBusAuthMechanismAnonPrivate *priv;
};

xtype_t _xdbus_auth_mechanism_anon_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __XDBUS_AUTH_MECHANISM_ANON_H__ */
