#include <glib.h>
#include <stdlib.h>

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
 * glist sort tests
 */
static void
test_list_sort (void)
{
  xlist_t *list = NULL;
  xint_t   i;

  for (i = 0; i < SIZE; i++)
    list = xlist_append (list, GINT_TO_POINTER (array[i]));

  list = xlist_sort (list, sort);
  for (i = 0; i < SIZE - 1; i++)
    {
      xpointer_t p1, p2;

      p1 = xlist_nth_data (list, i);
      p2 = xlist_nth_data (list, i+1);

      xassert (GPOINTER_TO_INT (p1) <= GPOINTER_TO_INT (p2));
    }

  xlist_free (list);
}

static void
test_list_sort_with_data (void)
{
  xlist_t *list = NULL;
  xint_t   i;

  for (i = 0; i < SIZE; i++)
    list = xlist_append (list, GINT_TO_POINTER (array[i]));

  list = xlist_sort_with_data (list, (GCompareDataFunc)sort, NULL);
  for (i = 0; i < SIZE - 1; i++)
    {
      xpointer_t p1, p2;

      p1 = xlist_nth_data (list, i);
      p2 = xlist_nth_data (list, i+1);

      xassert (GPOINTER_TO_INT (p1) <= GPOINTER_TO_INT (p2));
    }

  xlist_free (list);
}

/* test_t that the sort is stable. */
static void
test_list_sort_stable (void)
{
  xlist_t *list = NULL;  /* (element-type utf8) */
  xlist_t *copy = NULL;  /* (element-type utf8) */
  xsize_t i;

  /* Build a test list, already ordered. */
  for (i = 0; i < SIZE; i++)
    list = xlist_append (list, xstrdup_printf ("%" G_GSIZE_FORMAT, i / 5));

  /* Take a copy and sort it. */
  copy = xlist_copy (list);
  copy = xlist_sort (copy, (GCompareFunc) xstrcmp0);

  /* Compare the two lists, checking pointers are equal to ensure the elements
   * have been kept stable. */
  for (i = 0; i < SIZE; i++)
    {
      xpointer_t p1, p2;

      p1 = xlist_nth_data (list, i);
      p2 = xlist_nth_data (list, i);

      xassert (p1 == p2);
    }

  xlist_free (copy);
  xlist_free_full (list, g_free);
}

static void
test_list_insert_sorted (void)
{
  xlist_t *list = NULL;
  xint_t   i;

  for (i = 0; i < SIZE; i++)
    list = xlist_insert_sorted (list, GINT_TO_POINTER (array[i]), sort);

  for (i = 0; i < SIZE - 1; i++)
    {
      xpointer_t p1, p2;

      p1 = xlist_nth_data (list, i);
      p2 = xlist_nth_data (list, i+1);

      xassert (GPOINTER_TO_INT (p1) <= GPOINTER_TO_INT (p2));
    }

  xlist_free (list);
}

static void
test_list_insert_sorted_with_data (void)
{
  xlist_t *list = NULL;
  xint_t   i;

  for (i = 0; i < SIZE; i++)
    list = xlist_insert_sorted_with_data (list,
                                           GINT_TO_POINTER (array[i]),
                                           (GCompareDataFunc)sort,
                                           NULL);

  for (i = 0; i < SIZE - 1; i++)
    {
      xpointer_t p1, p2;

      p1 = xlist_nth_data (list, i);
      p2 = xlist_nth_data (list, i+1);

      xassert (GPOINTER_TO_INT (p1) <= GPOINTER_TO_INT (p2));
    }

  xlist_free (list);
}

static void
test_list_reverse (void)
{
  xlist_t *list = NULL;
  xlist_t *st;
  xint_t   nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t   i;

  for (i = 0; i < 10; i++)
    list = xlist_append (list, &nums[i]);

  list = xlist_reverse (list);

  for (i = 0; i < 10; i++)
    {
      st = xlist_nth (list, i);
      xassert (*((xint_t*) st->data) == (9 - i));
    }

  xlist_free (list);
}

static void
test_list_nth (void)
{
  xlist_t *list = NULL;
  xlist_t *st;
  xint_t   nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t   i;

  for (i = 0; i < 10; i++)
    list = xlist_append (list, &nums[i]);

  for (i = 0; i < 10; i++)
    {
      st = xlist_nth (list, i);
      xassert (*((xint_t*) st->data) == i);
    }

  xlist_free (list);
}

