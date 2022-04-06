/*
 * Copyright 2015 Lars Uebernickel
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Lars Uebernickel <lars@uebernic.de>
 */

#include <gio/gio.h>

#include <string.h>

/* Wrapper around xlist_model_get_item() and xlist_model_get_object() which
 * checks they return the same thing. */
static xpointer_t
list_model_get (xlist_model_t *model,
                xuint_t       position)
{
  xobject_t *item = xlist_model_get_item (model, position);
  xobject_t *object = xlist_model_get_object (model, position);

  g_assert_true (item == object);

  g_clear_object (&object);
  return g_steal_pointer (&item);
}

/* Test that constructing/getting/setting properties on a #xlist_store_t works. */
static void
test_store_properties (void)
{
  xlist_store_t *store = NULL;
  xtype_t item_type;

  store = xlist_store_new (XTYPE_MENU_ITEM);
  xobject_get (store, "item-type", &item_type, NULL);
  g_assert_cmpint (item_type, ==, XTYPE_MENU_ITEM);

  g_clear_object (&store);
}

/* Test that #xlist_store_t rejects non-xobject_t item types. */
static void
test_store_non_gobjects (void)
{
  if (g_test_subprocess ())
    {
      /* We have to use xobject_new() since xlist_store_new() checks the item
       * type. We want to check the property setter code works properly. */
      xobject_new (XTYPE_LIST_STORE, "item-type", XTYPE_LONG, NULL);
      return;
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*WARNING*value * of type 'xtype_t' is invalid or "
                             "out of range for property 'item-type'*");
}

static void
test_store_boundaries (void)
{
  xlist_store_t *store;
  xmenu_item_t *item;

  store = xlist_store_new (XTYPE_MENU_ITEM);

  item = xmenu_item_new (NULL, NULL);

  /* remove an item from an empty list */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*g_sequence*");
  xlist_store_remove (store, 0);
  g_test_assert_expected_messages ();

  /* don't allow inserting an item past the end ... */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*g_sequence*");
  xlist_store_insert (store, 1, item);
  g_assert_cmpuint (xlist_model_get_n_items (XLIST_MODEL (store)), ==, 0);
  g_test_assert_expected_messages ();

  /* ... except exactly at the end */
  xlist_store_insert (store, 0, item);
  g_assert_cmpuint (xlist_model_get_n_items (XLIST_MODEL (store)), ==, 1);

  /* remove a non-existing item at exactly the end of the list */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*g_sequence*");
  xlist_store_remove (store, 1);
  g_test_assert_expected_messages ();

  xlist_store_remove (store, 0);
  g_assert_cmpuint (xlist_model_get_n_items (XLIST_MODEL (store)), ==, 0);

  /* splice beyond the end of the list */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*position*");
  xlist_store_splice (store, 1, 0, NULL, 0);
  g_test_assert_expected_messages ();

  /* remove items from an empty list */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*position*");
  xlist_store_splice (store, 0, 1, NULL, 0);
  g_test_assert_expected_messages ();

  xlist_store_append (store, item);
  xlist_store_splice (store, 0, 1, (xpointer_t *) &item, 1);
  g_assert_cmpuint (xlist_model_get_n_items (XLIST_MODEL (store)), ==, 1);

  /* remove more items than exist */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*position*");
  xlist_store_splice (store, 0, 5, NULL, 0);
  g_test_assert_expected_messages ();
  g_assert_cmpuint (xlist_model_get_n_items (XLIST_MODEL (store)), ==, 1);

  xobject_unref (store);
  g_assert_finalize_object (item);
}

