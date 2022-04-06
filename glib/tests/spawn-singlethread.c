/*
 * Copyright (C) 2011 Red Hat, Inc.
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
 *
 * Author: Colin Walters <walters@verbum.org>
 */

#include "config.h"

#include <glib.h>
#include <locale.h>
#include <string.h>
#include <fcntl.h>
#include <glib/gstdio.h>

#ifdef G_OS_UNIX
#include <glib-unix.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <io.h>
#define LINEEND "\r\n"
#else
#define LINEEND "\n"
#endif

/* MinGW builds are likely done using a BASH-style shell, so run the
 * normal script there, as on non-Windows builds, as it is more likely
 * that one will run 'make check' in such shells to test the code
 */
#if defined (G_OS_WIN32) && defined (_MSC_VER)
#define SCRIPT_EXT ".bat"
#else
#define SCRIPT_EXT
#endif

static char *echo_prog_path;
static char *echo_script_path;

typedef struct {
  xmain_loop_t *loop;
  xboolean_t child_exited;
  xboolean_t stdout_done;
  xstring_t *stdout_buf;
} SpawnAsyncMultithreadedData;

static xboolean_t
on_child_exited (xpid_t     pid,
		 xint_t     status,
		 xpointer_t datap)
{
  SpawnAsyncMultithreadedData *data = datap;

  data->child_exited = TRUE;
  if (data->child_exited && data->stdout_done)
    xmain_loop_quit (data->loop);

  return G_SOURCE_REMOVE;
}

static xboolean_t
on_child_stdout (xio_channel_t   *channel,
		 xio_condition_t  condition,
		 xpointer_t      datap)
{
  char buf[1024];
  xerror_t *error = NULL;
  xsize_t bytes_read;
  SpawnAsyncMultithreadedData *data = datap;

  if (condition & G_IO_IN)
    {
      GIOStatus status;
      status = g_io_channel_read_chars (channel, buf, sizeof (buf), &bytes_read, &error);
      g_assert_no_error (error);
      xstring_append_len (data->stdout_buf, buf, (xssize_t) bytes_read);
      if (status == G_IO_STATUS_EOF)
	data->stdout_done = TRUE;
    }
  if (condition & G_IO_HUP)
    data->stdout_done = TRUE;
  if (condition & G_IO_ERR)
    xerror ("Error reading from child stdin");

  if (data->child_exited && data->stdout_done)
    xmain_loop_quit (data->loop);

  return !data->stdout_done;
}

static void
test_spawn_async (void)
{
  int tnum = 1;
  xerror_t *error = NULL;
  xptr_array_t *argv;
  char *arg;
  xpid_t pid;
  xmain_context_t *context;
  xmain_loop_t *loop;
  xio_channel_t *channel;
  xsource_t *source;
  int child_stdout_fd;
  SpawnAsyncMultithreadedData data;

  context = xmain_context_new ();
  loop = xmain_loop_new (context, TRUE);

  arg = xstrdup_printf ("thread %d", tnum);

  argv = xptr_array_new ();
  xptr_array_add (argv, echo_prog_path);
  xptr_array_add (argv, arg);
  xptr_array_add (argv, NULL);

  g_spawn_async_with_pipes (NULL, (char**)argv->pdata, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL,
			    &child_stdout_fd, NULL, &error);
  g_assert_no_error (error);
  xptr_array_free (argv, TRUE);

  data.loop = loop;
  data.stdout_done = FALSE;
  data.child_exited = FALSE;
  data.stdout_buf = xstring_new (0);

  source = g_child_watch_source_new (pid);
  xsource_set_callback (source, (xsource_func_t)on_child_exited, &data, NULL);
  xsource_attach (source, context);
  xsource_unref (source);

  channel = g_io_channel_unix_new (child_stdout_fd);
  source = g_io_create_watch (channel, G_IO_IN | G_IO_HUP | G_IO_ERR);
  xsource_set_callback (source, (xsource_func_t)on_child_stdout, &data, NULL);
  xsource_attach (source, context);
  xsource_unref (source);

  xmain_loop_run (loop);

  g_assert (data.child_exited);
  g_assert (data.stdout_done);
  g_assert_cmpstr (data.stdout_buf->str, ==, arg);
  xstring_free (data.stdout_buf, TRUE);

  g_io_channel_unref (channel);
  xmain_context_unref (context);
  xmain_loop_unref (loop);

  g_free (arg);
}

