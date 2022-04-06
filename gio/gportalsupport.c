/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2016 Red Hat, Inc.
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

#include "config.h"

#include "gportalsupport.h"

static xboolean_t use_portal;
static xboolean_t network_available;
static xboolean_t dconf_access;

static void
read_flatpak_info (void)
{
  static xsize_t flatpak_info_read = 0;
  const xchar_t *path = "/.flatpak-info";

  if (!g_once_init_enter (&flatpak_info_read))
    return;

  if (xfile_test (path, XFILE_TEST_EXISTS))
    {
      xkey_file_t *keyfile;

      use_portal = TRUE;
      network_available = FALSE;
      dconf_access = FALSE;

      keyfile = xkey_file_new ();
      if (xkey_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, NULL))
        {
          char **shared = NULL;
          char *dconf_policy = NULL;

          shared = xkey_file_get_string_list (keyfile, "Context", "shared", NULL, NULL);
          if (shared)
            {
              network_available = xstrv_contains ((const char * const *)shared, "network");
              xstrfreev (shared);
            }

          dconf_policy = xkey_file_get_string (keyfile, "Session Bus Policy", "ca.desrt.dconf", NULL);
          if (dconf_policy)
            {
              if (strcmp (dconf_policy, "talk") == 0)
                dconf_access = TRUE;
              g_free (dconf_policy);
            }
        }

      xkey_file_unref (keyfile);
    }
  else
    {
      const char *var;

      var = g_getenv ("GTK_USE_PORTAL");
      if (var && var[0] == '1')
        use_portal = TRUE;
      network_available = TRUE;
      dconf_access = TRUE;
    }

  g_once_init_leave (&flatpak_info_read, 1);
}

xboolean_t
glib_should_use_portal (void)
{
  read_flatpak_info ();
  return use_portal;
}

xboolean_t
glib_network_available_in_sandbox (void)
{
  read_flatpak_info ();
  return network_available;
}

xboolean_t
glib_has_dconf_access_in_sandbox (void)
{
  read_flatpak_info ();
  return dconf_access;
}
