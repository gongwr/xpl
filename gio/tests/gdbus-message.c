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

#include <locale.h>
#include <gio/gio.h>

/* ---------------------------------------------------------------------------------------------------- */

static void
on_notify_locked (xobject_t    *object,
                  xparam_spec_t *pspec,
                  xpointer_t    user_data)
{
  xint_t *count = user_data;
  *count += 1;
}

static void
message_lock (void)
{
  xdbus_message_t *m;
  xint_t count;

  count = 0;
  m = xdbus_message_new ();
  xsignal_connect (m,
                    "notify::locked",
                    G_CALLBACK (on_notify_locked),
                    &count);
  xassert (!xdbus_message_get_locked (m));
  xdbus_message_lock (m);
  xassert (xdbus_message_get_locked (m));
  g_assert_cmpint (count, ==, 1);
  xdbus_message_lock (m);
  xassert (xdbus_message_get_locked (m));
  g_assert_cmpint (count, ==, 1);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*Attempted to modify a locked message*");
  xdbus_message_set_serial (m, 42);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*Attempted to modify a locked message*");
  xdbus_message_set_byte_order (m, G_DBUS_MESSAGE_BYTE_ORDER_BIG_ENDIAN);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*Attempted to modify a locked message*");
  xdbus_message_set_message_type (m, G_DBUS_MESSAGE_TYPE_METHOD_CALL);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*Attempted to modify a locked message*");
  xdbus_message_set_flags (m, G_DBUS_MESSAGE_FLAGS_NONE);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*Attempted to modify a locked message*");
  xdbus_message_set_body (m, NULL);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*Attempted to modify a locked message*");
  xdbus_message_set_header (m, 0, NULL);
  g_test_assert_expected_messages ();

  xobject_unref (m);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
message_copy (void)
{
  xdbus_message_t *m;
  xdbus_message_t *copy;
  xerror_t *error;
  xuchar_t *m_headers;
  xuchar_t *copy_headers;
  xuint_t n;

  m = xdbus_message_new_method_call ("org.example.Name",
                                      "/org/example/Object",
                                      "org.example.Interface",
                                      "Method");
  xdbus_message_set_serial (m, 42);
  xdbus_message_set_byte_order (m, G_DBUS_MESSAGE_BYTE_ORDER_BIG_ENDIAN);

  error = NULL;
  copy = xdbus_message_copy (m, &error);
  g_assert_no_error (error);
  xassert (X_IS_DBUS_MESSAGE (copy));
  xassert (m != copy);
  g_assert_cmpint (G_OBJECT (m)->ref_count, ==, 1);
  g_assert_cmpint (G_OBJECT (copy)->ref_count, ==, 1);

  g_assert_cmpint (xdbus_message_get_serial (copy), ==, xdbus_message_get_serial (m));
  g_assert_cmpint (xdbus_message_get_byte_order (copy), ==, xdbus_message_get_byte_order (m));
  g_assert_cmpint (xdbus_message_get_flags (copy), ==, xdbus_message_get_flags (m));
  g_assert_cmpint (xdbus_message_get_message_type (copy), ==, xdbus_message_get_message_type (m));
  m_headers = xdbus_message_get_header_fields (m);
  copy_headers = xdbus_message_get_header_fields (copy);
  xassert (m_headers != NULL);
  xassert (copy_headers != NULL);
  for (n = 0; m_headers[n] != 0; n++)
    {
      xvariant_t *m_val;
      xvariant_t *copy_val;
      m_val = xdbus_message_get_header (m, m_headers[n]);
      copy_val = xdbus_message_get_header (m, m_headers[n]);
      xassert (m_val != NULL);
      xassert (copy_val != NULL);
      g_assert_cmpvariant (m_val, copy_val);
    }
  g_assert_cmpint (n, >, 0); /* make sure we actually compared headers etc. */
  g_assert_cmpint (copy_headers[n], ==, 0);
  g_free (m_headers);
  g_free (copy_headers);

  xobject_unref (copy);
  xobject_unref (m);
}

/* ---------------------------------------------------------------------------------------------------- */

/* test_t xdbus_message_bytes_needed() returns correct results for a variety of
 * arbitrary binary inputs.*/
static void
message_bytes_needed (void)
{
  const struct
    {
      const xuint8_t blob[16];
      xssize_t expected_bytes_needed;
    }
  vectors[] =
    {
      /* Little endian with header rounding */
      { { 'l', 0, 0, 1,  /* endianness, message type, flags, protocol version */
          50, 0, 0, 0,  /* body length */
          1, 0, 0, 0,  /* message serial */
          7, 0, 0, 0  /* header length */}, 74 },
      /* Little endian without header rounding */
      { { 'l', 0, 0, 1,  /* endianness, message type, flags, protocol version */
          50, 0, 0, 0,  /* body length */
          1, 0, 0, 0,  /* message serial */
          8, 0, 0, 0  /* header length */}, 74 },
      /* Big endian with header rounding */
      { { 'B', 0, 0, 1,  /* endianness, message type, flags, protocol version */
          0, 0, 0, 50,  /* body length */
          0, 0, 0, 1,  /* message serial */
          0, 0, 0, 7  /* header length */}, 74 },
      /* Big endian without header rounding */
      { { 'B', 0, 0, 1,  /* endianness, message type, flags, protocol version */
          0, 0, 0, 50,  /* body length */
          0, 0, 0, 1,  /* message serial */
          0, 0, 0, 8  /* header length */}, 74 },
      /* Invalid endianness */
      { { '!', 0, 0, 1,  /* endianness, message type, flags, protocol version */
          0, 0, 0, 50,  /* body length */
          0, 0, 0, 1,  /* message serial */
          0, 0, 0, 8  /* header length */}, -1 },
      /* Oversized */
      { { 'l', 0, 0, 1,  /* endianness, message type, flags, protocol version */
          0, 0, 0, 0x08,  /* body length (128MiB) */
          1, 0, 0, 0,  /* message serial */
          7, 0, 0, 0  /* header length */}, -1 },
    };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      xssize_t bytes_needed;
      xerror_t *local_error = NULL;

      g_test_message ("Vector: %" G_GSIZE_FORMAT, i);

      bytes_needed = xdbus_message_bytes_needed ((xuchar_t *) vectors[i].blob,
                                                  G_N_ELEMENTS (vectors[i].blob),
                                                  &local_error);

      if (vectors[i].expected_bytes_needed < 0)
        g_assert_error (local_error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
      else
        g_assert_no_error (local_error);
      g_assert_cmpint (bytes_needed, ==, vectors[i].expected_bytes_needed);

      g_clear_error (&local_error);
    }
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  setlocale (LC_ALL, "C");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gdbus/message/lock", message_lock);
  g_test_add_func ("/gdbus/message/copy", message_copy);
  g_test_add_func ("/gdbus/message/bytes-needed", message_bytes_needed);

  return g_test_run ();
}