static void
test_list_concat (void)
{
  xlist_t *list1 = NULL;
  xlist_t *list2 = NULL;
  xlist_t *st;
  xint_t   nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t i;

  for (i = 0; i < 5; i++)
    {
      list1 = xlist_append (list1, &nums[i]);
      list2 = xlist_append (list2, &nums[i+5]);
    }

  g_assert_cmpint (xlist_length (list1), ==, 5);
  g_assert_cmpint (xlist_length (list2), ==, 5);

  list1 = xlist_concat (list1, list2);

  g_assert_cmpint (xlist_length (list1), ==, 10);

  for (i = 0; i < 10; i++)
    {
      st = xlist_nth (list1, i);
      xassert (*((xint_t*) st->data) == i);
    }

  list2 = xlist_concat (NULL, list1);
  g_assert_cmpint (xlist_length (list2), ==, 10);

  list2 = xlist_concat (list1, NULL);
  g_assert_cmpint (xlist_length (list2), ==, 10);

  list2 = xlist_concat (NULL, NULL);
  xassert (list2 == NULL);

  xlist_free (list1);
}

static void
test_list_remove (void)
{
  xlist_t *list = NULL;
  xlist_t *st;
  xint_t   nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t   i;

  for (i = 0; i < 10; i++)
    {
      list = xlist_append (list, &nums[i]);
      list = xlist_append (list, &nums[i]);
    }

  g_assert_cmpint (xlist_length (list), ==, 20);

  for (i = 0; i < 10; i++)
    {
      list = xlist_remove (list, &nums[i]);
    }

  g_assert_cmpint (xlist_length (list), ==, 10);

  for (i = 0; i < 10; i++)
    {
      st = xlist_nth (list, i);
      xassert (*((xint_t*) st->data) == i);
    }

  xlist_free (list);
}

static void
test_list_remove_all (void)
{
  xlist_t *list = NULL;
  xint_t   nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t   i;

  for (i = 0; i < 10; i++)
    {
      list = xlist_append (list, &nums[i]);
      list = xlist_append (list, &nums[i]);
    }

  g_assert_cmpint (xlist_length (list), ==, 20);

  for (i = 0; i < 5; i++)
    {
      list = xlist_remove_all (list, &nums[2 * i + 1]);
      list = xlist_remove_all (list, &nums[8 - 2 * i]);
    }

  g_assert_cmpint (xlist_length (list), ==, 0);
  xassert (list == NULL);
}

static void
test_list_first_last (void)
{
  xlist_t *list = NULL;
  xlist_t *st;
  xint_t   nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t   i;

  for (i = 0; i < 10; i++)
    list = xlist_append (list, &nums[i]);

  st = xlist_last (list);
  xassert (*((xint_t*) st->data) == 9);
  st = xlist_nth_prev (st, 3);
  xassert (*((xint_t*) st->data) == 6);
  st = xlist_first (st);
  xassert (*((xint_t*) st->data) == 0);

  xlist_free (list);
}

static void
test_list_insert (void)
{
  xlist_t *list = NULL;
  xlist_t *st;
  xint_t   nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  xint_t   i;

  list = xlist_insert_before (NULL, NULL, &nums[1]);
  list = xlist_insert (list, &nums[3], 1);
  list = xlist_insert (list, &nums[4], -1);
  list = xlist_insert (list, &nums[0], 0);
  list = xlist_insert (list, &nums[5], 100);
  list = xlist_insert_before (list, NULL, &nums[6]);
  list = xlist_insert_before (list, list->next->next, &nums[2]);

  list = xlist_insert (list, &nums[9], 7);
  list = xlist_insert (list, &nums[8], 7);
  list = xlist_insert (list, &nums[7], 7);

  for (i = 0; i < 10; i++)
    {
      st = xlist_nth (list, i);
      xassert (*((xint_t*) st->data) == i);
    }

  xlist_free (list);
}

typedef struct
{
  xboolean_t freed;
  int x;
} ListItem;

static void
free_func (xpointer_t data)
{
  ListItem *item = data;

  item->freed = TRUE;
}

static ListItem *
new_item (int x)
{
  ListItem *item;

  item = g_slice_new (ListItem);
  item->freed = FALSE;
  item->x = x;

  return item;
}

