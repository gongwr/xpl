/*
 * Copyright © 2010 Codethink Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include <glib/gvariant-internal.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#define BASIC "bynqiuxthdsog?"
#define N_BASIC (G_N_ELEMENTS (BASIC) - 1)

#define INVALIDS "cefjklpwz&@^$"
#define N_INVALIDS (G_N_ELEMENTS (INVALIDS) - 1)

/* see comment in gvariant-serialiser.c about this madness.
 *
 * we use this to get testing of non-strictly-aligned xvariant_t instances
 * on machines that can tolerate it.  it is necessary to support this
 * because some systems have malloc() that returns non-8-aligned
 * pointers.  it is necessary to have special support in the tests
 * because on most machines malloc() is 8-aligned.
 */
#define ALIGN_BITS (sizeof (struct { char a; union {                       \
                      xuint64_t x; void *y; xdouble_t z; } b; }) - 9)

static xboolean_t
randomly (xdouble_t prob)
{
  return g_test_rand_double_range (0, 1) < prob;
}

/* corecursion */
static xvariant_type_t *
append_tuple_type_string (xstring_t *, xstring_t *, xboolean_t, xint_t);

/* append a random xvariant_type_t to a xstring_t
 * append a description of the type to another xstring_t
 * return what the type is
 */
static xvariant_type_t *
append_type_string (xstring_t  *string,
                    xstring_t  *description,
                    xboolean_t  definite,
                    xint_t      depth)
{
  if (!depth-- || randomly (0.3))
    {
      xchar_t b = BASIC[g_test_rand_int_range (0, N_BASIC - definite)];
      xstring_append_c (string, b);
      xstring_append_c (description, b);

      switch (b)
        {
        case 'b':
          return xvariant_type_copy (G_VARIANT_TYPE_BOOLEAN);
        case 'y':
          return xvariant_type_copy (G_VARIANT_TYPE_BYTE);
        case 'n':
          return xvariant_type_copy (G_VARIANT_TYPE_INT16);
        case 'q':
          return xvariant_type_copy (G_VARIANT_TYPE_UINT16);
        case 'i':
          return xvariant_type_copy (G_VARIANT_TYPE_INT32);
        case 'u':
          return xvariant_type_copy (G_VARIANT_TYPE_UINT32);
        case 'x':
          return xvariant_type_copy (G_VARIANT_TYPE_INT64);
        case 't':
          return xvariant_type_copy (G_VARIANT_TYPE_UINT64);
        case 'h':
          return xvariant_type_copy (G_VARIANT_TYPE_HANDLE);
        case 'd':
          return xvariant_type_copy (G_VARIANT_TYPE_DOUBLE);
        case 's':
          return xvariant_type_copy (G_VARIANT_TYPE_STRING);
        case 'o':
          return xvariant_type_copy (G_VARIANT_TYPE_OBJECT_PATH);
        case 'g':
          return xvariant_type_copy (G_VARIANT_TYPE_SIGNATURE);
        case '?':
          return xvariant_type_copy (G_VARIANT_TYPE_BASIC);
        default:
          g_assert_not_reached ();
        }
    }
  else
    {
      xvariant_type_t *result;

      switch (g_test_rand_int_range (0, definite ? 5 : 7))
        {
        case 0:
          {
            xvariant_type_t *element;

            xstring_append_c (string, 'a');
            xstring_append (description, "a of ");
            element = append_type_string (string, description,
                                          definite, depth);
            result = xvariant_type_new_array (element);
            xvariant_type_free (element);
          }

          g_assert_true (xvariant_type_is_array (result));
          break;

        case 1:
          {
            xvariant_type_t *element;

            xstring_append_c (string, 'm');
            xstring_append (description, "m of ");
            element = append_type_string (string, description,
                                          definite, depth);
            result = xvariant_type_new_maybe (element);
            xvariant_type_free (element);
          }

          g_assert_true (xvariant_type_is_maybe (result));
          break;

        case 2:
          result = append_tuple_type_string (string, description,
                                             definite, depth);

          g_assert_true (xvariant_type_is_tuple (result));
          break;

        case 3:
          {
            xvariant_type_t *key, *value;

            xstring_append_c (string, '{');
            xstring_append (description, "e of [");
            key = append_type_string (string, description, definite, 0);
            xstring_append (description, ", ");
            value = append_type_string (string, description, definite, depth);
            xstring_append_c (description, ']');
            xstring_append_c (string, '}');
            result = xvariant_type_new_dict_entry (key, value);
            xvariant_type_free (key);
            xvariant_type_free (value);
          }

          g_assert_true (xvariant_type_is_dict_entry (result));
          break;

        case 4:
          xstring_append_c (string, 'v');
          xstring_append_c (description, 'V');
          result = xvariant_type_copy (G_VARIANT_TYPE_VARIANT);
          g_assert_true (xvariant_type_equal (result, G_VARIANT_TYPE_VARIANT));
          break;

        case 5:
          xstring_append_c (string, '*');
          xstring_append_c (description, 'S');
          result = xvariant_type_copy (G_VARIANT_TYPE_ANY);
          g_assert_true (xvariant_type_equal (result, G_VARIANT_TYPE_ANY));
          break;

        case 6:
          xstring_append_c (string, 'r');
          xstring_append_c (description, 'R');
          result = xvariant_type_copy (G_VARIANT_TYPE_TUPLE);
          g_assert_true (xvariant_type_is_tuple (result));
          break;

        default:
          g_assert_not_reached ();
        }

      return result;
    }
}

static xvariant_type_t *
append_tuple_type_string (xstring_t  *string,
                          xstring_t  *description,
                          xboolean_t  definite,
                          xint_t      depth)
{
  xvariant_type_t *result, *other_result;
  xvariant_type_t **types;
  xsize_t i, size;

  xstring_append_c (string, '(');
  xstring_append (description, "t of [");

  size = g_test_rand_int_range (0, 20);
  types = g_new (xvariant_type_t *, size + 1);

  for (i = 0; i < size; i++)
    {
      types[i] = append_type_string (string, description, definite, depth);

      if (i < size - 1)
        xstring_append (description, ", ");
    }

  types[i] = NULL;

  xstring_append_c (description, ']');
  xstring_append_c (string, ')');

  result = xvariant_type_new_tuple ((xpointer_t) types, size);
  other_result = xvariant_type_new_tuple ((xpointer_t) types, -1);
  g_assert_true (xvariant_type_equal (result, other_result));
  xvariant_type_free (other_result);
  for (i = 0; i < size; i++)
    xvariant_type_free (types[i]);
  g_free (types);

  return result;
}

/* given a valid type string, make it invalid */
static xchar_t *
invalid_mutation (const xchar_t *type_string)
{
  xboolean_t have_parens, have_braces;

  /* it's valid, so '(' implies ')' and same for '{' and '}' */
  have_parens = strchr (type_string, '(') != NULL;
  have_braces = strchr (type_string, '{') != NULL;

  if (have_parens && have_braces && randomly (0.3))
    {
      /* swap a paren and a brace */
      xchar_t *pp, *bp;
      xint_t np, nb;
      xchar_t p, b;
      xchar_t *new;

      new = xstrdup (type_string);

      if (randomly (0.5))
        p = '(', b = '{';
      else
        p = ')', b = '}';

      np = nb = 0;
      pp = bp = new - 1;

      /* count number of parens/braces */
      while ((pp = strchr (pp + 1, p))) np++;
      while ((bp = strchr (bp + 1, b))) nb++;

      /* randomly pick one of each */
      np = g_test_rand_int_range (0, np) + 1;
      nb = g_test_rand_int_range (0, nb) + 1;

      /* find it */
      pp = bp = new - 1;
      while (np--) pp = strchr (pp + 1, p);
      while (nb--) bp = strchr (bp + 1, b);

      /* swap */
      g_assert_true (*bp == b && *pp == p);
      *bp = p;
      *pp = b;

      return new;
    }

  if ((have_parens || have_braces) && randomly (0.3))
    {
      /* drop a paren/brace */
      xchar_t *new;
      xchar_t *pp;
      xint_t np;
      xchar_t p;

      if (have_parens)
        if (randomly (0.5)) p = '('; else p = ')';
      else
        if (randomly (0.5)) p = '{'; else p = '}';

      new = xstrdup (type_string);

      np = 0;
      pp = new - 1;
      while ((pp = strchr (pp + 1, p))) np++;
      np = g_test_rand_int_range (0, np) + 1;
      pp = new - 1;
      while (np--) pp = strchr (pp + 1, p);
      g_assert_cmpint (*pp, ==, p);

      while (*pp)
        {
          *pp = *(pp + 1);
          pp++;
        }

      return new;
    }

  /* else, perform a random mutation at a random point */
  {
    xint_t length, n;
    xchar_t *new;
    xchar_t p;

    if (randomly (0.3))
      {
        /* insert a paren/brace */
        if (randomly (0.5))
          if (randomly (0.5)) p = '('; else p = ')';
        else
          if (randomly (0.5)) p = '{'; else p = '}';
      }
    else if (randomly (0.5))
      {
        /* insert junk */
        p = INVALIDS[g_test_rand_int_range (0, N_INVALIDS)];
      }
    else
      {
        /* truncate */
        p = '\0';
      }


    length = strlen (type_string);
    new = g_malloc (length + 2);
    n = g_test_rand_int_range (0, length);
    memcpy (new, type_string, n);
    new[n] = p;
    memcpy (new + n + 1, type_string + n, length - n);
    new[length + 1] = '\0';

    return new;
  }
}

/* describe a type using the same language as is generated
 * while generating the type with append_type_string
 */
static xchar_t *
describe_type (const xvariant_type_t *type)
{
  xchar_t *result;

  if (xvariant_type_is_container (type))
    {
      g_assert_false (xvariant_type_is_basic (type));

      if (xvariant_type_is_array (type))
        {
          xchar_t *subtype = describe_type (xvariant_type_element (type));
          result = xstrdup_printf ("a of %s", subtype);
          g_free (subtype);
        }
      else if (xvariant_type_is_maybe (type))
        {
          xchar_t *subtype = describe_type (xvariant_type_element (type));
          result = xstrdup_printf ("m of %s", subtype);
          g_free (subtype);
        }
      else if (xvariant_type_is_tuple (type))
        {
          if (!xvariant_type_equal (type, G_VARIANT_TYPE_TUPLE))
            {
              const xvariant_type_t *sub;
              xstring_t *string;
              xsize_t i, length;

              string = xstring_new ("t of [");

              length = xvariant_type_n_items (type);
              sub = xvariant_type_first (type);
              for (i = 0; i < length; i++)
                {
                  xchar_t *subtype = describe_type (sub);
                  xstring_append (string, subtype);
                  g_free (subtype);

                  if ((sub = xvariant_type_next (sub)))
                    xstring_append (string, ", ");
                }
              g_assert_null (sub);
              xstring_append_c (string, ']');

              result = xstring_free (string, FALSE);
            }
          else
            result = xstrdup ("R");
        }
      else if (xvariant_type_is_dict_entry (type))
        {
          xchar_t *key, *value, *key2, *value2;

          key = describe_type (xvariant_type_key (type));
          value = describe_type (xvariant_type_value (type));
          key2 = describe_type (xvariant_type_first (type));
          value2 = describe_type (
            xvariant_type_next (xvariant_type_first (type)));
          g_assert_null (xvariant_type_next (xvariant_type_next (
            xvariant_type_first (type))));
          g_assert_cmpstr (key, ==, key2);
          g_assert_cmpstr (value, ==, value2);
          result = xstrjoin ("", "e of [", key, ", ", value, "]", NULL);
          g_free (key2);
          g_free (value2);
          g_free (key);
          g_free (value);
        }
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_VARIANT))
        {
          result = xstrdup ("V");
        }
      else
        g_assert_not_reached ();
    }
  else
    {
      if (xvariant_type_is_definite (type))
        {
          g_assert_true (xvariant_type_is_basic (type));

          if (xvariant_type_equal (type, G_VARIANT_TYPE_BOOLEAN))
            result = xstrdup ("b");
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_BYTE))
            result = xstrdup ("y");
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_INT16))
            result = xstrdup ("n");
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_UINT16))
            result = xstrdup ("q");
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_INT32))
            result = xstrdup ("i");
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_UINT32))
            result = xstrdup ("u");
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_INT64))
            result = xstrdup ("x");
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_UINT64))
            result = xstrdup ("t");
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_HANDLE))
            result = xstrdup ("h");
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_DOUBLE))
            result = xstrdup ("d");
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_STRING))
            result = xstrdup ("s");
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_OBJECT_PATH))
            result = xstrdup ("o");
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_SIGNATURE))
            result = xstrdup ("g");
          else
            g_assert_not_reached ();
        }
      else
        {
          if (xvariant_type_equal (type, G_VARIANT_TYPE_ANY))
            {
              result = xstrdup ("S");
            }
          else if (xvariant_type_equal (type, G_VARIANT_TYPE_BASIC))
            {
              result = xstrdup ("?");
            }
          else
            g_assert_not_reached ();
        }
    }

  return result;
}

/* given a type string, replace one of the indefinite type characters in
 * it with a matching type (possibly the same type).
 */
static xchar_t *
generate_subtype (const xchar_t *type_string)
{
  xvariant_type_t *replacement;
  xstring_t *result, *junk;
  xint_t l;
  xsize_t length, n = 0;

  result = xstring_new (NULL);
  junk = xstring_new (NULL);

  /* count the number of indefinite type characters */
  for (length = 0; type_string[length]; length++)
    n += type_string[length] == 'r' ||
         type_string[length] == '?' ||
         type_string[length] == '*';
  /* length now is strlen (type_string) */

  /* pick one at random to replace */
  n = g_test_rand_int_range (0, n) + 1;

  /* find it */
  l = -1;
  while (n--) l += strcspn (type_string + l + 1, "r?*") + 1;
  g_assert_true (type_string[l] == 'r' ||
                 type_string[l] == '?' ||
                 type_string[l] == '*');

  /* store up to that point in a xstring_t */
  xstring_append_len (result, type_string, l);

  /* then store the replacement in the xstring_t */
  if (type_string[l] == 'r')
    replacement = append_tuple_type_string (result, junk, FALSE, 3);

  else if (type_string[l] == '?')
    replacement = append_type_string (result, junk, FALSE, 0);

  else if (type_string[l] == '*')
    replacement = append_type_string (result, junk, FALSE, 3);

  else
    g_assert_not_reached ();

  /* ensure the replacement has the proper type */
  g_assert_true (xvariant_type_is_subtype_of (replacement,
                                               (xpointer_t) &type_string[l]));

  /* store the rest from the original type string */
  xstring_append (result, type_string + l + 1);

  xvariant_type_free (replacement);
  xstring_free (junk, TRUE);

  return xstring_free (result, FALSE);
}

struct typestack
{
  const xvariant_type_t *type;
  struct typestack *parent;
};

/* given an indefinite type string, replace one of the indefinite
 * characters in it with a matching type and ensure that the result is a
 * subtype of the original.  repeat.
 */
static void
subtype_check (const xchar_t      *type_string,
               struct typestack *parent_ts)
{
  struct typestack ts, *node;
  xchar_t *subtype;
  xint_t depth = 0;

  subtype = generate_subtype (type_string);

  ts.type = G_VARIANT_TYPE (subtype);
  ts.parent = parent_ts;

  for (node = &ts; node; node = node->parent)
    {
      /* this type should be a subtype of each parent type */
      g_assert_true (xvariant_type_is_subtype_of (ts.type, node->type));

      /* it should only be a supertype when it is exactly equal */
      g_assert_true (xvariant_type_is_subtype_of (node->type, ts.type) ==
                     xvariant_type_equal (ts.type, node->type));

      depth++;
    }

  if (!xvariant_type_is_definite (ts.type) && depth < 5)
    {
      /* the type is still indefinite and we haven't repeated too many
       * times.  go once more.
       */

      subtype_check (subtype, &ts);
    }

  g_free (subtype);
}

static void
test_gvarianttype (void)
{
  xsize_t i;

  for (i = 0; i < 2000; i++)
    {
      xstring_t *type_string, *description;
      xvariant_type_t *type, *other_type;
      const xvariant_type_t *ctype;
      xchar_t *invalid;
      xchar_t *desc;

      type_string = xstring_new (NULL);
      description = xstring_new (NULL);

      /* generate a random type, its type string and a description
       *
       * exercises type constructor functions and xvariant_type_copy()
       */
      type = append_type_string (type_string, description, FALSE, 6);

      /* convert the type string to a type and ensure that it is equal
       * to the one produced with the type constructor routines
       */
      ctype = G_VARIANT_TYPE (type_string->str);
      g_assert_true (xvariant_type_equal (ctype, type));
      g_assert_cmpuint (xvariant_type_hash (ctype), ==, xvariant_type_hash (type));
      g_assert_true (xvariant_type_is_subtype_of (ctype, type));
      g_assert_true (xvariant_type_is_subtype_of (type, ctype));

      /* check if the type is indefinite */
      if (!xvariant_type_is_definite (type))
        {
          struct typestack ts = { type, NULL };

          /* if it is indefinite, then replace one of the indefinite
           * characters with a matching type and ensure that the result
           * is a subtype of the original type.  repeat.
           */
          subtype_check (type_string->str, &ts);
        }
      else
        /* ensure that no indefinite characters appear */
        g_assert_cmpint (strcspn (type_string->str, "r?*"), ==, type_string->len);


      /* describe the type.
       *
       * exercises the type iterator interface
       */
      desc = describe_type (type);

      /* make sure the description matches */
      g_assert_cmpstr (desc, ==, description->str);
      g_free (desc);

      /* make an invalid mutation to the type and make sure the type
       * validation routines catch it */
      invalid = invalid_mutation (type_string->str);
      g_assert_true (xvariant_type_string_is_valid (type_string->str));
      g_assert_false (xvariant_type_string_is_valid (invalid));
      g_free (invalid);

      /* concatenate another type to the type string and ensure that
       * the result is recognised as being invalid
       */
      other_type = append_type_string (type_string, description, FALSE, 2);

      xstring_free (description, TRUE);
      xstring_free (type_string, TRUE);
      xvariant_type_free (other_type);
      xvariant_type_free (type);
    }
}

/* test_t that scanning a deeply recursive type string doesn’t exhaust our
 * stack space (which it would if the type string scanner was recursive). */
static void
test_gvarianttype_string_scan_recursion_tuple (void)
{
  xchar_t *type_string = NULL;
  xsize_t type_string_len = 1000001;  /* not including nul terminator */
  xsize_t i;

  /* Build a long type string of ‘((…u…))’. */
  type_string = g_new0 (xchar_t, type_string_len + 1);
  for (i = 0; i < type_string_len; i++)
    {
      if (i < type_string_len / 2)
        type_string[i] = '(';
      else if (i == type_string_len / 2)
        type_string[i] = 'u';
      else
        type_string[i] = ')';
    }

  /* Goes (way) over allowed recursion limit. */
  g_assert_false (xvariant_type_string_is_valid (type_string));

  g_free (type_string);
}

/* Same as above, except with an array rather than a tuple. */
static void
test_gvarianttype_string_scan_recursion_array (void)
{
  xchar_t *type_string = NULL;
  xsize_t type_string_len = 1000001;  /* not including nul terminator */
  xsize_t i;

  /* Build a long type string of ‘aaa…aau’. */
  type_string = g_new0 (xchar_t, type_string_len + 1);
  for (i = 0; i < type_string_len; i++)
    {
      if (i < type_string_len - 1)
        type_string[i] = 'a';
      else
        type_string[i] = 'u';
    }

  /* Goes (way) over allowed recursion limit. */
  g_assert_false (xvariant_type_string_is_valid (type_string));

  g_free (type_string);
}

#define ALIGNED(x, y)   (((x + (y - 1)) / y) * y)

/* do our own calculation of the fixed_size and alignment of a type
 * using a simple algorithm to make sure the "fancy" one in the
 * implementation is correct.
 */
