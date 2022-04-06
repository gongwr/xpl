/*
 * Copyright © 2013 Canonical Limited
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "gdbus-sessionbus.h"

static xdesktop_app_info_t *appinfo;
static int current_state;
static xboolean_t saw_startup_id;
static xboolean_t requested_startup_id;


static xtype_t test_app_launch_context_get_type (void);
typedef xapp_launch_context_t TestAppLaunchContext;
typedef xapp_launch_context_class_t TestAppLaunchContextClass;
G_DEFINE_TYPE (TestAppLaunchContext, test_app_launch_context, XTYPE_APP_LAUNCH_CONTEXT)

static xchar_t *
test_app_launch_context_get_startup_notify_id (xapp_launch_context_t *context,
                                               xapp_info_t          *info,
                                               xlist_t             *uris)
{
  requested_startup_id = TRUE;
  return xstrdup ("expected startup id");
}

static void
test_app_launch_context_init (TestAppLaunchContext *ctx)
{
}

static void
test_app_launch_context_class_init (xapp_launch_context_class_t *class)
{
  class->get_startup_notify_id = test_app_launch_context_get_startup_notify_id;
}

static xtype_t test_application_get_type (void);
typedef xapplication_t TestApplication;
typedef xapplication_class_t TestApplicationClass;
G_DEFINE_TYPE (TestApplication, test_application, XTYPE_APPLICATION)

static void
saw_action (const xchar_t *action)
{
  /* This is the main driver of the test.  It's a bit of a state
   * machine.
   *
   * Each time some event arrives on the app, it calls here to report
   * which event it was.  The initial activation of the app is what
   * starts everything in motion (starting from state 0).  At each
   * state, we assert that we receive the expected event, send the next
   * event, then update the current_state variable so we do the correct
   * thing next time.
   */

  switch (current_state)
    {
      case 0: g_assert_cmpstr (action, ==, "activate");

      /* Let's try another activation... */
      xapp_info_launch (G_APP_INFO (appinfo), NULL, NULL, NULL);
      current_state = 1; return; case 1: g_assert_cmpstr (action, ==, "activate");


      /* Now let's try opening some files... */
      {
        xlist_t *files;

        files = xlist_prepend (NULL, xfile_new_for_uri ("file:///a/b"));
        files = xlist_append (files, xfile_new_for_uri ("file:///c/d"));
        xapp_info_launch (G_APP_INFO (appinfo), files, NULL, NULL);
        xlist_free_full (files, xobject_unref);
      }
      current_state = 2; return; case 2: g_assert_cmpstr (action, ==, "open");

      /* Now action activations... */
      xdesktop_app_info_launch_action (appinfo, "frob", NULL);
      current_state = 3; return; case 3: g_assert_cmpstr (action, ==, "frob");

      xdesktop_app_info_launch_action (appinfo, "tweak", NULL);
      current_state = 4; return; case 4: g_assert_cmpstr (action, ==, "tweak");

      xdesktop_app_info_launch_action (appinfo, "twiddle", NULL);
      current_state = 5; return; case 5: g_assert_cmpstr (action, ==, "twiddle");

      /* Now launch the app with startup notification */
      {
        xapp_launch_context_t *ctx;

        g_assert (saw_startup_id == FALSE);
        ctx = xobject_new (test_app_launch_context_get_type (), NULL);
        xapp_info_launch (G_APP_INFO (appinfo), NULL, ctx, NULL);
        g_assert (requested_startup_id);
        requested_startup_id = FALSE;
        xobject_unref (ctx);
      }
      current_state = 6; return; case 6: g_assert_cmpstr (action, ==, "activate"); g_assert (saw_startup_id);
      saw_startup_id = FALSE;

      /* Now do the same for an action */
      {
        xapp_launch_context_t *ctx;

        g_assert (saw_startup_id == FALSE);
        ctx = xobject_new (test_app_launch_context_get_type (), NULL);
        xdesktop_app_info_launch_action (appinfo, "frob", ctx);
        g_assert (requested_startup_id);
        requested_startup_id = FALSE;
        xobject_unref (ctx);
      }
      current_state = 7; return; case 7: g_assert_cmpstr (action, ==, "frob"); g_assert (saw_startup_id);
      saw_startup_id = FALSE;

      /* Now quit... */
      xdesktop_app_info_launch_action (appinfo, "quit", NULL);
      current_state = 8; return; case 8: g_assert_not_reached ();
    }
}

static void
test_application_frob (xsimple_action_t *action,
                       xvariant_t      *parameter,
                       xpointer_t       user_data)
{
  g_assert (parameter == NULL);
  saw_action ("frob");
}

static void
test_application_tweak (xsimple_action_t *action,
                        xvariant_t      *parameter,
                        xpointer_t       user_data)
{
  g_assert (parameter == NULL);
  saw_action ("tweak");
}

static void
test_application_twiddle (xsimple_action_t *action,
                          xvariant_t      *parameter,
                          xpointer_t       user_data)
{
  g_assert (parameter == NULL);
  saw_action ("twiddle");
}

static void
test_application_quit (xsimple_action_t *action,
                       xvariant_t      *parameter,
                       xpointer_t       user_data)
{
  xapplication_t *application = user_data;

  xapplication_quit (application);
}

static const xaction_entry_t app_actions[] = {
  { "frob",         test_application_frob,    NULL, NULL, NULL, { 0 } },
  { "tweak",        test_application_tweak,   NULL, NULL, NULL, { 0 } },
  { "twiddle",      test_application_twiddle, NULL, NULL, NULL, { 0 } },
  { "quit",         test_application_quit,    NULL, NULL, NULL, { 0 } }
};

