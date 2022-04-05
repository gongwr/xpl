#include "gcontextspecificgroup.c"
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

#define N_THREADS 10

static xchar_t *test_file;

char *test_file_buffer;
xsize_t test_file_size;
static char async_read_buffer[8192];

static void
read_data (xobject_t *source, xasync_result_t *result, xpointer_t loop)
{
  xinput_stream_t *in = G_INPUT_STREAM (source);
  xerror_t *error = NULL;
  gssize nread;

  nread = g_input_stream_read_finish (in, result, &error);
  g_assert_no_error (error);

  g_assert_cmpint (nread, >, 0);
  g_assert_cmpint (nread, <=, MIN(sizeof (async_read_buffer), test_file_size));
  g_assert (memcmp (async_read_buffer, test_file_buffer, nread) == 0);

  g_main_loop_quit (loop);
}

static void
opened_for_read (xobject_t *source, xasync_result_t *result, xpointer_t loop)
{
  xfile_t *file = G_FILE (source);
  GFileInputStream *in;
  xerror_t *error = NULL;

  in = g_file_read_finish (file, result, &error);
  g_assert_no_error (error);

  memset (async_read_buffer, 0, sizeof (async_read_buffer));
  g_input_stream_read_async (G_INPUT_STREAM (in),
			     async_read_buffer, sizeof (async_read_buffer),
			     G_PRIORITY_DEFAULT, NULL,
			     read_data, loop);

  g_object_unref (in);
}

/* Test 1: Async I/O started in a thread with a thread-default context
 * will stick to that thread, and will complete even if the default
 * main loop is blocked. (NB: the last part would not be true if we
 * were testing GFileMonitor!)
 */

static xboolean_t idle_start_test1_thread (xpointer_t loop);
static xpointer_t test1_thread (xpointer_t user_data);

static xboolean_t test1_done;
static GCond test1_cond;
static GMutex test1_mutex;

static void
test_thread_independence (void)
{
  GMainLoop *loop;

  loop = g_main_loop_new (NULL, FALSE);
  g_idle_add (idle_start_test1_thread, loop);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);
}

static xboolean_t
idle_start_test1_thread (xpointer_t loop)
{
  gint64 time;
  GThread *thread;
  xboolean_t io_completed;

  g_mutex_lock (&test1_mutex);
  thread = g_thread_new ("test1", test1_thread, NULL);

  time = g_get_monotonic_time () + 2 * G_TIME_SPAN_SECOND;
  while (!test1_done)
    {
      io_completed = g_cond_wait_until (&test1_cond, &test1_mutex, time);
      g_assert (io_completed);
    }
  g_thread_join (thread);

  g_mutex_unlock (&test1_mutex);
  g_main_loop_quit (loop);
  return G_SOURCE_REMOVE;
}

static xpointer_t
test1_thread (xpointer_t user_data)
{
  GMainContext *context;
  GMainLoop *loop;
  xfile_t *file;

  /* Wait for main thread to be waiting on test1_cond */
  g_mutex_lock (&test1_mutex);

  context = g_main_context_new ();
  g_assert (g_main_context_get_thread_default () == NULL);
  g_main_context_push_thread_default (context);
  g_assert (g_main_context_get_thread_default () == context);

  file = g_file_new_for_path (test_file);
  g_assert (g_file_supports_thread_contexts (file));

  loop = g_main_loop_new (context, FALSE);
  g_file_read_async (file, G_PRIORITY_DEFAULT, NULL,
		     opened_for_read, loop);
  g_object_unref (file);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  test1_done = TRUE;
  g_cond_signal (&test1_cond);
  g_mutex_unlock (&test1_mutex);

  g_main_context_pop_thread_default (context);
  g_main_context_unref (context);

  return NULL;
}

/* Test 2: If we push a thread-default context in the main thread, we
 * can run async ops in that context without running the default
 * context.
 */

static xboolean_t test2_fail (xpointer_t user_data);

