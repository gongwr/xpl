#include <glib-object.h>

typedef struct {
  xobject_t instance;
} xobj_t;

typedef struct {
  xobject_class_t parent_class;
} xobj_class_t;

enum {
  SIGNAL1,
  SIGNAL2,
  LAST_SIGNAL
};

xuint_t signals[LAST_SIGNAL];

xtype_t xobj_get_type (void);

G_DEFINE_TYPE (xobj, my_obj, XTYPE_OBJECT)

static void
xobj_init (xobj_t *o)
{
}

static void
xobj_class_init (xobj_class_t *class)
{
  signals[SIGNAL1] =
    xsignal_new ("signal1",
                  XTYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, XTYPE_NONE, 0);
  signals[SIGNAL2] =
    xsignal_new ("signal2",
                  XTYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, XTYPE_NONE, 0);
}

static void
nop (void)
{
}

#define HANDLERS 500000

static void
test_connect_many (void)
{
  xobj_t *o;
  xdouble_t time_elapsed;
  xint_t i;

  o = xobject_new (xobj_get_type (), NULL);

  g_test_timer_start ();

  for (i = 0; i < HANDLERS; i++)
    xsignal_connect (o, "signal1", G_CALLBACK (nop), NULL);

  time_elapsed = g_test_timer_elapsed ();

  xobject_unref (o);

  g_test_minimized_result (time_elapsed, "connected %u handlers in %6.3f seconds", HANDLERS, time_elapsed);
}

static void
test_disconnect_many_ordered (void)
{
  xobj_t *o;
  xulong_t handlers[HANDLERS];
  xdouble_t time_elapsed;
  xint_t i;

  o = xobject_new (xobj_get_type (), NULL);

  for (i = 0; i < HANDLERS; i++)
    handlers[i] = xsignal_connect (o, "signal1", G_CALLBACK (nop), NULL);

  g_test_timer_start ();

  for (i = 0; i < HANDLERS; i++)
    xsignal_handler_disconnect (o, handlers[i]);

  time_elapsed = g_test_timer_elapsed ();

  xobject_unref (o);

  g_test_minimized_result (time_elapsed, "disconnected %u handlers in %6.3f seconds", HANDLERS, time_elapsed);
}

static void
test_disconnect_many_inverse (void)
{
  xobj_t *o;
  xulong_t handlers[HANDLERS];
  xdouble_t time_elapsed;
  xint_t i;

  o = xobject_new (xobj_get_type (), NULL);

  for (i = 0; i < HANDLERS; i++)
    handlers[i] = xsignal_connect (o, "signal1", G_CALLBACK (nop), NULL);

  g_test_timer_start ();

  for (i = HANDLERS - 1; i >= 0; i--)
    xsignal_handler_disconnect (o, handlers[i]);

  time_elapsed = g_test_timer_elapsed ();

  xobject_unref (o);

  g_test_minimized_result (time_elapsed, "disconnected %u handlers in %6.3f seconds", HANDLERS, time_elapsed);
}

static void
test_disconnect_many_random (void)
{
  xobj_t *o;
  xulong_t handlers[HANDLERS];
  xulong_t id;
  xdouble_t time_elapsed;
  xint_t i, j;

  o = xobject_new (xobj_get_type (), NULL);

  for (i = 0; i < HANDLERS; i++)
    handlers[i] = xsignal_connect (o, "signal1", G_CALLBACK (nop), NULL);

  for (i = 0; i < HANDLERS; i++)
    {
      j = g_test_rand_int_range (0, HANDLERS);
      id = handlers[i];
      handlers[i] = handlers[j];
      handlers[j] = id;
    }

  g_test_timer_start ();

  for (i = 0; i < HANDLERS; i++)
    xsignal_handler_disconnect (o, handlers[i]);

  time_elapsed = g_test_timer_elapsed ();

  xobject_unref (o);

  g_test_minimized_result (time_elapsed, "disconnected %u handlers in %6.3f seconds", HANDLERS, time_elapsed);
}

