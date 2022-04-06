/* GLib testing framework examples and tests
 * Copyright (C) 2008 Red Hat, Inc
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
 */

#include <gio/gio.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <glib.h>
#include <glib/glib-unix.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* sizeof(DATA) will give the number of bytes in the array, plus the terminating nul */
static const xchar_t DATA[] = "abcdefghijklmnopqrstuvwxyz";

int writer_pipe[2], reader_pipe[2];
xcancellable_t *writer_cancel, *reader_cancel, *main_cancel;
xmain_loop_t *loop;


static xpointer_t
writer_thread (xpointer_t user_data)
{
  xoutput_stream_t *out;
  xssize_t nwrote, offset;
  xerror_t *err = NULL;

  out = g_unix_output_stream_new (writer_pipe[1], TRUE);

  do
    {
      g_usleep (10);

      offset = 0;
      while (offset < (xssize_t) sizeof (DATA))
	{
	  nwrote = xoutput_stream_write (out, DATA + offset,
					  sizeof (DATA) - offset,
					  writer_cancel, &err);
	  if (nwrote <= 0 || err != NULL)
	    break;
	  offset += nwrote;
	}

      g_assert (nwrote > 0 || err != NULL);
    }
  while (err == NULL);

  if (xcancellable_is_cancelled (writer_cancel))
    {
      g_clear_error (&err);
      xcancellable_cancel (main_cancel);
      xobject_unref (out);
      return NULL;
    }

  g_warning ("writer: %s", err->message);
  g_assert_not_reached ();
}

static xpointer_t
reader_thread (xpointer_t user_data)
{
  xinput_stream_t *in;
  xssize_t nread = 0, total;
  xerror_t *err = NULL;
  char buf[sizeof (DATA)];

  in = g_unix_input_stream_new (reader_pipe[0], TRUE);

  do
    {
      total = 0;
      while (total < (xssize_t) sizeof (DATA))
	{
	  nread = xinput_stream_read (in, buf + total, sizeof (buf) - total,
				       reader_cancel, &err);
	  if (nread <= 0 || err != NULL)
	    break;
	  total += nread;
	}

      if (err)
	break;

      if (nread == 0)
	{
	  g_assert (err == NULL);
	  /* pipe closed */
	  xobject_unref (in);
	  return NULL;
	}

      g_assert_cmpstr (buf, ==, DATA);
      g_assert (!xcancellable_is_cancelled (reader_cancel));
    }
  while (err == NULL);

  g_warning ("reader: %s", err->message);
  g_assert_not_reached ();
}

static char main_buf[sizeof (DATA)];
static xssize_t main_len, main_offset;

static void main_thread_read (xobject_t *source, xasync_result_t *res, xpointer_t user_data);
static void main_thread_skipped (xobject_t *source, xasync_result_t *res, xpointer_t user_data);
static void main_thread_wrote (xobject_t *source, xasync_result_t *res, xpointer_t user_data);

static void
do_main_cancel (xoutput_stream_t *out)
{
  xoutput_stream_close (out, NULL, NULL);
  xmain_loop_quit (loop);
}

static void
main_thread_skipped (xobject_t *source, xasync_result_t *res, xpointer_t user_data)
{
  xinput_stream_t *in = G_INPUT_STREAM (source);
  xoutput_stream_t *out = user_data;
  xerror_t *err = NULL;
  xssize_t nskipped;

  nskipped = xinput_stream_skip_finish (in, res, &err);

  if (xcancellable_is_cancelled (main_cancel))
    {
      do_main_cancel (out);
      return;
    }

  g_assert_no_error (err);

  main_offset += nskipped;
  if (main_offset == main_len)
    {
      main_offset = 0;
      xoutput_stream_write_async (out, main_buf, main_len,
                                   G_PRIORITY_DEFAULT, main_cancel,
                                   main_thread_wrote, in);
    }
  else
    {
      xinput_stream_skip_async (in, main_len - main_offset,
				 G_PRIORITY_DEFAULT, main_cancel,
				 main_thread_skipped, out);
    }
}