static void
test_store_refcounts (void)
{
  xlist_store_t *store;
  xmenu_item_t *items[10];
  xmenu_item_t *tmp;
  xuint_t i;
  xuint_t n_items;

  store = xlist_store_new (XTYPE_MENU_ITEM);

  g_assert_cmpuint (xlist_model_get_n_items (XLIST_MODEL (store)), ==, 0);
  g_assert_null (list_model_get (XLIST_MODEL (store), 0));

  n_items = G_N_ELEMENTS (items);
  for (i = 0; i < n_items; i++)
    {
      items[i] = xmenu_item_new (NULL, NULL);
      xobject_add_weak_pointer (G_OBJECT (items[i]), (xpointer_t *) &items[i]);
      xlist_store_append (store, items[i]);

      xobject_unref (items[i]);
      g_assert_nonnull (items[i]);
    }

  g_assert_cmpuint (xlist_model_get_n_items (XLIST_MODEL (store)), ==, n_items);
  g_assert_null (list_model_get (XLIST_MODEL (store), n_items));

  tmp = list_model_get (XLIST_MODEL (store), 3);
  g_assert_true (tmp == items[3]);
  xobject_unref (tmp);

  xlist_store_remove (store, 4);
  g_assert_null (items[4]);
  n_items--;
  g_assert_cmpuint (xlist_model_get_n_items (XLIST_MODEL (store)), ==, n_items);
  g_assert_null (list_model_get (XLIST_MODEL (store), n_items));

  xobject_unref (store);
  for (i = 0; i < G_N_ELEMENTS (items); i++)
    g_assert_null (items[i]);
}

static xchar_t *
make_random_string (void)
{
  xchar_t *str = g_malloc (10);
  xint_t i;

  for (i = 0; i < 9; i++)
    str[i] = g_test_rand_int_range ('a', 'z');
  str[i] = '\0';

  return str;
}

static xint_t
compare_items (xconstpointer a_p,
               xconstpointer b_p,
               xpointer_t      user_data)
{
  xobject_t *a_o = (xobject_t *) a_p;
  xobject_t *b_o = (xobject_t *) b_p;

  xchar_t *a = xobject_get_data (a_o, "key");
  xchar_t *b = xobject_get_data (b_o, "key");

  g_assert (user_data == GUINT_TO_POINTER(0x1234u));

  return strcmp (a, b);
}

static void
insert_string (xlist_store_t  *store,
               const xchar_t *str)
{
  xobject_t *obj;

  obj = xobject_new (XTYPE_OBJECT, NULL);
  xobject_set_data_full (obj, "key", xstrdup (str), g_free);

  xlist_store_insert_sorted (store, obj, compare_items, GUINT_TO_POINTER(0x1234u));

  xobject_unref (obj);
}

static void
test_store_sorted (void)
{
  xlist_store_t *store;
  xuint_t i;

  store = xlist_store_new (XTYPE_OBJECT);

  for (i = 0; i < 1000; i++)
    {
      xchar_t *str = make_random_string ();
      insert_string (store, str);
      insert_string (store, str); /* multiple copies of the same are OK */
      g_free (str);
    }

  g_assert_cmpint (xlist_model_get_n_items (XLIST_MODEL (store)), ==, 2000);

  for (i = 0; i < 1000; i++)
    {
      xobject_t *a, *b;

      /* should see our two copies */
      a = list_model_get (XLIST_MODEL (store), i * 2);
      b = list_model_get (XLIST_MODEL (store), i * 2 + 1);

      g_assert (compare_items (a, b, GUINT_TO_POINTER(0x1234)) == 0);
      g_assert (a != b);

      if (i)
        {
          xobject_t *c;

          c = list_model_get (XLIST_MODEL (store), i * 2 - 1);
          g_assert (c != a);
          g_assert (c != b);

          g_assert (compare_items (b, c, GUINT_TO_POINTER(0x1234)) > 0);
          g_assert (compare_items (a, c, GUINT_TO_POINTER(0x1234)) > 0);

          xobject_unref (c);
        }

      xobject_unref (a);
      xobject_unref (b);
    }

  xobject_unref (store);
}