static void
calculate_type_info (const xvariant_type_t *type,
                     xsize_t              *fixed_size,
                     xuint_t              *alignment)
{
  if (xvariant_type_is_array (type) ||
      xvariant_type_is_maybe (type))
    {
      calculate_type_info (xvariant_type_element (type), NULL, alignment);

      if (fixed_size)
        *fixed_size = 0;
    }
  else if (xvariant_type_is_tuple (type) ||
           xvariant_type_is_dict_entry (type))
    {
      if (xvariant_type_n_items (type))
        {
          const xvariant_type_t *sub;
          xboolean_t variable;
          xsize_t size;
          xuint_t al;

          variable = FALSE;
          size = 0;
          al = 0;

          sub = xvariant_type_first (type);
          do
            {
              xsize_t this_fs;
              xuint_t this_al;

              calculate_type_info (sub, &this_fs, &this_al);

              al = MAX (al, this_al);

              if (!this_fs)
                {
                  variable = TRUE;
                  size = 0;
                }

              if (!variable)
                {
                  size = ALIGNED (size, this_al);
                  size += this_fs;
                }
            }
          while ((sub = xvariant_type_next (sub)));

          size = ALIGNED (size, al);

          if (alignment)
            *alignment = al;

          if (fixed_size)
            *fixed_size = size;
        }
      else
        {
          if (fixed_size)
            *fixed_size = 1;

          if (alignment)
            *alignment = 1;
        }
    }
  else
    {
      xint_t fs, al;

      if (xvariant_type_equal (type, G_VARIANT_TYPE_BOOLEAN) ||
          xvariant_type_equal (type, G_VARIANT_TYPE_BYTE))
        {
          al = fs = 1;
        }

      else if (xvariant_type_equal (type, G_VARIANT_TYPE_INT16) ||
               xvariant_type_equal (type, G_VARIANT_TYPE_UINT16))
        {
          al = fs = 2;
        }

      else if (xvariant_type_equal (type, G_VARIANT_TYPE_INT32) ||
               xvariant_type_equal (type, G_VARIANT_TYPE_UINT32) ||
               xvariant_type_equal (type, G_VARIANT_TYPE_HANDLE))
        {
          al = fs = 4;
        }

      else if (xvariant_type_equal (type, G_VARIANT_TYPE_INT64) ||
               xvariant_type_equal (type, G_VARIANT_TYPE_UINT64) ||
               xvariant_type_equal (type, G_VARIANT_TYPE_DOUBLE))
        {
          al = fs = 8;
        }
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_STRING) ||
               xvariant_type_equal (type, G_VARIANT_TYPE_OBJECT_PATH) ||
               xvariant_type_equal (type, G_VARIANT_TYPE_SIGNATURE))
        {
          al = 1;
          fs = 0;
        }
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_VARIANT))
        {
          al = 8;
          fs = 0;
        }
      else
        g_assert_not_reached ();

      if (fixed_size)
        *fixed_size = fs;

      if (alignment)
        *alignment = al;
    }
}

/* same as the describe_type() function above, but iterates over
 * typeinfo instead of types.
 */
static xchar_t *
describe_info (GVariantTypeInfo *info)
{
  xchar_t *result;

  switch (xvariant_type_info_get_type_char (info))
    {
    case G_VARIANT_TYPE_INFO_CHAR_MAYBE:
      {
        xchar_t *element;

        element = describe_info (xvariant_type_info_element (info));
        result = xstrdup_printf ("m of %s", element);
        g_free (element);
      }
      break;

    case G_VARIANT_TYPE_INFO_CHAR_ARRAY:
      {
        xchar_t *element;

        element = describe_info (xvariant_type_info_element (info));
        result = xstrdup_printf ("a of %s", element);
        g_free (element);
      }
      break;

    case G_VARIANT_TYPE_INFO_CHAR_TUPLE:
      {
        const xchar_t *sep = "";
        xstring_t *string;
        xsize_t i, length;

        string = xstring_new ("t of [");
        length = xvariant_type_info_n_members (info);

        for (i = 0; i < length; i++)
          {
            const GVariantMemberInfo *minfo;
            xchar_t *subtype;

            xstring_append (string, sep);
            sep = ", ";

            minfo = xvariant_type_info_member_info (info, i);
            subtype = describe_info (minfo->type_info);
            xstring_append (string, subtype);
            g_free (subtype);
          }

        xstring_append_c (string, ']');

        result = xstring_free (string, FALSE);
      }
      break;

    case G_VARIANT_TYPE_INFO_CHAR_DICT_ENTRY:
      {
        const GVariantMemberInfo *keyinfo, *valueinfo;
        xchar_t *key, *value;

        g_assert_cmpint (xvariant_type_info_n_members (info), ==, 2);
        keyinfo = xvariant_type_info_member_info (info, 0);
        valueinfo = xvariant_type_info_member_info (info, 1);
        key = describe_info (keyinfo->type_info);
        value = describe_info (valueinfo->type_info);
        result = xstrjoin ("", "e of [", key, ", ", value, "]", NULL);
        g_free (key);
        g_free (value);
      }
      break;

    case G_VARIANT_TYPE_INFO_CHAR_VARIANT:
      result = xstrdup ("V");
      break;

    default:
      result = xstrdup (xvariant_type_info_get_type_string (info));
      g_assert_cmpint (strlen (result), ==, 1);
      break;
    }

  return result;
}

/* check that the O(1) method of calculating offsets meshes with the
 * results of simple iteration.
 */
static void
check_offsets (GVariantTypeInfo   *info,
               const xvariant_type_t *type)
{
  xsize_t flavour, length;

  length = xvariant_type_info_n_members (info);
  g_assert_cmpuint (length, ==, xvariant_type_n_items (type));

  /* the 'flavour' is the low order bits of the ending point of
   * variable-size items in the tuple.  this lets us test that the type
   * info is correct for various starting alignments.
   */
  for (flavour = 0; flavour < 8; flavour++)
    {
      const xvariant_type_t *subtype;
      xsize_t last_offset_index;
      xsize_t last_offset;
      xsize_t position;
      xsize_t i;

      subtype = xvariant_type_first (type);
      last_offset_index = -1;
      last_offset = 0;
      position = 0;

      /* go through the tuple, keeping track of our position */
      for (i = 0; i < length; i++)
        {
          xsize_t fixed_size;
          xuint_t alignment;

          calculate_type_info (subtype, &fixed_size, &alignment);

          position = ALIGNED (position, alignment);

          /* compare our current aligned position (ie: the start of this
           * item) to the start offset that would be calculated if we
           * used the type info
           */
          {
            const GVariantMemberInfo *member;
            xsize_t start;

            member = xvariant_type_info_member_info (info, i);
            g_assert_cmpint (member->i, ==, last_offset_index);

            /* do the calculation using the typeinfo */
            start = last_offset;
            start += member->a;
            start &= member->b;
            start |= member->c;

            /* did we reach the same spot? */
            g_assert_cmpint (start, ==, position);
          }

          if (fixed_size)
            {
              /* fixed size.  add that size. */
              position += fixed_size;
            }
          else
            {
              /* variable size.  do the flavouring. */
              while ((position & 0x7) != flavour)
                position++;

              /* and store the offset, just like it would be in the
               * serialized data.
               */
              last_offset = position;
              last_offset_index++;
            }

          /* next type */
          subtype = xvariant_type_next (subtype);
        }

      /* make sure we used up exactly all the types */
      g_assert_null (subtype);
    }
}

static void
test_gvarianttypeinfo (void)
{
  xsize_t i;

  for (i = 0; i < 2000; i++)
    {
      xstring_t *type_string, *description;
      xsize_t fixed_size1, fixed_size2;
      xuint_t alignment1, alignment2;
      GVariantTypeInfo *info;
      xvariant_type_t *type;
      xchar_t *desc;

      type_string = xstring_new (NULL);
      description = xstring_new (NULL);

      /* random type */
      type = append_type_string (type_string, description, TRUE, 6);

      /* create a typeinfo for it */
      info = xvariant_type_info_get (type);

      /* make sure the typeinfo has the right type string */
      g_assert_cmpstr (xvariant_type_info_get_type_string (info), ==,
                       type_string->str);

      /* calculate the alignment and fixed size, compare to the
       * typeinfo's calculations
       */
      calculate_type_info (type, &fixed_size1, &alignment1);
      xvariant_type_info_query (info, &alignment2, &fixed_size2);
      g_assert_cmpint (fixed_size1, ==, fixed_size2);
      g_assert_cmpint (alignment1, ==, alignment2 + 1);

      /* test the iteration functions over typeinfo structures by
       * "describing" the typeinfo and verifying equality.
       */
      desc = describe_info (info);
      g_assert_cmpstr (desc, ==, description->str);

      /* do extra checks for containers */
      if (xvariant_type_is_array (type) ||
          xvariant_type_is_maybe (type))
        {
          const xvariant_type_t *element;
          xsize_t efs1, efs2;
          xuint_t ea1, ea2;

          element = xvariant_type_element (type);
          calculate_type_info (element, &efs1, &ea1);
          xvariant_type_info_query_element (info, &ea2, &efs2);
          g_assert_cmpint (efs1, ==, efs2);
          g_assert_cmpint (ea1, ==, ea2 + 1);

          g_assert_cmpint (ea1, ==, alignment1);
          g_assert_cmpint (0, ==, fixed_size1);
        }
      else if (xvariant_type_is_tuple (type) ||
               xvariant_type_is_dict_entry (type))
        {
          /* make sure the "magic constants" are working */
          check_offsets (info, type);
        }

      xstring_free (type_string, TRUE);
      xstring_free (description, TRUE);
      xvariant_type_info_unref (info);
      xvariant_type_free (type);
      g_free (desc);
    }

  xvariant_type_info_assert_no_infos ();
}

#define MAX_FIXED_MULTIPLIER    256
#define MAX_INSTANCE_SIZE       1024
#define MAX_ARRAY_CHILDREN      128
#define MAX_TUPLE_CHILDREN      128

/* this function generates a random type such that all characteristics
 * that are "interesting" to the serializer are tested.
 *
 * this basically means:
 *   - test different alignments
 *   - test variable sized items and fixed sized items
 *   - test different fixed sizes
 */
static xchar_t *
random_type_string (void)
{
  const xuchar_t base_types[] = "ynix";
  xuchar_t base_type;

  base_type = base_types[g_test_rand_int_range (0, 4)];

  if (g_test_rand_bit ())
    /* construct a fixed-sized type */
    {
      char type_string[MAX_FIXED_MULTIPLIER];
      xuint_t multiplier;
      xsize_t i = 0;

      multiplier = g_test_rand_int_range (1, sizeof type_string - 1);

      type_string[i++] = '(';
      while (multiplier--)
        type_string[i++] = base_type;
      type_string[i++] = ')';

      return xstrndup (type_string, i);
    }
  else
    /* construct a variable-sized type */
    {
      char type_string[2] = { 'a', base_type };

      return xstrndup (type_string, 2);
    }
}

typedef struct
{
  GVariantTypeInfo *type_info;
  xuint_t alignment;
  xsize_t size;
  xboolean_t is_fixed_sized;

  xuint32_t seed;

#define INSTANCE_MAGIC    1287582829
  xuint_t magic;
} RandomInstance;

static RandomInstance *
random_instance (GVariantTypeInfo *type_info)
{
  RandomInstance *instance;

  instance = g_slice_new (RandomInstance);

  if (type_info == NULL)
    {
      xchar_t *str = random_type_string ();
      instance->type_info = xvariant_type_info_get (G_VARIANT_TYPE (str));
      g_free (str);
    }
  else
    instance->type_info = xvariant_type_info_ref (type_info);

  instance->seed = g_test_rand_int ();

  xvariant_type_info_query (instance->type_info,
                             &instance->alignment,
                             &instance->size);

  instance->is_fixed_sized = instance->size != 0;

  if (!instance->is_fixed_sized)
    instance->size = g_test_rand_int_range (0, MAX_INSTANCE_SIZE);

  instance->magic = INSTANCE_MAGIC;

  return instance;
}

static void
random_instance_free (RandomInstance *instance)
{
  xvariant_type_info_unref (instance->type_info);
  g_slice_free (RandomInstance, instance);
}

static void
append_instance_size (RandomInstance *instance,
                      xsize_t          *offset)
{
  *offset += (-*offset) & instance->alignment;
  *offset += instance->size;
}

static void
random_instance_write (RandomInstance *instance,
                       xuchar_t         *buffer)
{
  xrand_t *rand;
  xsize_t i;

  g_assert_cmpint ((xsize_t) buffer & ALIGN_BITS & instance->alignment, ==, 0);

  rand = g_rand_new_with_seed (instance->seed);
  for (i = 0; i < instance->size; i++)
    buffer[i] = g_rand_int (rand);
  g_rand_free (rand);
}

static void
append_instance_data (RandomInstance  *instance,
                      xuchar_t         **buffer)
{
  while (((xsize_t) *buffer) & instance->alignment)
    *(*buffer)++ = '\0';

  random_instance_write (instance, *buffer);
  *buffer += instance->size;
}

static xboolean_t
random_instance_assert (RandomInstance *instance,
                        xuchar_t         *buffer,
                        xsize_t           size)
{
  xrand_t *rand;
  xsize_t i;

  g_assert_cmpint ((xsize_t) buffer & ALIGN_BITS & instance->alignment, ==, 0);
  g_assert_cmpint (size, ==, instance->size);

  rand = g_rand_new_with_seed (instance->seed);
  for (i = 0; i < instance->size; i++)
    {
      xuchar_t byte = g_rand_int (rand);

      g_assert_cmpuint (buffer[i], ==, byte);
    }
  g_rand_free (rand);

  return i == instance->size;
}

static xboolean_t
random_instance_check (RandomInstance *instance,
                       xuchar_t         *buffer,
                       xsize_t           size)
{
  xrand_t *rand;
  xsize_t i;

  g_assert_cmpint ((xsize_t) buffer & ALIGN_BITS & instance->alignment, ==, 0);

  if (size != instance->size)
    return FALSE;

  rand = g_rand_new_with_seed (instance->seed);
  for (i = 0; i < instance->size; i++)
    if (buffer[i] != (xuchar_t) g_rand_int (rand))
      break;
  g_rand_free (rand);

  return i == instance->size;
}

static void
random_instance_filler (GVariantSerialised *serialised,
                        xpointer_t            data)
{
  RandomInstance *instance = data;

  g_assert_cmpuint (instance->magic, ==, INSTANCE_MAGIC);

  if (serialised->type_info == NULL)
    serialised->type_info = instance->type_info;

  if (serialised->size == 0)
    serialised->size = instance->size;

  serialised->depth = 0;

  g_assert_true (serialised->type_info == instance->type_info);
  g_assert_cmpuint (serialised->size, ==, instance->size);

  if (serialised->data)
    random_instance_write (instance, serialised->data);
}

static xsize_t
calculate_offset_size (xsize_t body_size,
                       xsize_t n_offsets)
{
  if (body_size == 0)
    return 0;

  if (body_size + n_offsets <= G_MAXUINT8)
    return 1;

  if (body_size + 2 * n_offsets <= G_MAXUINT16)
    return 2;

  if (body_size + 4 * n_offsets <= G_MAXUINT32)
    return 4;

  /* the test case won't generate anything bigger */
  g_assert_not_reached ();
}

static xpointer_t
flavoured_malloc (xsize_t size, xsize_t flavour)
{
  g_assert_cmpuint (flavour, <, 8);

  if (size == 0)
    return NULL;

  return ((xchar_t *) g_malloc (size + flavour)) + flavour;
}

static void
flavoured_free (xpointer_t data,
                xsize_t flavour)
{
  if (!data)
    return;
  g_free (((xchar_t *) data) - flavour);
}

static xpointer_t
align_malloc (xsize_t size)
{
  xpointer_t mem;

#ifdef HAVE_POSIX_MEMALIGN
  if (posix_memalign (&mem, 8, size))
    xerror ("posix_memalign failed");
#else
  /* NOTE: there may be platforms that lack posix_memalign() and also
   * have malloc() that returns non-8-aligned.  if so, we need to try
   * harder here.
   */
  mem = malloc (size);
#endif

  return mem;
}

static void
align_free (xpointer_t mem)
{
  free (mem);
}

static void
append_offset (xuchar_t **offset_ptr,
               xsize_t    offset,
               xuint_t    offset_size)
{
  union
  {
    xuchar_t bytes[sizeof (xsize_t)];
    xsize_t integer;
  } tmpvalue;

  tmpvalue.integer = GSIZE_TO_LE (offset);
  memcpy (*offset_ptr, tmpvalue.bytes, offset_size);
  *offset_ptr += offset_size;
}

static void
prepend_offset (xuchar_t **offset_ptr,
                xsize_t    offset,
                xuint_t    offset_size)
{
  union
  {
    xuchar_t bytes[sizeof (xsize_t)];
    xsize_t integer;
  } tmpvalue;

  *offset_ptr -= offset_size;
  tmpvalue.integer = GSIZE_TO_LE (offset);
  memcpy (*offset_ptr, tmpvalue.bytes, offset_size);
}

static void
test_maybe (void)
{
  GVariantTypeInfo *type_info;
  RandomInstance *instance;
  xsize_t needed_size;
  xuchar_t *data;

  instance = random_instance (NULL);

  {
    const xchar_t *element;
    xchar_t *tmp;

    element = xvariant_type_info_get_type_string (instance->type_info);
    tmp = xstrdup_printf ("m%s", element);
    type_info = xvariant_type_info_get (G_VARIANT_TYPE (tmp));
    g_free (tmp);
  }

  needed_size = xvariant_serialiser_needed_size (type_info,
                                                  random_instance_filler,
                                                  NULL, 0);
  g_assert_cmpint (needed_size, ==, 0);

  needed_size = xvariant_serialiser_needed_size (type_info,
                                                  random_instance_filler,
                                                  (xpointer_t *) &instance, 1);

  if (instance->is_fixed_sized)
    g_assert_cmpint (needed_size, ==, instance->size);
  else
    g_assert_cmpint (needed_size, ==, instance->size + 1);

  {
    xuchar_t *ptr;

    ptr = data = align_malloc (needed_size);
    append_instance_data (instance, &ptr);

    if (!instance->is_fixed_sized)
      *ptr++ = '\0';

    g_assert_cmpint (ptr - data, ==, needed_size);
  }

  {
    xuint_t alignment;
    xsize_t flavour;

    alignment = (instance->alignment & ALIGN_BITS) + 1;

    for (flavour = 0; flavour < 8; flavour += alignment)
      {
        GVariantSerialised serialised;
        GVariantSerialised child;

        serialised.type_info = type_info;
        serialised.data = flavoured_malloc (needed_size, flavour);
        serialised.size = needed_size;
        serialised.depth = 0;

        xvariant_serialiser_serialise (serialised,
                                        random_instance_filler,
                                        (xpointer_t *) &instance, 1);
        child = xvariant_serialised_get_child (serialised, 0);
        g_assert_true (child.type_info == instance->type_info);
        random_instance_assert (instance, child.data, child.size);
        xvariant_type_info_unref (child.type_info);
        flavoured_free (serialised.data, flavour);
      }
  }

  xvariant_type_info_unref (type_info);
  random_instance_free (instance);
  align_free (data);
}

static void
test_maybes (void)
{
  xsize_t i;

  for (i = 0; i < 1000; i++)
    test_maybe ();

  xvariant_type_info_assert_no_infos ();
}

