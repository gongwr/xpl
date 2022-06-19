/* GLib testing framework examples and tests
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

#include <gio/gio.h>
#include <unistd.h>
#include <string.h>

#include "gdbus-tests.h"

static xdbus_connection_t *the_connection = NULL;

#define MY_TYPE_OBJECT   (my_object_get_type ())
#define MY_OBJECT(o)     (XTYPE_CHECK_INSTANCE_CAST ((o), MY_TYPE_OBJECT, xobject_t))
#define MY_IS_OBJECT(o)  (XTYPE_CHECK_INSTANCE_TYPE ((o), MY_TYPE_OBJECT))

typedef struct {
  xobject_t parent_instance;
} xobject_t;

typedef struct {
  xobject_class_t parent_class;
} xobject_class_t;

xtype_t my_object_get_type (void) G_GNUC_CONST;

XDEFINE_TYPE (xobject_t, my_object, XTYPE_OBJECT)

static void
my_object_init (xobject_t *object)
{
}

static void
my_object_class_init (xobject_class_t *klass)
{
  xerror_t *error;
  error = NULL;
  the_connection = g_bus_get_sync (G_BUS_TYPE_SESSION,
                                   NULL, /* xcancellable_t* */
                                   &error);
  g_assert_no_error (error);
  xassert (X_IS_DBUS_CONNECTION (the_connection));
}

static void
test_bz627724 (void)
{
  xobject_t *object;

  session_bus_up ();
  xassert (the_connection == NULL);
  object = xobject_new (MY_TYPE_OBJECT, NULL);
  xassert (the_connection != NULL);
  xobject_unref (the_connection);
  xobject_unref (object);
  session_bus_down ();
}


int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_dbus_unset ();

  g_test_add_func ("/gdbus/bz627724", test_bz627724);
  return g_test_run();
}
