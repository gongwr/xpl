#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

static xboolean_t
strv_equal (xchar_t **strv, ...)
{
  xsize_t count;
  va_list list;
  const xchar_t *str;
  xboolean_t res;

  res = TRUE;
  count = 0;
  va_start (list, strv);
  while (1)
    {
      str = va_arg (list, const xchar_t *);
      if (str == NULL)
        break;
      if (xstrcmp0 (str, strv[count]) != 0)
        {
          res = FALSE;
          break;
        }
      count++;
    }
  va_end (list);

  if (res)
    res = xstrv_length (strv) == count;

  return res;
}

const xchar_t *myapp_data =
  "[Desktop Entry]\n"
  "Encoding=UTF-8\n"
  "Version=1.0\n"
  "Type=Application\n"
  "Exec=true %f\n"
  "Name=my app\n";

const xchar_t *myapp2_data =
  "[Desktop Entry]\n"
  "Encoding=UTF-8\n"
  "Version=1.0\n"
  "Type=Application\n"
  "Exec=sleep %f\n"
  "Name=my app 2\n";

const xchar_t *myapp3_data =
  "[Desktop Entry]\n"
  "Encoding=UTF-8\n"
  "Version=1.0\n"
  "Type=Application\n"
  "Exec=sleep 1\n"
  "Name=my app 3\n"
  "MimeType=image/png;";

const xchar_t *myapp4_data =
  "[Desktop Entry]\n"
  "Encoding=UTF-8\n"
  "Version=1.0\n"
  "Type=Application\n"
  "Exec=echo %f\n"
  "Name=my app 4\n"
  "MimeType=image/bmp;";

const xchar_t *myapp5_data =
  "[Desktop Entry]\n"
  "Encoding=UTF-8\n"
  "Version=1.0\n"
  "Type=Application\n"
  "Exec=true %f\n"
  "Name=my app 5\n"
  "MimeType=image/bmp;x-scheme-handler/ftp;";

const xchar_t *nosuchapp_data =
  "[Desktop Entry]\n"
  "Encoding=UTF-8\n"
  "Version=1.0\n"
  "Type=Application\n"
  "Exec=no_such_application %f\n"
  "Name=no such app\n";

const xchar_t *defaults_data =
  "[Default Applications]\n"
  "image/bmp=myapp4.desktop;\n"
  "image/png=myapp3.desktop;\n"
  "x-scheme-handler/ftp=myapp5.desktop;\n";

const xchar_t *mimecache_data =
  "[MIME Cache]\n"
  "image/bmp=myapp4.desktop;myapp5.desktop;\n"
  "image/png=myapp3.desktop;\n";

typedef struct
{
  xchar_t *mimeapps_list_home;  /* (owned) */
} Fixture;

/* Set up XDG_DATA_HOME and XDG_DATA_DIRS.
 * XDG_DATA_DIRS/applications will contain mimeapps.list
 * XDG_DATA_HOME/applications will contain myapp.desktop
 * and myapp2.desktop, and no mimeapps.list
 */