static void
test_array (void)
{
  GVariantTypeInfo *element_info;
  GVariantTypeInfo *array_info;
  RandomInstance **instances;
  xsize_t needed_size;
  xsize_t offset_size;
  xuint_t n_children;
  xuchar_t *data;

  {
    xchar_t *element_type, *array_type;

    element_type = random_type_string ();
    array_type = xstrdup_printf ("a%s", element_type);

    element_info = xvariant_type_info_get (G_VARIANT_TYPE (element_type));
    array_info = xvariant_type_info_get (G_VARIANT_TYPE (array_type));
    g_assert_true (xvariant_type_info_element (array_info) == element_info);

    g_free (element_type);
    g_free (array_type);
  }

  {
    xsize_t i;

    n_children = g_test_rand_int_range (0, MAX_ARRAY_CHILDREN);
    instances = g_new (RandomInstance *, n_children);
    for (i = 0; i < n_children; i++)
      instances[i] = random_instance (element_info);
  }

  needed_size = xvariant_serialiser_needed_size (array_info,
                                                  random_instance_filler,
                                                  (xpointer_t *) instances,
                                                  n_children);

  {
    xsize_t element_fixed_size;
    xsize_t body_size = 0;
    xsize_t i;

    for (i = 0; i < n_children; i++)
      append_instance_size (instances[i], &body_size);

    xvariant_type_info_query (element_info, NULL, &element_fixed_size);

    if (!element_fixed_size)
      {
        offset_size = calculate_offset_size (body_size, n_children);

        if (offset_size == 0)
          offset_size = 1;
      }
    else
      offset_size = 0;

    g_assert_cmpint (needed_size, ==, body_size + n_children * offset_size);
  }

  {
    xuchar_t *offset_ptr, *body_ptr;
    xsize_t i;

    body_ptr = data = align_malloc (needed_size);
    offset_ptr = body_ptr + needed_size - offset_size * n_children;

    for (i = 0; i < n_children; i++)
      {
        append_instance_data (instances[i], &body_ptr);
        append_offset (&offset_ptr, body_ptr - data, offset_size);
      }

    g_assert_true (body_ptr == data + needed_size - offset_size * n_children);
    g_assert_true (offset_ptr == data + needed_size);
  }

  {
    xuint_t alignment;
    xsize_t flavour;
    xsize_t i;

    xvariant_type_info_query (array_info, &alignment, NULL);
    alignment = (alignment & ALIGN_BITS) + 1;

    for (flavour = 0; flavour < 8; flavour += alignment)
      {
        GVariantSerialised serialised;

        serialised.type_info = array_info;
        serialised.data = flavoured_malloc (needed_size, flavour);
        serialised.size = needed_size;
        serialised.depth = 0;

        xvariant_serialiser_serialise (serialised, random_instance_filler,
                                        (xpointer_t *) instances, n_children);

        if (serialised.size)
          g_assert_cmpint (memcmp (serialised.data, data, serialised.size), ==, 0);

        g_assert_cmpuint (xvariant_serialised_n_children (serialised), ==, n_children);

        for (i = 0; i < n_children; i++)
          {
            GVariantSerialised child;

            child = xvariant_serialised_get_child (serialised, i);
            g_assert_true (child.type_info == instances[i]->type_info);
            random_instance_assert (instances[i], child.data, child.size);
            xvariant_type_info_unref (child.type_info);
          }

        flavoured_free (serialised.data, flavour);
      }
  }

  {
    xsize_t i;

    for (i = 0; i < n_children; i++)
      random_instance_free (instances[i]);
    g_free (instances);
  }

  xvariant_type_info_unref (element_info);
  xvariant_type_info_unref (array_info);
  align_free (data);
}

static void
test_arrays (void)
{
  xsize_t i;

  for (i = 0; i < 100; i++)
    test_array ();

  xvariant_type_info_assert_no_infos ();
}

static void
test_tuple (void)
{
  GVariantTypeInfo *type_info;
  RandomInstance **instances;
  xboolean_t fixed_size;
  xsize_t needed_size;
  xsize_t offset_size;
  xuint_t n_children;
  xuint_t alignment;
  xuchar_t *data;

  n_children = g_test_rand_int_range (0, MAX_TUPLE_CHILDREN);
  instances = g_new (RandomInstance *, n_children);

  {
    xstring_t *type_string;
    xsize_t i;

    fixed_size = TRUE;
    alignment = 0;

    type_string = xstring_new ("(");
    for (i = 0; i < n_children; i++)
      {
        const xchar_t *str;

        instances[i] = random_instance (NULL);

        alignment |= instances[i]->alignment;
        if (!instances[i]->is_fixed_sized)
          fixed_size = FALSE;

        str = xvariant_type_info_get_type_string (instances[i]->type_info);
        xstring_append (type_string, str);
      }
    xstring_append_c (type_string, ')');

    type_info = xvariant_type_info_get (G_VARIANT_TYPE (type_string->str));
    xstring_free (type_string, TRUE);
  }

  needed_size = xvariant_serialiser_needed_size (type_info,
                                                  random_instance_filler,
                                                  (xpointer_t *) instances,
                                                  n_children);
  {
    xsize_t body_size = 0;
    xsize_t offsets = 0;
    xsize_t i;

    for (i = 0; i < n_children; i++)
      {
        append_instance_size (instances[i], &body_size);

        if (i != n_children - 1 && !instances[i]->is_fixed_sized)
          offsets++;
      }

    if (fixed_size)
      {
        body_size += (-body_size) & alignment;

        g_assert_true ((body_size == 0) == (n_children == 0));
        if (n_children == 0)
          body_size = 1;
      }

    offset_size = calculate_offset_size (body_size, offsets);
    g_assert_cmpint (needed_size, ==, body_size + offsets * offset_size);
  }

  {
    xuchar_t *body_ptr;
    xuchar_t *ofs_ptr;
    xsize_t i;

    body_ptr = data = align_malloc (needed_size);
    ofs_ptr = body_ptr + needed_size;

    for (i = 0; i < n_children; i++)
      {
        append_instance_data (instances[i], &body_ptr);

        if (i != n_children - 1 && !instances[i]->is_fixed_sized)
          prepend_offset (&ofs_ptr, body_ptr - data, offset_size);
      }

    if (fixed_size)
      {
        while (((xsize_t) body_ptr) & alignment)
          *body_ptr++ = '\0';

        g_assert_true ((body_ptr == data) == (n_children == 0));
        if (n_children == 0)
          *body_ptr++ = '\0';

      }


    g_assert_true (body_ptr == ofs_ptr);
  }

  {
    xsize_t flavour;
    xsize_t i;

    alignment = (alignment & ALIGN_BITS) + 1;

    for (flavour = 0; flavour < 8; flavour += alignment)
      {
        GVariantSerialised serialised;

        serialised.type_info = type_info;
        serialised.data = flavoured_malloc (needed_size, flavour);
        serialised.size = needed_size;
        serialised.depth = 0;

        xvariant_serialiser_serialise (serialised, random_instance_filler,
                                        (xpointer_t *) instances, n_children);

        if (serialised.size)
          g_assert_cmpint (memcmp (serialised.data, data, serialised.size), ==, 0);

        g_assert_cmpuint (xvariant_serialised_n_children (serialised), ==, n_children);

        for (i = 0; i < n_children; i++)
          {
            GVariantSerialised child;

            child = xvariant_serialised_get_child (serialised, i);
            g_assert_true (child.type_info == instances[i]->type_info);
            random_instance_assert (instances[i], child.data, child.size);
            xvariant_type_info_unref (child.type_info);
          }

        flavoured_free (serialised.data, flavour);
      }
  }

  {
    xsize_t i;

    for (i = 0; i < n_children; i++)
      random_instance_free (instances[i]);
    g_free (instances);
  }

  xvariant_type_info_unref (type_info);
  align_free (data);
}

static void
test_tuples (void)
{
  xsize_t i;

  for (i = 0; i < 100; i++)
    test_tuple ();

  xvariant_type_info_assert_no_infos ();
}

static void
test_variant (void)
{
  GVariantTypeInfo *type_info;
  RandomInstance *instance;
  const xchar_t *type_string;
  xsize_t needed_size;
  xuchar_t *data;
  xsize_t len;

  type_info = xvariant_type_info_get (G_VARIANT_TYPE_VARIANT);
  instance = random_instance (NULL);

  type_string = xvariant_type_info_get_type_string (instance->type_info);
  len = strlen (type_string);

  needed_size = xvariant_serialiser_needed_size (type_info,
                                                  random_instance_filler,
                                                  (xpointer_t *) &instance, 1);

  g_assert_cmpint (needed_size, ==, instance->size + 1 + len);

  {
    xuchar_t *ptr;

    ptr = data = align_malloc (needed_size);
    append_instance_data (instance, &ptr);
    *ptr++ = '\0';
    memcpy (ptr, type_string, len);
    ptr += len;

    g_assert_true (data + needed_size == ptr);
  }

  {
    xsize_t alignment;
    xsize_t flavour;

    /* variants are always 8-aligned */
    alignment = ALIGN_BITS + 1;

    for (flavour = 0; flavour < 8; flavour += alignment)
      {
        GVariantSerialised serialised;
        GVariantSerialised child;

        serialised.type_info = type_info;
        serialised.data = flavoured_malloc (needed_size, flavour);
        serialised.size = needed_size;
        serialised.depth = 0;

        xvariant_serialiser_serialise (serialised, random_instance_filler,
                                        (xpointer_t *) &instance, 1);

        if (serialised.size)
          g_assert_cmpint (memcmp (serialised.data, data, serialised.size), ==, 0);

        g_assert_cmpuint (xvariant_serialised_n_children (serialised), ==, 1);

        child = xvariant_serialised_get_child (serialised, 0);
        g_assert_true (child.type_info == instance->type_info);
        random_instance_check (instance, child.data, child.size);

        xvariant_type_info_unref (child.type_info);
        flavoured_free (serialised.data, flavour);
      }
  }

  xvariant_type_info_unref (type_info);
  random_instance_free (instance);
  align_free (data);
}

static void
test_variants (void)
{
  xsize_t i;

  for (i = 0; i < 100; i++)
    test_variant ();

  xvariant_type_info_assert_no_infos ();
}

static void
test_strings (void)
{
  struct {
    xuint_t flags;
    xuint_t size;
    xconstpointer data;
  } test_cases[] = {
#define is_nval           0
#define is_string         1
#define is_objpath        is_string | 2
#define is_sig            is_string | 4
    { is_sig,       1, "" },
    { is_nval,      0, NULL },
    { is_nval,     13, "hello\xffworld!" },
    { is_string,   13, "hello world!" },
    { is_nval,     13, "hello world\0" },
    { is_nval,     13, "hello\0world!" },
    { is_nval,     12, "hello world!" },
    { is_nval,     13, "hello world!\xff" },

    { is_objpath,   2, "/" },
    { is_objpath,   3, "/a" },
    { is_string,    3, "//" },
    { is_objpath,  11, "/some/path" },
    { is_string,   12, "/some/path/" },
    { is_nval,     11, "/some\0path" },
    { is_string,   11, "/some\\path" },
    { is_string,   12, "/some//path" },
    { is_string,   12, "/some-/path" },

    { is_sig,       2, "i" },
    { is_sig,       2, "s" },
    { is_sig,       5, "(si)" },
    { is_string,    4, "(si" },
    { is_string,    2, "*" },
    { is_sig,       3, "ai" },
    { is_string,    3, "mi" },
    { is_string,    2, "r" },
    { is_sig,      15, "(yyy{sv}ssiai)" },
    { is_string,   16, "(yyy{yv}ssiai))" },
    { is_string,   15, "(yyy{vv}ssiai)" },
    { is_string,   15, "(yyy{sv)ssiai}" }
  };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (test_cases); i++)
    {
      xuint_t flags;

      flags = xvariant_serialiser_is_string (test_cases[i].data,
                                              test_cases[i].size)
        ? 1 : 0;

      flags |= xvariant_serialiser_is_object_path (test_cases[i].data,
                                                    test_cases[i].size)
        ? 2 : 0;

      flags |= xvariant_serialiser_is_signature (test_cases[i].data,
                                                  test_cases[i].size)
        ? 4 : 0;

      g_assert_cmpuint (flags, ==, test_cases[i].flags);
    }
}

typedef struct _TreeInstance TreeInstance;
struct _TreeInstance
{
  GVariantTypeInfo *info;

  TreeInstance **children;
  xsize_t n_children;

  union {
    xuint64_t integer;
    xdouble_t floating;
    xchar_t string[200];
  } data;
  xsize_t data_size;
};

static xvariant_type_t *
make_random_definite_type (int depth)
{
  xstring_t *description;
  xstring_t *type_string;
  xvariant_type_t *type;

  description = xstring_new (NULL);
  type_string = xstring_new (NULL);
  type = append_type_string (type_string, description, TRUE, depth);
  xstring_free (description, TRUE);
  xstring_free (type_string, TRUE);

  return type;
}

static void
make_random_string (xchar_t              *string,
                    xsize_t               size,
                    const xvariant_type_t *type)
{
  xsize_t i;

  /* create strings that are valid signature strings */
#define good_chars "bynqiuxthdsog"

  for (i = 0; i < size - 1; i++)
    string[i] = good_chars[g_test_rand_int_range (0, strlen (good_chars))];
  string[i] = '\0';

  /* in case we need an object path, prefix a '/' */
  if (*xvariant_type_peek_string (type) == 'o')
    string[0] = '/';

#undef good_chars
}

static TreeInstance *
tree_instance_new (const xvariant_type_t *type,
                   int                 depth)
{
  const xvariant_type_t *child_type = NULL;
  xvariant_type_t *mytype = NULL;
  TreeInstance *instance;
  xboolean_t is_tuple_type;

  if (type == NULL)
    type = mytype = make_random_definite_type (depth);

  instance = g_slice_new (TreeInstance);
  instance->info = xvariant_type_info_get (type);
  instance->children = NULL;
  instance->n_children = 0;
  instance->data_size = 0;

  is_tuple_type = FALSE;

  switch (*xvariant_type_peek_string (type))
    {
    case G_VARIANT_TYPE_INFO_CHAR_MAYBE:
      instance->n_children = g_test_rand_int_range (0, 2);
      child_type = xvariant_type_element (type);
      break;

    case G_VARIANT_TYPE_INFO_CHAR_ARRAY:
      instance->n_children = g_test_rand_int_range (0, MAX_ARRAY_CHILDREN);
      child_type = xvariant_type_element (type);
      break;

    case G_VARIANT_TYPE_INFO_CHAR_DICT_ENTRY:
    case G_VARIANT_TYPE_INFO_CHAR_TUPLE:
      instance->n_children = xvariant_type_n_items (type);
      child_type = xvariant_type_first (type);
      is_tuple_type = TRUE;
      break;

    case G_VARIANT_TYPE_INFO_CHAR_VARIANT:
      instance->n_children = 1;
      child_type = NULL;
      break;

    case 'b':
      instance->data.integer = g_test_rand_int_range (0, 2);
      instance->data_size = 1;
      break;

    case 'y':
      instance->data.integer = g_test_rand_int ();
      instance->data_size = 1;
      break;

    case 'n': case 'q':
      instance->data.integer = g_test_rand_int ();
      instance->data_size = 2;
      break;

    case 'i': case 'u': case 'h':
      instance->data.integer = g_test_rand_int ();
      instance->data_size = 4;
      break;

    case 'x': case 't':
      instance->data.integer = g_test_rand_int ();
      instance->data.integer <<= 32;
      instance->data.integer |= (xuint32_t) g_test_rand_int ();
      instance->data_size = 8;
      break;

    case 'd':
      instance->data.floating = g_test_rand_double ();
      instance->data_size = 8;
      break;

    case 's': case 'o': case 'g':
      instance->data_size = g_test_rand_int_range (10, 200);
      make_random_string (instance->data.string, instance->data_size, type);
      break;
    }

  if (instance->data_size == 0)
    /* no data -> it is a container */
    {
      xsize_t i;

      instance->children = g_new (TreeInstance *, instance->n_children);

      for (i = 0; i < instance->n_children; i++)
        {
          instance->children[i] = tree_instance_new (child_type, depth - 1);

          if (is_tuple_type)
            child_type = xvariant_type_next (child_type);
        }

      g_assert_true (!is_tuple_type || child_type == NULL);
    }

  xvariant_type_free (mytype);

  return instance;
}

static void
tree_instance_free (TreeInstance *instance)
{
  xsize_t i;

  xvariant_type_info_unref (instance->info);
  for (i = 0; i < instance->n_children; i++)
    tree_instance_free (instance->children[i]);
  g_free (instance->children);
  g_slice_free (TreeInstance, instance);
}

static xboolean_t i_am_writinxbyteswapped;

static void
tree_filler (GVariantSerialised *serialised,
             xpointer_t            data)
{
  TreeInstance *instance = data;

  if (serialised->type_info == NULL)
    serialised->type_info = instance->info;

  serialised->depth = 0;

  if (instance->data_size == 0)
    /* is a container */
    {
      if (serialised->size == 0)
        serialised->size =
          xvariant_serialiser_needed_size (instance->info, tree_filler,
                                            (xpointer_t *) instance->children,
                                            instance->n_children);

      if (serialised->data)
        xvariant_serialiser_serialise (*serialised, tree_filler,
                                        (xpointer_t *) instance->children,
                                        instance->n_children);
    }
  else
    /* it is a leaf */
    {
      if (serialised->size == 0)
        serialised->size = instance->data_size;

      if (serialised->data)
        {
          switch (instance->data_size)
            {
            case 1:
              *serialised->data = instance->data.integer;
              break;

            case 2:
              {
                xuint16_t value = instance->data.integer;

                if (i_am_writinxbyteswapped)
                  value = GUINT16_SWAP_LE_BE (value);

                *(xuint16_t *) serialised->data = value;
              }
              break;

            case 4:
              {
                xuint32_t value = instance->data.integer;

                if (i_am_writinxbyteswapped)
                  value = GUINT32_SWAP_LE_BE (value);

                *(xuint32_t *) serialised->data = value;
              }
              break;

            case 8:
              {
                xuint64_t value = instance->data.integer;

                if (i_am_writinxbyteswapped)
                  value = GUINT64_SWAP_LE_BE (value);

                *(xuint64_t *) serialised->data = value;
              }
              break;

            default:
              memcpy (serialised->data,
                      instance->data.string,
                      instance->data_size);
              break;
            }
        }
    }
}

static xboolean_t
check_tree (TreeInstance       *instance,
            GVariantSerialised  serialised)
{
  if (instance->info != serialised.type_info)
    return FALSE;

  if (instance->data_size == 0)
    /* is a container */
    {
      xsize_t i;

      if (xvariant_serialised_n_children (serialised) !=
          instance->n_children)
        return FALSE;

      for (i = 0; i < instance->n_children; i++)
        {
          GVariantSerialised child;
          xpointer_t data = NULL;
          xboolean_t ok;

          child = xvariant_serialised_get_child (serialised, i);
          if (child.size && child.data == NULL)
            child.data = data = g_malloc0 (child.size);
          ok = check_tree (instance->children[i], child);
          xvariant_type_info_unref (child.type_info);
          g_free (data);

          if (!ok)
            return FALSE;
        }

      return TRUE;
    }
  else
    /* it is a leaf */
    {
      switch (instance->data_size)
        {
        case 1:
          g_assert_cmpuint (serialised.size, ==, 1);
          return *(xuint8_t *) serialised.data ==
                  (xuint8_t) instance->data.integer;

        case 2:
          g_assert_cmpuint (serialised.size, ==, 2);
          return *(xuint16_t *) serialised.data ==
                  (xuint16_t) instance->data.integer;

        case 4:
          g_assert_cmpuint (serialised.size, ==, 4);
          return *(xuint32_t *) serialised.data ==
                  (xuint32_t) instance->data.integer;

        case 8:
          g_assert_cmpuint (serialised.size, ==, 8);
          return *(xuint64_t *) serialised.data ==
                  (xuint64_t) instance->data.integer;

        default:
          if (serialised.size != instance->data_size)
            return FALSE;

          return memcmp (serialised.data,
                         instance->data.string,
                         instance->data_size) == 0;
        }
    }
}

static void
serialise_tree (TreeInstance       *tree,
                GVariantSerialised *serialised)
{
  GVariantSerialised empty = {0, };

  *serialised = empty;
  tree_filler (serialised, tree);
  serialised->data = g_malloc (serialised->size);
  tree_filler (serialised, tree);
}

static void
test_byteswap (void)
{
  GVariantSerialised one, two;
  TreeInstance *tree;

  tree = tree_instance_new (NULL, 3);
  serialise_tree (tree, &one);

  i_am_writinxbyteswapped = TRUE;
  serialise_tree (tree, &two);
  i_am_writinxbyteswapped = FALSE;

  xvariant_serialised_byteswap (two);

  g_assert_cmpmem (one.data, one.size, two.data, two.size);
  g_assert_cmpuint (one.depth, ==, two.depth);

  tree_instance_free (tree);
  g_free (one.data);
  g_free (two.data);
}

static void
test_byteswaps (void)
{
  int i;

  for (i = 0; i < 200; i++)
    test_byteswap ();

  xvariant_type_info_assert_no_infos ();
}

