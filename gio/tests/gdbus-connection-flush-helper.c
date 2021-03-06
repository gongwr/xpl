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

int
main (int   argc,
      char *argv[])
{
  xdbus_connection_t *c;
  xerror_t *error;
  xboolean_t ret;

  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION,
                      NULL, /* xcancellable_t* */
                      &error);
  g_assert_no_error (error);

  error = NULL;
  xdbus_connection_emit_signal (c,
                                 NULL, /* const xchar_t *destination_bus_name */
                                 "/org/gtk/GDBus/FlushObject",
                                 "org.gtk.GDBus.FlushInterface",
                                 "SomeSignal",
                                 NULL, /* xvariant_t *parameters */
                                 &error);
  g_assert_no_error (error);

  error = NULL;
  ret = xdbus_connection_flush_sync (c,
                                      NULL, /* xcancellable_t* */
                                      &error);
  g_assert_no_error (error);
  xassert (ret);

  /* and now exit immediately! */
  return 0;
}