/* Windows close() causes failure through the Invalid Parameter Handler
 * Routine if the file descriptor does not exist.
 */
static void
safe_close (int fd)
{
  if (fd >= 0)
    close (fd);
}

/* test_t g_spawn_async_with_fds() with a variety of different inputs */
static void
test_spawn_async_with_fds (void)
{
  int tnum = 1;
  xptr_array_t *argv;
  char *arg;
  xsize_t i;

  /* Each test has 3 variable parameters: stdin, stdout, stderr */
  enum fd_type {
    NO_FD,        /* pass fd -1 (unset) */
    FD_NEGATIVE,  /* pass fd of negative value (equivalent to unset) */
    PIPE,         /* pass fd of new/unique pipe */
    STDOUT_PIPE,  /* pass the same pipe as stdout */
  } tests[][3] = {
    { NO_FD, NO_FD, NO_FD },       /* test_t with no fds passed */
    { NO_FD, FD_NEGATIVE, NO_FD }, /* test_t another negative fd value */
    { PIPE, PIPE, PIPE },          /* test_t with unique fds passed */
    { NO_FD, PIPE, STDOUT_PIPE },  /* test_t the same fd for stdout + stderr */
  };

  arg = xstrdup_printf ("thread %d", tnum);

  argv = xptr_array_new ();
  xptr_array_add (argv, echo_prog_path);
  xptr_array_add (argv, arg);
  xptr_array_add (argv, NULL);

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      xerror_t *error = NULL;
      xpid_t pid;
      xmain_context_t *context;
      xmain_loop_t *loop;
      xio_channel_t *channel = NULL;
      xsource_t *source;
      SpawnAsyncMultithreadedData data;
      enum fd_type *fd_info = tests[i];
      xint_t test_pipe[3][2];
      int j;

      for (j = 0; j < 3; j++)
        {
          switch (fd_info[j])
            {
            case NO_FD:
              test_pipe[j][0] = -1;
              test_pipe[j][1] = -1;
              break;
            case FD_NEGATIVE:
              test_pipe[j][0] = -5;
              test_pipe[j][1] = -5;
              break;
            case PIPE:
#ifdef G_OS_UNIX
              g_unix_open_pipe (test_pipe[j], FD_CLOEXEC, &error);
              g_assert_no_error (error);
#else
              g_assert_cmpint (_pipe (test_pipe[j], 4096, _O_BINARY), >=, 0);
#endif
              break;
            case STDOUT_PIPE:
              g_assert_cmpint (j, ==, 2); /* only works for stderr */
              test_pipe[j][0] = test_pipe[1][0];
              test_pipe[j][1] = test_pipe[1][1];
              break;
            default:
              g_assert_not_reached ();
            }
        }

      context = xmain_context_new ();
      loop = xmain_loop_new (context, TRUE);

      g_spawn_async_with_fds (NULL, (char**)argv->pdata, NULL,
			      G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid,
			      test_pipe[0][0], test_pipe[1][1], test_pipe[2][1],
			      &error);
      g_assert_no_error (error);
      safe_close (test_pipe[0][0]);
      safe_close (test_pipe[1][1]);
      if (fd_info[2] != STDOUT_PIPE)
        safe_close (test_pipe[2][1]);

      data.loop = loop;
      data.stdout_done = FALSE;
      data.child_exited = FALSE;
      data.stdout_buf = xstring_new (0);

      source = g_child_watch_source_new (pid);
      xsource_set_callback (source, (xsource_func_t)on_child_exited, &data, NULL);
      xsource_attach (source, context);
      xsource_unref (source);

      if (test_pipe[1][0] >= 0)
        {
          channel = g_io_channel_unix_new (test_pipe[1][0]);
          source = g_io_create_watch (channel, G_IO_IN | G_IO_HUP | G_IO_ERR);
          xsource_set_callback (source, (xsource_func_t)on_child_stdout,
                                 &data, NULL);
          xsource_attach (source, context);
          xsource_unref (source);
        }
      else
        {
          /* Don't check stdout data if we didn't pass a fd */
          data.stdout_done = TRUE;
        }

      xmain_loop_run (loop);

      g_assert_true (data.child_exited);

      if (test_pipe[1][0] >= 0)
        {
          /* Check for echo on stdout */
          g_assert_true (data.stdout_done);
          g_assert_cmpstr (data.stdout_buf->str, ==, arg);
          g_io_channel_unref (channel);
        }
      xstring_free (data.stdout_buf, TRUE);

      xmain_context_unref (context);
      xmain_loop_unref (loop);
      safe_close (test_pipe[0][1]);
      safe_close (test_pipe[1][0]);
      if (fd_info[2] != STDOUT_PIPE)
        safe_close (test_pipe[2][0]);
    }

  xptr_array_free (argv, TRUE);
  g_free (arg);
}