static void
test_free_full (void)
{
  ListItem *one, *two, *three;
  xslist_t *slist = NULL;
  xlist_t *list = NULL;

  slist = xslist_prepend (slist, one = new_item (1));
  slist = xslist_prepend (slist, two = new_item (2));
  slist = xslist_prepend (slist, three = new_item (3));
  xassert (!one->freed);
  xassert (!two->freed);
  xassert (!three->freed);
  xslist_free_full (slist, free_func);
  xassert (one->freed);
  xassert (two->freed);
  xassert (three->freed);
  g_slice_free (ListItem, one);
  g_slice_free (ListItem, two);
  g_slice_free (ListItem, three);

  list = xlist_prepend (list, one = new_item (1));
  list = xlist_prepend (list, two = new_item (2));
  list = xlist_prepend (list, three = new_item (3));
  xassert (!one->freed);
  xassert (!two->freed);
  xassert (!three->freed);
  xlist_free_full (list, free_func);
  xassert (one->freed);
  xassert (two->freed);
  xassert (three->freed);
  g_slice_free (ListItem, one);
  g_slice_free (ListItem, two);
  g_slice_free (ListItem, three);
}

static void
test_list_copy (void)
{
  xlist_t *l, *l2;
  xlist_t *u, *v;

  l = NULL;
  l = xlist_append (l, GINT_TO_POINTER (1));
  l = xlist_append (l, GINT_TO_POINTER (2));
  l = xlist_append (l, GINT_TO_POINTER (3));

  l2 = xlist_copy (l);

  for (u = l, v = l2; u && v; u = u->next, v = v->next)
    {
      xassert (u->data == v->data);
    }

  xlist_free (l);
  xlist_free (l2);
}

static xpointer_t
multiply_value (xconstpointer value, xpointer_t data)
{
  return GINT_TO_POINTER (GPOINTER_TO_INT (value) * GPOINTER_TO_INT (data));
}

static void
test_list_copy_deep (void)
{
  xlist_t *l, *l2;
  xlist_t *u, *v;

  l = NULL;
  l = xlist_append (l, GINT_TO_POINTER (1));
  l = xlist_append (l, GINT_TO_POINTER (2));
  l = xlist_append (l, GINT_TO_POINTER (3));

  l2 = xlist_copy_deep (l, multiply_value, GINT_TO_POINTER (2));

  for (u = l, v = l2; u && v; u = u->next, v = v->next)
    {
      g_assert_cmpint (GPOINTER_TO_INT (u->data) * 2, ==, GPOINTER_TO_INT (v->data));
    }

  xlist_free (l);
  xlist_free (l2);
}

static void
test_delete_link (void)
{
  xlist_t *l, *l2;

  l = NULL;
  l = xlist_append (l, GINT_TO_POINTER (1));
  l = xlist_append (l, GINT_TO_POINTER (2));
  l = xlist_append (l, GINT_TO_POINTER (3));

  l2 = l->next;

  l = xlist_delete_link (l, l2);
  xassert (l->data == GINT_TO_POINTER (1));
  xassert (l->next->data == GINT_TO_POINTER (3));

  xlist_free (l);
}

static void
test_prepend (void)
{
  xpointer_t a = "a";
  xpointer_t b = "b";
  xpointer_t c = "c";
  xlist_t *l, *l2;

  l = NULL;
  l = xlist_prepend (l, c);
  l = xlist_prepend (l, a);

  xassert (l->data == a);
  xassert (l->next->data == c);
  xassert (l->next->next == NULL);

  l2 = l->next;
  l2 = xlist_prepend (l2, b);
  xassert (l2->prev == l);

  xassert (l->data == a);
  xassert (l->next->data == b);
  xassert (l->next->next->data == c);
  xassert (l->next->next->next == NULL);

  xlist_free (l);
}

static void
test_position (void)
{
  xlist_t *l, *ll;
  char *a = "a";
  char *b = "b";
  char *c = "c";
  char *d = "d";

  l = NULL;
  l = xlist_append (l, a);
  l = xlist_append (l, b);
  l = xlist_append (l, c);

  ll = xlist_find (l, a);
  g_assert_cmpint (xlist_position (l, ll), ==, 0);
  g_assert_cmpint (xlist_index (l, a), ==, 0);
  ll = xlist_find (l, b);
  g_assert_cmpint (xlist_position (l, ll), ==, 1);
  g_assert_cmpint (xlist_index (l, b), ==, 1);
  ll = xlist_find (l, c);
  g_assert_cmpint (xlist_position (l, ll), ==, 2);
  g_assert_cmpint (xlist_index (l, c), ==, 2);

  ll = xlist_append (NULL, d);
  g_assert_cmpint (xlist_position (l, ll), ==, -1);
  g_assert_cmpint (xlist_index (l, d), ==, -1);

  xlist_free (l);
  xlist_free (ll);
}

