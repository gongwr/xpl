#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include <glib-object.h>

typedef struct _MyBoxed MyBoxed;

struct _MyBoxed
{
  xint_t ivalue;
  xchar_t *bla;
};

static xpointer_t
my_boxed_copy (xpointer_t orig)
{
  MyBoxed *a = orig;
  MyBoxed *b;

  b = g_slice_new (MyBoxed);
  b->ivalue = a->ivalue;
  b->bla = xstrdup (a->bla);

  return b;
}

static xint_t my_boxed_free_count;

static void
my_boxed_free (xpointer_t orig)
{
  MyBoxed *a = orig;

  g_free (a->bla);
  g_slice_free (MyBoxed, a);

  my_boxed_free_count++;
}

static xtype_t my_boxed_get_type (void);
#define MY_TYPE_BOXED (my_boxed_get_type ())

G_DEFINE_BOXED_TYPE (MyBoxed, my_boxed, my_boxed_copy, my_boxed_free)

static void
test_define_boxed (void)
{
  MyBoxed a;
  MyBoxed *b;

  a.ivalue = 20;
  a.bla = xstrdup ("bla");

  b = xboxed_copy (MY_TYPE_BOXED, &a);

  g_assert_cmpint (b->ivalue, ==, 20);
  g_assert_cmpstr (b->bla, ==, "bla");

  xboxed_free (MY_TYPE_BOXED, b);

  g_free (a.bla);
}

static void
test_boxed_ownership (void)
{
  xvalue_t value = G_VALUE_INIT;
  static MyBoxed boxed = { 10, "bla" };

  xvalue_init (&value, MY_TYPE_BOXED);

  my_boxed_free_count = 0;

  xvalue_set_static_boxed (&value, &boxed);
  xvalue_reset (&value);

  g_assert_cmpint (my_boxed_free_count, ==, 0);

  xvalue_set_boxed_take_ownership (&value, xboxed_copy (MY_TYPE_BOXED, &boxed));
  xvalue_reset (&value);
  g_assert_cmpint (my_boxed_free_count, ==, 1);

  xvalue_take_boxed (&value, xboxed_copy (MY_TYPE_BOXED, &boxed));
  xvalue_reset (&value);
  g_assert_cmpint (my_boxed_free_count, ==, 2);

  xvalue_set_boxed (&value, &boxed);
  xvalue_reset (&value);
  g_assert_cmpint (my_boxed_free_count, ==, 3);
}

static void
my_callback (xpointer_t user_data)
{
}

static xint_t destroy_count;

static void
my_closure_notify (xpointer_t user_data, xclosure_t *closure)
{
  destroy_count++;
}

static void
test_boxed_closure (void)
{
  xclosure_t *closure;
  xclosure_t *closure2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_CLOSURE);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  closure = g_cclosure_new (G_CALLBACK (my_callback), "bla", my_closure_notify);
  xvalue_take_boxed (&value, closure);

  closure2 = xvalue_get_boxed (&value);
  xassert (closure2 == closure);

  closure2 = xvalue_dup_boxed (&value);
  xassert (closure2 == closure); /* closures use ref/unref for copy/free */
  xclosure_unref (closure2);

  xvalue_unset (&value);
  g_assert_cmpint (destroy_count, ==, 1);
}

static void
test_boxed_date (void)
{
  xdate_t *date;
  xdate_t *date2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_DATE);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  date = xdate_new_dmy (1, 3, 1970);
  xvalue_take_boxed (&value, date);

  date2 = xvalue_get_boxed (&value);
  xassert (date2 == date);

  date2 = xvalue_dup_boxed (&value);
  xassert (date2 != date);
  xassert (xdate_compare (date, date2) == 0);
  xdate_free (date2);

  xvalue_unset (&value);
}

static void
test_boxed_value (void)
{
  xvalue_t value1 = G_VALUE_INIT;
  xvalue_t *value2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_VALUE);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  xvalue_init (&value1, XTYPE_INT);
  xvalue_set_int (&value1, 26);

  xvalue_set_static_boxed (&value, &value1);

  value2 = xvalue_get_boxed (&value);
  xassert (value2 == &value1);

  value2 = xvalue_dup_boxed (&value);
  xassert (value2 != &value1);
  xassert (G_VALUE_HOLDS_INT (value2));
  g_assert_cmpint (xvalue_get_int (value2), ==, 26);
  xboxed_free (XTYPE_VALUE, value2);

  xvalue_unset (&value);
}

