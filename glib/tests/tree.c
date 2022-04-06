/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

/* We are testing some deprecated APIs here */
#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include "glib.h"


static xint_t
my_compare (xconstpointer a,
            xconstpointer b)
{
  const char *cha = a;
  const char *chb = b;

  return *cha - *chb;
}

static xint_t
my_compare_with_data (xconstpointer a,
                      xconstpointer b,
                      xpointer_t      user_data)
{
  const char *cha = a;
  const char *chb = b;

  /* just check that we got the right data */
  g_assert (GPOINTER_TO_INT(user_data) == 123);

  return *cha - *chb;
}

static xint_t
my_search (xconstpointer a,
           xconstpointer b)
{
  return my_compare (b, a);
}

static xpointer_t destroyed_key = NULL;
static xpointer_t destroyed_value = NULL;
static xuint_t destroyed_key_count = 0;
static xuint_t destroyed_value_count = 0;

static void
my_key_destroy (xpointer_t key)
{
  destroyed_key = key;
  destroyed_key_count++;
}

static void
my_value_destroy (xpointer_t value)
{
  destroyed_value = value;
  destroyed_value_count++;
}

static xint_t
my_traverse (xpointer_t key,
             xpointer_t value,
             xpointer_t data)
{
  char *ch = key;

  g_assert ((*ch) > 0);

  if (*ch == 'd')
    return TRUE;

  return FALSE;
}

char chars[] =
  "0123456789"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz";

char chars2[] =
  "0123456789"
  "abcdefghijklmnopqrstuvwxyz";

static xint_t
check_order (xpointer_t key,
             xpointer_t value,
             xpointer_t data)
{
  char **p = data;
  char *ch = key;

  g_assert (**p == *ch);

  (*p)++;

  return FALSE;
}

static void
test_tree_search (void)
{
  xint_t i;
  xtree_t *tree;
  xboolean_t removed;
  xchar_t c;
  xchar_t *p, *d;

  tree = xtree_new_with_data (my_compare_with_data, GINT_TO_POINTER(123));

  for (i = 0; chars[i]; i++)
    xtree_insert (tree, &chars[i], &chars[i]);

  xtree_foreach (tree, my_traverse, NULL);

  g_assert_cmpint (xtree_nnodes (tree), ==, strlen (chars));
  g_assert_cmpint (xtree_height (tree), ==, 6);

  p = chars;
  xtree_foreach (tree, check_order, &p);

  for (i = 0; i < 26; i++)
    {
      removed = xtree_remove (tree, &chars[i + 10]);
      g_assert (removed);
    }

  c = '\0';
  removed = xtree_remove (tree, &c);
  g_assert (!removed);

  xtree_foreach (tree, my_traverse, NULL);

  g_assert_cmpint (xtree_nnodes (tree), ==, strlen (chars2));
  g_assert_cmpint (xtree_height (tree), ==, 6);

  p = chars2;
  xtree_foreach (tree, check_order, &p);

  for (i = 25; i >= 0; i--)
    xtree_insert (tree, &chars[i + 10], &chars[i + 10]);

  p = chars;
  xtree_foreach (tree, check_order, &p);

  c = '0';
  p = xtree_lookup (tree, &c);
  g_assert (p && *p == c);
  g_assert (xtree_lookup_extended (tree, &c, (xpointer_t *)&d, (xpointer_t *)&p));
  g_assert (c == *d && c == *p);

  c = 'A';
  p = xtree_lookup (tree, &c);
  g_assert (p && *p == c);

  c = 'a';
  p = xtree_lookup (tree, &c);
  g_assert (p && *p == c);

  c = 'z';
  p = xtree_lookup (tree, &c);
  g_assert (p && *p == c);

  c = '!';
  p = xtree_lookup (tree, &c);
  g_assert (p == NULL);

  c = '=';
  p = xtree_lookup (tree, &c);
  g_assert (p == NULL);

  c = '|';
  p = xtree_lookup (tree, &c);
  g_assert (p == NULL);

  c = '0';
  p = xtree_search (tree, my_search, &c);
  g_assert (p && *p == c);

  c = 'A';
  p = xtree_search (tree, my_search, &c);
  g_assert (p && *p == c);

  c = 'a';
  p = xtree_search (tree, my_search, &c);
  g_assert (p &&*p == c);

  c = 'z';
  p = xtree_search (tree, my_search, &c);
  g_assert (p && *p == c);

  c = '!';
  p = xtree_search (tree, my_search, &c);
  g_assert (p == NULL);

  c = '=';
  p = xtree_search (tree, my_search, &c);
  g_assert (p == NULL);

  c = '|';
  p = xtree_search (tree, my_search, &c);
  g_assert (p == NULL);

  xtree_destroy (tree);
}

