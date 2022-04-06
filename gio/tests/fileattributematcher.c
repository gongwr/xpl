#include <gio/gio.h>

static void
test_exact (void)
{
  const char *exact_matches[] = {
    "*",
    "a::*",
    "a::*,b::*",
    "a::a,a::b",
    "a::a,a::b,b::*"
  };

  xfile_attribute_matcher_t *matcher;
  char *s;
  xuint_t i;

  for (i = 0; i < G_N_ELEMENTS (exact_matches); i++)
    {
      matcher = xfile_attribute_matcher_new (exact_matches[i]);
      s = xfile_attribute_matcher_to_string (matcher);
      g_assert_cmpstr (exact_matches[i], ==, s);
      g_free (s);
      xfile_attribute_matcher_unref (matcher);
    }
}

static void
test_equality (void)
{
  struct {
    const char *expected;
    const char *actual;
  } equals[] = {
    /* star makes everything else go away */
    { "*", "*,*" },
    { "*", "*,a::*" },
    { "*", "*,a::b" },
    { "*", "a::*,*" },
    { "*", "a::b,*" },
    { "*", "a::b,*,a::*" },
    /* a::* makes a::<anything> go away */
    { "a::*", "a::*,a::*" },
    { "a::*", "a::*,a::b" },
    { "a::*", "a::b,a::*" },
    { "a::*", "a::b,a::*,a::c" },
    /* a::b does not allow duplicates */
    { "a::b", "a::b,a::b" },
    { "a::b,a::c", "a::b,a::c,a::b" },
    /* stuff gets ordered in registration order */
    { "a::b,a::c", "a::c,a::b" },
    { "a::*,b::*", "b::*,a::*" },
  };

  xfile_attribute_matcher_t *matcher;
  char *s;
  xuint_t i;

  for (i = 0; i < G_N_ELEMENTS (equals); i++)
    {
      matcher = xfile_attribute_matcher_new (equals[i].actual);
      s = xfile_attribute_matcher_to_string (matcher);
      g_assert_cmpstr (equals[i].expected, ==, s);
      g_free (s);
      xfile_attribute_matcher_unref (matcher);
    }
}

static void
test_subtract (void)
{
  struct {
    const char *attributes;
    const char *subtract;
    const char *result;
  } subtractions[] = {
    /* * subtracts everything */
    { "*", "*", NULL },
    { "a::*", "*", NULL },
    { "a::b", "*", NULL },
    { "a::b,a::c", "*", NULL },
    { "a::*,b::*", "*", NULL },
    { "a::*,b::c", "*", NULL },
    { "a::b,b::*", "*", NULL },
    { "a::b,b::c", "*", NULL },
    { "a::b,a::c,b::*", "*", NULL },
    { "a::b,a::c,b::c", "*", NULL },
    /* a::* subtracts all a's */
    { "*", "a::*", "*" },
    { "a::*", "a::*", NULL },
    { "a::b", "a::*", NULL },
    { "a::b,a::c", "a::*", NULL },
    { "a::*,b::*", "a::*", "b::*" },
    { "a::*,b::c", "a::*", "b::c" },
    { "a::b,b::*", "a::*", "b::*" },
    { "a::b,b::c", "a::*", "b::c" },
    { "a::b,a::c,b::*", "a::*", "b::*" },
    { "a::b,a::c,b::c", "a::*", "b::c" },
    /* a::b subtracts exactly that */
    { "*", "a::b", "*" },
    { "a::*", "a::b", "a::*" },
    { "a::b", "a::b", NULL },
    { "a::b,a::c", "a::b", "a::c" },
    { "a::*,b::*", "a::b", "a::*,b::*" },
    { "a::*,b::c", "a::b", "a::*,b::c" },
    { "a::b,b::*", "a::b", "b::*" },
    { "a::b,b::c", "a::b", "b::c" },
    { "a::b,a::c,b::*", "a::b", "a::c,b::*" },
    { "a::b,a::c,b::c", "a::b", "a::c,b::c" },
    /* a::b,b::* subtracts both of those */
    { "*", "a::b,b::*", "*" },
    { "a::*", "a::b,b::*", "a::*" },
    { "a::b", "a::b,b::*", NULL },
    { "a::b,a::c", "a::b,b::*", "a::c" },
    { "a::*,b::*", "a::b,b::*", "a::*" },
    { "a::*,b::c", "a::b,b::*", "a::*" },
    { "a::b,b::*", "a::b,b::*", NULL },
    { "a::b,b::c", "a::b,b::*", NULL },
    { "a::b,a::c,b::*", "a::b,b::*", "a::c" },
    { "a::b,a::c,b::c", "a::b,b::*", "a::c" },
    /* a::b,b::c should work, too */
    { "*", "a::b,b::c", "*" },
    { "a::*", "a::b,b::c", "a::*" },
    { "a::b", "a::b,b::c", NULL },
    { "a::b,a::c", "a::b,b::c", "a::c" },
    { "a::*,b::*", "a::b,b::c", "a::*,b::*" },
    { "a::*,b::c", "a::b,b::c", "a::*" },
    { "a::b,b::*", "a::b,b::c", "b::*" },
    { "a::b,b::c", "a::b,b::c", NULL },
    { "a::b,a::c,b::*", "a::b,b::c", "a::c,b::*" },
    { "a::b,a::c,b::c", "a::b,b::c", "a::c" },
  };

  xfile_attribute_matcher_t *matcher, *subtract, *result;
  char *s;
  xuint_t i;

  for (i = 0; i < G_N_ELEMENTS (subtractions); i++)
    {
      matcher = xfile_attribute_matcher_new (subtractions[i].attributes);
      subtract = xfile_attribute_matcher_new (subtractions[i].subtract);
      result = xfile_attribute_matcher_subtract (matcher, subtract);
      s = xfile_attribute_matcher_to_string (result);
      g_assert_cmpstr (subtractions[i].result, ==, s);
      g_free (s);
      xfile_attribute_matcher_unref (matcher);
      xfile_attribute_matcher_unref (subtract);
      xfile_attribute_matcher_unref (result);
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/fileattributematcher/exact", test_exact);
  g_test_add_func ("/fileattributematcher/equality", test_equality);
  g_test_add_func ("/fileattributematcher/subtract", test_subtract);

  return g_test_run ();
}
