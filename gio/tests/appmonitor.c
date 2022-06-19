#include <gio/gio.h>
#include <gstdio.h>

typedef struct
{
  xchar_t *applications_dir;
} Fixture;

static void
setup (Fixture       *fixture,
       xconstpointer  user_data)
{
  fixture->applications_dir = g_build_filename (g_get_user_data_dir (), "applications", NULL);
  g_assert_no_errno (g_mkdir_with_parents (fixture->applications_dir, 0755));

  g_test_message ("Using data directory: %s", g_get_user_data_dir ());
}

static void
teardown (Fixture       *fixture,
          xconstpointer  user_data)
{
  g_assert_no_errno (g_rmdir (fixture->applications_dir));
  g_clear_pointer (&fixture->applications_dir, g_free);
}

static xboolean_t
create_app (xpointer_t data)
{
  const xchar_t *path = data;
  xerror_t *error = NULL;
  const xchar_t *contents =
    "[Desktop Entry]\n"
    "Name=Application\n"
    "Version=1.0\n"
    "Type=Application\n"
    "Exec=true\n";

  xfile_set_contents (path, contents, -1, &error);
  g_assert_no_error (error);

  return XSOURCE_REMOVE;
}

static void
delete_app (xpointer_t data)
{
  const xchar_t *path = data;

  g_remove (path);
}

static xboolean_t changed_fired;

static void
changed_cb (xapp_info_monitor_t *monitor, xmain_loop_t *loop)
{
  changed_fired = TRUE;
  xmain_loop_quit (loop);
}

static xboolean_t
quit_loop (xpointer_t data)
{
  xmain_loop_t *loop = data;

  if (xmain_loop_is_running (loop))
    xmain_loop_quit (loop);

  return XSOURCE_REMOVE;
}

static void
test_app_monitor (Fixture       *fixture,
                  xconstpointer  user_data)
{
  xchar_t *app_path;
  xapp_info_monitor_t *monitor;
  xmain_loop_t *loop;

  app_path = g_build_filename (fixture->applications_dir, "app.desktop", NULL);

  /* FIXME: this shouldn't be required */
  xlist_free_full (xapp_info_get_all (), xobject_unref);

  monitor = xapp_info_monitor_get ();
  loop = xmain_loop_new (NULL, FALSE);

  xsignal_connect (monitor, "changed", G_CALLBACK (changed_cb), loop);

  g_idle_add (create_app, app_path);
  g_timeout_add_seconds (3, quit_loop, loop);

  xmain_loop_run (loop);
  xassert (changed_fired);
  changed_fired = FALSE;

  /* FIXME: this shouldn't be required */
  xlist_free_full (xapp_info_get_all (), xobject_unref);

  g_timeout_add_seconds (3, quit_loop, loop);

  delete_app (app_path);

  xmain_loop_run (loop);

  xassert (changed_fired);

  xmain_loop_unref (loop);
  g_remove (app_path);

  xobject_unref (monitor);

  g_free (app_path);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add ("/monitor/app", Fixture, NULL, setup, test_app_monitor, teardown);

  return g_test_run ();
}
