#include <gio/gio.h>
#include <string.h>

#ifdef G_OS_UNIX
#include <sys/wait.h>
#include <glib-unix.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <gio/gfiledescriptorbased.h>
#include <unistd.h>
#include <fcntl.h>
#endif

/* We write 2^1 + 2^2 ... + 2^10 or 2047 copies of "Hello World!\n"
 * ultimately
 */
#define TOTAL_HELLOS 2047
#define HELLO_WORLD "hello world!\n"

#ifdef G_OS_WIN32
#define LINEEND "\r\n"
#define EXEEXT ".exe"
#define SPLICELEN (TOTAL_HELLOS * (strlen (HELLO_WORLD) + 1)) /* because \r */
#else
#define LINEEND "\n"
#define EXEEXT
#define SPLICELEN (TOTAL_HELLOS * strlen (HELLO_WORLD))
#endif



#ifdef G_OS_WIN32
#define TESTPROG "gsubprocess-testprog.exe"
#else
#define TESTPROG "gsubprocess-testprog"
#endif

static xptr_array_t *
get_test_subprocess_args (const char *mode,
                          ...) G_GNUC_NULL_TERMINATED;

static xptr_array_t *
get_test_subprocess_args (const char *mode,
                          ...)
{
  xptr_array_t *ret;
  char *path;
  va_list args;
  xpointer_t arg;

  ret = xptr_array_new_with_free_func (g_free);

  path = g_test_build_filename (G_TEST_BUILT, TESTPROG, NULL);
  xptr_array_add (ret, path);
  xptr_array_add (ret, xstrdup (mode));

  va_start (args, mode);
  while ((arg = va_arg (args, xpointer_t)) != NULL)
    xptr_array_add (ret, xstrdup (arg));
  va_end (args);

  xptr_array_add (ret, NULL);
  return ret;
}

