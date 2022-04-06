/* GIO testing utilities
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
 * Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
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
 *          Xavier Claessens <xavier.claessens@collabora.co.uk>
 */

#ifndef __G_TEST_DBUS_H__
#define __G_TEST_DBUS_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_TEST_DBUS \
    (g_test_dbus_get_type ())
#define G_TEST_DBUS(obj) \
    (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_TEST_DBUS, \
        xtest_dbus))
#define X_IS_TEST_DBUS(obj) \
    (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_TEST_DBUS))

XPL_AVAILABLE_IN_2_34
xtype_t          g_test_dbus_get_type        (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_34
xtest_dbus_t *    g_test_dbus_new             (GTestDBusFlags flags);

XPL_AVAILABLE_IN_2_34
GTestDBusFlags g_test_dbus_get_flags       (xtest_dbus_t     *self);

XPL_AVAILABLE_IN_2_34
const xchar_t *  g_test_dbus_get_bus_address (xtest_dbus_t     *self);

XPL_AVAILABLE_IN_2_34
void           g_test_dbus_add_service_dir (xtest_dbus_t     *self,
                                            const xchar_t   *path);

XPL_AVAILABLE_IN_2_34
void           g_test_dbus_up              (xtest_dbus_t     *self);

XPL_AVAILABLE_IN_2_34
void           g_test_dbus_stop            (xtest_dbus_t     *self);

XPL_AVAILABLE_IN_2_34
void           g_test_dbus_down            (xtest_dbus_t     *self);

XPL_AVAILABLE_IN_2_34
void           g_test_dbus_unset           (void);

G_END_DECLS

#endif /* __G_TEST_DBUS_H__ */
