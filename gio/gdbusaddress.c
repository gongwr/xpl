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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "gioerror.h"
#include "gdbusutils.h"
#include "gdbusaddress.h"
#include "gdbuserror.h"
#include "gioenumtypes.h"
#include "glib-private.h"
#include "gnetworkaddress.h"
#include "gsocketclient.h"
#include "giostream.h"
#include "gasyncresult.h"
#include "gtask.h"
#include "glib-private.h"
#include "gdbusprivate.h"
#include "gstdio.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <gio/gunixsocketaddress.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include "glibintl.h"

/**
 * SECTION:gdbusaddress
 * @title: D-Bus Addresses
 * @short_description: D-Bus connection endpoints
 * @include: gio/gio.h
 *
 * Routines for working with D-Bus addresses. A D-Bus address is a string
 * like `unix:tmpdir=/tmp/my-app-name`. The exact format of addresses
 * is explained in detail in the
 * [D-Bus specification](http://dbus.freedesktop.org/doc/dbus-specification.html#addresses).
 *
 * TCP D-Bus connections are supported, but accessing them via a proxy is
 * currently not supported.
 *
 * Since GLib 2.72, `unix:` addresses are supported on Windows with `AF_UNIX`
 * support (Windows 10).
 */

static xchar_t *get_session_address_platform_specific (xerror_t **error);
static xchar_t *get_session_address_dbus_launch       (xerror_t **error);

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_is_address:
 * @string: A string.
 *
 * Checks if @string is a
 * [D-Bus address](https://dbus.freedesktop.org/doc/dbus-specification.html#addresses).
 *
 * This doesn't check if @string is actually supported by #xdbus_server_t
 * or #xdbus_connection_t - use g_dbus_is_supported_address() to do more
 * checks.
 *
 * Returns: %TRUE if @string is a valid D-Bus address, %FALSE otherwise.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_is_address (const xchar_t *string)
{
  xuint_t n;
  xchar_t **a;
  xboolean_t ret;

  ret = FALSE;

  g_return_val_if_fail (string != NULL, FALSE);

  a = xstrsplit (string, ";", 0);
  if (a[0] == NULL)
    goto out;

  for (n = 0; a[n] != NULL; n++)
    {
      if (!_g_dbus_address_parse_entry (a[n],
                                        NULL,
                                        NULL,
                                        NULL))
        goto out;
    }

  ret = TRUE;

 out:
  xstrfreev (a);
  return ret;
}

static xboolean_t
is_valid_unix (const xchar_t  *address_entry,
               xhashtable_t   *key_value_pairs,
               xerror_t      **error)
{
  xboolean_t ret;
  xlist_t *keys;
  xlist_t *l;
  const xchar_t *path;
  const xchar_t *dir;
  const xchar_t *tmpdir;
  const xchar_t *abstract;

  ret = FALSE;
  keys = NULL;
  path = NULL;
  dir = NULL;
  tmpdir = NULL;
  abstract = NULL;

  keys = xhash_table_get_keys (key_value_pairs);
  for (l = keys; l != NULL; l = l->next)
    {
      const xchar_t *key = l->data;
      if (xstrcmp0 (key, "path") == 0)
        path = xhash_table_lookup (key_value_pairs, key);
      else if (xstrcmp0 (key, "dir") == 0)
        dir = xhash_table_lookup (key_value_pairs, key);
      else if (xstrcmp0 (key, "tmpdir") == 0)
        tmpdir = xhash_table_lookup (key_value_pairs, key);
      else if (xstrcmp0 (key, "abstract") == 0)
        abstract = xhash_table_lookup (key_value_pairs, key);
      else if (xstrcmp0 (key, "guid") != 0)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Unsupported key “%s” in address entry “%s”"),
                       key,
                       address_entry);
          goto out;
        }
    }

  /* Exactly one key must be set */
  if ((path != NULL) + (dir != NULL) + (tmpdir != NULL) + (abstract != NULL) > 1)
    {
      g_set_error (error,
             G_IO_ERROR,
             G_IO_ERROR_INVALID_ARGUMENT,
             _("Meaningless key/value pair combination in address entry “%s”"),
             address_entry);
      goto out;
    }
  else if (path == NULL && dir == NULL && tmpdir == NULL && abstract == NULL)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Address “%s” is invalid (need exactly one of path, dir, tmpdir, or abstract keys)"),
                   address_entry);
      goto out;
    }

  ret = TRUE;

 out:
  xlist_free (keys);

  return ret;
}