static void
test_context_independence (void)
{
  GMainContext *context;
  GMainLoop *loop;
  xfile_t *file;
  xuint_t default_timeout;
  GSource *thread_default_timeout;

  context = g_main_context_new ();
  g_assert (g_main_context_get_thread_default () == NULL);
  g_main_context_push_thread_default (context);
  g_assert (g_main_context_get_thread_default () == context);

  file = g_file_new_for_path (test_file);
  g_assert (g_file_supports_thread_contexts (file));

  /* Add a timeout to the main loop, to fail immediately if it gets run */
  default_timeout = g_timeout_add_full (G_PRIORITY_HIGH, 0,
					test2_fail, NULL, NULL);
  /* Add a timeout to the alternate loop, to fail if the I/O *doesn't* run */
  thread_default_timeout = g_timeout_source_new_seconds (2);
  g_source_set_callback (thread_default_timeout, test2_fail, NULL, NULL);
  g_source_attach (thread_default_timeout, context);

  loop = g_main_loop_new (context, FALSE);
  g_file_read_async (file, G_PRIORITY_DEFAULT, NULL,
		     opened_for_read, loop);
  g_object_unref (file);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_source_remove (default_timeout);
  g_source_destroy (thread_default_timeout);
  g_source_unref (thread_default_timeout);

  g_main_context_pop_thread_default (context);
  g_main_context_unref (context);
}

static xboolean_t
test2_fail (xpointer_t user_data)
{
  g_assert_not_reached ();
  return FALSE;
}


typedef struct
{
  xobject_t parent_instance;

  GMainContext *context;
} PerThreadThing;

typedef xobject_class_t PerThreadThingClass;

static xtype_t per_thread_thing_get_type (void);

G_DEFINE_TYPE (PerThreadThing, per_thread_thing, XTYPE_OBJECT)

static GContextSpecificGroup group;
static xpointer_t instances[N_THREADS];
static xint_t is_running;
static xint_t current_value;
static xint_t observed_values[N_THREADS];

static void
start_func (void)
{
  g_assert (!is_running);
  g_atomic_int_set (&is_running, TRUE);
}

static void
stop_func (void)
{
  g_assert (is_running);
  g_atomic_int_set (&is_running, FALSE);
}

static void
per_thread_thing_finalize (xobject_t *object)
{
  PerThreadThing *thing = (PerThreadThing *) object;

  g_context_specific_group_remove (&group, thing->context, thing, stop_func);

  G_OBJECT_CLASS (per_thread_thing_parent_class)->finalize (object);
}

static void
per_thread_thing_init (PerThreadThing *thing)
{
}

static void
per_thread_thing_class_init (PerThreadThingClass *class)
{
  class->finalize = per_thread_thing_finalize;

  g_signal_new ("changed", per_thread_thing_get_type (), G_SIGNAL_RUN_FIRST, 0,
                NULL, NULL, g_cclosure_marshal_VOID__VOID, XTYPE_NONE, 0);
}

static xpointer_t
per_thread_thing_get (void)
{
  return g_context_specific_group_get (&group, per_thread_thing_get_type (),
                                       G_STRUCT_OFFSET (PerThreadThing, context),
                                       start_func);
}

static xpointer_t
test_identity_thread (xpointer_t user_data)
{
  xuint_t thread_nr = GPOINTER_TO_UINT (user_data);
  GMainContext *my_context;
  xuint_t i, j;

  my_context = g_main_context_new ();
  g_main_context_push_thread_default (my_context);

  g_assert (!instances[thread_nr]);
  instances[thread_nr] = per_thread_thing_get ();
  g_assert (g_atomic_int_get (&is_running));

  for (i = 0; i < 100; i++)
    {
      xpointer_t instance = per_thread_thing_get ();

      for (j = 0; j < N_THREADS; j++)
        g_assert ((instance == instances[j]) == (thread_nr == j));

      g_assert (g_atomic_int_get (&is_running));

      g_thread_yield ();

      g_assert (g_atomic_int_get (&is_running));
    }

  for (i = 0; i < 100; i++)
    {
      g_object_unref (instances[thread_nr]);

      for (j = 0; j < N_THREADS; j++)
        g_assert ((instances[thread_nr] == instances[j]) == (thread_nr == j));

      g_assert (g_atomic_int_get (&is_running));

      g_thread_yield ();
    }

  /* drop the last ref */
  g_object_unref (instances[thread_nr]);
  instances[thread_nr] = NULL;

  g_main_context_pop_thread_default (my_context);
  g_main_context_unref (my_context);

  /* at least one thread should see this cleared on exit */
  return GUINT_TO_POINTER (!group.requested_state);
}

