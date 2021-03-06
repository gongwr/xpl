#include "config.h"

#include <locale.h>
#include <glib.h>

typedef struct
{
  const xchar_t *string;
  const xchar_t *prefix;
  const xchar_t *locale;
  xboolean_t should_match;
} SearchTest;

/* test_t word separators and case */
SearchTest basic[] = {
  { "Hello World", "he", "C", TRUE },
  { "Hello World", "wo", "C", TRUE },
  { "Hello World", "lo", "C", FALSE },
  { "Hello World", "ld", "C", FALSE },
  { "Hello-World", "wo", "C", TRUE },
  { "HelloWorld", "wo", "C", FALSE },
  { NULL, NULL, NULL, FALSE }
};

/* test_t composed chars (accentued letters) */
SearchTest composed[] = {
  { "Jörgen", "jor", "sv_SE.UTF-8", TRUE },
  { "Gaëtan", "gaetan", "fr_FR.UTF-8", TRUE },
  { "élève", "ele", "fr_FR.UTF-8", TRUE },
  { "Azais", "AzaÏs", "fr_FR.UTF-8", FALSE },
  { "AzaÏs", "Azais", "fr_FR.UTF-8", TRUE },
  { NULL, NULL, NULL, FALSE }
};

/* test_t decomposed chars, they looks the same, but are actually
 * composed of multiple unicodes */
SearchTest decomposed[] = {
  { "Jorgen", "Jör", "sv_SE.UTF-8", FALSE },
  { "Jörgen", "jor", "sv_SE.UTF-8", TRUE },
  { NULL, NULL, NULL, FALSE }
};

/* Turkish special case */
SearchTest turkish[] = {
  { "İstanbul", "ist", "tr_TR.UTF-8", TRUE },
  { "Diyarbakır", "diyarbakir", "tr_TR.UTF-8", TRUE },
  { NULL, NULL, NULL, FALSE }
};

/* test_t unicode chars when no locale is available */
SearchTest c_locale_unicode[] = {
  { "Jörgen", "jor", "C", TRUE },
  { "Jorgen", "Jör", "C", FALSE },
  { "Jörgen", "jor", "C", TRUE },
  { NULL, NULL, NULL, FALSE }
};

/* Multi words */
SearchTest multi_words[] = {
  { "Xavier Claessens", "Xav Cla", "C", TRUE },
  { "Xavier Claessens", "Cla Xav", "C", TRUE },
  { "foo_t Bar baz_t", "   b  ", "C", TRUE },
  { "foo_t Bar baz_t", "bar bazz", "C", FALSE },
  { NULL, NULL, NULL, FALSE }
};

static void
test_search (xconstpointer d)
{
  const SearchTest *tests = d;
  xuint_t i;
  xboolean_t all_skipped = TRUE;

  g_debug ("Started");

  for (i = 0; tests[i].string != NULL; i++)
    {
      xboolean_t match;
      xboolean_t ok = FALSE;
      xboolean_t skipped;

      if (setlocale (LC_ALL, tests[i].locale))
        {
          skipped = FALSE;
          all_skipped = FALSE;
          match = xstr_match_string (tests[i].prefix, tests[i].string, TRUE);
          ok = (match == tests[i].should_match);
        }
      else
        {
          skipped = TRUE;
          g_test_message ("Locale '%s' is unavailable", tests[i].locale);
        }

      g_debug ("'%s' - '%s' %s: %s", tests[i].prefix, tests[i].string,
          tests[i].should_match ? "should match" : "should NOT match",
          skipped ? "SKIPPED" : ok ? "OK" : "FAILED");

      xassert (skipped || ok);
    }

  if (all_skipped)
    g_test_skip ("No locales for the test set are available");
}

int
main (int argc,
      char **argv)
{
  xchar_t *user_locale;

  g_test_init (&argc, &argv, NULL);

  setlocale (LC_ALL, "");
  user_locale = setlocale (LC_ALL, NULL);
  g_debug ("Current user locale: %s", user_locale);

  g_test_add_data_func ("/search/basic", basic, test_search);
  g_test_add_data_func ("/search/composed", composed, test_search);
  g_test_add_data_func ("/search/decomposed", decomposed, test_search);
  g_test_add_data_func ("/search/turkish", turkish, test_search);
  g_test_add_data_func ("/search/c_locale_unicode", c_locale_unicode, test_search);
  g_test_add_data_func ("/search/multi_words", multi_words, test_search);

  return g_test_run ();
}