static void
test_noop (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xptr_array_t *args;
  xsubprocess_t *proc;

  args = get_test_subprocess_args ("noop", NULL);
  proc = xsubprocess_newv ((const xchar_t * const *) args->pdata, G_SUBPROCESS_FLAGS_NONE, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  xsubprocess_wait_check (proc, NULL, error);
  g_assert_no_error (local_error);
  g_assert_true (xsubprocess_get_successful (proc));

  xobject_unref (proc);
}

static void
check_ready (xobject_t      *source,
             xasync_result_t *res,
             xpointer_t      user_data)
{
  xboolean_t ret;
  xerror_t *error = NULL;

  ret = xsubprocess_wait_check_finish (G_SUBPROCESS (source),
                                        res,
                                        &error);
  g_assert_true (ret);
  g_assert_no_error (error);

  xobject_unref (source);
}

static void
test_noop_all_to_null (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xptr_array_t *args;
  xsubprocess_t *proc;

  args = get_test_subprocess_args ("noop", NULL);
  proc = xsubprocess_newv ((const xchar_t * const *) args->pdata,
                            G_SUBPROCESS_FLAGS_STDOUT_SILENCE | G_SUBPROCESS_FLAGS_STDERR_SILENCE,
                            error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  xsubprocess_wait_check_async (proc, NULL, check_ready, NULL);
}

static void
test_noop_no_wait (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xptr_array_t *args;
  xsubprocess_t *proc;

  args = get_test_subprocess_args ("noop", NULL);
  proc = xsubprocess_newv ((const xchar_t * const *) args->pdata, G_SUBPROCESS_FLAGS_NONE, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  xobject_unref (proc);
}

static void
test_noop_stdin_inherit (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xptr_array_t *args;
  xsubprocess_t *proc;

  args = get_test_subprocess_args ("noop", NULL);
  proc = xsubprocess_newv ((const xchar_t * const *) args->pdata, G_SUBPROCESS_FLAGS_STDIN_INHERIT, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  xsubprocess_wait_check (proc, NULL, error);
  g_assert_no_error (local_error);

  xobject_unref (proc);
}

#ifdef G_OS_UNIX
static void
test_search_path (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_t *proc;

  proc = xsubprocess_new (G_SUBPROCESS_FLAGS_NONE, error, "true", NULL);
  g_assert_no_error (local_error);

  xsubprocess_wait_check (proc, NULL, error);
  g_assert_no_error (local_error);

  xobject_unref (proc);
}

static void
test_search_path_from_envp (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_launcher_t *launcher;
  xsubprocess_t *proc;
  const char *path;

  path = g_test_get_dir (G_TEST_BUILT);

  launcher = xsubprocess_launcher_new (G_SUBPROCESS_FLAGS_SEARCH_PATH_FROM_ENVP);
  xsubprocess_launcher_setenv (launcher, "PATH", path, TRUE);

  proc = xsubprocess_launcher_spawn (launcher, error, TESTPROG, "exit1", NULL);
  g_assert_no_error (local_error);
  xobject_unref (launcher);

  xsubprocess_wait_check (proc, NULL, error);
  g_assert_error (local_error, G_SPAWN_EXIT_ERROR, 1);
  g_clear_error (error);

  xobject_unref (proc);
}
#endif

static void
test_exit1 (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xptr_array_t *args;
  xsubprocess_t *proc;

  args = get_test_subprocess_args ("exit1", NULL);
  proc = xsubprocess_newv ((const xchar_t * const *) args->pdata, G_SUBPROCESS_FLAGS_NONE, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  xsubprocess_wait_check (proc, NULL, error);
  g_assert_error (local_error, G_SPAWN_EXIT_ERROR, 1);
  g_clear_error (error);

  xobject_unref (proc);
}

typedef struct {
  xmain_loop_t    *loop;
  xcancellable_t *cancellable;
  xboolean_t      cb_called;
} TestExit1CancelData;

static xboolean_t
test_exit1_cancel_idle_quit_cb (xpointer_t user_data)
{
  xmain_loop_t *loop = user_data;
  xmain_loop_quit (loop);
  return G_SOURCE_REMOVE;
}

static void
test_exit1_cancel_wait_check_cb (xobject_t      *source,
                                 xasync_result_t *result,
                                 xpointer_t      user_data)
{
  xsubprocess_t *subprocess = G_SUBPROCESS (source);
  TestExit1CancelData *data = user_data;
  xboolean_t ret;
  xerror_t *error = NULL;

  g_assert_false (data->cb_called);
  data->cb_called = TRUE;

  ret = xsubprocess_wait_check_finish (subprocess, result, &error);
  g_assert_false (ret);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);

  g_idle_add (test_exit1_cancel_idle_quit_cb, data->loop);
}

static void
test_exit1_cancel (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xptr_array_t *args;
  xsubprocess_t *proc;
  TestExit1CancelData data = { 0 };

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=786456");

  args = get_test_subprocess_args ("exit1", NULL);
  proc = xsubprocess_newv ((const xchar_t * const *) args->pdata, G_SUBPROCESS_FLAGS_NONE, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  data.loop = xmain_loop_new (NULL, FALSE);
  data.cancellable = g_cancellable_new ();
  xsubprocess_wait_check_async (proc, data.cancellable, test_exit1_cancel_wait_check_cb, &data);

  xsubprocess_wait_check (proc, NULL, error);
  g_assert_error (local_error, G_SPAWN_EXIT_ERROR, 1);
  g_clear_error (error);

  g_cancellable_cancel (data.cancellable);
  xmain_loop_run (data.loop);

  xobject_unref (proc);
  xmain_loop_unref (data.loop);
  g_clear_object (&data.cancellable);
}

static void
test_exit1_cancel_in_cb_wait_check_cb (xobject_t      *source,
                                       xasync_result_t *result,
                                       xpointer_t      user_data)
{
  xsubprocess_t *subprocess = G_SUBPROCESS (source);
  TestExit1CancelData *data = user_data;
  xboolean_t ret;
  xerror_t *error = NULL;

  g_assert_false (data->cb_called);
  data->cb_called = TRUE;

  ret = xsubprocess_wait_check_finish (subprocess, result, &error);
  g_assert_false (ret);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 1);
  g_clear_error (&error);

  g_cancellable_cancel (data->cancellable);

  g_idle_add (test_exit1_cancel_idle_quit_cb, data->loop);
}

static void
test_exit1_cancel_in_cb (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xptr_array_t *args;
  xsubprocess_t *proc;
  TestExit1CancelData data = { 0 };

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=786456");

  args = get_test_subprocess_args ("exit1", NULL);
  proc = xsubprocess_newv ((const xchar_t * const *) args->pdata, G_SUBPROCESS_FLAGS_NONE, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  data.loop = xmain_loop_new (NULL, FALSE);
  data.cancellable = g_cancellable_new ();
  xsubprocess_wait_check_async (proc, data.cancellable, test_exit1_cancel_in_cb_wait_check_cb, &data);

  xsubprocess_wait_check (proc, NULL, error);
  g_assert_error (local_error, G_SPAWN_EXIT_ERROR, 1);
  g_clear_error (error);

  xmain_loop_run (data.loop);

  xobject_unref (proc);
  xmain_loop_unref (data.loop);
  g_clear_object (&data.cancellable);
}

static xchar_t *
splice_to_string (xinput_stream_t   *stream,
                  xerror_t        **error)
{
  xmemory_output_stream_t *buffer = NULL;
  char *ret = NULL;

  buffer = (xmemory_output_stream_t*)g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  if (xoutput_stream_splice ((xoutput_stream_t*)buffer, stream, 0, NULL, error) < 0)
    goto out;

  if (!xoutput_stream_write ((xoutput_stream_t*)buffer, "\0", 1, NULL, error))
    goto out;

  if (!xoutput_stream_close ((xoutput_stream_t*)buffer, NULL, error))
    goto out;

  ret = g_memory_output_stream_steal_data (buffer);
 out:
  g_clear_object (&buffer);
  return ret;
}

static void
test_echo1 (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_t *proc;
  xptr_array_t *args;
  xinput_stream_t *stdout_stream;
  xchar_t *result;

  args = get_test_subprocess_args ("echo", "hello", "world!", NULL);
  proc = xsubprocess_newv ((const xchar_t * const *) args->pdata, G_SUBPROCESS_FLAGS_STDOUT_PIPE, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  stdout_stream = xsubprocess_get_stdout_pipe (proc);

  result = splice_to_string (stdout_stream, error);
  g_assert_no_error (local_error);

  g_assert_cmpstr (result, ==, "hello" LINEEND "world!" LINEEND);

  g_free (result);
  xobject_unref (proc);
}

#ifdef G_OS_UNIX
static void
test_echo_merged (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_t *proc;
  xptr_array_t *args;
  xinput_stream_t *stdout_stream;
  xchar_t *result;

  args = get_test_subprocess_args ("echo-stdout-and-stderr", "merge", "this", NULL);
  proc = xsubprocess_newv ((const xchar_t * const *) args->pdata,
                            G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_MERGE,
                            error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  stdout_stream = xsubprocess_get_stdout_pipe (proc);
  result = splice_to_string (stdout_stream, error);
  g_assert_no_error (local_error);

  g_assert_cmpstr (result, ==, "merge\nmerge\nthis\nthis\n");

  g_free (result);
  xobject_unref (proc);
}
#endif

typedef struct {
  xuint_t events_pending;
  xmain_loop_t *loop;
} TestCatData;

static void
test_cat_on_input_splice_complete (xobject_t      *object,
                                   xasync_result_t *result,
                                   xpointer_t      user_data)
{
  TestCatData *data = user_data;
  xerror_t *error = NULL;

  (void)xoutput_stream_splice_finish ((xoutput_stream_t*)object, result, &error);
  g_assert_no_error (error);

  data->events_pending--;
  if (data->events_pending == 0)
    xmain_loop_quit (data->loop);
}

static void
test_cat_utf8 (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_t *proc;
  xptr_array_t *args;
  xbytes_t *input_buf;
  xbytes_t *output_buf;
  xinput_stream_t *input_buf_stream = NULL;
  xoutput_stream_t *output_buf_stream = NULL;
  xoutput_stream_t *stdin_stream = NULL;
  xinput_stream_t *stdout_stream = NULL;
  TestCatData data;

  memset (&data, 0, sizeof (data));
  data.loop = xmain_loop_new (NULL, TRUE);

  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_newv ((const xchar_t * const *) args->pdata,
                            G_SUBPROCESS_FLAGS_STDIN_PIPE | G_SUBPROCESS_FLAGS_STDOUT_PIPE,
                            error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  stdin_stream = xsubprocess_get_stdin_pipe (proc);
  stdout_stream = xsubprocess_get_stdout_pipe (proc);

  input_buf = xbytes_new_static ("hello, world!", strlen ("hello, world!"));
  input_buf_stream = g_memory_input_stream_new_from_bytes (input_buf);
  xbytes_unref (input_buf);

  output_buf_stream = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);

  xoutput_stream_splice_async (stdin_stream, input_buf_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                G_PRIORITY_DEFAULT, NULL, test_cat_on_input_splice_complete,
                                &data);
  data.events_pending++;
  xoutput_stream_splice_async (output_buf_stream, stdout_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                G_PRIORITY_DEFAULT, NULL, test_cat_on_input_splice_complete,
                                &data);
  data.events_pending++;

  xmain_loop_run (data.loop);

  xsubprocess_wait_check (proc, NULL, error);
  g_assert_no_error (local_error);

  output_buf = g_memory_output_stream_steal_as_bytes ((xmemory_output_stream_t*)output_buf_stream);

  g_assert_cmpmem (xbytes_get_data (output_buf, NULL),
                   xbytes_get_size (output_buf),
                   "hello, world!", 13);

  xbytes_unref (output_buf);
  xmain_loop_unref (data.loop);
  xobject_unref (input_buf_stream);
  xobject_unref (output_buf_stream);
  xobject_unref (proc);
}

static xpointer_t
cancel_soon (xpointer_t user_data)
{
  xcancellable_t *cancellable = user_data;

  g_usleep (G_TIME_SPAN_SECOND);
  g_cancellable_cancel (cancellable);
  xobject_unref (cancellable);

  return NULL;
}

static void
test_cat_eof (void)
{
  xcancellable_t *cancellable;
  xerror_t *error = NULL;
  xsubprocess_t *cat;
  xboolean_t result;
  xchar_t buffer;
  xssize_t s;

#ifdef G_OS_WIN32
  g_test_skip ("This test has not been ported to Win32");
  return;
#endif

  /* Spawn 'cat' */
  cat = xsubprocess_new (G_SUBPROCESS_FLAGS_STDIN_PIPE | G_SUBPROCESS_FLAGS_STDOUT_PIPE, &error, "cat", NULL);
  g_assert_no_error (error);
  g_assert_nonnull (cat);

  /* Make sure that reading stdout blocks (until we cancel) */
  cancellable = g_cancellable_new ();
  xthread_unref (xthread_new ("cancel thread", cancel_soon, xobject_ref (cancellable)));
  s = xinput_stream_read (xsubprocess_get_stdout_pipe (cat), &buffer, sizeof buffer, cancellable, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_cmpint (s, ==, -1);
  xobject_unref (cancellable);
  g_clear_error (&error);

  /* Close the stream (EOF on cat's stdin) */
  result = xoutput_stream_close (xsubprocess_get_stdin_pipe (cat), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (result);

  /* Now check that reading cat's stdout gets us an EOF (since it quit) */
  s = xinput_stream_read (xsubprocess_get_stdout_pipe (cat), &buffer, sizeof buffer, NULL, &error);
  g_assert_no_error (error);
  g_assert_false (s);

  /* Check that the process has exited as a result of the EOF */
  result = xsubprocess_wait (cat, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (xsubprocess_get_if_exited (cat));
  g_assert_cmpint (xsubprocess_get_exit_status (cat), ==, 0);
  g_assert_true (result);

  xobject_unref (cat);
}

typedef struct {
  xuint_t events_pending;
  xboolean_t caught_error;
  xerror_t *error;
  xmain_loop_t *loop;

  xint_t counter;
  xoutput_stream_t *first_stdin;
} TestMultiSpliceData;

static void
on_one_multi_splice_done (xobject_t       *obj,
                          xasync_result_t  *res,
                          xpointer_t       user_data)
{
  TestMultiSpliceData *data = user_data;

  if (!data->caught_error)
    {
      if (xoutput_stream_splice_finish ((xoutput_stream_t*)obj, res, &data->error) < 0)
        data->caught_error = TRUE;
    }

  data->events_pending--;
  if (data->events_pending == 0)
    xmain_loop_quit (data->loop);
}

static xboolean_t
on_idle_multisplice (xpointer_t     user_data)
{
  TestMultiSpliceData *data = user_data;

  if (data->counter >= TOTAL_HELLOS || data->caught_error)
    {
      if (!xoutput_stream_close (data->first_stdin, NULL, &data->error))
        data->caught_error = TRUE;
      data->events_pending--;
      if (data->events_pending == 0)
        {
          xmain_loop_quit (data->loop);
        }
      return FALSE;
    }
  else
    {
      int i;
      for (i = 0; i < data->counter; i++)
        {
          xsize_t bytes_written;
          if (!xoutput_stream_write_all (data->first_stdin, HELLO_WORLD,
                                          strlen (HELLO_WORLD), &bytes_written,
                                          NULL, &data->error))
            {
              data->caught_error = TRUE;
              return FALSE;
            }
        }
      data->counter *= 2;
      return TRUE;
    }
}

static void
on_subprocess_exited (xobject_t         *object,
                      xasync_result_t    *result,
                      xpointer_t         user_data)
{
  xsubprocess_t *subprocess = G_SUBPROCESS (object);
  TestMultiSpliceData *data = user_data;
  xerror_t *error = NULL;

  if (!xsubprocess_wait_finish (subprocess, result, &error))
    {
      if (!data->caught_error)
        {
          data->caught_error = TRUE;
          g_propagate_error (&data->error, error);
        }
    }
  g_spawn_check_wait_status (xsubprocess_get_status (subprocess), &error);
  g_assert_no_error (error);
  data->events_pending--;
  if (data->events_pending == 0)
    xmain_loop_quit (data->loop);
}

static void
test_multi_1 (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xptr_array_t *args;
  xsubprocess_launcher_t *launcher;
  xsubprocess_t *first;
  xsubprocess_t *second;
  xsubprocess_t *third;
  xoutput_stream_t *first_stdin;
  xinput_stream_t *first_stdout;
  xoutput_stream_t *second_stdin;
  xinput_stream_t *second_stdout;
  xoutput_stream_t *third_stdin;
  xinput_stream_t *third_stdout;
  xoutput_stream_t *membuf;
  TestMultiSpliceData data;
  int splice_flags = G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET;

  args = get_test_subprocess_args ("cat", NULL);
  launcher = xsubprocess_launcher_new (G_SUBPROCESS_FLAGS_STDIN_PIPE | G_SUBPROCESS_FLAGS_STDOUT_PIPE);
  first = xsubprocess_launcher_spawnv (launcher, (const xchar_t * const *) args->pdata, error);
  g_assert_no_error (local_error);
  second = xsubprocess_launcher_spawnv (launcher, (const xchar_t * const *) args->pdata, error);
  g_assert_no_error (local_error);
  third = xsubprocess_launcher_spawnv (launcher, (const xchar_t * const *) args->pdata, error);
  g_assert_no_error (local_error);

  xptr_array_free (args, TRUE);

  membuf = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);

  first_stdin = xsubprocess_get_stdin_pipe (first);
  first_stdout = xsubprocess_get_stdout_pipe (first);
  second_stdin = xsubprocess_get_stdin_pipe (second);
  second_stdout = xsubprocess_get_stdout_pipe (second);
  third_stdin = xsubprocess_get_stdin_pipe (third);
  third_stdout = xsubprocess_get_stdout_pipe (third);

  memset (&data, 0, sizeof (data));
  data.loop = xmain_loop_new (NULL, TRUE);
  data.counter = 1;
  data.first_stdin = first_stdin;

  data.events_pending++;
  xoutput_stream_splice_async (second_stdin, first_stdout, splice_flags, G_PRIORITY_DEFAULT,
                                NULL, on_one_multi_splice_done, &data);
  data.events_pending++;
  xoutput_stream_splice_async (third_stdin, second_stdout, splice_flags, G_PRIORITY_DEFAULT,
                                NULL, on_one_multi_splice_done, &data);
  data.events_pending++;
  xoutput_stream_splice_async (membuf, third_stdout, splice_flags, G_PRIORITY_DEFAULT,
                                NULL, on_one_multi_splice_done, &data);

  data.events_pending++;
  g_timeout_add (250, on_idle_multisplice, &data);

  data.events_pending++;
  xsubprocess_wait_async (first, NULL, on_subprocess_exited, &data);
  data.events_pending++;
  xsubprocess_wait_async (second, NULL, on_subprocess_exited, &data);
  data.events_pending++;
  xsubprocess_wait_async (third, NULL, on_subprocess_exited, &data);

  xmain_loop_run (data.loop);

  g_assert_false (data.caught_error);
  g_assert_no_error (data.error);

  g_assert_cmpint (g_memory_output_stream_get_data_size ((xmemory_output_stream_t*)membuf), ==, SPLICELEN);

  xmain_loop_unref (data.loop);
  xobject_unref (membuf);
  xobject_unref (launcher);
  xobject_unref (first);
  xobject_unref (second);
  xobject_unref (third);
}

typedef struct {
  xsubprocess_flags_t flags;
  xboolean_t is_utf8;
  xboolean_t running;
  xerror_t *error;
} TestAsyncCommunicateData;

static void
on_communicate_complete (xobject_t               *proc,
                         xasync_result_t          *result,
                         xpointer_t               user_data)
{
  TestAsyncCommunicateData *data = user_data;
  xbytes_t *stdout_bytes = NULL, *stderr_bytes = NULL;
  char *stdout_str = NULL, *stderr_str = NULL;
  const xuint8_t *stdout_data;
  xsize_t stdout_len;

  data->running = FALSE;
  if (data->is_utf8)
    (void) xsubprocess_communicate_utf8_finish ((xsubprocess_t*)proc, result,
                                                 &stdout_str, &stderr_str, &data->error);
  else
    (void) xsubprocess_communicate_finish ((xsubprocess_t*)proc, result,
                                            &stdout_bytes, &stderr_bytes, &data->error);
  if (data->error)
      return;

  if (data->flags & G_SUBPROCESS_FLAGS_STDOUT_PIPE)
    {
      if (data->is_utf8)
        {
          stdout_data = (xuint8_t*)stdout_str;
          stdout_len = strlen (stdout_str);
        }
      else
        stdout_data = xbytes_get_data (stdout_bytes, &stdout_len);

      g_assert_cmpmem (stdout_data, stdout_len, "# hello world" LINEEND, 13 + strlen (LINEEND));
    }
  else
    {
      g_assert_null (stdout_str);
      g_assert_null (stdout_bytes);
    }

  if (data->flags & G_SUBPROCESS_FLAGS_STDERR_PIPE)
    {
      if (data->is_utf8)
        g_assert_nonnull (stderr_str);
      else
        g_assert_nonnull (stderr_bytes);
    }
  else
    {
      g_assert_null (stderr_str);
      g_assert_null (stderr_bytes);
    }

  g_clear_pointer (&stdout_bytes, xbytes_unref);
  g_clear_pointer (&stderr_bytes, xbytes_unref);
  g_free (stdout_str);
  g_free (stderr_str);
}

/* Test xsubprocess_communicate_async() works correctly with a variety of flags,
 * as passed in via @test_data. */
static void
test_communicate_async (xconstpointer test_data)
{
  xsubprocess_flags_t flags = GPOINTER_TO_INT (test_data);
  xerror_t *error = NULL;
  xptr_array_t *args;
  TestAsyncCommunicateData data = { flags, 0, 0, NULL };
  xsubprocess_t *proc;
  xcancellable_t *cancellable = NULL;
  xbytes_t *input;
  const char *hellostring;

  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_newv ((const xchar_t* const*)args->pdata,
                            G_SUBPROCESS_FLAGS_STDIN_PIPE | flags,
                            &error);
  g_assert_no_error (error);
  xptr_array_free (args, TRUE);

  /* Include a leading hash and trailing newline so that if this gets onto the
   * test’s stdout, it doesn’t mess up TAP output. */
  hellostring = "# hello world\n";
  input = xbytes_new_static (hellostring, strlen (hellostring));

  xsubprocess_communicate_async (proc, input,
                                  cancellable,
                                  on_communicate_complete,
                                  &data);

  data.running = TRUE;
  while (data.running)
    xmain_context_iteration (NULL, TRUE);

  g_assert_no_error (data.error);

  xbytes_unref (input);
  xobject_unref (proc);
}

/* Test xsubprocess_communicate() works correctly with a variety of flags,
 * as passed in via @test_data. */
static void
test_communicate (xconstpointer test_data)
{
  xsubprocess_flags_t flags = GPOINTER_TO_INT (test_data);
  xerror_t *error = NULL;
  xptr_array_t *args;
  xsubprocess_t *proc;
  xcancellable_t *cancellable = NULL;
  xbytes_t *input;
  const xchar_t *hellostring;
  xbytes_t *stdout_bytes, *stderr_bytes;
  const xchar_t *stdout_data;
  xsize_t stdout_len;

  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_newv ((const xchar_t* const*)args->pdata,
                            G_SUBPROCESS_FLAGS_STDIN_PIPE | flags,
                            &error);
  g_assert_no_error (error);
  xptr_array_free (args, TRUE);

  /* Include a leading hash and trailing newline so that if this gets onto the
   * test’s stdout, it doesn’t mess up TAP output. */
  hellostring = "# hello world\n";
  input = xbytes_new_static (hellostring, strlen (hellostring));

  xsubprocess_communicate (proc, input, cancellable, &stdout_bytes, &stderr_bytes, &error);
  g_assert_no_error (error);

  if (flags & G_SUBPROCESS_FLAGS_STDOUT_PIPE)
    {
      stdout_data = xbytes_get_data (stdout_bytes, &stdout_len);
      g_assert_cmpmem (stdout_data, stdout_len, "# hello world" LINEEND, 13 + strlen (LINEEND));
    }
  else
    g_assert_null (stdout_bytes);
  if (flags & G_SUBPROCESS_FLAGS_STDERR_PIPE)
    g_assert_nonnull (stderr_bytes);
  else
    g_assert_null (stderr_bytes);

  xbytes_unref (input);
  g_clear_pointer (&stdout_bytes, xbytes_unref);
  g_clear_pointer (&stderr_bytes, xbytes_unref);
  xobject_unref (proc);
}

typedef struct {
  xsubprocess_t  *proc;
  xcancellable_t *cancellable;
  xboolean_t is_utf8;
  xboolean_t running;
  xerror_t *error;
} TestCancelledCommunicateData;

static xboolean_t
on_test_communicate_cancelled_idle (xpointer_t user_data)
{
  TestCancelledCommunicateData *data = user_data;
  xbytes_t *input;
  const xchar_t *hellostring;
  xbytes_t *stdout_bytes = NULL, *stderr_bytes = NULL;
  xchar_t *stdout_buf = NULL, *stderr_buf = NULL;

  /* Include a leading hash and trailing newline so that if this gets onto the
   * test’s stdout, it doesn’t mess up TAP output. */
  hellostring = "# hello world\n";
  input = xbytes_new_static (hellostring, strlen (hellostring));

  if (data->is_utf8)
    xsubprocess_communicate_utf8 (data->proc, hellostring, data->cancellable,
                                   &stdout_buf, &stderr_buf, &data->error);
  else
    xsubprocess_communicate (data->proc, input, data->cancellable, &stdout_bytes,
                              &stderr_bytes, &data->error);

  data->running = FALSE;

  if (data->is_utf8)
    {
      g_assert_null (stdout_buf);
      g_assert_null (stderr_buf);
    }
  else
    {
      g_assert_null (stdout_bytes);
      g_assert_null (stderr_bytes);
    }

  xbytes_unref (input);

  return G_SOURCE_REMOVE;
}

/* Test xsubprocess_communicate() can be cancelled correctly */
static void
test_communicate_cancelled (xconstpointer test_data)
{
  xsubprocess_flags_t flags = GPOINTER_TO_INT (test_data);
  xptr_array_t *args;
  xsubprocess_t *proc;
  xcancellable_t *cancellable = NULL;
  xerror_t *error = NULL;
  TestCancelledCommunicateData data = { 0 };

  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_newv ((const xchar_t* const*)args->pdata,
                            G_SUBPROCESS_FLAGS_STDIN_PIPE | flags,
                            &error);
  g_assert_no_error (error);
  xptr_array_free (args, TRUE);

  cancellable = g_cancellable_new ();

  data.proc = proc;
  data.cancellable = cancellable;
  data.error = error;

  g_cancellable_cancel (cancellable);
  g_idle_add (on_test_communicate_cancelled_idle, &data);

  data.running = TRUE;
  while (data.running)
    xmain_context_iteration (NULL, TRUE);

  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&data.error);

  xobject_unref (cancellable);
  xobject_unref (proc);
}

static void
on_communicate_cancelled_complete (xobject_t               *proc,
                                   xasync_result_t          *result,
                                   xpointer_t               user_data)
{
  TestAsyncCommunicateData *data = user_data;
  xbytes_t *stdout_bytes = NULL, *stderr_bytes = NULL;
  char *stdout_str = NULL, *stderr_str = NULL;

  data->running = FALSE;
  if (data->is_utf8)
    (void) xsubprocess_communicate_utf8_finish ((xsubprocess_t*)proc, result,
                                                 &stdout_str, &stderr_str, &data->error);
  else
    (void) xsubprocess_communicate_finish ((xsubprocess_t*)proc, result,
                                            &stdout_bytes, &stderr_bytes, &data->error);

  if (data->is_utf8)
    {
      g_assert_null (stdout_str);
      g_assert_null (stderr_str);
    }
  else
    {
      g_assert_null (stdout_bytes);
      g_assert_null (stderr_bytes);
    }
}

/* Test xsubprocess_communicate_async() can be cancelled correctly,
 * as passed in via @test_data. */
static void
test_communicate_cancelled_async (xconstpointer test_data)
{
  xsubprocess_flags_t flags = GPOINTER_TO_INT (test_data);
  xerror_t *error = NULL;
  xptr_array_t *args;
  TestAsyncCommunicateData data = { 0 };
  xsubprocess_t *proc;
  xcancellable_t *cancellable = NULL;
  xbytes_t *input;
  const char *hellostring;

  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_newv ((const xchar_t* const*)args->pdata,
                            G_SUBPROCESS_FLAGS_STDIN_PIPE | flags,
                            &error);
  g_assert_no_error (error);
  xptr_array_free (args, TRUE);

  /* Include a leading hash and trailing newline so that if this gets onto the
   * test’s stdout, it doesn’t mess up TAP output. */
  hellostring = "# hello world\n";
  input = xbytes_new_static (hellostring, strlen (hellostring));

  cancellable = g_cancellable_new ();

  xsubprocess_communicate_async (proc, input,
                                  cancellable,
                                  on_communicate_cancelled_complete,
                                  &data);

  g_cancellable_cancel (cancellable);

  data.running = TRUE;
  while (data.running)
    xmain_context_iteration (NULL, TRUE);

  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&data.error);

  xbytes_unref (input);
  xobject_unref (cancellable);
  xobject_unref (proc);
}

/* Test xsubprocess_communicate_utf8_async() works correctly with a variety of
 * flags, as passed in via @test_data. */
static void
test_communicate_utf8_async (xconstpointer test_data)
{
  xsubprocess_flags_t flags = GPOINTER_TO_INT (test_data);
  xerror_t *error = NULL;
  xptr_array_t *args;
  TestAsyncCommunicateData data = { flags, 0, 0, NULL };
  xsubprocess_t *proc;
  xcancellable_t *cancellable = NULL;

  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_newv ((const xchar_t* const*)args->pdata,
                            G_SUBPROCESS_FLAGS_STDIN_PIPE | flags,
                            &error);
  g_assert_no_error (error);
  xptr_array_free (args, TRUE);

  data.is_utf8 = TRUE;
  xsubprocess_communicate_utf8_async (proc, "# hello world\n",
                                       cancellable,
                                       on_communicate_complete,
                                       &data);

  data.running = TRUE;
  while (data.running)
    xmain_context_iteration (NULL, TRUE);

  g_assert_no_error (data.error);

  xobject_unref (proc);
}

/* Test xsubprocess_communicate_utf8_async() can be cancelled correctly. */
static void
test_communicate_utf8_cancelled_async (xconstpointer test_data)
{
  xsubprocess_flags_t flags = GPOINTER_TO_INT (test_data);
  xerror_t *error = NULL;
  xptr_array_t *args;
  TestAsyncCommunicateData data = { flags, 0, 0, NULL };
  xsubprocess_t *proc;
  xcancellable_t *cancellable = NULL;

  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_newv ((const xchar_t* const*)args->pdata,
                            G_SUBPROCESS_FLAGS_STDIN_PIPE | flags,
                            &error);
  g_assert_no_error (error);
  xptr_array_free (args, TRUE);

  cancellable = g_cancellable_new ();
  data.is_utf8 = TRUE;
  xsubprocess_communicate_utf8_async (proc, "# hello world\n",
                                       cancellable,
                                       on_communicate_cancelled_complete,
                                       &data);

  g_cancellable_cancel (cancellable);

  data.running = TRUE;
  while (data.running)
    xmain_context_iteration (NULL, TRUE);

  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&data.error);

  xobject_unref (cancellable);
  xobject_unref (proc);
}

/* Test xsubprocess_communicate_utf8() works correctly with a variety of flags,
 * as passed in via @test_data. */
static void
test_communicate_utf8 (xconstpointer test_data)
{
  xsubprocess_flags_t flags = GPOINTER_TO_INT (test_data);
  xerror_t *error = NULL;
  xptr_array_t *args;
  xsubprocess_t *proc;
  xcancellable_t *cancellable = NULL;
  const xchar_t *stdin_buf;
  xchar_t *stdout_buf, *stderr_buf;

  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_newv ((const xchar_t* const*)args->pdata,
                            G_SUBPROCESS_FLAGS_STDIN_PIPE | flags,
                            &error);
  g_assert_no_error (error);
  xptr_array_free (args, TRUE);

  /* Include a leading hash and trailing newline so that if this gets onto the
   * test’s stdout, it doesn’t mess up TAP output. */
  stdin_buf = "# hello world\n";

  xsubprocess_communicate_utf8 (proc, stdin_buf, cancellable, &stdout_buf, &stderr_buf, &error);
  g_assert_no_error (error);

  if (flags & G_SUBPROCESS_FLAGS_STDOUT_PIPE)
    g_assert_cmpstr (stdout_buf, ==, "# hello world" LINEEND);
  else
    g_assert_null (stdout_buf);
  if (flags & G_SUBPROCESS_FLAGS_STDERR_PIPE)
    g_assert_nonnull (stderr_buf);
  else     g_assert_null (stderr_buf);

  g_free (stdout_buf);
  g_free (stderr_buf);
  xobject_unref (proc);
}

/* Test xsubprocess_communicate_utf8() can be cancelled correctly */
static void
test_communicate_utf8_cancelled (xconstpointer test_data)
{
  xsubprocess_flags_t flags = GPOINTER_TO_INT (test_data);
  xptr_array_t *args;
  xsubprocess_t *proc;
  xcancellable_t *cancellable = NULL;
  xerror_t *error = NULL;
  TestCancelledCommunicateData data = { 0 };

  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_newv ((const xchar_t* const*)args->pdata,
                            G_SUBPROCESS_FLAGS_STDIN_PIPE | flags,
                            &error);
  g_assert_no_error (error);
  xptr_array_free (args, TRUE);

  cancellable = g_cancellable_new ();

  data.proc = proc;
  data.cancellable = cancellable;
  data.error = error;

  g_cancellable_cancel (cancellable);
  g_idle_add (on_test_communicate_cancelled_idle, &data);

  data.is_utf8 = TRUE;
  data.running = TRUE;
  while (data.running)
    xmain_context_iteration (NULL, TRUE);

  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&data.error);

  xobject_unref (cancellable);
  xobject_unref (proc);
}

static void
test_communicate_nothing (void)
{
  xerror_t *error = NULL;
  xptr_array_t *args;
  xsubprocess_t *proc;
  xcancellable_t *cancellable = NULL;
  xchar_t *stdout_buf;

  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_newv ((const xchar_t* const*)args->pdata,
                            G_SUBPROCESS_FLAGS_STDIN_PIPE
                            | G_SUBPROCESS_FLAGS_STDOUT_PIPE
                            | G_SUBPROCESS_FLAGS_STDERR_MERGE,
                            &error);
  g_assert_no_error (error);
  xptr_array_free (args, TRUE);

  xsubprocess_communicate_utf8 (proc, "", cancellable, &stdout_buf, NULL, &error);
  g_assert_no_error (error);

  g_assert_cmpstr (stdout_buf, ==, "");

  g_free (stdout_buf);

  xobject_unref (proc);
}

static void
test_communicate_utf8_async_invalid (void)
{
  xsubprocess_flags_t flags = G_SUBPROCESS_FLAGS_STDOUT_PIPE;
  xerror_t *error = NULL;
  xptr_array_t *args;
  TestAsyncCommunicateData data = { flags, 0, 0, NULL };
  xsubprocess_t *proc;
  xcancellable_t *cancellable = NULL;

  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_newv ((const xchar_t* const*)args->pdata,
                            G_SUBPROCESS_FLAGS_STDIN_PIPE | flags,
                            &error);
  g_assert_no_error (error);
  xptr_array_free (args, TRUE);

  data.is_utf8 = TRUE;
  xsubprocess_communicate_utf8_async (proc, "\xFF\xFF",
                                       cancellable,
                                       on_communicate_complete,
                                       &data);

  data.running = TRUE;
  while (data.running)
    xmain_context_iteration (NULL, TRUE);

  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_FAILED);
  xerror_free (data.error);

  xobject_unref (proc);
}

/* Test that invalid UTF-8 received using xsubprocess_communicate_utf8()
 * results in an error. */
static void
test_communicate_utf8_invalid (void)
{
  xsubprocess_flags_t flags = G_SUBPROCESS_FLAGS_STDOUT_PIPE;
  xerror_t *local_error = NULL;
  xboolean_t ret;
  xptr_array_t *args;
  xchar_t *stdout_str = NULL, *stderr_str = NULL;
  xsubprocess_t *proc;

  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_newv ((const xchar_t* const*)args->pdata,
                            G_SUBPROCESS_FLAGS_STDIN_PIPE | flags,
                            &local_error);
  g_assert_no_error (local_error);
  xptr_array_free (args, TRUE);

  ret = xsubprocess_communicate_utf8 (proc, "\xFF\xFF", NULL,
                                       &stdout_str, &stderr_str, &local_error);
  g_assert_error (local_error, G_IO_ERROR, G_IO_ERROR_FAILED);
  xerror_free (local_error);
  g_assert_false (ret);

  g_assert_null (stdout_str);
  g_assert_null (stderr_str);

  xobject_unref (proc);
}

static xboolean_t
send_terminate (xpointer_t   user_data)
{
  xsubprocess_t *proc = user_data;

  xsubprocess_force_exit (proc);

  return FALSE;
}

static void
on_request_quit_exited (xobject_t        *object,
                        xasync_result_t   *result,
                        xpointer_t        user_data)
{
  xsubprocess_t *subprocess = G_SUBPROCESS (object);
  xerror_t *error = NULL;

  xsubprocess_wait_finish (subprocess, result, &error);
  g_assert_no_error (error);
#ifdef G_OS_UNIX
  g_assert_true (xsubprocess_get_if_signaled (subprocess));
  g_assert_cmpint (xsubprocess_get_term_sig (subprocess), ==, 9);
#endif
  g_spawn_check_wait_status (xsubprocess_get_status (subprocess), &error);
  g_assert_nonnull (error);
  g_clear_error (&error);

  xmain_loop_quit ((xmain_loop_t*)user_data);
}

static void
test_terminate (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_t *proc;
  xptr_array_t *args;
  xmain_loop_t *loop;
  const xchar_t *id;

  args = get_test_subprocess_args ("sleep-forever", NULL);
  proc = xsubprocess_newv ((const xchar_t * const *) args->pdata, G_SUBPROCESS_FLAGS_NONE, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  id = xsubprocess_get_identifier (proc);
  g_assert_nonnull (id);

  loop = xmain_loop_new (NULL, TRUE);

  xsubprocess_wait_async (proc, NULL, on_request_quit_exited, loop);

  g_timeout_add_seconds (3, send_terminate, proc);

  xmain_loop_run (loop);

  xmain_loop_unref (loop);
  xobject_unref (proc);
}

#ifdef G_OS_UNIX
static xboolean_t
send_signal (xpointer_t user_data)
{
  xsubprocess_t *proc = user_data;

  xsubprocess_send_signal (proc, SIGKILL);

  return FALSE;
}

static void
test_signal (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_t *proc;
  xptr_array_t *args;
  xmain_loop_t *loop;

  args = get_test_subprocess_args ("sleep-forever", NULL);
  proc = xsubprocess_newv ((const xchar_t * const *) args->pdata, G_SUBPROCESS_FLAGS_NONE, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  loop = xmain_loop_new (NULL, TRUE);

  xsubprocess_wait_async (proc, NULL, on_request_quit_exited, loop);

  g_timeout_add_seconds (3, send_signal, proc);

  xmain_loop_run (loop);

  xmain_loop_unref (loop);
  xobject_unref (proc);
}
#endif

static void
test_env (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_launcher_t *launcher;
  xsubprocess_t *proc;
  xptr_array_t *args;
  xinput_stream_t *stdout_stream;
  xchar_t *result;
  xchar_t *envp[] = { NULL, "ONE=1", "TWO=1", "THREE=3", "FOUR=1", NULL };
  xchar_t **split;

  envp[0] = xstrdup_printf ("PATH=%s", g_getenv ("PATH"));
  args = get_test_subprocess_args ("env", NULL);
  launcher = xsubprocess_launcher_new (G_SUBPROCESS_FLAGS_NONE);
  xsubprocess_launcher_set_flags (launcher, G_SUBPROCESS_FLAGS_STDOUT_PIPE);
  xsubprocess_launcher_set_environ (launcher, envp);
  xsubprocess_launcher_setenv (launcher, "TWO", "2", TRUE);
  xsubprocess_launcher_setenv (launcher, "THREE", "1", FALSE);
  xsubprocess_launcher_unsetenv (launcher, "FOUR");

  g_assert_null (xsubprocess_launcher_getenv (launcher, "FOUR"));

  proc = xsubprocess_launcher_spawn (launcher, error, args->pdata[0], "env", NULL);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);
  g_free (envp[0]);

  stdout_stream = xsubprocess_get_stdout_pipe (proc);

  result = splice_to_string (stdout_stream, error);
  split = xstrsplit (result, LINEEND, -1);
  g_assert_cmpstr (g_environ_getenv (split, "ONE"), ==, "1");
  g_assert_cmpstr (g_environ_getenv (split, "TWO"), ==, "2");
  g_assert_cmpstr (g_environ_getenv (split, "THREE"), ==, "3");
  g_assert_null (g_environ_getenv (split, "FOUR"));

  xstrfreev (split);
  g_free (result);
  xobject_unref (proc);
  xobject_unref (launcher);
}

/* Test that explicitly inheriting and modifying the parent process’
 * environment works. */
static void
test_env_inherit (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_launcher_t *launcher;
  xsubprocess_t *proc;
  xptr_array_t *args;
  xinput_stream_t *stdout_stream;
  xchar_t *result;
  xchar_t **split;

  g_setenv ("TEST_ENV_INHERIT1", "1", TRUE);
  g_setenv ("TEST_ENV_INHERIT2", "2", TRUE);

  args = get_test_subprocess_args ("env", NULL);
  launcher = xsubprocess_launcher_new (G_SUBPROCESS_FLAGS_NONE);
  xsubprocess_launcher_set_flags (launcher, G_SUBPROCESS_FLAGS_STDOUT_PIPE);
  xsubprocess_launcher_set_environ (launcher, NULL);
  xsubprocess_launcher_setenv (launcher, "TWO", "2", TRUE);
  xsubprocess_launcher_unsetenv (launcher, "TEST_ENV_INHERIT1");

  g_assert_null (xsubprocess_launcher_getenv (launcher, "TEST_ENV_INHERIT1"));
  g_assert_cmpstr (xsubprocess_launcher_getenv (launcher, "TEST_ENV_INHERIT2"), ==, "2");
  g_assert_cmpstr (xsubprocess_launcher_getenv (launcher, "TWO"), ==, "2");

  proc = xsubprocess_launcher_spawn (launcher, error, args->pdata[0], "env", NULL);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  stdout_stream = xsubprocess_get_stdout_pipe (proc);

  result = splice_to_string (stdout_stream, error);
  split = xstrsplit (result, LINEEND, -1);
  g_assert_null (g_environ_getenv (split, "TEST_ENV_INHERIT1"));
  g_assert_cmpstr (g_environ_getenv (split, "TEST_ENV_INHERIT2"), ==, "2");
  g_assert_cmpstr (g_environ_getenv (split, "TWO"), ==, "2");

  xstrfreev (split);
  g_free (result);
  xobject_unref (proc);
  xobject_unref (launcher);
}

static void
test_cwd (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_launcher_t *launcher;
  xsubprocess_t *proc;
  xptr_array_t *args;
  xinput_stream_t *stdout_stream;
  xchar_t *result;
  const char *basename;
  xchar_t *tmp_lineend;
  const xchar_t *tmp_lineend_basename;

  args = get_test_subprocess_args ("cwd", NULL);
  launcher = xsubprocess_launcher_new (G_SUBPROCESS_FLAGS_STDOUT_PIPE);
  xsubprocess_launcher_set_flags (launcher, G_SUBPROCESS_FLAGS_STDOUT_PIPE);
  xsubprocess_launcher_set_cwd (launcher, g_get_tmp_dir ());
  tmp_lineend = xstrdup_printf ("%s%s", g_get_tmp_dir (), LINEEND);
  tmp_lineend_basename = xstrrstr (tmp_lineend, G_DIR_SEPARATOR_S);

  proc = xsubprocess_launcher_spawnv (launcher, (const char * const *)args->pdata, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  stdout_stream = xsubprocess_get_stdout_pipe (proc);

  result = splice_to_string (stdout_stream, error);

  basename = xstrrstr (result, G_DIR_SEPARATOR_S);
  g_assert_nonnull (basename);
  g_assert_cmpstr (basename, ==, tmp_lineend_basename);
  g_free (tmp_lineend);

  g_free (result);
  xobject_unref (proc);
  xobject_unref (launcher);
}
#ifdef G_OS_UNIX

static void
test_subprocess_launcher_close (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_launcher_t *launcher;
  xsubprocess_t *proc;
  xptr_array_t *args;
  int fd, fd2;
  xboolean_t is_open;

  /* Open two arbitrary FDs. One of them, @fd, will be transferred to the
   * launcher, and the other’s FD integer will be used as its target FD, giving
   * the mapping `fd → fd2` if a child process were to be spawned.
   *
   * The launcher will then be closed, which should close @fd but *not* @fd2,
   * as the value of @fd2 is only valid as an FD in a child process. (A child
   * process is not actually spawned in this test.)
   */
  fd = dup (0);
  fd2 = dup (0);
  launcher = xsubprocess_launcher_new (G_SUBPROCESS_FLAGS_NONE);
  xsubprocess_launcher_take_fd (launcher, fd, fd2);

  is_open = fcntl (fd, F_GETFD) != -1;
  g_assert_true (is_open);
  is_open = fcntl (fd2, F_GETFD) != -1;
  g_assert_true (is_open);

  xsubprocess_launcher_close (launcher);

  is_open = fcntl (fd, F_GETFD) != -1;
  g_assert_false (is_open);
  is_open = fcntl (fd2, F_GETFD) != -1;
  g_assert_true (is_open);

  /* Now test that actually trying to spawn the child gives %G_IO_ERROR_CLOSED,
   * as xsubprocess_launcher_close() has been called. */
  args = get_test_subprocess_args ("cat", NULL);
  proc = xsubprocess_launcher_spawnv (launcher, (const xchar_t * const *) args->pdata, error);
  xptr_array_free (args, TRUE);
  g_assert_null (proc);
  g_assert_error (local_error, G_IO_ERROR, G_IO_ERROR_CLOSED);
  g_clear_error (error);

  close (fd2);
  xobject_unref (launcher);
}

static void
test_stdout_file (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_launcher_t *launcher;
  xsubprocess_t *proc;
  xptr_array_t *args;
  xfile_t *tmpfile;
  xfile_io_stream_t *iostream;
  xoutput_stream_t *stdin_stream;
  const char *test_data = "this is some test data\n";
  char *tmp_contents;
  char *tmp_file_path;

  tmpfile = xfile_new_tmp ("gsubprocessXXXXXX", &iostream, error);
  g_assert_no_error (local_error);
  g_clear_object (&iostream);

  tmp_file_path = xfile_get_path (tmpfile);

  args = get_test_subprocess_args ("cat", NULL);
  launcher = xsubprocess_launcher_new (G_SUBPROCESS_FLAGS_STDIN_PIPE);
  xsubprocess_launcher_set_stdout_file_path (launcher, tmp_file_path);
  proc = xsubprocess_launcher_spawnv (launcher, (const xchar_t * const *) args->pdata, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  stdin_stream = xsubprocess_get_stdin_pipe (proc);

  xoutput_stream_write_all (stdin_stream, test_data, strlen (test_data), NULL, NULL, error);
  g_assert_no_error (local_error);

  xoutput_stream_close (stdin_stream, NULL, error);
  g_assert_no_error (local_error);

  xsubprocess_wait_check (proc, NULL, error);

  xobject_unref (launcher);
  xobject_unref (proc);

  xfile_load_contents (tmpfile, NULL, &tmp_contents, NULL, NULL, error);
  g_assert_no_error (local_error);

  g_assert_cmpstr (test_data, ==, tmp_contents);
  g_free (tmp_contents);

  (void) xfile_delete (tmpfile, NULL, NULL);
  xobject_unref (tmpfile);
  g_free (tmp_file_path);
}

static void
test_stdout_fd (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_launcher_t *launcher;
  xsubprocess_t *proc;
  xptr_array_t *args;
  xfile_t *tmpfile;
  xfile_io_stream_t *iostream;
  xfile_descriptor_based_t *descriptor_stream;
  xoutput_stream_t *stdin_stream;
  const char *test_data = "this is some test data\n";
  char *tmp_contents;

  tmpfile = xfile_new_tmp ("gsubprocessXXXXXX", &iostream, error);
  g_assert_no_error (local_error);

  args = get_test_subprocess_args ("cat", NULL);
  launcher = xsubprocess_launcher_new (G_SUBPROCESS_FLAGS_STDIN_PIPE);
  descriptor_stream = XFILE_DESCRIPTOR_BASED (g_io_stream_get_output_stream (XIO_STREAM (iostream)));
  xsubprocess_launcher_take_stdout_fd (launcher, dup (xfile_descriptor_based_get_fd (descriptor_stream)));
  proc = xsubprocess_launcher_spawnv (launcher, (const xchar_t * const *) args->pdata, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  g_clear_object (&iostream);

  stdin_stream = xsubprocess_get_stdin_pipe (proc);

  xoutput_stream_write_all (stdin_stream, test_data, strlen (test_data), NULL, NULL, error);
  g_assert_no_error (local_error);

  xoutput_stream_close (stdin_stream, NULL, error);
  g_assert_no_error (local_error);

  xsubprocess_wait_check (proc, NULL, error);

  xobject_unref (launcher);
  xobject_unref (proc);

  xfile_load_contents (tmpfile, NULL, &tmp_contents, NULL, NULL, error);
  g_assert_no_error (local_error);

  g_assert_cmpstr (test_data, ==, tmp_contents);
  g_free (tmp_contents);

  (void) xfile_delete (tmpfile, NULL, NULL);
  xobject_unref (tmpfile);
}

static void
child_setup (xpointer_t user_data)
{
  dup2 (GPOINTER_TO_INT (user_data), 1);
}

static void
test_child_setup (void)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xsubprocess_launcher_t *launcher;
  xsubprocess_t *proc;
  xptr_array_t *args;
  xfile_t *tmpfile;
  xfile_io_stream_t *iostream;
  xoutput_stream_t *stdin_stream;
  const char *test_data = "this is some test data\n";
  char *tmp_contents;
  int fd;

  tmpfile = xfile_new_tmp ("gsubprocessXXXXXX", &iostream, error);
  g_assert_no_error (local_error);

  fd = xfile_descriptor_based_get_fd (XFILE_DESCRIPTOR_BASED (g_io_stream_get_output_stream (XIO_STREAM (iostream))));

  args = get_test_subprocess_args ("cat", NULL);
  launcher = xsubprocess_launcher_new (G_SUBPROCESS_FLAGS_STDIN_PIPE);
  xsubprocess_launcher_set_child_setup (launcher, child_setup, GINT_TO_POINTER (fd), NULL);
  proc = xsubprocess_launcher_spawnv (launcher, (const xchar_t * const *) args->pdata, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  g_clear_object (&iostream);

  stdin_stream = xsubprocess_get_stdin_pipe (proc);

  xoutput_stream_write_all (stdin_stream, test_data, strlen (test_data), NULL, NULL, error);
  g_assert_no_error (local_error);

  xoutput_stream_close (stdin_stream, NULL, error);
  g_assert_no_error (local_error);

  xsubprocess_wait_check (proc, NULL, error);

  xobject_unref (launcher);
  xobject_unref (proc);

  xfile_load_contents (tmpfile, NULL, &tmp_contents, NULL, NULL, error);
  g_assert_no_error (local_error);

  g_assert_cmpstr (test_data, ==, tmp_contents);
  g_free (tmp_contents);

  (void) xfile_delete (tmpfile, NULL, NULL);
  xobject_unref (tmpfile);
}

static void
do_test_pass_fd (xsubprocess_flags_t     flags,
                 GSpawnChildSetupFunc child_setup)
{
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xinput_stream_t *child_input;
  xdata_input_stream_t *child_datainput;
  xsubprocess_launcher_t *launcher;
  xsubprocess_t *proc;
  xptr_array_t *args;
  int basic_pipefds[2];
  int needdup_pipefds[2];
  char *buf;
  xsize_t len;
  char *basic_fd_str;
  char *needdup_fd_str;

  g_unix_open_pipe (basic_pipefds, FD_CLOEXEC, error);
  g_assert_no_error (local_error);
  g_unix_open_pipe (needdup_pipefds, FD_CLOEXEC, error);
  g_assert_no_error (local_error);

  basic_fd_str = xstrdup_printf ("%d", basic_pipefds[1]);
  needdup_fd_str = xstrdup_printf ("%d", needdup_pipefds[1] + 1);

  args = get_test_subprocess_args ("write-to-fds", basic_fd_str, needdup_fd_str, NULL);
  launcher = xsubprocess_launcher_new (flags);
  xsubprocess_launcher_take_fd (launcher, basic_pipefds[1], basic_pipefds[1]);
  xsubprocess_launcher_take_fd (launcher, needdup_pipefds[1], needdup_pipefds[1] + 1);
  if (child_setup != NULL)
    xsubprocess_launcher_set_child_setup (launcher, child_setup, NULL, NULL);
  proc = xsubprocess_launcher_spawnv (launcher, (const xchar_t * const *) args->pdata, error);
  xptr_array_free (args, TRUE);
  g_assert_no_error (local_error);

  g_free (basic_fd_str);
  g_free (needdup_fd_str);

  child_input = g_unix_input_stream_new (basic_pipefds[0], TRUE);
  child_datainput = g_data_input_stream_new (child_input);
  buf = g_data_input_stream_read_line_utf8 (child_datainput, &len, NULL, error);
  g_assert_no_error (local_error);
  g_assert_cmpstr (buf, ==, "hello world");
  xobject_unref (child_datainput);
  xobject_unref (child_input);
  g_free (buf);

  child_input = g_unix_input_stream_new (needdup_pipefds[0], TRUE);
  child_datainput = g_data_input_stream_new (child_input);
  buf = g_data_input_stream_read_line_utf8 (child_datainput, &len, NULL, error);
  g_assert_no_error (local_error);
  g_assert_cmpstr (buf, ==, "hello world");
  g_free (buf);
  xobject_unref (child_datainput);
  xobject_unref (child_input);

  xobject_unref (launcher);
  xobject_unref (proc);
}

static void
test_pass_fd (void)
{
  do_test_pass_fd (G_SUBPROCESS_FLAGS_NONE, NULL);
}

static void
empty_child_setup (xpointer_t user_data)
{
}

static void
test_pass_fd_empty_child_setup (void)
{
  /* Using a child setup function forces gspawn to use fork/exec
   * rather than posix_spawn.
   */
  do_test_pass_fd (G_SUBPROCESS_FLAGS_NONE, empty_child_setup);
}

static void
test_pass_fd_inherit_fds (void)
{
  /* Try to test the optimized posix_spawn codepath instead of
   * fork/exec. Currently this requires using INHERIT_FDS since gspawn's
   * posix_spawn codepath does not currently handle closing
   * non-inherited fds. Note that using INHERIT_FDS means our testing of
   * xsubprocess_launcher_take_fd() is less-comprehensive than when
   * using G_SUBPROCESS_FLAGS_NONE.
   */
  do_test_pass_fd (G_SUBPROCESS_FLAGS_INHERIT_FDS, NULL);
}

static void
do_test_fd_conflation (xsubprocess_flags_t     flags,
                       GSpawnChildSetupFunc child_setup,
                       xboolean_t             test_child_err_report_fd)
{
  char success_message[] = "Yay success!";
  xerror_t *error = NULL;
  xoutput_stream_t *output_stream;
  xsubprocess_launcher_t *launcher;
  xsubprocess_t *proc;
  xptr_array_t *args;
  int unused_pipefds[2];
  int pipefds[2];
  int fd_to_pass_to_child;
  xsize_t bytes_written;
  xboolean_t success;
  char *fd_str;

  /* This test must run in a new process because it is extremely sensitive to
   * order of opened fds.
   */
  if (!g_test_subprocess ())
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_INHERIT_STDOUT | G_TEST_SUBPROCESS_INHERIT_STDERR);
      g_test_trap_assert_passed ();
      return;
    }

  g_unix_open_pipe (unused_pipefds, FD_CLOEXEC, &error);
  g_assert_no_error (error);

  g_unix_open_pipe (pipefds, FD_CLOEXEC, &error);
  g_assert_no_error (error);

  /* The fds should be sequential since we are in a new process. */
  g_assert_cmpint (unused_pipefds[0] /* 3 */, ==, unused_pipefds[1] - 1);
  g_assert_cmpint (unused_pipefds[1] /* 4 */, ==, pipefds[0] - 1);
  g_assert_cmpint (pipefds[0] /* 5 */, ==, pipefds[1] /* 6 */ - 1);

  /* Because xsubprocess_t allows arbitrary remapping of fds, it has to be careful
   * to avoid fd conflation issues, e.g. it should properly handle 5 -> 4 and
   * 4 -> 5 at the same time. GIO previously attempted to handle this by naively
   * dup'ing the source fds, but this was not good enough because it was
   * possible that the dup'ed result could still conflict with one of the target
   * fds. For example:
   *
   * source_fd 5 -> target_fd 9, source_fd 3 -> target_fd 7
   *
   * dup(5) -> dup returns 8
   * dup(3) -> dup returns 9
   *
   * After dup'ing, we wind up with: 8 -> 9, 9 -> 7. That means that after we
   * dup2(8, 9), we have clobbered fd 9 before we dup2(9, 7). The end result is
   * we have remapped 5 -> 9 as expected, but then remapped 5 -> 7 instead of
   * 3 -> 7 as the application intended.
   *
   * This issue has been fixed in the simplest way possible, by passing a
   * minimum fd value when using F_DUPFD_CLOEXEC that is higher than any of the
   * target fds, to guarantee all source fds are different than all target fds,
   * eliminating any possibility of conflation.
   *
   * Anyway, that is why we have the unused_pipefds here. We need to open fds in
   * a certain order in order to trick older xsubprocess_t into conflating the
   * fds. The primary goal of this test is to ensure this particular conflation
   * issue is not reintroduced. See glib#2503.
   *
   * This test also has an alternate mode of operation where it instead tests
   * for conflation with gspawn's child_err_report_fd, glib#2506.
   *
   * Be aware this test is necessarily extremely fragile. To reproduce these
   * bugs, it relies on internals of gspawn and gmain that will likely change
   * in the future, eventually causing this test to no longer test the bugs
   * it was originally designed to test. That is OK! If the test fails, at
   * least you know *something* is wrong.
   */
  if (test_child_err_report_fd)
    fd_to_pass_to_child = pipefds[1] + 2 /* 8 */;
  else
    fd_to_pass_to_child = pipefds[1] + 3 /* 9 */;

  launcher = xsubprocess_launcher_new (flags);
  xsubprocess_launcher_take_fd (launcher, pipefds[0] /* 5 */, fd_to_pass_to_child);
  xsubprocess_launcher_take_fd (launcher, unused_pipefds[0] /* 3 */, pipefds[1] + 1 /* 7 */);
  if (child_setup != NULL)
    xsubprocess_launcher_set_child_setup (launcher, child_setup, NULL, NULL);
  fd_str = xstrdup_printf ("%d", fd_to_pass_to_child);
  args = get_test_subprocess_args ("read-from-fd", fd_str, NULL);
  proc = xsubprocess_launcher_spawnv (launcher, (const xchar_t * const *) args->pdata, &error);
  g_assert_no_error (error);
  g_assert_nonnull (proc);
  xptr_array_free (args, TRUE);
  xobject_unref (launcher);
  g_free (fd_str);

  /* Close the read ends of the pipes. */
  close (unused_pipefds[0]);
  close (pipefds[0]);

  /* Also close the write end of the unused pipe. */
  close (unused_pipefds[1]);

  /* If doing our normal test:
   *
   * So now pipefds[0] should be inherited into the subprocess as
   * pipefds[1] + 2, and unused_pipefds[0] should be inherited as
   * pipefds[1] + 1. We will write to pipefds[1] and the subprocess will verify
   * that it reads the expected data. But older broken GIO will accidentally
   * clobber pipefds[1] + 2 with pipefds[1] + 1! This will cause the subprocess
   * to hang trying to read from the wrong pipe.
   *
   * If testing conflation with child_err_report_fd:
   *
   * We are actually already done. The real test succeeded if we made it this
   * far without hanging while spawning the child. But let's continue with our
   * write and read anyway, to ensure things are good.
   */
  output_stream = g_unix_output_stream_new (pipefds[1], TRUE);
  success = xoutput_stream_write_all (output_stream,
                                       success_message, sizeof (success_message),
                                       &bytes_written,
                                       NULL,
                                       &error);
  g_assert_no_error (error);
  g_assert_cmpint (bytes_written, ==, sizeof (success_message));
  g_assert_true (success);
  xobject_unref (output_stream);

  success = xsubprocess_wait_check (proc, NULL, &error);
  g_assert_no_error (error);
  xobject_unref (proc);
}

static void
test_fd_conflation (void)
{
  do_test_fd_conflation (G_SUBPROCESS_FLAGS_NONE, NULL, FALSE);
}

static void
test_fd_conflation_empty_child_setup (void)
{
  /* Using a child setup function forces gspawn to use fork/exec
   * rather than posix_spawn.
   */
  do_test_fd_conflation (G_SUBPROCESS_FLAGS_NONE, empty_child_setup, FALSE);
}

static void
test_fd_conflation_inherit_fds (void)
{
  /* Try to test the optimized posix_spawn codepath instead of
   * fork/exec. Currently this requires using INHERIT_FDS since gspawn's
   * posix_spawn codepath does not currently handle closing
   * non-inherited fds.
   */
  do_test_fd_conflation (G_SUBPROCESS_FLAGS_INHERIT_FDS, NULL, FALSE);
}

static void
test_fd_conflation_child_err_report_fd (void)
{
  /* Using a child setup function forces gspawn to use fork/exec
   * rather than posix_spawn.
   */
  do_test_fd_conflation (G_SUBPROCESS_FLAGS_NONE, empty_child_setup, TRUE);
}

#endif

static void
test_launcher_environment (void)
{
  xsubprocess_launcher_t *launcher;
  xerror_t *error = NULL;
  xsubprocess_t *proc;
  xptr_array_t *args;
  xchar_t *out;

  g_setenv ("A", "B", TRUE);
  g_setenv ("C", "D", TRUE);

  launcher = xsubprocess_launcher_new (G_SUBPROCESS_FLAGS_STDOUT_PIPE);

  /* unset a variable */
  xsubprocess_launcher_unsetenv (launcher, "A");

  /* and set a different one */
  xsubprocess_launcher_setenv (launcher, "E", "F", TRUE);

  args = get_test_subprocess_args ("printenv", "A", "C", "E", NULL);
  proc = xsubprocess_launcher_spawnv (launcher, (const xchar_t **) args->pdata, &error);
  g_assert_no_error (error);
  g_assert_nonnull (proc);

  xsubprocess_communicate_utf8 (proc, NULL, NULL, &out, NULL, &error);
  g_assert_no_error (error);

  g_assert_cmpstr (out, ==, "C=D" LINEEND "E=F" LINEEND);
  g_free (out);

  xobject_unref (proc);
  xobject_unref (launcher);
  xptr_array_unref (args);
}

int
main (int argc, char **argv)
{
  const struct
    {
      const xchar_t *subtest;
      xsubprocess_flags_t flags;
    }
  flags_vectors[] =
    {
      { "", G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_MERGE },
      { "/no-pipes", G_SUBPROCESS_FLAGS_NONE },
      { "/separate-stderr", G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE },
      { "/stdout-only", G_SUBPROCESS_FLAGS_STDOUT_PIPE },
      { "/stderr-only", G_SUBPROCESS_FLAGS_STDERR_PIPE },
      { "/stdout-silence", G_SUBPROCESS_FLAGS_STDOUT_SILENCE },
    };
  xsize_t i;

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gsubprocess/noop", test_noop);
  g_test_add_func ("/gsubprocess/noop-all-to-null", test_noop_all_to_null);
  g_test_add_func ("/gsubprocess/noop-no-wait", test_noop_no_wait);
  g_test_add_func ("/gsubprocess/noop-stdin-inherit", test_noop_stdin_inherit);
#ifdef G_OS_UNIX
  g_test_add_func ("/gsubprocess/search-path", test_search_path);
  g_test_add_func ("/gsubprocess/search-path-from-envp", test_search_path_from_envp);
  g_test_add_func ("/gsubprocess/signal", test_signal);
#endif
  g_test_add_func ("/gsubprocess/exit1", test_exit1);
  g_test_add_func ("/gsubprocess/exit1/cancel", test_exit1_cancel);
  g_test_add_func ("/gsubprocess/exit1/cancel_in_cb", test_exit1_cancel_in_cb);
  g_test_add_func ("/gsubprocess/echo1", test_echo1);
#ifdef G_OS_UNIX
  g_test_add_func ("/gsubprocess/echo-merged", test_echo_merged);
#endif
  g_test_add_func ("/gsubprocess/cat-utf8", test_cat_utf8);
  g_test_add_func ("/gsubprocess/cat-eof", test_cat_eof);
  g_test_add_func ("/gsubprocess/multi1", test_multi_1);

  /* Add various tests for xsubprocess_communicate() with different flags. */
  for (i = 0; i < G_N_ELEMENTS (flags_vectors); i++)
    {
      xchar_t *test_path = NULL;

      test_path = xstrdup_printf ("/gsubprocess/communicate%s", flags_vectors[i].subtest);
      g_test_add_data_func (test_path, GINT_TO_POINTER (flags_vectors[i].flags),
                            test_communicate);
      g_free (test_path);

      test_path = xstrdup_printf ("/gsubprocess/communicate/cancelled%s", flags_vectors[i].subtest);
      g_test_add_data_func (test_path, GINT_TO_POINTER (flags_vectors[i].flags),
                            test_communicate_cancelled);
      g_free (test_path);

      test_path = xstrdup_printf ("/gsubprocess/communicate/async%s", flags_vectors[i].subtest);
      g_test_add_data_func (test_path, GINT_TO_POINTER (flags_vectors[i].flags),
                            test_communicate_async);
      g_free (test_path);

      test_path = xstrdup_printf ("/gsubprocess/communicate/async/cancelled%s", flags_vectors[i].subtest);
      g_test_add_data_func (test_path, GINT_TO_POINTER (flags_vectors[i].flags),
                            test_communicate_cancelled_async);
      g_free (test_path);

      test_path = xstrdup_printf ("/gsubprocess/communicate/utf8%s", flags_vectors[i].subtest);
      g_test_add_data_func (test_path, GINT_TO_POINTER (flags_vectors[i].flags),
                            test_communicate_utf8);
      g_free (test_path);

      test_path = xstrdup_printf ("/gsubprocess/communicate/utf8/cancelled%s", flags_vectors[i].subtest);
      g_test_add_data_func (test_path, GINT_TO_POINTER (flags_vectors[i].flags),
                            test_communicate_utf8_cancelled);
      g_free (test_path);

      test_path = xstrdup_printf ("/gsubprocess/communicate/utf8/async%s", flags_vectors[i].subtest);
      g_test_add_data_func (test_path, GINT_TO_POINTER (flags_vectors[i].flags),
                            test_communicate_utf8_async);
      g_free (test_path);

      test_path = xstrdup_printf ("/gsubprocess/communicate/utf8/async/cancelled%s", flags_vectors[i].subtest);
      g_test_add_data_func (test_path, GINT_TO_POINTER (flags_vectors[i].flags),
                            test_communicate_utf8_cancelled_async);
      g_free (test_path);
    }

  g_test_add_func ("/gsubprocess/communicate/utf8/async/invalid", test_communicate_utf8_async_invalid);
  g_test_add_func ("/gsubprocess/communicate/utf8/invalid", test_communicate_utf8_invalid);
  g_test_add_func ("/gsubprocess/communicate/nothing", test_communicate_nothing);
  g_test_add_func ("/gsubprocess/terminate", test_terminate);
  g_test_add_func ("/gsubprocess/env", test_env);
  g_test_add_func ("/gsubprocess/env/inherit", test_env_inherit);
  g_test_add_func ("/gsubprocess/cwd", test_cwd);
#ifdef G_OS_UNIX
  g_test_add_func ("/gsubprocess/launcher-close", test_subprocess_launcher_close);
  g_test_add_func ("/gsubprocess/stdout-file", test_stdout_file);
  g_test_add_func ("/gsubprocess/stdout-fd", test_stdout_fd);
  g_test_add_func ("/gsubprocess/child-setup", test_child_setup);
  g_test_add_func ("/gsubprocess/pass-fd/basic", test_pass_fd);
  g_test_add_func ("/gsubprocess/pass-fd/empty-child-setup", test_pass_fd_empty_child_setup);
  g_test_add_func ("/gsubprocess/pass-fd/inherit-fds", test_pass_fd_inherit_fds);
  g_test_add_func ("/gsubprocess/fd-conflation/basic", test_fd_conflation);
  g_test_add_func ("/gsubprocess/fd-conflation/empty-child-setup", test_fd_conflation_empty_child_setup);
  g_test_add_func ("/gsubprocess/fd-conflation/inherit-fds", test_fd_conflation_inherit_fds);
  g_test_add_func ("/gsubprocess/fd-conflation/child-err-report-fd", test_fd_conflation_child_err_report_fd);
#endif
  g_test_add_func ("/gsubprocess/launcher-environment", test_launcher_environment);

  return g_test_run ();
}
