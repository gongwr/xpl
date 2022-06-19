#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gdbus-tests.h"
#include "gdbus-sessionbus.h"

#if 0
/* These tests are racy -- there is no guarantee about the order of data
 * arriving over D-Bus.
 *
 * They're also a bit ridiculous -- xapplication_t was never meant to be
 * abused in this way...
 *
 * We need new tests.
 */
static xint_t outstanding_watches;
static xmain_loop_t *main_loop;

typedef struct
{
  xchar_t *expected_stdout;
  xint_t stdout_pipe;
  xchar_t *expected_stderr;
  xint_t stderr_pipe;
} ChildData;

static void
check_data (xint_t fd, const xchar_t *expected)
{
  xssize_t len, actual;
  xchar_t *buffer;

  len = strlen (expected);
  buffer = g_alloca (len + 100);
  actual = read (fd, buffer, len + 100);

  g_assert_cmpint (actual, >=, 0);

  if (actual != len ||
      memcmp (buffer, expected, len) != 0)
    {
      buffer[MIN(len + 100, actual)] = '\0';

      xerror ("\nExpected\n-----\n%s-----\nGot (%s)\n-----\n%s-----\n",
               expected,
               (actual > len) ? "truncated" : "full", buffer);
    }
}

static void
child_quit (xpid_t     pid,
            xint_t     status,
            xpointer_t data)
{
  ChildData *child = data;

  g_assert_cmpint (status, ==, 0);

  if (--outstanding_watches == 0)
    xmain_loop_quit (main_loop);

  check_data (child->stdout_pipe, child->expected_stdout);
  close (child->stdout_pipe);
  g_free (child->expected_stdout);

  if (child->expected_stderr)
    {
      check_data (child->stderr_pipe, child->expected_stderr);
      close (child->stderr_pipe);
      g_free (child->expected_stderr);
    }

  g_slice_free (ChildData, child);
}

static void
spawn (const xchar_t *expected_stdout,
       const xchar_t *expected_stderr,
       const xchar_t *first_arg,
       ...)
{
  xerror_t *error = NULL;
  const xchar_t *arg;
  xptr_array_t *array;
  ChildData *data;
  xchar_t **args;
  va_list ap;
  xpid_t pid;
  xpollfd_t fd;
  xchar_t **env;

  va_start (ap, first_arg);
  array = xptr_array_new ();
  xptr_array_add (array, g_test_build_filename (G_TEST_BUILT, "basic-application", NULL));
  for (arg = first_arg; arg; arg = va_arg (ap, const xchar_t *))
    xptr_array_add (array, xstrdup (arg));
  xptr_array_add (array, NULL);
  args = (xchar_t **) xptr_array_free (array, FALSE);
  va_end (ap);

  env = g_environ_setenv (g_get_environ (), "TEST", "1", TRUE);

  data = g_slice_new (ChildData);
  data->expected_stdout = xstrdup (expected_stdout);
  data->expected_stderr = xstrdup (expected_stderr);

  g_spawn_async_with_pipes (NULL, args, env,
                            G_SPAWN_DO_NOT_REAP_CHILD,
                            NULL, NULL, &pid, NULL,
                            &data->stdout_pipe,
                            expected_stderr ? &data->stderr_pipe : NULL,
                            &error);
  g_assert_no_error (error);

  xstrfreev (env);

  g_child_watch_add (pid, child_quit, data);
  outstanding_watches++;

  /* we block until the children write to stdout to make sure
   * they have started, as they need to be executed in order;
   * see https://bugzilla.gnome.org/show_bug.cgi?id=664627
   */
  fd.fd = data->stdout_pipe;
  fd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
  g_poll (&fd, 1, -1);
}

static void
basic (void)
{
  xdbus_connection_t *c;

  xassert (outstanding_watches == 0);

  session_bus_up ();
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  main_loop = xmain_loop_new (NULL, 0);

  /* spawn the main instance */
  spawn ("activated\n"
         "open file:///a file:///b\n"
         "exit status: 0\n", NULL,
         "./app", NULL);

  /* send it some files */
  spawn ("exit status: 0\n", NULL,
         "./app", "/a", "/b", NULL);

  xmain_loop_run (main_loop);

  xobject_unref (c);
  session_bus_down ();

  xmain_loop_unref (main_loop);
}

