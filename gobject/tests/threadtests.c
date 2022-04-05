/* GLib testing framework examples and tests
 * Copyright (C) 2008 Imendio AB
 * Authors: Tim Janik
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

#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include <glib.h>
#include <glib-object.h>

static int mtsafe_call_counter = 0; /* multi thread safe call counter, must be accessed atomically */
static int unsafe_call_counter = 0; /* single-threaded call counter */
static GCond sync_cond;
static GMutex sync_mutex;

#define NUM_COUNTER_INCREMENTS 100000

static void
call_counter_init (xpointer_t tclass)
{
  int i;
  for (i = 0; i < NUM_COUNTER_INCREMENTS; i++)
    {
      int saved_unsafe_call_counter = unsafe_call_counter;
      g_atomic_int_add (&mtsafe_call_counter, 1); /* real call count update */
      g_thread_yield(); /* let concurrent threads corrupt the unsafe_call_counter state */
      unsafe_call_counter = 1 + saved_unsafe_call_counter; /* non-atomic counter update */
    }
}

static void interface_per_class_init (void) { call_counter_init (NULL); }

/* define 3 test interfaces */
typedef xtype_interface_t MyFace0Interface;
static xtype_t my_face0_get_type (void);
G_DEFINE_INTERFACE (MyFace0, my_face0, XTYPE_OBJECT)
static void my_face0_default_init (MyFace0Interface *iface) { call_counter_init (iface); }
typedef xtype_interface_t MyFace1Interface;
static xtype_t my_face1_get_type (void);
G_DEFINE_INTERFACE (MyFace1, my_face1, XTYPE_OBJECT)
static void my_face1_default_init (MyFace1Interface *iface) { call_counter_init (iface); }

/* define 3 test objects, adding interfaces 0 & 1, and adding interface 2 after class initialization */
typedef xobject_t         MyTester0;
typedef xobject_class_t    MyTester0Class;
static xtype_t my_tester0_get_type (void);
G_DEFINE_TYPE_WITH_CODE (MyTester0, my_tester0, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (my_face0_get_type(), interface_per_class_init)
                         G_IMPLEMENT_INTERFACE (my_face1_get_type(), interface_per_class_init))
static void my_tester0_init (MyTester0*t) {}
static void my_tester0_class_init (MyTester0Class*c) { call_counter_init (c); }
typedef xobject_t         MyTester1;
typedef xobject_class_t    MyTester1Class;

/* Disabled for now (see https://bugzilla.gnome.org/show_bug.cgi?id=687659) */
#if 0
typedef xtype_interface_t MyFace2Interface;
static xtype_t my_face2_get_type (void);
G_DEFINE_INTERFACE (MyFace2, my_face2, XTYPE_OBJECT)
static void my_face2_default_init (MyFace2Interface *iface) { call_counter_init (iface); }

static xtype_t my_tester1_get_type (void);
G_DEFINE_TYPE_WITH_CODE (MyTester1, my_tester1, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (my_face0_get_type(), interface_per_class_init)
                         G_IMPLEMENT_INTERFACE (my_face1_get_type(), interface_per_class_init))
static void my_tester1_init (MyTester1*t) {}
static void my_tester1_class_init (MyTester1Class*c) { call_counter_init (c); }
typedef xobject_t         MyTester2;
typedef xobject_class_t    MyTester2Class;
static xtype_t my_tester2_get_type (void);
G_DEFINE_TYPE_WITH_CODE (MyTester2, my_tester2, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (my_face0_get_type(), interface_per_class_init)
                         G_IMPLEMENT_INTERFACE (my_face1_get_type(), interface_per_class_init))
static void my_tester2_init (MyTester2*t) {}
static void my_tester2_class_init (MyTester2Class*c) { call_counter_init (c); }

static xpointer_t
tester_init_thread (xpointer_t data)
{
  const GInterfaceInfo face2_interface_info = { (GInterfaceInitFunc) interface_per_class_init, NULL, NULL };
  xpointer_t klass;
  /* first, synchronize with other threads,
   * then run interface and class initializers,
   * using unsafe_call_counter concurrently
   */
  g_mutex_lock (&sync_mutex);
  g_mutex_unlock (&sync_mutex);
  /* test default interface initialization for face0 */
  g_type_default_interface_unref (g_type_default_interface_ref (my_face0_get_type()));
  /* test class initialization, face0 per-class initializer, face1 default and per-class initializer */
  klass = g_type_class_ref ((xtype_t) data);
  /* test face2 default and per-class initializer, after class_init */
  g_type_add_interface_static (XTYPE_FROM_CLASS (klass), my_face2_get_type(), &face2_interface_info);
  /* cleanups */
  g_type_class_unref (klass);
  return NULL;
}