static void
setup (Fixture       *fixture,
       xconstpointer  test_data)
{
  const xchar_t *xdgdatahome;
  const xchar_t * const *xdgdatadirs;
  xchar_t *appdir;
  xchar_t *apphome;
  xchar_t *mimeapps;
  xchar_t *name;
  xint_t res;
  xerror_t *error = NULL;

  /* These are already set to a temporary directory through our use of
   * %G_TEST_OPTION_ISOLATE_DIRS below. */
  xdgdatahome = g_get_user_data_dir ();
  xdgdatadirs = g_get_system_data_dirs ();

  appdir = g_build_filename (xdgdatadirs[0], "applications", NULL);
  g_test_message ("creating '%s'", appdir);
  res = g_mkdir_with_parents (appdir, 0700);
  g_assert_cmpint (res, ==, 0);

  name = g_build_filename (appdir, "mimeapps.list", NULL);
  g_test_message ("creating '%s'", name);
  xfile_set_contents (name, defaults_data, -1, &error);
  g_assert_no_error (error);
  g_free (name);

  apphome = g_build_filename (xdgdatahome, "applications", NULL);
  g_test_message ("creating '%s'", apphome);
  res = g_mkdir_with_parents (apphome, 0700);
  g_assert_cmpint (res, ==, 0);

  name = g_build_filename (apphome, "myapp.desktop", NULL);
  g_test_message ("creating '%s'", name);
  xfile_set_contents (name, myapp_data, -1, &error);
  g_assert_no_error (error);
  g_free (name);

  name = g_build_filename (apphome, "myapp2.desktop", NULL);
  g_test_message ("creating '%s'", name);
  xfile_set_contents (name, myapp2_data, -1, &error);
  g_assert_no_error (error);
  g_free (name);

  name = g_build_filename (apphome, "myapp3.desktop", NULL);
  g_test_message ("creating '%s'", name);
  xfile_set_contents (name, myapp3_data, -1, &error);
  g_assert_no_error (error);
  g_free (name);

  name = g_build_filename (apphome, "myapp4.desktop", NULL);
  g_test_message ("creating '%s'", name);
  xfile_set_contents (name, myapp4_data, -1, &error);
  g_assert_no_error (error);
  g_free (name);

  name = g_build_filename (apphome, "myapp5.desktop", NULL);
  g_test_message ("creating '%s'", name);
  xfile_set_contents (name, myapp5_data, -1, &error);
  g_assert_no_error (error);
  g_free (name);

  name = g_build_filename (apphome, "nosuchapp.desktop", NULL);
  g_test_message ("creating '%s'", name);
  xfile_set_contents (name, nosuchapp_data, -1, &error);
  g_assert_no_error (error);
  g_free (name);

  mimeapps = g_build_filename (apphome, "mimeapps.list", NULL);
  g_test_message ("removing '%s'", mimeapps);
  g_remove (mimeapps);

  name = g_build_filename (apphome, "mimeinfo.cache", NULL);
  g_test_message ("creating '%s'", name);
  xfile_set_contents (name, mimecache_data, -1, &error);
  g_assert_no_error (error);
  g_free (name);

  g_free (apphome);
  g_free (appdir);
  g_free (mimeapps);

  /* Pointer to one of the temporary directories. */
  fixture->mimeapps_list_home = g_build_filename (g_get_user_config_dir (), "mimeapps.list", NULL);
}

static void
teardown (Fixture       *fixture,
          xconstpointer  test_data)
{
  g_free (fixture->mimeapps_list_home);
}

static void
test_mime_api (Fixture       *fixture,
               xconstpointer  test_data)
{
  xapp_info_t *appinfo;
  xapp_info_t *appinfo2;
  xerror_t *error = NULL;
  xapp_info_t *def;
  xlist_t *list;
  const xchar_t *contenttype = "application/pdf";

  /* clear things out */
  xapp_info_reset_type_associations (contenttype);

  appinfo = (xapp_info_t*)xdesktop_app_info_new ("myapp.desktop");
  appinfo2 = (xapp_info_t*)xdesktop_app_info_new ("myapp2.desktop");

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_null (def);
  g_assert_null (list);

  /* 1. add a non-default association */
  xapp_info_add_supports_type (appinfo, contenttype, &error);
  g_assert_no_error (error);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo));
  g_assert_cmpint (xlist_length (list), ==, 1);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  /* 2. add another non-default association */
  xapp_info_add_supports_type (appinfo2, contenttype, &error);
  g_assert_no_error (error);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo));
  g_assert_cmpint (xlist_length (list), ==, 2);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->data, appinfo2));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  /* 3. make the first app the default */
  xapp_info_set_as_default_for_type (appinfo, contenttype, &error);
  g_assert_no_error (error);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo));
  g_assert_cmpint (xlist_length (list), ==, 2);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->data, appinfo2));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  /* 4. make the second app the last used one */
  xapp_info_set_as_last_used_for_type (appinfo2, contenttype, &error);
  g_assert_no_error (error);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo));
  g_assert_cmpint (xlist_length (list), ==, 2);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo2));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->data, appinfo));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  /* 5. reset everything */
  xapp_info_reset_type_associations (contenttype);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_null (def);
  g_assert_null (list);

  xobject_unref (appinfo);
  xobject_unref (appinfo2);
}