/* Test that using splice() to replace the middle element in a list store works. */
static void
test_store_splice_replace_middle (void)
{
  xlist_store_t *store;
  xlist_model_t *model;
  xaction_t *item;
  xptr_array_t *array;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=795307");

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);
  model = XLIST_MODEL (store);

  array = xptr_array_new_full (0, xobject_unref);
  xptr_array_add (array, g_simple_action_new ("1", NULL));
  xptr_array_add (array, g_simple_action_new ("2", NULL));
  xptr_array_add (array, g_simple_action_new ("3", NULL));
  xptr_array_add (array, g_simple_action_new ("4", NULL));
  xptr_array_add (array, g_simple_action_new ("5", NULL));

  /* Add three items through splice */
  xlist_store_splice (store, 0, 0, array->pdata, 3);
  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 3);

  item = list_model_get (model, 0);
  g_assert_cmpstr (g_action_get_name (item), ==, "1");
  xobject_unref (item);
  item = list_model_get (model, 1);
  g_assert_cmpstr (g_action_get_name (item), ==, "2");
  xobject_unref (item);
  item = list_model_get (model, 2);
  g_assert_cmpstr (g_action_get_name (item), ==, "3");
  xobject_unref (item);

  /* Replace the middle one with two new items */
  xlist_store_splice (store, 1, 1, array->pdata + 3, 2);
  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 4);

  item = list_model_get (model, 0);
  g_assert_cmpstr (g_action_get_name (item), ==, "1");
  xobject_unref (item);
  item = list_model_get (model, 1);
  g_assert_cmpstr (g_action_get_name (item), ==, "4");
  xobject_unref (item);
  item = list_model_get (model, 2);
  g_assert_cmpstr (g_action_get_name (item), ==, "5");
  xobject_unref (item);
  item = list_model_get (model, 3);
  g_assert_cmpstr (g_action_get_name (item), ==, "3");
  xobject_unref (item);

  xptr_array_unref (array);
  xobject_unref (store);
}

/* Test that using splice() to replace the whole list store works. */
static void
test_store_splice_replace_all (void)
{
  xlist_store_t *store;
  xlist_model_t *model;
  xptr_array_t *array;
  xaction_t *item;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=795307");

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);
  model = XLIST_MODEL (store);

  array = xptr_array_new_full (0, xobject_unref);
  xptr_array_add (array, g_simple_action_new ("1", NULL));
  xptr_array_add (array, g_simple_action_new ("2", NULL));
  xptr_array_add (array, g_simple_action_new ("3", NULL));
  xptr_array_add (array, g_simple_action_new ("4", NULL));

  /* Add the first two */
  xlist_store_splice (store, 0, 0, array->pdata, 2);

  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 2);
  item = list_model_get (model, 0);
  g_assert_cmpstr (g_action_get_name (item), ==, "1");
  xobject_unref (item);
  item = list_model_get (model, 1);
  g_assert_cmpstr (g_action_get_name (item), ==, "2");
  xobject_unref (item);

  /* Replace all with the last two */
  xlist_store_splice (store, 0, 2, array->pdata + 2, 2);

  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 2);
  item = list_model_get (model, 0);
  g_assert_cmpstr (g_action_get_name (item), ==, "3");
  xobject_unref (item);
  item = list_model_get (model, 1);
  g_assert_cmpstr (g_action_get_name (item), ==, "4");
  xobject_unref (item);

  xptr_array_unref (array);
  xobject_unref (store);
}

/* Test that using splice() without removing or adding anything works */
static void
test_store_splice_noop (void)
{
  xlist_store_t *store;
  xlist_model_t *model;
  xaction_t *item;

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);
  model = XLIST_MODEL (store);

  /* splice noop with an empty list */
  xlist_store_splice (store, 0, 0, NULL, 0);
  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 0);

  /* splice noop with a non-empty list */
  item = G_ACTION (g_simple_action_new ("1", NULL));
  xlist_store_append (store, item);
  xobject_unref (item);

  xlist_store_splice (store, 0, 0, NULL, 0);
  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 1);

  xlist_store_splice (store, 1, 0, NULL, 0);
  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 1);

  item = list_model_get (model, 0);
  g_assert_cmpstr (g_action_get_name (item), ==, "1");
  xobject_unref (item);

  xobject_unref (store);
}

static xboolean_t
model_array_equal (xlist_model_t *model, xptr_array_t *array)
{
  xuint_t i;

  if (xlist_model_get_n_items (model) != array->len)
    return FALSE;

  for (i = 0; i < array->len; i++)
    {
      xobject_t *ptr;
      xboolean_t ptrs_equal;

      ptr = list_model_get (model, i);
      ptrs_equal = (xptr_array_index (array, i) == ptr);
      xobject_unref (ptr);
      if (!ptrs_equal)
        return FALSE;
    }

  return TRUE;
}