static void
test_serialiser_children (void)
{
  xbytes_t *data1, *data2;
  xvariant_t *child1, *child2;
  xvariant_type_t *mv_type = xvariant_type_new_maybe (G_VARIANT_TYPE_VARIANT);
  xvariant_t *variant, *child;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/1865");
  g_test_summary ("test_t that getting a child variant before and after "
                  "serialisation of the parent works");

  /* Construct a variable sized array containing a child which serializes to a
   * zero-length bytestring. */
  child = xvariant_new_maybe (G_VARIANT_TYPE_VARIANT, NULL);
  variant = xvariant_new_array (mv_type, &child, 1);

  /* Get the child before serializing. */
  child1 = xvariant_get_child_value (variant, 0);
  data1 = xvariant_get_data_as_bytes (child1);

  /* Serialize the parent variant. */
  xvariant_get_data (variant);

  /* Get the child again after serializing — this uses a different code path. */
  child2 = xvariant_get_child_value (variant, 0);
  data2 = xvariant_get_data_as_bytes (child2);

  /* Check things are equal. */
  g_assert_cmpvariant (child1, child2);
  g_assert_true (xbytes_equal (data1, data2));

  xvariant_unref (child2);
  xvariant_unref (child1);
  xvariant_unref (variant);
  xbytes_unref (data2);
  xbytes_unref (data1);
  xvariant_type_free (mv_type);
}

static void
test_fuzz (xdouble_t *fuzziness)
{
  GVariantSerialised serialised;
  TreeInstance *tree;

  /* make an instance */
  tree = tree_instance_new (NULL, 3);

  /* serialize it */
  serialise_tree (tree, &serialised);

  g_assert_true (xvariant_serialised_is_normal (serialised));
  g_assert_true (check_tree (tree, serialised));

  if (serialised.size)
    {
      xboolean_t fuzzed = FALSE;
      xboolean_t a, b;

      while (!fuzzed)
        {
          xsize_t i;

          for (i = 0; i < serialised.size; i++)
            if (randomly (*fuzziness))
              {
                serialised.data[i] += g_test_rand_int_range (1, 256);
                fuzzed = TRUE;
              }
        }

      /* at least one byte in the serialized data has changed.
       *
       * this means that at least one of the following is true:
       *
       *    - the serialized data now represents a different value:
       *        check_tree() will return FALSE
       *
       *    - the serialized data is in non-normal form:
       *        xvariant_serialiser_is_normal() will return FALSE
       *
       * we always do both checks to increase exposure of the serializer
       * to corrupt data.
       */
      a = xvariant_serialised_is_normal (serialised);
      b = check_tree (tree, serialised);

      g_assert_true (!a || !b);
    }

  tree_instance_free (tree);
  g_free (serialised.data);
}


static void
test_fuzzes (xpointer_t data)
{
  xdouble_t fuzziness;
  int i;

  fuzziness = GPOINTER_TO_INT (data) / 100.;

  for (i = 0; i < 200; i++)
    test_fuzz (&fuzziness);

  xvariant_type_info_assert_no_infos ();
}

static xvariant_t *
tree_instance_get_gvariant (TreeInstance *tree)
{
  const xvariant_type_t *type;
  xvariant_t *result;

  type = (xvariant_type_t *) xvariant_type_info_get_type_string (tree->info);

  switch (xvariant_type_info_get_type_char (tree->info))
    {
    case G_VARIANT_TYPE_INFO_CHAR_MAYBE:
      {
        const xvariant_type_t *child_type;
        xvariant_t *child;

        if (tree->n_children)
          child = tree_instance_get_gvariant (tree->children[0]);
        else
          child = NULL;

        child_type = xvariant_type_element (type);

        if (child != NULL && randomly (0.5))
          child_type = NULL;

        result = xvariant_new_maybe (child_type, child);
      }
      break;

    case G_VARIANT_TYPE_INFO_CHAR_ARRAY:
      {
        const xvariant_type_t *child_type;
        xvariant_t **children;
        xsize_t i;

        children = g_new (xvariant_t *, tree->n_children);
        for (i = 0; i < tree->n_children; i++)
          children[i] = tree_instance_get_gvariant (tree->children[i]);

        child_type = xvariant_type_element (type);

        if (i > 0 && randomly (0.5))
          child_type = NULL;

        result = xvariant_new_array (child_type, children, tree->n_children);
        g_free (children);
      }
      break;

    case G_VARIANT_TYPE_INFO_CHAR_TUPLE:
      {
        xvariant_t **children;
        xsize_t i;

        children = g_new (xvariant_t *, tree->n_children);
        for (i = 0; i < tree->n_children; i++)
          children[i] = tree_instance_get_gvariant (tree->children[i]);

        result = xvariant_new_tuple (children, tree->n_children);
        g_free (children);
      }
      break;

    case G_VARIANT_TYPE_INFO_CHAR_DICT_ENTRY:
      {
        xvariant_t *key, *val;

        g_assert_cmpuint (tree->n_children, ==, 2);

        key = tree_instance_get_gvariant (tree->children[0]);
        val = tree_instance_get_gvariant (tree->children[1]);

        result = xvariant_new_dict_entry (key, val);
      }
      break;

    case G_VARIANT_TYPE_INFO_CHAR_VARIANT:
      {
        xvariant_t *value;

        g_assert_cmpuint (tree->n_children, ==, 1);

        value = tree_instance_get_gvariant (tree->children[0]);
        result = xvariant_new_variant (value);
      }
      break;

    case 'b':
      result = xvariant_new_boolean (tree->data.integer > 0);
      break;

    case 'y':
      result = xvariant_new_byte (tree->data.integer);
      break;

    case 'n':
      result = xvariant_new_int16 (tree->data.integer);
      break;

    case 'q':
      result = xvariant_new_uint16 (tree->data.integer);
      break;

    case 'i':
      result = xvariant_new_int32 (tree->data.integer);
      break;

    case 'u':
      result = xvariant_new_uint32 (tree->data.integer);
      break;

    case 'x':
      result = xvariant_new_int64 (tree->data.integer);
      break;

    case 't':
      result = xvariant_new_uint64 (tree->data.integer);
      break;

    case 'h':
      result = xvariant_new_handle (tree->data.integer);
      break;

    case 'd':
      result = xvariant_new_double (tree->data.floating);
      break;

    case 's':
      result = xvariant_new_string (tree->data.string);
      break;

    case 'o':
      result = xvariant_new_object_path (tree->data.string);
      break;

    case 'g':
      result = xvariant_new_signature (tree->data.string);
      break;

    default:
      g_assert_not_reached ();
    }

  return result;
}

static xboolean_t
tree_instance_check_gvariant (TreeInstance *tree,
                              xvariant_t     *value)
{
  const xvariant_type_t *type;

  type = (xvariant_type_t *) xvariant_type_info_get_type_string (tree->info);
  g_assert_true (xvariant_is_of_type (value, type));

  switch (xvariant_type_info_get_type_char (tree->info))
    {
    case G_VARIANT_TYPE_INFO_CHAR_MAYBE:
      {
        xvariant_t *child;
        xboolean_t equal;

        child = xvariant_get_maybe (value);

        if (child != NULL && tree->n_children == 1)
          equal = tree_instance_check_gvariant (tree->children[0], child);
        else if (child == NULL && tree->n_children == 0)
          equal = TRUE;
        else
          equal = FALSE;

        if (child != NULL)
          xvariant_unref (child);

        return equal;
      }
      break;

    case G_VARIANT_TYPE_INFO_CHAR_ARRAY:
    case G_VARIANT_TYPE_INFO_CHAR_TUPLE:
    case G_VARIANT_TYPE_INFO_CHAR_DICT_ENTRY:
      {
        xsize_t i;

        if (xvariant_n_children (value) != tree->n_children)
          return FALSE;

        for (i = 0; i < tree->n_children; i++)
          {
            xvariant_t *child;
            xboolean_t equal;

            child = xvariant_get_child_value (value, i);
            equal = tree_instance_check_gvariant (tree->children[i], child);
            xvariant_unref (child);

            if (!equal)
              return FALSE;
          }

        return TRUE;
      }
      break;

    case G_VARIANT_TYPE_INFO_CHAR_VARIANT:
      {
        const xchar_t *str1, *str2;
        xvariant_t *child;
        xboolean_t equal;

        child = xvariant_get_variant (value);
        str1 = xvariant_get_type_string (child);
        str2 = xvariant_type_info_get_type_string (tree->children[0]->info);
        /* xvariant_t only keeps one copy of type strings around */
        equal = str1 == str2 &&
                tree_instance_check_gvariant (tree->children[0], child);

        xvariant_unref (child);

        return equal;
      }
      break;

    case 'b':
      return xvariant_get_boolean (value) == (xboolean_t) tree->data.integer;

    case 'y':
      return xvariant_get_byte (value) == (xuchar_t) tree->data.integer;

    case 'n':
      return xvariant_get_int16 (value) == (gint16) tree->data.integer;

    case 'q':
      return xvariant_get_uint16 (value) == (xuint16_t) tree->data.integer;

    case 'i':
      return xvariant_get_int32 (value) == (gint32) tree->data.integer;

    case 'u':
      return xvariant_get_uint32 (value) == (xuint32_t) tree->data.integer;

    case 'x':
      return xvariant_get_int64 (value) == (sint64_t) tree->data.integer;

    case 't':
      return xvariant_get_uint64 (value) == (xuint64_t) tree->data.integer;

    case 'h':
      return xvariant_get_handle (value) == (gint32) tree->data.integer;

    case 'd':
      {
        xdouble_t floating = xvariant_get_double (value);

        return memcmp (&floating, &tree->data.floating, sizeof floating) == 0;
      }

    case 's':
    case 'o':
    case 'g':
      return strcmp (xvariant_get_string (value, NULL),
                     tree->data.string) == 0;

    default:
      g_assert_not_reached ();
    }
}

static void
tree_instance_build_gvariant (TreeInstance    *tree,
                              xvariant_builder_t *builder,
                              xboolean_t         guess_ok)
{
  const xvariant_type_t *type;

  type = (xvariant_type_t *) xvariant_type_info_get_type_string (tree->info);

  if (xvariant_type_is_container (type))
    {
      xsize_t i;

      /* force xvariant_builder_t to guess the type half the time */
      if (guess_ok && randomly (0.5))
        {
          if (xvariant_type_is_array (type) && tree->n_children)
            type = G_VARIANT_TYPE_ARRAY;

          if (xvariant_type_is_maybe (type) && tree->n_children)
            type = G_VARIANT_TYPE_MAYBE;

          if (xvariant_type_is_tuple (type))
            type = G_VARIANT_TYPE_TUPLE;

          if (xvariant_type_is_dict_entry (type))
            type = G_VARIANT_TYPE_DICT_ENTRY;
        }
      else
        guess_ok = FALSE;

      xvariant_builder_open (builder, type);

      for (i = 0; i < tree->n_children; i++)
        tree_instance_build_gvariant (tree->children[i], builder, guess_ok);

      xvariant_builder_close (builder);
    }
  else
    xvariant_builder_add_value (builder, tree_instance_get_gvariant (tree));
}


static xboolean_t
tree_instance_check_iter (TreeInstance *tree,
                          xvariant_iter_t *iter)
{
  xvariant_t *value;

  value = xvariant_iter_next_value (iter);

  if (xvariant_is_container (value))
    {
      xsize_t i;

      iter = xvariant_iter_new (value);
      xvariant_unref (value);

      if (xvariant_iter_n_children (iter) != tree->n_children)
        {
          xvariant_iter_free (iter);
          return FALSE;
        }

      for (i = 0; i < tree->n_children; i++)
        if (!tree_instance_check_iter (tree->children[i], iter))
          {
            xvariant_iter_free (iter);
            return FALSE;
          }

      g_assert_null (xvariant_iter_next_value (iter));
      xvariant_iter_free (iter);

      return TRUE;
    }

  else
    {
      xboolean_t equal;

      equal = tree_instance_check_gvariant (tree, value);
      xvariant_unref (value);

      return equal;
    }
}

static void
test_container (void)
{
  TreeInstance *tree;
  xvariant_t *value;
  xchar_t *s1, *s2;

  tree = tree_instance_new (NULL, 3);
  value = xvariant_ref_sink (tree_instance_get_gvariant (tree));

  s1 = xvariant_print (value, TRUE);
  g_assert_true (tree_instance_check_gvariant (tree, value));

  xvariant_get_data (value);

  s2 = xvariant_print (value, TRUE);
  g_assert_true (tree_instance_check_gvariant (tree, value));

  g_assert_cmpstr (s1, ==, s2);

  if (xvariant_is_container (value))
    {
      xvariant_builder_t builder;
      xvariant_iter_t iter;
      xvariant_t *built;
      xvariant_t *val;
      xchar_t *s3;

      xvariant_builder_init (&builder, G_VARIANT_TYPE_VARIANT);
      tree_instance_build_gvariant (tree, &builder, TRUE);
      built = xvariant_builder_end (&builder);
      xvariant_ref_sink (built);
      xvariant_get_data (built);
      val = xvariant_get_variant (built);

      s3 = xvariant_print (val, TRUE);
      g_assert_cmpstr (s1, ==, s3);

      xvariant_iter_init (&iter, built);
      g_assert_true (tree_instance_check_iter (tree, &iter));
      g_assert_null (xvariant_iter_next_value (&iter));

      xvariant_unref (built);
      xvariant_unref (val);
      g_free (s3);
    }

  tree_instance_free (tree);
  xvariant_unref (value);
  g_free (s2);
  g_free (s1);
}

static void
test_string (void)
{
  /* test_t some different methods of creating strings */
  xvariant_t *v;

  v = xvariant_new_string ("foo");
  g_assert_cmpstr (xvariant_get_string (v, NULL), ==, "foo");
  xvariant_unref (v);


  v = xvariant_new_take_string (xstrdup ("foo"));
  g_assert_cmpstr (xvariant_get_string (v, NULL), ==, "foo");
  xvariant_unref (v);

  v = xvariant_new_printf ("%s %d", "foo", 123);
  g_assert_cmpstr (xvariant_get_string (v, NULL), ==, "foo 123");
  xvariant_unref (v);
}

static void
test_utf8 (void)
{
  const xchar_t invalid[] = "hello\xffworld";
  xvariant_t *value;

  /* ensure that the test data is not valid utf8... */
  g_assert_false (xutf8_validate (invalid, -1, NULL));

  /* load the data untrusted */
  value = xvariant_new_from_data (G_VARIANT_TYPE_STRING,
                                   invalid, sizeof invalid,
                                   FALSE, NULL, NULL);

  /* ensure that the problem is caught and we get valid UTF-8 */
  g_assert_true (xutf8_validate (xvariant_get_string (value, NULL), -1, NULL));
  xvariant_unref (value);


  /* now load it trusted */
  value = xvariant_new_from_data (G_VARIANT_TYPE_STRING,
                                   invalid, sizeof invalid,
                                   TRUE, NULL, NULL);

  /* ensure we get the invalid data (ie: make sure that time wasn't
   * wasted on validating data that was marked as trusted)
   */
  g_assert_true (xvariant_get_string (value, NULL) == invalid);
  xvariant_unref (value);
}

static void
test_containers (void)
{
  xsize_t i;

  for (i = 0; i < 100; i++)
    {
      test_container ();
    }

  xvariant_type_info_assert_no_infos ();
}

static void
test_format_strings (void)
{
  xvariant_type_t *type;
  const xchar_t *end;

  g_assert_true (xvariant_format_string_scan ("i", NULL, &end) && *end == '\0');
  g_assert_true (xvariant_format_string_scan ("@i", NULL, &end) && *end == '\0');
  g_assert_true (xvariant_format_string_scan ("@ii", NULL, &end) && *end == 'i');
  g_assert_true (xvariant_format_string_scan ("^a&s", NULL, &end) && *end == '\0');
  g_assert_true (xvariant_format_string_scan ("(^as)", NULL, &end) &&
                 *end == '\0');
  g_assert_false (xvariant_format_string_scan ("(^s)", NULL, &end));
  g_assert_false (xvariant_format_string_scan ("(^a)", NULL, &end));
  g_assert_false (xvariant_format_string_scan ("(z)", NULL, &end));
  g_assert_false (xvariant_format_string_scan ("az", NULL, &end));
  g_assert_false (xvariant_format_string_scan ("{**}", NULL, &end));
  g_assert_false (xvariant_format_string_scan ("{@**}", NULL, &end));
  g_assert_true (xvariant_format_string_scan ("{@y*}", NULL, &end) &&
                 *end == '\0');
  g_assert_true (xvariant_format_string_scan ("{yv}", NULL, &end) &&
                 *end == '\0');
  g_assert_false (xvariant_format_string_scan ("{&?v}", NULL, &end));
  g_assert_true (xvariant_format_string_scan ("{@?v}", NULL, &end) &&
                 *end == '\0');
  g_assert_false (xvariant_format_string_scan ("{&@sv}", NULL, &end));
  g_assert_false (xvariant_format_string_scan ("{@&sv}", NULL, &end));
  g_assert_true (xvariant_format_string_scan ("{&sv}", NULL, &end) &&
                 *end == '\0');
  g_assert_false (xvariant_format_string_scan ("{vv}", NULL, &end));
  g_assert_false (xvariant_format_string_scan ("{y}", NULL, &end));
  g_assert_false (xvariant_format_string_scan ("{yyy}", NULL, &end));
  g_assert_false (xvariant_format_string_scan ("{ya}", NULL, &end));
  g_assert_true (xvariant_format_string_scan ("&s", NULL, &end) && *end == '\0');
  g_assert_false (xvariant_format_string_scan ("&as", NULL, &end));
  g_assert_false (xvariant_format_string_scan ("@z", NULL, &end));
  g_assert_false (xvariant_format_string_scan ("az", NULL, &end));
  g_assert_false (xvariant_format_string_scan ("a&s", NULL, &end));

  type = xvariant_format_string_scan_type ("mm(@xy^a&s*?@?)", NULL, &end);
  g_assert_true (type && *end == '\0');
  g_assert_true (xvariant_type_equal (type, G_VARIANT_TYPE ("mm(xyas*?\?)")));
  xvariant_type_free (type);

  type = xvariant_format_string_scan_type ("mm(@xy^a&*?@?)", NULL, NULL);
  g_assert_null (type);
}