static void
test_remote_command_line (void)
{
  xdbus_connection_t *c;
  xfile_t *file;
  xchar_t *replies;
  xchar_t *cwd;

  xassert (outstanding_watches == 0);

  session_bus_up ();
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  main_loop = xmain_loop_new (NULL, 0);

  file = xfile_new_for_commandline_arg ("foo");
  cwd = g_get_current_dir ();

  replies = xstrconcat ("got ./cmd 0\n",
                         "got ./cmd 1\n",
                         "cmdline ./cmd echo --abc -d\n",
                         "environment TEST=1\n",
                         "getenv TEST=1\n",
                         "file ", xfile_get_path (file), "\n",
                         "properties ok\n",
                         "cwd ", cwd, "\n",
                         "busy\n",
                         "idle\n",
                         "stdin ok\n",
                         "exit status: 0\n",
                         NULL);
  xobject_unref (file);

  /* spawn the main instance */
  spawn (replies, NULL,
         "./cmd", NULL);

  g_free (replies);

  /* send it a few commandlines */
  spawn ("exit status: 0\n", NULL,
         "./cmd", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "echo", "--abc", "-d", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "env", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "getenv", NULL);

  spawn ("print test\n"
         "exit status: 0\n", NULL,
         "./cmd", "print", "test", NULL);

  spawn ("exit status: 0\n", "printerr test\n",
         "./cmd", "printerr", "test", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "file", "foo", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "properties", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "cwd", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "busy", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "idle", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "stdin", NULL);

  xmain_loop_run (main_loop);

  xobject_unref (c);
  session_bus_down ();

  xmain_loop_unref (main_loop);
}

static void
test_remote_actions (void)
{
  xdbus_connection_t *c;

  xassert (outstanding_watches == 0);

  session_bus_up ();
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  main_loop = xmain_loop_new (NULL, 0);

  /* spawn the main instance */
  spawn ("got ./cmd 0\n"
         "activate action1\n"
         "change action2 1\n"
         "exit status: 0\n", NULL,
         "./cmd", NULL);

  spawn ("actions quit new action1 action2\n"
         "exit status: 0\n", NULL,
         "./actions", "list", NULL);

  spawn ("exit status: 0\n", NULL,
         "./actions", "activate", NULL);

  spawn ("exit status: 0\n", NULL,
         "./actions", "set-state", NULL);

  xmain_loop_run (main_loop);

  xobject_unref (c);
  session_bus_down ();

  xmain_loop_unref (main_loop);
}
#endif

#if 0
/* Now that we register non-unique apps on the bus we need to fix the
 * following test not to assume that it's safe to create multiple instances
 * of the same app in one process.
 *
 * See https://bugzilla.gnome.org/show_bug.cgi?id=647986 for the patch that
 * introduced this problem.
 */

static xapplication_t *recently_activated;
static xmain_loop_t *loop;

static void
nonunique_activate (xapplication_t *application)
{
  recently_activated = application;

  if (loop != NULL)
    xmain_loop_quit (loop);
}

static xapplication_t *
make_app (xboolean_t non_unique)
{
  xapplication_t *app;
  xboolean_t ok;

  app = xapplication_new ("org.gtk.test_t-Application",
                           non_unique ? G_APPLICATION_NON_UNIQUE : 0);
  xsignal_connect (app, "activate", G_CALLBACK (nonunique_activate), NULL);
  ok = xapplication_register (app, NULL, NULL);
  if (!ok)
    {
      xobject_unref (app);
      return NULL;
    }

  xapplication_activate (app);

  return app;
}

static void
test_nonunique (void)
{
  xapplication_t *first, *second, *third, *fourth;

  session_bus_up ();

  first = make_app (TRUE);
  /* non-remote because it is non-unique */
  xassert (!xapplication_get_is_remote (first));
  xassert (recently_activated == first);
  recently_activated = NULL;

  second = make_app (FALSE);
  /* non-remote because it is first */
  xassert (!xapplication_get_is_remote (second));
  xassert (recently_activated == second);
  recently_activated = NULL;

  third = make_app (TRUE);
  /* non-remote because it is non-unique */
  xassert (!xapplication_get_is_remote (third));
  xassert (recently_activated == third);
  recently_activated = NULL;

  fourth = make_app (FALSE);
  /* should have failed to register due to being
   * unable to register the object paths
   */
  xassert (fourth == NULL);
  xassert (recently_activated == NULL);

  xobject_unref (first);
  xobject_unref (second);
  xobject_unref (third);

  session_bus_down ();
}
#endif

