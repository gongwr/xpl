/* GDBus regression test - close a stream when a message remains to be written
 *
 * Copyright © 2006-2010 Red Hat, Inc.
 * Copyright © 2011 Nokia Corporation
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
 * Author: Simon McVittie <simon.mcvittie@collabora.co.uk>
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>

#ifdef G_OS_UNIX
# include <unistd.h>

# include <glib/glib-unix.h>
# include <gio/gunixinputstream.h>
# include <gio/gunixoutputstream.h>
# include <gio/gunixconnection.h>
#else
# error This test is currently Unix-specific due to use of g_unix_open_pipe()
#endif

#include "gdbus-tests.h"

#define CLOSE_TIME_MS 1
#define N_REPEATS_SLOW 5000
#define N_REPEATS 100

/* ---------- MyIOStream ------------------------------------------------- */

#define MY_TYPE_IO_STREAM  (my_io_stream_get_type ())
#define MY_IO_STREAM(o)    (XTYPE_CHECK_INSTANCE_CAST ((o), MY_TYPE_IO_STREAM, MyIOStream))
#define MY_IS_IO_STREAM(o) (XTYPE_CHECK_INSTANCE_TYPE ((o), MY_TYPE_IO_STREAM))

typedef struct
{
  xio_stream_t parent_instance;
  xinput_stream_t *input_stream;
  xoutput_stream_t *output_stream;
} MyIOStream;

typedef struct
{
  xio_stream_class_t parent_class;
} MyIOStreamClass;

static xtype_t my_io_stream_get_type (void) G_GNUC_CONST;

XDEFINE_TYPE (MyIOStream, my_io_stream, XTYPE_IO_STREAM)

static void
my_io_stream_finalize (xobject_t *object)
{
  MyIOStream *stream = MY_IO_STREAM (object);
  xobject_unref (stream->input_stream);
  xobject_unref (stream->output_stream);
  XOBJECT_CLASS (my_io_stream_parent_class)->finalize (object);
}

static void
my_io_stream_init (MyIOStream *stream)
{
}

static xinput_stream_t *
my_io_stream_get_input_stream (xio_stream_t *_stream)
{
  MyIOStream *stream = MY_IO_STREAM (_stream);
  return stream->input_stream;
}

static xoutput_stream_t *
my_io_stream_get_output_stream (xio_stream_t *_stream)
{
  MyIOStream *stream = MY_IO_STREAM (_stream);
  return stream->output_stream;
}

static void
my_io_stream_class_init (MyIOStreamClass *klass)
{
  xobject_class_t *xobject_class;
  xio_stream_class_t *giostream_class;

  xobject_class = XOBJECT_CLASS (klass);
  xobject_class->finalize = my_io_stream_finalize;

  giostream_class = XIO_STREAM_CLASS (klass);
  giostream_class->get_input_stream  = my_io_stream_get_input_stream;
  giostream_class->get_output_stream = my_io_stream_get_output_stream;
}

static xio_stream_t *
my_io_stream_new (xinput_stream_t  *input_stream,
                  xoutput_stream_t *output_stream)
{
  MyIOStream *stream;
  xreturn_val_if_fail (X_IS_INPUT_STREAM (input_stream), NULL);
  xreturn_val_if_fail (X_IS_OUTPUT_STREAM (output_stream), NULL);
  stream = MY_IO_STREAM (xobject_new (MY_TYPE_IO_STREAM, NULL));
  stream->input_stream = xobject_ref (input_stream);
  stream->output_stream = xobject_ref (output_stream);
  return XIO_STREAM (stream);
}

/* ---------- MySlowCloseOutputStream ------------------------------------ */

typedef struct
{
  xfilter_output_stream_t parent_instance;
} MySlowCloseOutputStream;

typedef struct
{
  xfilter_output_stream_class_t parent_class;
} MySlowCloseOutputStreamClass;

#define MY_TYPE_SLOW_CLOSE_OUTPUT_STREAM \
  (my_slow_close_output_stream_get_type ())
