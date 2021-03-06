/* GLib testing framework examples and tests
 *
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 * Authors: David Zeuthen <davidz@redhat.com>
 */

#include <glib/glib.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

static void
test_xicon_to_string (void)
{
  xicon_t *icon;
  xicon_t *icon2;
  xicon_t *icon3;
  xicon_t *icon4;
  xicon_t *icon5;
  xemblem_t *emblem1;
  xemblem_t *emblem2;
  const char *uri;
  xfile_t *location;
  char *data;
  xerror_t *error;
  xint_t origin;
  xicon_t *i;
  xfile_t *file;

  error = NULL;

  /* check that xfile_icon_t and xthemed_icon_t serialize to the encoding specified */

  uri = "file:///some/native/path/to/an/icon.png";
  location = xfile_new_for_uri (uri);
  icon = xfile_icon_new (location);

  xobject_get (icon, "file", &file, NULL);
  xassert (file == location);
  xobject_unref (file);

  data = xicon_to_string (icon);
  g_assert_cmpstr (data, ==, G_DIR_SEPARATOR_S "some" G_DIR_SEPARATOR_S "native" G_DIR_SEPARATOR_S "path" G_DIR_SEPARATOR_S "to" G_DIR_SEPARATOR_S "an" G_DIR_SEPARATOR_S "icon.png");
  icon2 = xicon_new_for_string (data, &error);
  g_assert_no_error (error);
  xassert (xicon_equal (icon, icon2));
  g_free (data);
  xobject_unref (icon);
  xobject_unref (icon2);
  xobject_unref (location);

  uri = "file:///some/native/path/to/an/icon with spaces.png";
  location = xfile_new_for_uri (uri);
  icon = xfile_icon_new (location);
  data = xicon_to_string (icon);
  g_assert_cmpstr (data, ==, G_DIR_SEPARATOR_S "some" G_DIR_SEPARATOR_S "native" G_DIR_SEPARATOR_S "path" G_DIR_SEPARATOR_S "to" G_DIR_SEPARATOR_S "an" G_DIR_SEPARATOR_S "icon with spaces.png");
  icon2 = xicon_new_for_string (data, &error);
  g_assert_no_error (error);
  xassert (xicon_equal (icon, icon2));
  g_free (data);
  xobject_unref (icon);
  xobject_unref (icon2);
  xobject_unref (location);

  uri = "sftp:///some/non-native/path/to/an/icon.png";
  location = xfile_new_for_uri (uri);
  icon = xfile_icon_new (location);
  data = xicon_to_string (icon);
  g_assert_cmpstr (data, ==, "sftp:///some/non-native/path/to/an/icon.png");
  icon2 = xicon_new_for_string (data, &error);
  g_assert_no_error (error);
  xassert (xicon_equal (icon, icon2));
  g_free (data);
  xobject_unref (icon);
  xobject_unref (icon2);
  xobject_unref (location);

#if 0
  uri = "sftp:///some/non-native/path/to/an/icon with spaces.png";
  location = xfile_new_for_uri (uri);
  icon = xfile_icon_new (location);
  data = xicon_to_string (icon);
  g_assert_cmpstr (data, ==, "sftp:///some/non-native/path/to/an/icon%20with%20spaces.png");
  icon2 = xicon_new_for_string (data, &error);
  g_assert_no_error (error);
  xassert (xicon_equal (icon, icon2));
  g_free (data);
  xobject_unref (icon);
  xobject_unref (icon2);
  xobject_unref (location);
#endif

  icon = g_themed_icon_new_with_default_fallbacks ("some-icon-symbolic");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "some-other-icon");
  data = xicon_to_string (icon);
  g_assert_cmpstr (data, ==, ". xthemed_icon_t "
                             "some-icon-symbolic some-symbolic some-other-icon some-other some "
                             "some-icon some-other-icon-symbolic some-other-symbolic");
  g_free (data);
  xobject_unref (icon);

  icon = g_themed_icon_new ("network-server");
  data = xicon_to_string (icon);
  g_assert_cmpstr (data, ==, "network-server");
  icon2 = xicon_new_for_string (data, &error);
  g_assert_no_error (error);
  xassert (xicon_equal (icon, icon2));
  g_free (data);
  xobject_unref (icon);
  xobject_unref (icon2);

  icon = g_themed_icon_new_with_default_fallbacks ("network-server");
  data = xicon_to_string (icon);
  g_assert_cmpstr (data, ==, ". xthemed_icon_t network-server network network-server-symbolic network-symbolic");
  icon2 = xicon_new_for_string (data, &error);
  g_assert_no_error (error);
  xassert (xicon_equal (icon, icon2));
  g_free (data);
  xobject_unref (icon);
  xobject_unref (icon2);

  /* Check that we can serialize from well-known specified formats */
  icon = xicon_new_for_string ("network-server%", &error);
  g_assert_no_error (error);
  icon2 = g_themed_icon_new ("network-server%");
  xassert (xicon_equal (icon, icon2));
  xobject_unref (icon);
  xobject_unref (icon2);

  icon = xicon_new_for_string ("/path/to/somewhere.png", &error);
  g_assert_no_error (error);
  location = xfile_new_for_commandline_arg ("/path/to/somewhere.png");
  icon2 = xfile_icon_new (location);
  xassert (xicon_equal (icon, icon2));
  xobject_unref (icon);
  xobject_unref (icon2);
  xobject_unref (location);

  icon = xicon_new_for_string ("/path/to/somewhere with whitespace.png", &error);
  g_assert_no_error (error);
  data = xicon_to_string (icon);
  g_assert_cmpstr (data, ==, G_DIR_SEPARATOR_S "path" G_DIR_SEPARATOR_S "to" G_DIR_SEPARATOR_S "somewhere with whitespace.png");
  g_free (data);
  location = xfile_new_for_commandline_arg ("/path/to/somewhere with whitespace.png");
  icon2 = xfile_icon_new (location);
  xassert (xicon_equal (icon, icon2));
  xobject_unref (location);
  xobject_unref (icon2);
  location = xfile_new_for_commandline_arg ("/path/to/somewhere%20with%20whitespace.png");
  icon2 = xfile_icon_new (location);
  xassert (!xicon_equal (icon, icon2));
  xobject_unref (location);
  xobject_unref (icon2);
  xobject_unref (icon);

  icon = xicon_new_for_string ("sftp:///path/to/somewhere.png", &error);
  g_assert_no_error (error);
  data = xicon_to_string (icon);
  g_assert_cmpstr (data, ==, "sftp:///path/to/somewhere.png");
  g_free (data);
  location = xfile_new_for_commandline_arg ("sftp:///path/to/somewhere.png");
  icon2 = xfile_icon_new (location);
  xassert (xicon_equal (icon, icon2));
  xobject_unref (icon);
  xobject_unref (icon2);
  xobject_unref (location);

