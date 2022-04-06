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

#include <stdlib.h>

#include <glib.h>
#include <string.h>

#include <sys/types.h>

static char *echo_prog_path;
static char *sleep_prog_path;

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#endif

typedef struct
{
  xmain_loop_t *main_loop;
  xint_t *n_alive;  /* (atomic) */
  xint_t ttl;  /* seconds */
  xmain_loop_t *thread_main_loop;  /* (nullable) */
} SpawnChildsData;

static xpid_t
get_a_child (xint_t ttl)
{
  xpid_t pid;

#ifdef G_OS_WIN32
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  xchar_t *cmdline;

  memset (&si, 0, sizeof (si));
  si.cb = sizeof (&si);
  memset (&pi, 0, sizeof (pi));

  cmdline = xstrdup_printf ("%s %d", sleep_prog_path, ttl);

  if (!CreateProcess (NULL, cmdline, NULL, NULL,
                      FALSE, 0, NULL, NULL, &si, &pi))
    xerror ("CreateProcess failed: %s",
             g_win32_error_message (GetLastError ()));

  g_free (cmdline);

  CloseHandle (pi.hThread);
  pid = pi.hProcess;

  return pid;
#else
  pid = fork ();
  if (pid < 0)
    exit (1);

  if (pid > 0)
    return pid;

  sleep (ttl);
  _exit (0);
#endif /* G_OS_WIN32 */
}

static void
child_watch_callback (xpid_t pid, xint_t status, xpointer_t user_data)
{
  SpawnChildsData *data = user_data;

  g_test_message ("Child %" G_PID_FORMAT " (ttl %d) exited, status %d",
                  pid, data->ttl, status);

  g_spawn_close_pid (pid);

  if (g_atomic_int_dec_and_test (data->n_alive))
    xmain_loop_quit (data->main_loop);
  if (data->thread_main_loop != NULL)
    xmain_loop_quit (data->thread_main_loop);
}

static xpointer_t
start_thread (xpointer_t user_data)
{
  xmain_loop_t *new_main_loop;
  xsource_t *source;
  xpid_t pid;
  SpawnChildsData *data = user_data;
  xint_t ttl = data->ttl;
  xmain_context_t *new_main_context = NULL;

  new_main_context = xmain_context_new ();
  new_main_loop = xmain_loop_new (new_main_context, FALSE);
  data->thread_main_loop = new_main_loop;

  pid = get_a_child (ttl);
  source = g_child_watch_source_new (pid);
  xsource_set_callback (source,
                         (xsource_func_t) child_watch_callback, data, NULL);
  xsource_attach (source, xmain_loop_get_context (new_main_loop));
  xsource_unref (source);

  g_test_message ("Created pid: %" G_PID_FORMAT " (ttl %d)", pid, ttl);

  xmain_loop_run (new_main_loop);
  xmain_loop_unref (new_main_loop);
  xmain_context_unref (new_main_context);

  return NULL;
}

static xboolean_t
quit_loop (xpointer_t data)
{
  xmain_loop_t *main_loop = data;

  xmain_loop_quit (main_loop);

  return TRUE;
}

static void
test_spawn_childs (void)
{
  xpid_t pid;
  xmain_loop_t *main_loop = NULL;
  SpawnChildsData child1_data = { 0, }, child2_data = { 0, };
  xint_t n_alive;
  xuint_t timeout_id;

  main_loop = xmain_loop_new (NULL, FALSE);

#ifdef G_OS_WIN32
  system ("cd .");
#else
  system ("true");
#endif

  n_alive = 2;
  timeout_id = g_timeout_add_seconds (30, quit_loop, main_loop);

  child1_data.main_loop = main_loop;
  child1_data.ttl = 1;
  child1_data.n_alive = &n_alive;
  pid = get_a_child (child1_data.ttl);
  g_child_watch_add (pid,
                     (GChildWatchFunc) child_watch_callback,
                     &child1_data);

  child2_data.main_loop = main_loop;
  child2_data.ttl = 2;
  child2_data.n_alive = &n_alive;
  pid = get_a_child (child2_data.ttl);
  g_child_watch_add (pid,
                     (GChildWatchFunc) child_watch_callback,
                     &child2_data);

  xmain_loop_run (main_loop);
  xmain_loop_unref (main_loop);
  xsource_remove (timeout_id);

  g_assert_cmpint (g_atomic_int_get (&n_alive), ==, 0);
}

static void
test_spawn_childs_threads (void)
{
  xmain_loop_t *main_loop = NULL;
  SpawnChildsData thread1_data = { 0, }, thread2_data = { 0, };
  xint_t n_alive;
  xuint_t timeout_id;
  xthread_t *thread1, *thread2;

  main_loop = xmain_loop_new (NULL, FALSE);

#ifdef G_OS_WIN32
  system ("cd .");
#else
  system ("true");
#endif

  n_alive = 2;
  timeout_id = g_timeout_add_seconds (30, quit_loop, main_loop);

  thread1_data.main_loop = main_loop;
  thread1_data.n_alive = &n_alive;
  thread1_data.ttl = 1;  /* seconds */
  thread1 = xthread_new (NULL, start_thread, &thread1_data);

  thread2_data.main_loop = main_loop;
  thread2_data.n_alive = &n_alive;
  thread2_data.ttl = 2;  /* seconds */
  thread2 = xthread_new (NULL, start_thread, &thread2_data);

  xmain_loop_run (main_loop);
  xmain_loop_unref (main_loop);
  xsource_remove (timeout_id);

  g_assert_cmpint (g_atomic_int_get (&n_alive), ==, 0);

  xthread_join (g_steal_pointer (&thread2));
  xthread_join (g_steal_pointer (&thread1));
}

