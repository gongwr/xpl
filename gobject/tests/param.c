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

#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include <glib-object.h>
#include <stdlib.h>

static void
test_param_spec_char (void)
{
  xparam_spec_t *pspec;
  xvalue_t value = G_VALUE_INIT;

  pspec = xparam_spec_char ("char", "nick", "blurb",
                             20, 40, 30, XPARAM_READWRITE);

  g_assert_cmpstr (xparam_spec_get_name (pspec), ==, "char");
  g_assert_cmpstr (xparam_spec_get_nick (pspec), ==, "nick");
  g_assert_cmpstr (xparam_spec_get_blurb (pspec), ==, "blurb");

  xvalue_init (&value, XTYPE_CHAR);
  xvalue_set_char (&value, 30);

  g_assert_true (g_param_value_defaults (pspec, &value));

  xvalue_set_char (&value, 0);
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_char (&value), ==, 20);

  xvalue_set_char (&value, 20);
  g_assert_false (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_char (&value), ==, 20);

  xvalue_set_char (&value, 40);
  g_assert_false (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_char (&value), ==, 40);

  xvalue_set_char (&value, 60);
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_char (&value), ==, 40);

  xvalue_set_schar (&value, 0);
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_schar (&value), ==, 20);

  xvalue_set_schar (&value, 20);
  g_assert_false (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_schar (&value), ==, 20);

  xvalue_set_schar (&value, 40);
  g_assert_false (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_schar (&value), ==, 40);

  xvalue_set_schar (&value, 60);
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_schar (&value), ==, 40);

  xparam_spec_unref (pspec);
}

static void
test_param_spec_string (void)
{
  xparam_spec_t *pspec;
  xvalue_t value = G_VALUE_INIT;

  pspec = xparam_spec_string ("string", "nick", "blurb",
                               NULL, XPARAM_READWRITE);
  xvalue_init (&value, XTYPE_STRING);

  xvalue_set_string (&value, "foobar");
  g_assert_false (g_param_value_validate (pspec, &value));

  xvalue_set_string (&value, "");
  g_assert_false (g_param_value_validate (pspec, &value));
  g_assert_nonnull (xvalue_get_string (&value));

  /* test ensure_non_null */

  XPARAM_SPEC_STRING (pspec)->ensure_non_null = TRUE;

  xvalue_set_string (&value, NULL);
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_nonnull (xvalue_get_string (&value));

  XPARAM_SPEC_STRING (pspec)->ensure_non_null = FALSE;

  /* test null_fold_if_empty */

  XPARAM_SPEC_STRING (pspec)->null_fold_if_empty = TRUE;

  xvalue_set_string (&value, "");
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_null (xvalue_get_string (&value));

  xvalue_set_static_string (&value, "");
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_null (xvalue_get_string (&value));

  XPARAM_SPEC_STRING (pspec)->null_fold_if_empty = FALSE;

  /* test cset_first */

  XPARAM_SPEC_STRING (pspec)->cset_first = xstrdup ("abc");
  XPARAM_SPEC_STRING (pspec)->substitutor = '-';

  xvalue_set_string (&value, "ABC");
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_string (&value)[0], ==, '-');

  xvalue_set_static_string (&value, "ABC");
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_string (&value)[0], ==, '-');

  /* test cset_nth */

  XPARAM_SPEC_STRING (pspec)->cset_nth = xstrdup ("abc");

  xvalue_set_string (&value, "aBC");
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_string (&value)[1], ==, '-');

  xvalue_set_static_string (&value, "aBC");
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_string (&value)[1], ==, '-');

  xvalue_unset (&value);
  xparam_spec_unref (pspec);
}

static void
test_param_spec_override (void)
{
  xparam_spec_t *ospec, *pspec;
  xvalue_t value = G_VALUE_INIT;

  ospec = xparam_spec_char ("char", "nick", "blurb",
                             20, 40, 30, XPARAM_READWRITE);

  pspec = xparam_spec_override ("override", ospec);

  g_assert_cmpstr (xparam_spec_get_name (pspec), ==, "override");
  g_assert_cmpstr (xparam_spec_get_nick (pspec), ==, "nick");
  g_assert_cmpstr (xparam_spec_get_blurb (pspec), ==, "blurb");

  xvalue_init (&value, XTYPE_CHAR);
  xvalue_set_char (&value, 30);

  g_assert_true (g_param_value_defaults (pspec, &value));

  xvalue_set_char (&value, 0);
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_char (&value), ==, 20);

  xvalue_set_char (&value, 20);
  g_assert_false (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_char (&value), ==, 20);

  xvalue_set_char (&value, 40);
  g_assert_false (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_char (&value), ==, 40);

  xvalue_set_char (&value, 60);
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_char (&value), ==, 40);

  xparam_spec_unref (pspec);
  xparam_spec_unref (ospec);
}

static void
test_param_spec_gtype (void)
{
  xparam_spec_t *pspec;
  xvalue_t value = G_VALUE_INIT;

  pspec = xparam_spec_gtype ("gtype", "nick", "blurb",
                              XTYPE_PARAM, XPARAM_READWRITE);

  xvalue_init (&value, XTYPE_GTYPE);
  xvalue_set_gtype (&value, XTYPE_PARAM);

  g_assert_true (g_param_value_defaults (pspec, &value));

  xvalue_set_gtype (&value, XTYPE_INT);
  g_assert_true (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_gtype (&value), ==, XTYPE_PARAM);

  xvalue_set_gtype (&value, XTYPE_PARAM_INT);
  g_assert_false (g_param_value_validate (pspec, &value));
  g_assert_cmpint (xvalue_get_gtype (&value), ==, XTYPE_PARAM_INT);

  xparam_spec_unref (pspec);
}

static void
test_param_spec_variant (void)
{
  xparam_spec_t *pspec;
  xvalue_t value = G_VALUE_INIT;
  xvalue_t value2 = G_VALUE_INIT;
  xvalue_t value3 = G_VALUE_INIT;
  xvalue_t value4 = G_VALUE_INIT;
  xvalue_t value5 = G_VALUE_INIT;
  xboolean_t modified;

  pspec = xparam_spec_variant ("variant", "nick", "blurb",
                                G_VARIANT_TYPE ("i"),
                                xvariant_new_int32 (42),
                                XPARAM_READWRITE);

  xvalue_init (&value, XTYPE_VARIANT);
  xvalue_set_variant (&value, xvariant_new_int32 (42));

  xvalue_init (&value2, XTYPE_VARIANT);
  xvalue_set_variant (&value2, xvariant_new_int32 (43));

  xvalue_init (&value3, XTYPE_VARIANT);
  xvalue_set_variant (&value3, xvariant_new_int16 (42));

  xvalue_init (&value4, XTYPE_VARIANT);
  xvalue_set_variant (&value4, xvariant_new_parsed ("[@u 15, @u 10]"));

  xvalue_init (&value5, XTYPE_VARIANT);
  xvalue_set_variant (&value5, NULL);

  g_assert_true (g_param_value_defaults (pspec, &value));
  g_assert_false (g_param_value_defaults (pspec, &value2));
  g_assert_false (g_param_value_defaults (pspec, &value3));
  g_assert_false (g_param_value_defaults (pspec, &value4));
  g_assert_false (g_param_value_defaults (pspec, &value5));

  modified = g_param_value_validate (pspec, &value);
  g_assert_false (modified);

  xvalue_reset (&value);
  xvalue_set_variant (&value, xvariant_new_uint32 (41));
  modified = g_param_value_validate (pspec, &value);
  g_assert_true (modified);
  g_assert_cmpint (xvariant_get_int32 (xvalue_get_variant (&value)), ==, 42);
  xvalue_unset (&value);

  xvalue_unset (&value5);
  xvalue_unset (&value4);
  xvalue_unset (&value3);
  xvalue_unset (&value2);

  xparam_spec_unref (pspec);
}