#if 0
  icon = xicon_new_for_string ("sftp:///path/to/somewhere with whitespace.png", &error);
  g_assert_no_error (error);
  data = xicon_to_string (icon);
  g_assert_cmpstr (data, ==, "sftp:///path/to/somewhere%20with%20whitespace.png");
  g_free (data);
  location = xfile_new_for_commandline_arg ("sftp:///path/to/somewhere with whitespace.png");
  icon2 = xfile_icon_new (location);
  xassert (xicon_equal (icon, icon2));
  xobject_unref (location);
  xobject_unref (icon2);
  location = xfile_new_for_commandline_arg ("sftp:///path/to/somewhere%20with%20whitespace.png");
  icon2 = xfile_icon_new (location);
  xassert (xicon_equal (icon, icon2));
  xobject_unref (location);
  xobject_unref (icon2);
  xobject_unref (icon);
#endif

  /* Check that xthemed_icon_t serialization works */

  icon = g_themed_icon_new ("network-server");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "computer");
  data = xicon_to_string (icon);
  icon2 = xicon_new_for_string (data, &error);
  g_assert_no_error (error);
  xassert (xicon_equal (icon, icon2));
  g_free (data);
  xobject_unref (icon);
  xobject_unref (icon2);

  icon = g_themed_icon_new ("icon name with whitespace");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "computer");
  data = xicon_to_string (icon);
  icon2 = xicon_new_for_string (data, &error);
  g_assert_no_error (error);
  xassert (xicon_equal (icon, icon2));
  g_free (data);
  xobject_unref (icon);
  xobject_unref (icon2);

  icon = g_themed_icon_new_with_default_fallbacks ("network-server-xyz");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "computer");
  data = xicon_to_string (icon);
  icon2 = xicon_new_for_string (data, &error);
  g_assert_no_error (error);
  xassert (xicon_equal (icon, icon2));
  g_free (data);
  xobject_unref (icon);
  xobject_unref (icon2);

  /* Check that xemblemed_icon_t serialization works */

  icon = g_themed_icon_new ("face-smirk");
  icon2 = g_themed_icon_new ("emblem-important");
  g_themed_icon_append_name (G_THEMED_ICON (icon2), "emblem-shared");
  location = xfile_new_for_uri ("file:///some/path/somewhere.png");
  icon3 = xfile_icon_new (location);
  xobject_unref (location);
  emblem1 = xemblem_new_with_origin (icon2, XEMBLEM_ORIGIN_DEVICE);
  emblem2 = xemblem_new_with_origin (icon3, XEMBLEM_ORIGIN_LIVEMETADATA);
  icon4 = g_emblemed_icon_new (icon, emblem1);
  g_emblemed_icon_add_emblem (G_EMBLEMED_ICON (icon4), emblem2);
  data = xicon_to_string (icon4);
  icon5 = xicon_new_for_string (data, &error);
  g_assert_no_error (error);
  xassert (xicon_equal (icon4, icon5));

  xobject_get (emblem1, "origin", &origin, "icon", &i, NULL);
  xassert (origin == XEMBLEM_ORIGIN_DEVICE);
  xassert (i == icon2);
  xobject_unref (i);

  xobject_unref (emblem1);
  xobject_unref (emblem2);
  xobject_unref (icon);
  xobject_unref (icon2);
  xobject_unref (icon3);
  xobject_unref (icon4);
  xobject_unref (icon5);
  g_free (data);
}

