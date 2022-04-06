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

#include <gio/gio.h>
#include <unistd.h>
#include <string.h>

#include <stdlib.h>

#ifdef G_OS_UNIX
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <gio/gunixconnection.h>
#endif

#include "gdbus-tests.h"

static xmain_loop_t *loop = NULL;

/* ---------------------------------------------------------------------------------------------------- */
#ifdef G_OS_UNIX

#include "test-pipe-unix.h"
#include "test-io-stream.h"

/* ---------------------------------------------------------------------------------------------------- */

static const xdbus_arg_info_t pokee_method_poke_out_arg0 = {
  -1,   /* ref_count */
  "result",
  "s",
  NULL  /* annotations */
};

static const xdbus_arg_info_t *pokee_method_poke_out_args[2] = {
  &pokee_method_poke_out_arg0,
  NULL,
};

static const xdbus_arg_info_t pokee_method_poke_in_arg0 = {
  -1,   /* ref_count */
  "value",
  "s",
  NULL  /* annotations */
};

static const xdbus_arg_info_t *pokee_method_poke_in_args[2] = {
  &pokee_method_poke_in_arg0,
  NULL,
};

static const xdbus_method_info_t pokee_method_poke = {
  -1,   /* ref_count */
  "Poke",
  (xdbus_arg_info_t**) pokee_method_poke_in_args,
  (xdbus_arg_info_t**) pokee_method_poke_out_args,
  NULL  /* annotations */
};

static const xdbus_method_info_t *pokee_methods[2] = {
  &pokee_method_poke,
  NULL
};

static const xdbus_interface_info_t pokee_object_info = {
  -1,  /* ref_count */
  "org.gtk.GDBus.Pokee",
  (xdbus_method_info_t**) pokee_methods,
  NULL, /* signals */
  NULL, /* properties */
  NULL  /* annotations */
};

static void
pokee_method_call (xdbus_connection_t       *connection,
                   const xchar_t           *sender,
                   const xchar_t           *object_path,
                   const xchar_t           *interface_name,
                   const xchar_t           *method_name,
                   xvariant_t              *parameters,
                   xdbus_method_invocation_t *invocation,
                   xpointer_t               user_data)
{
  const xchar_t *str;
  xchar_t *ret;

  g_assert_cmpstr (method_name, ==, "Poke");

  xvariant_get (parameters, "(&s)", &str);
  ret = xstrdup_printf ("You poked me with: '%s'", str);
  xdbus_method_invocation_return_value (invocation, xvariant_new ("(s)", ret));
  g_free (ret);
}

static const xdbus_interface_vtable_t pokee_vtable = {
  pokee_method_call,
  NULL, /* get_property */
  NULL, /* set_property */
  { 0 }
};

/* Processes:
 *
 * parent
 * \- first child (via fork()) is the pokee
 * \- second child (via g_test_trap_fork()) is the poker
 *
 * The second child only exists to avoid sharing a main context between several
 * second-children if we run a test resembling this one multiple times.
 * See https://bugzilla.gnome.org/show_bug.cgi?id=658999 for why that's bad.
 */
