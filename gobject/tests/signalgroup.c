/* xobject_t - GLib Type, Object, Parameter and Signal Library
 *
 * Copyright (C) 2015-2022 Christian Hergert <christian@hergert.me>
 * Copyright (C) 2015 Garrett Regier <garrettregier@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <glib-object.h>

G_DECLARE_FINAL_TYPE (signal_target, signal_target, TEST, SIGNAL_TARGET, xobject)

struct _signal_target
{
  xobject_t parent_instance;
};

XDEFINE_TYPE (signal_target, signal_target, XTYPE_OBJECT)

static G_DEFINE_QUARK (detail, signal_detail);

enum {
  THE_SIGNAL,
  NEVER_EMITTED,
  LAST_SIGNAL
};

static xuint_t signals[LAST_SIGNAL];

static void
signal_target_class_init (signal_target_class_t *klass)
{
  signals[THE_SIGNAL] =
      xsignal_new ("the-signal",
                    XTYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                    0,
                    NULL, NULL, NULL,
                    XTYPE_NONE,
                    1,
                    XTYPE_OBJECT);

  signals[NEVER_EMITTED] =
      xsignal_new ("never-emitted",
                    XTYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL, NULL, NULL,
                    XTYPE_NONE,
                    1,
                    XTYPE_OBJECT);
}

static void
signal_target_init (signal_target_t *self)
{
}

static xint_t global_signal_calls;
static xint_t global_weak_notify_called;

static void
connect_before_cb (signal_target_t *target,
                   xsignal_group_t *group,
                   xint_t         *signal_calls)
{
  signal_target_t *readback;

  g_assert_true (TEST_IS_SIGNAL_TARGET (target));
  g_assert_true (X_IS_SIGNAL_GROUP (group));
  g_assert_nonnull (signal_calls);
  g_assert_true (signal_calls == &global_signal_calls);

  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target);
  xobject_unref (readback);

  *signal_calls += 1;
}

static void
connect_after_cb (signal_target_t *target,
                  xsignal_group_t *group,
                  xint_t         *signal_calls)
{
  signal_target_t *readback;

  g_assert_true (TEST_IS_SIGNAL_TARGET (target));
  g_assert_true (X_IS_SIGNAL_GROUP (group));
  g_assert_nonnull (signal_calls);
  g_assert_true (signal_calls == &global_signal_calls);

  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target);
  xobject_unref (readback);

  g_assert_cmpint (*signal_calls, ==, 4);
  *signal_calls += 1;
}

static void
connect_swapped_cb (xint_t         *signal_calls,
                    xsignal_group_t *group,
                    signal_target_t *target)
{
  signal_target_t *readback;

  g_assert_true (signal_calls != NULL);
  g_assert_true (signal_calls == &global_signal_calls);
  g_assert_true (X_IS_SIGNAL_GROUP (group));
  g_assert_true (TEST_IS_SIGNAL_TARGET (target));

  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target);
  xobject_unref (readback);

  *signal_calls += 1;
}

static void
connect_object_cb (signal_target_t *target,
                   xsignal_group_t *group,
                   xobject_t      *object)
{
  signal_target_t *readback;
  xint_t *signal_calls;

  g_assert_true (TEST_IS_SIGNAL_TARGET (target));
  g_assert_true (X_IS_SIGNAL_GROUP (group));
  g_assert_true (X_IS_OBJECT (object));

  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target);
  xobject_unref (readback);

  signal_calls = xobject_get_data (object, "signal-calls");
  g_assert_nonnull (signal_calls);
  g_assert_true (signal_calls == &global_signal_calls);

  *signal_calls += 1;
}

static void
connect_bad_detail_cb (signal_target_t *target,
                       xsignal_group_t *group,
                       xobject_t      *object)
{
  xerror ("This detailed signal is never emitted!");
}

static void
connect_never_emitted_cb (signal_target_t *target,
                          xboolean_t     *weak_notify_called)
{
  xerror ("This signal is never emitted!");
}

static void
connect_data_notify_cb (xboolean_t *weak_notify_called,
                        xclosure_t *closure)
{
  g_assert_nonnull (weak_notify_called);
  g_assert_true (weak_notify_called == &global_weak_notify_called);
  g_assert_nonnull (closure);

  g_assert_false (*weak_notify_called);
  *weak_notify_called = TRUE;
}

static void
connect_data_weak_notify_cb (xboolean_t     *weak_notify_called,
                             xsignal_group_t *group)
{
  g_assert_nonnull (weak_notify_called);
  g_assert_true (weak_notify_called == &global_weak_notify_called);
  g_assert_true (X_IS_SIGNAL_GROUP (group));

  g_assert_true (*weak_notify_called);
}

static void
connect_all_signals (xsignal_group_t *group)
{
  xobject_t *object;

  /* Check that these are called in the right order */
  xsignal_group_connect (group,
                          "the-signal",
                          G_CALLBACK (connect_before_cb),
                          &global_signal_calls);
  xsignal_group_connect_after (group,
                                "the-signal",
                                G_CALLBACK (connect_after_cb),
                                &global_signal_calls);

  /* Check that this is called with the arguments swapped */
  xsignal_group_connect_swapped (group,
                                  "the-signal",
                                  G_CALLBACK (connect_swapped_cb),
                                  &global_signal_calls);

  /* Check that this is called with the arguments swapped */
  object = xobject_new (XTYPE_OBJECT, NULL);
  xobject_set_data (object, "signal-calls", &global_signal_calls);
  xsignal_group_connect_object (group,
                                 "the-signal",
                                 G_CALLBACK (connect_object_cb),
                                 object,
                                 0);
  xobject_weak_ref (G_OBJECT (group),
                     (GWeakNotify)xobject_unref,
                     object);

  /* Check that a detailed signal is handled correctly */
  xsignal_group_connect (group,
                          "the-signal::detail",
                          G_CALLBACK (connect_before_cb),
                          &global_signal_calls);
  xsignal_group_connect (group,
                          "the-signal::bad-detail",
                          G_CALLBACK (connect_bad_detail_cb),
                          NULL);

  /* Check that the notify is called correctly */
  global_weak_notify_called = FALSE;
  xsignal_group_connect_data (group,
                               "never-emitted",
                               G_CALLBACK (connect_never_emitted_cb),
                               &global_weak_notify_called,
                               (xclosure_notify_t)connect_data_notify_cb,
                               0);
  xobject_weak_ref (G_OBJECT (group),
                     (GWeakNotify)connect_data_weak_notify_cb,
                     &global_weak_notify_called);
}