/* test_t g_param_values_cmp() for #GParamSpecVariant. */
static void
test_param_spec_variant_cmp (void)
{
  const struct
    {
      const xvariant_type_t *pspec_type;
      const xchar_t *v1;
      enum
        {
          LESS_THAN = -1,
          EQUAL = 0,
          GREATER_THAN = 1,
          NOT_EQUAL,
        } expected_result;
      const xchar_t *v2;
    }
  vectors[] =
    {
      { G_VARIANT_TYPE ("i"), "@i 1", LESS_THAN, "@i 2" },
      { G_VARIANT_TYPE ("i"), "@i 2", EQUAL, "@i 2" },
      { G_VARIANT_TYPE ("i"), "@i 3", GREATER_THAN, "@i 2" },
      { G_VARIANT_TYPE ("i"), NULL, LESS_THAN, "@i 2" },
      { G_VARIANT_TYPE ("i"), NULL, EQUAL, NULL },
      { G_VARIANT_TYPE ("i"), "@i 1", GREATER_THAN, NULL },
      { G_VARIANT_TYPE ("i"), "@u 1", LESS_THAN, "@u 2" },
      { G_VARIANT_TYPE ("i"), "@as ['hi']", NOT_EQUAL, "@u 2" },
      { G_VARIANT_TYPE ("i"), "@as ['hi']", NOT_EQUAL, "@as ['there']" },
      { G_VARIANT_TYPE ("i"), "@as ['hi']", EQUAL, "@as ['hi']" },
    };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      xparam_spec_t *pspec;
      xvalue_t v1 = G_VALUE_INIT;
      xvalue_t v2 = G_VALUE_INIT;
      xint_t cmp;

      pspec = xparam_spec_variant ("variant", "nick", "blurb",
                                    vectors[i].pspec_type,
                                    NULL,
                                    XPARAM_READWRITE);

      xvalue_init (&v1, XTYPE_VARIANT);
      xvalue_set_variant (&v1,
                           (vectors[i].v1 != NULL) ?
                           xvariant_new_parsed (vectors[i].v1) : NULL);

      xvalue_init (&v2, XTYPE_VARIANT);
      xvalue_set_variant (&v2,
                           (vectors[i].v2 != NULL) ?
                           xvariant_new_parsed (vectors[i].v2) : NULL);

      cmp = g_param_values_cmp (pspec, &v1, &v2);

      switch (vectors[i].expected_result)
        {
        case LESS_THAN:
        case EQUAL:
        case GREATER_THAN:
          g_assert_cmpint (cmp, ==, vectors[i].expected_result);
          break;
        case NOT_EQUAL:
          g_assert_cmpint (cmp, !=, 0);
          break;
        default:
          g_assert_not_reached ();
        }

      xvalue_unset (&v2);
      xvalue_unset (&v1);
      xparam_spec_unref (pspec);
    }
}

static void
test_param_value (void)
{
  xparam_spec_t *p, *p2;
  xparam_spec_t *pp;
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, XTYPE_PARAM);
  g_assert_true (G_VALUE_HOLDS_PARAM (&value));

  p = xparam_spec_int ("my-int", "My Int", "Blurb", 0, 20, 10, XPARAM_READWRITE);

  xvalue_take_param (&value, p);
  p2 = xvalue_get_param (&value);
  g_assert_true (p2 == p);

  pp = xparam_spec_uint ("my-uint", "My UInt", "Blurb", 0, 10, 5, XPARAM_READWRITE);
  xvalue_set_param (&value, pp);

  p2 = xvalue_dup_param (&value);
  g_assert_true (p2 == pp); /* param specs use ref/unref for copy/free */
  xparam_spec_unref (p2);

  xvalue_unset (&value);
  xparam_spec_unref (pp);
}

static xint_t destroy_count;

static void
my_destroy (xpointer_t data)
{
  destroy_count++;
}

static void
test_param_qdata (void)
{
  xparam_spec_t *p;
  xchar_t *bla;
  xquark q;

  q = g_quark_from_string ("bla");

  p = xparam_spec_int ("my-int", "My Int", "Blurb", 0, 20, 10, XPARAM_READWRITE);
  xparam_spec_set_qdata (p, q, "bla");
  bla = xparam_spec_get_qdata (p, q);
  g_assert_cmpstr (bla, ==, "bla");

  g_assert_cmpint (destroy_count, ==, 0);
  xparam_spec_set_qdata_full (p, q, "bla", my_destroy);
  xparam_spec_set_qdata_full (p, q, "blabla", my_destroy);
  g_assert_cmpint (destroy_count, ==, 1);
  g_assert_cmpstr (xparam_spec_steal_qdata (p, q), ==, "blabla");
  g_assert_cmpint (destroy_count, ==, 1);
  g_assert_null (xparam_spec_get_qdata (p, q));

  xparam_spec_ref_sink (p);

  xparam_spec_unref (p);
}

static void
test_param_validate (void)
{
  xparam_spec_t *p;
  xvalue_t value = G_VALUE_INIT;

  p = xparam_spec_int ("my-int", "My Int", "Blurb", 0, 20, 10, XPARAM_READWRITE);

  xvalue_init (&value, XTYPE_INT);
  xvalue_set_int (&value, 100);
  g_assert_false (g_param_value_defaults (p, &value));
  g_assert_true (g_param_value_validate (p, &value));
  g_assert_cmpint (xvalue_get_int (&value), ==, 20);

  g_param_value_set_default (p, &value);
  g_assert_true (g_param_value_defaults (p, &value));
  g_assert_cmpint (xvalue_get_int (&value), ==, 10);

  xparam_spec_unref (p);
}

