#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <glib.h>

static int n_children = 3;
static int n_active_children;
static int n_iters = 10000;
static xmain_loop_t *loop;

static void
io_pipe (xio_channel_t **channels)
{
  int fds[2];

  if (pipe(fds) < 0)
    {
      int errsv = errno;
      fprintf (stderr, "Cannot create pipe %s\n", xstrerror (errsv));
      exit (1);
    }

  channels[0] = g_io_channel_unix_new (fds[0]);
  channels[1] = g_io_channel_unix_new (fds[1]);
}

static xboolean_t
read_all (xio_channel_t *channel, char *buf, xsize_t len)
{
  xsize_t bytes_read = 0;
  xsize_t count;
  GIOError err;

  while (bytes_read < len)
    {
      err = g_io_channel_read (channel, buf + bytes_read, len - bytes_read, &count);
      if (err)
	{
	  if (err != G_IO_ERROR_AGAIN)
	    return FALSE;
	}
      else if (count == 0)
	return FALSE;

      bytes_read += count;
    }

  return TRUE;
}

static xboolean_t
write_all (xio_channel_t *channel, char *buf, xsize_t len)
{
  xsize_t bytes_written = 0;
  xsize_t count;
  GIOError err;

  while (bytes_written < len)
    {
      err = g_io_channel_write (channel, buf + bytes_written, len - bytes_written, &count);
      if (err && err != G_IO_ERROR_AGAIN)
	return FALSE;

      bytes_written += count;
    }

  return TRUE;
}

static void
run_child (xio_channel_t *in_channel, xio_channel_t *out_channel)
{
  int i;
  int val = 1;
  xtimer_t *timer = g_timer_new();

  for (i = 0; i < n_iters; i++)
    {
      write_all (out_channel, (char *)&val, sizeof (val));
      read_all (in_channel, (char *)&val, sizeof (val));
    }

  val = 0;
  write_all (out_channel, (char *)&val, sizeof (val));

  val = g_timer_elapsed (timer, NULL) * 1000;

  write_all (out_channel, (char *)&val, sizeof (val));
  g_timer_destroy (timer);

  exit (0);
}

static xboolean_t
input_callback (xio_channel_t   *source,
		xio_condition_t  condition,
		xpointer_t      data)
{
  int val;
  xio_channel_t *dest = (xio_channel_t *)data;

  if (!read_all (source, (char *)&val, sizeof(val)))
    {
      fprintf (stderr, "Unexpected EOF\n");
      exit (1);
    }

  if (val)
    {
      write_all (dest, (char *)&val, sizeof(val));

      return TRUE;
    }
  else
    {
      g_io_channel_close (source);
      g_io_channel_close (dest);

      g_io_channel_unref (source);
      g_io_channel_unref (dest);

      n_active_children--;
      if (n_active_children == 0)
	xmain_loop_quit (loop);

      return FALSE;
    }
}

static void
create_child (void)
{
  int pid, errsv;
  xio_channel_t *in_channels[2];
  xio_channel_t *out_channels[2];

  io_pipe (in_channels);
  io_pipe (out_channels);

  pid = fork ();
  errsv = errno;

  if (pid > 0)			/* Parent */
    {
      g_io_channel_close (in_channels[0]);
      g_io_channel_unref (in_channels[0]);
      g_io_channel_close (out_channels[1]);
      g_io_channel_unref (out_channels[1]);

      g_io_add_watch (out_channels[0], G_IO_IN | G_IO_HUP,
		      input_callback, in_channels[1]);
    }
  else if (pid == 0)		/* Child */
    {
      g_io_channel_close (in_channels[1]);
      g_io_channel_close (out_channels[0]);

      setsid ();

      run_child (in_channels[0], out_channels[1]);
    }
  else				/* Error */
    {
      fprintf (stderr, "Cannot fork: %s\n", xstrerror (errsv));
      exit (1);
    }
}

static double
difftimeval (struct timeval *old, struct timeval *new)
{
  return
    (new->tv_sec - old->tv_sec) * 1000. + (new->tv_usec - old->tv_usec) / 1000;
}

int
main (int argc, char **argv)
{
  int i;
  struct rusage old_usage;
  struct rusage new_usage;

  if (argc > 1)
    n_children = atoi(argv[1]);

  if (argc > 2)
    n_iters = atoi(argv[2]);

  printf ("Children: %d     Iters: %d\n", n_children, n_iters);

  n_active_children = n_children;
  for (i = 0; i < n_children; i++)
    create_child ();

  getrusage (RUSAGE_SELF, &old_usage);
  loop = xmain_loop_new (NULL, FALSE);
  xmain_loop_run (loop);
  getrusage (RUSAGE_SELF, &new_usage);

  printf ("Elapsed user: %g\n",
	  difftimeval (&old_usage.ru_utime, &new_usage.ru_utime));
  printf ("Elapsed system: %g\n",
	  difftimeval (&old_usage.ru_stime, &new_usage.ru_stime));
  printf ("Elapsed total: %g\n",
	  difftimeval (&old_usage.ru_utime, &new_usage.ru_utime) +
	  difftimeval (&old_usage.ru_stime, &new_usage.ru_stime));
  printf ("total / iteration: %g\n",
	  (difftimeval (&old_usage.ru_utime, &new_usage.ru_utime) +
	   difftimeval (&old_usage.ru_stime, &new_usage.ru_stime)) /
	  (n_iters * n_children));

  xmain_loop_unref (loop);
  return 0;
}