static void
test_threaded_class_init (void)
{
  GThread *t1, *t2, *t3;

  /* pause newly created threads */
  g_mutex_lock (&sync_mutex);

  /* create threads */
  t1 = g_thread_create (tester_init_thread, (xpointer_t) my_tester0_get_type(), TRUE, NULL);
  t2 = g_thread_create (tester_init_thread, (xpointer_t) my_tester1_get_type(), TRUE, NULL);
  t3 = g_thread_create (tester_init_thread, (xpointer_t) my_tester2_get_type(), TRUE, NULL);

  /* execute threads */
  g_mutex_unlock (&sync_mutex);
  while (g_atomic_int_get (&mtsafe_call_counter) < (3 + 3 + 3 * 3) * NUM_COUNTER_INCREMENTS)
    {
      if (g_test_verbose())
        g_printerr ("Initializers counted: %u\n", g_atomic_int_get (&mtsafe_call_counter));
      g_usleep (50 * 1000); /* wait for threads to complete */
    }
  if (g_test_verbose())
    g_printerr ("Total initializers: %u\n", g_atomic_int_get (&mtsafe_call_counter));
  /* ensure non-corrupted counter updates */
  g_assert_cmpint (g_atomic_int_get (&mtsafe_call_counter), ==, unsafe_call_counter);

  g_thread_join (t1);
  g_thread_join (t2);
  g_thread_join (t3);
}
#endif

typedef struct {
  xobject_t parent;
  char   *name;
} PropTester;
typedef xobject_class_t    PropTesterClass;
static xtype_t prop_tester_get_type (void);
G_DEFINE_TYPE (PropTester, prop_tester, XTYPE_OBJECT)
#define PROP_NAME 1
static void
prop_tester_init (PropTester* t)
{
  if (t->name == NULL)
    { } /* needs unit test framework initialization: g_test_bug ("race initializing properties"); */
}
static void
prop_tester_set_property (xobject_t        *object,
                          xuint_t           property_id,
                          const GValue   *value,
                          GParamSpec     *pspec)
{}
static void
prop_tester_class_init (PropTesterClass *c)
{
  int i;
  GParamSpec *param;
  xobject_class_t *gobject_class = G_OBJECT_CLASS (c);

  gobject_class->set_property = prop_tester_set_property; /* silence xobject_t checks */

  g_mutex_lock (&sync_mutex);
  g_cond_signal (&sync_cond);
  g_mutex_unlock (&sync_mutex);

  for (i = 0; i < 100; i++) /* wait a bit. */
    g_thread_yield();

  call_counter_init (c);
  param = g_param_spec_string ("name", "name_i18n",
			       "yet-more-wasteful-i18n",
			       NULL,
			       G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
			       G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB |
			       G_PARAM_STATIC_NICK);
  g_object_class_install_property (gobject_class, PROP_NAME, param);
}

static xpointer_t
object_create (xpointer_t data)
{
  xobject_t *obj = g_object_new (prop_tester_get_type(), "name", "fish", NULL);
  g_object_unref (obj);
  return NULL;
}

static void
test_threaded_object_init (void)
{
  GThread *creator;
  g_mutex_lock (&sync_mutex);

  creator = g_thread_create (object_create, NULL, TRUE, NULL);
  /* really provoke the race */
  g_cond_wait (&sync_cond, &sync_mutex);

  object_create (NULL);
  g_mutex_unlock (&sync_mutex);

  g_thread_join (creator);
}

typedef struct {
    MyTester0 *strong;
    xuint_t unref_delay;
} UnrefInThreadData;

static xpointer_t
unref_in_thread (xpointer_t p)
{
  UnrefInThreadData *data = p;

  g_usleep (data->unref_delay);
  g_object_unref (data->strong);

  return NULL;
}

/* undefine to see this test fail without GWeakRef */
#define HAVE_G_WEAK_REF

#define SLEEP_MIN_USEC 1
#define SLEEP_MAX_USEC 10

