#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include <errno.h>
#include <glib.h>
#ifdef G_OS_UNIX
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef G_OS_WIN32
#include <fcntl.h>		/* For _O_BINARY used by pipe() macro */
#include <io.h>			/* for _pipe() */
#define pipe(fds) _pipe(fds, 4096, _O_BINARY)
#endif

#define ITERS 10000
#define INCREMENT 10
#define NTHREADS 4
#define NCRAWLERS 4
#define CRAWLER_TIMEOUT_RANGE 40
#define RECURSER_TIMEOUT 50

/* The partial ordering between the context array mutex and
 * crawler array mutex is that the crawler array mutex cannot
 * be locked while the context array mutex is locked
 */
xptr_array_t *context_array;
xmutex_t context_array_mutex;
xcond_t context_array_cond;

xmain_loop_t *main_loop;

G_LOCK_DEFINE_STATIC (crawler_array_lock);
xptr_array_t *crawler_array;

typedef struct _AddrData AddrData;
typedef struct _TestData TestData;

struct _AddrData
{
  xmain_loop_t *loop;
  xio_channel_t *dest;
  xint_t count;
};

struct _TestData
{
  xint_t current_val;
  xint_t iters;
  xio_channel_t *in;
};

static void cleanup_crawlers (xmain_context_t *context);

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

static xboolean_t
adder_callback (xio_channel_t   *source,
		xio_condition_t  condition,
		xpointer_t      data)
{
  char buf1[32];
  char buf2[32];

  char result[32] = { 0, };

  AddrData *addr_data = data;

  if (!read_all (source, buf1, 32) ||
      !read_all (source, buf2, 32))
    {
      xmain_loop_quit (addr_data->loop);
      return FALSE;
    }

  sprintf (result, "%d", atoi(buf1) + atoi(buf2));
  write_all (addr_data->dest, result, 32);

  return TRUE;
}

static xboolean_t
timeout_callback (xpointer_t data)
{
  AddrData *addr_data = data;

  addr_data->count++;

  return TRUE;
}

static xpointer_t
adder_thread (xpointer_t data)
{
  xmain_context_t *context;
  xsource_t *adder_source;
  xsource_t *timeout_source;

  xio_channel_t **channels = data;
  AddrData addr_data;

  context = xmain_context_new ();

  g_mutex_lock (&context_array_mutex);

  xptr_array_add (context_array, context);

  if (context_array->len == NTHREADS)
    g_cond_broadcast (&context_array_cond);

  g_mutex_unlock (&context_array_mutex);

  addr_data.dest = channels[1];
  addr_data.loop = xmain_loop_new (context, FALSE);
  addr_data.count = 0;

  adder_source = g_io_create_watch (channels[0], G_IO_IN | G_IO_HUP);
  xsource_set_static_name (adder_source, "Adder I/O");
  xsource_set_callback (adder_source, (xsource_func_t)adder_callback, &addr_data, NULL);
  xsource_attach (adder_source, context);
  xsource_unref (adder_source);

  timeout_source = g_timeout_source_new (10);
  xsource_set_static_name (timeout_source, "Adder timeout");
  xsource_set_callback (timeout_source, (xsource_func_t)timeout_callback, &addr_data, NULL);
  xsource_set_priority (timeout_source, G_PRIORITY_HIGH);
  xsource_attach (timeout_source, context);
  xsource_unref (timeout_source);

  xmain_loop_run (addr_data.loop);

  g_io_channel_unref (channels[0]);
  g_io_channel_unref (channels[1]);

  g_free (channels);

  xmain_loop_unref (addr_data.loop);

#ifdef VERBOSE
  g_print ("Timeout run %d times\n", addr_data.count);
#endif

  g_mutex_lock (&context_array_mutex);
  xptr_array_remove (context_array, context);
  if (context_array->len == 0)
    xmain_loop_quit (main_loop);
  g_mutex_unlock (&context_array_mutex);

  cleanup_crawlers (context);
  xmain_context_unref (context);

  return NULL;
}

static void
io_pipe (xio_channel_t **channels)
{
  xint_t fds[2];

  if (pipe(fds) < 0)
    {
      int errsv = errno;
      g_warning ("Cannot create pipe %s", xstrerror (errsv));
      exit (1);
    }

  channels[0] = g_io_channel_unix_new (fds[0]);
  channels[1] = g_io_channel_unix_new (fds[1]);

  g_io_channel_set_close_on_unref (channels[0], TRUE);
  g_io_channel_set_close_on_unref (channels[1], TRUE);
}

static void
do_add (xio_channel_t *in, xint_t a, xint_t b)
{
  char buf1[32] = { 0, };
  char buf2[32] = { 0, };

  sprintf (buf1, "%d", a);
  sprintf (buf2, "%d", b);

  write_all (in, buf1, 32);
  write_all (in, buf2, 32);
}