static xboolean_t
is_valid_nonce_tcp (const xchar_t  *address_entry,
                    xhashtable_t   *key_value_pairs,
                    xerror_t      **error)
{
  xboolean_t ret;
  xlist_t *keys;
  xlist_t *l;
  const xchar_t *host;
  const xchar_t *port;
  const xchar_t *family;
  const xchar_t *nonce_file;
  xint_t port_num;
  xchar_t *endp;

  ret = FALSE;
  keys = NULL;
  host = NULL;
  port = NULL;
  family = NULL;
  nonce_file = NULL;

  keys = xhash_table_get_keys (key_value_pairs);
  for (l = keys; l != NULL; l = l->next)
    {
      const xchar_t *key = l->data;
      if (xstrcmp0 (key, "host") == 0)
        host = xhash_table_lookup (key_value_pairs, key);
      else if (xstrcmp0 (key, "port") == 0)
        port = xhash_table_lookup (key_value_pairs, key);
      else if (xstrcmp0 (key, "family") == 0)
        family = xhash_table_lookup (key_value_pairs, key);
      else if (xstrcmp0 (key, "noncefile") == 0)
        nonce_file = xhash_table_lookup (key_value_pairs, key);
      else if (xstrcmp0 (key, "guid") != 0)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Unsupported key “%s” in address entry “%s”"),
                       key,
                       address_entry);
          goto out;
        }
    }

  if (port != NULL)
    {
      port_num = strtol (port, &endp, 10);
      if ((*port == '\0' || *endp != '\0') || port_num < 0 || port_num >= 65536)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Error in address “%s” — the “%s” attribute is malformed"),
                       address_entry, "port");
          goto out;
        }
    }

  if (family != NULL && !(xstrcmp0 (family, "ipv4") == 0 || xstrcmp0 (family, "ipv6") == 0))
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Error in address “%s” — the “%s” attribute is malformed"),
                   address_entry, "family");
      goto out;
    }

  if (host != NULL)
    {
      /* TODO: validate host */
    }

  if (nonce_file != NULL && *nonce_file == '\0')
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Error in address “%s” — the “%s” attribute is malformed"),
                   address_entry, "noncefile");
      goto out;
    }

  ret = TRUE;

 out:
  xlist_free (keys);

  return ret;
}

static xboolean_t
is_valid_tcp (const xchar_t  *address_entry,
              xhashtable_t   *key_value_pairs,
              xerror_t      **error)
{
  xboolean_t ret;
  xlist_t *keys;
  xlist_t *l;
  const xchar_t *host;
  const xchar_t *port;
  const xchar_t *family;
  xint_t port_num;
  xchar_t *endp;

  ret = FALSE;
  keys = NULL;
  host = NULL;
  port = NULL;
  family = NULL;

  keys = xhash_table_get_keys (key_value_pairs);
  for (l = keys; l != NULL; l = l->next)
    {
      const xchar_t *key = l->data;
      if (xstrcmp0 (key, "host") == 0)
        host = xhash_table_lookup (key_value_pairs, key);
      else if (xstrcmp0 (key, "port") == 0)
        port = xhash_table_lookup (key_value_pairs, key);
      else if (xstrcmp0 (key, "family") == 0)
        family = xhash_table_lookup (key_value_pairs, key);
      else if (xstrcmp0 (key, "guid") != 0)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Unsupported key “%s” in address entry “%s”"),
                       key,
                       address_entry);
          goto out;
        }
    }

  if (port != NULL)
    {
      port_num = strtol (port, &endp, 10);
      if ((*port == '\0' || *endp != '\0') || port_num < 0 || port_num >= 65536)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Error in address “%s” — the “%s” attribute is malformed"),
                       address_entry, "port");
          goto out;
        }
    }

  if (family != NULL && !(xstrcmp0 (family, "ipv4") == 0 || xstrcmp0 (family, "ipv6") == 0))
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Error in address “%s” — the “%s” attribute is malformed"),
                   address_entry, "family");
      goto out;
    }

  if (host != NULL)
    {
      /* TODO: validate host */
    }

  ret= TRUE;

 out:
  xlist_free (keys);

  return ret;
}