static void
test_tree_remove (void)
{
  xtree_t *tree;
  char c, d;
  xint_t i;
  xboolean_t removed;
  xchar_t *remove;

  tree = xtree_new_full ((GCompareDataFunc)my_compare, NULL,
                          my_key_destroy,
                          my_value_destroy);

  for (i = 0; chars[i]; i++)
    xtree_insert (tree, &chars[i], &chars[i]);

  c = '0';
  xtree_insert (tree, &c, &c);
  g_assert (destroyed_key == &c);
  g_assert (destroyed_value == &chars[0]);
  destroyed_key = NULL;
  destroyed_value = NULL;

  d = '1';
  xtree_replace (tree, &d, &d);
  g_assert (destroyed_key == &chars[1]);
  g_assert (destroyed_value == &chars[1]);
  destroyed_key = NULL;
  destroyed_value = NULL;

  c = '2';
  removed = xtree_remove (tree, &c);
  g_assert (removed);
  g_assert (destroyed_key == &chars[2]);
  g_assert (destroyed_value == &chars[2]);
  destroyed_key = NULL;
  destroyed_value = NULL;

  c = '3';
  removed = xtree_steal (tree, &c);
  g_assert (removed);
  g_assert (destroyed_key == NULL);
  g_assert (destroyed_value == NULL);

  remove = "omkjigfedba";
  for (i = 0; remove[i]; i++)
    {
      removed = xtree_remove (tree, &remove[i]);
      g_assert (removed);
    }

  xtree_destroy (tree);
}

static void
test_tree_remove_all (void)
{
  xtree_t *tree;
  xsize_t i;

  tree = xtree_new_full ((GCompareDataFunc)my_compare, NULL,
                          my_key_destroy,
                          my_value_destroy);

  for (i = 0; chars[i]; i++)
    xtree_insert (tree, &chars[i], &chars[i]);

  destroyed_key_count = 0;
  destroyed_value_count = 0;

  xtree_remove_all (tree);

  g_assert_cmpuint (destroyed_key_count, ==, strlen (chars));
  g_assert_cmpuint (destroyed_value_count, ==, strlen (chars));
  g_assert_cmpint (xtree_height (tree), ==, 0);
  g_assert_cmpint (xtree_nnodes (tree), ==, 0);

  xtree_unref (tree);
}

static void
test_tree_destroy (void)
{
  xtree_t *tree;
  xint_t i;

  tree = xtree_new (my_compare);

  for (i = 0; chars[i]; i++)
    xtree_insert (tree, &chars[i], &chars[i]);

  g_assert_cmpint (xtree_nnodes (tree), ==, strlen (chars));

  xtree_ref (tree);
  xtree_destroy (tree);

  g_assert_cmpint (xtree_nnodes (tree), ==, 0);

  xtree_unref (tree);
}

typedef struct {
  xstring_t *s;
  xint_t count;
} CallbackData;

static xboolean_t
traverse_func (xpointer_t key, xpointer_t value, xpointer_t data)
{
  CallbackData *d = data;

  xchar_t *c = value;
  xstring_append_c (d->s, *c);

  d->count--;

  if (d->count == 0)
    return TRUE;

  return FALSE;
}