static void
test_boxed_string (void)
{
  xstring_t *v;
  xstring_t *v2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_GSTRING);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  v = xstring_new ("bla");
  xvalue_take_boxed (&value, v);

  v2 = xvalue_get_boxed (&value);
  xassert (v2 == v);

  v2 = xvalue_dup_boxed (&value);
  xassert (v2 != v);
  xassert (xstring_equal (v, v2));
  xstring_free (v2, TRUE);

  xvalue_unset (&value);
}

static void
test_boxed_hashtable (void)
{
  xhashtable_t *v;
  xhashtable_t *v2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_HASH_TABLE);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  v = xhash_table_new (xstr_hash, xstr_equal);
  xvalue_take_boxed (&value, v);

  v2 = xvalue_get_boxed (&value);
  xassert (v2 == v);

  v2 = xvalue_dup_boxed (&value);
  xassert (v2 == v);  /* hash tables use ref/unref for copy/free */
  xhash_table_unref (v2);

  xvalue_unset (&value);
}

static void
test_boxed_array (void)
{
  xarray_t *v;
  xarray_t *v2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_ARRAY);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  v = g_array_new (TRUE, FALSE, 1);
  xvalue_take_boxed (&value, v);

  v2 = xvalue_get_boxed (&value);
  xassert (v2 == v);

  v2 = xvalue_dup_boxed (&value);
  xassert (v2 == v);  /* arrays use ref/unref for copy/free */
  g_array_unref (v2);

  xvalue_unset (&value);
}

static void
test_boxed_ptrarray (void)
{
  xptr_array_t *v;
  xptr_array_t *v2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_PTR_ARRAY);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  v = xptr_array_new ();
  xvalue_take_boxed (&value, v);

  v2 = xvalue_get_boxed (&value);
  xassert (v2 == v);

  v2 = xvalue_dup_boxed (&value);
  xassert (v2 == v);  /* ptr arrays use ref/unref for copy/free */
  xptr_array_unref (v2);

  xvalue_unset (&value);
}

static void
test_boxed_regex (void)
{
  xregex_t *v;
  xregex_t *v2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_REGEX);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  v = xregex_new ("a+b+", 0, 0, NULL);
  xvalue_take_boxed (&value, v);

  v2 = xvalue_get_boxed (&value);
  xassert (v2 == v);

  v2 = xvalue_dup_boxed (&value);
  xassert (v2 == v);  /* regexes use ref/unref for copy/free */
  xregex_unref (v2);

  xvalue_unset (&value);
}

static void
test_boxed_matchinfo (void)
{
  xregex_t *r;
  xmatch_info_t *info, *info2;
  xboolean_t ret;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_MATCH_INFO);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  r = xregex_new ("ab", 0, 0, NULL);
  ret = xregex_match (r, "blabla abab bla", 0, &info);
  xassert (ret);
  xvalue_take_boxed (&value, info);

  info2 = xvalue_get_boxed (&value);
  xassert (info == info2);

  info2 = xvalue_dup_boxed (&value);
  xassert (info == info2);  /* matchinfo uses ref/unref for copy/free */
  xmatch_info_unref (info2);

  xvalue_unset (&value);
  xregex_unref (r);
}

static void
test_boxed_varianttype (void)
{
  xvariant_type_t *v;
  xvariant_type_t *v2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_VARIANT_TYPE);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  v = xvariant_type_new ("mas");
  xvalue_take_boxed (&value, v);

  v2 = xvalue_get_boxed (&value);
  xassert (v2 == v);

  v2 = xvalue_dup_boxed (&value);
  xassert (v2 != v);
  g_assert_cmpstr (xvariant_type_peek_string (v), ==, xvariant_type_peek_string (v2));
  xvariant_type_free (v2);

  xvalue_unset (&value);
}

static void
test_boxed_datetime (void)
{
  xdatetime_t *v;
  xdatetime_t *v2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_DATE_TIME);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  v = xdate_time_new_now_local ();
  xvalue_take_boxed (&value, v);

  v2 = xvalue_get_boxed (&value);
  xassert (v2 == v);

  v2 = xvalue_dup_boxed (&value);
  xassert (v2 == v); /* datetime uses ref/unref for copy/free */
  xdate_time_unref (v2);

  xvalue_unset (&value);
}