static void
do_failed_test (const char *test,
                const xchar_t *pattern)
{
  g_test_trap_subprocess (test, 1000000, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr (pattern);
}

static void
test_invalid_varargs (void)
{
  xvariant_t *value;
  const xchar_t *end;

  if (!g_test_undefined ())
    return;

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*xvariant_t format string*");
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*valid_format_string*");
  value = xvariant_new ("z");
  g_test_assert_expected_messages ();
  g_assert_null (value);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*valid xvariant_t format string as a prefix*");
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*valid_format_string*");
  value = xvariant_new_va ("z", &end, NULL);
  g_test_assert_expected_messages ();
  g_assert_null (value);

  value = xvariant_new ("y", 'a');
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*type of 'q' but * has a type of 'y'*");
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*valid_format_string*");
  xvariant_get (value, "q");
  g_test_assert_expected_messages ();
  xvariant_unref (value);
}

static void
check_and_free (xvariant_t    *value,
                const xchar_t *str)
{
  xchar_t *valstr = xvariant_print (value, FALSE);
  g_assert_cmpstr (str, ==, valstr);
  xvariant_unref (value);
  g_free (valstr);
}

static void
test_varargs_empty_array (void)
{
  xvariant_new ("(a{s*})", NULL);

  g_assert_not_reached ();
}

static void
test_varargs (void)
{
  {
    xvariant_builder_t array;

    xvariant_builder_init (&array, G_VARIANT_TYPE_ARRAY);
    xvariant_builder_add_parsed (&array, "{'size', <(%i, %i)> }", 800, 600);
    xvariant_builder_add (&array, "{sv}", "title",
                           xvariant_new_string ("test_t case"));
    xvariant_builder_add_value (&array,
      xvariant_new_dict_entry (xvariant_new_string ("temperature"),
                                xvariant_new_variant (
                                  xvariant_new_double (37.5))));
    check_and_free (xvariant_new ("(ma{sv}m(a{sv})ma{sv}ii)",
                                   NULL, FALSE, NULL, &array, 7777, 8888),
                    "(nothing, nothing, {'size': <(800, 600)>, "
                                        "'title': <'test_t case'>, "
                                        "'temperature': <37.5>}, "
                     "7777, 8888)");

    check_and_free (xvariant_new ("(imimimmimmimmi)",
                                   123,
                                   FALSE, 321,
                                   TRUE, 123,
                                   FALSE, TRUE, 321,
                                   TRUE, FALSE, 321,
                                   TRUE, TRUE, 123),
                    "(123, nothing, 123, nothing, just nothing, 123)");

    check_and_free (xvariant_new ("(ybnixd)",
                                   'a', 1, 22, 33, (xuint64_t) 44, 5.5),
                    "(0x61, true, 22, 33, 44, 5.5)");

    check_and_free (xvariant_new ("(@y?*rv)",
                                   xvariant_new ("y", 'a'),
                                   xvariant_new ("y", 'b'),
                                   xvariant_new ("y", 'c'),
                                   xvariant_new ("(y)", 'd'),
                                   xvariant_new ("y", 'e')),
                    "(0x61, 0x62, 0x63, (0x64,), <byte 0x65>)");
  }

  {
    xvariant_builder_t array;
    xvariant_iter_t iter;
    xvariant_t *value;
    xchar_t *number;
    xboolean_t just;
    xuint_t i;
    xint_t val;

    xvariant_builder_init (&array, G_VARIANT_TYPE_ARRAY);
    for (i = 0; i < 100; i++)
      {
        number = xstrdup_printf ("%u", i);
        xvariant_builder_add (&array, "s", number);
        g_free (number);
      }

    value = xvariant_builder_end (&array);
    xvariant_iter_init (&iter, value);

    i = 0;
    while (xvariant_iter_loop (&iter, "s", &number))
      {
        xchar_t *check = xstrdup_printf ("%u", i++);
        g_assert_cmpstr (number, ==, check);
        g_free (check);
      }
    g_assert_null (number);
    g_assert_cmpuint (i, ==, 100);

    xvariant_unref (value);

    xvariant_builder_init (&array, G_VARIANT_TYPE_ARRAY);
    for (i = 0; i < 100; i++)
      xvariant_builder_add (&array, "mi", i % 2 == 0, i);
    value = xvariant_builder_end (&array);

    i = 0;
    xvariant_iter_init (&iter, value);
    while (xvariant_iter_loop (&iter, "mi", NULL, &val))
      g_assert_true (val == (xint_t) i++ || val == 0);
    g_assert_cmpuint (i, ==, 100);

    i = 0;
    xvariant_iter_init (&iter, value);
    while (xvariant_iter_loop (&iter, "mi", &just, &val))
      {
        xint_t this = i++;

        if (this % 2 == 0)
          {
            g_assert_true (just);
            g_assert_cmpint (val, ==, this);
          }
        else
          {
            g_assert_false (just);
            g_assert_cmpint (val, ==, 0);
          }
      }
    g_assert_cmpuint (i, ==, 100);

    xvariant_unref (value);
  }

  {
    const xchar_t *strvector[] = {"/hello", "/world", NULL};
    const xchar_t *test_strs[] = {"/foo", "/bar", "/baz" };
    xvariant_builder_t builder;
    xvariant_iter_t *array;
    xvariant_iter_t tuple;
    const xchar_t **strv;
    xchar_t **my_strv;
    xvariant_t *value;
    xchar_t *str;
    xsize_t i;

    xvariant_builder_init (&builder, G_VARIANT_TYPE ("as"));
    xvariant_builder_add (&builder, "s", test_strs[0]);
    xvariant_builder_add (&builder, "s", test_strs[1]);
    xvariant_builder_add (&builder, "s", test_strs[2]);
    value = xvariant_new ("(as^as^a&s)", &builder, strvector, strvector);
    xvariant_iter_init (&tuple, value);
    xvariant_iter_next (&tuple, "as", &array);

    i = 0;
    while (xvariant_iter_loop (array, "s", &str))
      g_assert_cmpstr (str, ==, test_strs[i++]);
    g_assert_cmpuint (i, ==, 3);

    xvariant_iter_free (array);

    /* start over */
    xvariant_iter_init (&tuple, value);
    xvariant_iter_next (&tuple, "as", &array);

    i = 0;
    while (xvariant_iter_loop (array, "&s", &str))
      g_assert_cmpstr (str, ==, test_strs[i++]);
    g_assert_cmpuint (i, ==, 3);

    xvariant_iter_free (array);

    xvariant_iter_next (&tuple, "^a&s", &strv);
    xvariant_iter_next (&tuple, "^as", &my_strv);

    g_assert_cmpstrv (strv, strvector);
    g_assert_cmpstrv (my_strv, strvector);

    xvariant_unref (value);
    xstrfreev (my_strv);
    g_free (strv);
  }

  {
    const xchar_t *strvector[] = {"/hello", "/world", NULL};
    const xchar_t *test_strs[] = {"/foo", "/bar", "/baz" };
    xvariant_builder_t builder;
    xvariant_iter_t *array;
    xvariant_iter_t tuple;
    const xchar_t **strv;
    xchar_t **my_strv;
    xvariant_t *value;
    xchar_t *str;
    xsize_t i;

    xvariant_builder_init (&builder, G_VARIANT_TYPE ("aaay"));
    xvariant_builder_add (&builder, "^aay", strvector);
    xvariant_builder_add (&builder, "^aay", strvector);
    xvariant_builder_add (&builder, "^aay", strvector);
    value = xvariant_new ("aaay", &builder);
    array = xvariant_iter_new (value);
    i = 0;
    while (xvariant_iter_loop (array, "^aay", &my_strv))
      i++;
    g_assert_cmpuint (i, ==, 3);

    /* start over */
    xvariant_iter_init (array, value);
    i = 0;
    while (xvariant_iter_loop (array, "^a&ay", &strv))
      i++;
    g_assert_cmpuint (i, ==, 3);
    xvariant_unref (value);
    xvariant_iter_free (array);

    /* next test */
    xvariant_builder_init (&builder, G_VARIANT_TYPE ("aay"));
    xvariant_builder_add (&builder, "^ay", test_strs[0]);
    xvariant_builder_add (&builder, "^ay", test_strs[1]);
    xvariant_builder_add (&builder, "^ay", test_strs[2]);
    value = xvariant_new ("(aay^aay^a&ay)", &builder, strvector, strvector);
    xvariant_iter_init (&tuple, value);
    xvariant_iter_next (&tuple, "aay", &array);

    i = 0;
    while (xvariant_iter_loop (array, "^ay", &str))
      g_assert_cmpstr (str, ==, test_strs[i++]);
    g_assert_cmpuint (i, ==, 3);

    xvariant_iter_free (array);

    /* start over */
    xvariant_iter_init (&tuple, value);
    xvariant_iter_next (&tuple, "aay", &array);

    i = 0;
    while (xvariant_iter_loop (array, "^&ay", &str))
      g_assert_cmpstr (str, ==, test_strs[i++]);
    g_assert_cmpuint (i, ==, 3);

    xvariant_iter_free (array);

    xvariant_iter_next (&tuple, "^a&ay", &strv);
    xvariant_iter_next (&tuple, "^aay", &my_strv);

    g_assert_cmpstrv (strv, strvector);
    g_assert_cmpstrv (my_strv, strvector);

    xvariant_unref (value);
    xstrfreev (my_strv);
    g_free (strv);
  }

  {
    const xchar_t *strvector[] = {"/hello", "/world", NULL};
    const xchar_t *test_strs[] = {"/foo", "/bar", "/baz" };
    xvariant_builder_t builder;
    xvariant_iter_t *array;
    xvariant_iter_t tuple;
    const xchar_t **strv;
    xchar_t **my_strv;
    xvariant_t *value;
    xchar_t *str;
    xsize_t i;

    xvariant_builder_init (&builder, G_VARIANT_TYPE_OBJECT_PATH_ARRAY);
    xvariant_builder_add (&builder, "o", test_strs[0]);
    xvariant_builder_add (&builder, "o", test_strs[1]);
    xvariant_builder_add (&builder, "o", test_strs[2]);
    value = xvariant_new ("(ao^ao^a&o)", &builder, strvector, strvector);
    xvariant_iter_init (&tuple, value);
    xvariant_iter_next (&tuple, "ao", &array);

    i = 0;
    while (xvariant_iter_loop (array, "o", &str))
      g_assert_cmpstr (str, ==, test_strs[i++]);
    g_assert_cmpuint (i, ==, 3);

    xvariant_iter_free (array);

    /* start over */
    xvariant_iter_init (&tuple, value);
    xvariant_iter_next (&tuple, "ao", &array);

    i = 0;
    while (xvariant_iter_loop (array, "&o", &str))
      g_assert_cmpstr (str, ==, test_strs[i++]);
    g_assert_cmpuint (i, ==, 3);

    xvariant_iter_free (array);

    xvariant_iter_next (&tuple, "^a&o", &strv);
    xvariant_iter_next (&tuple, "^ao", &my_strv);

    g_assert_cmpstrv (strv, strvector);
    g_assert_cmpstrv (my_strv, strvector);

    xvariant_unref (value);
    xstrfreev (my_strv);
    g_free (strv);
  }

  {
    const xchar_t *strvector[] = { "i", "ii", "iii", "iv", "v", "vi", NULL };
    xvariant_builder_t builder;
    xvariant_iter_t iter;
    xvariant_iter_t *i2;
    xvariant_iter_t *i3;
    xvariant_t *value;
    xvariant_t *sub;
    xchar_t **strv;
    xsize_t i;

    xvariant_builder_init (&builder, G_VARIANT_TYPE ("aas"));
    xvariant_builder_open (&builder, G_VARIANT_TYPE ("as"));
    for (i = 0; i < 6; i++)
      if (i & 1)
        xvariant_builder_add (&builder, "s", strvector[i]);
      else
        xvariant_builder_add (&builder, "&s", strvector[i]);
    xvariant_builder_close (&builder);
    xvariant_builder_add (&builder, "^as", strvector);
    xvariant_builder_add (&builder, "^as", strvector);
    value = xvariant_new ("aas", &builder);

    xvariant_iter_init (&iter, value);
    while (xvariant_iter_loop (&iter, "^as", &strv))
      for (i = 0; i < 6; i++)
        g_assert_cmpstr (strv[i], ==, strvector[i]);

    xvariant_iter_init (&iter, value);
    while (xvariant_iter_loop (&iter, "^a&s", &strv))
      for (i = 0; i < 6; i++)
        g_assert_cmpstr (strv[i], ==, strvector[i]);

    xvariant_iter_init (&iter, value);
    while (xvariant_iter_loop (&iter, "as", &i2))
      {
        xchar_t *str;

        i = 0;
        while (xvariant_iter_loop (i2, "s", &str))
          g_assert_cmpstr (str, ==, strvector[i++]);
        g_assert_cmpuint (i, ==, 6);
      }

    xvariant_iter_init (&iter, value);
    i3 = xvariant_iter_copy (&iter);
    while (xvariant_iter_loop (&iter, "@as", &sub))
      {
        xchar_t *str = xvariant_print (sub, TRUE);
        g_assert_cmpstr (str, ==,
                         "['i', 'ii', 'iii', 'iv', 'v', 'vi']");
        g_free (str);
      }

    g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                           "*NULL has already been returned*");
    xvariant_iter_next_value (&iter);
    g_test_assert_expected_messages ();

    while (xvariant_iter_loop (i3, "*", &sub))
      {
        xchar_t *str = xvariant_print (sub, TRUE);
        g_assert_cmpstr (str, ==,
                         "['i', 'ii', 'iii', 'iv', 'v', 'vi']");
        g_free (str);
      }

    xvariant_iter_free (i3);

    for (i = 0; i < xvariant_n_children (value); i++)
      {
        xsize_t j;

        xvariant_get_child (value, i, "*", &sub);

        for (j = 0; j < xvariant_n_children (sub); j++)
          {
            const xchar_t *str = NULL;
            xvariant_t *cval;

            xvariant_get_child (sub, j, "&s", &str);
            g_assert_cmpstr (str, ==, strvector[j]);

            cval = xvariant_get_child_value (sub, j);
            xvariant_get (cval, "&s", &str);
            g_assert_cmpstr (str, ==, strvector[j]);
            xvariant_unref (cval);
          }

        xvariant_unref (sub);
      }

    xvariant_unref (value);
  }

  {
    xboolean_t justs[10];
    xvariant_t *value;

    xvariant_t *vval;
    xuchar_t byteval;
    xboolean_t bval;
    gint16 i16val;
    xuint16_t u16val;
    gint32 i32val;
    xuint32_t u32val;
    sint64_t i64val;
    xuint64_t u64val;
    xdouble_t dval;
    gint32 hval;

    /* test all 'nothing' */
    value = xvariant_new ("(mymbmnmqmimumxmtmhmdmv)",
                           FALSE, 'a',
                           FALSE, TRUE,
                           FALSE, (gint16) 123,
                           FALSE, (xuint16_t) 123,
                           FALSE, (gint32) 123,
                           FALSE, (xuint32_t) 123,
                           FALSE, (sint64_t) 123,
                           FALSE, (xuint64_t) 123,
                           FALSE, (gint32) -1,
                           FALSE, (xdouble_t) 37.5,
                           NULL);

    /* both NULL */
    xvariant_get (value, "(mymbmnmqmimumxmtmhmdmv)",
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL);

    /* NULL values */
    memset (justs, 1, sizeof justs);
    xvariant_get (value, "(mymbmnmqmimumxmtmhmdmv)",
                   &justs[0], NULL,
                   &justs[1], NULL,
                   &justs[2], NULL,
                   &justs[3], NULL,
                   &justs[4], NULL,
                   &justs[5], NULL,
                   &justs[6], NULL,
                   &justs[7], NULL,
                   &justs[8], NULL,
                   &justs[9], NULL,
                   NULL);
    g_assert_true (!(justs[0] || justs[1] || justs[2] || justs[3] || justs[4] ||
                     justs[5] || justs[6] || justs[7] || justs[8] || justs[9]));

    /* both non-NULL */
    memset (justs, 1, sizeof justs);
    byteval = i16val = u16val = i32val = u32val = i64val = u64val = hval = 88;
    vval = (void *) 1;
    bval = TRUE;
    dval = 88.88;
    xvariant_get (value, "(mymbmnmqmimumxmtmhmdmv)",
                   &justs[0], &byteval,
                   &justs[1], &bval,
                   &justs[2], &i16val,
                   &justs[3], &u16val,
                   &justs[4], &i32val,
                   &justs[5], &u32val,
                   &justs[6], &i64val,
                   &justs[7], &u64val,
                   &justs[8], &hval,
                   &justs[9], &dval,
                   &vval);
    g_assert_true (!(justs[0] || justs[1] || justs[2] || justs[3] || justs[4] ||
                     justs[5] || justs[6] || justs[7] || justs[8] || justs[9]));
    g_assert_true (byteval == '\0' && bval == FALSE);
    g_assert_true (i16val == 0 && u16val == 0 && i32val == 0 &&
                   u32val == 0 && i64val == 0 && u64val == 0 &&
                   hval == 0 && dval == 0.0);
    g_assert_null (vval);

    /* NULL justs */
    byteval = i16val = u16val = i32val = u32val = i64val = u64val = hval = 88;
    vval = (void *) 1;
    bval = TRUE;
    dval = 88.88;
    xvariant_get (value, "(mymbmnmqmimumxmtmhmdmv)",
                   NULL, &byteval,
                   NULL, &bval,
                   NULL, &i16val,
                   NULL, &u16val,
                   NULL, &i32val,
                   NULL, &u32val,
                   NULL, &i64val,
                   NULL, &u64val,
                   NULL, &hval,
                   NULL, &dval,
                   &vval);
    g_assert_true (byteval == '\0' && bval == FALSE);
    g_assert_true (i16val == 0 && u16val == 0 && i32val == 0 &&
                   u32val == 0 && i64val == 0 && u64val == 0 &&
                   hval == 0 && dval == 0.0);
    g_assert_null (vval);

    xvariant_unref (value);


    /* test all 'just' */
    value = xvariant_new ("(mymbmnmqmimumxmtmhmdmv)",
                           TRUE, 'a',
                           TRUE, TRUE,
                           TRUE, (gint16) 123,
                           TRUE, (xuint16_t) 123,
                           TRUE, (gint32) 123,
                           TRUE, (xuint32_t) 123,
                           TRUE, (sint64_t) 123,
                           TRUE, (xuint64_t) 123,
                           TRUE, (gint32) -1,
                           TRUE, (xdouble_t) 37.5,
                           xvariant_new ("()"));

    /* both NULL */
    xvariant_get (value, "(mymbmnmqmimumxmtmhmdmv)",
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL, NULL,
                   NULL);

    /* NULL values */
    memset (justs, 0, sizeof justs);
    xvariant_get (value, "(mymbmnmqmimumxmtmhmdmv)",
                   &justs[0], NULL,
                   &justs[1], NULL,
                   &justs[2], NULL,
                   &justs[3], NULL,
                   &justs[4], NULL,
                   &justs[5], NULL,
                   &justs[6], NULL,
                   &justs[7], NULL,
                   &justs[8], NULL,
                   &justs[9], NULL,
                   NULL);
    g_assert_true (justs[0] && justs[1] && justs[2] && justs[3] && justs[4] &&
                   justs[5] && justs[6] && justs[7] && justs[8] && justs[9]);

    /* both non-NULL */
    memset (justs, 0, sizeof justs);
    byteval = i16val = u16val = i32val = u32val = i64val = u64val = hval = 88;
    vval = (void *) 1;
    bval = FALSE;
    dval = 88.88;
    xvariant_get (value, "(mymbmnmqmimumxmtmhmdmv)",
                   &justs[0], &byteval,
                   &justs[1], &bval,
                   &justs[2], &i16val,
                   &justs[3], &u16val,
                   &justs[4], &i32val,
                   &justs[5], &u32val,
                   &justs[6], &i64val,
                   &justs[7], &u64val,
                   &justs[8], &hval,
                   &justs[9], &dval,
                   &vval);
    g_assert_true (justs[0] && justs[1] && justs[2] && justs[3] && justs[4] &&
                   justs[5] && justs[6] && justs[7] && justs[8] && justs[9]);
    g_assert_true (byteval == 'a' && bval == TRUE);
    g_assert_true (i16val == 123 && u16val == 123 && i32val == 123 &&
                   u32val == 123 && i64val == 123 && u64val == 123 &&
                   hval == -1 && dval == 37.5);
    g_assert_true (xvariant_is_of_type (vval, G_VARIANT_TYPE_UNIT));
    xvariant_unref (vval);

    /* NULL justs */
    byteval = i16val = u16val = i32val = u32val = i64val = u64val = hval = 88;
    vval = (void *) 1;
    bval = TRUE;
    dval = 88.88;
    xvariant_get (value, "(mymbmnmqmimumxmtmhmdmv)",
                   NULL, &byteval,
                   NULL, &bval,
                   NULL, &i16val,
                   NULL, &u16val,
                   NULL, &i32val,
                   NULL, &u32val,
                   NULL, &i64val,
                   NULL, &u64val,
                   NULL, &hval,
                   NULL, &dval,
                   &vval);
    g_assert_true (byteval == 'a' && bval == TRUE);
    g_assert_true (i16val == 123 && u16val == 123 && i32val == 123 &&
                   u32val == 123 && i64val == 123 && u64val == 123 &&
                   hval == -1 && dval == 37.5);
    g_assert_true (xvariant_is_of_type (vval, G_VARIANT_TYPE_UNIT));
    xvariant_unref (vval);

    xvariant_unref (value);
  }

  {
    xvariant_t *value;
    xchar_t *str;

    value = xvariant_new ("(masas)", NULL, NULL);
    xvariant_ref_sink (value);

    str = xvariant_print (value, TRUE);
    g_assert_cmpstr (str, ==, "(@mas nothing, @as [])");
    xvariant_unref (value);
    g_free (str);

    do_failed_test ("/gvariant/varargs/subprocess/empty-array",
                    "*which type of empty array*");
  }

  xvariant_type_info_assert_no_infos ();
}

static void
hash_get (xvariant_t    *value,
          const xchar_t *format,
          ...)
{
  const xchar_t *endptr = NULL;
  xboolean_t hash;
  va_list ap;

  hash = xstr_has_suffix (format, "#");

  va_start (ap, format);
  xvariant_get_va (value, format, hash ? &endptr : NULL, &ap);
  va_end (ap);

  if (hash)
    g_assert_cmpint (*endptr, ==, '#');
}