static void
test_application_activate (xapplication_t *application)
{
  /* Unbalanced, but that's OK because we will quit() */
  xapplication_hold (application);

  saw_action ("activate");
}

static void
test_application_open (xapplication_t  *application,
                       xfile_t        **files,
                       xint_t           n_files,
                       const xchar_t   *hint)
{
  xfile_t *f;

  g_assert_cmpstr (hint, ==, "");

  g_assert_cmpint (n_files, ==, 2);
  f = xfile_new_for_uri ("file:///a/b");
  g_assert (xfile_equal (files[0], f));
  xobject_unref (f);
  f = xfile_new_for_uri ("file:///c/d");
  g_assert (xfile_equal (files[1], f));
  xobject_unref (f);

  saw_action ("open");
}

static void
test_application_startup (xapplication_t *application)
{
  G_APPLICATION_CLASS (test_application_parent_class)
    ->startup (application);

  xaction_map_add_action_entries (G_ACTION_MAP (application), app_actions, G_N_ELEMENTS (app_actions), application);
}

static void
test_application_before_emit (xapplication_t *application,
                              xvariant_t     *platform_data)
{
  const xchar_t *startup_id;

  g_assert (!saw_startup_id);

  if (!xvariant_lookup (platform_data, "desktop-startup-id", "&s", &startup_id))
    return;

  g_assert_cmpstr (startup_id, ==, "expected startup id");
  saw_startup_id = TRUE;
}

static void
test_application_init (TestApplication *app)
{
}

static void
test_application_class_init (xapplication_class_t *class)
{
  class->before_emit = test_application_before_emit;
  class->startup = test_application_startup;
  class->activate = test_application_activate;
  class->open = test_application_open;
}

static void
test_dbus_appinfo (void)
{
  const xchar_t *argv[] = { "myapp", NULL };
  TestApplication *app;
  int status;
  xchar_t *desktop_file = NULL;

  desktop_file = g_test_build_filename (G_TEST_DIST,
                                        "org.gtk.test.dbusappinfo.desktop",
                                        NULL);
  appinfo = xdesktop_app_info_new_from_filename (desktop_file);
  g_assert (appinfo != NULL);
  g_free (desktop_file);

  app = xobject_new (test_application_get_type (),
                      "application-id", "org.gtk.test.dbusappinfo",
                      "flags", G_APPLICATION_HANDLES_OPEN,
                      NULL);
  status = xapplication_run (app, 1, (xchar_t **) argv);

  g_assert_cmpint (status, ==, 0);
  g_assert_cmpint (current_state, ==, 8);

  xobject_unref (appinfo);
  xobject_unref (app);
}

static void
on_flatpak_launch_uris_finish (xobject_t *object,
                               xasync_result_t *result,
                               xpointer_t user_data)
{
  xapplication_t *app = user_data;
  xerror_t *error = NULL;

  xapp_info_launch_uris_finish (G_APP_INFO (object), result, &error);
  g_assert_no_error (error);

  xapplication_release (app);
}

static void
on_flatpak_activate (xapplication_t *app,
                     xpointer_t user_data)
{
  xdesktop_app_info_t *flatpak_appinfo = user_data;
  char *uri;
  xlist_t *uris;

  /* The app will be released in on_flatpak_launch_uris_finish */
  xapplication_hold (app);

  uri = xfilename_to_uri (xdesktop_app_info_get_filename (flatpak_appinfo), NULL, NULL);
  g_assert_nonnull (uri);
  uris = xlist_prepend (NULL, uri);
  xapp_info_launch_uris_async (G_APP_INFO (flatpak_appinfo), uris, NULL,
                                NULL, on_flatpak_launch_uris_finish, app);
  xlist_free (uris);
  g_free (uri);
}

static void
on_flatpak_open (xapplication_t  *app,
                 xfile_t        **files,
                 xint_t           n_files,
                 const char    *hint)
{
  xfile_t *f;

  g_assert_cmpint (n_files, ==, 1);
  g_test_message ("on_flatpak_open received file '%s'", xfile_peek_path (files[0]));

  /* The file has been exported via the document portal */
  f = xfile_new_for_uri ("file:///document-portal/document-id/org.gtk.test.dbusappinfo.flatpak.desktop");
  g_assert_true (xfile_equal (files[0], f));
  xobject_unref (f);
}

static void
test_flatpak_doc_export (void)
{
  const xchar_t *argv[] = { "myapp", NULL };
  xchar_t *desktop_file = NULL;
  xdesktop_app_info_t *flatpak_appinfo;
  xapplication_t *app;
  int status;

  g_test_summary ("test_t that files launched via Flatpak apps are made available via the document portal.");

  desktop_file = g_test_build_filename (G_TEST_DIST,
                                        "org.gtk.test.dbusappinfo.flatpak.desktop",
                                        NULL);
  flatpak_appinfo = xdesktop_app_info_new_from_filename (desktop_file);
  g_assert_nonnull (flatpak_appinfo);
  g_free (desktop_file);

  app = xapplication_new ("org.gtk.test.dbusappinfo.flatpak",
                           G_APPLICATION_HANDLES_OPEN);
  xsignal_connect (app, "activate", G_CALLBACK (on_flatpak_activate),
                    flatpak_appinfo);
  xsignal_connect (app, "open", G_CALLBACK (on_flatpak_open), NULL);

  status = xapplication_run (app, 1, (xchar_t **) argv);
  g_assert_cmpint (status, ==, 0);

  xobject_unref (app);
  xobject_unref (flatpak_appinfo);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/appinfo/dbusappinfo", test_dbus_appinfo);
  g_test_add_func ("/appinfo/flatpak-doc-export", test_flatpak_doc_export);

  return session_bus_run ();
}