/* Test that using splice() to remove multiple items at different
 * positions works */
static void
test_store_splice_remove_multiple (void)
{
  xlist_store_t *store;
  xlist_model_t *model;
  xptr_array_t *array;

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);
  model = XLIST_MODEL (store);

  array = xptr_array_new_full (0, xobject_unref);
  xptr_array_add (array, g_simple_action_new ("1", NULL));
  xptr_array_add (array, g_simple_action_new ("2", NULL));
  xptr_array_add (array, g_simple_action_new ("3", NULL));
  xptr_array_add (array, g_simple_action_new ("4", NULL));
  xptr_array_add (array, g_simple_action_new ("5", NULL));
  xptr_array_add (array, g_simple_action_new ("6", NULL));
  xptr_array_add (array, g_simple_action_new ("7", NULL));
  xptr_array_add (array, g_simple_action_new ("8", NULL));
  xptr_array_add (array, g_simple_action_new ("9", NULL));
  xptr_array_add (array, g_simple_action_new ("10", NULL));

  /* Add all */
  xlist_store_splice (store, 0, 0, array->pdata, array->len);
  g_assert_true (model_array_equal (model, array));

  /* Remove the first two */
  xlist_store_splice (store, 0, 2, NULL, 0);
  g_assert_false (model_array_equal (model, array));
  xptr_array_remove_range (array, 0, 2);
  g_assert_true (model_array_equal (model, array));
  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 8);

  /* Remove two in the middle */
  xlist_store_splice (store, 2, 2, NULL, 0);
  g_assert_false (model_array_equal (model, array));
  xptr_array_remove_range (array, 2, 2);
  g_assert_true (model_array_equal (model, array));
  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 6);

  /* Remove two at the end */
  xlist_store_splice (store, 4, 2, NULL, 0);
  g_assert_false (model_array_equal (model, array));
  xptr_array_remove_range (array, 4, 2);
  g_assert_true (model_array_equal (model, array));
  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 4);

  xptr_array_unref (array);
  xobject_unref (store);
}

/* Test that using splice() to add multiple items at different
 * positions works */
static void
test_store_splice_add_multiple (void)
{
  xlist_store_t *store;
  xlist_model_t *model;
  xptr_array_t *array;

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);
  model = XLIST_MODEL (store);

  array = xptr_array_new_full (0, xobject_unref);
  xptr_array_add (array, g_simple_action_new ("1", NULL));
  xptr_array_add (array, g_simple_action_new ("2", NULL));
  xptr_array_add (array, g_simple_action_new ("3", NULL));
  xptr_array_add (array, g_simple_action_new ("4", NULL));
  xptr_array_add (array, g_simple_action_new ("5", NULL));
  xptr_array_add (array, g_simple_action_new ("6", NULL));

  /* Add two at the beginning */
  xlist_store_splice (store, 0, 0, array->pdata, 2);

  /* Add two at the end */
  xlist_store_splice (store, 2, 0, array->pdata + 4, 2);

  /* Add two in the middle */
  xlist_store_splice (store, 2, 0, array->pdata + 2, 2);

  g_assert_true (model_array_equal (model, array));

  xptr_array_unref (array);
  xobject_unref (store);
}

/* Test that get_item_type() returns the right type */
static void
test_store_item_type (void)
{
  xlist_store_t *store;
  xtype_t gtype;

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);
  gtype = xlist_model_get_item_type (XLIST_MODEL (store));
  g_assert (gtype == XTYPE_SIMPLE_ACTION);

  xobject_unref (store);
}

/* Test that remove_all() removes all items */
static void
test_store_remove_all (void)
{
  xlist_store_t *store;
  xlist_model_t *model;
  xsimple_action_t *item;

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);
  model = XLIST_MODEL (store);

  /* Test with an empty list */
  xlist_store_remove_all (store);
  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 0);

  /* Test with a non-empty list */
  item = g_simple_action_new ("42", NULL);
  xlist_store_append (store, item);
  xlist_store_append (store, item);
  xobject_unref (item);
  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 2);
  xlist_store_remove_all (store);
  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 0);

  xobject_unref (store);
}