/* Repeat the same tests, this time checking that we handle
 * mimeapps.list as expected. These tests are different from
 * the ones in test_mime_api() in that we directly parse
 * mimeapps.list to verify the results.
 */
static void
test_mime_file (Fixture       *fixture,
                xconstpointer  test_data)
{
  xchar_t **assoc;
  xapp_info_t *appinfo;
  xapp_info_t *appinfo2;
  xerror_t *error = NULL;
  xkey_file_t *keyfile;
  xchar_t *str;
  xboolean_t res;
  xapp_info_t *def;
  xlist_t *list;
  const xchar_t *contenttype = "application/pdf";

  /* clear things out */
  xapp_info_reset_type_associations (contenttype);

  appinfo = (xapp_info_t*)xdesktop_app_info_new ("myapp.desktop");
  appinfo2 = (xapp_info_t*)xdesktop_app_info_new ("myapp2.desktop");

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_null (def);
  g_assert_null (list);

  /* 1. add a non-default association */
  xapp_info_add_supports_type (appinfo, contenttype, &error);
  g_assert_no_error (error);

  keyfile = xkey_file_new ();
  xkey_file_load_from_file (keyfile, fixture->mimeapps_list_home, G_KEY_FILE_NONE, &error);
  g_assert_no_error (error);

  assoc = xkey_file_get_string_list (keyfile, "Added Associations", contenttype, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (strv_equal (assoc, "myapp.desktop", NULL));
  xstrfreev (assoc);

  /* we've unset XDG_DATA_DIRS so there should be no default */
  assoc = xkey_file_get_string_list (keyfile, "Default Applications", contenttype, NULL, &error);
  g_assert_nonnull (error);
  g_clear_error (&error);

  xkey_file_free (keyfile);

  /* 2. add another non-default association */
  xapp_info_add_supports_type (appinfo2, contenttype, &error);
  g_assert_no_error (error);

  keyfile = xkey_file_new ();
  xkey_file_load_from_file (keyfile, fixture->mimeapps_list_home, G_KEY_FILE_NONE, &error);
  g_assert_no_error (error);

  assoc = xkey_file_get_string_list (keyfile, "Added Associations", contenttype, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (strv_equal (assoc, "myapp.desktop", "myapp2.desktop", NULL));
  xstrfreev (assoc);

  assoc = xkey_file_get_string_list (keyfile, "Default Applications", contenttype, NULL, &error);
  g_assert_nonnull (error);
  g_clear_error (&error);

  xkey_file_free (keyfile);

  /* 3. make the first app the default */
  xapp_info_set_as_default_for_type (appinfo, contenttype, &error);
  g_assert_no_error (error);

  keyfile = xkey_file_new ();
  xkey_file_load_from_file (keyfile, fixture->mimeapps_list_home, G_KEY_FILE_NONE, &error);
  g_assert_no_error (error);

  assoc = xkey_file_get_string_list (keyfile, "Added Associations", contenttype, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (strv_equal (assoc, "myapp.desktop", "myapp2.desktop", NULL));
  xstrfreev (assoc);

  str = xkey_file_get_string (keyfile, "Default Applications", contenttype, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (str, ==, "myapp.desktop");
  g_free (str);

  xkey_file_free (keyfile);

  /* 4. make the second app the last used one */
  xapp_info_set_as_last_used_for_type (appinfo2, contenttype, &error);
  g_assert_no_error (error);

  keyfile = xkey_file_new ();
  xkey_file_load_from_file (keyfile, fixture->mimeapps_list_home, G_KEY_FILE_NONE, &error);
  g_assert_no_error (error);

  assoc = xkey_file_get_string_list (keyfile, "Added Associations", contenttype, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (strv_equal (assoc, "myapp2.desktop", "myapp.desktop", NULL));
  xstrfreev (assoc);

  xkey_file_free (keyfile);

  /* 5. reset everything */
  xapp_info_reset_type_associations (contenttype);

  keyfile = xkey_file_new ();
  xkey_file_load_from_file (keyfile, fixture->mimeapps_list_home, G_KEY_FILE_NONE, &error);
  g_assert_no_error (error);

  res = xkey_file_has_key (keyfile, "Added Associations", contenttype, NULL);
  g_assert_false (res);

  res = xkey_file_has_key (keyfile, "Default Applications", contenttype, NULL);
  g_assert_false (res);

  xkey_file_free (keyfile);

  xobject_unref (appinfo);
  xobject_unref (appinfo2);
}

/* test interaction between mimeapps.list at different levels */
static void
test_mime_default (Fixture       *fixture,
                   xconstpointer  test_data)
{
  xapp_info_t *appinfo;
  xapp_info_t *appinfo2;
  xapp_info_t *appinfo3;
  xerror_t *error = NULL;
  xapp_info_t *def;
  xlist_t *list;
  const xchar_t *contenttype = "image/png";

  /* clear things out */
  xapp_info_reset_type_associations (contenttype);

  appinfo = (xapp_info_t*)xdesktop_app_info_new ("myapp.desktop");
  appinfo2 = (xapp_info_t*)xdesktop_app_info_new ("myapp2.desktop");
  appinfo3 = (xapp_info_t*)xdesktop_app_info_new ("myapp3.desktop");

  /* myapp3 is set as the default in defaults.list */
  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo3));
  g_assert_cmpint (xlist_length (list), ==, 1);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo3));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  /* 1. add a non-default association */
  xapp_info_add_supports_type (appinfo, contenttype, &error);
  g_assert_no_error (error);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo3)); /* default is unaffected */
  g_assert_cmpint (xlist_length (list), ==, 2);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->data, appinfo3));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  /* 2. add another non-default association */
  xapp_info_add_supports_type (appinfo2, contenttype, &error);
  g_assert_no_error (error);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo3));
  g_assert_cmpint (xlist_length (list), ==, 3);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->data, appinfo2));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->next->data, appinfo3));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  /* 3. make the first app the default */
  xapp_info_set_as_default_for_type (appinfo, contenttype, &error);
  g_assert_no_error (error);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo));
  g_assert_cmpint (xlist_length (list), ==, 3);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->data, appinfo2));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->next->data, appinfo3));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  xobject_unref (appinfo);
  xobject_unref (appinfo2);
  xobject_unref (appinfo3);
}

