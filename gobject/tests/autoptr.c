/* GLib testing framework examples and tests
 * Copyright (C) 2018 Canonical Ltd
 * Authors: Marco Trevisan <marco@ubuntu.com>
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

#include <glib-object.h>
#include <string.h>

G_DECLARE_DERIVABLE_TYPE (test_auto_cleanup_base, test_base_auto_cleanup, TEST, BASE_AUTO_CLEANUP, xobject)

struct _test_auto_cleanup_base_class {
  xobject_class_t parent_class;
};

G_DEFINE_TYPE (test_auto_cleanup_base, test_base_auto_cleanup, XTYPE_OBJECT)

static void
test_base_auto_cleanup_class_init (test_auto_cleanup_base_class_t *class)
{
}

static void
test_base_auto_cleanup_init (test_auto_cleanup_base_t *tac)
{
}

G_DECLARE_FINAL_TYPE (test_auto_cleanup, test_auto_cleanup, TEST, AUTO_CLEANUP, test_auto_cleanup_base)

struct _test_auto_cleanup
{
  test_auto_cleanup_base_t parent_instance;
};

G_DEFINE_TYPE (test_auto_cleanup, test_auto_cleanup, XTYPE_OBJECT)

static void
test_auto_cleanup_class_init (test_auto_cleanup_class_t *class)
{
}

static void
test_auto_cleanup_init (test_auto_cleanup_t *tac)
{
}

static test_auto_cleanup_t *
test_auto_cleanup_new (void)
{
  return xobject_new (test_auto_cleanup_get_type (), NULL);
}

/* Verify that an object declared with G_DECLARE_FINAL_TYPE provides by default
 * autocleanup functions, defined using the ones of the base type (defined with
 * G_DECLARE_DERIVABLE_TYPE) and so that it can be used with x_autoptr */
static void
test_autoptr (void)
{
  test_auto_cleanup_t *tac_ptr = test_auto_cleanup_new ();
  xobject_add_weak_pointer (G_OBJECT (tac_ptr), (xpointer_t *) &tac_ptr);

  {
    x_autoptr (test_auto_cleanup) tac = tac_ptr;
    g_assert_nonnull (tac);
  }
#ifdef __GNUC__
  g_assert_null (tac_ptr);
#endif
}

/* Verify that an object declared with G_DECLARE_FINAL_TYPE provides by default
 * autocleanup functions, defined using the ones of the base type (defined with
 * G_DECLARE_DERIVABLE_TYPE) and that stealing an autopointer works properly */
static void
test_autoptr_steal (void)
{
  x_autoptr (test_auto_cleanup) tac1 = test_auto_cleanup_new ();
  test_auto_cleanup_t *tac_ptr = tac1;

  xobject_add_weak_pointer (G_OBJECT (tac_ptr), (xpointer_t *) &tac_ptr);

  {
    x_autoptr (test_auto_cleanup) tac2 = g_steal_pointer (&tac1);
    g_assert_nonnull (tac_ptr);
    g_assert_null (tac1);
    g_assert_true (tac2 == tac_ptr);
  }
#ifdef __GNUC__
  g_assert_null (tac_ptr);
#endif
}

/* Verify that an object declared with G_DECLARE_FINAL_TYPE provides by default
 * autolist cleanup functions defined using the ones of the parent type
 * and so that can be used with g_autolist, and that freeing the list correctly
 * unrefs the object too */
static void
test_autolist (void)
{
  test_auto_cleanup_t *tac1 = test_auto_cleanup_new ();
  test_auto_cleanup_t *tac2 = test_auto_cleanup_new ();
  x_autoptr (test_auto_cleanup) tac3 = test_auto_cleanup_new ();

  xobject_add_weak_pointer (G_OBJECT (tac1), (xpointer_t *) &tac1);
  xobject_add_weak_pointer (G_OBJECT (tac2), (xpointer_t *) &tac2);
  xobject_add_weak_pointer (G_OBJECT (tac3), (xpointer_t *) &tac3);

  {
    g_autolist (test_auto_cleanup) l = NULL;

    l = xlist_prepend (l, tac1);
    l = xlist_prepend (l, tac2);

    /* Squash warnings about dead stores */
    (void) l;
  }

  /* Only assert if autoptr works */
#ifdef __GNUC__
  g_assert_null (tac1);
  g_assert_null (tac2);
#endif
  g_assert_nonnull (tac3);

  g_clear_object (&tac3);
  g_assert_null (tac3);
}