/**
 * g_dbus_is_supported_address:
 * @string: A string.
 * @error: Return location for error or %NULL.
 *
 * Like g_dbus_is_address() but also checks if the library supports the
 * transports in @string and that key/value pairs for each transport
 * are valid. See the specification of the
 * [D-Bus address format](https://dbus.freedesktop.org/doc/dbus-specification.html#addresses).
 *
 * Returns: %TRUE if @string is a valid D-Bus address that is
 * supported by this library, %FALSE if @error is set.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_is_supported_address (const xchar_t  *string,
                             xerror_t      **error)
{
  xuint_t n;
  xchar_t **a;
  xboolean_t ret;

  ret = FALSE;

  g_return_val_if_fail (string != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  a = xstrsplit (string, ";", 0);
  for (n = 0; a[n] != NULL; n++)
    {
      xchar_t *transport_name;
      xhashtable_t *key_value_pairs;
      xboolean_t supported;

      if (!_g_dbus_address_parse_entry (a[n],
                                        &transport_name,
                                        &key_value_pairs,
                                        error))
        goto out;

      supported = FALSE;
      if (xstrcmp0 (transport_name, "unix") == 0)
        supported = is_valid_unix (a[n], key_value_pairs, error);
      else if (xstrcmp0 (transport_name, "tcp") == 0)
        supported = is_valid_tcp (a[n], key_value_pairs, error);
      else if (xstrcmp0 (transport_name, "nonce-tcp") == 0)
        supported = is_valid_nonce_tcp (a[n], key_value_pairs, error);
      else if (xstrcmp0 (a[n], "autolaunch:") == 0)
        supported = TRUE;
      else
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                     _("Unknown or unsupported transport “%s” for address “%s”"),
                     transport_name, a[n]);

      g_free (transport_name);
      xhash_table_unref (key_value_pairs);

      if (!supported)
        goto out;
    }

  ret = TRUE;

 out:
  xstrfreev (a);

  g_assert (ret || (!ret && (error == NULL || *error != NULL)));

  return ret;
}

xboolean_t
_g_dbus_address_parse_entry (const xchar_t  *address_entry,
                             xchar_t       **out_transport_name,
                             xhashtable_t  **out_key_value_pairs,
                             xerror_t      **error)
{
  xboolean_t ret;
  xhashtable_t *key_value_pairs;
  xchar_t *transport_name;
  xchar_t **kv_pairs;
  const xchar_t *s;
  xuint_t n;

  ret = FALSE;
  kv_pairs = NULL;
  transport_name = NULL;
  key_value_pairs = NULL;

  s = strchr (address_entry, ':');
  if (s == NULL)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Address element “%s” does not contain a colon (:)"),
                   address_entry);
      goto out;
    }
  else if (s == address_entry)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Transport name in address element “%s” must not be empty"),
                   address_entry);
      goto out;
    }

  transport_name = xstrndup (address_entry, s - address_entry);
  key_value_pairs = xhash_table_new_full (xstr_hash, xstr_equal, g_free, g_free);

  kv_pairs = xstrsplit (s + 1, ",", 0);
  for (n = 0; kv_pairs[n] != NULL; n++)
    {
      const xchar_t *kv_pair = kv_pairs[n];
      xchar_t *key;
      xchar_t *value;

      s = strchr (kv_pair, '=');
      if (s == NULL)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Key/Value pair %d, “%s”, in address element “%s” does not contain an equal sign"),
                       n,
                       kv_pair,
                       address_entry);
          goto out;
        }
      else if (s == kv_pair)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Key/Value pair %d, “%s”, in address element “%s” must not have an empty key"),
                       n,
                       kv_pair,
                       address_entry);
          goto out;
        }

      key = xuri_unescape_segment (kv_pair, s, NULL);
      value = xuri_unescape_segment (s + 1, kv_pair + strlen (kv_pair), NULL);
      if (key == NULL || value == NULL)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Error unescaping key or value in Key/Value pair %d, “%s”, in address element “%s”"),
                       n,
                       kv_pair,
                       address_entry);
          g_free (key);
          g_free (value);
          goto out;
        }
      xhash_table_insert (key_value_pairs, key, value);
    }

  ret = TRUE;

out:
  if (ret)
    {
      if (out_transport_name != NULL)
        *out_transport_name = g_steal_pointer (&transport_name);
      if (out_key_value_pairs != NULL)
        *out_key_value_pairs = g_steal_pointer (&key_value_pairs);
    }

  g_clear_pointer (&key_value_pairs, xhash_table_unref);
  g_free (transport_name);
  xstrfreev (kv_pairs);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static xio_stream_t *
g_dbus_address_try_connect_one (const xchar_t   *address_entry,
                                xchar_t        **out_guid,
                                xcancellable_t  *cancellable,
                                xerror_t       **error);

/* TODO: Declare an extension point called GDBusTransport (or similar)
 * and move code below to extensions implementing said extension
 * point. That way we can implement a D-Bus transport over X11 without
 * making libgio link to libX11...
 */