static xvariant_t *
hash_new (const xchar_t *format,
          ...)
{
  const xchar_t *endptr = NULL;
  xvariant_t *value;
  xboolean_t hash;
  va_list ap;

  hash = xstr_has_suffix (format, "#");

  va_start (ap, format);
  value = xvariant_new_va (format, hash ? &endptr : NULL, &ap);
  va_end (ap);

  if (hash)
    g_assert_cmpint (*endptr, ==, '#');

  return value;
}

static void
test_valist (void)
{
  xvariant_t *value;
  gint32 x;

  x = 0;
  value = hash_new ("i", 234);
  hash_get (value, "i", &x);
  g_assert_cmpint (x, ==, 234);
  xvariant_unref (value);

  x = 0;
  value = hash_new ("i#", 234);
  hash_get (value, "i#", &x);
  g_assert_cmpint (x, ==, 234);
  xvariant_unref (value);

  xvariant_type_info_assert_no_infos ();
}

static void
test_builder_memory (void)
{
  xvariant_builder_t *hb;
  xvariant_builder_t sb;

  hb = xvariant_builder_new  (G_VARIANT_TYPE_ARRAY);
  xvariant_builder_open (hb, G_VARIANT_TYPE_ARRAY);
  xvariant_builder_open (hb, G_VARIANT_TYPE_ARRAY);
  xvariant_builder_open (hb, G_VARIANT_TYPE_ARRAY);
  xvariant_builder_add (hb, "s", "some value");
  xvariant_builder_ref (hb);
  xvariant_builder_unref (hb);
  xvariant_builder_unref (hb);

  hb = xvariant_builder_new (G_VARIANT_TYPE_ARRAY);
  xvariant_builder_unref (hb);

  hb = xvariant_builder_new (G_VARIANT_TYPE_ARRAY);
  xvariant_builder_clear (hb);
  xvariant_builder_unref (hb);

  xvariant_builder_init (&sb, G_VARIANT_TYPE_ARRAY);
  xvariant_builder_open (&sb, G_VARIANT_TYPE_ARRAY);
  xvariant_builder_open (&sb, G_VARIANT_TYPE_ARRAY);
  xvariant_builder_add (&sb, "s", "some value");
  xvariant_builder_clear (&sb);

  xvariant_type_info_assert_no_infos ();
}

static void
test_hashing (void)
{
  xvariant_t *items[4096];
  xhashtable_t *table;
  xsize_t i;

  table = xhash_table_new_full (xvariant_hash, xvariant_equal,
                                 (xdestroy_notify_t ) xvariant_unref,
                                 NULL);

  for (i = 0; i < G_N_ELEMENTS (items); i++)
    {
      TreeInstance *tree;
      xsize_t j;

 again:
      tree = tree_instance_new (NULL, 0);
      items[i] = tree_instance_get_gvariant (tree);
      tree_instance_free (tree);

      for (j = 0; j < i; j++)
        if (xvariant_equal (items[i], items[j]))
          {
            xvariant_unref (items[i]);
            goto again;
          }

      xhash_table_insert (table,
                           xvariant_ref_sink (items[i]),
                           GINT_TO_POINTER (i));
    }

  for (i = 0; i < G_N_ELEMENTS (items); i++)
    {
      xpointer_t result;

      result = xhash_table_lookup (table, items[i]);
      g_assert_cmpint (GPOINTER_TO_INT (result), ==, i);
    }

  xhash_table_unref (table);

  xvariant_type_info_assert_no_infos ();
}

static void
test_gv_byteswap (void)
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
# define native16(x)  x, 0
# define swapped16(x) 0, x
#else
# define native16(x)  0, x
# define swapped16(x) x, 0
#endif
  /* all kinds of of crazy randomised testing already performed on the
   * byteswapper in the /gvariant/serializer/byteswap test and all kinds
   * of crazy randomised testing performed against the serializer
   * normalisation functions in the /gvariant/serializer/fuzz/ tests.
   *
   * just test a few simple cases here to make sure they each work
   */
  xuchar_t validbytes[] = { 'a', '\0', swapped16(66), 2,
                          0,
                          'b', '\0', swapped16(77), 2,
                          5, 11 };
  xuchar_t corruptbytes[] = { 'a', '\0', swapped16(66), 2,
                            0,
                            'b', '\0', swapped16(77), 2,
                            6, 11 };
  xuint_t valid_data[4], corrupt_data[4];
  xvariant_t *value, *swapped;
  xchar_t *string, *string2;

  memcpy (valid_data, validbytes, sizeof validbytes);
  memcpy (corrupt_data, corruptbytes, sizeof corruptbytes);

  /* trusted */
  value = xvariant_new_from_data (G_VARIANT_TYPE ("a(sn)"),
                                   valid_data, sizeof validbytes, TRUE,
                                   NULL, NULL);
  swapped = xvariant_byteswap (value);
  xvariant_unref (value);
  g_assert_cmpuint (xvariant_get_size (swapped), ==, 13);
  string = xvariant_print (swapped, FALSE);
  xvariant_unref (swapped);
  g_assert_cmpstr (string, ==, "[('a', 66), ('b', 77)]");
  g_free (string);

  /* untrusted but valid */
  value = xvariant_new_from_data (G_VARIANT_TYPE ("a(sn)"),
                                   valid_data, sizeof validbytes, FALSE,
                                   NULL, NULL);
  swapped = xvariant_byteswap (value);
  xvariant_unref (value);
  g_assert_cmpuint (xvariant_get_size (swapped), ==, 13);
  string = xvariant_print (swapped, FALSE);
  xvariant_unref (swapped);
  g_assert_cmpstr (string, ==, "[('a', 66), ('b', 77)]");
  g_free (string);

  /* untrusted, invalid */
  value = xvariant_new_from_data (G_VARIANT_TYPE ("a(sn)"),
                                   corrupt_data, sizeof corruptbytes, FALSE,
                                   NULL, NULL);
  string = xvariant_print (value, FALSE);
  swapped = xvariant_byteswap (value);
  xvariant_unref (value);
  g_assert_cmpuint (xvariant_get_size (swapped), ==, 13);
  value = xvariant_byteswap (swapped);
  xvariant_unref (swapped);
  string2 = xvariant_print (value, FALSE);
  g_assert_cmpuint (xvariant_get_size (value), ==, 13);
  xvariant_unref (value);
  g_assert_cmpstr (string, ==, string2);
  g_free (string2);
  g_free (string);
}

static void
test_parser (void)
{
  TreeInstance *tree;
  xvariant_t *parsed;
  xvariant_t *value;
  xchar_t *pt, *p;
  xchar_t *res;

  tree = tree_instance_new (NULL, 3);
  value = tree_instance_get_gvariant (tree);
  tree_instance_free (tree);

  pt = xvariant_print (value, TRUE);
  p = xvariant_print (value, FALSE);

  parsed = xvariant_parse (NULL, pt, NULL, NULL, NULL);
  res = xvariant_print (parsed, FALSE);
  g_assert_cmpstr (p, ==, res);
  xvariant_unref (parsed);
  g_free (res);

  parsed = xvariant_parse (xvariant_get_type (value), p,
                            NULL, NULL, NULL);
  res = xvariant_print (parsed, TRUE);
  g_assert_cmpstr (pt, ==, res);
  xvariant_unref (parsed);
  g_free (res);

  xvariant_unref (value);
  g_free (pt);
  g_free (p);
}

static void
test_parses (void)
{
  xsize_t i;

  for (i = 0; i < 100; i++)
    {
      test_parser ();
    }

  /* mini test */
  {
    xerror_t *error = NULL;
    xchar_t str[128];
    xvariant_t *val;
    xchar_t *p, *p2;

    for (i = 0; i < 127; i++)
      str[i] = i + 1;
    str[i] = 0;

    val = xvariant_new_string (str);
    p = xvariant_print (val, FALSE);
    xvariant_unref (val);

    val = xvariant_parse (NULL, p, NULL, NULL, &error);
    p2 = xvariant_print (val, FALSE);

    g_assert_cmpstr (str, ==, xvariant_get_string (val, NULL));
    g_assert_cmpstr (p, ==, p2);

    xvariant_unref (val);
    g_free (p2);
    g_free (p);
  }

  /* another mini test */
  {
    const xchar_t *end;
    xvariant_t *value;

    value = xvariant_parse (G_VARIANT_TYPE_INT32, "1 2 3", NULL, &end, NULL);
    g_assert_cmpint (xvariant_get_int32 (value), ==, 1);
    /* make sure endptr returning works */
    g_assert_cmpstr (end, ==, " 2 3");
    xvariant_unref (value);
  }

  /* unicode mini test */
  {
    /* ał𝄞 */
    const xchar_t orig[] = "a\xc5\x82\xf0\x9d\x84\x9e \t\n";
    xvariant_t *value;
    xchar_t *printed;

    value = xvariant_new_string (orig);
    printed = xvariant_print (value, FALSE);
    xvariant_unref (value);

    g_assert_cmpstr (printed, ==, "'a\xc5\x82\xf0\x9d\x84\x9e \\t\\n'");
    value = xvariant_parse (NULL, printed, NULL, NULL, NULL);
    g_assert_cmpstr (xvariant_get_string (value, NULL), ==, orig);
    xvariant_unref (value);
    g_free (printed);
  }

  /* escapes */
  {
    const xchar_t orig[] = " \342\200\254 \360\220\210\240 \a \b \f \n \r \t \v ";
    xvariant_t *value;
    xchar_t *printed;

    value = xvariant_new_string (orig);
    printed = xvariant_print (value, FALSE);
    xvariant_unref (value);

    g_assert_cmpstr (printed, ==, "' \\u202c \\U00010220 \\a \\b \\f \\n \\r \\t \\v '");
    value = xvariant_parse (NULL, printed, NULL, NULL, NULL);
    g_assert_cmpstr (xvariant_get_string (value, NULL), ==, orig);
    xvariant_unref (value);
    g_free (printed);
  }

  /* pattern coalese of `MN` and `*` is `MN` */
  {
    xvariant_t *value = NULL;
    xerror_t *error = NULL;

    value = xvariant_parse (NULL, "[[0], [], [nothing]]", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpstr (xvariant_get_type_string (value), ==, "aami");
    xvariant_unref (value);
  }

#ifndef _MSC_VER
  /* inf/nan strings are C99 features which Visual C++ does not support */
  /* inf/nan mini test */
  {
    const xchar_t *tests[] = { "inf", "-inf", "nan" };
    xvariant_t *value;
    xchar_t *printed;
    xchar_t *printed_down;
    xsize_t i;

    for (i = 0; i < G_N_ELEMENTS (tests); i++)
      {
        xerror_t *error = NULL;
        value = xvariant_parse (NULL, tests[i], NULL, NULL, &error);
        printed = xvariant_print (value, FALSE);
        /* Canonicalize to lowercase; https://bugzilla.gnome.org/show_bug.cgi?id=704585 */
        printed_down = g_ascii_strdown (printed, -1);
        g_assert_true (xstr_has_prefix (printed_down, tests[i]));
        g_free (printed);
        g_free (printed_down);
        xvariant_unref (value);
      }
  }
#endif

  xvariant_type_info_assert_no_infos ();
}

static void
test_parse_failures (void)
{
  const xchar_t *test[] = {
    "[1, 2,",                   "6:",              "expected value",
    "",                         "0:",              "expected value",
    "(1, 2,",                   "6:",              "expected value",
    "<1",                       "2:",              "expected '>'",
    "[]",                       "0-2:",            "unable to infer",
    "(,",                       "1:",              "expected value",
    "[4,'']",                   "1-2,3-5:",        "common type",
    "[4, '', 5]",               "1-2,4-6:",        "common type",
    "['', 4, 5]",               "1-3,5-6:",        "common type",
    "[4, 5, '']",               "1-2,7-9:",        "common type",
    "[[4], [], ['']]",          "1-4,10-14:",      "common type",
    "[[], [4], ['']]",          "5-8,10-14:",      "common type",
    "just",                     "4:",              "expected value",
    "nothing",                  "0-7:",            "unable to infer",
    "just [4, '']",             "6-7,9-11:",       "common type",
    "[[4,'']]",                 "2-3,4-6:",        "common type",
    "([4,''],)",                "2-3,4-6:",        "common type",
    "(4)",                      "2:",              "','",
    "{}",                       "0-2:",            "unable to infer",
    "{[1,2],[3,4]}",            "0-13:",           "basic types",
    "{[1,2]:[3,4]}",            "0-13:",           "basic types",
    "justt",                    "0-5:",            "unknown keyword",
    "nothng",                   "0-6:",            "unknown keyword",
    "uint33",                   "0-6:",            "unknown keyword",
    "@mi just ''",              "9-11:",           "can not parse as",
    "@ai ['']",                 "5-7:",            "can not parse as",
    "@(i) ('',)",               "6-8:",            "can not parse as",
    "[[], 5]",                  "1-3,5-6:",        "common type",
    "[[5], 5]",                 "1-4,6-7:",        "common type",
    "5 5",                      "2:",              "expected end of input",
    "[5, [5, '']]",             "5-6,8-10:",       "common type",
    "@i just 5",                "3-9:",            "can not parse as",
    "@i nothing",               "3-10:",           "can not parse as",
    "@i []",                    "3-5:",            "can not parse as",
    "@i ()",                    "3-5:",            "can not parse as",
    "@ai (4,)",                 "4-8:",            "can not parse as",
    "@(i) []",                  "5-7:",            "can not parse as",
    "(5 5)",                    "3:",              "expected ','",
    "[5 5]",                    "3:",              "expected ',' or ']'",
    "(5, 5 5)",                 "6:",              "expected ',' or ')'",
    "[5, 5 5]",                 "6:",              "expected ',' or ']'",
    "<@i []>",                  "4-6:",            "can not parse as",
    "<[5 5]>",                  "4:",              "expected ',' or ']'",
    "{[4,''],5}",               "2-3,4-6:",        "common type",
    "{5,[4,'']}",               "4-5,6-8:",        "common type",
    "@i {1,2}",                 "3-8:",            "can not parse as",
    "{@i '', 5}",               "4-6:",            "can not parse as",
    "{5, @i ''}",               "7-9:",            "can not parse as",
    "@ai {}",                   "4-6:",            "can not parse as",
    "{@i '': 5}",               "4-6:",            "can not parse as",
    "{5: @i ''}",               "7-9:",            "can not parse as",
    "{<4,5}",                   "3:",              "expected '>'",
    "{4,<5}",                   "5:",              "expected '>'",
    "{4,5,6}",                  "4:",              "expected '}'",
    "{5 5}",                    "3:",              "expected ':' or ','",
    "{4: 5: 6}",                "5:",              "expected ',' or '}'",
    "{4:5,<6:7}",               "7:",              "expected '>'",
    "{4:5,6:<7}",               "9:",              "expected '>'",
    "{4:5,6 7}",                "7:",              "expected ':'",
    "@o 'foo'",                 "3-8:",            "object path",
    "@g 'zzz'",                 "3-8:",            "signature",
    "@i true",                  "3-7:",            "can not parse as",
    "@z 4",                     "0-2:",            "invalid type",
    "@a* []",                   "0-3:",            "definite",
    "@ai [3 3]",                "7:",              "expected ',' or ']'",
    "18446744073709551616",     "0-20:",           "too big for any type",
    "-18446744073709551616",    "0-21:",           "too big for any type",
    "byte 256",                 "5-8:",            "out of range for type",
    "byte -1",                  "5-7:",            "out of range for type",
    "int16 32768",              "6-11:",           "out of range for type",
    "int16 -32769",             "6-12:",           "out of range for type",
    "uint16 -1",                "7-9:",            "out of range for type",
    "uint16 65536",             "7-12:",           "out of range for type",
    "2147483648",               "0-10:",           "out of range for type",
    "-2147483649",              "0-11:",           "out of range for type",
    "uint32 -1",                "7-9:",            "out of range for type",
    "uint32 4294967296",        "7-17:",           "out of range for type",
    "@x 9223372036854775808",   "3-22:",           "out of range for type",
    "@x -9223372036854775809",  "3-23:",           "out of range for type",
    "@t -1",                    "3-5:",            "out of range for type",
    "@t 18446744073709551616",  "3-23:",           "too big for any type",
    "handle 2147483648",        "7-17:",           "out of range for type",
    "handle -2147483649",       "7-18:",           "out of range for type",
    "1.798e308",                "0-9:",            "too big for any type",
    "37.5a488",                 "4-5:",            "invalid character",
    "0x7ffgf",                  "5-6:",            "invalid character",
    "07758",                    "4-5:",            "invalid character",
    "123a5",                    "3-4:",            "invalid character",
    "@ai 123",                  "4-7:",            "can not parse as",
    "'\"\\'",                   "0-4:",            "unterminated string",
    "'\"\\'\\",                 "0-5:",            "unterminated string",
    "boolean 4",                "8-9:",            "can not parse as",
    "int32 true",               "6-10:",           "can not parse as",
    "[double 5, int32 5]",      "1-9,11-18:",      "common type",
    "string 4",                 "7-8:",            "can not parse as",
    "\x0a",                     "1:",              "expected value",
    "((",                       "2:",              "expected value",
    "(b",                       "1:",              "expected value",
    "b'",                       "0-2:",            "unterminated string constant",
    "b\"",                      "0-2:",            "unterminated string constant",
    "b'a",                      "0-3:",            "unterminated string constant",
    "b\"a",                     "0-3:",            "unterminated string constant",
    "b'\\",                     "0-3:",            "unterminated string constant",
    "b\"\\",                    "0-3:",            "unterminated string constant",
    "b'\\'",                    "0-4:",            "unterminated string constant",
    "b\"\\\"",                  "0-4:",            "unterminated string constant",
    "b'\\'a",                   "0-5:",            "unterminated string constant",
    "b\"\\\"a",                 "0-5:",            "unterminated string constant",
    "'\\u-ff4'",                "3:",              "invalid 4-character unicode escape",
    "'\\u+ff4'",                "3:",              "invalid 4-character unicode escape",
    "'\\u'",                    "3:",              "invalid 4-character unicode escape",
    "'\\u0'",                   "3-4:",            "invalid 4-character unicode escape",
    "'\\uHELLO'",               "3:",              "invalid 4-character unicode escape",
    "'\\u ff4'",                "3:",              "invalid 4-character unicode escape",
    "'\\u012'",                 "3-6:",            "invalid 4-character unicode escape",
    "'\\u0xff4'",               "3-4:",            "invalid 4-character unicode escape",
    "'\\U-ff4'",                "3:",              "invalid 8-character unicode escape",
    "'\\U+ff4'",                "3:",              "invalid 8-character unicode escape",
    "'\\U'",                    "3:",              "invalid 8-character unicode escape",
    "'\\U0'",                   "3-4:",            "invalid 8-character unicode escape",
    "'\\UHELLO'",               "3:",              "invalid 8-character unicode escape",
    "'\\U ff4'",                "3:",              "invalid 8-character unicode escape",
    "'\\U0123456'",             "3-10:",           "invalid 8-character unicode escape",
    "'\\U0xff4'",               "3-4:",            "invalid 8-character unicode escape",
  };
  xuint_t i;

  for (i = 0; i < G_N_ELEMENTS (test); i += 3)
    {
      xerror_t *error1 = NULL, *error2 = NULL;
      xvariant_t *value;

      /* Copy the test string and drop its nul terminator, then use the @limit
       * parameter of xvariant_parse() to set the length. This allows valgrind
       * to catch 1-byte heap buffer overflows. */
      xsize_t test_len = MAX (strlen (test[i]), 1);
      xchar_t *test_blob = g_malloc0 (test_len);  /* no nul terminator */

      memcpy (test_blob, test[i], test_len);
      value = xvariant_parse (NULL, test_blob, test_blob + test_len, NULL, &error1);
      g_assert_null (value);

      g_free (test_blob);

      if (!strstr (error1->message, test[i+2]))
        xerror ("test %u: Can't find '%s' in '%s'", i / 3,
                 test[i+2], error1->message);

      if (!xstr_has_prefix (error1->message, test[i+1]))
        xerror ("test %u: Expected location '%s' in '%s'", i / 3,
                 test[i+1], error1->message);

      /* test_t again with the nul terminator this time. The behaviour should be
       * the same. */
      value = xvariant_parse (NULL, test[i], NULL, NULL, &error2);
      g_assert_null (value);

      g_assert_cmpint (error1->domain, ==, error2->domain);
      g_assert_cmpint (error1->code, ==, error2->code);
      g_assert_cmpstr (error1->message, ==, error2->message);

      g_clear_error (&error1);
      g_clear_error (&error2);
    }
}

/* test_t that parsing xvariant_t text format integers works at the boundaries of
 * those integer types. We’re especially interested in the handling of the most
 * negative numbers, since those can’t be represented in sign + absolute value
 * form. */
static void
test_parser_integer_bounds (void)
{
  xvariant_t *value = NULL;
  xerror_t *local_error = NULL;

#define test_bound(TYPE, type, text, expected_value) \
  value = xvariant_parse (G_VARIANT_TYPE_##TYPE, text, NULL, NULL, &local_error); \
  g_assert_no_error (local_error); \
  g_assert_nonnull (value); \
  g_assert_true (xvariant_is_of_type (value, G_VARIANT_TYPE_##TYPE)); \
  g_assert_cmpint (xvariant_get_##type (value), ==, expected_value); \
  xvariant_unref (value)

  test_bound (BYTE, byte, "0", 0);
  test_bound (BYTE, byte, "255", G_MAXUINT8);
  test_bound (INT16, int16, "-32768", G_MININT16);
  test_bound (INT16, int16, "32767", G_MAXINT16);
  test_bound (INT32, int32, "-2147483648", G_MININT32);
  test_bound (INT32, int32, "2147483647", G_MAXINT32);
  test_bound (INT64, int64, "-9223372036854775808", G_MININT64);
  test_bound (INT64, int64, "9223372036854775807", G_MAXINT64);
  test_bound (HANDLE, handle, "-2147483648", G_MININT32);
  test_bound (HANDLE, handle, "2147483647", G_MAXINT32);

#undef test_bound
}

/* test_t that #GVariants which recurse too deeply are rejected. */
static void
test_parser_recursion (void)
{
  xvariant_t *value = NULL;
  xerror_t *local_error = NULL;
  const xuint_t recursion_depth = G_VARIANT_MAX_RECURSION_DEPTH + 1;
  xchar_t *silly_dict = g_malloc0 (recursion_depth * 2 + 1);
  xsize_t i;

  for (i = 0; i < recursion_depth; i++)
    {
      silly_dict[i] = '{';
      silly_dict[recursion_depth * 2 - i - 1] = '}';
    }

  value = xvariant_parse (NULL, silly_dict, NULL, NULL, &local_error);
  g_assert_error (local_error, G_VARIANT_PARSE_ERROR, G_VARIANT_PARSE_ERROR_RECURSION);
  g_assert_null (value);
  xerror_free (local_error);
  g_free (silly_dict);
}

static void
test_parse_bad_format_char (void)
{
  xvariant_new_parsed ("%z");

  g_assert_not_reached ();
}

static void
test_parse_bad_format_string (void)
{
  xvariant_new_parsed ("uint32 %i", 2);

  g_assert_not_reached ();
}

static void
test_parse_bad_args (void)
{
  xvariant_new_parsed ("%@i", xvariant_new_uint32 (2));

  g_assert_not_reached ();
}

static void
test_parse_positional (void)
{
  xvariant_t *value;
  check_and_free (xvariant_new_parsed ("[('one', 1), (%s, 2),"
                                        " ('three', %i)]", "two", 3),
                  "[('one', 1), ('two', 2), ('three', 3)]");
  value = xvariant_new_parsed ("[('one', 1), (%s, 2),"
                                " ('three', %u)]", "two", 3);
  g_assert_true (xvariant_is_of_type (value, G_VARIANT_TYPE ("a(su)")));
  check_and_free (value, "[('one', 1), ('two', 2), ('three', 3)]");
  check_and_free (xvariant_new_parsed ("{%s:%i}", "one", 1), "{'one': 1}");

  if (g_test_undefined ())
    {
      do_failed_test ("/gvariant/parse/subprocess/bad-format-char",
                      "*xvariant_t format string*");

      do_failed_test ("/gvariant/parse/subprocess/bad-format-string",
                      "*can not parse as*");

      do_failed_test ("/gvariant/parse/subprocess/bad-args",
                      "*expected xvariant_t of type 'i'*");
    }
}

static void
test_floating (void)
{
  xvariant_t *value;

  value = xvariant_new_int32 (42);
  g_assert_true (xvariant_is_floating (value));
  xvariant_ref_sink (value);
  g_assert_true (!xvariant_is_floating (value));
  xvariant_unref (value);
}

static void
test_bytestring (void)
{
  const xchar_t *test_string = "foo,bar,baz,quux,\xffoooo";
  xvariant_t *value;
  xchar_t **strv;
  xchar_t *str;
  const xchar_t *const_str;
  xvariant_t *untrusted_empty;

  strv = xstrsplit (test_string, ",", 0);

  value = xvariant_new_bytestring_array ((const xchar_t **) strv, -1);
  g_assert_true (xvariant_is_floating (value));
  xstrfreev (strv);

  str = xvariant_print (value, FALSE);
  xvariant_unref (value);

  value = xvariant_parse (NULL, str, NULL, NULL, NULL);
  g_free (str);

  strv = xvariant_dup_bytestring_array (value, NULL);
  xvariant_unref (value);

  str = xstrjoinv (",", strv);
  xstrfreev (strv);

  g_assert_cmpstr (str, ==, test_string);
  g_free (str);

  strv = xstrsplit (test_string, ",", 0);
  value = xvariant_new ("(^aay^a&ay^ay^&ay)",
                         strv, strv, strv[0], strv[0]);
  xstrfreev (strv);

  xvariant_get_child (value, 0, "^a&ay", &strv);
  str = xstrjoinv (",", strv);
  g_free (strv);
  g_assert_cmpstr (str, ==, test_string);
  g_free (str);

  xvariant_get_child (value, 0, "^aay", &strv);
  str = xstrjoinv (",", strv);
  xstrfreev (strv);
  g_assert_cmpstr (str, ==, test_string);
  g_free (str);

  xvariant_get_child (value, 1, "^a&ay", &strv);
  str = xstrjoinv (",", strv);
  g_free (strv);
  g_assert_cmpstr (str, ==, test_string);
  g_free (str);

  xvariant_get_child (value, 1, "^aay", &strv);
  str = xstrjoinv (",", strv);
  xstrfreev (strv);
  g_assert_cmpstr (str, ==, test_string);
  g_free (str);

  xvariant_get_child (value, 2, "^ay", &str);
  g_assert_cmpstr (str, ==, "foo");
  g_free (str);

  xvariant_get_child (value, 2, "^&ay", &str);
  g_assert_cmpstr (str, ==, "foo");

  xvariant_get_child (value, 3, "^ay", &str);
  g_assert_cmpstr (str, ==, "foo");
  g_free (str);

  xvariant_get_child (value, 3, "^&ay", &str);
  g_assert_cmpstr (str, ==, "foo");
  xvariant_unref (value);

  untrusted_empty = xvariant_new_from_data (G_VARIANT_TYPE ("ay"), NULL, 0, FALSE, NULL, NULL);
  value = xvariant_get_normal_form (untrusted_empty);
  const_str = xvariant_get_bytestring (value);
  (void) const_str;
  xvariant_unref (value);
  xvariant_unref (untrusted_empty);
}

static void
test_lookup_value (void)
{
  struct {
    const xchar_t *dict, *key, *value;
  } cases[] = {
    { "@a{ss} {'x':  'y'}",   "x",  "'y'" },
    { "@a{ss} {'x':  'y'}",   "y",  NULL  },
    { "@a{os} {'/x': 'y'}",   "/x", "'y'" },
    { "@a{os} {'/x': 'y'}",   "/y", NULL  },
    { "@a{sv} {'x':  <'y'>}", "x",  "'y'" },
    { "@a{sv} {'x':  <5>}",   "x",  "5"   },
    { "@a{sv} {'x':  <'y'>}", "y",  NULL  }
  };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (cases); i++)
    {
      xvariant_t *dictionary;
      xvariant_t *value;
      xchar_t *p;

      dictionary = xvariant_parse (NULL, cases[i].dict, NULL, NULL, NULL);
      value = xvariant_lookup_value (dictionary, cases[i].key, NULL);
      xvariant_unref (dictionary);

      if (value == NULL && cases[i].value == NULL)
        continue;

      g_assert_true (value && cases[i].value);
      p = xvariant_print (value, FALSE);
      g_assert_cmpstr (cases[i].value, ==, p);
      xvariant_unref (value);
      g_free (p);
    }
}

static void
test_lookup (void)
{
  const xchar_t *str;
  xvariant_t *dict;
  xboolean_t ok;
  xint_t num;

  dict = xvariant_parse (NULL,
                          "{'a': <5>, 'b': <'c'>}",
                          NULL, NULL, NULL);

  ok = xvariant_lookup (dict, "a", "i", &num);
  g_assert_true (ok);
  g_assert_cmpint (num, ==, 5);

  ok = xvariant_lookup (dict, "a", "&s", &str);
  g_assert_false (ok);

  ok = xvariant_lookup (dict, "q", "&s", &str);
  g_assert_false (ok);

  ok = xvariant_lookup (dict, "b", "i", &num);
  g_assert_false (ok);

  ok = xvariant_lookup (dict, "b", "&s", &str);
  g_assert_true (ok);
  g_assert_cmpstr (str, ==, "c");

  ok = xvariant_lookup (dict, "q", "&s", &str);
  g_assert_false (ok);

  xvariant_unref (dict);
}

static xvariant_t *
untrusted (xvariant_t *a)
{
  xvariant_t *b;
  const xvariant_type_t *type;
  xbytes_t *bytes;

  type = xvariant_get_type (a);
  bytes = xvariant_get_data_as_bytes (a);
  b = xvariant_new_from_bytes (type, bytes, FALSE);
  xbytes_unref (bytes);
  xvariant_unref (a);

  return b;
}

static void
test_compare (void)
{
  xvariant_t *a;
  xvariant_t *b;

  a = untrusted (xvariant_new_byte (5));
  b = xvariant_new_byte (6);
  g_assert_cmpint (xvariant_compare (a, b), <, 0);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_int16 (G_MININT16));
  b = xvariant_new_int16 (G_MAXINT16);
  g_assert_cmpint (xvariant_compare (a, b), <, 0);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_uint16 (0));
  b = xvariant_new_uint16 (G_MAXUINT16);
  g_assert_cmpint (xvariant_compare (a, b), <, 0);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_int32 (G_MININT32));
  b = xvariant_new_int32 (G_MAXINT32);
  g_assert_cmpint (xvariant_compare (a, b), <, 0);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_uint32 (0));
  b = xvariant_new_uint32 (G_MAXUINT32);
  g_assert_cmpint (xvariant_compare (a, b), <, 0);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_int64 (G_MININT64));
  b = xvariant_new_int64 (G_MAXINT64);
  g_assert_cmpint (xvariant_compare (a, b), <, 0);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_uint64 (0));
  b = xvariant_new_uint64 (G_MAXUINT64);
  g_assert_cmpint (xvariant_compare (a, b), <, 0);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_double (G_MINDOUBLE));
  b = xvariant_new_double (G_MAXDOUBLE);
  g_assert_cmpint (xvariant_compare (a, b), <, 0);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_string ("abc"));
  b = xvariant_new_string ("abd");
  g_assert_cmpint (xvariant_compare (a, b), <, 0);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_object_path ("/abc"));
  b = xvariant_new_object_path ("/abd");
  g_assert_cmpint (xvariant_compare (a, b), <, 0);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_signature ("g"));
  b = xvariant_new_signature ("o");
  g_assert_cmpint (xvariant_compare (a, b), <, 0);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_boolean (FALSE));
  b = xvariant_new_boolean (TRUE);
  g_assert_cmpint (xvariant_compare (a, b), <, 0);
  xvariant_unref (a);
  xvariant_unref (b);
}