#define MY_OUTPUT_STREAM(o) \
  (XTYPE_CHECK_INSTANCE_CAST ((o), MY_TYPE_SLOW_CLOSE_OUTPUT_STREAM, \
                               MySlowCloseOutputStream))
#define MY_IS_SLOW_CLOSE_OUTPUT_STREAM(o) \
  (XTYPE_CHECK_INSTANCE_TYPE ((o), MY_TYPE_SLOW_CLOSE_OUTPUT_STREAM))

static xtype_t my_slow_close_output_stream_get_type (void) G_GNUC_CONST;

XDEFINE_TYPE (MySlowCloseOutputStream, my_slow_close_output_stream,
               XTYPE_FILTER_OUTPUT_STREAM)

static void
my_slow_close_output_stream_init (MySlowCloseOutputStream *stream)
{
}

static xboolean_t
my_slow_close_output_stream_close (xoutput_stream_t  *stream,
                                   xcancellable_t   *cancellable,
                                   xerror_t        **error)
{
  g_usleep (CLOSE_TIME_MS * 1000);
  return G_OUTPUT_STREAM_CLASS (my_slow_close_output_stream_parent_class)->
    close_fn (stream, cancellable, error);
}

typedef struct {
    xoutput_stream_t *stream;
    xint_t io_priority;
    xcancellable_t *cancellable;
    xasync_ready_callback_t callback;
    xpointer_t user_data;
} DelayedClose;

static void
delayed_close_free (xpointer_t data)
{
  DelayedClose *df = data;

  xobject_unref (df->stream);
  if (df->cancellable)
    xobject_unref (df->cancellable);
  g_free (df);
}

static xboolean_t
delayed_close_cb (xpointer_t data)
{
  DelayedClose *df = data;

  G_OUTPUT_STREAM_CLASS (my_slow_close_output_stream_parent_class)->
    close_async (df->stream, df->io_priority, df->cancellable, df->callback,
                 df->user_data);

  return XSOURCE_REMOVE;
}

static void
my_slow_close_output_stream_close_async  (xoutput_stream_t            *stream,
                                          int                       io_priority,
                                          xcancellable_t             *cancellable,
                                          xasync_ready_callback_t       callback,
                                          xpointer_t                  user_data)
{
  xsource_t *later;
  DelayedClose *df;

  df = g_new0 (DelayedClose, 1);
  df->stream = xobject_ref (stream);
  df->io_priority = io_priority;
  df->cancellable = (cancellable != NULL ? xobject_ref (cancellable) : NULL);
  df->callback = callback;
  df->user_data = user_data;

  later = g_timeout_source_new (CLOSE_TIME_MS);
  xsource_set_callback (later, delayed_close_cb, df, delayed_close_free);
  xsource_attach (later, xmain_context_get_thread_default ());
}

static xboolean_t
my_slow_close_output_stream_close_finish  (xoutput_stream_t  *stream,
                                xasync_result_t   *result,
                                xerror_t        **error)
{
  return G_OUTPUT_STREAM_CLASS (my_slow_close_output_stream_parent_class)->
    close_finish (stream, result, error);
}

static void
my_slow_close_output_stream_class_init (MySlowCloseOutputStreamClass *klass)
{
  xoutput_stream_class_t *ostream_class;

  ostream_class = G_OUTPUT_STREAM_CLASS (klass);
  ostream_class->close_fn = my_slow_close_output_stream_close;
  ostream_class->close_async = my_slow_close_output_stream_close_async;
  ostream_class->close_finish = my_slow_close_output_stream_close_finish;
}

static xio_stream_t *
my_io_stream_new_for_fds (xint_t fd_in, xint_t fd_out)
{
  xio_stream_t *stream;
  xinput_stream_t *input_stream;
  xoutput_stream_t *real_output_stream;
  xoutput_stream_t *output_stream;

  input_stream = g_unix_input_stream_new (fd_in, TRUE);
  real_output_stream = g_unix_output_stream_new (fd_out, TRUE);
  output_stream = xobject_new (MY_TYPE_SLOW_CLOSE_OUTPUT_STREAM,
                                "base-stream", real_output_stream,
                                NULL);
  stream = my_io_stream_new (input_stream, output_stream);
  xobject_unref (input_stream);
  xobject_unref (output_stream);
  xobject_unref (real_output_stream);
  return stream;
}

