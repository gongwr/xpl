#include "config.h"

#include <glib.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

static xboolean_t missing_locale = FALSE;

typedef struct {
  const xchar_t **input;
  const xchar_t **sorted;
  const xchar_t **file_sorted;
} CollateTest;

typedef struct {
  xchar_t *key;
  const xchar_t *str;
} Line;

static void
clear_line (Line *line)
{
  g_free (line->key);
}

static int
compare_collate (const void *a, const void *b)
{
  const Line *line_a = a;
  const Line *line_b = b;

  return xutf8_collate (line_a->str, line_b->str);
}

static int
compare_key (const void *a, const void *b)
{
  const Line *line_a = a;
  const Line *line_b = b;

  return strcmp (line_a->key, line_b->key);
}

static void
do_collate (xboolean_t for_file, xboolean_t use_key, const CollateTest *test)
{
  xarray_t *line_array;
  Line line;
  xint_t i;

  if (missing_locale)
    {
      g_test_skip ("no en_US locale");
      return;
    }

  line_array = g_array_new (FALSE, FALSE, sizeof(Line));
  g_array_set_clear_func (line_array, (xdestroy_notify_t)clear_line);

  for (i = 0; test->input[i]; i++)
    {
      line.str = test->input[i];
      if (for_file)
        line.key = xutf8_collate_key_for_filename (line.str, -1);
      else
        line.key = xutf8_collate_key (line.str, -1);

      g_array_append_val (line_array, line);
    }

  qsort (line_array->data, line_array->len, sizeof (Line), use_key ? compare_key : compare_collate);

  for (i = 0; test->input[i]; i++)
    {
      const xchar_t *str;
      str = g_array_index (line_array, Line, i).str;
      if (for_file)
        g_assert_cmpstr (str, ==, test->file_sorted[i]);
      else
        g_assert_cmpstr (str, ==, test->sorted[i]);
    }

  g_array_free (line_array, TRUE);
}

static void
test_collate (xconstpointer d)
{
  const CollateTest *test = d;
  do_collate (FALSE, FALSE, test);
}

static void
test_collate_key (xconstpointer d)
{
  const CollateTest *test = d;
  do_collate (FALSE, TRUE, test);
}

static void
test_collate_file (xconstpointer d)
{
  const CollateTest *test = d;
  do_collate (TRUE, TRUE, test);
}

const xchar_t *input0[] = {
  "z",
  "c",
  "eer34",
  "223",
  "er1",
  "üĠണ",
  "foo",
  "bar",
  "baz",
  "GTK+",
  NULL
};

const xchar_t *sorted0[] = {
  "223",
  "bar",
  "baz",
  "c",
  "eer34",
  "er1",
  "foo",
  "GTK+",
  "üĠണ",
  "z",
  NULL
};

const xchar_t *file_sorted0[] = {
  "223",
  "bar",
  "baz",
  "c",
  "eer34",
  "er1",
  "foo",
  "GTK+",
  "üĠണ",
  "z",
  NULL
};

const xchar_t *input1[] = {
  "file.txt",
  "file2.bla",
  "file.c",
  "file3.xx",
  "bla001",
  "bla02",
  "bla03",
  "bla4",
  "bla10",
  "bla100",
  "event.c",
  "eventgenerator.c",
  "event.h",
  NULL
};

const xchar_t *sorted1[] = {
  "bla001",
  "bla02",
  "bla03",
  "bla10",
  "bla100",
  "bla4",
  "event.c",
  "eventgenerator.c",
  "event.h",
  "file2.bla",
  "file3.xx",
  "file.c",
  "file.txt",
  NULL
};

const xchar_t *file_sorted1[] = {
  "bla001",
  "bla02",
  "bla03",
  "bla4",
  "bla10",
  "bla100",
  "event.c",
  "event.h",
  "eventgenerator.c",
  "file.c",
  "file.txt",
  "file2.bla",
  "file3.xx",
  NULL
};

const xchar_t *input2[] = {
  "file26",
  "file100",
  "file1",
  "file:foo",
  "a.a",
  "file027",
  "file10",
  "aa.a",
  "file5",
  "file0027",
  "a-.a",
  "file0000",
  "file000x",
  NULL
};

const xchar_t *sorted2[] = {
  "a-.a",
  "a.a",
  "aa.a",
  "file0000",
  "file000x",
  "file0027",
  "file027",
  "file1",
  "file10",
  "file100",
  "file26",
  "file5",
  "file:foo",
  NULL
};

const xchar_t *file_sorted2[] = {
  /* Filename collation in OS X follows Finder style which gives
   * a slightly different order from usual Linux locales. */
#ifdef HAVE_CARBON
  "a-.a",
  "a.a",
  "aa.a",
  "file:foo",
  "file0000",
  "file000x",
  "file1",
  "file5",
  "file10",
  "file26",
  "file0027",
  "file027",
  "file100",
#else
  "a.a",
  "a-.a",
  "aa.a",
  "file0000",
  "file000x",
  "file1",
  "file5",
  "file10",
  "file26",
  "file027",
  "file0027",
  "file100",
  "file:foo",
#endif
  NULL
};

int
main (int argc, char *argv[])
{
  xchar_t *path;
  xuint_t i;
  const xchar_t *locale;
  CollateTest test[3];

  g_test_init (&argc, &argv, NULL);

  g_setenv ("LC_ALL", "en_US", TRUE);
  locale = setlocale (LC_ALL, "");
  if (locale == NULL || strcmp (locale, "en_US") != 0)
    {
      g_test_message ("No suitable locale, skipping tests");
      missing_locale = TRUE;
      /* let the tests run to completion so they show up as SKIP'd in TAP
       * output */
    }

  test[0].input = input0;
  test[0].sorted = sorted0;
  test[0].file_sorted = file_sorted0;
  test[1].input = input1;
  test[1].sorted = sorted1;
  test[1].file_sorted = file_sorted1;
  test[2].input = input2;
  test[2].sorted = sorted2;
  test[2].file_sorted = file_sorted2;

  for (i = 0; i < G_N_ELEMENTS (test); i++)
    {
      path = xstrdup_printf ("/unicode/collate/%d", i);
      g_test_add_data_func (path, &test[i], test_collate);
      g_free (path);
      path = xstrdup_printf ("/unicode/collate-key/%d", i);
      g_test_add_data_func (path, &test[i], test_collate_key);
      g_free (path);
      path = xstrdup_printf ("/unicode/collate-filename/%d", i);
      g_test_add_data_func (path, &test[i], test_collate_file);
      g_free (path);
    }

  return g_test_run ();
}
