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

#include "glib-unix.h"
#include <string.h>
#include <pwd.h>

static void
test_pipe (void)
{
  xerror_t *error = NULL;
  int pipefd[2];
  char buf[1024];
  xssize_t bytes_read;
  xboolean_t res;

  res = g_unix_open_pipe (pipefd, FD_CLOEXEC, &error);
  g_assert (res);
  g_assert_no_error (error);

  write (pipefd[1], "hello", sizeof ("hello"));
  memset (buf, 0, sizeof (buf));
  bytes_read = read (pipefd[0], buf, sizeof(buf) - 1);
  g_assert_cmpint (bytes_read, >, 0);
  buf[bytes_read] = '\0';

  close (pipefd[0]);
  close (pipefd[1]);

  g_assert (xstr_has_prefix (buf, "hello"));
}

static void
test_error (void)
{
  xerror_t *error = NULL;
  xboolean_t res;

  res = g_unix_set_fd_nonblocking (123456, TRUE, &error);
  g_assert_cmpint (errno, ==, EBADF);
  g_assert (!res);
  g_assert_error (error, G_UNIX_ERROR, 0);
  g_clear_error (&error);
}

static void
test_nonblocking (void)
{
  xerror_t *error = NULL;
  int pipefd[2];
  xboolean_t res;
  int flags;

  res = g_unix_open_pipe (pipefd, FD_CLOEXEC, &error);
  g_assert (res);
  g_assert_no_error (error);

  res = g_unix_set_fd_nonblocking (pipefd[0], TRUE, &error);
  g_assert (res);
  g_assert_no_error (error);

  flags = fcntl (pipefd[0], F_GETFL);
  g_assert_cmpint (flags, !=, -1);
  g_assert (flags & O_NONBLOCK);

  res = g_unix_set_fd_nonblocking (pipefd[0], FALSE, &error);
  g_assert (res);
  g_assert_no_error (error);

  flags = fcntl (pipefd[0], F_GETFL);
  g_assert_cmpint (flags, !=, -1);
  g_assert (!(flags & O_NONBLOCK));

  close (pipefd[0]);
  close (pipefd[1]);
}

static xboolean_t sig_received = FALSE;
static xboolean_t sig_timeout = FALSE;
static int sig_counter = 0;

static xboolean_t
on_sig_received (xpointer_t user_data)
{
  xmain_loop_t *loop = user_data;
  xmain_loop_quit (loop);
  sig_received = TRUE;
  sig_counter ++;
  return G_SOURCE_REMOVE;
}

static xboolean_t
on_sig_timeout (xpointer_t data)
{
  xmain_loop_t *loop = data;
  xmain_loop_quit (loop);
  sig_timeout = TRUE;
  return G_SOURCE_REMOVE;
}

static xboolean_t
exit_mainloop (xpointer_t data)
{
  xmain_loop_t *loop = data;
  xmain_loop_quit (loop);
  return G_SOURCE_REMOVE;
}

static xboolean_t
on_sig_received_2 (xpointer_t data)
{
  xmain_loop_t *loop = data;

  sig_counter ++;
  if (sig_counter == 2)
    xmain_loop_quit (loop);
  return G_SOURCE_REMOVE;
}

static void
test_signal (int signum)
{
  xmain_loop_t *mainloop;
  int id;

  mainloop = xmain_loop_new (NULL, FALSE);

  sig_received = FALSE;
  sig_counter = 0;
  g_unix_signal_add (signum, on_sig_received, mainloop);
  kill (getpid (), signum);
  g_assert (!sig_received);
  id = g_timeout_add (5000, on_sig_timeout, mainloop);
  xmain_loop_run (mainloop);
  g_assert (sig_received);
  sig_received = FALSE;
  xsource_remove (id);

  /* Ensure we don't get double delivery */
  g_timeout_add (500, exit_mainloop, mainloop);
  xmain_loop_run (mainloop);
  g_assert (!sig_received);

  /* Ensure that two sources for the same signal get it */
  sig_counter = 0;
  g_unix_signal_add (signum, on_sig_received_2, mainloop);
  g_unix_signal_add (signum, on_sig_received_2, mainloop);
  id = g_timeout_add (5000, on_sig_timeout, mainloop);

  kill (getpid (), signum);
  xmain_loop_run (mainloop);
  g_assert_cmpint (sig_counter, ==, 2);
  xsource_remove (id);

  xmain_loop_unref (mainloop);
}

static void
test_sighup (void)
{
  test_signal (SIGHUP);
}

static void
test_sigterm (void)
{
  test_signal (SIGTERM);
}

