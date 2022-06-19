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

#include <glib/glib.h>
#include <gio/gio.h>
#include <gio/gwin32inputstream.h>
#include <gio/gwin32outputstream.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

#include <windows.h>

#define DATA "abcdefghijklmnopqrstuvwxyz"

int writer_pipe[2], reader_pipe[2];
xcancellable_t *writer_cancel, *reader_cancel, *main_cancel;
xmain_loop_t *loop;

static xpointer_t
writer_thread (xpointer_t user_data)
{
  xoutput_stream_t *out;
  xssize_t nwrote, offset;
  xerror_t *err = NULL;
  HANDLE out_handle;

  xassert (DuplicateHandle (GetCurrentProcess (),
			     (HANDLE) (gintptr) _get_osfhandle (writer_pipe[1]),
			     GetCurrentProcess (),
			     &out_handle,
			     0, FALSE,
			     DUPLICATE_SAME_ACCESS));
  close (writer_pipe[1]);

  out = g_win32_output_stream_new (out_handle, TRUE);
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

      xassert (nwrote > 0 || err != NULL);
    }
  while (err == NULL);

  if (xcancellable_is_cancelled (writer_cancel))
    {
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
  HANDLE in_handle;

  xassert (DuplicateHandle (GetCurrentProcess (),
			     (HANDLE) (gintptr) _get_osfhandle (reader_pipe[0]),
			     GetCurrentProcess (),
			     &in_handle,
			     0, FALSE,
			     DUPLICATE_SAME_ACCESS));
  close (reader_pipe[0]);

  in = g_win32_input_stream_new (in_handle, TRUE);

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
	  xassert (err == NULL);
	  /* pipe closed */
	  xobject_unref (in);
	  return NULL;
	}

      g_assert_cmpstr (buf, ==, DATA);
      xassert (!xcancellable_is_cancelled (reader_cancel));
    }
  while (err == NULL);

  g_warning ("reader: %s", err->message);
  g_assert_not_reached ();
}

char main_buf[sizeof (DATA)];
xssize_t main_len, main_offset;

static void readable (xobject_t *source, xasync_result_t *res, xpointer_t user_data);
static void writable (xobject_t *source, xasync_result_t *res, xpointer_t user_data);

static void
do_main_cancel (xoutput_stream_t *out)
{
  xoutput_stream_close (out, NULL, NULL);
  xmain_loop_quit (loop);
}

static void
readable (xobject_t *source, xasync_result_t *res, xpointer_t user_data)
{
  xinput_stream_t *in = G_INPUT_STREAM (source);
  xoutput_stream_t *out = user_data;
  xerror_t *err = NULL;

  main_len = xinput_stream_read_finish (in, res, &err);

  if (xcancellable_is_cancelled (main_cancel))
    {
      do_main_cancel (out);
      return;
    }

  xassert (err == NULL);

  main_offset = 0;
  xoutput_stream_write_async (out, main_buf, main_len,
			       G_PRIORITY_DEFAULT, main_cancel,
			       writable, in);
}

static void
writable (xobject_t *source, xasync_result_t *res, xpointer_t user_data)
{
  xoutput_stream_t *out = G_OUTPUT_STREAM (source);
  xinput_stream_t *in = user_data;
  xerror_t *err = NULL;
  xssize_t nwrote;

  nwrote = xoutput_stream_write_finish (out, res, &err);

  if (xcancellable_is_cancelled (main_cancel))
    {
      do_main_cancel (out);
      return;
    }

  xassert (err == NULL);
  g_assert_cmpint (nwrote, <=, main_len - main_offset);

  main_offset += nwrote;
  if (main_offset == main_len)
    {
      xinput_stream_read_async (in, main_buf, sizeof (main_buf),
				 G_PRIORITY_DEFAULT, main_cancel,
				 readable, out);
    }
  else
    {
      xoutput_stream_write_async (out, main_buf + main_offset,
				   main_len - main_offset,
				   G_PRIORITY_DEFAULT, main_cancel,
				   writable, in);
    }
}

static xboolean_t
timeout (xpointer_t cancellable)
{
  xcancellable_cancel (cancellable);
  return FALSE;
}

