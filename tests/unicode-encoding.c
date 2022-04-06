#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

static xint_t exit_status = 0;

G_GNUC_PRINTF (1, 2)
static void
croak (char *format, ...)
{
  va_list va;

  va_start (va, format);
  vfprintf (stderr, format, va);
  va_end (va);

  exit (1);
}

G_GNUC_PRINTF (1, 2)
static void
fail (char *format, ...)
{
  va_list va;

  va_start (va, format);
  vfprintf (stderr, format, va);
  va_end (va);

  exit_status |= 1;
}

typedef enum
{
  VALID,
  INCOMPLETE,
  NOTUNICODE,
  OVERLONG,
  MALFORMED
} Status;

static xboolean_t
ucs4_equal (xunichar_t *a, xunichar_t *b)
{
  while (*a && *b && (*a == *b))
    {
      a++;
      b++;
    }

  return (*a == *b);
}

static xboolean_t
utf16_equal (xunichar2_t *a, xunichar2_t *b)
{
  while (*a && *b && (*a == *b))
    {
      a++;
      b++;
    }

  return (*a == *b);
}

static xint_t
utf16_count (xunichar2_t *a)
{
  xint_t result = 0;

  while (a[result])
    result++;

  return result;
}

static void
print_ucs4 (const xchar_t *prefix, xunichar_t *ucs4, xint_t ucs4_len)
{
  xint_t i;
  g_print ("%s ", prefix);
  for (i = 0; i < ucs4_len; i++)
    g_print ("%x ", ucs4[i]);
  g_print ("\n");
}