static void
properties (void)
{
  xdbus_connection_t *c;
  xobject_t *app;
  xchar_t *id;
  GApplicationFlags flags;
  xboolean_t registered;
  xuint_t timeout;
  xboolean_t remote;
  xboolean_t ret;
  xerror_t *error = NULL;

  session_bus_up ();
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  app = xobject_new (XTYPE_APPLICATION,
                      "application-id", "org.gtk.test_application_t",
                      NULL);

  xobject_get (app,
                "application-id", &id,
                "flags", &flags,
                "is-registered", &registered,
                "inactivity-timeout", &timeout,
                NULL);

  g_assert_cmpstr (id, ==, "org.gtk.test_application_t");
  g_assert_cmpint (flags, ==, G_APPLICATION_FLAGS_NONE);
  xassert (!registered);
  g_assert_cmpint (timeout, ==, 0);

  ret = xapplication_register (G_APPLICATION (app), NULL, &error);
  xassert (ret);
  g_assert_no_error (error);

  xobject_get (app,
                "is-registered", &registered,
                "is-remote", &remote,
                NULL);

  xassert (registered);
  xassert (!remote);

  xobject_set (app,
                "inactivity-timeout", 1000,
                NULL);

  xapplication_quit (G_APPLICATION (app));

  xobject_unref (c);
  xobject_unref (app);
  g_free (id);

  session_bus_down ();
}

static void
appid (void)
{
  xchar_t *id;

  g_assert_false (xapplication_id_is_valid (""));
  g_assert_false (xapplication_id_is_valid ("."));
  g_assert_false (xapplication_id_is_valid ("a"));
  g_assert_false (xapplication_id_is_valid ("abc"));
  g_assert_false (xapplication_id_is_valid (".abc"));
  g_assert_false (xapplication_id_is_valid ("abc."));
  g_assert_false (xapplication_id_is_valid ("a..b"));
  g_assert_false (xapplication_id_is_valid ("a/b"));
  g_assert_false (xapplication_id_is_valid ("a\nb"));
  g_assert_false (xapplication_id_is_valid ("a\nb"));
  g_assert_false (xapplication_id_is_valid ("emoji_picker"));
  g_assert_false (xapplication_id_is_valid ("emoji-picker"));
  g_assert_false (xapplication_id_is_valid ("emojipicker"));
  g_assert_false (xapplication_id_is_valid ("my.Terminal.0123"));
  id = g_new0 (xchar_t, 261);
  memset (id, 'a', 260);
  id[1] = '.';
  id[260] = 0;
  g_assert_false (xapplication_id_is_valid (id));
  g_free (id);

  g_assert_true (xapplication_id_is_valid ("a.b"));
  g_assert_true (xapplication_id_is_valid ("A.B"));
  g_assert_true (xapplication_id_is_valid ("A-.B"));
  g_assert_true (xapplication_id_is_valid ("a_b.c-d"));
  g_assert_true (xapplication_id_is_valid ("_a.b"));
  g_assert_true (xapplication_id_is_valid ("-a.b"));
  g_assert_true (xapplication_id_is_valid ("org.gnome.SessionManager"));
  g_assert_true (xapplication_id_is_valid ("my.Terminal._0123"));
  g_assert_true (xapplication_id_is_valid ("com.example.MyApp"));
  g_assert_true (xapplication_id_is_valid ("com.example.internal_apps.Calculator"));
  g_assert_true (xapplication_id_is_valid ("org._7_zip.Archiver"));
}

static xboolean_t nodbus_activated;

static xboolean_t
release_app (xpointer_t user_data)
{
  xapplication_release (user_data);
  return XSOURCE_REMOVE;
}

static void
nodbus_activate (xapplication_t *app)
{
  nodbus_activated = TRUE;
  xapplication_hold (app);

  xassert (xapplication_get_dbus_connection (app) == NULL);
  xassert (xapplication_get_dbus_object_path (app) == NULL);

  g_idle_add (release_app, app);
}

static void
test_nodbus (void)
{
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  xchar_t *argv[] = { binpath, NULL };
  xapplication_t *app;

  app = xapplication_new ("org.gtk.Unimportant", G_APPLICATION_FLAGS_NONE);
  xsignal_connect (app, "activate", G_CALLBACK (nodbus_activate), NULL);
  xapplication_run (app, 1, argv);
  xobject_unref (app);

  xassert (nodbus_activated);
  g_free (binpath);
}

static xboolean_t noappid_activated;

static void
noappid_activate (xapplication_t *app)
{
  noappid_activated = TRUE;
  xapplication_hold (app);

  xassert (xapplication_get_flags (app) & G_APPLICATION_NON_UNIQUE);

  g_idle_add (release_app, app);
}