/* Test that splice() logs an error when passed the wrong item type */
static void
test_store_splice_wrong_type (void)
{
  xlist_store_t *store;

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);

  g_test_expect_message (G_LOG_DOMAIN,
                         G_LOG_LEVEL_CRITICAL,
                         "*xlist_store_t instead of a xsimple_action_t*");
  xlist_store_splice (store, 0, 0, (xpointer_t)&store, 1);

  xobject_unref (store);
}

static xint_t
ptr_array_cmp_action_by_name (xaction_t **a, xaction_t **b)
{
  return xstrcmp0 (g_action_get_name (*a), g_action_get_name (*b));
}

static xint_t
list_model_cmp_action_by_name (xaction_t *a, xaction_t *b, xpointer_t user_data)
{
  return xstrcmp0 (g_action_get_name (a), g_action_get_name (b));
}

/* Test if sort() works */
static void
test_store_sort (void)
{
  xlist_store_t *store;
  xlist_model_t *model;
  xptr_array_t *array;

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);
  model = XLIST_MODEL (store);

  array = xptr_array_new_full (0, xobject_unref);
  xptr_array_add (array, g_simple_action_new ("2", NULL));
  xptr_array_add (array, g_simple_action_new ("3", NULL));
  xptr_array_add (array, g_simple_action_new ("9", NULL));
  xptr_array_add (array, g_simple_action_new ("4", NULL));
  xptr_array_add (array, g_simple_action_new ("5", NULL));
  xptr_array_add (array, g_simple_action_new ("8", NULL));
  xptr_array_add (array, g_simple_action_new ("6", NULL));
  xptr_array_add (array, g_simple_action_new ("7", NULL));
  xptr_array_add (array, g_simple_action_new ("1", NULL));

  /* Sort an empty list */
  xlist_store_sort (store, (GCompareDataFunc)list_model_cmp_action_by_name, NULL);

  /* Add all */
  xlist_store_splice (store, 0, 0, array->pdata, array->len);
  g_assert_true (model_array_equal (model, array));

  /* Sort both and check if the result is the same */
  xptr_array_sort (array, (GCompareFunc)ptr_array_cmp_action_by_name);
  g_assert_false (model_array_equal (model, array));
  xlist_store_sort (store, (GCompareDataFunc)list_model_cmp_action_by_name, NULL);
  g_assert_true (model_array_equal (model, array));

  xptr_array_unref (array);
  xobject_unref (store);
}

/* Test the cases where the item store tries to speed up item access by caching
 * the last iter/position */
static void
test_store_get_item_cache (void)
{
  xlist_store_t *store;
  xlist_model_t *model;
  xsimple_action_t *item1, *item2, *temp;

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);
  model = XLIST_MODEL (store);

  /* Add two */
  item1 = g_simple_action_new ("1", NULL);
  xlist_store_append (store, item1);
  item2 = g_simple_action_new ("2", NULL);
  xlist_store_append (store, item2);

  /* Clear the cache */
  g_assert_null (list_model_get (model, 42));

  /* Access the same position twice */
  temp = list_model_get (model, 1);
  g_assert (temp == item2);
  xobject_unref (temp);
  temp = list_model_get (model, 1);
  g_assert (temp == item2);
  xobject_unref (temp);

  g_assert_null (list_model_get (model, 42));

  /* Access forwards */
  temp = list_model_get (model, 0);
  g_assert (temp == item1);
  xobject_unref (temp);
  temp = list_model_get (model, 1);
  g_assert (temp == item2);
  xobject_unref (temp);

  g_assert_null (list_model_get (model, 42));

  /* Access backwards */
  temp = list_model_get (model, 1);
  g_assert (temp == item2);
  xobject_unref (temp);
  temp = list_model_get (model, 0);
  g_assert (temp == item1);
  xobject_unref (temp);

  xobject_unref (item1);
  xobject_unref (item2);
  xobject_unref (store);
}

struct ItemsChangedData
{
  xuint_t position;
  xuint_t removed;
  xuint_t added;
  xboolean_t called;
};