static void
assert_signals (signal_target_t *target,
                xsignal_group_t *group,
                xboolean_t      success)
{
  xassert (TEST_IS_SIGNAL_TARGET (target));
  xassert (group == NULL || X_IS_SIGNAL_GROUP (group));

  global_signal_calls = 0;
  xsignal_emit (target, signals[THE_SIGNAL],
                 signal_detail_quark (), group);
  g_assert_cmpint (global_signal_calls, ==, success ? 5 : 0);
}

static void
dummy_handler (void)
{
}

static void
test_signal_group_invalid (void)
{
  xobject_t *invalid_target = xobject_new (XTYPE_OBJECT, NULL);
  signal_target_t *target = xobject_new (signal_target_get_type (), NULL);
  xsignal_group_t *group = xsignal_group_new (signal_target_get_type ());

  /* Invalid Target Type */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*xtype_is_a*XTYPE_OBJECT*");
  xsignal_group_new (XTYPE_DATE_TIME);
  g_test_assert_expected_messages ();

  /* Invalid Target */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*Failed to set xsignal_group_t of target type signal_target_t using target * of type xobject_t*");
  xsignal_group_set_target (group, invalid_target);
  g_assert_finalize_object (group);
  g_test_assert_expected_messages ();

  /* Invalid Signal Name */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*xsignal_parse_name*");
  group = xsignal_group_new (signal_target_get_type ());
  xsignal_group_connect (group,
                          "does-not-exist",
                          G_CALLBACK (connect_before_cb),
                          NULL);
  g_test_assert_expected_messages ();
  g_assert_finalize_object (group);

  /* Invalid Callback */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*c_handler != NULL*");
  group = xsignal_group_new (signal_target_get_type ());
  xsignal_group_connect (group,
                          "the-signal",
                          G_CALLBACK (NULL),
                          NULL);
  g_test_assert_expected_messages ();
  g_assert_finalize_object (group);

  /* Connecting after setting target */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*Cannot add signals after setting target*");
  group = xsignal_group_new (signal_target_get_type ());
  xsignal_group_set_target (group, target);
  xsignal_group_connect (group,
                          "the-signal",
                          G_CALLBACK (dummy_handler),
                          NULL);
  g_test_assert_expected_messages ();
  g_assert_finalize_object (group);

  g_assert_finalize_object (target);
  g_assert_finalize_object (invalid_target);
}