/* test that no appid -> non-unique */
static void
test_noappid (void)
{
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  xchar_t *argv[] = { binpath, NULL };
  xapplication_t *app;

  app = xapplication_new (NULL, G_APPLICATION_FLAGS_NONE);
  xsignal_connect (app, "activate", G_CALLBACK (noappid_activate), NULL);
  xapplication_run (app, 1, argv);
  xobject_unref (app);

  xassert (noappid_activated);
  g_free (binpath);
}

static xboolean_t activated;
static xboolean_t quitted;

static xboolean_t
quit_app (xpointer_t user_data)
{
  quitted = TRUE;
  xapplication_quit (user_data);
  return XSOURCE_REMOVE;
}

static void
quit_activate (xapplication_t *app)
{
  activated = TRUE;
  xapplication_hold (app);

  xassert (xapplication_get_dbus_connection (app) != NULL);
  xassert (xapplication_get_dbus_object_path (app) != NULL);

  g_idle_add (quit_app, app);
}

static void
test_quit (void)
{
  xdbus_connection_t *c;
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  xchar_t *argv[] = { binpath, NULL };
  xapplication_t *app;

  session_bus_up ();
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  app = xapplication_new ("org.gtk.Unimportant",
                           G_APPLICATION_FLAGS_NONE);
  activated = FALSE;
  quitted = FALSE;
  xsignal_connect (app, "activate", G_CALLBACK (quit_activate), NULL);
  xapplication_run (app, 1, argv);
  xobject_unref (app);
  xobject_unref (c);

  xassert (activated);
  xassert (quitted);

  session_bus_down ();
  g_free (binpath);
}

typedef struct
{
  xboolean_t shutdown;
  xparam_spec_t *notify_spec; /* (owned) (nullable) */
} RegisteredData;

static void
on_registered_shutdown (xapplication_t *app,
                        xpointer_t user_data)
{
  RegisteredData *registered_data = user_data;

  registered_data->shutdown = TRUE;
}

static void
on_registered_notify (xapplication_t *app,
                      xparam_spec_t *spec,
                      xpointer_t user_data)
{
  RegisteredData *registered_data = user_data;
  registered_data->notify_spec = xparam_spec_ref (spec);

  if (xapplication_get_is_registered (app))
    g_assert_false (registered_data->shutdown);
  else
    g_assert_true (registered_data->shutdown);
}

static void
test_registered (void)
{
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  xchar_t *argv[] = { binpath, NULL };
  RegisteredData registered_data = { FALSE, NULL };
  xapplication_t *app;

  app = xapplication_new (NULL, G_APPLICATION_FLAGS_NONE);
  xsignal_connect (app, "activate", G_CALLBACK (noappid_activate), NULL);
  xsignal_connect (app, "shutdown", G_CALLBACK (on_registered_shutdown), &registered_data);
  xsignal_connect (app, "notify::is-registered", G_CALLBACK (on_registered_notify), &registered_data);

  g_assert_null (registered_data.notify_spec);

  g_assert_true (xapplication_register (app, NULL, NULL));
  g_assert_true (xapplication_get_is_registered (app));

  g_assert_nonnull (registered_data.notify_spec);
  g_assert_cmpstr (registered_data.notify_spec->name, ==, "is-registered");
  g_clear_pointer (&registered_data.notify_spec, xparam_spec_unref);

  g_assert_false (registered_data.shutdown);

  xapplication_run (app, 1, argv);

  g_assert_true (registered_data.shutdown);
  g_assert_false (xapplication_get_is_registered (app));
  g_assert_nonnull (registered_data.notify_spec);
  g_assert_cmpstr (registered_data.notify_spec->name, ==, "is-registered");
  g_clear_pointer (&registered_data.notify_spec, xparam_spec_unref);

  /* Register it again */
  registered_data.shutdown = FALSE;
  g_assert_true (xapplication_register (app, NULL, NULL));
  g_assert_true (xapplication_get_is_registered (app));
  g_assert_nonnull (registered_data.notify_spec);
  g_assert_cmpstr (registered_data.notify_spec->name, ==, "is-registered");
  g_clear_pointer (&registered_data.notify_spec, xparam_spec_unref);
  g_assert_false (registered_data.shutdown);

  xobject_unref (app);

  g_free (binpath);
}