static xio_stream_t *
g_dbus_address_connect (const xchar_t   *address_entry,
                        const xchar_t   *transport_name,
                        xhashtable_t    *key_value_pairs,
                        xcancellable_t  *cancellable,
                        xerror_t       **error)
{
  xio_stream_t *ret;
  xsocket_connectable_t *connectable;
  const xchar_t *nonce_file;

  connectable = NULL;
  ret = NULL;
  nonce_file = NULL;

  if (xstrcmp0 (transport_name, "unix") == 0)
    {
      const xchar_t *path;
      const xchar_t *abstract;
      path = xhash_table_lookup (key_value_pairs, "path");
      abstract = xhash_table_lookup (key_value_pairs, "abstract");
      if ((path == NULL && abstract == NULL) || (path != NULL && abstract != NULL))
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Error in address “%s” — the unix transport requires exactly one of the "
                         "keys “path” or “abstract” to be set"),
                       address_entry);
        }
      else if (path != NULL)
        {
          connectable = XSOCKET_CONNECTABLE (g_unix_socket_address_new (path));
        }
      else if (abstract != NULL)
        {
          connectable = XSOCKET_CONNECTABLE (g_unix_socket_address_new_with_type (abstract,
                                                                                   -1,
                                                                                   G_UNIX_SOCKET_ADDRESS_ABSTRACT));
        }
      else
        {
          g_assert_not_reached ();
        }
    }
  else if (xstrcmp0 (transport_name, "tcp") == 0 || xstrcmp0 (transport_name, "nonce-tcp") == 0)
    {
      const xchar_t *s;
      const xchar_t *host;
      xlong_t port;
      xchar_t *endp;
      xboolean_t is_nonce;

      is_nonce = (xstrcmp0 (transport_name, "nonce-tcp") == 0);

      host = xhash_table_lookup (key_value_pairs, "host");
      if (host == NULL)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Error in address “%s” — the host attribute is missing or malformed"),
                       address_entry);
          goto out;
        }

      s = xhash_table_lookup (key_value_pairs, "port");
      if (s == NULL)
        s = "0";
      port = strtol (s, &endp, 10);
      if ((*s == '\0' || *endp != '\0') || port < 0 || port >= 65536)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Error in address “%s” — the port attribute is missing or malformed"),
                       address_entry);
          goto out;
        }


      if (is_nonce)
        {
          nonce_file = xhash_table_lookup (key_value_pairs, "noncefile");
          if (nonce_file == NULL)
            {
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Error in address “%s” — the noncefile attribute is missing or malformed"),
                           address_entry);
              goto out;
            }
        }

      /* TODO: deal with family key/value-pair */
      connectable = g_network_address_new (host, port);
    }
  else if (xstrcmp0 (address_entry, "autolaunch:") == 0)
    {
      xchar_t *autolaunch_address;
      autolaunch_address = get_session_address_dbus_launch (error);
      if (autolaunch_address != NULL)
        {
          ret = g_dbus_address_try_connect_one (autolaunch_address, NULL, cancellable, error);
          g_free (autolaunch_address);
          goto out;
        }
      else
        {
          g_prefix_error (error, _("Error auto-launching: "));
        }
    }
  else
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Unknown or unsupported transport “%s” for address “%s”"),
                   transport_name,
                   address_entry);
    }

  if (connectable != NULL)
    {
      xsocket_client_t *client;
      xsocket_connection_t *connection;

      g_assert (ret == NULL);
      client = xsocket_client_new ();

      /* Disable proxy support to prevent a deadlock on startup, since loading a
       * proxy resolver causes the GIO modules to be loaded, and there will
       * almost certainly be one of them which then tries to use GDBus.
       * See: https://bugzilla.gnome.org/show_bug.cgi?id=792499 */
      xsocket_client_set_enable_proxy (client, FALSE);

      connection = xsocket_client_connect (client,
                                            connectable,
                                            cancellable,
                                            error);
      xobject_unref (connectable);
      xobject_unref (client);
      if (connection == NULL)
        goto out;

      ret = XIO_STREAM (connection);

      if (nonce_file != NULL)
        {
          xchar_t nonce_contents[16 + 1];
          size_t num_bytes_read;
          FILE *f;
          int errsv;

          /* be careful to read only 16 bytes - we also check that the file is only 16 bytes long */
          f = fopen (nonce_file, "rb");
          errsv = errno;
          if (f == NULL)
            {
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Error opening nonce file “%s”: %s"),
                           nonce_file,
                           xstrerror (errsv));
              xobject_unref (ret);
              ret = NULL;
              goto out;
            }
          num_bytes_read = fread (nonce_contents,
                                  sizeof (xchar_t),
                                  16 + 1,
                                  f);
          errsv = errno;
          if (num_bytes_read != 16)
            {
              if (num_bytes_read == 0)
                {
                  g_set_error (error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("Error reading from nonce file “%s”: %s"),
                               nonce_file,
                               xstrerror (errsv));
                }
              else
                {
                  g_set_error (error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("Error reading from nonce file “%s”, expected 16 bytes, got %d"),
                               nonce_file,
                               (xint_t) num_bytes_read);
                }
              xobject_unref (ret);
              ret = NULL;
              fclose (f);
              goto out;
            }
          fclose (f);

          if (!xoutput_stream_write_all (g_io_stream_get_output_stream (ret),
                                          nonce_contents,
                                          16,
                                          NULL,
                                          cancellable,
                                          error))
            {
              g_prefix_error (error, _("Error writing contents of nonce file “%s” to stream:"), nonce_file);
              xobject_unref (ret);
              ret = NULL;
              goto out;
            }
        }
    }

 out:

  return ret;
}

static xio_stream_t *
g_dbus_address_try_connect_one (const xchar_t   *address_entry,
                                xchar_t        **out_guid,
                                xcancellable_t  *cancellable,
                                xerror_t       **error)
{
  xio_stream_t *ret;
  xhashtable_t *key_value_pairs;
  xchar_t *transport_name;
  const xchar_t *guid;

  ret = NULL;
  transport_name = NULL;
  key_value_pairs = NULL;

  if (!_g_dbus_address_parse_entry (address_entry,
                                    &transport_name,
                                    &key_value_pairs,
                                    error))
    goto out;

  ret = g_dbus_address_connect (address_entry,
                                transport_name,
                                key_value_pairs,
                                cancellable,
                                error);
  if (ret == NULL)
    goto out;

  guid = xhash_table_lookup (key_value_pairs, "guid");
  if (guid != NULL && out_guid != NULL)
    *out_guid = xstrdup (guid);

out:
  g_free (transport_name);
  if (key_value_pairs != NULL)
    xhash_table_unref (key_value_pairs);
  return ret;
}