static void
main_thread_read (xobject_t *source, xasync_result_t *res, xpointer_t user_data)
{
  xinput_stream_t *in = G_INPUT_STREAM (source);
  xoutput_stream_t *out = user_data;
  xerror_t *err = NULL;
  xssize_t nread;

  nread = xinput_stream_read_finish (in, res, &err);

  if (xcancellable_is_cancelled (main_cancel))
    {
      do_main_cancel (out);
      g_clear_error (&err);
      return;
    }

  g_assert_no_error (err);

  main_offset += nread;
  if (main_offset == sizeof (DATA))
    {
      main_len = main_offset;
      main_offset = 0;
      /* Now skip the same amount */
      xinput_stream_skip_async (in, main_len,
				 G_PRIORITY_DEFAULT, main_cancel,
				 main_thread_skipped, out);
    }
  else
    {
      xinput_stream_read_async (in, main_buf, sizeof (main_buf),
				 G_PRIORITY_DEFAULT, main_cancel,
				 main_thread_read, out);
    }
}

static void
main_thread_wrote (xobject_t *source, xasync_result_t *res, xpointer_t user_data)
{
  xoutput_stream_t *out = G_OUTPUT_STREAM (source);
  xinput_stream_t *in = user_data;
  xerror_t *err = NULL;
  xssize_t nwrote;

  nwrote = xoutput_stream_write_finish (out, res, &err);

  if (xcancellable_is_cancelled (main_cancel))
    {
      do_main_cancel (out);
      g_clear_error (&err);
      return;
    }

  g_assert_no_error (err);
  g_assert_cmpint (nwrote, <=, main_len - main_offset);

  main_offset += nwrote;
  if (main_offset == main_len)
    {
      main_offset = 0;
      xinput_stream_read_async (in, main_buf, sizeof (main_buf),
				 G_PRIORITY_DEFAULT, main_cancel,
				 main_thread_read, out);
    }
  else
    {
      xoutput_stream_write_async (out, main_buf + main_offset,
				   main_len - main_offset,
				   G_PRIORITY_DEFAULT, main_cancel,
				   main_thread_wrote, in);
    }
}

static xboolean_t
timeout (xpointer_t cancellable)
{
  xcancellable_cancel (cancellable);
  return FALSE;
}

static void
test_pipe_io (xconstpointer nonblocking)
{
  xthread_t *writer, *reader;
  xinput_stream_t *in;
  xoutput_stream_t *out;

  /* Split off two (additional) threads, a reader and a writer. From
   * the writer thread, write data synchronously in small chunks,
   * which gets alternately read and skipped asynchronously by the
   * main thread and then (if not skipped) written asynchronously to
   * the reader thread, which reads it synchronously. Eventually a
   * timeout in the main thread will cause it to cancel the writer
   * thread, which will in turn cancel the read op in the main thread,
   * which will then close the pipe to the reader thread, causing the
   * read op to fail.
   */

  g_assert (pipe (writer_pipe) == 0 && pipe (reader_pipe) == 0);

  if (nonblocking)
    {
      xerror_t *error = NULL;

      g_unix_set_fd_nonblocking (writer_pipe[0], TRUE, &error);
      g_assert_no_error (error);
      g_unix_set_fd_nonblocking (writer_pipe[1], TRUE, &error);
      g_assert_no_error (error);
      g_unix_set_fd_nonblocking (reader_pipe[0], TRUE, &error);
      g_assert_no_error (error);
      g_unix_set_fd_nonblocking (reader_pipe[1], TRUE, &error);
      g_assert_no_error (error);
    }

  writer_cancel = xcancellable_new ();
  reader_cancel = xcancellable_new ();
  main_cancel = xcancellable_new ();

  writer = xthread_new ("writer", writer_thread, NULL);
  reader = xthread_new ("reader", reader_thread, NULL);

  in = g_unix_input_stream_new (writer_pipe[0], TRUE);
  out = g_unix_output_stream_new (reader_pipe[1], TRUE);

  xinput_stream_read_async (in, main_buf, sizeof (main_buf),
			     G_PRIORITY_DEFAULT, main_cancel,
			     main_thread_read, out);

  g_timeout_add (500, timeout, writer_cancel);

  loop = xmain_loop_new (NULL, TRUE);
  xmain_loop_run (loop);
  xmain_loop_unref (loop);

  xthread_join (reader);
  xthread_join (writer);

  xobject_unref (main_cancel);
  xobject_unref (reader_cancel);
  xobject_unref (writer_cancel);
  xobject_unref (in);
  xobject_unref (out);
}

