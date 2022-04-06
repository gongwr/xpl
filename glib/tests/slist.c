#include <glib.h>

#define SIZE       50
#define NUMBER_MIN 0000
#define NUMBER_MAX 9999


static xuint32_t array[SIZE];


static xint_t
sort (xconstpointer p1, xconstpointer p2)
{
  gint32 a, b;

  a = GPOINTER_TO_INT (p1);
  b = GPOINTER_TO_INT (p2);

  return (a > b ? +1 : a == b ? 0 : -1);
}

/*
 * gslist sort tests
 */
static void
test_slist_sort (void)
{
  xslist_t *slist = NULL;
  xint_t    i;

  for (i = 0; i < SIZE; i++)
    slist = xslist_append (slist, GINT_TO_POINTER (array[i]));

  slist = xslist_sort (slist, sort);
  for (i = 0; i < SIZE - 1; i++)
    {
      xpointer_t p1, p2;

      p1 = xslist_nth_data (slist, i);
      p2 = xslist_nth_data (slist, i+1);

      g_assert (GPOINTER_TO_INT (p1) <= GPOINTER_TO_INT (p2));
    }

  xslist_free (slist);
}

static void
test_slist_sort_with_data (void)
{
  xslist_t *slist = NULL;
  xint_t    i;

  for (i = 0; i < SIZE; i++)
    slist = xslist_append (slist, GINT_TO_POINTER (array[i]));

  slist = xslist_sort_with_data (slist, (GCompareDataFunc)sort, NULL);
  for (i = 0; i < SIZE - 1; i++)
    {
      xpointer_t p1, p2;

      p1 = xslist_nth_data (slist, i);
      p2 = xslist_nth_data (slist, i+1);

      g_assert (GPOINTER_TO_INT (p1) <= GPOINTER_TO_INT (p2));
    }

  xslist_free (slist);
}

/* Test that the sort is stable. */
static void
test_slist_sort_stable (void)
{
  xslist_t *list = NULL;  /* (element-type utf8) */
  xslist_t *copy = NULL;  /* (element-type utf8) */
  xsize_t i;

  /* Build a test list, already ordered. */
  for (i = 0; i < SIZE; i++)
    list = xslist_append (list, xstrdup_printf ("%" G_GSIZE_FORMAT, i / 5));

  /* Take a copy and sort it. */
  copy = xslist_copy (list);
  copy = xslist_sort (copy, (GCompareFunc) xstrcmp0);

  /* Compare the two lists, checking pointers are equal to ensure the elements
   * have been kept stable. */
  for (i = 0; i < SIZE; i++)
    {
      xpointer_t p1, p2;

      p1 = xslist_nth_data (list, i);
      p2 = xslist_nth_data (list, i);

      g_assert (p1 == p2);
    }

  xslist_free (copy);
  xslist_free_full (list, g_free);
}

static void
test_slist_insert_sorted (void)
{
  xslist_t *slist = NULL;
  xint_t    i;

  for (i = 0; i < SIZE; i++)
    slist = xslist_insert_sorted (slist, GINT_TO_POINTER (array[i]), sort);

  for (i = 0; i < SIZE - 1; i++)
    {
      xpointer_t p1, p2;

      p1 = xslist_nth_data (slist, i);
      p2 = xslist_nth_data (slist, i+1);

      g_assert (GPOINTER_TO_INT (p1) <= GPOINTER_TO_INT (p2));
    }

  xslist_free (slist);
}

static void
test_slist_insert_sorted_with_data (void)
{
  xslist_t *slist = NULL;
  xint_t    i;

  for (i = 0; i < SIZE; i++)
    slist = xslist_insert_sorted_with_data (slist,
                                           GINT_TO_POINTER (array[i]),
                                           (GCompareDataFunc)sort,
                                           NULL);

  for (i = 0; i < SIZE - 1; i++)
    {
      xpointer_t p1, p2;

      p1 = xslist_nth_data (slist, i);
      p2 = xslist_nth_data (slist, i+1);

      g_assert (GPOINTER_TO_INT (p1) <= GPOINTER_TO_INT (p2));
    }

  xslist_free (slist);
}

static void
test_slist_reverse (void)
{
  xslist_t *slist = NULL;
  xslist_t *st;
  xint_t    nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t    i;

  for (i = 0; i < 10; i++)
    slist = xslist_append (slist, &nums[i]);

  slist = xslist_reverse (slist);

  for (i = 0; i < 10; i++)
    {
      st = xslist_nth (slist, i);
      g_assert (*((xint_t*) st->data) == (9 - i));
    }

  xslist_free (slist);
}