static void
test_disconnect_2_signals (void)
{
  xobj_t *o;
  xulong_t handlers[HANDLERS];
  xulong_t id;
  xdouble_t time_elapsed;
  xint_t i, j;

  o = xobject_new (xobj_get_type (), NULL);

  for (i = 0; i < HANDLERS; i++)
    {
      if (i % 2 == 0)
        handlers[i] = xsignal_connect (o, "signal1", G_CALLBACK (nop), NULL);
      else
        handlers[i] = xsignal_connect (o, "signal2", G_CALLBACK (nop), NULL);
    }

  for (i = 0; i < HANDLERS; i++)
    {
      j = g_test_rand_int_range (0, HANDLERS);
      id = handlers[i];
      handlers[i] = handlers[j];
      handlers[j] = id;
    }

  g_test_timer_start ();

  for (i = 0; i < HANDLERS; i++)
    xsignal_handler_disconnect (o, handlers[i]);

  time_elapsed = g_test_timer_elapsed ();

  xobject_unref (o);

  g_test_minimized_result (time_elapsed, "disconnected %u handlers in %6.3f seconds", HANDLERS, time_elapsed);
}

static void
test_disconnect_2_objects (void)
{
  xobj_t *o1, *o2, *o;
  xulong_t handlers[HANDLERS];
  xobj_t *objects[HANDLERS];
  xulong_t id;
  xdouble_t time_elapsed;
  xint_t i, j;

  o1 = xobject_new (xobj_get_type (), NULL);
  o2 = xobject_new (xobj_get_type (), NULL);

  for (i = 0; i < HANDLERS; i++)
    {
      if (i % 2 == 0)
        {
          handlers[i] = xsignal_connect (o1, "signal1", G_CALLBACK (nop), NULL);
          objects[i] = o1;
        }
      else
        {
          handlers[i] = xsignal_connect (o2, "signal1", G_CALLBACK (nop), NULL);
          objects[i] = o2;
        }
    }

  for (i = 0; i < HANDLERS; i++)
    {
      j = g_test_rand_int_range (0, HANDLERS);
      id = handlers[i];
      handlers[i] = handlers[j];
      handlers[j] = id;
      o = objects[i];
      objects[i] = objects[j];
      objects[j] = o;
    }

  g_test_timer_start ();

  for (i = 0; i < HANDLERS; i++)
    xsignal_handler_disconnect (objects[i], handlers[i]);

  time_elapsed = g_test_timer_elapsed ();

  xobject_unref (o1);
  xobject_unref (o2);

  g_test_minimized_result (time_elapsed, "disconnected %u handlers in %6.3f seconds", HANDLERS, time_elapsed);
}

static void
test_block_many (void)
{
  xobj_t *o;
  xulong_t handlers[HANDLERS];
  xulong_t id;
  xdouble_t time_elapsed;
  xint_t i, j;

  o = xobject_new (xobj_get_type (), NULL);

  for (i = 0; i < HANDLERS; i++)
    handlers[i] = xsignal_connect (o, "signal1", G_CALLBACK (nop), NULL);

  for (i = 0; i < HANDLERS; i++)
    {
      j = g_test_rand_int_range (0, HANDLERS);
      id = handlers[i];
      handlers[i] = handlers[j];
      handlers[j] = id;
    }

  g_test_timer_start ();

  for (i = 0; i < HANDLERS; i++)
    xsignal_handler_block (o, handlers[i]);

  for (i = HANDLERS - 1; i >= 0; i--)
    xsignal_handler_unblock (o, handlers[i]);

  time_elapsed = g_test_timer_elapsed ();

  xobject_unref (o);

  g_test_minimized_result (time_elapsed, "blocked and unblocked %u handlers in %6.3f seconds", HANDLERS, time_elapsed);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  if (g_test_perf ())
    {
      g_test_add_func ("/signal/handler/connect-many", test_connect_many);
      g_test_add_func ("/signal/handler/disconnect-many-ordered", test_disconnect_many_ordered);
      g_test_add_func ("/signal/handler/disconnect-many-inverse", test_disconnect_many_inverse);
      g_test_add_func ("/signal/handler/disconnect-many-random", test_disconnect_many_random);
      g_test_add_func ("/signal/handler/disconnect-2-signals", test_disconnect_2_signals);
      g_test_add_func ("/signal/handler/disconnect-2-objects", test_disconnect_2_objects);
      g_test_add_func ("/signal/handler/block-many", test_block_many);
    }

  return g_test_run ();
}