/* test interaction between mimeinfo.cache, defaults.list and mimeapps.list
 * to ensure xapp_info_set_as_last_used_for_type doesn't incorrectly
 * change the default
 */
static void
test_mime_default_last_used (Fixture       *fixture,
                             xconstpointer  test_data)
{
  xapp_info_t *appinfo4;
  xapp_info_t *appinfo5;
  xerror_t *error = NULL;
  xapp_info_t *def;
  xlist_t *list;
  const xchar_t *contenttype = "image/bmp";

  /* clear things out */
  xapp_info_reset_type_associations (contenttype);

  appinfo4 = (xapp_info_t*)xdesktop_app_info_new ("myapp4.desktop");
  appinfo5 = (xapp_info_t*)xdesktop_app_info_new ("myapp5.desktop");

  /* myapp4 is set as the default in defaults.list */
  /* myapp4 and myapp5 can both handle image/bmp */
  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo4));
  g_assert_cmpint (xlist_length (list), ==, 2);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo4));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->data, appinfo5));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  /* 1. set default (myapp4) as last used */
  xapp_info_set_as_last_used_for_type (appinfo4, contenttype, &error);
  g_assert_no_error (error);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo4)); /* default is unaffected */
  g_assert_cmpint (xlist_length (list), ==, 2);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo4));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->data, appinfo5));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  /* 2. set other (myapp5) as last used */
  xapp_info_set_as_last_used_for_type (appinfo5, contenttype, &error);
  g_assert_no_error (error);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo4));
  g_assert_cmpint (xlist_length (list), ==, 2);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo5));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->data, appinfo4));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  /* 3. change the default to myapp5 */
  xapp_info_set_as_default_for_type (appinfo5, contenttype, &error);
  g_assert_no_error (error);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo5));
  g_assert_cmpint (xlist_length (list), ==, 2);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo5));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->data, appinfo4));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  /* 4. set myapp4 as last used */
  xapp_info_set_as_last_used_for_type (appinfo4, contenttype, &error);
  g_assert_no_error (error);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo5));
  g_assert_cmpint (xlist_length (list), ==, 2);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo4));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->data, appinfo5));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  /* 5. set myapp5 as last used again */
  xapp_info_set_as_last_used_for_type (appinfo5, contenttype, &error);
  g_assert_no_error (error);

  def = xapp_info_get_default_for_type (contenttype, FALSE);
  list = xapp_info_get_recommended_for_type (contenttype);
  g_assert_true (xapp_info_equal (def, appinfo5));
  g_assert_cmpint (xlist_length (list), ==, 2);
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->data, appinfo5));
  g_assert_true (xapp_info_equal ((xapp_info_t*)list->next->data, appinfo4));
  xobject_unref (def);
  xlist_free_full (list, xobject_unref);

  xobject_unref (appinfo4);
  xobject_unref (appinfo5);
}