static void
test_sighup_add_remove (void)
{
  xuint_t id;
  struct sigaction action;

  sig_received = FALSE;
  id = g_unix_signal_add (SIGHUP, on_sig_received, NULL);
  xsource_remove (id);

  sigaction (SIGHUP, NULL, &action);
  g_assert (action.sa_handler == SIG_DFL);
}

static xboolean_t
nested_idle (xpointer_t data)
{
  xmain_loop_t *nested;
  xmain_context_t *context;
  xsource_t *source;

  context = xmain_context_new ();
  nested = xmain_loop_new (context, FALSE);

  source = g_unix_signal_source_new (SIGHUP);
  xsource_set_callback (source, on_sig_received, nested, NULL);
  xsource_attach (source, context);
  xsource_unref (source);

  kill (getpid (), SIGHUP);
  xmain_loop_run (nested);
  g_assert_cmpint (sig_counter, ==, 1);

  xmain_loop_unref (nested);
  xmain_context_unref (context);

  return G_SOURCE_REMOVE;
}

static void
test_sighup_nested (void)
{
  xmain_loop_t *mainloop;

  mainloop = xmain_loop_new (NULL, FALSE);

  sig_counter = 0;
  sig_received = FALSE;
  g_unix_signal_add (SIGHUP, on_sig_received, mainloop);
  g_idle_add (nested_idle, mainloop);

  xmain_loop_run (mainloop);
  g_assert_cmpint (sig_counter, ==, 2);

  xmain_loop_unref (mainloop);
}

static xboolean_t
on_sigwinch_received (xpointer_t data)
{
  xmain_loop_t *loop = (xmain_loop_t *) data;

  sig_counter ++;

  if (sig_counter == 1)
    kill (getpid (), SIGWINCH);
  else if (sig_counter == 2)
    xmain_loop_quit (loop);
  else if (sig_counter > 2)
    g_assert_not_reached ();

  /* Increase the time window in which an issue could happen. */
  g_usleep (G_USEC_PER_SEC);

  return G_SOURCE_CONTINUE;
}

static void
test_callback_after_signal (void)
{
  /* Checks that user signal callback is invoked *after* receiving a signal.
   * In other words a new signal is never merged with the one being currently
   * dispatched or whose dispatch had already finished. */

  xmain_loop_t *mainloop;
  xmain_context_t *context;
  xsource_t *source;

  sig_counter = 0;

  context = xmain_context_new ();
  mainloop = xmain_loop_new (context, FALSE);

  source = g_unix_signal_source_new (SIGWINCH);
  xsource_set_callback (source, on_sigwinch_received, mainloop, NULL);
  xsource_attach (source, context);
  xsource_unref (source);

  g_assert_cmpint (sig_counter, ==, 0);
  kill (getpid (), SIGWINCH);
  xmain_loop_run (mainloop);
  g_assert_cmpint (sig_counter, ==, 2);

  xmain_loop_unref (mainloop);
  xmain_context_unref (context);
}

static void
test_get_passwd_entry_root (void)
{
  struct passwd *pwd;
  xerror_t *local_error = NULL;

  g_test_summary ("Tests that g_unix_get_passwd_entry() works for a "
                  "known-existing username.");

  pwd = g_unix_get_passwd_entry ("root", &local_error);
  g_assert_no_error (local_error);

  g_assert_cmpstr (pwd->pw_name, ==, "root");
  g_assert_cmpuint (pwd->pw_uid, ==, 0);

  g_free (pwd);
}

static void
test_get_passwd_entry_nonexistent (void)
{
  struct passwd *pwd;
  xerror_t *local_error = NULL;

  g_test_summary ("Tests that g_unix_get_passwd_entry() returns an error for a "
                  "nonexistent username.");

  pwd = g_unix_get_passwd_entry ("thisusernamedoesntexist", &local_error);
  g_assert_error (local_error, G_UNIX_ERROR, 0);
  g_assert_null (pwd);

  g_clear_error (&local_error);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/glib-unix/pipe", test_pipe);
  g_test_add_func ("/glib-unix/error", test_error);
  g_test_add_func ("/glib-unix/nonblocking", test_nonblocking);
  g_test_add_func ("/glib-unix/sighup", test_sighup);
  g_test_add_func ("/glib-unix/sigterm", test_sigterm);
  g_test_add_func ("/glib-unix/sighup_again", test_sighup);
  g_test_add_func ("/glib-unix/sighup_add_remove", test_sighup_add_remove);
  g_test_add_func ("/glib-unix/sighup_nested", test_sighup_nested);
  g_test_add_func ("/glib-unix/callback_after_signal", test_callback_after_signal);
  g_test_add_func ("/glib-unix/get-passwd-entry/root", test_get_passwd_entry_root);
  g_test_add_func ("/glib-unix/get-passwd-entry/nonexistent", test_get_passwd_entry_nonexistent);

  return g_test_run();
}