static void
test_equal (void)
{
  xvariant_t *a;
  xvariant_t *b;

  a = untrusted (xvariant_new_byte (5));
  b = xvariant_get_normal_form (a);
  g_assert_cmpvariant (a, b);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_int16 (G_MININT16));
  b = xvariant_get_normal_form (a);
  g_assert_cmpvariant (a, b);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_uint16 (0));
  b = xvariant_get_normal_form (a);
  g_assert_cmpvariant (a, b);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_int32 (G_MININT32));
  b = xvariant_get_normal_form (a);
  g_assert_cmpvariant (a, b);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_uint32 (0));
  b = xvariant_get_normal_form (a);
  g_assert_cmpvariant (a, b);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_int64 (G_MININT64));
  b = xvariant_get_normal_form (a);
  g_assert_cmpvariant (a, b);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_uint64 (0));
  b = xvariant_get_normal_form (a);
  g_assert_cmpvariant (a, b);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_double (G_MINDOUBLE));
  b = xvariant_get_normal_form (a);
  g_assert_cmpvariant (a, b);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_string ("abc"));
  g_assert_cmpvariant (a, a);
  b = xvariant_get_normal_form (a);
  g_assert_cmpvariant (a, b);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_object_path ("/abc"));
  g_assert_cmpvariant (a, a);
  b = xvariant_get_normal_form (a);
  a = untrusted (a);
  g_assert_cmpvariant (a, b);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_signature ("g"));
  g_assert_cmpvariant (a, a);
  b = xvariant_get_normal_form (a);
  a = untrusted (a);
  g_assert_cmpvariant (a, b);
  xvariant_unref (a);
  xvariant_unref (b);
  a = untrusted (xvariant_new_boolean (FALSE));
  b = xvariant_get_normal_form (a);
  g_assert_cmpvariant (a, b);
  xvariant_unref (a);
  xvariant_unref (b);
}

static void
test_fixed_array (void)
{
  xvariant_t *a;
  gint32 values[5];
  const gint32 *elts;
  xsize_t n_elts;
  xsize_t i;

  n_elts = 0;
  a = xvariant_new_parsed ("[1,2,3,4,5]");
  elts = xvariant_get_fixed_array (a, &n_elts, sizeof (gint32));
  g_assert_cmpuint (n_elts, ==, 5);
  for (i = 0; i < 5; i++)
    g_assert_cmpint (elts[i], ==, i + 1);
  xvariant_unref (a);

  n_elts = 0;
  for (i = 0; i < 5; i++)
    values[i] = i + 1;
  a = xvariant_new_fixed_array (G_VARIANT_TYPE_INT32, values,
                                 G_N_ELEMENTS (values), sizeof (values[0]));
  g_assert_cmpstr (xvariant_get_type_string (a), ==, "ai");
  elts = xvariant_get_fixed_array (a, &n_elts, sizeof (gint32));
  g_assert_cmpuint (n_elts, ==, 5);
  for (i = 0; i < 5; i++)
    g_assert_cmpint (elts[i], ==, i + 1);
  xvariant_unref (a);
}

static void
test_check_format_string (void)
{
  xvariant_t *value;

  value = xvariant_new ("(sas)", "foo", NULL);
  xvariant_ref_sink (value);

  g_assert_true (xvariant_check_format_string (value, "(s*)", TRUE));
  g_assert_true (xvariant_check_format_string (value, "(s*)", FALSE));
  g_assert_false (xvariant_check_format_string (value, "(u*)", TRUE));
  g_assert_false (xvariant_check_format_string (value, "(u*)", FALSE));

  g_assert_true (xvariant_check_format_string (value, "(&s*)", FALSE));
  g_test_expect_message ("GLib", G_LOG_LEVEL_CRITICAL, "*contains a '&' character*");
  g_assert_false (xvariant_check_format_string (value, "(&s*)", TRUE));
  g_test_assert_expected_messages ();

  g_assert_true (xvariant_check_format_string (value, "(s^as)", TRUE));
  g_assert_true (xvariant_check_format_string (value, "(s^as)", FALSE));

  g_test_expect_message ("GLib", G_LOG_LEVEL_CRITICAL, "*contains a '&' character*");
  g_assert_false (xvariant_check_format_string (value, "(s^a&s)", TRUE));
  g_test_assert_expected_messages ();
  g_assert_true (xvariant_check_format_string (value, "(s^a&s)", FALSE));

  xvariant_unref (value);

  /* Do it again with a type that will let us put a '&' after a '^' */
  value = xvariant_new ("(say)", "foo", NULL);
  xvariant_ref_sink (value);

  g_assert_true (xvariant_check_format_string (value, "(s*)", TRUE));
  g_assert_true (xvariant_check_format_string (value, "(s*)", FALSE));
  g_assert_false (xvariant_check_format_string (value, "(u*)", TRUE));
  g_assert_false (xvariant_check_format_string (value, "(u*)", FALSE));

  g_assert_true (xvariant_check_format_string (value, "(&s*)", FALSE));
  g_test_expect_message ("GLib", G_LOG_LEVEL_CRITICAL, "*contains a '&' character*");
  g_assert_false (xvariant_check_format_string (value, "(&s*)", TRUE));
  g_test_assert_expected_messages ();

  g_assert_true (xvariant_check_format_string (value, "(s^ay)", TRUE));
  g_assert_true (xvariant_check_format_string (value, "(s^ay)", FALSE));

  g_test_expect_message ("GLib", G_LOG_LEVEL_CRITICAL, "*contains a '&' character*");
  g_assert_false (xvariant_check_format_string (value, "(s^&ay)", TRUE));
  g_test_assert_expected_messages ();
  g_assert_true (xvariant_check_format_string (value, "(s^&ay)", FALSE));

  g_assert_true (xvariant_check_format_string (value, "r", FALSE));
  g_assert_true (xvariant_check_format_string (value, "(?a?)", FALSE));

  xvariant_unref (value);
}

static void
verify_gvariant_checksum (const xchar_t  *sha256,
			  xvariant_t     *v)

{
  xchar_t *checksum;
  checksum = g_compute_checksum_for_data (G_CHECKSUM_SHA256,
					  xvariant_get_data (v),
					  xvariant_get_size (v));
  g_assert_cmpstr (sha256, ==, checksum);
  g_free (checksum);
}

static void
verify_gvariant_checksum_va (const xchar_t *sha256,
			     const xchar_t *fmt,
			     ...)
{
  va_list args;
  xvariant_t *v;

  va_start (args, fmt);

  v = xvariant_new_va (fmt, NULL, &args);
  xvariant_ref_sink (v);
#if G_BYTE_ORDER == G_BIG_ENDIAN
  {
    xvariant_t *byteswapped = xvariant_byteswap (v);
    xvariant_unref (v);
    v = byteswapped;
  }
#endif

  va_end (args);

  verify_gvariant_checksum (sha256, v);

  xvariant_unref (v);
}

static void
test_checksum_basic (void)
{
  verify_gvariant_checksum_va ("e8a4b2ee7ede79a3afb332b5b6cc3d952a65fd8cffb897f5d18016577c33d7cc",
			       "u", 42);
  verify_gvariant_checksum_va ("c53e363c33b00cfce298229ee83856b8a98c2e6126cab13f65899f62473b0df5",
			       "s", "moocow");
  verify_gvariant_checksum_va ("2b4c342f5433ebe591a1da77e013d1b72475562d48578dca8b84bac6651c3cb9",
			       "y", 9);
  verify_gvariant_checksum_va ("12a3ae445661ce5dee78d0650d33362dec29c4f82af05e7e57fb595bbbacf0ca",
			       "t", G_MAXUINT64);
  verify_gvariant_checksum_va ("e25a59b24440eb6c833aa79c93b9840e6eab6966add0dacf31df7e9e7000f5b3",
			       "d", 3.14159);
  verify_gvariant_checksum_va ("4bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a",
			       "b", TRUE);
  verify_gvariant_checksum_va ("ca2fd00fa001190744c15c317643ab092e7048ce086a243e2be9437c898de1bb",
			       "q", G_MAXUINT16);
}