static xboolean_t
adder_response (xio_channel_t   *source,
		xio_condition_t  condition,
		xpointer_t      data)
{
  char result[32];
  TestData *test_data = data;

  if (!read_all (source, result, 32))
    return FALSE;

  test_data->current_val = atoi (result);
  test_data->iters--;

  if (test_data->iters == 0)
    {
      if (test_data->current_val != ITERS * INCREMENT)
	{
	  g_print ("Addition failed: %d != %d\n",
		   test_data->current_val, ITERS * INCREMENT);
	  exit (1);
	}

      g_io_channel_unref (source);
      g_io_channel_unref (test_data->in);

      g_free (test_data);

      return FALSE;
    }

  do_add (test_data->in, test_data->current_val, INCREMENT);

  return TRUE;
}

static xthread_t *
create_adder_thread (void)
{
  xthread_t *thread;
  TestData *test_data;

  xio_channel_t *in_channels[2];
  xio_channel_t *out_channels[2];

  xio_channel_t **sub_channels;

  sub_channels = g_new (xio_channel_t *, 2);

  io_pipe (in_channels);
  io_pipe (out_channels);

  sub_channels[0] = in_channels[0];
  sub_channels[1] = out_channels[1];

  thread = xthread_new ("adder", adder_thread, sub_channels);

  test_data = g_new (TestData, 1);
  test_data->in = in_channels[1];
  test_data->current_val = 0;
  test_data->iters = ITERS;

  g_io_add_watch (out_channels[0], G_IO_IN | G_IO_HUP,
		  adder_response, test_data);

  do_add (test_data->in, test_data->current_val, INCREMENT);

  return thread;
}

static void create_crawler (void);

static void
remove_crawler (void)
{
  xsource_t *other_source;

  if (crawler_array->len > 0)
    {
      other_source = crawler_array->pdata[g_random_int_range (0, crawler_array->len)];
      xsource_destroy (other_source);
      xassert (xptr_array_remove_fast (crawler_array, other_source));
    }
}

static xint_t
crawler_callback (xpointer_t data)
{
  xsource_t *source = data;

  G_LOCK (crawler_array_lock);

  if (!xptr_array_remove_fast (crawler_array, source))
    remove_crawler();

  remove_crawler();
  G_UNLOCK (crawler_array_lock);

  create_crawler();
  create_crawler();

  return FALSE;
}

static void
create_crawler (void)
{
  xsource_t *source = g_timeout_source_new (g_random_int_range (0, CRAWLER_TIMEOUT_RANGE));
  xsource_set_static_name (source, "Crawler timeout");
  xsource_set_callback (source, (xsource_func_t)crawler_callback, source, NULL);

  G_LOCK (crawler_array_lock);
  xptr_array_add (crawler_array, source);

  g_mutex_lock (&context_array_mutex);
  xsource_attach (source, context_array->pdata[g_random_int_range (0, context_array->len)]);
  xsource_unref (source);
  g_mutex_unlock (&context_array_mutex);

  G_UNLOCK (crawler_array_lock);
}

static void
cleanup_crawlers (xmain_context_t *context)
{
  xuint_t i;

  G_LOCK (crawler_array_lock);
  for (i = 0; i < crawler_array->len; i++)
    {
      if (xsource_get_context (crawler_array->pdata[i]) == context)
	{
	  xsource_destroy (xptr_array_remove_index (crawler_array, i));
	  i--;
	}
    }
  G_UNLOCK (crawler_array_lock);
}

static xboolean_t
recurser_idle (xpointer_t data)
{
  xmain_context_t *context = data;
  xint_t i;

  for (i = 0; i < 10; i++)
    xmain_context_iteration (context, FALSE);

  return FALSE;
}

static xboolean_t
recurser_start (xpointer_t data)
{
  xmain_context_t *context;
  xsource_t *source;

  g_mutex_lock (&context_array_mutex);
  if (context_array->len > 0)
    {
      context = context_array->pdata[g_random_int_range (0, context_array->len)];
      source = g_idle_source_new ();
      xsource_set_static_name (source, "Recursing idle source");
      xsource_set_callback (source, recurser_idle, context, NULL);
      xsource_attach (source, context);
      xsource_unref (source);
    }
  g_mutex_unlock (&context_array_mutex);

  return TRUE;
}

int
main (int   argc,
      char *argv[])
{
  xint_t i;
  xthread_t *threads[NTHREADS];

  context_array = xptr_array_new ();

  crawler_array = xptr_array_new ();

  main_loop = xmain_loop_new (NULL, FALSE);

  for (i = 0; i < NTHREADS; i++)
    threads[i] = create_adder_thread ();

  /* Wait for all threads to start
   */
  g_mutex_lock (&context_array_mutex);

  while (context_array->len < NTHREADS)
    g_cond_wait (&context_array_cond, &context_array_mutex);

  g_mutex_unlock (&context_array_mutex);

  for (i = 0; i < NCRAWLERS; i++)
    create_crawler ();

  g_timeout_add (RECURSER_TIMEOUT, recurser_start, NULL);

  xmain_loop_run (main_loop);
  xmain_loop_unref (main_loop);

  for (i = 0; i < NTHREADS; i++)
    xthread_join (threads[i]);

  xptr_array_unref (crawler_array);
  xptr_array_unref (context_array);

  return 0;
}