/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
  xchar_t *address;
  xchar_t *guid;
} GetStreamData;

static void
get_stream_data_free (GetStreamData *data)
{
  g_free (data->address);
  g_free (data->guid);
  g_free (data);
}

static void
get_stream_thread_func (xtask_t         *task,
                        xpointer_t       source_object,
                        xpointer_t       task_data,
                        xcancellable_t  *cancellable)
{
  GetStreamData *data = task_data;
  xio_stream_t *stream;
  xerror_t *error = NULL;

  stream = g_dbus_address_get_stream_sync (data->address,
                                           &data->guid,
                                           cancellable,
                                           &error);
  if (stream)
    xtask_return_pointer (task, stream, xobject_unref);
  else
    xtask_return_error (task, error);
}

/**
 * g_dbus_address_get_stream:
 * @address: A valid D-Bus address.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @callback: A #xasync_ready_callback_t to call when the request is satisfied.
 * @user_data: Data to pass to @callback.
 *
 * Asynchronously connects to an endpoint specified by @address and
 * sets up the connection so it is in a state to run the client-side
 * of the D-Bus authentication conversation. @address must be in the
 * [D-Bus address format](https://dbus.freedesktop.org/doc/dbus-specification.html#addresses).
 *
 * When the operation is finished, @callback will be invoked. You can
 * then call g_dbus_address_get_stream_finish() to get the result of
 * the operation.
 *
 * This is an asynchronous failable function. See
 * g_dbus_address_get_stream_sync() for the synchronous version.
 *
 * Since: 2.26
 */
void
g_dbus_address_get_stream (const xchar_t         *address,
                           xcancellable_t        *cancellable,
                           xasync_ready_callback_t  callback,
                           xpointer_t             user_data)
{
  xtask_t *task;
  GetStreamData *data;

  g_return_if_fail (address != NULL);

  data = g_new0 (GetStreamData, 1);
  data->address = xstrdup (address);

  task = xtask_new (NULL, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_dbus_address_get_stream);
  xtask_set_task_data (task, data, (xdestroy_notify_t) get_stream_data_free);
  xtask_run_in_thread (task, get_stream_thread_func);
  xobject_unref (task);
}

/**
 * g_dbus_address_get_stream_finish:
 * @res: A #xasync_result_t obtained from the xasync_ready_callback_t passed to g_dbus_address_get_stream().
 * @out_guid: (optional) (out) (nullable): %NULL or return location to store the GUID extracted from @address, if any.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with g_dbus_address_get_stream().
 *
 * A server is not required to set a GUID, so @out_guid may be set to %NULL
 * even on success.
 *
 * Returns: (transfer full): A #xio_stream_t or %NULL if @error is set.
 *
 * Since: 2.26
 */
