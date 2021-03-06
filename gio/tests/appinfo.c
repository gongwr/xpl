
#include <locale.h>
#include <string.h>

#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

static void
test_launch_for_app_info (xapp_info_t *appinfo)
{
  xerror_t *error = NULL;
  xboolean_t success;
  xfile_t *file;
  xlist_t *l;
  const xchar_t *path;
  xchar_t *uri;

  if (g_getenv ("DISPLAY") == NULL || g_getenv ("DISPLAY")[0] == '\0')
    {
      g_test_skip ("No DISPLAY set");
      return;
    }

  success = xapp_info_launch (appinfo, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (success);

  success = xapp_info_launch_uris (appinfo, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (success);

  path = g_test_get_filename (G_TEST_BUILT, "appinfo-test.desktop", NULL);
  file = xfile_new_for_path (path);
  l = NULL;
  l = xlist_append (l, file);

  success = xapp_info_launch (appinfo, l, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (success);
  xlist_free (l);
  xobject_unref (file);

  l = NULL;
  uri = xstrconcat ("file://", g_test_get_dir (G_TEST_BUILT), "/appinfo-test.desktop", NULL);
  l = xlist_append (l, uri);
  l = xlist_append (l, "file:///etc/group#adm");

  success = xapp_info_launch_uris (appinfo, l, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (success);

  xlist_free (l);
  g_free (uri);
}

static void
test_launch (void)
{
  xapp_info_t *appinfo;
  const xchar_t *path;

  path = g_test_get_filename (G_TEST_BUILT, "appinfo-test.desktop", NULL);
  appinfo = (xapp_info_t*)xdesktop_app_info_new_from_filename (path);

  if (appinfo == NULL)
    {
      g_test_skip ("appinfo-test binary not installed");
      return;
    }

  test_launch_for_app_info (appinfo);
  xobject_unref (appinfo);
}

static void
test_launch_no_app_id (void)
{
  const xchar_t desktop_file_base_contents[] =
    "[Desktop Entry]\n"
    "Type=Application\n"
    "GenericName=generic-appinfo-test\n"
    "Name=appinfo-test\n"
    "Name[de]=appinfo-test-de\n"
    "X-GNOME-FullName=example\n"
    "X-GNOME-FullName[de]=Beispiel\n"
    "Comment=xapp_info_t example\n"
    "Comment[de]=xapp_info_t Beispiel\n"
    "Icon=testicon.svg\n"
    "Terminal=false\n"
    "StartupNotify=true\n"
    "StartupWMClass=appinfo-class\n"
    "MimeType=image/png;image/jpeg;\n"
    "Keywords=keyword1;test keyword;\n"
    "Categories=GNOME;GTK;\n";

  xchar_t *exec_line_variants[2];
  xsize_t i;

  exec_line_variants[0] = xstrdup_printf (
      "Exec=%s/appinfo-test --option %%U %%i --name %%c --filename %%k %%m %%%%",
      g_test_get_dir (G_TEST_BUILT));
  exec_line_variants[1] = xstrdup_printf (
      "Exec=%s/appinfo-test --option %%u %%i --name %%c --filename %%k %%m %%%%",
      g_test_get_dir (G_TEST_BUILT));

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=791337");

  for (i = 0; i < G_N_ELEMENTS (exec_line_variants); i++)
    {
      xchar_t *desktop_file_contents;
      xkey_file_t *fake_desktop_file;
      xapp_info_t *appinfo;
      xboolean_t loaded;

      g_test_message ("Exec line variant #%" G_GSIZE_FORMAT, i);

      desktop_file_contents = xstrdup_printf ("%s\n%s",
                                               desktop_file_base_contents,
                                               exec_line_variants[i]);

      /* We load a desktop file from memory to force the app not
       * to have an app ID, which would check different codepaths.
       */
      fake_desktop_file = xkey_file_new ();
      loaded = xkey_file_load_from_data (fake_desktop_file, desktop_file_contents, -1, G_KEY_FILE_NONE, NULL);
      g_assert_true (loaded);

      appinfo = (xapp_info_t*)xdesktop_app_info_new_from_keyfile (fake_desktop_file);
      g_assert_nonnull (appinfo);

      test_launch_for_app_info (appinfo);

      g_free (desktop_file_contents);
      xobject_unref (appinfo);
      xkey_file_unref (fake_desktop_file);
    }

  g_free (exec_line_variants[1]);
  g_free (exec_line_variants[0]);
}

static void
test_locale (const char *locale)
{
  xapp_info_t *appinfo;
  xchar_t *orig = NULL;
  const xchar_t *path;

  orig = xstrdup (setlocale (LC_ALL, NULL));
  g_setenv ("LANGUAGE", locale, TRUE);
  setlocale (LC_ALL, "");

  path = g_test_get_filename (G_TEST_DIST, "appinfo-test-static.desktop", NULL);
  appinfo = (xapp_info_t*)xdesktop_app_info_new_from_filename (path);

  if (xstrcmp0 (locale, "C") == 0)
    {
      g_assert_cmpstr (xapp_info_get_name (appinfo), ==, "appinfo-test");
      g_assert_cmpstr (xapp_info_get_description (appinfo), ==, "xapp_info_t example");
      g_assert_cmpstr (xapp_info_get_display_name (appinfo), ==, "example");
    }
  else if (xstr_has_prefix (locale, "en"))
    {
      g_assert_cmpstr (xapp_info_get_name (appinfo), ==, "appinfo-test");
      g_assert_cmpstr (xapp_info_get_description (appinfo), ==, "xapp_info_t example");
      g_assert_cmpstr (xapp_info_get_display_name (appinfo), ==, "example");
    }
  else if (xstr_has_prefix (locale, "de"))
    {
      g_assert_cmpstr (xapp_info_get_name (appinfo), ==, "appinfo-test-de");
      g_assert_cmpstr (xapp_info_get_description (appinfo), ==, "xapp_info_t Beispiel");
      g_assert_cmpstr (xapp_info_get_display_name (appinfo), ==, "Beispiel");
    }

  xobject_unref (appinfo);

  g_setenv ("LANGUAGE", orig, TRUE);
  setlocale (LC_ALL, "");
  g_free (orig);
}

static void
test_text (void)
{
  test_locale ("C");
  test_locale ("en_US");
  test_locale ("de");
  test_locale ("de_DE.UTF-8");
}

static void
test_basic (void)
{
  xapp_info_t *appinfo;
  xapp_info_t *appinfo2;
  xicon_t *icon, *icon2;
  const xchar_t *path;

  path = g_test_get_filename (G_TEST_DIST, "appinfo-test-static.desktop", NULL);
  appinfo = (xapp_info_t*)xdesktop_app_info_new_from_filename (path);
  g_assert_nonnull (appinfo);

  g_assert_cmpstr (xapp_info_get_id (appinfo), ==, "appinfo-test-static.desktop");
  g_assert_nonnull (strstr (xapp_info_get_executable (appinfo), "true"));

  icon = xapp_info_get_icon (appinfo);
  g_assert_true (X_IS_THEMED_ICON (icon));
  icon2 = g_themed_icon_new ("testicon");
  g_assert_true (xicon_equal (icon, icon2));
  xobject_unref (icon2);

  appinfo2 = xapp_info_dup (appinfo);
  g_assert_cmpstr (xapp_info_get_id (appinfo), ==, xapp_info_get_id (appinfo2));
  g_assert_cmpstr (xapp_info_get_commandline (appinfo), ==, xapp_info_get_commandline (appinfo2));

  xobject_unref (appinfo);
  xobject_unref (appinfo2);
}

static void
test_show_in (void)
{
  xapp_info_t *appinfo;
  const xchar_t *path;

  path = g_test_get_filename (G_TEST_BUILT, "appinfo-test.desktop", NULL);
  appinfo = (xapp_info_t*)xdesktop_app_info_new_from_filename (path);

  if (appinfo == NULL)
    {
      g_test_skip ("appinfo-test binary not installed");
      return;
    }

  g_assert_true (xapp_info_should_show (appinfo));
  xobject_unref (appinfo);

  path = g_test_get_filename (G_TEST_BUILT, "appinfo-test-gnome.desktop", NULL);
  appinfo = (xapp_info_t*)xdesktop_app_info_new_from_filename (path);
  g_assert_true (xapp_info_should_show (appinfo));
  xobject_unref (appinfo);

  path = g_test_get_filename (G_TEST_BUILT, "appinfo-test-notgnome.desktop", NULL);
  appinfo = (xapp_info_t*)xdesktop_app_info_new_from_filename (path);
  g_assert_false (xapp_info_should_show (appinfo));
  xobject_unref (appinfo);
}

static void
test_commandline (void)
{
  xapp_info_t *appinfo;
  xerror_t *error;
  xchar_t *cmdline;
  xchar_t *cmdline_out;

  cmdline = xstrconcat (g_test_get_dir (G_TEST_BUILT), "/appinfo-test --option", NULL);
  cmdline_out = xstrconcat (cmdline, " %u", NULL);

  error = NULL;
  appinfo = xapp_info_create_from_commandline (cmdline,
                                                "cmdline-app-test",
                                                G_APP_INFO_CREATE_SUPPORTS_URIS,
                                                &error);
  g_assert_no_error (error);
  g_assert_nonnull (appinfo);
  g_assert_cmpstr (xapp_info_get_name (appinfo), ==, "cmdline-app-test");
  g_assert_cmpstr (xapp_info_get_commandline (appinfo), ==, cmdline_out);
  g_assert_true (xapp_info_supports_uris (appinfo));
  g_assert_false (xapp_info_supports_files (appinfo));

  xobject_unref (appinfo);

  g_free (cmdline_out);
  cmdline_out = xstrconcat (cmdline, " %f", NULL);

  error = NULL;
  appinfo = xapp_info_create_from_commandline (cmdline,
                                                "cmdline-app-test",
                                                G_APP_INFO_CREATE_NONE,
                                                &error);
  g_assert_no_error (error);
  g_assert_nonnull (appinfo);
  g_assert_cmpstr (xapp_info_get_name (appinfo), ==, "cmdline-app-test");
  g_assert_cmpstr (xapp_info_get_commandline (appinfo), ==, cmdline_out);
  g_assert_false (xapp_info_supports_uris (appinfo));
  g_assert_true (xapp_info_supports_files (appinfo));

  xobject_unref (appinfo);

  g_free (cmdline);
  g_free (cmdline_out);
}

static void
test_launch_context (void)
{
  xapp_launch_context_t *context;
  xapp_info_t *appinfo;
  xchar_t *str;
  xchar_t *cmdline;

  cmdline = xstrconcat (g_test_get_dir (G_TEST_BUILT), "/appinfo-test --option", NULL);

  context = xapp_launch_context_new ();
  appinfo = xapp_info_create_from_commandline (cmdline,
                                                "cmdline-app-test",
                                                G_APP_INFO_CREATE_SUPPORTS_URIS,
                                                NULL);

  str = xapp_launch_context_get_display (context, appinfo, NULL);
  g_assert_null (str);

  str = xapp_launch_context_get_startup_notify_id (context, appinfo, NULL);
  g_assert_null (str);

  xobject_unref (appinfo);
  xobject_unref (context);

  g_free (cmdline);
}

static xboolean_t launched_reached;

static void
launched (xapp_launch_context_t *context,
          xapp_info_t          *info,
          xvariant_t          *platform_data,
          xpointer_t           user_data)
{
  xint_t pid;

  pid = 0;
  g_assert_true (xvariant_lookup (platform_data, "pid", "i", &pid));
  g_assert_cmpint (pid, !=, 0);

  launched_reached = TRUE;
}

static void
launch_failed (xapp_launch_context_t *context,
               const xchar_t       *startup_notify_id)
{
  g_assert_not_reached ();
}

static void
test_launch_context_signals (void)
{
  xapp_launch_context_t *context;
  xapp_info_t *appinfo;
  xerror_t *error = NULL;
  xboolean_t success;
  xchar_t *cmdline;

  cmdline = xstrconcat (g_test_get_dir (G_TEST_BUILT), "/appinfo-test --option", NULL);

  context = xapp_launch_context_new ();
  xsignal_connect (context, "launched", G_CALLBACK (launched), NULL);
  xsignal_connect (context, "launch_failed", G_CALLBACK (launch_failed), NULL);
  appinfo = xapp_info_create_from_commandline (cmdline,
                                                "cmdline-app-test",
                                                G_APP_INFO_CREATE_SUPPORTS_URIS,
                                                NULL);

  success = xapp_info_launch (appinfo, NULL, context, &error);
  g_assert_no_error (error);
  g_assert_true (success);

  g_assert_true (launched_reached);

  xobject_unref (appinfo);
  xobject_unref (context);

  g_free (cmdline);
}

static void
test_tryexec (void)
{
  xapp_info_t *appinfo;
  const xchar_t *path;

  path = g_test_get_filename (G_TEST_BUILT, "appinfo-test2.desktop", NULL);
  appinfo = (xapp_info_t*)xdesktop_app_info_new_from_filename (path);

  g_assert_null (appinfo);
}

/* test_t that we can set an appinfo as default for a mime type or
 * file extension, and also add and remove handled mime types.
 */
static void
test_associations (void)
{
  xapp_info_t *appinfo;
  xapp_info_t *appinfo2;
  xerror_t *error = NULL;
  xboolean_t result;
  xlist_t *list;
  xchar_t *cmdline;

  cmdline = xstrconcat (g_test_get_dir (G_TEST_BUILT), "/appinfo-test --option", NULL);
  appinfo = xapp_info_create_from_commandline (cmdline,
                                                "cmdline-app-test",
                                                G_APP_INFO_CREATE_SUPPORTS_URIS,
                                                NULL);
  g_free (cmdline);

  result = xapp_info_set_as_default_for_type (appinfo, "application/x-glib-test", &error);
  g_assert_no_error (error);
  g_assert_true (result);

  appinfo2 = xapp_info_get_default_for_type ("application/x-glib-test", FALSE);

  g_assert_nonnull (appinfo2);
  g_assert_cmpstr (xapp_info_get_commandline (appinfo), ==, xapp_info_get_commandline (appinfo2));

  xobject_unref (appinfo2);

  result = xapp_info_set_as_default_for_extension (appinfo, "gio-tests", &error);
  g_assert_no_error (error);
  g_assert_true (result);

  appinfo2 = xapp_info_get_default_for_type ("application/x-extension-gio-tests", FALSE);

  g_assert_nonnull (appinfo2);
  g_assert_cmpstr (xapp_info_get_commandline (appinfo), ==, xapp_info_get_commandline (appinfo2));

  xobject_unref (appinfo2);

  result = xapp_info_add_supports_type (appinfo, "application/x-gio-test", &error);
  g_assert_no_error (error);
  g_assert_true (result);

  list = xapp_info_get_all_for_type ("application/x-gio-test");
  g_assert_cmpint (xlist_length (list), ==, 1);
  appinfo2 = list->data;
  g_assert_cmpstr (xapp_info_get_commandline (appinfo), ==, xapp_info_get_commandline (appinfo2));
  xobject_unref (appinfo2);
  xlist_free (list);

  g_assert_true (xapp_info_can_remove_supports_type (appinfo));
  result = xapp_info_remove_supports_type (appinfo, "application/x-gio-test", &error);
  g_assert_no_error (error);
  g_assert_true (result);

  g_assert_true (xapp_info_can_delete (appinfo));
  g_assert_true (xapp_info_delete (appinfo));
  xobject_unref (appinfo);
}

static void
test_environment (void)
{
  xapp_launch_context_t *ctx;
  xchar_t **env;
  const xchar_t *path;

  g_unsetenv ("FOO");
  g_unsetenv ("BLA");
  path = g_getenv ("PATH");

  ctx = xapp_launch_context_new ();

  env = xapp_launch_context_get_environment (ctx);

  g_assert_null (g_environ_getenv (env, "FOO"));
  g_assert_null (g_environ_getenv (env, "BLA"));
  g_assert_cmpstr (g_environ_getenv (env, "PATH"), ==, path);

  xstrfreev (env);

  xapp_launch_context_setenv (ctx, "FOO", "bar");
  xapp_launch_context_setenv (ctx, "BLA", "bla");

  env = xapp_launch_context_get_environment (ctx);

  g_assert_cmpstr (g_environ_getenv (env, "FOO"), ==, "bar");
  g_assert_cmpstr (g_environ_getenv (env, "BLA"), ==, "bla");
  g_assert_cmpstr (g_environ_getenv (env, "PATH"), ==, path);

  xstrfreev (env);

  xapp_launch_context_setenv (ctx, "FOO", "baz");
  xapp_launch_context_unsetenv (ctx, "BLA");

  env = xapp_launch_context_get_environment (ctx);

  g_assert_cmpstr (g_environ_getenv (env, "FOO"), ==, "baz");
  g_assert_null (g_environ_getenv (env, "BLA"));

  xstrfreev (env);

  xobject_unref (ctx);
}

static void
test_startup_wm_class (void)
{
  xdesktop_app_info_t *appinfo;
  const char *wm_class;
  const xchar_t *path;

  path = g_test_get_filename (G_TEST_DIST, "appinfo-test-static.desktop", NULL);
  appinfo = xdesktop_app_info_new_from_filename (path);
  wm_class = xdesktop_app_info_get_startup_wm_class (appinfo);

  g_assert_cmpstr (wm_class, ==, "appinfo-class");

  xobject_unref (appinfo);
}

static void
test_supported_types (void)
{
  xapp_info_t *appinfo;
  const char * const *content_types;
  const xchar_t *path;

  path = g_test_get_filename (G_TEST_DIST, "appinfo-test-static.desktop", NULL);
  appinfo = G_APP_INFO (xdesktop_app_info_new_from_filename (path));
  content_types = xapp_info_get_supported_types (appinfo);

  g_assert_cmpint (xstrv_length ((char**)content_types), ==, 2);
  g_assert_cmpstr (content_types[0], ==, "image/png");

  xobject_unref (appinfo);
}

static void
test_from_keyfile (void)
{
  xdesktop_app_info_t *info;
  xkey_file_t *kf;
  xerror_t *error = NULL;
  const xchar_t *categories;
  xchar_t **categories_list;
  xsize_t categories_count;
  xchar_t **keywords;
  const xchar_t *file;
  const xchar_t *name;
  const xchar_t *path;

  path = g_test_get_filename (G_TEST_DIST, "appinfo-test-static.desktop", NULL);
  kf = xkey_file_new ();
  xkey_file_load_from_file (kf, path, G_KEY_FILE_NONE, &error);
  g_assert_no_error (error);
  info = xdesktop_app_info_new_from_keyfile (kf);
  xkey_file_unref (kf);
  g_assert_nonnull (info);

  xobject_get (info, "filename", &file, NULL);
  g_assert_null (file);

  file = xdesktop_app_info_get_filename (info);
  g_assert_null (file);
  categories = xdesktop_app_info_get_categories (info);
  g_assert_cmpstr (categories, ==, "GNOME;GTK;");
  categories_list = xdesktop_app_info_get_string_list (info, "Categories", &categories_count);
  g_assert_cmpint (categories_count, ==, 2);
  g_assert_cmpint (xstrv_length (categories_list), ==, 2);
  g_assert_cmpstr (categories_list[0], ==, "GNOME");
  g_assert_cmpstr (categories_list[1], ==, "GTK");
  keywords = (xchar_t **)xdesktop_app_info_get_keywords (info);
  g_assert_cmpint (xstrv_length (keywords), ==, 2);
  g_assert_cmpstr (keywords[0], ==, "keyword1");
  g_assert_cmpstr (keywords[1], ==, "test keyword");
  name = xdesktop_app_info_get_generic_name (info);
  g_assert_cmpstr (name, ==, "generic-appinfo-test");
  g_assert_false (xdesktop_app_info_get_nodisplay (info));

  xstrfreev (categories_list);
  xobject_unref (info);
}

int
main (int argc, char *argv[])
{
  g_setenv ("XDG_CURRENT_DESKTOP", "GNOME", TRUE);

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/appinfo/basic", test_basic);
  g_test_add_func ("/appinfo/text", test_text);
  g_test_add_func ("/appinfo/launch", test_launch);
  g_test_add_func ("/appinfo/launch/no-appid", test_launch_no_app_id);
  g_test_add_func ("/appinfo/show-in", test_show_in);
  g_test_add_func ("/appinfo/commandline", test_commandline);
  g_test_add_func ("/appinfo/launch-context", test_launch_context);
  g_test_add_func ("/appinfo/launch-context-signals", test_launch_context_signals);
  g_test_add_func ("/appinfo/tryexec", test_tryexec);
  g_test_add_func ("/appinfo/associations", test_associations);
  g_test_add_func ("/appinfo/environment", test_environment);
  g_test_add_func ("/appinfo/startup-wm-class", test_startup_wm_class);
  g_test_add_func ("/appinfo/supported-types", test_supported_types);
  g_test_add_func ("/appinfo/from-keyfile", test_from_keyfile);

  return g_test_run ();
}
