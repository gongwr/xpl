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

#ifndef __G_CREDENTIALS_H__
#define __G_CREDENTIALS_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

#ifdef G_OS_UNIX
/* To get the uid_t type */
#include <unistd.h>
#include <sys/types.h>
#endif

G_BEGIN_DECLS

#define XTYPE_CREDENTIALS         (xcredentials_get_type ())
#define G_CREDENTIALS(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_CREDENTIALS, xcredentials_t))
#define G_CREDENTIALS_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_CREDENTIALS, xcredentials_class_t))
#define G_CREDENTIALS_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_CREDENTIALS, xcredentials_class_t))
#define X_IS_CREDENTIALS(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_CREDENTIALS))
#define X_IS_CREDENTIALS_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_CREDENTIALS))

typedef struct _xcredentials_class   xcredentials_class_t;

XPL_AVAILABLE_IN_ALL
xtype_t            xcredentials_get_type           (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xcredentials_t    *xcredentials_new                (void);

XPL_AVAILABLE_IN_ALL
xchar_t           *xcredentials_to_string          (xcredentials_t    *credentials);

XPL_AVAILABLE_IN_ALL
xpointer_t         xcredentials_get_native         (xcredentials_t    *credentials,
                                                   GCredentialsType native_type);

XPL_AVAILABLE_IN_ALL
void             xcredentials_set_native         (xcredentials_t    *credentials,
                                                   GCredentialsType native_type,
                                                   xpointer_t         native);

XPL_AVAILABLE_IN_ALL
xboolean_t         xcredentials_is_same_user       (xcredentials_t    *credentials,
                                                   xcredentials_t    *other_credentials,
                                                   xerror_t         **error);

#ifdef G_OS_UNIX
XPL_AVAILABLE_IN_2_36
pid_t            xcredentials_get_unix_pid       (xcredentials_t    *credentials,
                                                   xerror_t         **error);
XPL_AVAILABLE_IN_ALL
uid_t            xcredentials_get_unix_user      (xcredentials_t    *credentials,
                                                   xerror_t         **error);
XPL_AVAILABLE_IN_ALL
xboolean_t         xcredentials_set_unix_user      (xcredentials_t    *credentials,
                                                   uid_t           uid,
                                                   xerror_t         **error);
#endif

G_END_DECLS

#endif /* __G_CREDENTIALS_H__ */