xio_stream_t *
g_dbus_address_get_stream_finish (xasync_result_t        *res,
                                  xchar_t              **out_guid,
                                  xerror_t             **error)
{
  xtask_t *task;
  GetStreamData *data;
  xio_stream_t *ret;

  g_return_val_if_fail (xtask_is_valid (res, NULL), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  task = XTASK (res);
  ret = xtask_propagate_pointer (task, error);

  if (ret != NULL && out_guid != NULL)
    {
      data = xtask_get_task_data (task);
      *out_guid = data->guid;
      data->guid = NULL;
    }

  return ret;
}

/**
 * g_dbus_address_get_stream_sync:
 * @address: A valid D-Bus address.
 * @out_guid: (optional) (out) (nullable): %NULL or return location to store the GUID extracted from @address, if any.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Synchronously connects to an endpoint specified by @address and
 * sets up the connection so it is in a state to run the client-side
 * of the D-Bus authentication conversation. @address must be in the
 * [D-Bus address format](https://dbus.freedesktop.org/doc/dbus-specification.html#addresses).
 *
 * A server is not required to set a GUID, so @out_guid may be set to %NULL
 * even on success.
 *
 * This is a synchronous failable function. See
 * g_dbus_address_get_stream() for the asynchronous version.
 *
 * Returns: (transfer full): A #xio_stream_t or %NULL if @error is set.
 *
 * Since: 2.26
 */
xio_stream_t *
g_dbus_address_get_stream_sync (const xchar_t   *address,
                                xchar_t        **out_guid,
                                xcancellable_t  *cancellable,
                                xerror_t       **error)
{
  xio_stream_t *ret;
  xchar_t **addr_array;
  xuint_t n;
  xerror_t *last_error;

  g_return_val_if_fail (address != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  ret = NULL;
  last_error = NULL;

  addr_array = xstrsplit (address, ";", 0);
  if (addr_array[0] == NULL)
    {
      last_error = xerror_new_literal (G_IO_ERROR,
                                        G_IO_ERROR_INVALID_ARGUMENT,
                                        _("The given address is empty"));
      goto out;
    }

  for (n = 0; addr_array[n] != NULL; n++)
    {
      const xchar_t *addr = addr_array[n];
      xerror_t *this_error;

      this_error = NULL;
      ret = g_dbus_address_try_connect_one (addr,
                                            out_guid,
                                            cancellable,
                                            &this_error);
      if (ret != NULL)
        {
          goto out;
        }
      else
        {
          g_assert (this_error != NULL);
          if (last_error != NULL)
            xerror_free (last_error);
          last_error = this_error;
        }
    }

 out:
  if (ret != NULL)
    {
      if (last_error != NULL)
        xerror_free (last_error);
    }
  else
    {
      g_assert (last_error != NULL);
      g_propagate_error (error, last_error);
    }

  xstrfreev (addr_array);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/*
 * Return the address of XDG_RUNTIME_DIR/bus if it exists, belongs to
 * us, and is a socket, and we are on Unix.
 */
static xchar_t *
get_session_address_xdg (void)
{
#ifdef G_OS_UNIX
  xchar_t *ret = NULL;
  xchar_t *bus;
  xchar_t *tmp;
  GStatBuf buf;

  bus = g_build_filename (g_get_user_runtime_dir (), "bus", NULL);

  /* if ENOENT, EPERM, etc., quietly don't use it */
  if (g_stat (bus, &buf) < 0)
    goto out;

  /* if it isn't ours, we have incorrectly inherited someone else's
   * XDG_RUNTIME_DIR; silently don't use it
   */
  if (buf.st_uid != geteuid ())
    goto out;

  /* if it isn't a socket, silently don't use it */
  if ((buf.st_mode & S_IFMT) != S_IFSOCK)
    goto out;

  tmp = g_dbus_address_escape_value (bus);
  ret = xstrconcat ("unix:path=", tmp, NULL);
  g_free (tmp);

out:
  g_free (bus);
  return ret;
#else
  return NULL;
#endif
}

/* ---------------------------------------------------------------------------------------------------- */

#ifdef G_OS_UNIX
static xchar_t *
get_session_address_dbus_launch (xerror_t **error)
{
  xchar_t *ret;
  xchar_t *machine_id;
  xchar_t *command_line;
  xchar_t *launch_stdout;
  xchar_t *launch_stderr;
  xint_t wait_status;
  xchar_t *old_dbus_verbose;
  xboolean_t restore_dbus_verbose;

  ret = NULL;
  machine_id = NULL;
  command_line = NULL;
  launch_stdout = NULL;
  launch_stderr = NULL;
  restore_dbus_verbose = FALSE;
  old_dbus_verbose = NULL;

  /* Don't run binaries as root if we're setuid. */
  if (XPL_PRIVATE_CALL (g_check_setuid) ())
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   _("Cannot spawn a message bus when AT_SECURE is set"));
      goto out;
    }

  machine_id = _g_dbus_get_machine_id (error);
  if (machine_id == NULL)
    {
      g_prefix_error (error, _("Cannot spawn a message bus without a machine-id: "));
      goto out;
    }

  if (g_getenv ("DISPLAY") == NULL)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   _("Cannot autolaunch D-Bus without X11 $DISPLAY"));
      goto out;
    }

  /* We're using private libdbus facilities here. When everything
   * (X11, Mac OS X, Windows) is spec'ed out correctly (not even the
   * X11 property is correctly documented right now) we should
   * consider using the spec instead of dbus-launch.
   *
   *   --autolaunch=MACHINEID
   *          This option implies that dbus-launch should scan  for  a  previ‐
   *          ously-started  session  and  reuse the values found there. If no
   *          session is found, it will start a new session. The  --exit-with-
   *          session option is implied if --autolaunch is given.  This option
   *          is for the exclusive use of libdbus, you do not want to  use  it
   *          manually. It may change in the future.
   */

  /* TODO: maybe provide a variable for where to look for the dbus-launch binary? */
  command_line = xstrdup_printf ("dbus-launch --autolaunch=%s --binary-syntax --close-stderr", machine_id);

  if (G_UNLIKELY (_g_dbus_debug_address ()))
    {
      _g_dbus_debug_print_lock ();
      g_print ("GDBus-debug:Address: Running '%s' to get bus address (possibly autolaunching)\n", command_line);
      old_dbus_verbose = xstrdup (g_getenv ("DBUS_VERBOSE"));
      restore_dbus_verbose = TRUE;
      g_setenv ("DBUS_VERBOSE", "1", TRUE);
      _g_dbus_debug_print_unlock ();
    }

  if (!g_spawn_command_line_sync (command_line,
                                  &launch_stdout,
                                  &launch_stderr,
                                  &wait_status,
                                  error))
    {
      goto out;
    }

  if (!g_spawn_check_wait_status (wait_status, error))
    {
      g_prefix_error (error, _("Error spawning command line “%s”: "), command_line);
      goto out;
    }

  /* From the dbus-launch(1) man page:
   *
   *   --binary-syntax Write to stdout a nul-terminated bus address,
   *   then the bus PID as a binary integer of size sizeof(pid_t),
   *   then the bus X window ID as a binary integer of size
   *   sizeof(long).  Integers are in the machine's byte order, not
   *   network byte order or any other canonical byte order.
   */
  ret = xstrdup (launch_stdout);

 out:
  if (G_UNLIKELY (_g_dbus_debug_address ()))
    {
      xchar_t *s;
      _g_dbus_debug_print_lock ();
      g_print ("GDBus-debug:Address: dbus-launch output:");
      if (launch_stdout != NULL)
        {
          s = _g_dbus_hexdump (launch_stdout, strlen (launch_stdout) + 1 + sizeof (pid_t) + sizeof (long), 2);
          g_print ("\n%s", s);
          g_free (s);
        }
      else
        {
          g_print (" (none)\n");
        }
      g_print ("GDBus-debug:Address: dbus-launch stderr output:");
      if (launch_stderr != NULL)
        g_print ("\n%s", launch_stderr);
      else
        g_print (" (none)\n");
      _g_dbus_debug_print_unlock ();
    }

  g_free (machine_id);
  g_free (command_line);
  g_free (launch_stdout);
  g_free (launch_stderr);
  if (G_UNLIKELY (restore_dbus_verbose))
    {
      if (old_dbus_verbose != NULL)
        g_setenv ("DBUS_VERBOSE", old_dbus_verbose, TRUE);
      else
        g_unsetenv ("DBUS_VERBOSE");
    }
  g_free (old_dbus_verbose);
  return ret;
}