static void
test_xicon_serialize (void)
{
  xicon_t *icon;
  xicon_t *icon2;
  xicon_t *icon3;
  xicon_t *icon4;
  xicon_t *icon5;
  xemblem_t *emblem1;
  xemblem_t *emblem2;
  xfile_t *location;
  xvariant_t *data;
  xint_t origin;
  xicon_t *i;

  /* Check that we can deserialize from well-known specified formats */
  data = xvariant_new_string ("network-server%");
  icon = xicon_deserialize (xvariant_ref_sink (data));
  xvariant_unref (data);
  icon2 = g_themed_icon_new ("network-server%");
  xassert (xicon_equal (icon, icon2));
  xobject_unref (icon);
  xobject_unref (icon2);

  data = xvariant_new_string ("/path/to/somewhere.png");
  icon = xicon_deserialize (xvariant_ref_sink (data));
  xvariant_unref (data);
  location = xfile_new_for_commandline_arg ("/path/to/somewhere.png");
  icon2 = xfile_icon_new (location);
  xassert (xicon_equal (icon, icon2));
  xobject_unref (icon);
  xobject_unref (icon2);
  xobject_unref (location);

  data = xvariant_new_string ("/path/to/somewhere with whitespace.png");
  icon = xicon_deserialize (xvariant_ref_sink (data));
  xvariant_unref (data);
  location = xfile_new_for_commandline_arg ("/path/to/somewhere with whitespace.png");
  icon2 = xfile_icon_new (location);
  xassert (xicon_equal (icon, icon2));
  xobject_unref (location);
  xobject_unref (icon2);
  location = xfile_new_for_commandline_arg ("/path/to/somewhere%20with%20whitespace.png");
  icon2 = xfile_icon_new (location);
  xassert (!xicon_equal (icon, icon2));
  xobject_unref (location);
  xobject_unref (icon2);
  xobject_unref (icon);

  data = xvariant_new_string ("sftp:///path/to/somewhere.png");
  icon = xicon_deserialize (xvariant_ref_sink (data));
  xvariant_unref (data);
  location = xfile_new_for_commandline_arg ("sftp:///path/to/somewhere.png");
  icon2 = xfile_icon_new (location);
  xassert (xicon_equal (icon, icon2));
  xobject_unref (icon);
  xobject_unref (icon2);
  xobject_unref (location);

  /* Check that xthemed_icon_t serialization works */

  icon = g_themed_icon_new ("network-server");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "computer");
  data = xicon_serialize (icon);
  icon2 = xicon_deserialize (data);
  xassert (xicon_equal (icon, icon2));
  xvariant_unref (data);
  xobject_unref (icon);
  xobject_unref (icon2);

  icon = g_themed_icon_new ("icon name with whitespace");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "computer");
  data = xicon_serialize (icon);
  icon2 = xicon_deserialize (data);
  xassert (xicon_equal (icon, icon2));
  xvariant_unref (data);
  xobject_unref (icon);
  xobject_unref (icon2);

  icon = g_themed_icon_new_with_default_fallbacks ("network-server-xyz");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "computer");
  data = xicon_serialize (icon);
  icon2 = xicon_deserialize (data);
  xassert (xicon_equal (icon, icon2));
  xvariant_unref (data);
  xobject_unref (icon);
  xobject_unref (icon2);

  /* Check that xemblemed_icon_t serialization works */

  icon = g_themed_icon_new ("face-smirk");
  icon2 = g_themed_icon_new ("emblem-important");
  g_themed_icon_append_name (G_THEMED_ICON (icon2), "emblem-shared");
  location = xfile_new_for_uri ("file:///some/path/somewhere.png");
  icon3 = xfile_icon_new (location);
  xobject_unref (location);
  emblem1 = xemblem_new_with_origin (icon2, XEMBLEM_ORIGIN_DEVICE);
  emblem2 = xemblem_new_with_origin (icon3, XEMBLEM_ORIGIN_LIVEMETADATA);
  icon4 = g_emblemed_icon_new (icon, emblem1);
  g_emblemed_icon_add_emblem (G_EMBLEMED_ICON (icon4), emblem2);
  data = xicon_serialize (icon4);
  icon5 = xicon_deserialize (data);
  xassert (xicon_equal (icon4, icon5));

  xobject_get (emblem1, "origin", &origin, "icon", &i, NULL);
  xassert (origin == XEMBLEM_ORIGIN_DEVICE);
  xassert (i == icon2);
  xobject_unref (i);

  xobject_unref (emblem1);
  xobject_unref (emblem2);
  xobject_unref (icon);
  xobject_unref (icon2);
  xobject_unref (icon3);
  xobject_unref (icon4);
  xobject_unref (icon5);
  xvariant_unref (data);
}