static void
test_checksum_nested (void)
{
  static const char* const strv[] = {"foo", "bar", "baz", NULL};

  verify_gvariant_checksum_va ("31fbc92f08fddaca716188fe4b5d44ae122fc6306fd3c6925af53cfa47ea596d",
			       "(uu)", 41, 43);
  verify_gvariant_checksum_va ("01759d683cead856d1d386d59af0578841698a424a265345ad5413122f220de8",
			       "(su)", "moocow", 79);
  verify_gvariant_checksum_va ("52b3ae95f19b3e642ea1d01185aea14a09004c1d1712672644427403a8a0afe6",
			       "(qyst)", G_MAXUINT16, 9, "moocow", G_MAXUINT64);
  verify_gvariant_checksum_va ("6fc6f4524161c3ae0d316812d7088e3fcd372023edaea2d7821093be40ae1060",
			       "(@ay)", xvariant_new_bytestring ("\xFF\xFF\xFF"));
  verify_gvariant_checksum_va ("572aca386e1a983dd23bb6eb6e3dfa72eef9ca7c7744581aa800e18d7d9d0b0b",
			       "(^as)", strv);
  verify_gvariant_checksum_va ("4bddf6174c791bb44fc6a4106573031690064df34b741033a0122ed8dc05bcf3",
			       "(yvu)", 254, xvariant_new ("(^as)", strv), 42);
}

static void
test_gbytes (void)
{
  xvariant_t *a;
  xvariant_t *tuple;
  xbytes_t *bytes;
  xbytes_t *bytes2;
  const xuint8_t values[5] = { 1, 2, 3, 4, 5 };
  const xuint8_t *elts;
  xsize_t n_elts;
  xsize_t i;

  bytes = xbytes_new (&values, 5);
  a = xvariant_new_from_bytes (G_VARIANT_TYPE_BYTESTRING, bytes, TRUE);
  xbytes_unref (bytes);
  n_elts = 0;
  elts = xvariant_get_fixed_array (a, &n_elts, sizeof (xuint8_t));
  g_assert_cmpuint (n_elts, ==, 5);
  for (i = 0; i < 5; i++)
    g_assert_cmpuint (elts[i], ==, i + 1);

  bytes2 = xvariant_get_data_as_bytes (a);
  xvariant_unref (a);

  bytes = xbytes_new (&values, 5);
  g_assert_true (xbytes_equal (bytes, bytes2));
  xbytes_unref (bytes);
  xbytes_unref (bytes2);

  tuple = xvariant_new_parsed ("['foo', 'bar']");
  bytes = xvariant_get_data_as_bytes (tuple); /* force serialization */
  a = xvariant_get_child_value (tuple, 1);
  bytes2 = xvariant_get_data_as_bytes (a);
  g_assert_false (xbytes_equal (bytes, bytes2));

  xbytes_unref (bytes);
  xbytes_unref (bytes2);
  xvariant_unref (a);
  xvariant_unref (tuple);
}

typedef struct {
  const xvariant_type_t *type;
  const xchar_t *in;
  const xchar_t *out;
} ContextTest;

static void
test_print_context (void)
{
  ContextTest tests[] = {
    { NULL, "(1, 2, 3, 'abc", "          ^^^^" },
    { NULL, "[1, 2, 3, 'str']", " ^        ^^^^^" },
    { G_VARIANT_TYPE_UINT16, "{ 'abc':'def' }", "  ^^^^^^^^^^^^^^^" },
    { NULL, "<5", "    ^" },
    { NULL, "'ab\\ux'", "       ^ " },
    { NULL, "'ab\\U00efx'", "       ^^^^  " }
  };
  xvariant_t *v;
  xchar_t *s;
  xsize_t i;
  xerror_t *error = NULL;

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      v = xvariant_parse (tests[i].type, tests[i].in, NULL, NULL, &error);
      g_assert_null (v);
      s = xvariant_parse_error_print_context (error, tests[i].in);
      g_assert_nonnull (strstr (s, tests[i].out));
      g_free (s);
      g_clear_error (&error);
    }
}

static void
test_error_quark (void)
{
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_assert_cmpuint (xvariant_parser_get_error_quark (), ==, xvariant_parse_error_quark ());
G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
test_stack_builder_init (void)
{
  xvariant_builder_t builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE_BYTESTRING);
  xvariant_t *variant;

  xvariant_builder_add_value (&builder, xvariant_new_byte ('g'));
  xvariant_builder_add_value (&builder, xvariant_new_byte ('l'));
  xvariant_builder_add_value (&builder, xvariant_new_byte ('i'));
  xvariant_builder_add_value (&builder, xvariant_new_byte ('b'));
  xvariant_builder_add_value (&builder, xvariant_new_byte ('\0'));

  variant = xvariant_ref_sink (xvariant_builder_end (&builder));
  g_assert_nonnull (variant);
  g_assert_true (xvariant_type_equal (xvariant_get_type (variant),
                                       G_VARIANT_TYPE_BYTESTRING));
  g_assert_cmpuint (xvariant_n_children (variant), ==, 5);
  g_assert_cmpstr (xvariant_get_bytestring (variant), ==, "glib");
  xvariant_unref (variant);
}

static xvariant_t *
get_asv (void)
{
  xvariant_builder_t builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE_VARDICT);

  xvariant_builder_add (&builder, "{s@v}", "foo", xvariant_new_variant (xvariant_new_string ("FOO")));
  xvariant_builder_add (&builder, "{s@v}", "bar", xvariant_new_variant (xvariant_new_string ("BAR")));

  return xvariant_ref_sink (xvariant_builder_end (&builder));
}

static void
test_stack_dict_init (void)
{
  xvariant_t *asv = get_asv ();
  xvariant_dict_t dict = XVARIANT_DICT_INIT (asv);
  xvariant_t *variant;
  xvariant_iter_t iter;
  xchar_t *key;
  xvariant_t *value;

  xvariant_dict_insert_value (&dict, "baz", xvariant_new_string ("BAZ"));
  xvariant_dict_insert_value (&dict, "quux", xvariant_new_string ("QUUX"));

  variant = xvariant_ref_sink (xvariant_dict_end (&dict));
  g_assert_nonnull (variant);
  g_assert_true (xvariant_type_equal (xvariant_get_type (variant),
                                       G_VARIANT_TYPE_VARDICT));
  g_assert_cmpuint (xvariant_n_children (variant), ==, 4);

  xvariant_iter_init (&iter, variant);
  while (xvariant_iter_next (&iter, "{sv}", &key, &value))
    {
      xchar_t *strup = g_ascii_strup (key, -1);

      g_assert_cmpstr (strup, ==, xvariant_get_string (value, NULL));
      g_free (key);
      g_free (strup);
      xvariant_unref (value);
    }

  xvariant_unref (asv);
  xvariant_unref (variant);
}

/* test_t checking arbitrary binary data for normal form. This time, it’s a tuple
 * with invalid element ends. */
static void
test_normal_checking_tuples (void)
{
  const xuint8_t data[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
    'a', '(', 'a', 'o', 'a', 'o', 'a', 'a', 'o', 'a', 'a', 'o', ')'
  };
  xsize_t size = sizeof (data);
  xvariant_t *variant = NULL;
  xvariant_t *normal_variant = NULL;

  variant = xvariant_new_from_data (G_VARIANT_TYPE_VARIANT, data, size,
                                     FALSE, NULL, NULL);
  g_assert_nonnull (variant);

  normal_variant = xvariant_get_normal_form (variant);
  g_assert_nonnull (normal_variant);

  xvariant_unref (normal_variant);
  xvariant_unref (variant);
}

/* Check that deeply nested variants are not considered in normal form when
 * deserialized from untrusted data.*/
static void
test_recursion_limits_variant_in_variant (void)
{
  xvariant_t *wrapper_variant = NULL;
  xsize_t i;
  xbytes_t *bytes = NULL;
  xvariant_t *deserialised_variant = NULL;

  /* Construct a hierarchy of variants, containing a single string. This is just
   * below the maximum recursion level, as a series of nested variant types. */
  wrapper_variant = xvariant_new_string ("hello");

  for (i = 0; i < G_VARIANT_MAX_RECURSION_DEPTH - 1; i++)
    wrapper_variant = xvariant_new_variant (g_steal_pointer (&wrapper_variant));

  /* Serialize and deserialize it as untrusted data, to force normalisation. */
  bytes = xvariant_get_data_as_bytes (wrapper_variant);
  deserialised_variant = xvariant_new_from_bytes (G_VARIANT_TYPE_VARIANT,
                                                   bytes, FALSE);
  g_assert_nonnull (deserialised_variant);
  g_assert_true (xvariant_is_normal_form (deserialised_variant));

  xbytes_unref (bytes);
  xvariant_unref (deserialised_variant);

  /* Wrap it once more. Normalisation should now fail. */
  wrapper_variant = xvariant_new_variant (g_steal_pointer (&wrapper_variant));

  bytes = xvariant_get_data_as_bytes (wrapper_variant);
  deserialised_variant = xvariant_new_from_bytes (G_VARIANT_TYPE_VARIANT,
                                                   bytes, FALSE);
  g_assert_nonnull (deserialised_variant);
  g_assert_false (xvariant_is_normal_form (deserialised_variant));

  xvariant_unref (deserialised_variant);

  /* Deserialize it again, but trusted this time. This should succeed. */
  deserialised_variant = xvariant_new_from_bytes (G_VARIANT_TYPE_VARIANT,
                                                   bytes, TRUE);
  g_assert_nonnull (deserialised_variant);
  g_assert_true (xvariant_is_normal_form (deserialised_variant));

  xbytes_unref (bytes);
  xvariant_unref (deserialised_variant);
  xvariant_unref (wrapper_variant);
}

/* Check that deeply nested arrays are not considered in normal form when
 * deserialized from untrusted data after being wrapped in a variant. This is
 * worth testing, because neither the deeply nested array, nor the variant,
 * have a static #xvariant_type_t which is too deep — only when nested together do
 * they become too deep. */
static void
test_recursion_limits_array_in_variant (void)
{
  xvariant_t *child_variant = NULL;
  xvariant_t *wrapper_variant = NULL;
  xsize_t i;
  xbytes_t *bytes = NULL;
  xvariant_t *deserialised_variant = NULL;

  /* Construct a hierarchy of arrays, containing a single string. This is just
   * below the maximum recursion level, all in a single definite type. */
  child_variant = xvariant_new_string ("hello");

  for (i = 0; i < G_VARIANT_MAX_RECURSION_DEPTH - 1; i++)
    child_variant = xvariant_new_array (NULL, &child_variant, 1);

  /* Serialize and deserialize it as untrusted data, to force normalisation. */
  bytes = xvariant_get_data_as_bytes (child_variant);
  deserialised_variant = xvariant_new_from_bytes (xvariant_get_type (child_variant),
                                                   bytes, FALSE);
  g_assert_nonnull (deserialised_variant);
  g_assert_true (xvariant_is_normal_form (deserialised_variant));

  xbytes_unref (bytes);
  xvariant_unref (deserialised_variant);

  /* Wrap it in a variant. Normalisation should now fail. */
  wrapper_variant = xvariant_new_variant (g_steal_pointer (&child_variant));

  bytes = xvariant_get_data_as_bytes (wrapper_variant);
  deserialised_variant = xvariant_new_from_bytes (G_VARIANT_TYPE_VARIANT,
                                                   bytes, FALSE);
  g_assert_nonnull (deserialised_variant);
  g_assert_false (xvariant_is_normal_form (deserialised_variant));

  xvariant_unref (deserialised_variant);

  /* Deserialize it again, but trusted this time. This should succeed. */
  deserialised_variant = xvariant_new_from_bytes (G_VARIANT_TYPE_VARIANT,
                                                   bytes, TRUE);
  g_assert_nonnull (deserialised_variant);
  g_assert_true (xvariant_is_normal_form (deserialised_variant));

  xbytes_unref (bytes);
  xvariant_unref (deserialised_variant);
  xvariant_unref (wrapper_variant);
}

/* test_t that an array with invalidly large values in its offset table is
 * normalised successfully without looping infinitely. */
static void
test_normal_checking_array_offsets (void)
{
  const xuint8_t data[] = {
    0x07, 0xe5, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00,
    'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'g',
  };
  xsize_t size = sizeof (data);
  xvariant_t *variant = NULL;
  xvariant_t *normal_variant = NULL;

  variant = xvariant_new_from_data (G_VARIANT_TYPE_VARIANT, data, size,
                                     FALSE, NULL, NULL);
  g_assert_nonnull (variant);

  normal_variant = xvariant_get_normal_form (variant);
  g_assert_nonnull (normal_variant);

  xvariant_unref (normal_variant);
  xvariant_unref (variant);
}

/* test_t that a tuple with invalidly large values in its offset table is
 * normalised successfully without looping infinitely. */
static void
test_normal_checking_tuple_offsets (void)
{
  const xuint8_t data[] = {
    0x07, 0xe5, 0x00, 0x07, 0x00, 0x07,
    '(', 'a', 's', 'a', 's', 'a', 's', 'a', 's', 'a', 's', 'a', 's', ')',
  };
  xsize_t size = sizeof (data);
  xvariant_t *variant = NULL;
  xvariant_t *normal_variant = NULL;

  variant = xvariant_new_from_data (G_VARIANT_TYPE_VARIANT, data, size,
                                     FALSE, NULL, NULL);
  g_assert_nonnull (variant);

  normal_variant = xvariant_get_normal_form (variant);
  g_assert_nonnull (normal_variant);

  xvariant_unref (normal_variant);
  xvariant_unref (variant);
}

/* test_t that an empty object path is normalised successfully to the base object
 * path, ‘/’. */
static void
test_normal_checking_empty_object_path (void)
{
  const xuint8_t data[] = {
    0x20, 0x20, 0x00, 0x00, 0x00, 0x00,
    '(', 'h', '(', 'a', 'i', 'a', 'b', 'i', 'o', ')', ')',
  };
  xsize_t size = sizeof (data);
  xvariant_t *variant = NULL;
  xvariant_t *normal_variant = NULL;

  variant = xvariant_new_from_data (G_VARIANT_TYPE_VARIANT, data, size,
                                     FALSE, NULL, NULL);
  g_assert_nonnull (variant);

  normal_variant = xvariant_get_normal_form (variant);
  g_assert_nonnull (normal_variant);

  xvariant_unref (normal_variant);
  xvariant_unref (variant);
}

/* test_t that constructing a #xvariant_t from data which is not correctly aligned
 * for the variant type is OK, by loading a variant from data at various offsets
 * which are aligned and unaligned. When unaligned, a slow construction path
 * should be taken. */
static void
test_unaligned_construction (void)
{
  const xuint8_t data[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  };
  xvariant_t *variant = NULL;
  xvariant_t *normal_variant = NULL;
  xsize_t i, offset;
  const struct {
    const xvariant_type_t *type;
    xsize_t size;
    xsize_t max_offset;
  } vectors[] = {
    { G_VARIANT_TYPE_UINT64, sizeof (xuint64_t), sizeof (xuint64_t) },
    { G_VARIANT_TYPE_UINT32, sizeof (xuint32_t), sizeof (xuint32_t) },
    { G_VARIANT_TYPE_UINT16, sizeof (xuint16_t), sizeof (xuint16_t) },
    { G_VARIANT_TYPE_BYTE, sizeof (xuint8_t), 3 },
  };

  G_STATIC_ASSERT (sizeof (xuint64_t) * 2 <= sizeof (data));

  for (i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      for (offset = 0; offset < vectors[i].max_offset; offset++)
        {
          variant = xvariant_new_from_data (vectors[i].type, data + offset,
                                             vectors[i].size,
                                             FALSE, NULL, NULL);
          g_assert_nonnull (variant);

          normal_variant = xvariant_get_normal_form (variant);
          g_assert_nonnull (normal_variant);

          xvariant_unref (normal_variant);
          xvariant_unref (variant);
        }
    }
}

int
main (int argc, char **argv)
{
  xuint_t i;

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gvariant/type", test_gvarianttype);
  g_test_add_func ("/gvariant/type/string-scan/recursion/tuple",
                   test_gvarianttype_string_scan_recursion_tuple);
  g_test_add_func ("/gvariant/type/string-scan/recursion/array",
                   test_gvarianttype_string_scan_recursion_array);
  g_test_add_func ("/gvariant/typeinfo", test_gvarianttypeinfo);
  g_test_add_func ("/gvariant/serialiser/maybe", test_maybes);
  g_test_add_func ("/gvariant/serialiser/array", test_arrays);
  g_test_add_func ("/gvariant/serialiser/tuple", test_tuples);
  g_test_add_func ("/gvariant/serialiser/variant", test_variants);
  g_test_add_func ("/gvariant/serialiser/strings", test_strings);
  g_test_add_func ("/gvariant/serialiser/byteswap", test_byteswaps);
  g_test_add_func ("/gvariant/serialiser/children", test_serialiser_children);

  for (i = 1; i <= 20; i += 4)
    {
      char *testname;

      testname = xstrdup_printf ("/gvariant/serialiser/fuzz/%u%%", i);
      g_test_add_data_func (testname, GINT_TO_POINTER (i),
                            (xpointer_t) test_fuzzes);
      g_free (testname);
    }

  g_test_add_func ("/gvariant/string", test_string);
  g_test_add_func ("/gvariant/utf8", test_utf8);
  g_test_add_func ("/gvariant/containers", test_containers);
  g_test_add_func ("/gvariant/format-strings", test_format_strings);
  g_test_add_func ("/gvariant/invalid-varargs", test_invalid_varargs);
  g_test_add_func ("/gvariant/varargs", test_varargs);
  g_test_add_func ("/gvariant/varargs/subprocess/empty-array", test_varargs_empty_array);
  g_test_add_func ("/gvariant/valist", test_valist);
  g_test_add_func ("/gvariant/builder-memory", test_builder_memory);
  g_test_add_func ("/gvariant/hashing", test_hashing);
  g_test_add_func ("/gvariant/byteswap", test_gv_byteswap);
  g_test_add_func ("/gvariant/parser", test_parses);
  g_test_add_func ("/gvariant/parser/integer-bounds", test_parser_integer_bounds);
  g_test_add_func ("/gvariant/parser/recursion", test_parser_recursion);
  g_test_add_func ("/gvariant/parse-failures", test_parse_failures);
  g_test_add_func ("/gvariant/parse-positional", test_parse_positional);
  g_test_add_func ("/gvariant/parse/subprocess/bad-format-char", test_parse_bad_format_char);
  g_test_add_func ("/gvariant/parse/subprocess/bad-format-string", test_parse_bad_format_string);
  g_test_add_func ("/gvariant/parse/subprocess/bad-args", test_parse_bad_args);
  g_test_add_func ("/gvariant/floating", test_floating);
  g_test_add_func ("/gvariant/bytestring", test_bytestring);
  g_test_add_func ("/gvariant/lookup-value", test_lookup_value);
  g_test_add_func ("/gvariant/lookup", test_lookup);
  g_test_add_func ("/gvariant/compare", test_compare);
  g_test_add_func ("/gvariant/equal", test_equal);
  g_test_add_func ("/gvariant/fixed-array", test_fixed_array);
  g_test_add_func ("/gvariant/check-format-string", test_check_format_string);

  g_test_add_func ("/gvariant/checksum-basic", test_checksum_basic);
  g_test_add_func ("/gvariant/checksum-nested", test_checksum_nested);

  g_test_add_func ("/gvariant/gbytes", test_gbytes);
  g_test_add_func ("/gvariant/print-context", test_print_context);
  g_test_add_func ("/gvariant/error-quark", test_error_quark);

  g_test_add_func ("/gvariant/stack-builder-init", test_stack_builder_init);
  g_test_add_func ("/gvariant/stack-dict-init", test_stack_dict_init);

  g_test_add_func ("/gvariant/normal-checking/tuples",
                   test_normal_checking_tuples);
  g_test_add_func ("/gvariant/normal-checking/array-offsets",
                   test_normal_checking_array_offsets);
  g_test_add_func ("/gvariant/normal-checking/tuple-offsets",
                   test_normal_checking_tuple_offsets);
  g_test_add_func ("/gvariant/normal-checking/empty-object-path",
                   test_normal_checking_empty_object_path);

  g_test_add_func ("/gvariant/recursion-limits/variant-in-variant",
                   test_recursion_limits_variant_in_variant);
  g_test_add_func ("/gvariant/recursion-limits/array-in-variant",
                   test_recursion_limits_array_in_variant);

  g_test_add_func ("/gvariant/unaligned-construction",
                   test_unaligned_construction);

  return g_test_run ();
}