static void
on_activate (xapplication_t *app)
{
  xchar_t **actions;
  xaction_t *action;
  xvariant_t *state;

  xassert (!xapplication_get_is_remote (app));

  actions = xaction_group_list_actions (XACTION_GROUP (app));
  xassert (xstrv_length (actions) == 0);
  xstrfreev (actions);

  action = (xaction_t*)g_simple_action_new_stateful ("test", G_VARIANT_TYPE_BOOLEAN, xvariant_new_boolean (FALSE));
  xaction_map_add_action (G_ACTION_MAP (app), action);

  actions = xaction_group_list_actions (XACTION_GROUP (app));
  xassert (xstrv_length (actions) == 1);
  xstrfreev (actions);

  xaction_group_change_action_state (XACTION_GROUP (app), "test", xvariant_new_boolean (TRUE));
  state = xaction_group_get_action_state (XACTION_GROUP (app), "test");
  xassert (xvariant_get_boolean (state) == TRUE);

  action = xaction_map_lookup_action (G_ACTION_MAP (app), "test");
  xassert (action != NULL);

  xaction_map_remove_action (G_ACTION_MAP (app), "test");

  actions = xaction_group_list_actions (XACTION_GROUP (app));
  xassert (xstrv_length (actions) == 0);
  xstrfreev (actions);
}

static void
test_local_actions (void)
{
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  xchar_t *argv[] = { binpath, NULL };
  xapplication_t *app;

  app = xapplication_new ("org.gtk.Unimportant",
                           G_APPLICATION_FLAGS_NONE);
  xsignal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  xapplication_run (app, 1, argv);
  xobject_unref (app);
  g_free (binpath);
}

typedef xapplication_t TestLocCmdApp;
typedef xapplication_class_t TestLocCmdAppClass;

static xtype_t test_loc_cmd_app_get_type (void);
XDEFINE_TYPE (TestLocCmdApp, test_loc_cmd_app, XTYPE_APPLICATION)

static void
test_loc_cmd_app_init (TestLocCmdApp *app)
{
}

static void
test_loc_cmd_app_startup (xapplication_t *app)
{
  g_assert_not_reached ();
}

static void
test_loc_cmd_app_shutdown (xapplication_t *app)
{
  g_assert_not_reached ();
}

static xboolean_t
test_loc_cmd_app_local_command_line (xapplication_t   *application,
                                     xchar_t        ***arguments,
                                     xint_t           *exit_status)
{
  return TRUE;
}

static void
test_loc_cmd_app_class_init (TestLocCmdAppClass *klass)
{
  G_APPLICATION_CLASS (klass)->startup = test_loc_cmd_app_startup;
  G_APPLICATION_CLASS (klass)->shutdown = test_loc_cmd_app_shutdown;
  G_APPLICATION_CLASS (klass)->local_command_line = test_loc_cmd_app_local_command_line;
}

static void
test_local_command_line (void)
{
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  xchar_t *argv[] = { binpath, "-invalid", NULL };
  xapplication_t *app;

  app = xobject_new (test_loc_cmd_app_get_type (),
                      "application-id", "org.gtk.Unimportant",
                      "flags", G_APPLICATION_FLAGS_NONE,
                      NULL);
  xapplication_run (app, 1, argv);
  xobject_unref (app);
  g_free (binpath);
}

static void
test_resource_path (void)
{
  xapplication_t *app;

  app = xapplication_new ("x.y.z", 0);
  g_assert_cmpstr (xapplication_get_resource_base_path (app), ==, "/x/y/z");

  /* this should not change anything */
  xapplication_set_application_id (app, "a.b.c");
  g_assert_cmpstr (xapplication_get_resource_base_path (app), ==, "/x/y/z");

  /* but this should... */
  xapplication_set_resource_base_path (app, "/x");
  g_assert_cmpstr (xapplication_get_resource_base_path (app), ==, "/x");

  /* ... and this */
  xapplication_set_resource_base_path (app, NULL);
  g_assert_cmpstr (xapplication_get_resource_base_path (app), ==, NULL);

  xobject_unref (app);

  /* Make sure that overriding at construction time works properly */
  app = xobject_new (XTYPE_APPLICATION, "application-id", "x.y.z", "resource-base-path", "/a", NULL);
  g_assert_cmpstr (xapplication_get_resource_base_path (app), ==, "/a");
  xobject_unref (app);

  /* ... particularly if we override to NULL */
  app = xobject_new (XTYPE_APPLICATION, "application-id", "x.y.z", "resource-base-path", NULL, NULL);
  g_assert_cmpstr (xapplication_get_resource_base_path (app), ==, NULL);
  xobject_unref (app);
}

static xint_t
test_help_command_line (xapplication_t            *app,
                        xapplication_command_line_t *command_line,
                        xpointer_t                 user_data)
{
  xboolean_t *called = user_data;

  *called = TRUE;

  return 0;
}