static void
test_pipe_io (void)
{
  xthread_t *writer, *reader;
  xinput_stream_t *in;
  xoutput_stream_t *out;
  HANDLE in_handle, out_handle;

  /* Split off two (additional) threads, a reader and a writer. From
   * the writer thread, write data synchronously in small chunks,
   * which gets read asynchronously by the main thread and then
   * written asynchronously to the reader thread, which reads it
   * synchronously. Eventually a timeout in the main thread will cause
   * it to cancel the writer thread, which will in turn cancel the
   * read op in the main thread, which will then close the pipe to
   * the reader thread, causing the read op to fail.
   */

  xassert (_pipe (writer_pipe, 10, _O_BINARY) == 0 && _pipe (reader_pipe, 10, _O_BINARY) == 0);

  writer_cancel = xcancellable_new ();
  reader_cancel = xcancellable_new ();
  main_cancel = xcancellable_new ();

  writer = xthread_new ("writer", writer_thread, NULL);
  reader = xthread_new ("reader", reader_thread, NULL);

  xassert (DuplicateHandle (GetCurrentProcess (),
			     (HANDLE) (gintptr) _get_osfhandle (writer_pipe[0]),
			     GetCurrentProcess (),
			     &in_handle,
			     0, FALSE,
			     DUPLICATE_SAME_ACCESS));
  close (writer_pipe[0]);

  xassert (DuplicateHandle (GetCurrentProcess (),
			     (HANDLE) (gintptr) _get_osfhandle (reader_pipe[1]),
			     GetCurrentProcess (),
			     &out_handle,
			     0, FALSE,
			     DUPLICATE_SAME_ACCESS));
  close (reader_pipe[1]);

  in = g_win32_input_stream_new (in_handle, TRUE);
  out = g_win32_output_stream_new (out_handle, TRUE);

  xinput_stream_read_async (in, main_buf, sizeof (main_buf),
			     G_PRIORITY_DEFAULT, main_cancel,
			     readable, out);

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

typedef struct _PipeIOOverlapReader
{
  char buf[sizeof (DATA)];
  xinput_stream_t *in;
  xthread_t *thread;
  xcancellable_t *cancellable;
  xboolean_t success;
} PipeIOOverlapReader;

#define TEST_PIPE_IO_OVERLAP (1024 * 4)

static xpointer_t
pipe_io_overlap_reader_thread (xpointer_t user_data)
{
  PipeIOOverlapReader *p = user_data;
  xerror_t *err = NULL;
  xsize_t read;
  xuint_t i;

  for (i = 0; i < TEST_PIPE_IO_OVERLAP; ++i) {
    memset (p->buf, 0, sizeof (p->buf));
    xinput_stream_read_all (p->in, p->buf, sizeof (p->buf),
                             &read, NULL, &err);

    g_assert_cmpuint (read, ==, sizeof (p->buf));
    g_assert_no_error (err);
    g_assert_cmpstr (p->buf, ==, DATA);
  }

  return NULL;
}

static xpointer_t
pipe_io_overlap_writer_thread (xpointer_t user_data)
{
  xoutput_stream_t *out = user_data;
  xerror_t *err = NULL;
  xsize_t bytes_written;
  xuint_t i;

  for (i = 0; i < TEST_PIPE_IO_OVERLAP; ++i) {
    xoutput_stream_write_all (out, DATA, sizeof (DATA),
                               &bytes_written, NULL, &err);

    g_assert_cmpuint (bytes_written, ==, sizeof (DATA));
    g_assert_no_error (err);
  }

  return NULL;
}

static void
test_pipe_io_overlap (void)
{
  xoutput_stream_t *out_server, *out_client;
  xthread_t *writer_server, *writer_client;
  PipeIOOverlapReader rs, rc;
  HANDLE server, client;
  xchar_t name[256];

  g_snprintf (name, sizeof (name),
              "\\\\.\\pipe\\gtest-io-overlap-%u", (xuint_t) GetCurrentProcessId ());

  server = CreateNamedPipe (name,
                            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                            PIPE_READMODE_BYTE | PIPE_WAIT,
                            1, 0, 0, 0, NULL);
  xassert (server != INVALID_HANDLE_VALUE);

  client = CreateFile (name, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
  xassert (client != INVALID_HANDLE_VALUE);

  out_server = g_win32_output_stream_new (server, TRUE);
  writer_server = xthread_new ("writer_server", pipe_io_overlap_writer_thread, out_server);
  rs.in = g_win32_input_stream_new (server, TRUE);
  rs.thread = xthread_new ("reader_server", pipe_io_overlap_reader_thread, &rs);

  out_client = g_win32_output_stream_new (client, TRUE);
  writer_client = xthread_new ("writer_client", pipe_io_overlap_writer_thread, out_client);
  rc.in = g_win32_input_stream_new (client, TRUE);
  rc.thread = xthread_new ("reader_client", pipe_io_overlap_reader_thread, &rc);

  xthread_join (writer_client);
  xthread_join (writer_server);
  xthread_join (rc.thread);
  xthread_join (rs.thread);

  xobject_unref (rs.in);
  xobject_unref (rc.in);
  xobject_unref (out_server);
  xobject_unref (out_client);
}

static xpointer_t
pipe_io_concurrent_writer_thread (xpointer_t user_data)
{
  xoutput_stream_t *out = user_data;
  xerror_t *err = NULL;
  xsize_t bytes_written;

  xoutput_stream_write_all (out, DATA, 1, &bytes_written, NULL, &err);

  g_assert_cmpuint (bytes_written, ==, 1);
  g_assert_no_error (err);

  return NULL;
}

static xpointer_t
pipe_io_concurrent_reader_thread (xpointer_t user_data)
{
  PipeIOOverlapReader *p = user_data;
  xerror_t *err = NULL;
  xsize_t read;

  memset (p->buf, 0, sizeof (p->buf));
  p->success = xinput_stream_read_all (p->in, p->buf, 1, &read, p->cancellable, &err);

  /* only one thread will succeed, the other will be cancelled */
  if (p->success)
    {
      /* continue the main thread */
      write (writer_pipe[1], "", 1);
      g_assert_cmpuint (read, ==, 1);
      g_assert_no_error (err);
    }

  return NULL;
}

static void
test_pipe_io_concurrent (void)
{
  xoutput_stream_t *out_server;
  xthread_t *writer_server;
  PipeIOOverlapReader rc1, rc2;
  HANDLE server, client;
  xchar_t name[256], c;

  g_snprintf (name, sizeof (name),
              "\\\\.\\pipe\\gtest-io-concurrent-%u", (xuint_t) GetCurrentProcessId ());

  server = CreateNamedPipe (name,
                            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                            PIPE_READMODE_BYTE | PIPE_WAIT,
                            1, 0, 0, 0, NULL);
  xassert (server != INVALID_HANDLE_VALUE);
  xassert (_pipe (writer_pipe, 10, _O_BINARY) == 0);

  client = CreateFile (name, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
  xassert (client != INVALID_HANDLE_VALUE);

  rc1.in = g_win32_input_stream_new (client, TRUE);
  rc1.success = FALSE;
  rc1.cancellable = xcancellable_new ();
  rc1.thread = xthread_new ("reader_client", pipe_io_concurrent_reader_thread, &rc1);

  rc2.in = g_win32_input_stream_new (client, TRUE);
  rc2.success = FALSE;
  rc2.cancellable = xcancellable_new ();
  rc2.thread = xthread_new ("reader_client", pipe_io_concurrent_reader_thread, &rc2);

  /* FIXME: how to synchronize on both reader thread waiting in read,
     before starting the writer thread? */
  g_usleep (G_USEC_PER_SEC / 10);

  out_server = g_win32_output_stream_new (server, TRUE);
  writer_server = xthread_new ("writer_server", pipe_io_concurrent_writer_thread, out_server);

  read (writer_pipe[0], &c, 1);

  xassert (rc1.success ^ rc2.success);

  xcancellable_cancel (rc1.cancellable);
  xcancellable_cancel (rc2.cancellable);

  xthread_join (writer_server);
  xthread_join (rc1.thread);
  xthread_join (rc2.thread);

  xobject_unref (rc1.in);
  xobject_unref (rc2.in);
  xobject_unref (out_server);

  close (writer_pipe[0]);
  close (writer_pipe[1]);
}

static void
readable_cancel (xobject_t *source, xasync_result_t *res, xpointer_t user_data)
{
  xinput_stream_t *in = G_INPUT_STREAM (source);
  xerror_t *err = NULL;
  xssize_t len;

  len = xinput_stream_read_finish (in, res, &err);
  g_assert_cmpint (len, ==, -1);
  g_assert_error (err, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  xerror_free (err);

  xmain_loop_quit (loop);
}

static void
test_pipe_io_cancel (void)
{
  xinput_stream_t *in;
  xoutput_stream_t *out;
  HANDLE in_handle, out_handle;
  xchar_t name[256];

  g_snprintf (name, sizeof (name),
              "\\\\.\\pipe\\gtest-io-cancel-%u", (xuint_t) GetCurrentProcessId ());

  in_handle = CreateNamedPipe (name,
                               PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                               PIPE_READMODE_BYTE | PIPE_WAIT,
                               1, 0, 0, 0, NULL);
  xassert (in_handle != INVALID_HANDLE_VALUE);

  out_handle = CreateFile (name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
  xassert (out_handle != INVALID_HANDLE_VALUE);

  in = g_win32_input_stream_new (in_handle, TRUE);
  out = g_win32_output_stream_new (out_handle, TRUE);

  reader_cancel = xcancellable_new ();
  xinput_stream_read_async (in, main_buf, sizeof (main_buf),
                             G_PRIORITY_DEFAULT, reader_cancel,
                             readable_cancel, out);

  g_timeout_add (500, timeout, reader_cancel);

  loop = xmain_loop_new (NULL, TRUE);
  xmain_loop_run (loop);
  xmain_loop_unref (loop);

  xobject_unref (reader_cancel);
  xobject_unref (in);
  xobject_unref (out);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/win32-streams/pipe-io-test", test_pipe_io);
  g_test_add_func ("/win32-streams/pipe-io-cancel-test", test_pipe_io_cancel);
  g_test_add_func ("/win32-streams/pipe-io-overlap-test", test_pipe_io_overlap);
  g_test_add_func ("/win32-streams/pipe-io-concurrent-test", test_pipe_io_concurrent);

  return g_test_run();
}