static void
test_double_free (void)
{
  xlist_t *list, *link;
  // Casts to size_t first ensure compilers won't warn about pointer casts that change size
  // MSVC's C4312 warning with /Wp64
  xlist_t  intruder = { NULL, (xpointer_t)(size_t)0xDEADBEEF, (xpointer_t)(size_t)0xDEADBEEF };

  if (g_test_subprocess ())
    {
      list = NULL;
      list = xlist_append (list, "a");
      link = list = xlist_append (list, "b");
      list = xlist_append (list, "c");

      list = xlist_remove_link (list, link);
      link->prev = list;
      link->next = &intruder;
      list = xlist_remove_link (list, link);

      xlist_free (list);
      return;
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*corrupted double-linked list detected*");
}

static void
test_list_insert_before_link (void)
{
  xlist_t a = {0};
  xlist_t b = {0};
  xlist_t c = {0};
  xlist_t d = {0};
  xlist_t e = {0};
  xlist_t *list;

  list = xlist_insert_before_link (NULL, NULL, &a);
  g_assert_nonnull (list);
  g_assert_true (list == &a);
  g_assert_null (a.prev);
  g_assert_null (a.next);
  g_assert_cmpint (xlist_length (list), ==, 1);

  list = xlist_insert_before_link (list, &a, &b);
  g_assert_nonnull (list);
  g_assert_true (list == &b);
  g_assert_null (b.prev);
  g_assert_true (b.next == &a);
  g_assert_true (a.prev == &b);
  g_assert_null (a.next);
  g_assert_cmpint (xlist_length (list), ==, 2);

  list = xlist_insert_before_link (list, &a, &c);
  g_assert_nonnull (list);
  g_assert_true (list == &b);
  g_assert_null (b.prev);
  g_assert_true (b.next == &c);
  g_assert_true (c.next == &a);
  g_assert_true (c.prev == &b);
  g_assert_true (a.prev == &c);
  g_assert_null (a.next);
  g_assert_cmpint (xlist_length (list), ==, 3);

  list = xlist_insert_before_link (list, &b, &d);
  g_assert_nonnull (list);
  g_assert_true (list == &d);
  g_assert_null (d.prev);
  g_assert_true (b.prev == &d);
  g_assert_true (c.prev == &b);
  g_assert_true (a.prev == &c);
  g_assert_true (d.next == &b);
  g_assert_true (b.next == &c);
  g_assert_true (c.next == &a);
  g_assert_null (a.next);
  g_assert_cmpint (xlist_length (list), ==, 4);

  list = xlist_insert_before_link (list, NULL, &e);
  g_assert_nonnull (list);
  g_assert_true (list == &d);
  g_assert_null (d.prev);
  g_assert_true (b.prev == &d);
  g_assert_true (c.prev == &b);
  g_assert_true (a.prev == &c);
  g_assert_true (d.next == &b);
  g_assert_true (b.next == &c);
  g_assert_true (c.next == &a);
  g_assert_true (a.next == &e);
  g_assert_true (e.prev == &a);
  g_assert_null (e.next);
  g_assert_cmpint (xlist_length (list), ==, 5);
}

int
main (int argc, char *argv[])
{
  xint_t i;

  g_test_init (&argc, &argv, NULL);

  /* Create an array of random numbers. */
  for (i = 0; i < SIZE; i++)
    array[i] = g_test_rand_int_range (NUMBER_MIN, NUMBER_MAX);

  g_test_add_func ("/list/sort", test_list_sort);
  g_test_add_func ("/list/sort-with-data", test_list_sort_with_data);
  g_test_add_func ("/list/sort/stable", test_list_sort_stable);
  g_test_add_func ("/list/insert-before-link", test_list_insert_before_link);
  g_test_add_func ("/list/insert-sorted", test_list_insert_sorted);
  g_test_add_func ("/list/insert-sorted-with-data", test_list_insert_sorted_with_data);
  g_test_add_func ("/list/reverse", test_list_reverse);
  g_test_add_func ("/list/nth", test_list_nth);
  g_test_add_func ("/list/concat", test_list_concat);
  g_test_add_func ("/list/remove", test_list_remove);
  g_test_add_func ("/list/remove-all", test_list_remove_all);
  g_test_add_func ("/list/first-last", test_list_first_last);
  g_test_add_func ("/list/insert", test_list_insert);
  g_test_add_func ("/list/free-full", test_free_full);
  g_test_add_func ("/list/copy", test_list_copy);
  g_test_add_func ("/list/copy-deep", test_list_copy_deep);
  g_test_add_func ("/list/delete-link", test_delete_link);
  g_test_add_func ("/list/prepend", test_prepend);
  g_test_add_func ("/list/position", test_position);
  g_test_add_func ("/list/double-free", test_double_free);

  return g_test_run ();
}