static void
test_signal_group_simple (void)
{
  signal_target_t *target;
  xsignal_group_t *group;
  signal_target_t *readback;

  /* Set the target before connecting the signals */
  group = xsignal_group_new (signal_target_get_type ());
  target = xobject_new (signal_target_get_type (), NULL);
  g_assert_null (xsignal_group_dup_target (group));
  xsignal_group_set_target (group, target);
  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target);
  xobject_unref (readback);
  g_assert_finalize_object (group);
  assert_signals (target, NULL, FALSE);
  g_assert_finalize_object (target);

  group = xsignal_group_new (signal_target_get_type ());
  target = xobject_new (signal_target_get_type (), NULL);
  connect_all_signals (group);
  xsignal_group_set_target (group, target);
  assert_signals (target, group, TRUE);
  g_assert_finalize_object (target);
  g_assert_finalize_object (group);
}

static void
test_signal_group_changing_target (void)
{
  signal_target_t *target1, *target2;
  xsignal_group_t *group = xsignal_group_new (signal_target_get_type ());
  signal_target_t *readback;

  connect_all_signals (group);
  g_assert_null (xsignal_group_dup_target (group));

  /* Set the target after connecting the signals */
  target1 = xobject_new (signal_target_get_type (), NULL);
  xsignal_group_set_target (group, target1);
  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target1);
  xobject_unref (readback);

  assert_signals (target1, group, TRUE);

  /* Set the same target */
  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target1);
  xobject_unref (readback);
  xsignal_group_set_target (group, target1);

  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target1);
  xobject_unref (readback);

  assert_signals (target1, group, TRUE);

  /* Set a new target when the current target is non-NULL */
  target2 = xobject_new (signal_target_get_type (), NULL);
  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target1);
  xobject_unref (readback);

  xsignal_group_set_target (group, target2);
  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target2);
  xobject_unref (readback);

  assert_signals (target2, group, TRUE);

  g_assert_finalize_object (target2);
  g_assert_finalize_object (target1);
  g_assert_finalize_object (group);
}

static void
assert_blocking (signal_target_t *target,
                 xsignal_group_t *group,
                 xint_t          count)
{
  xint_t i;

  assert_signals (target, group, TRUE);

  /* Assert that multiple blocks are effective */
  for (i = 0; i < count; ++i)
    {
      xsignal_group_block (group);
      assert_signals (target, group, FALSE);
    }

  /* Assert that the signal is not emitted after the first unblock */
  for (i = 0; i < count; ++i)
    {
      assert_signals (target, group, FALSE);
      xsignal_group_unblock (group);
    }

  assert_signals (target, group, TRUE);
}

static void
test_signal_group_blocking (void)
{
  signal_target_t *target1, *target2, *readback;
  xsignal_group_t *group = xsignal_group_new (signal_target_get_type ());

  /* test_t blocking and unblocking null target */
  xsignal_group_block (group);
  xsignal_group_unblock (group);

  connect_all_signals (group);
  g_assert_null (xsignal_group_dup_target (group));

  target1 = xobject_new (signal_target_get_type (), NULL);
  xsignal_group_set_target (group, target1);
  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target1);
  xobject_unref (readback);

  assert_blocking (target1, group, 1);
  assert_blocking (target1, group, 3);
  assert_blocking (target1, group, 15);

  /* Assert that blocking transfers across changing the target */
  xsignal_group_block (group);
  xsignal_group_block (group);

  /* Set a new target when the current target is non-NULL */
  target2 = xobject_new (signal_target_get_type (), NULL);
  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target1);
  xobject_unref (readback);
  xsignal_group_set_target (group, target2);
  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target2);
  xobject_unref (readback);

  assert_signals (target2, group, FALSE);
  xsignal_group_unblock (group);
  assert_signals (target2, group, FALSE);
  xsignal_group_unblock (group);
  assert_signals (target2, group, TRUE);

  g_assert_finalize_object (target2);
  g_assert_finalize_object (target1);
  g_assert_finalize_object (group);
}

static void
test_signal_group_weak_ref_target (void)
{
  signal_target_t *target = xobject_new (signal_target_get_type (), NULL);
  xsignal_group_t *group = xsignal_group_new (signal_target_get_type ());
  signal_target_t *readback;

  g_assert_null (xsignal_group_dup_target (group));
  xsignal_group_set_target (group, target);
  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target);
  xobject_unref (readback);

  g_assert_finalize_object (target);
  g_assert_null (xsignal_group_dup_target (group));
  g_assert_finalize_object (group);
}