static void
test_themed_icon (void)
{
  xicon_t *icon1, *icon2, *icon3, *icon4;
  const xchar_t *const *names;
  const xchar_t *names2[] = { "first-symbolic", "testicon", "last", NULL };
  xchar_t *str;
  xboolean_t fallbacks;
  xvariant_t *variant;

  icon1 = g_themed_icon_new ("testicon");

  xobject_get (icon1, "use-default-fallbacks", &fallbacks, NULL);
  xassert (!fallbacks);

  names = g_themed_icon_get_names (G_THEMED_ICON (icon1));
  g_assert_cmpint (xstrv_length ((xchar_t **)names), ==, 2);
  g_assert_cmpstr (names[0], ==, "testicon");
  g_assert_cmpstr (names[1], ==, "testicon-symbolic");

  g_themed_icon_prepend_name (G_THEMED_ICON (icon1), "first-symbolic");
  g_themed_icon_append_name (G_THEMED_ICON (icon1), "last");
  names = g_themed_icon_get_names (G_THEMED_ICON (icon1));
  g_assert_cmpint (xstrv_length ((xchar_t **)names), ==, 6);
  g_assert_cmpstr (names[0], ==, "first-symbolic");
  g_assert_cmpstr (names[1], ==, "testicon");
  g_assert_cmpstr (names[2], ==, "last");
  g_assert_cmpstr (names[3], ==, "first");
  g_assert_cmpstr (names[4], ==, "testicon-symbolic");
  g_assert_cmpstr (names[5], ==, "last-symbolic");
  g_assert_cmpuint (xicon_hash (icon1), ==, 1812785139);

  icon2 = g_themed_icon_new_from_names ((xchar_t**)names2, -1);
  xassert (xicon_equal (icon1, icon2));

  str = xicon_to_string (icon2);
  icon3 = xicon_new_for_string (str, NULL);
  xassert (xicon_equal (icon2, icon3));
  g_free (str);

  variant = xicon_serialize (icon3);
  icon4 = xicon_deserialize (variant);
  xassert (xicon_equal (icon3, icon4));
  xassert (xicon_hash (icon3) == xicon_hash (icon4));
  xvariant_unref (variant);

  xobject_unref (icon1);
  xobject_unref (icon2);
  xobject_unref (icon3);
  xobject_unref (icon4);
}