static void
test_scheme_handler (Fixture       *fixture,
                     xconstpointer  test_data)
{
  xapp_info_t *info, *info5;

  info5 = (xapp_info_t*)xdesktop_app_info_new ("myapp5.desktop");
  info = xapp_info_get_default_for_uri_scheme ("ftp");
  g_assert_true (xapp_info_equal (info, info5));

  xobject_unref (info);
  xobject_unref (info5);
}

/* test that xapp_info_* ignores desktop files with nonexisting executables
 */
static void
test_mime_ignore_nonexisting (Fixture       *fixture,
                              xconstpointer  test_data)
{
  xapp_info_t *appinfo;

  appinfo = (xapp_info_t*)xdesktop_app_info_new ("nosuchapp.desktop");
  g_assert_null (appinfo);
}

static void
test_all (Fixture       *fixture,
          xconstpointer  test_data)
{
  xlist_t *all, *l;

  all = xapp_info_get_all ();

  for (l = all; l; l = l->next)
    g_assert_true (X_IS_APP_INFO (l->data));

  xlist_free_full (all, xobject_unref);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add ("/appinfo/mime/api", Fixture, NULL, setup,
              test_mime_api, teardown);
  g_test_add ("/appinfo/mime/default", Fixture, NULL, setup,
              test_mime_default, teardown);
  g_test_add ("/appinfo/mime/file", Fixture, NULL, setup,
              test_mime_file, teardown);
  g_test_add ("/appinfo/mime/scheme-handler", Fixture, NULL, setup,
              test_scheme_handler, teardown);
  g_test_add ("/appinfo/mime/default-last-used", Fixture, NULL, setup,
              test_mime_default_last_used, teardown);
  g_test_add ("/appinfo/mime/ignore-nonexisting", Fixture, NULL, setup,
              test_mime_ignore_nonexisting, teardown);
  g_test_add ("/appinfo/all", Fixture, NULL, setup, test_all, teardown);

  return g_test_run ();
}
