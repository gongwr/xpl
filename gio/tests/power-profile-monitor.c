/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2021 Igalia S.L.
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
  xpower_profile_monitor_t *monitor;

  monitor = xpower_profile_monitor_dup_default ();
  g_assert_nonnull (monitor);
  xobject_unref (monitor);
}

static void
power_saver_enabled_cb (xpower_profile_monitor_t *monitor,
                        xparam_spec_t           *pspec,
                        xpointer_t              user_data)
{
  xboolean_t enabled;

  enabled = xpower_profile_monitor_get_power_saver_enabled (monitor);
  g_debug ("Power Saver %s (%d)", enabled ? "enabled" : "disabled", enabled);
}

static void
do_watch_power_profile (void)
{
  xpower_profile_monitor_t *monitor;
  xmain_loop_t *loop;
  xulong_t signal_id;

  monitor = xpower_profile_monitor_dup_default ();
  signal_id = xsignal_connect (G_OBJECT (monitor), "notify::power-saver-enabled",
		                G_CALLBACK (power_saver_enabled_cb), NULL);

  loop = xmain_loop_new (NULL, TRUE);
  xmain_loop_run (loop);

  xsignal_handler_disconnect (monitor, signal_id);
  xobject_unref (monitor);
  xmain_loop_unref (loop);
}

int
main (int argc, char **argv)
{
  int ret;

  if (argc == 2 && !strcmp (argv[1], "--watch"))
    {
      do_watch_power_profile ();
      return 0;
    }

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/power-profile-monitor/default", test_dup_default);

  ret = g_test_run ();

  return ret;
}