static void
expect_items_changed (struct ItemsChangedData *expected,
                      xuint_t position,
                      xuint_t removed,
                      xuint_t added)
{
  expected->position = position;
  expected->removed = removed;
  expected->added = added;
  expected->called = FALSE;
}

static void
on_items_changed (xlist_model_t *model,
                  xuint_t position,
                  xuint_t removed,
                  xuint_t added,
                  struct ItemsChangedData *expected)
{
  g_assert_false (expected->called);
  g_assert_cmpuint (expected->position, ==, position);
  g_assert_cmpuint (expected->removed, ==, removed);
  g_assert_cmpuint (expected->added, ==, added);
  expected->called = TRUE;
}

/* Test that all operations on the list emit the items-changed signal */
static void
test_store_signal_items_changed (void)
{
  xlist_store_t *store;
  xlist_model_t *model;
  xsimple_action_t *item;
  struct ItemsChangedData expected = {0};

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);
  model = XLIST_MODEL (store);

  xobject_connect (model, "signal::items-changed",
                    on_items_changed, &expected, NULL);

  /* Emit the signal manually */
  expect_items_changed (&expected, 0, 0, 0);
  xlist_model_items_changed (model, 0, 0, 0);
  g_assert_true (expected.called);

  /* Append an item */
  expect_items_changed (&expected, 0, 0, 1);
  item = g_simple_action_new ("2", NULL);
  xlist_store_append (store, item);
  xobject_unref (item);
  g_assert_true (expected.called);

  /* Insert an item */
  expect_items_changed (&expected, 1, 0, 1);
  item = g_simple_action_new ("1", NULL);
  xlist_store_insert (store, 1, item);
  xobject_unref (item);
  g_assert_true (expected.called);

  /* Sort the list */
  expect_items_changed (&expected, 0, 2, 2);
  xlist_store_sort (store,
                     (GCompareDataFunc)list_model_cmp_action_by_name,
                     NULL);
  g_assert_true (expected.called);

  /* Insert sorted */
  expect_items_changed (&expected, 2, 0, 1);
  item = g_simple_action_new ("3", NULL);
  xlist_store_insert_sorted (store,
                              item,
                              (GCompareDataFunc)list_model_cmp_action_by_name,
                              NULL);
  xobject_unref (item);
  g_assert_true (expected.called);

  /* Remove an item */
  expect_items_changed (&expected, 1, 1, 0);
  xlist_store_remove (store, 1);
  g_assert_true (expected.called);

  /* Splice */
  expect_items_changed (&expected, 0, 2, 1);
  item = g_simple_action_new ("4", NULL);
  g_assert_cmpuint (xlist_model_get_n_items (model), >=, 2);
  xlist_store_splice (store, 0, 2, (xpointer_t)&item, 1);
  xobject_unref (item);
  g_assert_true (expected.called);

  /* Remove all */
  expect_items_changed (&expected, 0, 1, 0);
  g_assert_cmpuint (xlist_model_get_n_items (model), ==, 1);
  xlist_store_remove_all (store);
  g_assert_true (expected.called);

  xobject_unref (store);
}

/* Due to an overflow in the list store last-iter optimization,
 * the sequence 'lookup 0; lookup MAXUINT' was returning the
 * same item twice, and not NULL for the second lookup.
 * See #1639.
 */
static void
test_store_past_end (void)
{
  xlist_store_t *store;
  xlist_model_t *model;
  xsimple_action_t *item;

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);
  model = XLIST_MODEL (store);

  item = g_simple_action_new ("2", NULL);
  xlist_store_append (store, item);
  xobject_unref (item);

  g_assert_cmpint (xlist_model_get_n_items (model), ==, 1);
  item = xlist_model_get_item (model, 0);
  g_assert_nonnull (item);
  xobject_unref (item);
  item = xlist_model_get_item (model, G_MAXUINT);
  g_assert_null (item);

  xobject_unref (store);
}

static xboolean_t
list_model_casecmp_action_by_name (xconstpointer a,
                                   xconstpointer b)
{
  return g_ascii_strcasecmp (g_action_get_name (G_ACTION (a)),
                             g_action_get_name (G_ACTION (b))) == 0;
}

