// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2021  Emmanuele Bassi

#include <glib-object.h>

#define TEST_TYPE_FINAL (test_final_get_type())
G_DECLARE_FINAL_TYPE (test_final, test_final, TEST, FINAL, xobject)

struct _test_final
{
  xobject_t parent_instance;
};

struct _test_final_class
{
  xobject_class_t parent_class;
};

G_DEFINE_FINAL_TYPE (test_final, test_final, XTYPE_OBJECT)

static void
test_final_class_init (test_final_class_t *klass)
{
}

static void
test_final_init (test_final_t *self)
{
}

#define TEST_TYPE_FINAL2 (test_final2_get_type())
G_DECLARE_FINAL_TYPE (test_final2, test_final2, TEST, FINAL2, test_final)

struct _test_final2
{
  test_final_t parent_instance;
};

struct _test_final2_class
{
  test_final_class_t parent_class;
};

XDEFINE_TYPE (test_final2, test_final2, TEST_TYPE_FINAL)

static void
test_final2_class_init (test_final2_class_t *klass)
{
}

static void
test_final2_init (test_final2_t *self)
{
}

/* test_type_flags_final: Check that trying to derive from a final class
 * will result in a warning from the type system
 */
static void
test_type_flags_final (void)
{
  xtype_t final2_type;

  /* This is the message we print out when registering the type */
  g_test_expect_message ("GLib-xobject_t", G_LOG_LEVEL_WARNING,
                         "*cannot derive*");

  /* This is the message when we fail to return from the GOnce init
   * block within the test_final2_get_type() function
   */
  g_test_expect_message ("GLib", G_LOG_LEVEL_CRITICAL,
                         "*g_once_init_leave: assertion*");

  final2_type = TEST_TYPE_FINAL2;
  g_assert_true (final2_type == XTYPE_INVALID);

  g_test_assert_expected_messages ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/type/flags/final", test_type_flags_final);

  return g_test_run ();
}
