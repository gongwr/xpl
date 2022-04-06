/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2019 Red Hat, Inc.
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
 */

#include <gio/gio.h>

static void
test_dup_default (void)
{
  xmemory_monitor_t *monitor;

  monitor = xmemory_monitor_dup_default ();
  g_assert_nonnull (monitor);
  xobject_unref (monitor);
}

static void
warning_cb (xmemory_monitor_t *m,
	    GMemoryMonitorWarningLevel level)
{
  char *str = xenum_to_string (XTYPE_MEMORY_MONITOR_WARNING_LEVEL, level);
  g_message ("Warning level: %s (%d)", str , level);
  g_free (str);
}

static void
do_watch_memory (void)
{
  xmemory_monitor_t *m;
  xmain_loop_t *loop;

  m = xmemory_monitor_dup_default ();
  g_signal_connect (G_OBJECT (m), "low-memory-warning",
		    G_CALLBACK (warning_cb), NULL);

  loop = xmain_loop_new (NULL, TRUE);
  xmain_loop_run (loop);
}

int
main (int argc, char **argv)
{
  int ret;

  if (argc == 2 && !strcmp (argv[1], "--watch"))
    {
      do_watch_memory ();
      return 0;
    }

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/memory-monitor/default", test_dup_default);

  ret = g_test_run ();

  return ret;
}