/* test_t whether --help is handled when HANDLES_COMMND_LINE is set and
 * options have been added.
 */
static void
test_help (void)
{
  if (g_test_subprocess ())
    {
      char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
      xchar_t *argv[] = { binpath, "--help", NULL };
      xapplication_t *app;
      xboolean_t called = FALSE;
      int status;

      app = xapplication_new ("org.gtk.test_application_t", G_APPLICATION_HANDLES_COMMAND_LINE);
      xapplication_add_main_option (app, "foo", 'f', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      xsignal_connect (app, "command-line", G_CALLBACK (test_help_command_line), &called);

      status = xapplication_run (app, G_N_ELEMENTS (argv) -1, argv);
      xassert (called == TRUE);
      g_assert_cmpint (status, ==, 0);

      xobject_unref (app);
      g_free (binpath);
      return;
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*Application options*");
}

static void
test_busy (void)
{
  xapplication_t *app;

  /* use xsimple_action_t to bind to the busy state, because it's easy to
   * create and has an easily modifiable boolean property */
  xsimple_action_t *action1;
  xsimple_action_t *action2;

  session_bus_up ();

  app = xapplication_new ("org.gtk.test_application_t", G_APPLICATION_NON_UNIQUE);
  xassert (xapplication_register (app, NULL, NULL));

  xassert (!xapplication_get_is_busy (app));
  xapplication_mark_busy (app);
  xassert (xapplication_get_is_busy (app));
  xapplication_unmark_busy (app);
  xassert (!xapplication_get_is_busy (app));

  action1 = g_simple_action_new ("action", NULL);
  xapplication_bind_busy_property (app, action1, "enabled");
  xassert (xapplication_get_is_busy (app));

  g_simple_action_set_enabled (action1, FALSE);
  xassert (!xapplication_get_is_busy (app));

  xapplication_mark_busy (app);
  xassert (xapplication_get_is_busy (app));

  action2 = g_simple_action_new ("action", NULL);
  xapplication_bind_busy_property (app, action2, "enabled");
  xassert (xapplication_get_is_busy (app));

  xapplication_unmark_busy (app);
  xassert (xapplication_get_is_busy (app));

  xobject_unref (action2);
  xassert (!xapplication_get_is_busy (app));

  g_simple_action_set_enabled (action1, TRUE);
  xassert (xapplication_get_is_busy (app));

  xapplication_mark_busy (app);
  xassert (xapplication_get_is_busy (app));

  xapplication_unbind_busy_property (app, action1, "enabled");
  xassert (xapplication_get_is_busy (app));

  xapplication_unmark_busy (app);
  xassert (!xapplication_get_is_busy (app));

  xobject_unref (action1);
  xobject_unref (app);

  session_bus_down ();
}

/*
 * test_t that handle-local-options works as expected
 */

static xint_t
test_local_options (xapplication_t *app,
                    xvariant_dict_t *options,
                    xpointer_t      data)
{
  xboolean_t *called = data;

  *called = TRUE;

  if (xvariant_dict_contains (options, "success"))
    return 0;
  else if (xvariant_dict_contains (options, "failure"))
    return 1;
  else
    return -1;
}

static xint_t
second_handler (xapplication_t *app,
                xvariant_dict_t *options,
                xpointer_t      data)
{
  xboolean_t *called = data;

  *called = TRUE;

  return 2;
}

static void
test_handle_local_options_success (void)
{
  if (g_test_subprocess ())
    {
      char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
      xchar_t *argv[] = { binpath, "--success", NULL };
      xapplication_t *app;
      xboolean_t called = FALSE;
      xboolean_t called2 = FALSE;
      int status;

      app = xapplication_new ("org.gtk.test_application_t", 0);
      xapplication_add_main_option (app, "success", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      xapplication_add_main_option (app, "failure", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      xsignal_connect (app, "handle-local-options", G_CALLBACK (test_local_options), &called);
      xsignal_connect (app, "handle-local-options", G_CALLBACK (second_handler), &called2);

      status = xapplication_run (app, G_N_ELEMENTS (argv) -1, argv);
      xassert (called);
      xassert (!called2);
      g_assert_cmpint (status, ==, 0);

      xobject_unref (app);
      g_free (binpath);
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_INHERIT_STDOUT | G_TEST_SUBPROCESS_INHERIT_STDERR);
  g_test_trap_assert_passed ();
}

static void
test_handle_local_options_failure (void)
{
  if (g_test_subprocess ())
    {
      char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
      xchar_t *argv[] = { binpath, "--failure", NULL };
      xapplication_t *app;
      xboolean_t called = FALSE;
      xboolean_t called2 = FALSE;
      int status;

      app = xapplication_new ("org.gtk.test_application_t", 0);
      xapplication_add_main_option (app, "success", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      xapplication_add_main_option (app, "failure", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      xsignal_connect (app, "handle-local-options", G_CALLBACK (test_local_options), &called);
      xsignal_connect (app, "handle-local-options", G_CALLBACK (second_handler), &called2);

      status = xapplication_run (app, G_N_ELEMENTS (argv) -1, argv);
      xassert (called);
      xassert (!called2);
      g_assert_cmpint (status, ==, 1);

      xobject_unref (app);
      g_free (binpath);
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_INHERIT_STDOUT | G_TEST_SUBPROCESS_INHERIT_STDERR);
  g_test_trap_assert_passed ();
}

static void
test_handle_local_options_passthrough (void)
{
  if (g_test_subprocess ())
    {
      char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
      xchar_t *argv[] = { binpath, NULL };
      xapplication_t *app;
      xboolean_t called = FALSE;
      xboolean_t called2 = FALSE;
      int status;

      app = xapplication_new ("org.gtk.test_application_t", 0);
      xapplication_add_main_option (app, "success", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      xapplication_add_main_option (app, "failure", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      xsignal_connect (app, "handle-local-options", G_CALLBACK (test_local_options), &called);
      xsignal_connect (app, "handle-local-options", G_CALLBACK (second_handler), &called2);

      status = xapplication_run (app, G_N_ELEMENTS (argv) -1, argv);
      xassert (called);
      xassert (called2);
      g_assert_cmpint (status, ==, 2);

      xobject_unref (app);
      g_free (binpath);
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_INHERIT_STDOUT | G_TEST_SUBPROCESS_INHERIT_STDERR);
  g_test_trap_assert_passed ();
}

static void
test_api (void)
{
  xapplication_t *app;
  xsimple_action_t *action;

  app = xapplication_new ("org.gtk.test_application_t", 0);

  /* add an action without a name */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*assertion*failed*");
  action = g_simple_action_new (NULL, NULL);
  xassert (action == NULL);
  g_test_assert_expected_messages ();

  /* also, gapplication shouldn't accept actions without names */
  action = xobject_new (XTYPE_SIMPLE_ACTION, NULL);
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*action has no name*");
  xaction_map_add_action (G_ACTION_MAP (app), G_ACTION (action));
  g_test_assert_expected_messages ();

  xobject_unref (action);
  xobject_unref (app);
}

/* Check that G_APPLICATION_ALLOW_REPLACEMENT works. To do so, we launch
 * a xapplication_t in this process that allows replacement, and then
 * launch a subprocess with --gapplication-replace. We have to do our
 * own async version of g_test_trap_subprocess() here since we need
 * the main process to keep spinning its mainloop.
 */

static xboolean_t
name_was_lost (xapplication_t *app,
               xboolean_t     *called)
{
  *called = TRUE;
  xapplication_quit (app);
  return TRUE;
}

static void
startup_in_subprocess (xapplication_t *app,
                       xboolean_t     *called)
{
  *called = TRUE;
}

typedef struct
{
  xboolean_t allow_replacement;
  xsubprocess_t *subprocess;
} TestReplaceData;

static void
startup_cb (xapplication_t *app,
            TestReplaceData *data)
{
  const char *argv[] = { NULL, "--verbose", "--quiet", "-p", NULL, "--GTestSubprocess", NULL };
  xsubprocess_launcher_t *launcher;
  xerror_t *local_error = NULL;

  xapplication_hold (app);

  argv[0] = g_get_prgname ();

  if (data->allow_replacement)
    argv[4] = "/gapplication/replace";
  else
    argv[4] = "/gapplication/no-replace";

  /* Now that we are the primary instance, launch our replacement.
   * We inherit the environment to share the test session bus.
   */
  g_test_message ("launching subprocess");

  launcher = xsubprocess_launcher_new (G_SUBPROCESS_FLAGS_NONE);
  xsubprocess_launcher_set_environ (launcher, NULL);
  data->subprocess = xsubprocess_launcher_spawnv (launcher, argv, &local_error);
  g_assert_no_error (local_error);
  xobject_unref (launcher);

  if (!data->allow_replacement)
    {
      /* make sure we exit after a bit, if the subprocess is not replacing us */
      xapplication_set_inactivity_timeout (app, 500);
      xapplication_release (app);
    }
}

static void
activate (xpointer_t data)
{
  /* xapplication_t complains if we don't connect to ::activate */
}

static xboolean_t
quit_already (xpointer_t data)
{
  xapplication_t *app = data;

  xapplication_quit (app);

  return XSOURCE_REMOVE;
}

static void
test_replace (xconstpointer data)
{
  xboolean_t allow = GPOINTER_TO_INT (data);

  if (g_test_subprocess ())
    {
      char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
      char *argv[] = { binpath, "--gapplication-replace", NULL };
      xapplication_t *app;
      xboolean_t startup = FALSE;

      app = xapplication_new ("org.gtk.test_application_t.Replace", G_APPLICATION_ALLOW_REPLACEMENT);
      xsignal_connect (app, "startup", G_CALLBACK (startup_in_subprocess), &startup);
      xsignal_connect (app, "activate", G_CALLBACK (activate), NULL);

      xapplication_run (app, G_N_ELEMENTS (argv) - 1, argv);

      if (allow)
        g_assert_true (startup);
      else
        g_assert_false (startup);

      xobject_unref (app);
      g_free (binpath);
    }
  else
    {
      char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
      xchar_t *argv[] = { binpath, NULL };
      xapplication_t *app;
      xboolean_t name_lost = FALSE;
      TestReplaceData data;
      xtest_dbus_t *bus;

      data.allow_replacement = allow;
      data.subprocess = NULL;

      bus = g_test_dbus_new (0);
      g_test_dbus_up (bus);

      app = xapplication_new ("org.gtk.test_application_t.Replace", allow ? G_APPLICATION_ALLOW_REPLACEMENT : G_APPLICATION_FLAGS_NONE);
      xapplication_set_inactivity_timeout (app, 500);
      xsignal_connect (app, "name-lost", G_CALLBACK (name_was_lost), &name_lost);
      xsignal_connect (app, "startup", G_CALLBACK (startup_cb), &data);
      xsignal_connect (app, "activate", G_CALLBACK (activate), NULL);

      if (!allow)
        g_timeout_add_seconds (1, quit_already, app);

      xapplication_run (app, G_N_ELEMENTS (argv) - 1, argv);

      g_assert_nonnull (data.subprocess);
      if (allow)
        g_assert_true (name_lost);
      else
        g_assert_false (name_lost);

      xobject_unref (app);
      g_free (binpath);

      xsubprocess_wait (data.subprocess, NULL, NULL);
      g_clear_object (&data.subprocess);

      g_test_dbus_down (bus);
      xobject_unref (bus);
    }
}

int
main (int argc, char **argv)
{
  g_setenv ("LC_ALL", "C", TRUE);

  g_test_init (&argc, &argv, NULL);

  if (!g_test_subprocess ())
    g_test_dbus_unset ();

  g_test_add_func ("/gapplication/no-dbus", test_nodbus);
/*  g_test_add_func ("/gapplication/basic", basic); */
  g_test_add_func ("/gapplication/no-appid", test_noappid);
/*  g_test_add_func ("/gapplication/non-unique", test_nonunique); */
  g_test_add_func ("/gapplication/properties", properties);
  g_test_add_func ("/gapplication/app-id", appid);
  g_test_add_func ("/gapplication/quit", test_quit);
  g_test_add_func ("/gapplication/registered", test_registered);
  g_test_add_func ("/gapplication/local-actions", test_local_actions);
/*  g_test_add_func ("/gapplication/remote-actions", test_remote_actions); */
  g_test_add_func ("/gapplication/local-command-line", test_local_command_line);
/*  g_test_add_func ("/gapplication/remote-command-line", test_remote_command_line); */
  g_test_add_func ("/gapplication/resource-path", test_resource_path);
  g_test_add_func ("/gapplication/test-help", test_help);
  g_test_add_func ("/gapplication/test-busy", test_busy);
  g_test_add_func ("/gapplication/test-handle-local-options1", test_handle_local_options_success);
  g_test_add_func ("/gapplication/test-handle-local-options2", test_handle_local_options_failure);
  g_test_add_func ("/gapplication/test-handle-local-options3", test_handle_local_options_passthrough);
  g_test_add_func ("/gapplication/api", test_api);
  g_test_add_data_func ("/gapplication/replace", GINT_TO_POINTER (TRUE), test_replace);
  g_test_add_data_func ("/gapplication/no-replace", GINT_TO_POINTER (FALSE), test_replace);

  return g_test_run ();
}