static void
test_param_strings (void)
{
  xparam_spec_t *p;

  /* test canonicalization */
  p = xparam_spec_int ("my_int", "My Int", "Blurb", 0, 20, 10, XPARAM_READWRITE);

  g_assert_cmpstr (xparam_spec_get_name (p), ==, "my-int");
  g_assert_cmpstr (xparam_spec_get_nick (p), ==, "My Int");
  g_assert_cmpstr (xparam_spec_get_blurb (p), ==, "Blurb");

  xparam_spec_unref (p);

  /* test nick defaults to name */
  p = xparam_spec_int ("my-int", NULL, NULL, 0, 20, 10, XPARAM_READWRITE);

  g_assert_cmpstr (xparam_spec_get_name (p), ==, "my-int");
  g_assert_cmpstr (xparam_spec_get_nick (p), ==, "my-int");
  g_assert_null (xparam_spec_get_blurb (p));

  xparam_spec_unref (p);
}

static void
test_param_invalid_name (xconstpointer test_data)
{
  const xchar_t *invalid_name = test_data;

  g_test_summary ("test_t that properties cannot be created with invalid names");

  if (g_test_subprocess ())
    {
      xparam_spec_t *p;
      p = xparam_spec_int (invalid_name, "My Int", "Blurb", 0, 20, 10, XPARAM_READWRITE);
      xparam_spec_unref (p);
      return;
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*xparam_spec_is_valid_name (name)*");
}

static void
test_param_convert (void)
{
  xparam_spec_t *p;
  xvalue_t v1 = G_VALUE_INIT;
  xvalue_t v2 = G_VALUE_INIT;

  p = xparam_spec_int ("my-int", "My Int", "Blurb", 0, 20, 10, XPARAM_READWRITE);
  xvalue_init (&v1, XTYPE_UINT);
  xvalue_set_uint (&v1, 43);

  xvalue_init (&v2, XTYPE_INT);
  xvalue_set_int (&v2, -4);

  g_assert_false (g_param_value_convert (p, &v1, &v2, TRUE));
  g_assert_cmpint (xvalue_get_int (&v2), ==, -4);

  g_assert_true (g_param_value_convert (p, &v1, &v2, FALSE));
  g_assert_cmpint (xvalue_get_int (&v2), ==, 20);

  xparam_spec_unref (p);
}

static void
test_value_transform (void)
{
  xvalue_t src = G_VALUE_INIT;
  xvalue_t dest = G_VALUE_INIT;

#define CHECK_INT_CONVERSION(type, getter, value)                       \
  g_assert_true (xvalue_type_transformable (XTYPE_INT, type));        \
  xvalue_init (&src, XTYPE_INT);                                      \
  xvalue_init (&dest, type);                                           \
  xvalue_set_int (&src, value);                                        \
  g_assert_true (xvalue_transform (&src, &dest));                      \
  g_assert_cmpint (xvalue_get_##getter (&dest), ==, value);            \
  xvalue_unset (&src);                                                 \
  xvalue_unset (&dest);

  /* Keep a check for an integer in the range of 0-127 so we're
   * still testing xvalue_get_char().  See
   * https://bugzilla.gnome.org/show_bug.cgi?id=659870
   * for why it is broken.
   */
  CHECK_INT_CONVERSION(XTYPE_CHAR, char, 124)

  CHECK_INT_CONVERSION(XTYPE_CHAR, schar, -124)
  CHECK_INT_CONVERSION(XTYPE_CHAR, schar, 124)
  CHECK_INT_CONVERSION(XTYPE_UCHAR, uchar, 0)
  CHECK_INT_CONVERSION(XTYPE_UCHAR, uchar, 255)
  CHECK_INT_CONVERSION(XTYPE_INT, int, -12345)
  CHECK_INT_CONVERSION(XTYPE_INT, int, 12345)
  CHECK_INT_CONVERSION(XTYPE_UINT, uint, 0)
  CHECK_INT_CONVERSION(XTYPE_UINT, uint, 12345)
  CHECK_INT_CONVERSION(XTYPE_LONG, long, -12345678)
  CHECK_INT_CONVERSION(XTYPE_ULONG, ulong, 12345678)
  CHECK_INT_CONVERSION(XTYPE_INT64, int64, -12345678)
  CHECK_INT_CONVERSION(XTYPE_UINT64, uint64, 12345678)
  CHECK_INT_CONVERSION(XTYPE_FLOAT, float, 12345678)
  CHECK_INT_CONVERSION(XTYPE_DOUBLE, double, 12345678)

#define CHECK_UINT_CONVERSION(type, getter, value)                      \
  g_assert_true (xvalue_type_transformable (XTYPE_UINT, type));       \
  xvalue_init (&src, XTYPE_UINT);                                     \
  xvalue_init (&dest, type);                                           \
  xvalue_set_uint (&src, value);                                       \
  g_assert_true (xvalue_transform (&src, &dest));                      \
  g_assert_cmpuint (xvalue_get_##getter (&dest), ==, value);           \
  xvalue_unset (&src);                                                 \
  xvalue_unset (&dest);

  CHECK_UINT_CONVERSION(XTYPE_CHAR, char, 124)
  CHECK_UINT_CONVERSION(XTYPE_CHAR, char, 124)
  CHECK_UINT_CONVERSION(XTYPE_UCHAR, uchar, 0)
  CHECK_UINT_CONVERSION(XTYPE_UCHAR, uchar, 255)
  CHECK_UINT_CONVERSION(XTYPE_INT, int, 12345)
  CHECK_UINT_CONVERSION(XTYPE_INT, int, 12345)
  CHECK_UINT_CONVERSION(XTYPE_UINT, uint, 0)
  CHECK_UINT_CONVERSION(XTYPE_UINT, uint, 12345)
  CHECK_UINT_CONVERSION(XTYPE_LONG, long, 12345678)
  CHECK_UINT_CONVERSION(XTYPE_ULONG, ulong, 12345678)
  CHECK_UINT_CONVERSION(XTYPE_INT64, int64, 12345678)
  CHECK_UINT_CONVERSION(XTYPE_UINT64, uint64, 12345678)
  CHECK_UINT_CONVERSION(XTYPE_FLOAT, float, 12345678)
  CHECK_UINT_CONVERSION(XTYPE_DOUBLE, double, 12345678)

#define CHECK_LONG_CONVERSION(type, getter, value)                      \
  g_assert_true (xvalue_type_transformable (XTYPE_LONG, type));       \
  xvalue_init (&src, XTYPE_LONG);                                     \
  xvalue_init (&dest, type);                                           \
  xvalue_set_long (&src, value);                                       \
  g_assert_true (xvalue_transform (&src, &dest));                      \
  g_assert_cmpint (xvalue_get_##getter (&dest), ==, value);            \
  xvalue_unset (&src);                                                 \
  xvalue_unset (&dest);

  CHECK_LONG_CONVERSION(XTYPE_CHAR, schar, -124)
  CHECK_LONG_CONVERSION(XTYPE_CHAR, schar, 124)
  CHECK_LONG_CONVERSION(XTYPE_UCHAR, uchar, 0)
  CHECK_LONG_CONVERSION(XTYPE_UCHAR, uchar, 255)
  CHECK_LONG_CONVERSION(XTYPE_INT, int, -12345)
  CHECK_LONG_CONVERSION(XTYPE_INT, int, 12345)
  CHECK_LONG_CONVERSION(XTYPE_UINT, uint, 0)
  CHECK_LONG_CONVERSION(XTYPE_UINT, uint, 12345)
  CHECK_LONG_CONVERSION(XTYPE_LONG, long, -12345678)
  CHECK_LONG_CONVERSION(XTYPE_ULONG, ulong, 12345678)
  CHECK_LONG_CONVERSION(XTYPE_INT64, int64, -12345678)
  CHECK_LONG_CONVERSION(XTYPE_UINT64, uint64, 12345678)
  CHECK_LONG_CONVERSION(XTYPE_FLOAT, float, 12345678)
  CHECK_LONG_CONVERSION(XTYPE_DOUBLE, double, 12345678)

#define CHECK_ULONG_CONVERSION(type, getter, value)                     \
  g_assert_true (xvalue_type_transformable (XTYPE_ULONG, type));      \
  xvalue_init (&src, XTYPE_ULONG);                                    \
  xvalue_init (&dest, type);                                           \
  xvalue_set_ulong (&src, value);                                      \
  g_assert_true (xvalue_transform (&src, &dest));                      \
  g_assert_cmpuint (xvalue_get_##getter (&dest), ==, value);           \
  xvalue_unset (&src);                                                 \
  xvalue_unset (&dest);

  CHECK_ULONG_CONVERSION(XTYPE_CHAR, char, 124)
  CHECK_ULONG_CONVERSION(XTYPE_CHAR, char, 124)
  CHECK_ULONG_CONVERSION(XTYPE_UCHAR, uchar, 0)
  CHECK_ULONG_CONVERSION(XTYPE_UCHAR, uchar, 255)
  CHECK_ULONG_CONVERSION(XTYPE_INT, int, -12345)
  CHECK_ULONG_CONVERSION(XTYPE_INT, int, 12345)
  CHECK_ULONG_CONVERSION(XTYPE_UINT, uint, 0)
  CHECK_ULONG_CONVERSION(XTYPE_UINT, uint, 12345)
  CHECK_ULONG_CONVERSION(XTYPE_LONG, long, 12345678)
  CHECK_ULONG_CONVERSION(XTYPE_ULONG, ulong, 12345678)
  CHECK_ULONG_CONVERSION(XTYPE_INT64, int64, 12345678)
  CHECK_ULONG_CONVERSION(XTYPE_UINT64, uint64, 12345678)
  CHECK_ULONG_CONVERSION(XTYPE_FLOAT, float, 12345678)
  CHECK_ULONG_CONVERSION(XTYPE_DOUBLE, double, 12345678)

#define CHECK_INT64_CONVERSION(type, getter, value)                     \
  g_assert_true (xvalue_type_transformable (XTYPE_INT64, type));      \
  xvalue_init (&src, XTYPE_INT64);                                    \
  xvalue_init (&dest, type);                                           \
  xvalue_set_int64 (&src, value);                                      \
  g_assert_true (xvalue_transform (&src, &dest));                      \
  g_assert_cmpint (xvalue_get_##getter (&dest), ==, value);            \
  xvalue_unset (&src);                                                 \
  xvalue_unset (&dest);

  CHECK_INT64_CONVERSION(XTYPE_CHAR, schar, -124)
  CHECK_INT64_CONVERSION(XTYPE_CHAR, schar, 124)
  CHECK_INT64_CONVERSION(XTYPE_UCHAR, uchar, 0)
  CHECK_INT64_CONVERSION(XTYPE_UCHAR, uchar, 255)
  CHECK_INT64_CONVERSION(XTYPE_INT, int, -12345)
  CHECK_INT64_CONVERSION(XTYPE_INT, int, 12345)
  CHECK_INT64_CONVERSION(XTYPE_UINT, uint, 0)
  CHECK_INT64_CONVERSION(XTYPE_UINT, uint, 12345)
  CHECK_INT64_CONVERSION(XTYPE_LONG, long, -12345678)
  CHECK_INT64_CONVERSION(XTYPE_ULONG, ulong, 12345678)
  CHECK_INT64_CONVERSION(XTYPE_INT64, int64, -12345678)
  CHECK_INT64_CONVERSION(XTYPE_UINT64, uint64, 12345678)
  CHECK_INT64_CONVERSION(XTYPE_FLOAT, float, 12345678)
  CHECK_INT64_CONVERSION(XTYPE_DOUBLE, double, 12345678)

#define CHECK_UINT64_CONVERSION(type, getter, value)                    \
  g_assert_true (xvalue_type_transformable (XTYPE_UINT64, type));     \
  xvalue_init (&src, XTYPE_UINT64);                                   \
  xvalue_init (&dest, type);                                           \
  xvalue_set_uint64 (&src, value);                                     \
  g_assert_true (xvalue_transform (&src, &dest));                      \
  g_assert_cmpuint (xvalue_get_##getter (&dest), ==, value);           \
  xvalue_unset (&src);                                                 \
  xvalue_unset (&dest);

  CHECK_UINT64_CONVERSION(XTYPE_CHAR, schar, -124)
  CHECK_UINT64_CONVERSION(XTYPE_CHAR, schar, 124)
  CHECK_UINT64_CONVERSION(XTYPE_UCHAR, uchar, 0)
  CHECK_UINT64_CONVERSION(XTYPE_UCHAR, uchar, 255)
  CHECK_UINT64_CONVERSION(XTYPE_INT, int, -12345)
  CHECK_UINT64_CONVERSION(XTYPE_INT, int, 12345)
  CHECK_UINT64_CONVERSION(XTYPE_UINT, uint, 0)
  CHECK_UINT64_CONVERSION(XTYPE_UINT, uint, 12345)
  CHECK_UINT64_CONVERSION(XTYPE_LONG, long, -12345678)
  CHECK_UINT64_CONVERSION(XTYPE_ULONG, ulong, 12345678)
  CHECK_UINT64_CONVERSION(XTYPE_INT64, int64, -12345678)
  CHECK_UINT64_CONVERSION(XTYPE_UINT64, uint64, 12345678)
  CHECK_UINT64_CONVERSION(XTYPE_FLOAT, float, 12345678)
  CHECK_UINT64_CONVERSION(XTYPE_DOUBLE, double, 12345678)

#define CHECK_FLOAT_CONVERSION(type, getter, value)                    \
  g_assert_true (xvalue_type_transformable (XTYPE_FLOAT, type));     \
  xvalue_init (&src, XTYPE_FLOAT);                                   \
  xvalue_init (&dest, type);                                          \
  xvalue_set_float (&src, value);                                     \
  g_assert_true (xvalue_transform (&src, &dest));                     \
  g_assert_cmpfloat (xvalue_get_##getter (&dest), ==, value);         \
  xvalue_unset (&src);                                                \
  xvalue_unset (&dest);

  CHECK_FLOAT_CONVERSION(XTYPE_CHAR, schar, -124)
  CHECK_FLOAT_CONVERSION(XTYPE_CHAR, schar, 124)
  CHECK_FLOAT_CONVERSION(XTYPE_UCHAR, uchar, 0)
  CHECK_FLOAT_CONVERSION(XTYPE_UCHAR, uchar, 255)
  CHECK_FLOAT_CONVERSION(XTYPE_INT, int, -12345)
  CHECK_FLOAT_CONVERSION(XTYPE_INT, int, 12345)
  CHECK_FLOAT_CONVERSION(XTYPE_UINT, uint, 0)
  CHECK_FLOAT_CONVERSION(XTYPE_UINT, uint, 12345)
  CHECK_FLOAT_CONVERSION(XTYPE_LONG, long, -12345678)
  CHECK_FLOAT_CONVERSION(XTYPE_ULONG, ulong, 12345678)
  CHECK_FLOAT_CONVERSION(XTYPE_INT64, int64, -12345678)
  CHECK_FLOAT_CONVERSION(XTYPE_UINT64, uint64, 12345678)
  CHECK_FLOAT_CONVERSION(XTYPE_FLOAT, float, 12345678)
  CHECK_FLOAT_CONVERSION(XTYPE_DOUBLE, double, 12345678)

#define CHECK_DOUBLE_CONVERSION(type, getter, value)                    \
  g_assert_true (xvalue_type_transformable (XTYPE_DOUBLE, type));     \
  xvalue_init (&src, XTYPE_DOUBLE);                                   \
  xvalue_init (&dest, type);                                           \
  xvalue_set_double (&src, value);                                     \
  g_assert_true (xvalue_transform (&src, &dest));                      \
  g_assert_cmpfloat (xvalue_get_##getter (&dest), ==, value);          \
  xvalue_unset (&src);                                                 \
  xvalue_unset (&dest);

  CHECK_DOUBLE_CONVERSION(XTYPE_CHAR, schar, -124)
  CHECK_DOUBLE_CONVERSION(XTYPE_CHAR, schar, 124)
  CHECK_DOUBLE_CONVERSION(XTYPE_UCHAR, uchar, 0)
  CHECK_DOUBLE_CONVERSION(XTYPE_UCHAR, uchar, 255)
  CHECK_DOUBLE_CONVERSION(XTYPE_INT, int, -12345)
  CHECK_DOUBLE_CONVERSION(XTYPE_INT, int, 12345)
  CHECK_DOUBLE_CONVERSION(XTYPE_UINT, uint, 0)
  CHECK_DOUBLE_CONVERSION(XTYPE_UINT, uint, 12345)
  CHECK_DOUBLE_CONVERSION(XTYPE_LONG, long, -12345678)
  CHECK_DOUBLE_CONVERSION(XTYPE_ULONG, ulong, 12345678)
  CHECK_DOUBLE_CONVERSION(XTYPE_INT64, int64, -12345678)
  CHECK_DOUBLE_CONVERSION(XTYPE_UINT64, uint64, 12345678)
  CHECK_DOUBLE_CONVERSION(XTYPE_FLOAT, float, 12345678)
  CHECK_DOUBLE_CONVERSION(XTYPE_DOUBLE, double, 12345678)

#define CHECK_BOOLEAN_CONVERSION(type, setter, value)                   \
  g_assert_true (xvalue_type_transformable (type, XTYPE_BOOLEAN));    \
  xvalue_init (&src, type);                                            \
  xvalue_init (&dest, XTYPE_BOOLEAN);                                 \
  xvalue_set_##setter (&src, value);                                   \
  g_assert_true (xvalue_transform (&src, &dest));                      \
  g_assert_cmpint (xvalue_get_boolean (&dest), ==, TRUE);              \
  xvalue_set_##setter (&src, 0);                                       \
  g_assert_true (xvalue_transform (&src, &dest));                      \
  g_assert_cmpint (xvalue_get_boolean (&dest), ==, FALSE);             \
  xvalue_unset (&src);                                                 \
  xvalue_unset (&dest);

  CHECK_BOOLEAN_CONVERSION(XTYPE_INT, int, -12345)
  CHECK_BOOLEAN_CONVERSION(XTYPE_UINT, uint, 12345)
  CHECK_BOOLEAN_CONVERSION(XTYPE_LONG, long, -12345678)
  CHECK_BOOLEAN_CONVERSION(XTYPE_ULONG, ulong, 12345678)
  CHECK_BOOLEAN_CONVERSION(XTYPE_INT64, int64, -12345678)
  CHECK_BOOLEAN_CONVERSION(XTYPE_UINT64, uint64, 12345678)

#define CHECK_STRING_CONVERSION(int_type, setter, int_value)            \
  g_assert_true (xvalue_type_transformable (int_type, XTYPE_STRING)); \
  xvalue_init (&src, int_type);                                        \
  xvalue_init (&dest, XTYPE_STRING);                                  \
  xvalue_set_##setter (&src, int_value);                               \
  g_assert_true (xvalue_transform (&src, &dest));                      \
  g_assert_cmpstr (xvalue_get_string (&dest), ==, #int_value);         \
  xvalue_unset (&src);                                                 \
  xvalue_unset (&dest);

  CHECK_STRING_CONVERSION(XTYPE_INT, int, -12345)
  CHECK_STRING_CONVERSION(XTYPE_UINT, uint, 12345)
  CHECK_STRING_CONVERSION(XTYPE_LONG, long, -12345678)
  CHECK_STRING_CONVERSION(XTYPE_ULONG, ulong, 12345678)
  CHECK_STRING_CONVERSION(XTYPE_INT64, int64, -12345678)
  CHECK_STRING_CONVERSION(XTYPE_UINT64, uint64, 12345678)
  CHECK_STRING_CONVERSION(XTYPE_FLOAT, float, 0.500000)
  CHECK_STRING_CONVERSION(XTYPE_DOUBLE, double, -1.234567)

  g_assert_false (xvalue_type_transformable (XTYPE_STRING, XTYPE_CHAR));
  xvalue_init (&src, XTYPE_STRING);
  xvalue_init (&dest, XTYPE_CHAR);
  xvalue_set_static_string (&src, "bla");
  xvalue_set_schar (&dest, 'c');
  g_assert_false (xvalue_transform (&src, &dest));
  g_assert_cmpint (xvalue_get_schar (&dest), ==, 'c');
  xvalue_unset (&src);
  xvalue_unset (&dest);
}


/* We create some dummy objects with a simple relationship:
 *
 *           xobject_t
 *          /       \
 * test_object_a_t     test_object_c_t
 *      |
 * test_object_b_t
 *
 * ie: test_object_b_t is a subclass of test_object_a_t and test_object_c_t is
 * related to neither.
 */

static xtype_t test_object_a_get_type (void);
typedef xobject_t test_object_a_t; typedef xobject_class_t test_object_a_class_t;
XDEFINE_TYPE (test_object_a, test_object_a, XTYPE_OBJECT)
static void test_object_a_class_init (test_object_a_class_t *class) { }
static void test_object_a_init (test_object_a_t *a) { }

static xtype_t test_object_b_get_type (void);
typedef xobject_t test_object_b_t; typedef xobject_class_t test_object_b_class_t;
XDEFINE_TYPE (test_object_b, test_object_b, test_object_a_get_type ())
static void test_object_b_class_init (test_object_b_class_t *class) { }
static void test_object_b_init (test_object_b_t *b) { }

static xtype_t test_object_c_get_type (void);
typedef xobject_t test_object_c_t; typedef xobject_class_t test_object_c_class_t;
XDEFINE_TYPE (test_object_c, test_object_c, XTYPE_OBJECT)
static void test_object_c_class_init (test_object_c_class_t *class) { }
static void test_object_c_init (test_object_c_t *c) { }

/* We create an interface and programmatically populate it with
 * properties of each of the above type, with various flag combinations.
 *
 * Properties are named like "type-perm" where type is 'a', 'b' or 'c'
 * and perm is a series of characters, indicating the permissions:
 *
 *   - 'r': readable
 *   - 'w': writable
 *   - 'c': construct
 *   - 'C': construct-only
 *
 * It doesn't make sense to have a property that is neither readable nor
 * writable.  It is also not valid to have construct or construct-only
 * on read-only params.  Finally, it is invalid to have both construct
 * and construct-only specified, so we do not consider those cases.
 * That gives us 7 possible permissions:
 *
 *     'r', 'w', 'rw', 'wc', 'rwc', 'wC', 'rwC'
 *
 * And 9 impossible ones:
 *
 *     '', 'c', 'rc', 'C', 'rC', 'cC', 'rcC', 'wcC', rwcC'
 *
 * For a total of 16 combinations.
 *
 * That gives a total of 48 (16 * 3) possible flag/type combinations, of
 * which 27 (9 * 3) are impossible to install.
 *
 * That gives 21 (7 * 3) properties that will be installed.
 */
typedef xtype_interface_t test_interface_interface_t;
static xtype_t test_interface_get_type (void);
//typedef struct _TestInterface test_interface_t;
G_DEFINE_INTERFACE (test_interface, test_interface, XTYPE_OBJECT)
static void
test_interface_default_init (test_interface_interface_t *iface)
{
  const xchar_t *names[] = { "a", "b", "c" };
  const xchar_t *perms[] = { NULL, "r",  "w",  "rw",
                           NULL, NULL, "wc", "rwc",
                           NULL, NULL, "wC", "rwC",
                           NULL, NULL, NULL, NULL };
  const xtype_t types[] = { test_object_a_get_type (), test_object_b_get_type (), test_object_c_get_type () };
  xuint_t i, j;

  for (i = 0; i < G_N_ELEMENTS (types); i++)
    for (j = 0; j < G_N_ELEMENTS (perms); j++)
      {
        xchar_t prop_name[10];
        xparam_spec_t *pspec;

        if (perms[j] == NULL)
          {
            if (!g_test_undefined ())
              continue;

            /* we think that this is impossible.  make sure. */
            pspec = xparam_spec_object ("xyz", "xyz", "xyz", types[i], j);

            g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                                   "*assertion*pspec->flags*failed*");
            xobject_interface_install_property (iface, pspec);
            g_test_assert_expected_messages ();

            xparam_spec_unref (pspec);
            continue;
          }

        /* install the property */
        g_snprintf (prop_name, sizeof prop_name, "%s-%s", names[i], perms[j]);
        pspec = xparam_spec_object (prop_name, prop_name, prop_name, types[i], j);
        xobject_interface_install_property (iface, pspec);
      }
}

/* We now have 21 properties.  Each property may be correctly
 * implemented with the following types:
 *
 *   Properties         Valid Types       Reason
 *
 *   a-r                a, b              Read only can provide subclasses
 *   a-w, wc, wC        a, xobject_t        Write only can accept superclasses
 *   a-rw, rwc, rwC     a                 Read-write must be exactly equal
 *
 *   b-r                b                 (as above)
 *   b-w, wc, wC        b, a, xobject_t
 *   b-rw, rwc, rwC     b
 *
 *   c-r                c                 (as above)
 *   c-wo, wc, wC       c, xobject_t
 *   c-rw, rwc, rwC     c
 *
 * We can express this in a 48-by-4 table where each row represents an
 * installed property and each column represents a type.  The value in
 * the table represents if it is valid to subclass the row's property
 * with the type of the column:
 *
 *   - 0:   invalid because the interface property doesn't exist (invalid flags)
 *   - 'v': valid
 *   - '=': invalid because of the type not being exactly equal
 *   - '<': invalid because of the type not being a subclass
 *   - '>': invalid because of the type not being a superclass
 *
 * We organise the table by interface property type ('a', 'b', 'c') then
 * by interface property flags.
 */

static xint_t valid_impl_types[48][4] = {
                    /* a    b    c    xobject_t */
    /* 'a-' */       { 0, },
    /* 'a-r' */      { 'v', 'v', '<', '<' },
    /* 'a-w' */      { 'v', '>', '>', 'v' },
    /* 'a-rw' */     { 'v', '=', '=', '=' },
    /* 'a-c */       { 0, },
    /* 'a-rc' */     { 0, },
    /* 'a-wc' */     { 'v', '>', '>', 'v' },
    /* 'a-rwc' */    { 'v', '=', '=', '=' },
    /* 'a-C */       { 0, },
    /* 'a-rC' */     { 0, },
    /* 'a-wC' */     { 'v', '>', '>', 'v' },
    /* 'a-rwC' */    { 'v', '=', '=', '=' },
    /* 'a-cC */      { 0, },
    /* 'a-rcC' */    { 0, },
    /* 'a-wcC' */    { 0, },
    /* 'a-rwcC' */   { 0, },

    /* 'b-' */       { 0, },
    /* 'b-r' */      { '<', 'v', '<', '<' },
    /* 'b-w' */      { 'v', 'v', '>', 'v' },
    /* 'b-rw' */     { '=', 'v', '=', '=' },
    /* 'b-c */       { 0, },
    /* 'b-rc' */     { 0, },
    /* 'b-wc' */     { 'v', 'v', '>', 'v' },
    /* 'b-rwc' */    { '=', 'v', '=', '=' },
    /* 'b-C */       { 0, },
    /* 'b-rC' */     { 0, },
    /* 'b-wC' */     { 'v', 'v', '>', 'v' },
    /* 'b-rwC' */    { '=', 'v', '=', '=' },
    /* 'b-cC */      { 0, },
    /* 'b-rcC' */    { 0, },
    /* 'b-wcC' */    { 0, },
    /* 'b-rwcC' */   { 0, },

    /* 'c-' */       { 0, },
    /* 'c-r' */      { '<', '<', 'v', '<' },
    /* 'c-w' */      { '>', '>', 'v', 'v' },
    /* 'c-rw' */     { '=', '=', 'v', '=' },
    /* 'c-c */       { 0, },
    /* 'c-rc' */     { 0, },
    /* 'c-wc' */     { '>', '>', 'v', 'v' },
    /* 'c-rwc' */    { '=', '=', 'v', '=' },
    /* 'c-C */       { 0, },
    /* 'c-rC' */     { 0, },
    /* 'c-wC' */     { '>', '>', 'v', 'v' },
    /* 'c-rwC' */    { '=', '=', 'v', '=' },
    /* 'c-cC */      { 0, },
    /* 'c-rcC' */    { 0, },
    /* 'c-wcC' */    { 0, },
    /* 'c-rwcC' */   { 0, }
};

/* We also try to change the flags.  We must ensure that all
 * implementations provide all functionality promised by the interface.
 * We must therefore never remove readability or writability (but we can
 * add them).  Construct-only is a restrictions that applies to
 * writability, so we can never add it unless writability was never
 * present in the first place, in which case "writable at construct
 * only" is still better than "not writable".
 *
 * The 'construct' flag is of interest only to the implementation.  It
 * may be changed at any time.
 *
 *   Properties         Valid Access      Reason
 *
 *   *-r                r, rw, rwc, rwC   Must keep readable, but can restrict newly-added writable
 *   *-w                w, rw, rwc        Must keep writable unrestricted
 *   *-rw               rw, rwc           Must not add any restrictions
 *   *-rwc              rw, rwc           Must not add any restrictions
 *   *-rwC              rw, rwc, rwC      Can remove 'construct-only' restriction
 *   *-wc               rwc, rw, w, wc    Can add readability
 *   *-wC               rwC, rw, w, wC    Can add readability or remove 'construct only' restriction
 *                        rwc, wc
 *
 * We can represent this with a 16-by-16 table.  The rows represent the
 * flags of the property on the interface.  The columns is the flags to
 * try to use when overriding the property.  The cell contents are:
 *
 *   - 0:   invalid because the interface property doesn't exist (invalid flags)
 *   - 'v': valid
 *   - 'i': invalid because the implementation flags are invalid
 *   - 'f': invalid because of the removal of functionality
 *   - 'r': invalid because of the addition of restrictions (ie: construct-only)
 *
 * We also ensure that removal of functionality is reported before
 * addition of restrictions, since this is a more basic problem.
 */
static xint_t valid_impl_flags[16][16] = {
                 /* ''   r    w    rw   c    rc   wc   rwc  C    rC   wC   rwC  cC   rcC  wcC  rwcC */
    /* '*-' */    { 0, },
    /* '*-r' */   { 'i', 'v', 'f', 'v', 'i', 'i', 'f', 'v', 'i', 'i', 'f', 'v', 'i', 'i', 'i', 'i' },
    /* '*-w' */   { 'i', 'f', 'v', 'v', 'i', 'i', 'v', 'v', 'i', 'i', 'r', 'r', 'i', 'i', 'i', 'i' },
    /* '*-rw' */  { 'i', 'f', 'f', 'v', 'i', 'i', 'f', 'v', 'i', 'i', 'f', 'r', 'i', 'i', 'i', 'i' },
    /* '*-c */    { 0, },
    /* '*-rc' */  { 0, },
    /* '*-wc' */  { 'i', 'f', 'v', 'v', 'i', 'i', 'v', 'v', 'i', 'i', 'r', 'r', 'i', 'i', 'i', 'i' },
    /* '*-rwc' */ { 'i', 'f', 'f', 'v', 'i', 'i', 'f', 'v', 'i', 'i', 'f', 'r', 'i', 'i', 'i', 'i' },
    /* '*-C */    { 0, },
    /* '*-rC' */  { 0, },
    /* '*-wC' */  { 'i', 'f', 'v', 'v', 'i', 'i', 'v', 'v', 'i', 'i', 'v', 'v', 'i', 'i', 'i', 'i' },
    /* '*-rwC' */ { 'i', 'f', 'f', 'v', 'i', 'i', 'f', 'v', 'i', 'i', 'f', 'v', 'i', 'i', 'i', 'i' },
};

static xuint_t change_this_flag;
static xuint_t change_this_type;
static xuint_t use_this_flag;
static xuint_t use_this_type;

typedef xobject_class_t test_implementation_class_t;
typedef xobject_t test_implementation_t;

static void test_implementation_init (test_implementation_t *impl) { }
static void test_implementation_iface_init (test_interface_interface_t *iface) { }

static xtype_t test_implementation_get_type (void);
G_DEFINE_TYPE_WITH_CODE (test_implementation, test_implementation, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (test_interface_get_type (), test_implementation_iface_init))

static void test_implementation_class_init (test_implementation_class_t *class)
{
  const xchar_t *names[] = { "a", "b", "c" };
  const xchar_t *perms[] = { NULL, "r",  "w",  "rw",
                           NULL, NULL, "wc", "rwc",
                           NULL, NULL, "wC", "rwC",
                           NULL, NULL, NULL, NULL };
  const xtype_t types[] = { test_object_a_get_type (), test_object_b_get_type (),
                          test_object_c_get_type (), XTYPE_OBJECT };
  xchar_t prop_name[10];
  xparam_spec_t *pspec;
  xuint_t i, j;

  class->get_property = GINT_TO_POINTER (1);
  class->set_property = GINT_TO_POINTER (1);

  /* Install all of the non-modified properties or else xobject_t will
   * complain about non-implemented properties.
   */
  for (i = 0; i < 3; i++)
    for (j = 0; j < G_N_ELEMENTS (perms); j++)
      {
        if (i == change_this_type && j == change_this_flag)
          continue;

        if (perms[j] != NULL)
          {
            /* override the property without making changes */
            g_snprintf (prop_name, sizeof prop_name, "%s-%s", names[i], perms[j]);
            xobject_class_override_property (class, 1, prop_name);
          }
      }

  /* Now try installing our modified property */
  if (perms[change_this_flag] == NULL)
    xerror ("Interface property does not exist");

  g_snprintf (prop_name, sizeof prop_name, "%s-%s", names[change_this_type], perms[change_this_flag]);
  pspec = xparam_spec_object (prop_name, prop_name, prop_name, types[use_this_type], use_this_flag);
  xobject_class_install_property (class, 1, pspec);
}

typedef struct {
  xint_t change_this_flag;
  xint_t change_this_type;
  xint_t use_this_flag;
  xint_t use_this_type;
} TestParamImplementData;

static void
test_param_implement_child (xconstpointer user_data)
{
  TestParamImplementData *data = (xpointer_t) user_data;

  /* xobject_t oddity: xobject_class_t must be initialised before we can
   * initialise a xtype_interface_t.
   */
  xtype_class_ref (XTYPE_OBJECT);

  /* Bring up the interface first. */
  xtype_default_interface_ref (test_interface_get_type ());

  /* Copy the flags into the global vars so
   * test_implementation_class_init() will see them.
   */
  change_this_flag = data->change_this_flag;
  change_this_type = data->change_this_type;
  use_this_flag = data->use_this_flag;
  use_this_type = data->use_this_type;

  xtype_class_ref (test_implementation_get_type ());
}

static void
test_param_implement (void)
{
  xchar_t *test_path;

  for (change_this_flag = 0; change_this_flag < 16; change_this_flag++)
    for (change_this_type = 0; change_this_type < 3; change_this_type++)
      for (use_this_flag = 0; use_this_flag < 16; use_this_flag++)
        for (use_this_type = 0; use_this_type < 4; use_this_type++)
          {
            if (!g_test_undefined ())
              {
                /* only test the valid (defined) cases, e.g. under valgrind */
                if (valid_impl_flags[change_this_flag][use_this_flag] != 'v')
                  continue;

                if (valid_impl_types[change_this_type * 16 + change_this_flag][use_this_type] != 'v')
                  continue;
              }

            test_path = xstrdup_printf ("/param/implement/subprocess/%d-%d-%d-%d",
                                         change_this_flag, change_this_type,
                                         use_this_flag, use_this_type);
            g_test_trap_subprocess (test_path, G_TIME_SPAN_SECOND, 0);
            g_free (test_path);

            /* We want to ensure that any flags mismatch problems are reported first. */
            switch (valid_impl_flags[change_this_flag][use_this_flag])
              {
              case 0:
                /* make sure the other table agrees */
                g_assert_cmpint (valid_impl_types[change_this_type * 16 + change_this_flag][use_this_type], ==, 0);
                g_test_trap_assert_failed ();
                g_test_trap_assert_stderr ("*Interface property does not exist*");
                continue;

              case 'i':
                g_test_trap_assert_failed ();
                g_test_trap_assert_stderr ("*xobject_class_install_property*");
                continue;

              case 'f':
                g_test_trap_assert_failed ();
                g_test_trap_assert_stderr ("*remove functionality*");
                continue;

              case 'r':
                g_test_trap_assert_failed ();
                g_test_trap_assert_stderr ("*introduce additional restrictions*");
                continue;

              case 'v':
                break;
              }

            /* Next, we check if there should have been a type error. */
            switch (valid_impl_types[change_this_type * 16 + change_this_flag][use_this_type])
              {
              case 0:
                /* this should have been caught above */
                g_assert_not_reached ();

              case '=':
                g_test_trap_assert_failed ();
                g_test_trap_assert_stderr ("*exactly equal*");
                continue;

              case '<':
                g_test_trap_assert_failed ();
                g_test_trap_assert_stderr ("*equal to or more restrictive*");
                continue;

              case '>':
                g_test_trap_assert_failed ();
                g_test_trap_assert_stderr ("*equal to or less restrictive*");
                continue;

              case 'v':
                break;
              }

            g_test_trap_assert_passed ();
          }
}

static void
test_param_default (void)
{
  xparam_spec_t *param;
  const xvalue_t *def;

  param = xparam_spec_int ("my-int", "My Int", "Blurb", 0, 20, 10, XPARAM_READWRITE);
  def = xparam_spec_get_default_value (param);

  g_assert_true (G_VALUE_HOLDS (def, XTYPE_INT));
  g_assert_cmpint (xvalue_get_int (def), ==, 10);

  xparam_spec_unref (param);
}

static void
test_param_is_valid_name (void)
{
  const xchar_t *valid_names[] =
    {
      "property",
      "i",
      "multiple-segments",
      "segment0-SEGMENT1",
      "using_underscores",
    };
  const xchar_t *invalid_names[] =
    {
      "",
      "7zip",
      "my_int:hello",
    };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (valid_names); i++)
    g_assert_true (xparam_spec_is_valid_name (valid_names[i]));

  for (i = 0; i < G_N_ELEMENTS (invalid_names); i++)
    g_assert_false (xparam_spec_is_valid_name (invalid_names[i]));
}

int
main (int argc, char *argv[])
{
  TestParamImplementData data, *test_data;
  xchar_t *test_path;

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/param/value", test_param_value);
  g_test_add_func ("/param/strings", test_param_strings);
  g_test_add_data_func ("/param/invalid-name/colon", "my_int:hello", test_param_invalid_name);
  g_test_add_data_func ("/param/invalid-name/first-char", "7zip", test_param_invalid_name);
  g_test_add_data_func ("/param/invalid-name/empty", "", test_param_invalid_name);
  g_test_add_func ("/param/qdata", test_param_qdata);
  g_test_add_func ("/param/validate", test_param_validate);
  g_test_add_func ("/param/convert", test_param_convert);

  if (g_test_slow ())
    g_test_add_func ("/param/implement", test_param_implement);

  for (data.change_this_flag = 0; data.change_this_flag < 16; data.change_this_flag++)
    for (data.change_this_type = 0; data.change_this_type < 3; data.change_this_type++)
      for (data.use_this_flag = 0; data.use_this_flag < 16; data.use_this_flag++)
        for (data.use_this_type = 0; data.use_this_type < 4; data.use_this_type++)
          {
            test_path = xstrdup_printf ("/param/implement/subprocess/%d-%d-%d-%d",
                                         data.change_this_flag, data.change_this_type,
                                         data.use_this_flag, data.use_this_type);
            test_data = g_memdup2 (&data, sizeof (TestParamImplementData));
            g_test_add_data_func_full (test_path, test_data, test_param_implement_child, g_free);
            g_free (test_data);
            g_free (test_path);
          }

  g_test_add_func ("/value/transform", test_value_transform);
  g_test_add_func ("/param/default", test_param_default);
  g_test_add_func ("/param/is-valid-name", test_param_is_valid_name);
  g_test_add_func ("/paramspec/char", test_param_spec_char);
  g_test_add_func ("/paramspec/string", test_param_spec_string);
  g_test_add_func ("/paramspec/override", test_param_spec_override);
  g_test_add_func ("/paramspec/gtype", test_param_spec_gtype);
  g_test_add_func ("/paramspec/variant", test_param_spec_variant);
  g_test_add_func ("/paramspec/variant/cmp", test_param_spec_variant_cmp);

  return g_test_run ();
}