static void
test_spawn_sync (void)
{
  int tnum = 1;
  xerror_t *error = NULL;
  char *arg = xstrdup_printf ("thread %d", tnum);
  /* Include arguments with special symbols to test that they are correctly passed to child.
   * This is tested on all platforms, but the most prone to failure is win32,
   * where args are specially escaped during spawning.
   */
  const char * const argv[] = {
    echo_prog_path,
    arg,
    "doublequotes\\\"after\\\\\"\"backslashes", /* this would be special escaped on win32 */
    "\\\"\"doublequotes spaced after backslashes\\\\\"", /* this would be special escaped on win32 */
    "even$$dollars",
    "even%%percents",
    "even\"\"doublequotes",
    "even''singlequotes",
    "even\\\\backslashes",
    "even//slashes",
    "$odd spaced$dollars$",
    "%odd spaced%spercents%",
    "\"odd spaced\"doublequotes\"",
    "'odd spaced'singlequotes'",
    "\\odd spaced\\backslashes\\", /* this wasn't handled correctly on win32 in glib <=2.58 */
    "/odd spaced/slashes/",
    NULL
  };
  char *joined_args_str = xstrjoinv ("", (char**)argv + 1);
  char *stdout_str;
  int estatus;

  g_spawn_sync (NULL, (char**)argv, NULL, 0, NULL, NULL, &stdout_str, NULL, &estatus, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (joined_args_str, ==, stdout_str);
  g_free (arg);
  g_free (stdout_str);
  g_free (joined_args_str);
}

/* Like test_spawn_sync but uses spawn flags that trigger the optimized
 * posix_spawn codepath.
 */
static void
test_posix_spawn (void)
{
  int tnum = 1;
  xerror_t *error = NULL;
  xptr_array_t *argv;
  char *arg;
  char *stdout_str;
  int estatus;
  GSpawnFlags flags = G_SPAWN_CLOEXEC_PIPES | G_SPAWN_LEAVE_DESCRIPTORS_OPEN;

  arg = xstrdup_printf ("thread %d", tnum);

  argv = xptr_array_new ();
  xptr_array_add (argv, echo_prog_path);
  xptr_array_add (argv, arg);
  xptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char**)argv->pdata, NULL, flags, NULL, NULL, &stdout_str, NULL, &estatus, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (arg, ==, stdout_str);
  g_free (arg);
  g_free (stdout_str);
  xptr_array_free (argv, TRUE);
}

static void
test_spawn_script (void)
{
  xerror_t *error = NULL;
  xptr_array_t *argv;
  char *stdout_str;
  int estatus;

  argv = xptr_array_new ();
  xptr_array_add (argv, echo_script_path);
  xptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char**)argv->pdata, NULL, 0, NULL, NULL, &stdout_str, NULL, &estatus, &error);
  g_assert_no_error (error);
  g_assert_cmpstr ("echo" LINEEND, ==, stdout_str);
  g_free (stdout_str);
  xptr_array_free (argv, TRUE);
}

/* test_t that spawning a non-existent executable returns %G_SPAWN_ERROR_NOENT. */
static void
test_spawn_nonexistent (void)
{
  xerror_t *error = NULL;
  xptr_array_t *argv = NULL;
  xchar_t *stdout_str = NULL;
  xint_t wait_status = -1;

  argv = xptr_array_new ();
  xptr_array_add (argv, "this does not exist");
  xptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char**) argv->pdata, NULL, 0, NULL, NULL, &stdout_str,
                NULL, &wait_status, &error);
  g_assert_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);
  g_assert_null (stdout_str);
  g_assert_cmpint (wait_status, ==, -1);

  xptr_array_free (argv, TRUE);

  g_clear_error (&error);
}