static void
test_emblemed_icon (void)
{
  xicon_t *icon;
  xicon_t *icon1, *icon2, *icon3, *icon4, *icon5;
  xemblem_t *emblem, *emblem1, *emblem2;
  xlist_t *emblems;
  xvariant_t *variant;

  icon1 = g_themed_icon_new ("testicon");
  icon2 = g_themed_icon_new ("testemblem");
  emblem1 = xemblem_new (icon2);
  emblem2 = xemblem_new_with_origin (icon2, XEMBLEM_ORIGIN_TAG);

  icon3 = g_emblemed_icon_new (icon1, emblem1);
  emblems = g_emblemed_icon_get_emblems (G_EMBLEMED_ICON (icon3));
  g_assert_cmpint (xlist_length (emblems), ==, 1);
  xassert (g_emblemed_icon_get_icon (G_EMBLEMED_ICON (icon3)) == icon1);

  icon4 = g_emblemed_icon_new (icon1, emblem1);
  g_emblemed_icon_add_emblem (G_EMBLEMED_ICON (icon4), emblem2);
  emblems = g_emblemed_icon_get_emblems (G_EMBLEMED_ICON (icon4));
  g_assert_cmpint (xlist_length (emblems), ==, 2);

  xassert (!xicon_equal (icon3, icon4));

  variant = xicon_serialize (icon4);
  icon5 = xicon_deserialize (variant);
  xassert (xicon_equal (icon4, icon5));
  xassert (xicon_hash (icon4) == xicon_hash (icon5));
  xvariant_unref (variant);

  emblem = emblems->data;
  xassert (xemblem_get_icon (emblem) == icon2);
  xassert (xemblem_get_origin (emblem) == XEMBLEM_ORIGIN_UNKNOWN);

  emblem = emblems->next->data;
  xassert (xemblem_get_icon (emblem) == icon2);
  xassert (xemblem_get_origin (emblem) == XEMBLEM_ORIGIN_TAG);

  g_emblemed_icon_clear_emblems (G_EMBLEMED_ICON (icon4));
  xassert (g_emblemed_icon_get_emblems (G_EMBLEMED_ICON (icon4)) == NULL);

  xassert (xicon_hash (icon4) != xicon_hash (icon2));
  xobject_get (icon4, "gicon", &icon, NULL);
  xassert (icon == icon1);
  xobject_unref (icon);

  xobject_unref (icon1);
  xobject_unref (icon2);
  xobject_unref (icon3);
  xobject_unref (icon4);
  xobject_unref (icon5);

  xobject_unref (emblem1);
  xobject_unref (emblem2);
}

