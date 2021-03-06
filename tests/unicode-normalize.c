#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

xboolean_t success = TRUE;

static char *
decode (const xchar_t *input)
{
  unsigned ch;
  int offset = 0;
  xstring_t *result = xstring_new (NULL);

  do
    {
      if (sscanf (input + offset, "%x", &ch) != 1)
	{
	  fprintf (stderr, "Error parsing character string %s\n", input);
	  exit (1);
	}

      xstring_append_unichar (result, ch);

      while (input[offset] && input[offset] != ' ')
	offset++;
      while (input[offset] && input[offset] == ' ')
	offset++;
    }
  while (input[offset]);

  return xstring_free (result, FALSE);
}

const char *names[4] = {
  "NFD",
  "NFC",
  "NFKD",
  "NFKC"
};

static char *
encode (const xchar_t *input)
{
  xstring_t *result = xstring_new(NULL);

  const xchar_t *p = input;
  while (*p)
    {
      xunichar_t c = xutf8_get_char (p);
      xstring_append_printf (result, "%04X ", c);
      p = xutf8_next_char(p);
    }

  return xstring_free (result, FALSE);
}

static void
test_form (int            line,
	   xnormalize_mode_t mode,
	   xboolean_t       do_compat,
	   int            expected,
	   char         **c,
	   char         **raw)
{
  int i;

  xboolean_t mode_is_compat = (mode == XNORMALIZE_NFKC ||
			     mode == XNORMALIZE_NFKD);

  if (mode_is_compat || !do_compat)
    {
      for (i = 0; i < 3; i++)
	{
	  char *result = xutf8_normalize (c[i], -1, mode);
	  if (strcmp (result, c[expected]) != 0)
	    {
	      char *result_raw = encode(result);
	      fprintf (stderr, "\nFailure: %d/%d: %s\n", line, i + 1, raw[5]);
	      fprintf (stderr, "  xutf8_normalize (%s, %s) != %s but %s\n",
		   raw[i], names[mode], raw[expected], result_raw);
	      g_free (result_raw);
	      success = FALSE;
	    }

	  g_free (result);
	}
    }
  if (mode_is_compat || do_compat)
    {
      for (i = 3; i < 5; i++)
	{
	  char *result = xutf8_normalize (c[i], -1, mode);
	  if (strcmp (result, c[expected]) != 0)
	    {
	      char *result_raw = encode(result);
	      fprintf (stderr, "\nFailure: %d/%d: %s\n", line, i, raw[5]);
	      fprintf (stderr, "  xutf8_normalize (%s, %s) != %s but %s\n",
		   raw[i], names[mode], raw[expected], result_raw);
	      g_free (result_raw);
	      success = FALSE;
	    }

	  g_free (result);
	}
    }
}

static xboolean_t
process_one (int line, xchar_t **columns)
{
  char *c[5];
  int i;
  xboolean_t skip = FALSE;

  for (i=0; i < 5; i++)
    {
      c[i] = decode(columns[i]);
      if (!c[i])
	skip = TRUE;
    }

  if (!skip)
    {
      test_form (line, XNORMALIZE_NFD, FALSE, 2, c, columns);
      test_form (line, XNORMALIZE_NFD, TRUE, 4, c, columns);
      test_form (line, XNORMALIZE_NFC, FALSE, 1, c, columns);
      test_form (line, XNORMALIZE_NFC, TRUE, 3, c, columns);
      test_form (line, XNORMALIZE_NFKD, TRUE, 4, c, columns);
      test_form (line, XNORMALIZE_NFKC, TRUE, 3, c, columns);
    }

  for (i=0; i < 5; i++)
    g_free (c[i]);

  return TRUE;
}

int main (int argc, char **argv)
{
  xio_channel_t *in;
  xerror_t *error = NULL;
  xstring_t *buffer = xstring_new (NULL);
  int line_to_do = 0;
  int line = 1;

  if (argc != 2 && argc != 3)
    {
      fprintf (stderr, "Usage: unicode-normalize NormalizationTest.txt LINE\n");
      return 1;
    }

  if (argc == 3)
    line_to_do = atoi(argv[2]);

  in = g_io_channel_new_file (argv[1], "r", &error);
  if (!in)
    {
      fprintf (stderr, "Cannot open %s: %s\n", argv[1], error->message);
      return 1;
    }

  while (TRUE)
    {
      xsize_t term_pos;
      xchar_t **columns;

      if (g_io_channel_read_line_string (in, buffer, &term_pos, &error) != G_IO_STATUS_NORMAL)
	break;

      if (line_to_do && line != line_to_do)
	goto next;

      buffer->str[term_pos] = '\0';

      if (buffer->str[0] == '#') /* Comment */
	goto next;
      if (buffer->str[0] == '@') /* Part */
	{
	  fprintf (stderr, "\nProcessing %s\n", buffer->str + 1);
	  goto next;
	}

      columns = xstrsplit (buffer->str, ";", -1);
      if (!columns[0])
	goto next;

      if (!process_one (line, columns))
	return 1;
      xstrfreev (columns);

    next:
      xstring_truncate (buffer, 0);
      line++;
    }

  if (error)
    {
      fprintf (stderr, "Error reading test file, %s\n", error->message);
      return 1;
    }

  g_io_channel_unref (in);
  xstring_free (buffer, TRUE);

  return !success;
}
