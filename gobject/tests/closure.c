#include <glib-object.h>

#ifdef G_OS_UNIX
#include <glib-unix.h>

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#endif

static void
test_source (xsource_t *one, xcallback_t quit_callback)
{
  xclosure_t *closure;
  xmain_loop_t *loop;

  /* Callback with xmain_loop_t user_data */
  loop = xmain_loop_new (NULL, FALSE);

  closure = g_cclosure_new (quit_callback, loop, NULL);
  xsource_set_closure (one, closure);

  xsource_attach (one, NULL);
  xmain_loop_run (loop);

  xsource_destroy (one);
  xmain_loop_unref (loop);
}

static xboolean_t
simple_quit_callback (xpointer_t user_data)
{
  xmain_loop_t *loop = user_data;

  xmain_loop_quit (loop);

  return TRUE;
}

static void
test_closure_idle (void)
{
  xsource_t *source;

  source = g_idle_source_new ();
  test_source (source, G_CALLBACK (simple_quit_callback));
  xsource_unref (source);
}

static void
test_closure_timeout (void)
{
  xsource_t *source;

  source = g_timeout_source_new (10);
  test_source (source, G_CALLBACK (simple_quit_callback));
  xsource_unref (source);
}

static xboolean_t
iochannel_quit_callback (xio_channel_t   *channel,
                         xio_condition_t  cond,
                         xpointer_t      user_data)
{
  xmain_loop_t *loop = user_data;

  xmain_loop_quit (loop);

  return TRUE;
}

static void
test_closure_iochannel (void)
{
  xio_channel_t *chan;
  xsource_t *source;
  char *path;
  xerror_t *error = NULL;

  if (g_path_is_absolute (g_get_prgname ()))
    path = xstrdup (g_get_prgname ());
  else
    {
      path = g_test_build_filename (G_TEST_BUILT,
                                    g_get_prgname (),
                                    NULL);
    }
  chan = g_io_channel_new_file (path, "r", &error);
  g_assert_no_error (error);
  g_free (path);

  source = g_io_create_watch (chan, G_IO_IN);
  test_source (source, G_CALLBACK (iochannel_quit_callback));
  xsource_unref (source);

  g_io_channel_unref (chan);
}

static void
test_closure_child (void)
{
  xsource_t *source;
  xpid_t pid;
  xerror_t *error = NULL;
  xchar_t *argv[3];

  g_assert (g_getenv ("DO_NOT_ACCIDENTALLY_RECURSE") == NULL);
  g_setenv ("DO_NOT_ACCIDENTALLY_RECURSE", "1", TRUE);

  if (g_path_is_absolute (g_get_prgname ()))
    argv[0] = xstrdup (g_get_prgname ());
  else
    {
      argv[0] = g_test_build_filename (G_TEST_BUILT,
                                       g_get_prgname (),
                                       NULL);
    }
  argv[1] = "-l";
  argv[2] = NULL;

  g_spawn_async (NULL, argv, NULL,
                 G_SPAWN_STDOUT_TO_DEV_NULL |
                 G_SPAWN_STDERR_TO_DEV_NULL |
                 G_SPAWN_DO_NOT_REAP_CHILD,
                 NULL, NULL,
                 &pid, &error);
  g_assert_no_error (error);

  g_free (argv[0]);

  source = g_child_watch_source_new (pid);
  test_source (source, G_CALLBACK (iochannel_quit_callback));
  xsource_unref (source);
}

#ifdef G_OS_UNIX
static xboolean_t
fd_quit_callback (xint_t         fd,
                  xio_condition_t condition,
                  xpointer_t     user_data)
{
  xmain_loop_t *loop = user_data;

  xmain_loop_quit (loop);

  return TRUE;
}

static void
test_closure_fd (void)
{
  xint_t fd;
  xsource_t *source;

  fd = open ("/dev/null", O_RDONLY);
  g_assert (fd != -1);

  source = g_unix_fd_source_new (fd, G_IO_IN);
  test_source (source, G_CALLBACK (fd_quit_callback));
  xsource_unref (source);

  close (fd);
}

static xboolean_t
send_usr1 (xpointer_t user_data)
{
  kill (getpid (), SIGUSR1);
  return FALSE;
}

static xboolean_t
closure_quit_callback (xpointer_t     user_data)
{
  xmain_loop_t *loop = user_data;

  xmain_loop_quit (loop);

  return TRUE;
}

static void
test_closure_signal (void)
{
  xsource_t *source;

  g_idle_add_full (G_PRIORITY_LOW, send_usr1, NULL, NULL);

  source = g_unix_signal_source_new (SIGUSR1);
  test_source (source, G_CALLBACK (closure_quit_callback));
  xsource_unref (source);
}
#endif

int
main (int argc,
      char *argv[])
{
#ifndef G_OS_WIN32
  sigset_t sig_mask, old_mask;

  sigemptyset (&sig_mask);
  sigaddset (&sig_mask, SIGUSR1);
  if (sigprocmask (SIG_UNBLOCK, &sig_mask, &old_mask) == 0)
    {
      if (sigismember (&old_mask, SIGUSR1))
        g_message ("SIGUSR1 was blocked, unblocking it");
    }
#endif

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/closure/idle", test_closure_idle);
  g_test_add_func ("/closure/timeout", test_closure_timeout);
  g_test_add_func ("/closure/iochannel", test_closure_iochannel);
  g_test_add_func ("/closure/child", test_closure_child);
#ifdef G_OS_UNIX
  g_test_add_func ("/closure/fd", test_closure_fd);
  g_test_add_func ("/closure/signal", test_closure_signal);
#endif

  return g_test_run ();
}