static void
load_cb (xobject_t      *source_object,
         xasync_result_t *res,
         xpointer_t      data)
{
  xloadable_icon_t *icon = G_LOADABLE_ICON (source_object);
  xmain_loop_t *loop = data;
  xerror_t *error = NULL;
  xinput_stream_t *stream;

  stream = g_loadable_icon_load_finish (icon, res, NULL, &error);
  g_assert_no_error (error);
  xassert (X_IS_INPUT_STREAM (stream));
  xobject_unref (stream);
  xmain_loop_quit (loop);
}

static void
loadable_icon_tests (xloadable_icon_t *icon)
{
  xerror_t *error = NULL;
  xinput_stream_t *stream;
  xmain_loop_t *loop;

  stream = g_loadable_icon_load (icon, 20, NULL, NULL, &error);
  g_assert_no_error (error);
  xassert (X_IS_INPUT_STREAM (stream));
  xobject_unref (stream);

  loop = xmain_loop_new (NULL, FALSE);
  g_loadable_icon_load_async (icon, 20, NULL, load_cb, loop);
  xmain_loop_run (loop);
  xmain_loop_unref (loop);
}

static void
test_file_icon (void)
{
  xfile_t *file;
  xicon_t *icon;
  xicon_t *icon2;
  xicon_t *icon3;
  xicon_t *icon4;
  xchar_t *str;
  xvariant_t *variant;

  file = xfile_new_for_path (g_test_get_filename (G_TEST_DIST, "g-icon.c", NULL));
  icon = xfile_icon_new (file);
  xobject_unref (file);

  loadable_icon_tests (G_LOADABLE_ICON (icon));

  str = xicon_to_string (icon);
  icon2 = xicon_new_for_string (str, NULL);
  xassert (xicon_equal (icon, icon2));
  g_free (str);

  file = xfile_new_for_path ("/\1\2\3/\244");
  icon4 = xfile_icon_new (file);

  variant = xicon_serialize (icon4);
  icon3 = xicon_deserialize (variant);
  xassert (xicon_equal (icon4, icon3));
  xassert (xicon_hash (icon4) == xicon_hash (icon3));
  xvariant_unref (variant);

  xobject_unref (icon);
  xobject_unref (icon2);
  xobject_unref (icon3);
  xobject_unref (icon4);
  xobject_unref (file);
}

static void
test_bytes_icon (void)
{
  xbytes_t *bytes;
  xbytes_t *bytes2;
  xicon_t *icon;
  xicon_t *icon2;
  xicon_t *icon3;
  xvariant_t *variant;
  const xchar_t *data = "1234567890987654321";

  bytes = xbytes_new_static (data, strlen (data));
  icon = xbytes_icon_new (bytes);
  icon2 = xbytes_icon_new (bytes);

  xassert (xbytes_icon_get_bytes (XBYTES_ICON (icon)) == bytes);
  xassert (xicon_equal (icon, icon2));
  xassert (xicon_hash (icon) == xicon_hash (icon2));

  xobject_get (icon, "bytes", &bytes2, NULL);
  xassert (bytes == bytes2);
  xbytes_unref (bytes2);

  variant = xicon_serialize (icon);
  icon3 = xicon_deserialize (variant);
  xassert (xicon_equal (icon, icon3));
  xassert (xicon_hash (icon) == xicon_hash (icon3));

  loadable_icon_tests (G_LOADABLE_ICON (icon));

  xvariant_unref (variant);
  xobject_unref (icon);
  xobject_unref (icon2);
  xobject_unref (icon3);
  xbytes_unref (bytes);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/icons/to-string", test_xicon_to_string);
  g_test_add_func ("/icons/serialize", test_xicon_serialize);
  g_test_add_func ("/icons/themed", test_themed_icon);
  g_test_add_func ("/icons/emblemed", test_emblemed_icon);
  g_test_add_func ("/icons/file", test_file_icon);
  g_test_add_func ("/icons/bytes", test_bytes_icon);

  return g_test_run();
}