static void
process (xint_t      line,
	 xchar_t    *utf8,
	 Status    status,
	 xunichar_t *ucs4,
	 xint_t      ucs4_len)
{
  const xchar_t *end;
  xboolean_t is_valid = xutf8_validate (utf8, -1, &end);
  xerror_t *error = NULL;
  xlong_t items_read, items_written;

  switch (status)
    {
    case VALID:
      if (!is_valid)
	{
	  fail ("line %d: valid but xutf8_validate returned FALSE\n", line);
	  return;
	}
      break;
    case NOTUNICODE:
    case INCOMPLETE:
    case OVERLONG:
    case MALFORMED:
      if (is_valid)
	{
	  fail ("line %d: invalid but xutf8_validate returned TRUE\n", line);
	  return;
	}
      break;
    }

  if (status == INCOMPLETE)
    {
      xunichar_t *ucs4_result;

      ucs4_result = xutf8_to_ucs4 (utf8, -1, NULL, NULL, &error);

      if (!error || !xerror_matches (error, G_CONVERT_ERROR, G_CONVERT_ERROR_PARTIAL_INPUT))
	{
	  fail ("line %d: incomplete input not properly detected\n", line);
	  return;
	}
      g_clear_error (&error);

      ucs4_result = xutf8_to_ucs4 (utf8, -1, &items_read, NULL, &error);

      if (!ucs4_result || items_read == (xlong_t) strlen (utf8))
	{
	  fail ("line %d: incomplete input not properly detected\n", line);
	  return;
	}

      g_free (ucs4_result);
    }

  if (status == VALID || status == NOTUNICODE)
    {
      xunichar_t *ucs4_result;

      ucs4_result = xutf8_to_ucs4 (utf8, -1, &items_read, &items_written, &error);
      if (!ucs4_result)
	{
	  fail ("line %d: conversion with status %d to ucs4 failed: %s\n", line, status, error->message);
	  return;
	}

      if (!ucs4_equal (ucs4_result, ucs4) ||
          items_read != (xlong_t) strlen (utf8) ||
	  items_written != ucs4_len)
	{
	  fail ("line %d: results of conversion with status %d to ucs4 do not match expected.\n", line, status);
          print_ucs4 ("expected: ", ucs4, ucs4_len);
          print_ucs4 ("received: ", ucs4_result, items_written);
	  return;
	}

      g_free (ucs4_result);
    }

  if (status == VALID)
     {
      xunichar_t *ucs4_result;
      xchar_t *utf8_result;

      ucs4_result = xutf8_to_ucs4_fast (utf8, -1, &items_written);

      if (!ucs4_equal (ucs4_result, ucs4) ||
	  items_written != ucs4_len)
	{
	  fail ("line %d: results of fast conversion with status %d to ucs4 do not match expected.\n", line, status);
          print_ucs4 ("expected: ", ucs4, ucs4_len);
          print_ucs4 ("received: ", ucs4_result, items_written);
	  return;
	}

      utf8_result = g_ucs4_to_utf8 (ucs4_result, -1, &items_read, &items_written, &error);
      if (!utf8_result)
	{
	  fail ("line %d: conversion back to utf8 failed: %s", line, error->message);
	  return;
	}

      if (strcmp (utf8_result, utf8) != 0 ||
	  items_read != ucs4_len ||
          items_written != (xlong_t) strlen (utf8))
	{
	  fail ("line %d: conversion back to utf8 did not match original\n", line);
	  return;
	}

      g_free (utf8_result);
      g_free (ucs4_result);
    }

  if (status == VALID)
    {
      xunichar2_t *utf16_expected_tmp;
      xunichar2_t *utf16_expected;
      xunichar2_t *utf16_from_utf8;
      xunichar2_t *utf16_from_ucs4;
      xunichar_t *ucs4_result;
      xsize_t bytes_written;
      xint_t n_chars;
      xchar_t *utf8_result;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define TARGET "UTF-16LE"
#else
#define TARGET "UTF-16"
#endif

      if (!(utf16_expected_tmp = (xunichar2_t *)g_convert (utf8, -1, TARGET, "UTF-8",
							 NULL, &bytes_written, NULL)))
	{
	  fail ("line %d: could not convert to UTF-16 via g_convert\n", line);
	  return;
	}

      /* zero-terminate and remove BOM
       */
      n_chars = bytes_written / 2;
      if (utf16_expected_tmp[0] == 0xfeff) /* BOM */
	{
	  n_chars--;
	  utf16_expected = g_new (xunichar2_t, n_chars + 1);
	  memcpy (utf16_expected, utf16_expected_tmp + 1, sizeof(xunichar2_t) * n_chars);
	}
      else if (utf16_expected_tmp[0] == 0xfffe) /* ANTI-BOM */
	{
	  fail ("line %d: conversion via iconv to \"UTF-16\" is not native-endian\n", line);
	  return;
	}
      else
	{
	  utf16_expected = g_new (xunichar2_t, n_chars + 1);
	  memcpy (utf16_expected, utf16_expected_tmp, sizeof(xunichar2_t) * n_chars);
	}

      utf16_expected[n_chars] = '\0';

      if (!(utf16_from_utf8 = xutf8_to_utf16 (utf8, -1, &items_read, &items_written, &error)))
	{
	  fail ("line %d: conversion to ucs16 failed: %s\n", line, error->message);
	  return;
	}

      if (items_read != (xlong_t) strlen (utf8) ||
	  utf16_count (utf16_from_utf8) != items_written)
	{
	  fail ("line %d: length error in conversion to ucs16\n", line);
	  return;
	}

      if (!(utf16_from_ucs4 = g_ucs4_to_utf16 (ucs4, -1, &items_read, &items_written, &error)))
	{
	  fail ("line %d: conversion to ucs16 failed: %s\n", line, error->message);
	  return;
	}

      if (items_read != ucs4_len ||
	  utf16_count (utf16_from_ucs4) != items_written)
	{
	  fail ("line %d: length error in conversion to ucs16\n", line);
	  return;
	}

      if (!utf16_equal (utf16_from_utf8, utf16_expected) ||
	  !utf16_equal (utf16_from_ucs4, utf16_expected))
	{
	  fail ("line %d: results of conversion to ucs16 do not match\n", line);
	  return;
	}

      if (!(utf8_result = xutf16_to_utf8 (utf16_from_utf8, -1, &items_read, &items_written, &error)))
	{
	  fail ("line %d: conversion back to utf8 failed: %s\n", line, error->message);
	  return;
	}

      if (items_read != utf16_count (utf16_from_utf8) ||
          items_written != (xlong_t) strlen (utf8))
	{
	  fail ("line %d: length error in conversion from ucs16 to utf8\n", line);
	  return;
	}

      if (!(ucs4_result = xutf16_to_ucs4 (utf16_from_ucs4, -1, &items_read, &items_written, &error)))
	{
	  fail ("line %d: conversion back to utf8/ucs4 failed\n", line);
	  return;
	}

      if (items_read != utf16_count (utf16_from_utf8) ||
	  items_written != ucs4_len)
	{
	  fail ("line %d: length error in conversion from ucs16 to ucs4\n", line);
	  return;
	}

      if (strcmp (utf8, utf8_result) != 0 ||
	  !ucs4_equal (ucs4, ucs4_result))
	{
	  fail ("line %d: conversion back to utf8/ucs4 did not match original\n", line);
	  return;
	}

      g_free (utf16_expected_tmp);
      g_free (utf16_expected);
      g_free (utf16_from_utf8);
      g_free (utf16_from_ucs4);
      g_free (utf8_result);
      g_free (ucs4_result);
    }
}