static void
test_slist_nth (void)
{
  xslist_t *slist = NULL;
  xslist_t *st;
  xint_t    nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t    i;

  for (i = 0; i < 10; i++)
    slist = xslist_append (slist, &nums[i]);

  for (i = 0; i < 10; i++)
    {
      st = xslist_nth (slist, i);
      g_assert (*((xint_t*) st->data) == i);
    }

  xslist_free (slist);
}

static void
test_slist_remove (void)
{
  xslist_t *slist = NULL;
  xslist_t *st;
  xint_t    nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t    i;

  for (i = 0; i < 10; i++)
    {
      slist = xslist_append (slist, &nums[i]);
      slist = xslist_append (slist, &nums[i]);
    }

  g_assert_cmpint (xslist_length (slist), ==, 20);

  for (i = 0; i < 10; i++)
    {
      slist = xslist_remove (slist, &nums[i]);
    }

  g_assert_cmpint (xslist_length (slist), ==, 10);

  for (i = 0; i < 10; i++)
    {
      st = xslist_nth (slist, i);
      g_assert (*((xint_t*) st->data) == i);
    }

  xslist_free (slist);
}

static void
test_slist_remove_all (void)
{
  xslist_t *slist = NULL;
  xint_t    nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t    i;

  for (i = 0; i < 10; i++)
    {
      slist = xslist_append (slist, &nums[i]);
      slist = xslist_append (slist, &nums[i]);
    }

  g_assert_cmpint (xslist_length (slist), ==, 20);

  for (i = 0; i < 5; i++)
    {
      slist = xslist_remove_all (slist, &nums[2 * i + 1]);
      slist = xslist_remove_all (slist, &nums[8 - 2 * i]);
    }

  g_assert_cmpint (xslist_length (slist), ==, 0);
  g_assert (slist == NULL);
}

static void
test_slist_insert (void)
{
  xpointer_t a = "a";
  xpointer_t b = "b";
  xpointer_t c = "c";
  xslist_t *slist = NULL;
  xslist_t *st;
  xint_t   nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t   i;

  slist = xslist_insert_before (NULL, NULL, &nums[1]);
  slist = xslist_insert (slist, &nums[3], 1);
  slist = xslist_insert (slist, &nums[4], -1);
  slist = xslist_insert (slist, &nums[0], 0);
  slist = xslist_insert (slist, &nums[5], 100);
  slist = xslist_insert_before (slist, NULL, &nums[6]);
  slist = xslist_insert_before (slist, slist->next->next, &nums[2]);

  slist = xslist_insert (slist, &nums[9], 7);
  slist = xslist_insert (slist, &nums[8], 7);
  slist = xslist_insert (slist, &nums[7], 7);

  for (i = 0; i < 10; i++)
    {
      st = xslist_nth (slist, i);
      g_assert (*((xint_t*) st->data) == i);
    }

  xslist_free (slist);

  slist = xslist_insert (NULL, a, 1);
  g_assert (slist->data == a);
  g_assert (slist->next == NULL);
  xslist_free (slist);

  slist = xslist_append (NULL, a);
  slist = xslist_append (slist, b);
  slist = xslist_insert (slist, c, 5);

  g_assert (slist->next->next->data == c);
  g_assert (slist->next->next->next == NULL);
  xslist_free (slist);

  slist = xslist_append (NULL, a);
  slist = xslist_insert_before (slist, slist, b);
  g_assert (slist->data == b);
  g_assert (slist->next->data == a);
  g_assert (slist->next->next == NULL);
  xslist_free (slist);
}

static xint_t
find_num (xconstpointer l, xconstpointer data)
{
  return *(xint_t*)l - GPOINTER_TO_INT(data);
}

static void
test_slist_position (void)
{
  xslist_t *slist = NULL;
  xslist_t *st;
  xint_t    nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t    i;

  for (i = 0; i < 10; i++)
    {
      slist = xslist_append (slist, &nums[i]);
    }

  g_assert_cmpint (xslist_index (slist, NULL), ==, -1);
  g_assert_cmpint (xslist_position (slist, NULL), ==, -1);

  for (i = 0; i < 10; i++)
    {
      g_assert_cmpint (xslist_index (slist, &nums[i]), ==, i);
      st = xslist_find_custom (slist, GINT_TO_POINTER(i), find_num);
      g_assert (st != NULL);
      g_assert_cmpint (xslist_position (slist, st), ==, i);
    }

  st = xslist_find_custom (slist, GINT_TO_POINTER (1000), find_num);
  g_assert (st == NULL);

  xslist_free (slist);
}