static void
test_threaded_weak_ref (void)
{
  xuint_t i;
  xuint_t get_wins = 0, unref_wins = 0;
  xuint_t n;

  if (g_test_thorough ())
    n = NUM_COUNTER_INCREMENTS;
  else
    n = NUM_COUNTER_INCREMENTS / 20;

#ifdef G_OS_WIN32
  /* On Windows usleep has millisecond resolution and gets rounded up
   * leading to the test running for a long time. */
  n /= 10;
#endif

  for (i = 0; i < n; i++)
    {
      UnrefInThreadData data;
#ifdef HAVE_G_WEAK_REF
      /* GWeakRef<MyTester0> in C++ terms */
      GWeakRef weak;
#else
      xpointer_t weak;
#endif
      MyTester0 *strengthened;
      xuint_t get_delay;
      GThread *thread;
      xerror_t *error = NULL;

      if (g_test_verbose () && (i % (n/20)) == 0)
        g_printerr ("%u%%\n", ((i * 100) / n));

      /* Have an object and a weak ref to it */
      data.strong = g_object_new (my_tester0_get_type (), NULL);

#ifdef HAVE_G_WEAK_REF
      g_weak_ref_init (&weak, data.strong);
#else
      weak = data.strong;
      g_object_add_weak_pointer ((xobject_t *) weak, &weak);
#endif

      /* Delay for a random time on each side of the race, to perturb the
       * timing. Ideally, we want each side to win half the races; on
       * smcv's laptop, these timings are about right.
       */
      data.unref_delay = g_random_int_range (SLEEP_MIN_USEC / 2, SLEEP_MAX_USEC / 2);
      get_delay = g_random_int_range (SLEEP_MIN_USEC, SLEEP_MAX_USEC);

      /* One half of the race is to unref the shared object */
      thread = g_thread_create (unref_in_thread, &data, TRUE, &error);
      g_assert_no_error (error);

      /* The other half of the race is to get the object from the "global
       * singleton"
       */
      g_usleep (get_delay);

#ifdef HAVE_G_WEAK_REF
      strengthened = g_weak_ref_get (&weak);
#else
      /* Spot the unsafe pointer access! In GDBusConnection this is rather
       * better-hidden, but ends up with essentially the same thing, albeit
       * cleared in dispose() rather than by a traditional weak pointer
       */
      strengthened = weak;

      if (strengthened != NULL)
        g_object_ref (strengthened);
#endif

      if (strengthened != NULL)
        g_assert (X_IS_OBJECT (strengthened));

      /* Wait for the thread to run */
      g_thread_join (thread);

      if (strengthened != NULL)
        {
          get_wins++;
          g_assert (X_IS_OBJECT (strengthened));
          g_object_unref (strengthened);
        }
      else
        {
          unref_wins++;
        }

#ifdef HAVE_G_WEAK_REF
      g_weak_ref_clear (&weak);
#else
      if (weak != NULL)
        g_object_remove_weak_pointer (weak, &weak);
#endif
    }

  if (g_test_verbose ())
    g_printerr ("Race won by get %u times, unref %u times\n",
             get_wins, unref_wins);
}

typedef struct
{
  xobject_t *object;
  GWeakRef *weak;
  xint_t started; /* (atomic) */
  xint_t finished; /* (atomic) */
  xint_t disposing; /* (atomic) */
} ThreadedWeakRefData;

static void
on_weak_ref_disposed (xpointer_t data,
                      xobject_t *gobj)
{
  ThreadedWeakRefData *thread_data = data;

  /* Wait until the thread has started */
  while (!g_atomic_int_get (&thread_data->started))
    continue;

  g_atomic_int_set (&thread_data->disposing, 1);

  /* Wait for the thread to act, so that the object is still valid */
  while (!g_atomic_int_get (&thread_data->finished))
    continue;

  g_atomic_int_set (&thread_data->disposing, 0);
}

static xpointer_t
on_other_thread_weak_ref (xpointer_t user_data)
{
  ThreadedWeakRefData *thread_data = user_data;
  xobject_t *object = thread_data->object;

  g_atomic_int_set (&thread_data->started, 1);

  /* Ensure we've started disposal */
  while (!g_atomic_int_get (&thread_data->disposing))
    continue;

  g_object_ref (object);
  g_weak_ref_set (thread_data->weak, object);
  g_object_unref (object);

  g_assert_cmpint (thread_data->disposing, ==, 1);
  g_atomic_int_set (&thread_data->finished, 1);

  return NULL;
}