typedef struct {
  GTraverseType  traverse;
  xint_t           limit;
  const xchar_t   *expected;
} TraverseData;

static void
test_tree_traverse (void)
{
  xtree_t *tree;
  xsize_t i;
  TraverseData orders[] = {
    { G_IN_ORDER,   -1, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" },
    { G_IN_ORDER,    1, "0" },
    { G_IN_ORDER,    2, "01" },
    { G_IN_ORDER,    3, "012" },
    { G_IN_ORDER,    4, "0123" },
    { G_IN_ORDER,    5, "01234" },
    { G_IN_ORDER,    6, "012345" },
    { G_IN_ORDER,    7, "0123456" },
    { G_IN_ORDER,    8, "01234567" },
    { G_IN_ORDER,    9, "012345678" },
    { G_IN_ORDER,   10, "0123456789" },
    { G_IN_ORDER,   11, "0123456789A" },
    { G_IN_ORDER,   12, "0123456789AB" },
    { G_IN_ORDER,   13, "0123456789ABC" },
    { G_IN_ORDER,   14, "0123456789ABCD" },

    { G_PRE_ORDER,  -1, "VF73102546B98ADCENJHGILKMRPOQTSUldZXWYbachfegjiktpnmorqsxvuwyz" },
    { G_PRE_ORDER,   1, "V" },
    { G_PRE_ORDER,   2, "VF" },
    { G_PRE_ORDER,   3, "VF7" },
    { G_PRE_ORDER,   4, "VF73" },
    { G_PRE_ORDER,   5, "VF731" },
    { G_PRE_ORDER,   6, "VF7310" },
    { G_PRE_ORDER,   7, "VF73102" },
    { G_PRE_ORDER,   8, "VF731025" },
    { G_PRE_ORDER,   9, "VF7310254" },
    { G_PRE_ORDER,  10, "VF73102546" },
    { G_PRE_ORDER,  11, "VF73102546B" },
    { G_PRE_ORDER,  12, "VF73102546B9" },
    { G_PRE_ORDER,  13, "VF73102546B98" },
    { G_PRE_ORDER,  14, "VF73102546B98A" },

    { G_POST_ORDER, -1, "02146538A9CEDB7GIHKMLJOQPSUTRNFWYXacbZegfikjhdmonqsrpuwvzyxtlV" },
    { G_POST_ORDER,  1, "0" },
    { G_POST_ORDER,  2, "02" },
    { G_POST_ORDER,  3, "021" },
    { G_POST_ORDER,  4, "0214" },
    { G_POST_ORDER,  5, "02146" },
    { G_POST_ORDER,  6, "021465" },
    { G_POST_ORDER,  7, "0214653" },
    { G_POST_ORDER,  8, "02146538" },
    { G_POST_ORDER,  9, "02146538A" },
    { G_POST_ORDER, 10, "02146538A9" },
    { G_POST_ORDER, 11, "02146538A9C" },
    { G_POST_ORDER, 12, "02146538A9CE" },
    { G_POST_ORDER, 13, "02146538A9CED" },
    { G_POST_ORDER, 14, "02146538A9CEDB" }
  };
  CallbackData data;

  tree = xtree_new (my_compare);

  for (i = 0; chars[i]; i++)
    xtree_insert (tree, &chars[i], &chars[i]);

  data.s = xstring_new ("");
  for (i = 0; i < G_N_ELEMENTS (orders); i++)
    {
      xstring_set_size (data.s, 0);
      data.count = orders[i].limit;
      xtree_traverse (tree, traverse_func, orders[i].traverse, &data);
      g_assert_cmpstr (data.s->str, ==, orders[i].expected);
    }

  xtree_unref (tree);
  xstring_free (data.s, TRUE);
}

static void
test_tree_insert (void)
{
  xtree_t *tree;
  xchar_t *p;
  xint_t i;
  xchar_t *scrambled;

  tree = xtree_new (my_compare);

  for (i = 0; chars[i]; i++)
    xtree_insert (tree, &chars[i], &chars[i]);
  p = chars;
  xtree_foreach (tree, check_order, &p);

  xtree_unref (tree);
  tree = xtree_new (my_compare);

  for (i = strlen (chars) - 1; i >= 0; i--)
    xtree_insert (tree, &chars[i], &chars[i]);
  p = chars;
  xtree_foreach (tree, check_order, &p);

  xtree_unref (tree);
  tree = xtree_new (my_compare);

  scrambled = xstrdup (chars);

  for (i = 0; i < 30; i++)
    {
      xchar_t tmp;
      xint_t a, b;

      a = g_random_int_range (0, strlen (scrambled));
      b = g_random_int_range (0, strlen (scrambled));
      tmp = scrambled[a];
      scrambled[a] = scrambled[b];
      scrambled[b] = tmp;
    }

  for (i = 0; scrambled[i]; i++)
    xtree_insert (tree, &scrambled[i], &scrambled[i]);
  p = chars;
  xtree_foreach (tree, check_order, &p);

  g_free (scrambled);
  xtree_unref (tree);
}

static void
binary_tree_bound (xtree_t *tree,
                   char   c,
                   char   expected,
                   int    lower)
{
  GTreeNode *node;

  if (lower)
    node = xtree_lower_bound (tree, &c);
  else
    node = xtree_upper_bound (tree, &c);

  if (g_test_verbose ())
    g_test_message ("%c %s: ", c, lower ? "lower" : "upper");

  if (!node)
    {
      if (!xtree_nnodes (tree))
        {
          if (g_test_verbose ())
            g_test_message ("empty tree");
        }
      else
        {
          GTreeNode *last = xtree_node_last (tree);

          g_assert (last);
          if (g_test_verbose ())
            g_test_message ("past end last %c",
                            *(char *) xtree_node_key (last));
        }
      g_assert (expected == '\x00');
    }
  else
    {
      GTreeNode *begin = xtree_node_first (tree);
      GTreeNode *last = xtree_node_last (tree);
      GTreeNode *prev = xtree_node_previous (node);
      GTreeNode *next = xtree_node_next (node);

      g_assert (expected != '\x00');
      g_assert (expected == *(char *) xtree_node_key (node));

      if (g_test_verbose ())
        g_test_message ("%c", *(char *) xtree_node_key (node));

      if (node != begin)
        {
          g_assert (prev);
          if (g_test_verbose ())
            g_test_message (" prev %c", *(char *) xtree_node_key (prev));
        }
      else
        {
          g_assert (!prev);
          if (g_test_verbose ())
            g_test_message (" no prev, it's the first one");
        }

      if (node != last)
        {
          g_assert (next);
          if (g_test_verbose ())
            g_test_message (" next %c", *(char *) xtree_node_key (next));
        }
      else
        {
          g_assert (!next);
          if (g_test_verbose ())
            g_test_message (" no next, it's the last one");
        }
    }

  if (g_test_verbose ())
    g_test_message ("\n");
}

static void
binary_tree_bounds (xtree_t *tree,
                    char   c,
                    int    mode)
{
  char expectedl, expectedu;
  char first = mode == 0 ? '0' : mode == 1 ? 'A' : 'z';

  g_assert (mode >= 0 && mode <= 3);

  if (c < first)
    expectedl = first;
  else if (c > 'z')
    expectedl = '\x00';
  else
    expectedl = c;

  if (c < first)
    expectedu = first;
  else if (c >= 'z')
    expectedu = '\x00';
  else
    expectedu = c == '9' ? 'A' : c == 'Z' ? 'a' : c + 1;

  if (mode == 3)
    {
      expectedl = '\x00';
      expectedu = '\x00';
    }

  binary_tree_bound (tree, c, expectedl, 1);
  binary_tree_bound (tree, c, expectedu, 0);
}

static void
binary_tree_bounds_test (xtree_t *tree,
                         int    mode)
{
  binary_tree_bounds (tree, 'a', mode);
  binary_tree_bounds (tree, 'A', mode);
  binary_tree_bounds (tree, 'z', mode);
  binary_tree_bounds (tree, 'Z', mode);
  binary_tree_bounds (tree, 'Y', mode);
  binary_tree_bounds (tree, '0', mode);
  binary_tree_bounds (tree, '9', mode);
  binary_tree_bounds (tree, '0' - 1, mode);
  binary_tree_bounds (tree, 'z' + 1, mode);
  binary_tree_bounds (tree, '0' - 2, mode);
  binary_tree_bounds (tree, 'z' + 2, mode);
}

static void
test_tree_bounds (void)
{
  xqueue_t queue = G_QUEUE_INIT;
  xtree_t *tree;
  char chars[62];
  xuint_t i, j;

  tree = xtree_new (my_compare);

  i = 0;
  for (j = 0; j < 10; j++, i++)
    {
      chars[i] = '0' + j;
      g_queue_push_tail (&queue, &chars[i]);
    }

  for (j = 0; j < 26; j++, i++)
    {
      chars[i] = 'A' + j;
      g_queue_push_tail (&queue, &chars[i]);
    }

  for (j = 0; j < 26; j++, i++)
    {
      chars[i] = 'a' + j;
      g_queue_push_tail (&queue, &chars[i]);
    }

  if (g_test_verbose ())
    g_test_message ("tree insert: ");

  while (!g_queue_is_empty (&queue))
    {
      gint32 which = g_random_int_range (0, g_queue_get_length (&queue));
      xpointer_t elem = g_queue_pop_nth (&queue, which);
      GTreeNode *node;

      if (g_test_verbose ())
        g_test_message ("%c ", *(char *) elem);

      node = xtree_insert_node (tree, elem, elem);
      g_assert (xtree_node_key (node) == elem);
      g_assert (xtree_node_value (node) == elem);
    }

  if (g_test_verbose ())
    g_test_message ("\n");

  g_assert_cmpint (xtree_nnodes (tree), ==, 10 + 26 + 26);
  g_assert_cmpint (xtree_height (tree), >=, 6);
  g_assert_cmpint (xtree_height (tree), <=, 8);

  if (g_test_verbose ())
    {
      g_test_message ("tree: ");
      xtree_foreach (tree, my_traverse, NULL);
      g_test_message ("\n");
    }

  binary_tree_bounds_test (tree, 0);

  for (i = 0; i < 10; i++)
    xtree_remove (tree, &chars[i]);

  g_assert_cmpint (xtree_nnodes (tree), ==, 26 + 26);
  g_assert_cmpint (xtree_height (tree), >=, 6);
  g_assert_cmpint (xtree_height (tree), <=, 8);

  if (g_test_verbose ())
    {
      g_test_message ("tree: ");
      xtree_foreach (tree, my_traverse, NULL);
      g_test_message ("\n");
    }

  binary_tree_bounds_test (tree, 1);

  for (i = 10; i < 10 + 26 + 26 - 1; i++)
    xtree_remove (tree, &chars[i]);

  if (g_test_verbose ())
    {
      g_test_message ("tree: ");
      xtree_foreach (tree, my_traverse, NULL);
      g_test_message ("\n");
    }

  binary_tree_bounds_test (tree, 2);

  xtree_remove (tree, &chars[10 + 26 + 26 - 1]);

  if (g_test_verbose ())
    g_test_message ("empty tree\n");

  binary_tree_bounds_test (tree, 3);

  xtree_unref (tree);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/tree/search", test_tree_search);
  g_test_add_func ("/tree/remove", test_tree_remove);
  g_test_add_func ("/tree/destroy", test_tree_destroy);
  g_test_add_func ("/tree/traverse", test_tree_traverse);
  g_test_add_func ("/tree/insert", test_tree_insert);
  g_test_add_func ("/tree/bounds", test_tree_bounds);
  g_test_add_func ("/tree/remove-all", test_tree_remove_all);

  return g_test_run ();
}
