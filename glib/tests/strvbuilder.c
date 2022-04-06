/*
 * Copyright © 2020 Canonical Ltd.
 * Copyright © 2021 Alexandros Theodotou
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

#include "glib.h"

static void
test_strvbuilder_empty (void)
{
  xstrv_builder_t *builder;
  xstrv_t result;

  builder = xstrv_builder_new ();
  result = xstrv_builder_end (builder);
  g_assert_nonnull (result);
  g_assert_cmpint (xstrv_length (result), ==, 0);

  xstrfreev (result);
  xstrv_builder_unref (builder);
}

static void
test_strvbuilder_add (void)
{
  xstrv_builder_t *builder;
  xstrv_t result;
  const xchar_t *expected[] = { "one", "two", "three", NULL };

  builder = xstrv_builder_new ();
  xstrv_builder_add (builder, "one");
  xstrv_builder_add (builder, "two");
  xstrv_builder_add (builder, "three");
  result = xstrv_builder_end (builder);
  g_assert_nonnull (result);
  g_assert_true (xstrv_equal ((const xchar_t *const *) result, expected));

  xstrfreev (result);
  xstrv_builder_unref (builder);
}

static void
test_strvbuilder_addv (void)
{
  xstrv_builder_t *builder;
  xstrv_t result;
  const xchar_t *expected[] = { "one", "two", "three", NULL };

  builder = xstrv_builder_new ();
  xstrv_builder_addv (builder, expected);
  result = xstrv_builder_end (builder);
  g_assert_nonnull (result);
  g_assert_cmpstrv ((const xchar_t *const *) result, expected);

  xstrfreev (result);
  xstrv_builder_unref (builder);
}

static void
test_strvbuilder_add_many (void)
{
  xstrv_builder_t *builder;
  xstrv_t result;
  const xchar_t *expected[] = { "one", "two", "three", NULL };

  builder = xstrv_builder_new ();
  xstrv_builder_add_many (builder, "one", "two", "three", NULL);
  result = xstrv_builder_end (builder);
  g_assert_nonnull (result);
  g_assert_cmpstrv ((const xchar_t *const *) result, expected);

  xstrfreev (result);
  xstrv_builder_unref (builder);
}

static void
test_strvbuilder_ref (void)
{
  xstrv_builder_t *builder;

  builder = xstrv_builder_new ();
  xstrv_builder_ref (builder);
  xstrv_builder_unref (builder);
  xstrv_builder_unref (builder);
}

int
main (int argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/strvbuilder/empty", test_strvbuilder_empty);
  g_test_add_func ("/strvbuilder/add", test_strvbuilder_add);
  g_test_add_func ("/strvbuilder/addv", test_strvbuilder_addv);
  g_test_add_func ("/strvbuilder/add_many", test_strvbuilder_add_many);
  g_test_add_func ("/strvbuilder/ref", test_strvbuilder_ref);

  return g_test_run ();
}