static void
test_boxed_error (void)
{
  xerror_t *v;
  xerror_t *v2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_ERROR);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  v = xerror_new_literal (G_VARIANT_PARSE_ERROR,
                           G_VARIANT_PARSE_ERROR_NUMBER_TOO_BIG,
                           "Too damn big");
  xvalue_take_boxed (&value, v);

  v2 = xvalue_get_boxed (&value);
  xassert (v2 == v);

  v2 = xvalue_dup_boxed (&value);
  xassert (v2 != v);
  g_assert_cmpint (v->domain, ==, v2->domain);
  g_assert_cmpint (v->code, ==, v2->code);
  g_assert_cmpstr (v->message, ==, v2->message);
  xerror_free (v2);

  xvalue_unset (&value);
}

static void
test_boxed_keyfile (void)
{
  xkey_file_t *k, *k2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_KEY_FILE);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  k = xkey_file_new ();
  xvalue_take_boxed (&value, k);

  k2 = xvalue_get_boxed (&value);
  xassert (k == k2);

  k2 = xvalue_dup_boxed (&value);
  xassert (k == k2);  /* keyfile uses ref/unref for copy/free */
  xkey_file_unref (k2);

  xvalue_unset (&value);
}

static void
test_boxed_mainloop (void)
{
  xmain_loop_t *l, *l2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_MAIN_LOOP);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  l = xmain_loop_new (NULL, FALSE);
  xvalue_take_boxed (&value, l);

  l2 = xvalue_get_boxed (&value);
  xassert (l == l2);

  l2 = xvalue_dup_boxed (&value);
  xassert (l == l2);  /* mainloop uses ref/unref for copy/free */
  xmain_loop_unref (l2);

  xvalue_unset (&value);
}

static void
test_boxed_maincontext (void)
{
  xmain_context_t *c, *c2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_MAIN_CONTEXT);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  c = xmain_context_new ();
  xvalue_take_boxed (&value, c);

  c2 = xvalue_get_boxed (&value);
  xassert (c == c2);

  c2 = xvalue_dup_boxed (&value);
  xassert (c == c2);  /* maincontext uses ref/unref for copy/free */
  xmain_context_unref (c2);

  xvalue_unset (&value);
}

static void
test_boxed_source (void)
{
  xsource_t *s, *s2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_SOURCE);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  s = g_idle_source_new ();
  xvalue_take_boxed (&value, s);

  s2 = xvalue_get_boxed (&value);
  xassert (s == s2);

  s2 = xvalue_dup_boxed (&value);
  xassert (s == s2);  /* source uses ref/unref for copy/free */
  xsource_unref (s2);

  xvalue_unset (&value);
}

static void
test_boxed_variantbuilder (void)
{
  xvariant_builder_t *v, *v2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_VARIANT_BUILDER);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  v = xvariant_builder_new (G_VARIANT_TYPE_OBJECT_PATH_ARRAY);
  xvalue_take_boxed (&value, v);

  v2 = xvalue_get_boxed (&value);
  xassert (v == v2);

  v2 = xvalue_dup_boxed (&value);
  xassert (v == v2);  /* variantbuilder uses ref/unref for copy/free */
  xvariant_builder_unref (v2);

  xvalue_unset (&value);
}

static void
test_boxed_timezone (void)
{
  xtimezone_t *z, *z2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_TIME_ZONE);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  z = xtime_zone_new_utc ();
  xvalue_take_boxed (&value, z);

  z2 = xvalue_get_boxed (&value);
  xassert (z == z2);

  z2 = xvalue_dup_boxed (&value);
  xassert (z == z2);  /* timezone uses ref/unref for copy/free */
  xtime_zone_unref (z2);

  xvalue_unset (&value);
}

static void
test_boxed_pollfd (void)
{
  xpollfd_t *p, *p2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_POLLFD);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  p = g_new (xpollfd_t, 1);
  xvalue_take_boxed (&value, p);

  p2 = xvalue_get_boxed (&value);
  xassert (p == p2);

  p2 = xvalue_dup_boxed (&value);
  xassert (p != p2);
  g_free (p2);

  xvalue_unset (&value);
}

static void
test_boxed_markup (void)
{
  xmarkup_parse_context_t *c, *c2;
  const GMarkupParser parser = { 0 };
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_MARKUP_PARSE_CONTEXT);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  c = xmarkup_parse_context_new (&parser, 0, NULL, NULL);
  xvalue_take_boxed (&value, c);

  c2 = xvalue_get_boxed (&value);
  xassert (c == c2);

  c2 = xvalue_dup_boxed (&value);
  xassert (c == c2);
  xmarkup_parse_context_unref (c2);

  xvalue_unset (&value);
}