static void
test_context_specific_identity (void)
{
  GThread *threads[N_THREADS];
  xboolean_t exited = FALSE;
  xuint_t i;

  g_assert (!g_atomic_int_get (&is_running));
  for (i = 0; i < N_THREADS; i++)
    threads[i] = g_thread_new ("test", test_identity_thread, GUINT_TO_POINTER (i));
  for (i = 0; i < N_THREADS; i++)
    exited |= GPOINTER_TO_UINT (g_thread_join (threads[i]));
  g_assert (exited);
  g_assert (!group.requested_state);
}

static void
changed_emitted (PerThreadThing *thing,
                 xpointer_t        user_data)
{
  xint_t *observed_value = user_data;

  g_atomic_int_set (observed_value, g_atomic_int_get (&current_value));
}

static xpointer_t
test_emit_thread (xpointer_t user_data)
{
  xint_t *observed_value = user_data;
  GMainContext *my_context;
  xpointer_t instance;

  my_context = g_main_context_new ();
  g_main_context_push_thread_default (my_context);

  instance = per_thread_thing_get ();
  g_assert (g_atomic_int_get (&is_running));

  g_signal_connect (instance, "changed", G_CALLBACK (changed_emitted), observed_value);

  /* observe after connection */
  g_atomic_int_set (observed_value, g_atomic_int_get (&current_value));

  while (g_atomic_int_get (&current_value) != -1)
    g_main_context_iteration (my_context, TRUE);

  g_object_unref (instance);

  g_main_context_pop_thread_default (my_context);
  g_main_context_unref (my_context);

  /* at least one thread should see this cleared on exit */
  return GUINT_TO_POINTER (!group.requested_state);
}

static void
test_context_specific_emit (void)
{
  GThread *threads[N_THREADS];
  xboolean_t exited = FALSE;
  xsize_t i;
  xint_t k, n;

  for (i = 0; i < N_THREADS; i++)
    threads[i] = g_thread_new ("test", test_emit_thread, &observed_values[i]);

  /* make changes and ensure that they are observed */
  for (n = 0; n < 1000; n++)
    {
      gint64 expiry;

      /* don't burn CPU forever */
      expiry = g_get_monotonic_time () + 10 * G_TIME_SPAN_SECOND;

      g_atomic_int_set (&current_value, n);

      /* wake them to notice */
      for (k = 0; k < g_test_rand_int_range (1, 5); k++)
        g_context_specific_group_emit (&group, g_signal_lookup ("changed", per_thread_thing_get_type ()));

      for (i = 0; i < N_THREADS; i++)
        while (g_atomic_int_get (&observed_values[i]) != n)
          {
            g_thread_yield ();

            if (g_get_monotonic_time () > expiry)
              g_error ("timed out");
          }
    }

  /* tell them to quit */
  g_atomic_int_set (&current_value, -1);
  g_context_specific_group_emit (&group, g_signal_lookup ("notify", XTYPE_OBJECT));

  for (i = 0; i < N_THREADS; i++)
    exited |= GPOINTER_TO_UINT (g_thread_join (threads[i]));
  g_assert (exited);
  g_assert (!group.requested_state);
}

static void
test_context_specific_emit_and_unref (void)
{
  xpointer_t obj;

  obj = per_thread_thing_get ();
  g_context_specific_group_emit (&group, g_signal_lookup ("changed", per_thread_thing_get_type ()));
  g_object_unref (obj);

  while (g_main_context_iteration (NULL, 0))
    ;
}

int
main (int argc, char **argv)
{
  xerror_t *error = NULL;
  int ret;

  g_test_init (&argc, &argv, NULL);

  test_file = g_test_build_filename (G_TEST_DIST, "contexts.c", NULL);
  g_file_get_contents (test_file, &test_file_buffer,
		       &test_file_size, &error);
  g_assert_no_error (error);

  g_test_add_func ("/gio/contexts/thread-independence", test_thread_independence);
  g_test_add_func ("/gio/contexts/context-independence", test_context_independence);
  g_test_add_func ("/gio/contexts/context-specific/identity", test_context_specific_identity);
  g_test_add_func ("/gio/contexts/context-specific/emit", test_context_specific_emit);
  g_test_add_func ("/gio/contexts/context-specific/emit-and-unref", test_context_specific_emit_and_unref);

  ret = g_test_run();

  g_free (test_file_buffer);
  g_free (test_file);

  return ret;
}
