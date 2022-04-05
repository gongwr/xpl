/*
 * Copyright © 2009 Ryan Lortie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 */

#include <glib/glib.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static xobject_t      *got_source;
static xasync_result_t *got_result;
static xpointer_t      got_user_data;

static void
ensure_destroyed (xpointer_t obj)
{
  g_object_add_weak_pointer (obj, &obj);
  g_object_unref (obj);
  g_assert (obj == NULL);
}

static void
reset (void)
{
  got_source = NULL;

  if (got_result)
    ensure_destroyed (got_result);

  got_result = NULL;
  got_user_data = NULL;
}

static void
check (xpointer_t a, xpointer_t b, xpointer_t c)
{
  g_assert (a == got_source);
  g_assert (b == got_result);
  g_assert (c == got_user_data);
}

static void
callback_func (xobject_t      *source,
               xasync_result_t *result,
               xpointer_t      user_data)
{
  got_source = source;
  got_result = g_object_ref (result);
  got_user_data = user_data;
}

static xboolean_t
test_simple_async_idle (xpointer_t user_data)
{
  GSimpleAsyncResult *result;
  xobject_t *a, *b, *c;
  xboolean_t *ran = user_data;

  a = g_object_new (XTYPE_OBJECT, NULL);
  b = g_object_new (XTYPE_OBJECT, NULL);
  c = g_object_new (XTYPE_OBJECT, NULL);

  result = g_simple_async_result_new (a, callback_func, b, test_simple_async_idle);
  g_assert (g_async_result_get_user_data (G_ASYNC_RESULT (result)) == b);
  check (NULL, NULL, NULL);
  g_simple_async_result_complete (result);
  check (a, result, b);
  g_object_unref (result);

  g_assert (g_simple_async_result_is_valid (got_result, a, test_simple_async_idle));
  g_assert (!g_simple_async_result_is_valid (got_result, b, test_simple_async_idle));
  g_assert (!g_simple_async_result_is_valid (got_result, c, test_simple_async_idle));
  g_assert (!g_simple_async_result_is_valid (got_result, b, callback_func));
  g_assert (!g_simple_async_result_is_valid ((xpointer_t) a, NULL, NULL));
  reset ();
  reset ();
  reset ();

  ensure_destroyed (a);
  ensure_destroyed (b);
  ensure_destroyed (c);

  *ran = TRUE;
  return G_SOURCE_REMOVE;
}

static void
test_simple_async (void)
{
  GSimpleAsyncResult *result;
  xobject_t *a, *b;
  xboolean_t ran_test_in_idle = FALSE;

  g_idle_add (test_simple_async_idle, &ran_test_in_idle);
  g_main_context_iteration (NULL, FALSE);

  g_assert (ran_test_in_idle);

  a = g_object_new (XTYPE_OBJECT, NULL);
  b = g_object_new (XTYPE_OBJECT, NULL);

  result = g_simple_async_result_new (a, callback_func, b, test_simple_async);
  check (NULL, NULL, NULL);
  g_simple_async_result_complete_in_idle (result);
  g_object_unref (result);
  check (NULL, NULL, NULL);
  g_main_context_iteration (NULL, FALSE);
  check (a, result, b);
  reset ();

  ensure_destroyed (a);
  ensure_destroyed (b);
}

static void
test_valid (void)
{
  xasync_result_t *result;
  xobject_t *a, *b;

  a = g_object_new (XTYPE_OBJECT, NULL);
  b = g_object_new (XTYPE_OBJECT, NULL);

  /* Without source or tag */
  result = (xasync_result_t *) g_simple_async_result_new (NULL, NULL, NULL, NULL);
  g_assert_true (g_simple_async_result_is_valid (result, NULL, NULL));
  g_assert_true (g_simple_async_result_is_valid (result, NULL, test_valid));
  g_assert_true (g_simple_async_result_is_valid (result, NULL, test_simple_async));
  g_assert_false (g_simple_async_result_is_valid (result, a, NULL));
  g_assert_false (g_simple_async_result_is_valid (result, a, test_valid));
  g_assert_false (g_simple_async_result_is_valid (result, a, test_simple_async));
  g_object_unref (result);

  /* Without source, with tag */
  result = (xasync_result_t *) g_simple_async_result_new (NULL, NULL, NULL, test_valid);
  g_assert_true (g_simple_async_result_is_valid (result, NULL, NULL));
  g_assert_true (g_simple_async_result_is_valid (result, NULL, test_valid));
  g_assert_false (g_simple_async_result_is_valid (result, NULL, test_simple_async));
  g_assert_false (g_simple_async_result_is_valid (result, a, NULL));
  g_assert_false (g_simple_async_result_is_valid (result, a, test_valid));
  g_assert_false (g_simple_async_result_is_valid (result, a, test_simple_async));
  g_object_unref (result);

  /* With source, without tag */
  result = (xasync_result_t *) g_simple_async_result_new (a, NULL, NULL, NULL);
  g_assert_true (g_simple_async_result_is_valid (result, a, NULL));
  g_assert_true (g_simple_async_result_is_valid (result, a, test_valid));
  g_assert_true (g_simple_async_result_is_valid (result, a, test_simple_async));
  g_assert_false (g_simple_async_result_is_valid (result, NULL, NULL));
  g_assert_false (g_simple_async_result_is_valid (result, NULL, test_valid));
  g_assert_false (g_simple_async_result_is_valid (result, NULL, test_simple_async));
  g_assert_false (g_simple_async_result_is_valid (result, b, NULL));
  g_assert_false (g_simple_async_result_is_valid (result, b, test_valid));
  g_assert_false (g_simple_async_result_is_valid (result, b, test_simple_async));
  g_object_unref (result);

  /* With source and tag */
  result = (xasync_result_t *) g_simple_async_result_new (a, NULL, NULL, test_valid);
  g_assert_true (g_simple_async_result_is_valid (result, a, test_valid));
  g_assert_true (g_simple_async_result_is_valid (result, a, NULL));
  g_assert_false (g_simple_async_result_is_valid (result, a, test_simple_async));
  g_assert_false (g_simple_async_result_is_valid (result, NULL, NULL));
  g_assert_false (g_simple_async_result_is_valid (result, NULL, test_valid));
  g_assert_false (g_simple_async_result_is_valid (result, NULL, test_simple_async));
  g_assert_false (g_simple_async_result_is_valid (result, b, NULL));
  g_assert_false (g_simple_async_result_is_valid (result, b, test_valid));
  g_assert_false (g_simple_async_result_is_valid (result, b, test_simple_async));
  g_object_unref (result);

  /* Non-GSimpleAsyncResult */
  result = (xasync_result_t *) g_task_new (NULL, NULL, NULL, NULL);
  g_assert_false (g_simple_async_result_is_valid (result, NULL, NULL));
  g_object_unref (result);

  g_object_unref (a);
  g_object_unref (b);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gio/simple-async-result/test", test_simple_async);
  g_test_add_func ("/gio/simple-async-result/valid", test_valid);

  return g_test_run();
}

G_GNUC_END_IGNORE_DEPRECATIONS