static void
test_slist_concat (void)
{
  xpointer_t a = "a";
  xpointer_t b = "b";
  xslist_t *s1, *s2, *s;

  s1 = xslist_append (NULL, a);
  s2 = xslist_append (NULL, b);
  s = xslist_concat (s1, s2);
  g_assert (s->data == a);
  g_assert (s->next->data == b);
  g_assert (s->next->next == NULL);
  xslist_free (s);

  s1 = xslist_append (NULL, a);

  s = xslist_concat (NULL, s1);
  g_assert_cmpint (xslist_length (s), ==, 1);
  s = xslist_concat (s1, NULL);
  g_assert_cmpint (xslist_length (s), ==, 1);

  xslist_free (s);

  s = xslist_concat (NULL, NULL);
  g_assert (s == NULL);
}

static void
test_slist_copy (void)
{
  xslist_t *slist = NULL, *copy;
  xslist_t *s1, *s2;
  xuint_t nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xsize_t i;

  /* Copy and test a many-element list. */
  for (i = 0; i < 10; i++)
    slist = xslist_append (slist, &nums[i]);

  copy = xslist_copy (slist);

  g_assert_cmpuint (xslist_length (copy), ==, xslist_length (slist));

  for (s1 = copy, s2 = slist; s1 != NULL && s2 != NULL; s1 = s1->next, s2 = s2->next)
    g_assert (s1->data == s2->data);

  xslist_free (copy);
  xslist_free (slist);

  /* Copy a NULL list. */
  copy = xslist_copy (NULL);
  g_assert_null (copy);
}

static xpointer_t
copy_and_count_string (xconstpointer src,
                       xpointer_t      data)
{
  const xchar_t *str = src;
  xsize_t *count = data;

  *count = *count + 1;
  return xstrdup (str);
}

static void
test_slist_copy_deep (void)
{
  xslist_t *slist = NULL, *copy;
  xslist_t *s1, *s2;
  xsize_t count;

  /* Deep-copy a simple list. */
  slist = xslist_append (slist, "a");
  slist = xslist_append (slist, "b");
  slist = xslist_append (slist, "c");

  count = 0;
  copy = xslist_copy_deep (slist, copy_and_count_string, &count);

  g_assert_cmpuint (count, ==, xslist_length (slist));
  g_assert_cmpuint (xslist_length (copy), ==, count);
  for (s1 = slist, s2 = copy; s1 != NULL && s2 != NULL; s1 = s1->next, s2 = s2->next)
    {
      g_assert_cmpstr (s1->data, ==, s2->data);
      g_assert (s1->data != s2->data);
    }

  xslist_free_full (copy, g_free);
  xslist_free (slist);

  /* Try with an empty list. */
  count = 0;
  copy = xslist_copy_deep (NULL, copy_and_count_string, &count);
  g_assert_cmpuint (count, ==, 0);
  g_assert_null (copy);
}

int
main (int argc, char *argv[])
{
  xint_t i;

  g_test_init (&argc, &argv, NULL);

  /* Create an array of random numbers. */
  for (i = 0; i < SIZE; i++)
    array[i] = g_test_rand_int_range (NUMBER_MIN, NUMBER_MAX);

  g_test_add_func ("/slist/sort", test_slist_sort);
  g_test_add_func ("/slist/sort-with-data", test_slist_sort_with_data);
  g_test_add_func ("/slist/sort/stable", test_slist_sort_stable);
  g_test_add_func ("/slist/insert-sorted", test_slist_insert_sorted);
  g_test_add_func ("/slist/insert-sorted-with-data", test_slist_insert_sorted_with_data);
  g_test_add_func ("/slist/reverse", test_slist_reverse);
  g_test_add_func ("/slist/nth", test_slist_nth);
  g_test_add_func ("/slist/remove", test_slist_remove);
  g_test_add_func ("/slist/remove-all", test_slist_remove_all);
  g_test_add_func ("/slist/insert", test_slist_insert);
  g_test_add_func ("/slist/position", test_slist_position);
  g_test_add_func ("/slist/concat", test_slist_concat);
  g_test_add_func ("/slist/copy", test_slist_copy);
  g_test_add_func ("/slist/copy/deep", test_slist_copy_deep);

  return g_test_run ();
}