int
main (int argc, char **argv)
{
  xchar_t *testfile;
  xchar_t *contents;
  xerror_t *error = NULL;
  xchar_t *p, *end;
  char *tmp;
  xint_t state = 0;
  xint_t line = 1;
  xint_t start_line = 0;		/* Quiet GCC */
  xchar_t *utf8 = NULL;		/* Quiet GCC */
  xarray_t *ucs4;
  Status status = VALID;	/* Quiet GCC */

  g_test_init (&argc, &argv, NULL);

  testfile = g_test_build_filename (G_TEST_DIST, "utf8.txt", NULL);

  xfile_get_contents (testfile, &contents, NULL, &error);
  if (error)
    croak ("Cannot open utf8.txt: %s", error->message);

  ucs4 = g_array_new (TRUE, FALSE, sizeof(xunichar_t));

  p = contents;

  /* Loop over lines */
  while (*p)
    {
      while (*p && (*p == ' ' || *p == '\t'))
	p++;

      end = p;
      while (*end && (*end != '\r' && *end != '\n'))
	end++;

      if (!*p || *p == '#' || *p == '\r' || *p == '\n')
	goto next_line;

      tmp = xstrstrip (xstrndup (p, end - p));

      switch (state)
	{
	case 0:
	  /* UTF-8 string */
	  start_line = line;
	  utf8 = tmp;
	  tmp = NULL;
	  break;

	case 1:
	  /* Status */
	  if (!strcmp (tmp, "VALID"))
	    status = VALID;
	  else if (!strcmp (tmp, "INCOMPLETE"))
	    status = INCOMPLETE;
	  else if (!strcmp (tmp, "NOTUNICODE"))
	    status = NOTUNICODE;
	  else if (!strcmp (tmp, "OVERLONG"))
	    status = OVERLONG;
	  else if (!strcmp (tmp, "MALFORMED"))
	    status = MALFORMED;
	  else
	    croak ("Invalid status on line %d\n", line);

	  if (status != VALID && status != NOTUNICODE)
	    state++;		/* No UCS-4 data */

	  break;

	case 2:
	  /* UCS-4 version */

	  p = strtok (tmp, " \t");
	  while (p)
	    {
	      xchar_t *endptr;

	      xunichar_t ch = strtoul (p, &endptr, 16);
	      if (*endptr != '\0')
		croak ("Invalid UCS-4 character on line %d\n", line);

	      g_array_append_val (ucs4, ch);

	      p = strtok (NULL, " \t");
	    }

	  break;
	}

      g_free (tmp);
      state = (state + 1) % 3;

      if (state == 0)
	{
	  process (start_line, utf8, status, (xunichar_t *)ucs4->data, ucs4->len);
	  g_array_set_size (ucs4, 0);
	  g_free (utf8);
	}

    next_line:
      p = end;
      if (*p && *p == '\r')
	p++;
      if (*p && *p == '\n')
	p++;

      line++;
    }

  g_free (testfile);
  g_array_free (ucs4, TRUE);
  g_free (contents);
  return exit_status;
}