static void
test_non_socket (void)
{
  xio_stream_t *streams[2];
  xdbus_connection_t *connection;
  xerror_t *error;
  xchar_t *guid;
  pid_t first_child;
  xvariant_t *ret;
  const xchar_t *str;
  xboolean_t ok;

  error = NULL;

  ok = test_bidi_pipe (&streams[0], &streams[1], &error);
  g_assert_no_error (error);
  g_assert (ok);
  g_assert (X_IS_IO_STREAM (streams[0]));
  g_assert (X_IS_INPUT_STREAM (g_io_stream_get_input_stream (streams[0])));
  g_assert (X_IS_OUTPUT_STREAM (g_io_stream_get_output_stream (streams[0])));
  g_assert (X_IS_IO_STREAM (streams[1]));
  g_assert (X_IS_INPUT_STREAM (g_io_stream_get_input_stream (streams[1])));
  g_assert (X_IS_OUTPUT_STREAM (g_io_stream_get_output_stream (streams[1])));

  switch ((first_child = fork ()))
    {
    case -1:
      g_assert_not_reached ();
      break;

    case 0:
      /* first child */

      /* we shouldn't do this in the parent, because we shouldn't use a
       * xmain_context_t both before and after fork
       */
      loop = xmain_loop_new (NULL, FALSE);

      ok = g_io_stream_close (streams[1], NULL, &error);
      g_assert_no_error (error);
      g_assert (ok);
      xobject_unref (streams[1]);

      guid = g_dbus_generate_guid ();
      error = NULL;
      /* We need to delay message processing to avoid the race
       * described in
       *
       *  https://bugzilla.gnome.org/show_bug.cgi?id=627188
       *
       * This is because (early) dispatching is done on the IO thread
       * (method_call() isn't called until we're in the right thread
       * though) so in rare cases the parent sends the message before
       * we (the first child) register the object
       */
      connection = xdbus_connection_new_sync (streams[0],
                                               guid,
                                               G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER |
                                               G_DBUS_CONNECTION_FLAGS_DELAY_MESSAGE_PROCESSING,
                                               NULL, /* xdbus_auth_observer_t */
                                               NULL,
                                               &error);
      g_free (guid);
      g_assert_no_error (error);
      xobject_unref (streams[0]);

      /* make sure we exit along with the parent */
      xdbus_connection_set_exit_on_close (connection, TRUE);

      error = NULL;
      xdbus_connection_register_object (connection,
                                         "/pokee",
                                         (xdbus_interface_info_t *) &pokee_object_info,
                                         &pokee_vtable,
                                         NULL, /* user_data */
                                         NULL, /* user_data_free_func */
                                         &error);
      g_assert_no_error (error);

      /* and now start message processing */
      xdbus_connection_start_message_processing (connection);

      xmain_loop_run (loop);

      g_assert_not_reached ();
      break;

    default:
      /* parent continues below */
      break;
    }

  /* This is #ifdef G_OS_UNIX anyway, so just use g_test_trap_fork() */
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  if (!g_test_trap_fork (0, 0))
    {
      /* parent */
      xobject_unref (streams[0]);
      xobject_unref (streams[1]);

      g_test_trap_assert_passed ();
      g_assert_cmpint (kill (first_child, SIGTERM), ==, 0);
      return;
    }
  G_GNUC_END_IGNORE_DEPRECATIONS;

  /* second child */

  /* we shouldn't do this in the parent, because we shouldn't use a
   * xmain_context_t both before and after fork
   */
  loop = xmain_loop_new (NULL, FALSE);

  ok = g_io_stream_close (streams[0], NULL, &error);
  g_assert_no_error (error);
  g_assert (ok);
  xobject_unref (streams[0]);

  connection = xdbus_connection_new_sync (streams[1],
                                           NULL, /* guid */
                                           G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                           NULL, /* xdbus_auth_observer_t */
                                           NULL,
                                           &error);
  g_assert_no_error (error);
  xobject_unref (streams[1]);

  /* poke the first child */
  error = NULL;
  ret = xdbus_connection_call_sync (connection,
                                     NULL, /* name */
                                     "/pokee",
                                     "org.gtk.GDBus.Pokee",
                                     "Poke",
                                     xvariant_new ("(s)", "I am the POKER!"),
                                     G_VARIANT_TYPE ("(s)"), /* return type */
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1,
                                     NULL, /* cancellable */
                                     &error);
  g_assert_no_error (error);
  xvariant_get (ret, "(&s)", &str);
  g_assert_cmpstr (str, ==, "You poked me with: 'I am the POKER!'");
  xvariant_unref (ret);

  xobject_unref (connection);
  xmain_loop_unref (loop);
  exit (0);
}

#else /* G_OS_UNIX */

static void
test_non_socket (void)
{
  /* TODO: test this with e.g. GWin32InputStream/GWin32OutputStream */
}
#endif

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  xint_t ret;

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/gdbus/non-socket", test_non_socket);

  ret = g_test_run();

  return ret;
}