static void
test_basic (void)
{
  GUnixInputStream *is;
  GUnixOutputStream *os;
  xint_t fd;
  xboolean_t close_fd;

  is = G_UNIX_INPUT_STREAM (g_unix_input_stream_new (0, TRUE));
  xobject_get (is,
                "fd", &fd,
                "close-fd", &close_fd,
                NULL);
  g_assert_cmpint (fd, ==, 0);
  g_assert (close_fd);

  g_unix_input_stream_set_close_fd (is, FALSE);
  g_assert (!g_unix_input_stream_get_close_fd (is));
  g_assert_cmpint (g_unix_input_stream_get_fd (is), ==, 0);

  g_assert (!xinput_stream_has_pending (G_INPUT_STREAM (is)));

  xobject_unref (is);

  os = G_UNIX_OUTPUT_STREAM (g_unix_output_stream_new (1, TRUE));
  xobject_get (os,
                "fd", &fd,
                "close-fd", &close_fd,
                NULL);
  g_assert_cmpint (fd, ==, 1);
  g_assert (close_fd);

  g_unix_output_stream_set_close_fd (os, FALSE);
  g_assert (!g_unix_output_stream_get_close_fd (os));
  g_assert_cmpint (g_unix_output_stream_get_fd (os), ==, 1);

  g_assert (!xoutput_stream_has_pending (G_OUTPUT_STREAM (os)));

  xobject_unref (os);
}

typedef struct {
  xinput_stream_t *is;
  xoutput_stream_t *os;
  const xuint8_t *write_data;
  xuint8_t *read_data;
} TestReadWriteData;