static void
multithreaded_test_run (GThreadFunc function)
{
  xuint_t i;
  xptr_array_t *threads = xptr_array_new ();
  xuint_t n_threads;

  /* Limit to 64, otherwise we may hit file descriptor limits and such */
  n_threads = MIN (g_get_num_processors () * 2, 64);

  for (i = 0; i < n_threads; i++)
    {
      xthread_t *thread;

      thread = xthread_new ("test", function, GUINT_TO_POINTER (i));
      xptr_array_add (threads, thread);
    }

  for (i = 0; i < n_threads; i++)
    {
      xpointer_t ret;
      ret = xthread_join (xptr_array_index (threads, i));
      g_assert_cmpint (GPOINTER_TO_UINT (ret), ==, i);
    }
  xptr_array_free (threads, TRUE);
}

static xpointer_t
test_spawn_sync_multithreaded_instance (xpointer_t data)
{
  xuint_t tnum = GPOINTER_TO_UINT (data);
  xerror_t *error = NULL;
  xptr_array_t *argv;
  char *arg;
  char *stdout_str;
  int estatus;

  arg = xstrdup_printf ("thread %u", tnum);

  argv = xptr_array_new ();
  xptr_array_add (argv, echo_prog_path);
  xptr_array_add (argv, arg);
  xptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char**)argv->pdata, NULL, G_SPAWN_DEFAULT, NULL, NULL, &stdout_str, NULL, &estatus, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (arg, ==, stdout_str);
  g_free (arg);
  g_free (stdout_str);
  xptr_array_free (argv, TRUE);

  return GUINT_TO_POINTER (tnum);
}

static void
test_spawn_sync_multithreaded (void)
{
  multithreaded_test_run (test_spawn_sync_multithreaded_instance);
}

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
  GIOStatus status;
  SpawnAsyncMultithreadedData *data = datap;

 read:
  status = g_io_channel_read_chars (channel, buf, sizeof (buf), &bytes_read, &error);
  if (status == G_IO_STATUS_NORMAL)
    {
      xstring_append_len (data->stdout_buf, buf, (xssize_t) bytes_read);
      if (bytes_read == sizeof (buf))
	goto read;
    }
  else if (status == G_IO_STATUS_EOF)
    {
      xstring_append_len (data->stdout_buf, buf, (xssize_t) bytes_read);
      data->stdout_done = TRUE;
    }
  else if (status == G_IO_STATUS_ERROR)
    {
      xerror ("Error reading from child stdin: %s", error->message);
    }

  if (data->child_exited && data->stdout_done)
    xmain_loop_quit (data->loop);

  return !data->stdout_done;
}

static xpointer_t
test_spawn_async_multithreaded_instance (xpointer_t thread_data)
{
  xuint_t tnum = GPOINTER_TO_UINT (thread_data);
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

  arg = xstrdup_printf ("thread %u", tnum);

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
  source = g_io_create_watch (channel, G_IO_IN | G_IO_HUP);
  xsource_set_callback (source, (xsource_func_t)on_child_stdout, &data, NULL);
  xsource_attach (source, context);
  xsource_unref (source);

  xmain_loop_run (loop);

  g_assert_true (data.child_exited);
  g_assert_true (data.stdout_done);
  g_assert_cmpstr (data.stdout_buf->str, ==, arg);
  xstring_free (data.stdout_buf, TRUE);

  g_io_channel_unref (channel);
  xmain_context_unref (context);
  xmain_loop_unref (loop);

  g_free (arg);

  return GUINT_TO_POINTER (tnum);
}

static void
test_spawn_async_multithreaded (void)
{
  multithreaded_test_run (test_spawn_async_multithreaded_instance);
}

int
main (int   argc,
      char *argv[])
{
  char *dirname;
  int ret;

  g_test_init (&argc, &argv, NULL);

  dirname = g_path_get_dirname (argv[0]);
  echo_prog_path = g_build_filename (dirname, "test-spawn-echo" EXEEXT, NULL);
  sleep_prog_path = g_build_filename (dirname, "test-spawn-sleep" EXEEXT, NULL);
  g_free (dirname);

  g_assert (xfile_test (echo_prog_path, XFILE_TEST_EXISTS));
#ifdef G_OS_WIN32
  g_assert (xfile_test (sleep_prog_path, XFILE_TEST_EXISTS));
#endif

  g_test_add_func ("/gthread/spawn-childs", test_spawn_childs);
  g_test_add_func ("/gthread/spawn-childs-threads", test_spawn_childs_threads);
  g_test_add_func ("/gthread/spawn-sync", test_spawn_sync_multithreaded);
  g_test_add_func ("/gthread/spawn-async", test_spawn_async_multithreaded);

  ret = g_test_run();

  g_free (echo_prog_path);
  g_free (sleep_prog_path);

  return ret;
}