/* Verify that an object declared with G_DECLARE_FINAL_TYPE provides by default
 * autoslist cleanup functions (defined using the ones of the base type declared
 * with G_DECLARE_DERIVABLE_TYPE) and so that can be used with g_autoslist, and
 * that freeing the slist correctly unrefs the object too */
static void
test_autoslist (void)
{
  test_auto_cleanup_t *tac1 = test_auto_cleanup_new ();
  test_auto_cleanup_t *tac2 = test_auto_cleanup_new ();
  x_autoptr (test_auto_cleanup) tac3 = test_auto_cleanup_new ();

  xobject_add_weak_pointer (G_OBJECT (tac1), (xpointer_t *) &tac1);
  xobject_add_weak_pointer (G_OBJECT (tac2), (xpointer_t *) &tac2);
  xobject_add_weak_pointer (G_OBJECT (tac3), (xpointer_t *) &tac3);

  {
    g_autoslist (test_auto_cleanup) l = NULL;

    l = xslist_prepend (l, tac1);
    l = xslist_prepend (l, tac2);
  }

  /* Only assert if autoptr works */
#ifdef __GNUC__
  g_assert_null (tac1);
  g_assert_null (tac2);
#endif
  g_assert_nonnull (tac3);

  g_clear_object (&tac3);
  g_assert_null (tac3);
}

/* Verify that an object declared with G_DECLARE_FINAL_TYPE provides by default
 * autoqueue cleanup functions (defined using the ones of the base type declared
 * with G_DECLARE_DERIVABLE_TYPE) and so that can be used with g_autoqueue, and
 * that freeing the queue correctly unrefs the object too */
static void
test_autoqueue (void)
{
  test_auto_cleanup_t *tac1 = test_auto_cleanup_new ();
  test_auto_cleanup_t *tac2 = test_auto_cleanup_new ();
  x_autoptr (test_auto_cleanup) tac3 = test_auto_cleanup_new ();

  xobject_add_weak_pointer (G_OBJECT (tac1), (xpointer_t *) &tac1);
  xobject_add_weak_pointer (G_OBJECT (tac2), (xpointer_t *) &tac2);
  xobject_add_weak_pointer (G_OBJECT (tac3), (xpointer_t *) &tac3);

  {
    g_autoqueue (test_auto_cleanup) q = g_queue_new ();

    g_queue_push_head (q, tac1);
    g_queue_push_tail (q, tac2);
  }

  /* Only assert if autoptr works */
#ifdef __GNUC__
  g_assert_null (tac1);
  g_assert_null (tac2);
#endif
  g_assert_nonnull (tac3);

  g_clear_object (&tac3);
  g_assert_null (tac3);
}

static void
test_autoclass (void)
{
  x_autoptr (test_auto_cleanup_base) base_class_ptr = NULL;
  x_autoptr (test_auto_cleanup) class_ptr = NULL;

  base_class_ptr = xtype_class_ref (test_base_auto_cleanup_get_type ());
  class_ptr = xtype_class_ref (test_auto_cleanup_get_type ());

  g_assert_nonnull (base_class_ptr);
  g_assert_nonnull (class_ptr);
}

int
main (int argc, xchar_t *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/autoptr/autoptr", test_autoptr);
  g_test_add_func ("/autoptr/autoptr_steal", test_autoptr_steal);
  g_test_add_func ("/autoptr/autolist", test_autolist);
  g_test_add_func ("/autoptr/autoslist", test_autoslist);
  g_test_add_func ("/autoptr/autoqueue", test_autoqueue);
  g_test_add_func ("/autoptr/autoclass", test_autoclass);

  return g_test_run ();
}