static xpointer_t
test_read_write_write_thread (xpointer_t user_data)
{
  TestReadWriteData *data = user_data;
  xsize_t bytes_written;
  xerror_t *error = NULL;
  xboolean_t res;

  res = xoutput_stream_write_all (data->os, data->write_data, 1024, &bytes_written, NULL, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  g_assert_cmpuint (bytes_written, ==, 1024);

  return NULL;
}

static xpointer_t
test_read_write_read_thread (xpointer_t user_data)
{
  TestReadWriteData *data = user_data;
  xsize_t bytes_read;
  xerror_t *error = NULL;
  xboolean_t res;

  res = xinput_stream_read_all (data->is, data->read_data, 1024, &bytes_read, NULL, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  g_assert_cmpuint (bytes_read, ==, 1024);

  return NULL;
}

static xpointer_t
test_read_write_writev_thread (xpointer_t user_data)
{
  TestReadWriteData *data = user_data;
  xsize_t bytes_written;
  xerror_t *error = NULL;
  xboolean_t res;
  xoutput_vector_t vectors[3];

  vectors[0].buffer = data->write_data;
  vectors[0].size = 256;
  vectors[1].buffer = data->write_data + 256;
  vectors[1].size = 256;
  vectors[2].buffer = data->write_data + 512;
  vectors[2].size = 512;

  res = xoutput_stream_writev_all (data->os, vectors, G_N_ELEMENTS (vectors), &bytes_written, NULL, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  g_assert_cmpuint (bytes_written, ==, 1024);

  return NULL;
}

/* test if normal writing/reading from a pipe works */
static void
test_read_write (xconstpointer user_data)
{
  xboolean_t writev = GPOINTER_TO_INT (user_data);
  GUnixInputStream *is;
  GUnixOutputStream *os;
  xint_t fd[2];
  xuint8_t data_write[1024], data_read[1024];
  xuint_t i;
  xthread_t *write_thread, *read_thread;
  TestReadWriteData data;

  for (i = 0; i < sizeof (data_write); i++)
    data_write[i] = i;

  g_assert_cmpint (pipe (fd), ==, 0);

  is = G_UNIX_INPUT_STREAM (g_unix_input_stream_new (fd[0], TRUE));
  os = G_UNIX_OUTPUT_STREAM (g_unix_output_stream_new (fd[1], TRUE));

  data.is = G_INPUT_STREAM (is);
  data.os = G_OUTPUT_STREAM (os);
  data.read_data = data_read;
  data.write_data = data_write;

  if (writev)
    write_thread = xthread_new ("writer", test_read_write_writev_thread, &data);
  else
    write_thread = xthread_new ("writer", test_read_write_write_thread, &data);
  read_thread = xthread_new ("reader", test_read_write_read_thread, &data);

  xthread_join (write_thread);
  xthread_join (read_thread);

  g_assert_cmpmem (data_write, sizeof data_write, data_read, sizeof data_read);

  xobject_unref (os);
  xobject_unref (is);
}

/* test if xpollable_output_stream_write_nonblocking() and
 * xpollable_output_stream_read_nonblocking() correctly return WOULD_BLOCK
 * and correctly reset their status afterwards again, and all data that is
 * written can also be read again.
 */
static void
test_write_wouldblock (void)
{
#ifndef F_GETPIPE_SZ
  g_test_skip ("F_GETPIPE_SZ not defined");
#else  /* if F_GETPIPE_SZ */
  GUnixInputStream *is;
  GUnixOutputStream *os;
  xint_t fd[2];
  xerror_t *err = NULL;
  xuint8_t data_write[1024], data_read[1024];
  xsize_t i;
  int retval;
  xsize_t pipe_capacity;

  for (i = 0; i < sizeof (data_write); i++)
    data_write[i] = i;

  g_assert_cmpint (pipe (fd), ==, 0);

  g_assert_cmpint (fcntl (fd[0], F_SETPIPE_SZ, 4096, NULL), !=, 0);
  retval = fcntl (fd[0], F_GETPIPE_SZ);
  g_assert_cmpint (retval, >=, 0);
  pipe_capacity = (xsize_t) retval;
  g_assert_cmpint (pipe_capacity, >=, 4096);
  g_assert_cmpint (pipe_capacity % 1024, >=, 0);

  is = G_UNIX_INPUT_STREAM (g_unix_input_stream_new (fd[0], TRUE));
  os = G_UNIX_OUTPUT_STREAM (g_unix_output_stream_new (fd[1], TRUE));

  /* Run the whole thing three times to make sure that the streams
   * reset the writability/readability state again */
  for (i = 0; i < 3; i++) {
    xssize_t written = 0, written_complete = 0;
    xssize_t read = 0, read_complete = 0;

    do
      {
        written_complete += written;
        written = xpollable_output_stream_write_nonblocking (G_POLLABLE_OUTPUT_STREAM (os),
                                                              data_write,
                                                              sizeof (data_write),
                                                              NULL,
                                                              &err);
      }
    while (written > 0);

    g_assert_cmpuint (written_complete, >, 0);
    g_assert_nonnull (err);
    g_assert_error (err, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
    g_clear_error (&err);

    do
      {
        read_complete += read;
        read = g_pollable_input_stream_read_nonblocking (G_POLLABLE_INPUT_STREAM (is),
                                                         data_read,
                                                         sizeof (data_read),
                                                         NULL,
                                                         &err);
        if (read > 0)
          g_assert_cmpmem (data_read, read, data_write, sizeof (data_write));
      }
    while (read > 0);

    g_assert_cmpuint (read_complete, ==, written_complete);
    g_assert_nonnull (err);
    g_assert_error (err, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
    g_clear_error (&err);
  }

  xobject_unref (os);
  xobject_unref (is);
#endif  /* if F_GETPIPE_SZ */
}

/* test if xpollable_output_stream_writev_nonblocking() and
 * xpollable_output_stream_read_nonblocking() correctly return WOULD_BLOCK
 * and correctly reset their status afterwards again, and all data that is
 * written can also be read again.
 */
static void
test_writev_wouldblock (void)
{
#ifndef F_GETPIPE_SZ
  g_test_skip ("F_GETPIPE_SZ not defined");
#else  /* if F_GETPIPE_SZ */
  GUnixInputStream *is;
  GUnixOutputStream *os;
  xint_t fd[2];
  xerror_t *err = NULL;
  xuint8_t data_write[1024], data_read[1024];
  xsize_t i;
  int retval;
  xsize_t pipe_capacity;
  xoutput_vector_t vectors[4];
  GPollableReturn res;

  for (i = 0; i < sizeof (data_write); i++)
    data_write[i] = i;

  g_assert_cmpint (pipe (fd), ==, 0);

  g_assert_cmpint (fcntl (fd[0], F_SETPIPE_SZ, 4096, NULL), !=, 0);
  retval = fcntl (fd[0], F_GETPIPE_SZ);
  g_assert_cmpint (retval, >=, 0);
  pipe_capacity = (xsize_t) retval;
  g_assert_cmpint (pipe_capacity, >=, 4096);
  g_assert_cmpint (pipe_capacity % 1024, >=, 0);

  is = G_UNIX_INPUT_STREAM (g_unix_input_stream_new (fd[0], TRUE));
  os = G_UNIX_OUTPUT_STREAM (g_unix_output_stream_new (fd[1], TRUE));

  /* Run the whole thing three times to make sure that the streams
   * reset the writability/readability state again */
  for (i = 0; i < 3; i++) {
    xsize_t written = 0, written_complete = 0;
    xssize_t read = 0, read_complete = 0;

    do
    {
        written_complete += written;

        vectors[0].buffer = data_write;
        vectors[0].size = 256;
        vectors[1].buffer = data_write + 256;
        vectors[1].size = 256;
        vectors[2].buffer = data_write + 512;
        vectors[2].size = 256;
        vectors[3].buffer = data_write + 768;
        vectors[3].size = 256;

        res = xpollable_output_stream_writev_nonblocking (G_POLLABLE_OUTPUT_STREAM (os),
                                                           vectors,
                                                           G_N_ELEMENTS (vectors),
                                                           &written,
                                                           NULL,
                                                           &err);
      }
    while (res == G_POLLABLE_RETURN_OK);

    g_assert_cmpuint (written_complete, >, 0);
    g_assert_null (err);
    g_assert_cmpint (res, ==, G_POLLABLE_RETURN_WOULD_BLOCK);
    /* writev() on UNIX streams either succeeds fully or not at all */
    g_assert_cmpuint (written, ==, 0);

    do
      {
        read_complete += read;
        read = g_pollable_input_stream_read_nonblocking (G_POLLABLE_INPUT_STREAM (is),
                                                         data_read,
                                                         sizeof (data_read),
                                                         NULL,
                                                         &err);
        if (read > 0)
          g_assert_cmpmem (data_read, read, data_write, sizeof (data_write));
      }
    while (read > 0);

    g_assert_cmpuint (read_complete, ==, written_complete);
    g_assert_nonnull (err);
    g_assert_error (err, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
    g_clear_error (&err);
  }

  xobject_unref (os);
  xobject_unref (is);
#endif  /* if F_GETPIPE_SZ */
}

#ifdef F_GETPIPE_SZ
static void
write_async_wouldblock_cb (GUnixOutputStream *os,
                           xasync_result_t      *result,
                           xpointer_t           user_data)
{
  xsize_t *bytes_written = user_data;
  xerror_t *err = NULL;

  xoutput_stream_write_all_finish (G_OUTPUT_STREAM (os), result, bytes_written, &err);
  g_assert_no_error (err);
}

static void
read_async_wouldblock_cb (GUnixInputStream  *is,
                          xasync_result_t      *result,
                          xpointer_t           user_data)
{
  xsize_t *bytes_read = user_data;
  xerror_t *err = NULL;

  xinput_stream_read_all_finish (G_INPUT_STREAM (is), result, bytes_read, &err);
  g_assert_no_error (err);
}
#endif  /* if F_GETPIPE_SZ */

/* test if the async implementation of write_all() and read_all() in G*Stream
 * around the GPollable*Stream API is working correctly.
 */
static void
test_write_async_wouldblock (void)
{
#ifndef F_GETPIPE_SZ
  g_test_skip ("F_GETPIPE_SZ not defined");
#else  /* if F_GETPIPE_SZ */
  GUnixInputStream *is;
  GUnixOutputStream *os;
  xint_t fd[2];
  xuint8_t *data, *data_read;
  xsize_t i;
  int retval;
  xsize_t pipe_capacity;
  xsize_t bytes_written = 0, bytes_read = 0;

  g_assert_cmpint (pipe (fd), ==, 0);

  /* FIXME: These should not be needed but otherwise
   * g_unix_output_stream_write() will block because
   *   a) the fd is writable
   *   b) writing 4x capacity will block because writes are atomic
   *   c) the fd is blocking
   *
   * See https://gitlab.gnome.org/GNOME/glib/issues/1654
   */
  g_unix_set_fd_nonblocking (fd[0], TRUE, NULL);
  g_unix_set_fd_nonblocking (fd[1], TRUE, NULL);

  g_assert_cmpint (fcntl (fd[0], F_SETPIPE_SZ, 4096, NULL), !=, 0);
  retval = fcntl (fd[0], F_GETPIPE_SZ);
  g_assert_cmpint (retval, >=, 0);
  pipe_capacity = (xsize_t) retval;
  g_assert_cmpint (pipe_capacity, >=, 4096);

  data = g_new (xuint8_t, 4 * pipe_capacity);
  for (i = 0; i < 4 * pipe_capacity; i++)
    data[i] = i;
  data_read = g_new (xuint8_t, 4 * pipe_capacity);

  is = G_UNIX_INPUT_STREAM (g_unix_input_stream_new (fd[0], TRUE));
  os = G_UNIX_OUTPUT_STREAM (g_unix_output_stream_new (fd[1], TRUE));

  xoutput_stream_write_all_async (G_OUTPUT_STREAM (os),
                                   data,
                                   4 * pipe_capacity,
                                   G_PRIORITY_DEFAULT,
                                   NULL,
                                   (xasync_ready_callback_t) write_async_wouldblock_cb,
                                   &bytes_written);

  xinput_stream_read_all_async (G_INPUT_STREAM (is),
                                 data_read,
                                 4 * pipe_capacity,
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 (xasync_ready_callback_t) read_async_wouldblock_cb,
                                 &bytes_read);

  while (bytes_written == 0 && bytes_read == 0)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpuint (bytes_written, ==, 4 * pipe_capacity);
  g_assert_cmpuint (bytes_read, ==, 4 * pipe_capacity);
  g_assert_cmpmem (data_read, bytes_read, data, bytes_written);

  g_free (data);
  g_free (data_read);

  xobject_unref (os);
  xobject_unref (is);
#endif  /* if F_GETPIPE_SZ */
}

#ifdef F_GETPIPE_SZ
static void
writev_async_wouldblock_cb (GUnixOutputStream *os,
                            xasync_result_t      *result,
                            xpointer_t           user_data)
{
  xsize_t *bytes_written = user_data;
  xerror_t *err = NULL;

  xoutput_stream_writev_all_finish (G_OUTPUT_STREAM (os), result, bytes_written, &err);
  g_assert_no_error (err);
}
#endif  /* if F_GETPIPE_SZ */

/* test if the async implementation of writev_all() and read_all() in G*Stream
 * around the GPollable*Stream API is working correctly.
 */
static void
test_writev_async_wouldblock (void)
{
#ifndef F_GETPIPE_SZ
  g_test_skip ("F_GETPIPE_SZ not defined");
#else  /* if F_GETPIPE_SZ */
  GUnixInputStream *is;
  GUnixOutputStream *os;
  xint_t fd[2];
  xuint8_t *data, *data_read;
  xsize_t i;
  int retval;
  xsize_t pipe_capacity;
  xsize_t bytes_written = 0, bytes_read = 0;
  xoutput_vector_t vectors[4];

  g_assert_cmpint (pipe (fd), ==, 0);

  /* FIXME: These should not be needed but otherwise
   * g_unix_output_stream_writev() will block because
   *   a) the fd is writable
   *   b) writing 4x capacity will block because writes are atomic
   *   c) the fd is blocking
   *
   * See https://gitlab.gnome.org/GNOME/glib/issues/1654
   */
  g_unix_set_fd_nonblocking (fd[0], TRUE, NULL);
  g_unix_set_fd_nonblocking (fd[1], TRUE, NULL);

  g_assert_cmpint (fcntl (fd[0], F_SETPIPE_SZ, 4096, NULL), !=, 0);
  retval = fcntl (fd[0], F_GETPIPE_SZ);
  g_assert_cmpint (retval, >=, 0);
  pipe_capacity = (xsize_t) retval;
  g_assert_cmpint (pipe_capacity, >=, 4096);

  data = g_new (xuint8_t, 4 * pipe_capacity);
  for (i = 0; i < 4 * pipe_capacity; i++)
    data[i] = i;
  data_read = g_new (xuint8_t, 4 * pipe_capacity);

  vectors[0].buffer = data;
  vectors[0].size = 1024;
  vectors[1].buffer = data + 1024;
  vectors[1].size = 1024;
  vectors[2].buffer = data + 2048;
  vectors[2].size = 1024;
  vectors[3].buffer = data + 3072;
  vectors[3].size = 4 * pipe_capacity - 3072;

  is = G_UNIX_INPUT_STREAM (g_unix_input_stream_new (fd[0], TRUE));
  os = G_UNIX_OUTPUT_STREAM (g_unix_output_stream_new (fd[1], TRUE));

  xoutput_stream_writev_all_async (G_OUTPUT_STREAM (os),
                                    vectors,
                                    G_N_ELEMENTS (vectors),
                                    G_PRIORITY_DEFAULT,
                                    NULL,
                                    (xasync_ready_callback_t) writev_async_wouldblock_cb,
                                    &bytes_written);

  xinput_stream_read_all_async (G_INPUT_STREAM (is),
                                 data_read,
                                 4 * pipe_capacity,
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 (xasync_ready_callback_t) read_async_wouldblock_cb,
                                 &bytes_read);

  while (bytes_written == 0 && bytes_read == 0)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpuint (bytes_written, ==, 4 * pipe_capacity);
  g_assert_cmpuint (bytes_read, ==, 4 * pipe_capacity);
  g_assert_cmpmem (data_read, bytes_read, data, bytes_written);

  g_free (data);
  g_free (data_read);

  xobject_unref (os);
  xobject_unref (is);
#endif  /* F_GETPIPE_SZ */
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/unix-streams/basic", test_basic);
  g_test_add_data_func ("/unix-streams/pipe-io-test",
			GINT_TO_POINTER (FALSE),
			test_pipe_io);
  g_test_add_data_func ("/unix-streams/nonblocking-io-test",
			GINT_TO_POINTER (TRUE),
			test_pipe_io);

  g_test_add_data_func ("/unix-streams/read_write",
                        GINT_TO_POINTER (FALSE),
                        test_read_write);

  g_test_add_data_func ("/unix-streams/read_writev",
                        GINT_TO_POINTER (TRUE),
                        test_read_write);

  g_test_add_func ("/unix-streams/write-wouldblock",
		   test_write_wouldblock);
  g_test_add_func ("/unix-streams/writev-wouldblock",
		   test_writev_wouldblock);

  g_test_add_func ("/unix-streams/write-async-wouldblock",
		   test_write_async_wouldblock);
  g_test_add_func ("/unix-streams/writev-async-wouldblock",
		   test_writev_async_wouldblock);

  return g_test_run();
}