static void
test_threaded_weak_ref_finalization (void)
{
  xobject_t *obj = g_object_new (XTYPE_OBJECT, NULL);
  GWeakRef weak = { { GUINT_TO_POINTER (0xDEADBEEFU) } };
  ThreadedWeakRefData thread_data = {
    .object = obj, .weak = &weak, .started = 0, .finished = 0
  };

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2390");
  g_test_summary ("Test that a weak ref added by another thread during dispose "
                  "of a xobject_t is cleared during finalisation. "
                  "Use on_weak_ref_disposed() to synchronize the other thread "
                  "with the dispose vfunc.");

  g_weak_ref_init (&weak, NULL);
  g_object_weak_ref (obj, on_weak_ref_disposed, &thread_data);

  g_assert_cmpint (obj->ref_count, ==, 1);
  g_thread_unref (g_thread_new ("on_other_thread",
                                on_other_thread_weak_ref,
                                &thread_data));
  g_object_unref (obj);

  /* This is what this test is about: at this point the weak reference
   * should have been unset (and not point to a dead object either). */
  g_assert_null (g_weak_ref_get (&weak));
}

typedef struct
{
  xobject_t *object;
  int done;    /* (atomic) */
  int toggles; /* (atomic) */
} ToggleNotifyThreadData;

static xpointer_t
on_reffer_thread (xpointer_t user_data)
{
  ToggleNotifyThreadData *thread_data = user_data;

  while (!g_atomic_int_get (&thread_data->done))
    {
      g_object_ref (thread_data->object);
      g_object_unref (thread_data->object);
    }

  return NULL;
}

static void
on_toggle_notify (xpointer_t data,
                  xobject_t *object,
                  xboolean_t is_last_ref)
{
  /* Anything could be put here, but we don't care for this test.
   * Actually having this empty made the bug to happen more frequently (being
   * timing related).
   */
}

static xpointer_t
on_toggler_thread (xpointer_t user_data)
{
  ToggleNotifyThreadData *thread_data = user_data;

  while (!g_atomic_int_get (&thread_data->done))
    {
      g_object_ref (thread_data->object);
      g_object_remove_toggle_ref (thread_data->object, on_toggle_notify, thread_data);
      g_object_add_toggle_ref (thread_data->object, on_toggle_notify, thread_data);
      g_object_unref (thread_data->object);
      g_atomic_int_add (&thread_data->toggles, 1);
    }

  return NULL;
}

static void
test_threaded_toggle_notify (void)
{
  xobject_t *object = g_object_new (XTYPE_OBJECT, NULL);
  ToggleNotifyThreadData data = { object, FALSE, 0 };
  GThread *threads[3];
  xsize_t i;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/2394");
  g_test_summary ("Test that toggle reference notifications can be changed "
                  "safely from another (the main) thread without causing the "
                  "notifying thread to abort");

  g_object_add_toggle_ref (object, on_toggle_notify, &data);
  g_object_unref (object);

  g_assert_cmpint (object->ref_count, ==, 1);
  threads[0] = g_thread_new ("on_reffer_thread", on_reffer_thread, &data);
  threads[1] = g_thread_new ("on_another_reffer_thread", on_reffer_thread, &data);
  threads[2] = g_thread_new ("on_main_toggler_thread", on_toggler_thread, &data);

  /* We need to wait here for the threads to run for a bit in order to make the
   * race to happen, so we wait for an high number of toggle changes to be met
   * so that we can be consistent on each platform.
   */
  while (g_atomic_int_get (&data.toggles) < 1000000)
    ;
  g_atomic_int_set (&data.done, TRUE);

  for (i = 0; i < G_N_ELEMENTS (threads); i++)
    g_thread_join (threads[i]);

  g_assert_cmpint (object->ref_count, ==, 1);
  g_clear_object (&object);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  /* g_test_add_func ("/xobject_t/threaded-class-init", test_threaded_class_init); */
  g_test_add_func ("/xobject_t/threaded-object-init", test_threaded_object_init);
  g_test_add_func ("/xobject_t/threaded-weak-ref", test_threaded_weak_ref);
  g_test_add_func ("/xobject_t/threaded-weak-ref/on-finalization",
                   test_threaded_weak_ref_finalization);
  g_test_add_func ("/xobject_t/threaded-toggle-notify",
                   test_threaded_toggle_notify);

  return g_test_run();
}