static void
test_boxed_thread (void)
{
  xthread_t *t, *t2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_THREAD);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  t = xthread_self ();
  xvalue_set_boxed (&value, t);

  t2 = xvalue_get_boxed (&value);
  xassert (t == t2);

  t2 = xvalue_dup_boxed (&value);
  xassert (t == t2);
  xthread_unref (t2);

  xvalue_unset (&value);
}

static void
test_boxed_checksum (void)
{
  xchecksum_t *c, *c2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_CHECKSUM);
  xassert (G_VALUE_HOLDS_BOXED (&value));

  c = xchecksum_new (G_CHECKSUM_SHA512);
  xvalue_take_boxed (&value, c);

  c2 = xvalue_get_boxed (&value);
  xassert (c == c2);

  c2 = xvalue_dup_boxed (&value);
  xassert (c != c2);
  xchecksum_free (c2);

  xvalue_unset (&value);
}

static xint_t
treecmp (xconstpointer a, xconstpointer b)
{
  return (a < b) ? -1 : (a > b);
}

static void
test_boxed_tree (void)
{
  xtree_t *t, *t2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_TREE);
  g_assert_true (G_VALUE_HOLDS_BOXED (&value));

  t = xtree_new (treecmp);
  xvalue_take_boxed (&value, t);

  t2 = xvalue_get_boxed (&value);
  g_assert_true (t == t2);

  t2 = xvalue_dup_boxed (&value);
  g_assert_true (t == t2); /* trees use ref/unref for copy/free */
  xtree_unref (t2);

  xvalue_unset (&value);
}

static void
test_boxed_pattern_spec (void)
{
  xpattern_spec_t *ps, *ps2;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_PATTERN_SPEC);
  g_assert_true (G_VALUE_HOLDS_BOXED (&value));

  ps = xpattern_spec_new ("*abc*?cde");
  xvalue_take_boxed (&value, ps);

  ps2 = xvalue_get_boxed (&value);
  g_assert_true (ps == ps2);

  ps2 = xvalue_dup_boxed (&value);
  g_assert_true (ps != ps2);
  g_assert_true (xpattern_spec_equal (ps, ps2));
  xpattern_spec_free (ps2);

  xvalue_unset (&value);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/boxed/define", test_define_boxed);
  g_test_add_func ("/boxed/ownership", test_boxed_ownership);
  g_test_add_func ("/boxed/closure", test_boxed_closure);
  g_test_add_func ("/boxed/date", test_boxed_date);
  g_test_add_func ("/boxed/value", test_boxed_value);
  g_test_add_func ("/boxed/string", test_boxed_string);
  g_test_add_func ("/boxed/hashtable", test_boxed_hashtable);
  g_test_add_func ("/boxed/array", test_boxed_array);
  g_test_add_func ("/boxed/ptrarray", test_boxed_ptrarray);
  g_test_add_func ("/boxed/regex", test_boxed_regex);
  g_test_add_func ("/boxed/varianttype", test_boxed_varianttype);
  g_test_add_func ("/boxed/error", test_boxed_error);
  g_test_add_func ("/boxed/datetime", test_boxed_datetime);
  g_test_add_func ("/boxed/matchinfo", test_boxed_matchinfo);
  g_test_add_func ("/boxed/keyfile", test_boxed_keyfile);
  g_test_add_func ("/boxed/mainloop", test_boxed_mainloop);
  g_test_add_func ("/boxed/maincontext", test_boxed_maincontext);
  g_test_add_func ("/boxed/source", test_boxed_source);
  g_test_add_func ("/boxed/variantbuilder", test_boxed_variantbuilder);
  g_test_add_func ("/boxed/timezone", test_boxed_timezone);
  g_test_add_func ("/boxed/pollfd", test_boxed_pollfd);
  g_test_add_func ("/boxed/markup", test_boxed_markup);
  g_test_add_func ("/boxed/thread", test_boxed_thread);
  g_test_add_func ("/boxed/checksum", test_boxed_checksum);
  g_test_add_func ("/boxed/tree", test_boxed_tree);
  g_test_add_func ("/boxed/patternspec", test_boxed_pattern_spec);

  return g_test_run ();
}