static void
test_signal_group_connect_object (void)
{
  xobject_t *object = xobject_new (XTYPE_OBJECT, NULL);
  signal_target_t *target = xobject_new (signal_target_get_type (), NULL);
  xsignal_group_t *group = xsignal_group_new (signal_target_get_type ());
  signal_target_t *readback;

  /* We already do basic connect_object() tests in connect_signals(),
   * this is only needed to test the specifics of connect_object()
   */
  xsignal_group_connect_object (group,
                                 "the-signal",
                                 G_CALLBACK (connect_object_cb),
                                 object,
                                 0);

  g_assert_null (xsignal_group_dup_target (group));
  xsignal_group_set_target (group, target);
  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target);
  xobject_unref (readback);

  g_assert_finalize_object (object);

  /* This would cause a warning if the SignalGroup did not
   * have a weakref on the object as it would try to connect again
   */
  xsignal_group_set_target (group, NULL);
  g_assert_null (xsignal_group_dup_target (group));
  xsignal_group_set_target (group, target);
  readback = xsignal_group_dup_target (group);
  g_assert_true (readback == target);
  xobject_unref (readback);

  g_assert_finalize_object (group);
  g_assert_finalize_object (target);
}

static void
test_signal_group_signal_parsing (void)
{
  g_test_trap_subprocess ("/xobject_t/SignalGroup/signal-parsing/subprocess", 0,
                          G_TEST_SUBPROCESS_INHERIT_STDERR);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("");
}

static void
test_signal_group_signal_parsing_subprocess (void)
{
  xsignal_group_t *group;

  /* Check that the class has not been created and with it the
   * signals registered. This will cause xsignal_parse_name()
   * to fail unless xsignal_group_t calls xtype_class_ref().
   */
  g_assert_null (xtype_class_peek (signal_target_get_type ()));

  group = xsignal_group_new (signal_target_get_type ());
  xsignal_group_connect (group,
                          "the-signal",
                          G_CALLBACK (connect_before_cb),
                          NULL);

  g_assert_finalize_object (group);
}

static void
test_signal_group_properties (void)
{
  xsignal_group_t *group;
  signal_target_t *target, *other;
  xtype_t gtype;

  group = xsignal_group_new (signal_target_get_type ());
  xobject_get (group,
                "target", &target,
                "target-type", &gtype,
                NULL);
  g_assert_cmpint (gtype, ==, signal_target_get_type ());
  g_assert_null (target);

  target = xobject_new (signal_target_get_type (), NULL);
  xobject_set (group, "target", target, NULL);
  xobject_get (group, "target", &other, NULL);
  g_assert_true (target == other);
  xobject_unref (other);

  g_assert_finalize_object (target);
  g_assert_null (xsignal_group_dup_target (group));
  g_assert_finalize_object (group);
}

G_DECLARE_INTERFACE (signal_thing, signal_thing, SIGNAL, THING, xobject)

struct _signal_thing_interface
{
  xtype_interface_t iface;
  void (*changed) (signal_thing_t *thing);
};

G_DEFINE_INTERFACE (signal_thing, signal_thing, XTYPE_OBJECT)

static xuint_t signal_thing_changed;

static void
signal_thing_default_init (signal_thing_interface_t *iface)
{
  signal_thing_changed =
      xsignal_new ("changed",
                    XTYPE_FROM_INTERFACE (iface),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (signal_thing_interface_t, changed),
                    NULL, NULL, NULL,
                    XTYPE_NONE, 0);
}

G_GNUC_NORETURN static void
thing_changed_cb (signal_thing_t *thing,
                  xpointer_t     user_data G_GNUC_UNUSED)
{
  g_assert_not_reached ();
}

static void
test_signal_group_interface (void)
{
  xsignal_group_t *group;

  group = xsignal_group_new (signal_thing_get_type ());
  xsignal_group_connect (group,
                          "changed",
                          G_CALLBACK (thing_changed_cb),
                          NULL);
  g_assert_finalize_object (group);
}

xint_t
main (xint_t   argc,
      xchar_t *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/xobject_t/SignalGroup/invalid", test_signal_group_invalid);
  g_test_add_func ("/xobject_t/SignalGroup/simple", test_signal_group_simple);
  g_test_add_func ("/xobject_t/SignalGroup/changing-target", test_signal_group_changing_target);
  g_test_add_func ("/xobject_t/SignalGroup/blocking", test_signal_group_blocking);
  g_test_add_func ("/xobject_t/SignalGroup/weak-ref-target", test_signal_group_weak_ref_target);
  g_test_add_func ("/xobject_t/SignalGroup/connect-object", test_signal_group_connect_object);
  g_test_add_func ("/xobject_t/SignalGroup/signal-parsing", test_signal_group_signal_parsing);
  g_test_add_func ("/xobject_t/SignalGroup/signal-parsing/subprocess", test_signal_group_signal_parsing_subprocess);
  g_test_add_func ("/xobject_t/SignalGroup/properties", test_signal_group_properties);
  g_test_add_func ("/xobject_t/SignalGroup/interface", test_signal_group_interface);
  return g_test_run ();
}