/* test_t that FD assignments in a spawned process don’t overwrite and break the
 * child_err_report_fd which is used to report error information back from the
 * intermediate child process to the parent.
 *
 * https://gitlab.gnome.org/GNOME/glib/-/issues/2097 */
static void
test_spawn_fd_assignment_clash (void)
{
  int tmp_fd;
  xuint_t i;
#define N_FDS 10
  xint_t source_fds[N_FDS];
  xint_t target_fds[N_FDS];
  const xchar_t *argv[] = { "/nonexistent", NULL };
  xboolean_t retval;
  xerror_t *local_error = NULL;
  struct stat statbuf;

  /* Open a temporary file and duplicate its FD several times so we have several
   * FDs to remap in the child process. */
  tmp_fd = xfile_open_tmp ("glib-spawn-test-XXXXXX", NULL, NULL);
  g_assert_cmpint (tmp_fd, >=, 0);

  for (i = 0; i < (N_FDS - 1); ++i)
    {
      int source;
#ifdef F_DUPFD_CLOEXEC
      source = fcntl (tmp_fd, F_DUPFD_CLOEXEC, 3);
#else
      source = dup (tmp_fd);
#endif
      g_assert_cmpint (source, >=, 0);
      source_fds[i] = source;
      target_fds[i] = source + N_FDS;
    }

  source_fds[i] = tmp_fd;
  target_fds[i] = tmp_fd + N_FDS;

  /* Print out the FD map. */
  g_test_message ("FD map:");
  for (i = 0; i < N_FDS; i++)
    g_test_message (" • %d → %d", source_fds[i], target_fds[i]);

  /* Spawn the subprocess. This should fail because the executable doesn’t
   * exist. */
  retval = g_spawn_async_with_pipes_and_fds (NULL, argv, NULL, G_SPAWN_DEFAULT,
                                             NULL, NULL, -1, -1, -1,
                                             source_fds, target_fds, N_FDS,
                                             NULL, NULL, NULL, NULL,
                                             &local_error);
  g_assert_error (local_error, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);
  g_assert_false (retval);

  g_clear_error (&local_error);

  /* Check nothing was written to the temporary file, as would happen if the FD
   * mapping was messed up to conflict with the child process error reporting FD.
   * See https://gitlab.gnome.org/GNOME/glib/-/issues/2097 */
  g_assert_no_errno (fstat (tmp_fd, &statbuf));
  g_assert_cmpuint (statbuf.st_size, ==, 0);

  /* Clean up. */
  for (i = 0; i < N_FDS; i++)
    g_close (source_fds[i], NULL);
}

int
main (int   argc,
      char *argv[])
{
  char *dirname;
  int ret;

  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  dirname = g_path_get_dirname (argv[0]);
  echo_prog_path = g_build_filename (dirname, "test-spawn-echo" EXEEXT, NULL);
  echo_script_path = g_build_filename (dirname, "echo-script" SCRIPT_EXT, NULL);
  if (!xfile_test (echo_script_path, XFILE_TEST_EXISTS))
    {
      g_free (echo_script_path);
      echo_script_path = g_test_build_filename (G_TEST_DIST, "echo-script" SCRIPT_EXT, NULL);
    }
  g_free (dirname);

  g_assert (xfile_test (echo_prog_path, XFILE_TEST_EXISTS));
  g_assert (xfile_test (echo_script_path, XFILE_TEST_EXISTS));

  g_test_add_func ("/gthread/spawn-single-sync", test_spawn_sync);
  g_test_add_func ("/gthread/spawn-single-async", test_spawn_async);
  g_test_add_func ("/gthread/spawn-single-async-with-fds", test_spawn_async_with_fds);
  g_test_add_func ("/gthread/spawn-script", test_spawn_script);
  g_test_add_func ("/gthread/spawn/nonexistent", test_spawn_nonexistent);
  g_test_add_func ("/gthread/spawn-posix-spawn", test_posix_spawn);
  g_test_add_func ("/gthread/spawn/fd-assignment-clash", test_spawn_fd_assignment_clash);

  ret = g_test_run();

  g_free (echo_script_path);
  g_free (echo_prog_path);

  return ret;
}