/* Test if find() and find_with_equal_func() works */
static void
test_store_find (void)
{
  xlist_store_t *store;
  xuint_t position = 100;
  const xchar_t *item_strs[4] = { "aaa", "bbb", "xxx", "ccc" };
  xsimple_action_t *items[4] = { NULL, };
  xsimple_action_t *other_item;
  xuint_t i;

  store = xlist_store_new (XTYPE_SIMPLE_ACTION);

  for (i = 0; i < G_N_ELEMENTS (item_strs); i++)
    items[i] = g_simple_action_new (item_strs[i], NULL);

  /* Shouldn't crash on an empty list, or change the position pointer */
  g_assert_false (xlist_store_find (store, items[0], NULL));
  g_assert_false (xlist_store_find (store, items[0], &position));
  g_assert_cmpint (position, ==, 100);

  for (i = 0; i < G_N_ELEMENTS (item_strs); i++)
    xlist_store_append (store, items[i]);

  /* Check whether it could still find the the elements */
  for (i = 0; i < G_N_ELEMENTS (item_strs); i++)
    {
      g_assert_true (xlist_store_find (store, items[i], &position));
      g_assert_cmpint (position, ==, i);
      /* Shouldn't try to write to position pointer if NULL given */
      g_assert_true (xlist_store_find (store, items[i], NULL));
    }

  /* try to find element not part of the list */
  other_item = g_simple_action_new ("111", NULL);
  g_assert_false (xlist_store_find (store, other_item, NULL));
  g_clear_object (&other_item);

  /* Re-add item; find() should only return the first position */
  xlist_store_append (store, items[0]);
  g_assert_true (xlist_store_find (store, items[0], &position));
  g_assert_cmpint (position, ==, 0);

  /* try to find element which should only work with custom equality check */
  other_item = g_simple_action_new ("XXX", NULL);
  g_assert_false (xlist_store_find (store, other_item, NULL));
  g_assert_true (xlist_store_find_with_equal_func (store,
                                                    other_item,
                                                    list_model_casecmp_action_by_name,
                                                    &position));
  g_assert_cmpint (position, ==, 2);
  g_clear_object (&other_item);

  for (i = 0; i < G_N_ELEMENTS (item_strs); i++)
    g_clear_object(&items[i]);
  g_clear_object (&store);
}

int main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/glistmodel/store/properties", test_store_properties);
  g_test_add_func ("/glistmodel/store/non-gobjects", test_store_non_gobjects);
  g_test_add_func ("/glistmodel/store/boundaries", test_store_boundaries);
  g_test_add_func ("/glistmodel/store/refcounts", test_store_refcounts);
  g_test_add_func ("/glistmodel/store/sorted", test_store_sorted);
  g_test_add_func ("/glistmodel/store/splice-replace-middle",
                   test_store_splice_replace_middle);
  g_test_add_func ("/glistmodel/store/splice-replace-all",
                   test_store_splice_replace_all);
  g_test_add_func ("/glistmodel/store/splice-noop", test_store_splice_noop);
  g_test_add_func ("/glistmodel/store/splice-remove-multiple",
                   test_store_splice_remove_multiple);
  g_test_add_func ("/glistmodel/store/splice-add-multiple",
                   test_store_splice_add_multiple);
  g_test_add_func ("/glistmodel/store/splice-wrong-type",
                   test_store_splice_wrong_type);
  g_test_add_func ("/glistmodel/store/item-type",
                   test_store_item_type);
  g_test_add_func ("/glistmodel/store/remove-all",
                   test_store_remove_all);
  g_test_add_func ("/glistmodel/store/sort",
                   test_store_sort);
  g_test_add_func ("/glistmodel/store/get-item-cache",
                   test_store_get_item_cache);
  g_test_add_func ("/glistmodel/store/items-changed",
                   test_store_signal_items_changed);
  g_test_add_func ("/glistmodel/store/past-end", test_store_past_end);
  g_test_add_func ("/glistmodel/store/find", test_store_find);

  return g_test_run ();
}
