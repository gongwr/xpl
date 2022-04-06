/* Gtk+ object tests
 * Copyright (C) 2007 Patrick Hulin
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#include <glib.h>
#include <string.h>
#include <stdlib.h>


/* xscanner_t fixture */
typedef struct {
  xscanner_t *scanner;
} ScannerFixture;
static void
scanner_fixture_setup (ScannerFixture *fix,
                       xconstpointer   test_data)
{
  fix->scanner = g_scanner_new (NULL);
  g_assert (fix->scanner != NULL);
}
static void
scanner_fixture_teardown (ScannerFixture *fix,
                          xconstpointer   test_data)
{
  g_assert (fix->scanner != NULL);
  g_scanner_destroy (fix->scanner);
}

static void
scanner_msg_func (xscanner_t *scanner,
                  xchar_t    *message,
                  xboolean_t  error)
{
  g_assert_cmpstr (message, ==, "test");
}

static void
test_scanner_warn (ScannerFixture *fix,
                   xconstpointer   test_data)
{
  fix->scanner->msg_handler = scanner_msg_func;
  g_scanner_warn (fix->scanner, "test");
}

static void
test_scanner_error (ScannerFixture *fix,
                    xconstpointer   test_data)
{
  if (g_test_subprocess ())
    {
      int pe = fix->scanner->parse_errors;
      g_scanner_error (fix->scanner, "scanner-error-message-test");
      g_assert_cmpint (fix->scanner->parse_errors, ==, pe + 1);
      exit (0);
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("*scanner-error-message-test*");
}

static void
check_keys (xpointer_t key,
            xpointer_t value,
            xpointer_t user_data)
{
  g_assert_cmpint (GPOINTER_TO_INT (value), ==, g_ascii_strtoull (key, NULL, 0));
}

static void
test_scanner_symbols (ScannerFixture *fix,
                      xconstpointer   test_data)
{
  xint_t i;
  xchar_t buf[2];

  g_scanner_set_scope (fix->scanner, 1);

  for (i = 0; i < 10; i++)
    g_scanner_scope_add_symbol (fix->scanner,
                                1,
                                g_ascii_dtostr (buf, 2, (xdouble_t)i),
                                GINT_TO_POINTER (i));
  g_scanner_scope_foreach_symbol (fix->scanner, 1, check_keys, NULL);
  g_assert_cmpint (GPOINTER_TO_INT (g_scanner_lookup_symbol (fix->scanner, "5")), ==, 5);
  g_scanner_scope_remove_symbol (fix->scanner, 1, "5");
  g_assert (g_scanner_lookup_symbol (fix->scanner, "5") == NULL);

  g_assert_cmpint (GPOINTER_TO_INT (g_scanner_scope_lookup_symbol (fix->scanner, 1, "4")), ==, 4);
  g_assert_cmpint (GPOINTER_TO_INT (g_scanner_scope_lookup_symbol (fix->scanner, 1, "5")), ==, 0);
}

static void
test_scanner_tokens (ScannerFixture *fix,
                     xconstpointer   test_data)
{
  xchar_t buf[] = "(\t\n\r\\){}";
  const xint_t buflen = strlen (buf);
  xchar_t tokbuf[] = "(\\){}";
  const xsize_t tokbuflen = strlen (tokbuf);
  xsize_t i;

  g_scanner_input_text (fix->scanner, buf, buflen);

  g_assert_cmpint (g_scanner_cur_token (fix->scanner), ==, G_TOKEN_NONE);
  g_scanner_get_next_token (fix->scanner);
  g_assert_cmpint (g_scanner_cur_token (fix->scanner), ==, tokbuf[0]);
  g_assert_cmpint (g_scanner_cur_line (fix->scanner), ==, 1);

  for (i = 1; i < tokbuflen; i++)
    g_assert_cmpint (g_scanner_get_next_token (fix->scanner), ==, tokbuf[i]);
  g_assert_cmpint (g_scanner_get_next_token (fix->scanner), ==, G_TOKEN_EOF);
  return;
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/scanner/warn", ScannerFixture, 0, scanner_fixture_setup, test_scanner_warn, scanner_fixture_teardown);
  g_test_add ("/scanner/error", ScannerFixture, 0, scanner_fixture_setup, test_scanner_error, scanner_fixture_teardown);
  g_test_add ("/scanner/symbols", ScannerFixture, 0, scanner_fixture_setup, test_scanner_symbols, scanner_fixture_teardown);
  g_test_add ("/scanner/tokens", ScannerFixture, 0, scanner_fixture_setup, test_scanner_tokens, scanner_fixture_teardown);

  return g_test_run();
}