/* end of G_OS_UNIX case */
#elif defined(G_OS_WIN32)

static xchar_t *
get_session_address_dbus_launch (xerror_t **error)
{
  return _g_dbus_win32_get_session_address_dbus_launch (error);
}

#else /* neither G_OS_UNIX nor G_OS_WIN32 */
static xchar_t *
get_session_address_dbus_launch (xerror_t **error)
{
  g_set_error (error,
               G_IO_ERROR,
               G_IO_ERROR_FAILED,
               _("Cannot determine session bus address (not implemented for this OS)"));
  return NULL;
}
#endif /* neither G_OS_UNIX nor G_OS_WIN32 */

/* ---------------------------------------------------------------------------------------------------- */

static xchar_t *
get_session_address_platform_specific (xerror_t **error)
{
  xchar_t *ret;

  /* Use XDG_RUNTIME_DIR/bus if it exists and is suitable. This is appropriate
   * for systems using the "a session is a user-session" model described in
   * <http://lists.freedesktop.org/archives/dbus/2015-January/016522.html>,
   * and implemented in dbus >= 1.9.14 and sd-bus.
   *
   * On systems following the more traditional "a session is a login-session"
   * model, this will fail and we'll fall through to X11 autolaunching
   * (dbus-launch) below.
   */
  ret = get_session_address_xdg ();

  if (ret != NULL)
    return ret;

  /* TODO (#694472): try launchd on OS X, like
   * _dbus_lookup_session_address_launchd() does, since
   * 'dbus-launch --autolaunch' probably won't work there
   */

  /* As a last resort, try the "autolaunch:" transport. On Unix this means
   * X11 autolaunching; on Windows this means a different autolaunching
   * mechanism based on shared memory.
   */
  return get_session_address_dbus_launch (error);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_address_get_for_bus_sync:
 * @bus_type: a #GBusType
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @error: return location for error or %NULL
 *
 * Synchronously looks up the D-Bus address for the well-known message
 * bus instance specified by @bus_type. This may involve using various
 * platform specific mechanisms.
 *
 * The returned address will be in the
 * [D-Bus address format](https://dbus.freedesktop.org/doc/dbus-specification.html#addresses).
 *
 * Returns: (transfer full): a valid D-Bus address string for @bus_type or
 *     %NULL if @error is set
 *
 * Since: 2.26
 */
xchar_t *
g_dbus_address_get_for_bus_sync (GBusType       bus_type,
                                 xcancellable_t  *cancellable,
                                 xerror_t       **error)
{
  xboolean_t has_elevated_privileges = XPL_PRIVATE_CALL (g_check_setuid) ();
  xchar_t *ret, *s = NULL;
  const xchar_t *starter_bus;
  xerror_t *local_error;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  ret = NULL;
  local_error = NULL;

  if (G_UNLIKELY (_g_dbus_debug_address ()))
    {
      xuint_t n;
      _g_dbus_debug_print_lock ();
      s = _g_dbus_enum_to_string (XTYPE_BUS_TYPE, bus_type);
      g_print ("GDBus-debug:Address: In g_dbus_address_get_for_bus_sync() for bus type '%s'\n",
               s);
      g_free (s);
      for (n = 0; n < 3; n++)
        {
          const xchar_t *k;
          const xchar_t *v;
          switch (n)
            {
            case 0: k = "DBUS_SESSION_BUS_ADDRESS"; break;
            case 1: k = "DBUS_SYSTEM_BUS_ADDRESS"; break;
            case 2: k = "DBUS_STARTER_BUS_TYPE"; break;
            default: g_assert_not_reached ();
            }
          v = g_getenv (k);
          g_print ("GDBus-debug:Address: env var %s", k);
          if (v != NULL)
            g_print ("='%s'\n", v);
          else
            g_print (" is not set\n");
        }
      _g_dbus_debug_print_unlock ();
    }

  /* Don’t load the addresses from the environment if running as setuid, as they
   * come from an unprivileged caller. */
  switch (bus_type)
    {
    case G_BUS_TYPE_SYSTEM:
      if (has_elevated_privileges)
        ret = NULL;
      else
        ret = xstrdup (g_getenv ("DBUS_SYSTEM_BUS_ADDRESS"));

      if (ret == NULL)
        {
          ret = xstrdup ("unix:path=/var/run/dbus/system_bus_socket");
        }
      break;

    case G_BUS_TYPE_SESSION:
      if (has_elevated_privileges)
        ret = NULL;
      else
        ret = xstrdup (g_getenv ("DBUS_SESSION_BUS_ADDRESS"));

      if (ret == NULL)
        {
          ret = get_session_address_platform_specific (&local_error);
        }
      break;

    case G_BUS_TYPE_STARTER:
      starter_bus = g_getenv ("DBUS_STARTER_BUS_TYPE");
      if (xstrcmp0 (starter_bus, "session") == 0)
        {
          ret = g_dbus_address_get_for_bus_sync (G_BUS_TYPE_SESSION, cancellable, &local_error);
          goto out;
        }
      else if (xstrcmp0 (starter_bus, "system") == 0)
        {
          ret = g_dbus_address_get_for_bus_sync (G_BUS_TYPE_SYSTEM, cancellable, &local_error);
          goto out;
        }
      else
        {
          if (starter_bus != NULL)
            {
              g_set_error (&local_error,
                           G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           _("Cannot determine bus address from DBUS_STARTER_BUS_TYPE environment variable"
                             " — unknown value “%s”"),
                           starter_bus);
            }
          else
            {
              g_set_error_literal (&local_error,
                                   G_IO_ERROR,
                                   G_IO_ERROR_FAILED,
                                   _("Cannot determine bus address because the DBUS_STARTER_BUS_TYPE environment "
                                     "variable is not set"));
            }
        }
      break;

    default:
      g_set_error (&local_error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   _("Unknown bus type %d"),
                   bus_type);
      break;
    }

 out:
  if (G_UNLIKELY (_g_dbus_debug_address ()))
    {
      _g_dbus_debug_print_lock ();
      s = _g_dbus_enum_to_string (XTYPE_BUS_TYPE, bus_type);
      if (ret != NULL)
        {
          g_print ("GDBus-debug:Address: Returning address '%s' for bus type '%s'\n",
                   ret, s);
        }
      else
        {
          g_print ("GDBus-debug:Address: Cannot look-up address bus type '%s': %s\n",
                   s, local_error ? local_error->message : "");
        }
      g_free (s);
      _g_dbus_debug_print_unlock ();
    }

  if (local_error != NULL)
    g_propagate_error (error, local_error);

  return ret;
}

/**
 * g_dbus_address_escape_value:
 * @string: an unescaped string to be included in a D-Bus address
 *     as the value in a key-value pair
 *
 * Escape @string so it can appear in a D-Bus address as the value
 * part of a key-value pair.
 *
 * For instance, if @string is `/run/bus-for-:0`,
 * this function would return `/run/bus-for-%3A0`,
 * which could be used in a D-Bus address like
 * `unix:nonce-tcp:host=127.0.0.1,port=42,noncefile=/run/bus-for-%3A0`.
 *
 * Returns: (transfer full): a copy of @string with all
 *     non-optionally-escaped bytes escaped
 *
 * Since: 2.36
 */
xchar_t *
g_dbus_address_escape_value (const xchar_t *string)
{
  xstring_t *s;
  xsize_t i;

  g_return_val_if_fail (string != NULL, NULL);

  /* There will often not be anything needing escaping at all. */
  s = xstring_sized_new (strlen (string));

  /* D-Bus address escaping is mostly the same as URI escaping... */
  xstring_append_uri_escaped (s, string, "\\/", FALSE);

  /* ... but '~' is an unreserved character in URIs, but a
   * non-optionally-escaped character in D-Bus addresses. */
  for (i = 0; i < s->len; i++)
    {
      if (G_UNLIKELY (s->str[i] == '~'))
        {
          s->str[i] = '%';
          xstring_insert (s, i + 1, "7E");
          i += 2;
        }
    }

  return xstring_free (s, FALSE);
}