/* ---------- Tests ------------------------------------------------------ */

typedef struct {
  xint_t server_to_client[2];
  xint_t client_to_server[2];
  xio_stream_t *server_iostream;
  xdbus_connection_t *server_conn;
  xio_stream_t *iostream;
  xdbus_connection_t *connection;
  xchar_t *guid;
  xerror_t *error;
} Fixture;

static void
setup (Fixture       *f,
       xconstpointer  context)
{
  f->guid = g_dbus_generate_guid ();
}

static void
teardown (Fixture       *f,
          xconstpointer  context)
{
  g_clear_object (&f->server_iostream);
  g_clear_object (&f->server_conn);
  g_clear_object (&f->iostream);
  g_clear_object (&f->connection);
  g_clear_error (&f->error);
  g_free (f->guid);
}

static void
on_new_conn (xobject_t      *source,
             xasync_result_t *res,
             xpointer_t      user_data)
{
  xdbus_connection_t **connection = user_data;
  xerror_t *error = NULL;

  *connection = xdbus_connection_new_for_address_finish (res, &error);
  g_assert_no_error (error);
}

static void
test_once (Fixture       *f,
           xconstpointer  context)
{
  xdbus_message_t *message;
  xboolean_t pipe_res;

  pipe_res = g_unix_open_pipe (f->server_to_client, FD_CLOEXEC, &f->error);
  xassert (pipe_res);
  pipe_res = g_unix_open_pipe (f->client_to_server, FD_CLOEXEC, &f->error);
  xassert (pipe_res);

  f->server_iostream = my_io_stream_new_for_fds (f->client_to_server[0],
                                                 f->server_to_client[1]);
  f->iostream = my_io_stream_new_for_fds (f->server_to_client[0],
                                          f->client_to_server[1]);

  xdbus_connection_new (f->server_iostream,
                         f->guid,
                         (G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER |
                          G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS),
                         NULL /* auth observer */,
                         NULL /* cancellable */,
                         on_new_conn, &f->server_conn);

  xdbus_connection_new (f->iostream,
                         NULL,
                         G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                         NULL /* auth observer */,
                         NULL /* cancellable */,
                         on_new_conn, &f->connection);

  while (f->server_conn == NULL || f->connection == NULL)
    xmain_context_iteration (NULL, TRUE);

  /*
   * queue a message - it'll sometimes be sent while the close is pending,
   * triggering the bug
   */
  message = xdbus_message_new_signal ("/", "com.example.foo_t", "Bar");
  xdbus_connection_send_message (f->connection, message, 0, NULL, &f->error);
  g_assert_no_error (f->error);
  xobject_unref (message);

  /* close the connection (deliberately or via last-unref) */
  if (xstrcmp0 (context, "unref") == 0)
    {
      g_clear_object (&f->connection);
    }
  else
    {
      xdbus_connection_close_sync (f->connection, NULL, &f->error);
      g_assert_no_error (f->error);
    }

  /* either way, wait for the connection to close */
  while (!xdbus_connection_is_closed (f->server_conn))
    xmain_context_iteration (NULL, TRUE);

  /* clean up before the next run */
  g_clear_object (&f->iostream);
  g_clear_object (&f->server_iostream);
  g_clear_object (&f->connection);
  g_clear_object (&f->server_conn);
  g_clear_error (&f->error);
}

static void
test_many_times (Fixture       *f,
                 xconstpointer  context)
{
  xuint_t i, n_repeats;

  if (g_test_slow ())
    n_repeats = N_REPEATS_SLOW;
  else
    n_repeats = N_REPEATS;

  for (i = 0; i < n_repeats; i++)
    test_once (f, context);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/gdbus/close-pending", Fixture, "close",
      setup, test_many_times, teardown);
  g_test_add ("/gdbus/unref-pending", Fixture, "unref",
      setup, test_many_times, teardown);

  return g_test_run();
}
