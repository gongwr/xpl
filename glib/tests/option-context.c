/* Unit tests for xoption_context_t
 * Copyright (C) 2007 Openismus GmbH
 * Authors: Mathias Hasselmann
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <glib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <math.h>

static GOptionEntry global_main_entries[] = {
  { "main-switch", 0, 0,
    G_OPTION_ARG_NONE, NULL,
    "A switch that is in the main group", NULL },
  G_OPTION_ENTRY_NULL
};

static GOptionEntry global_group_entries[] = {
  { "test-switch", 0, 0,
    G_OPTION_ARG_NONE, NULL,
    "A switch that is in the test group", NULL },
  G_OPTION_ENTRY_NULL
};

static xoption_context_t *
make_options (int test_number)
{
  xoption_context_t *options;
  xoption_group_t   *group = NULL;
  xboolean_t have_main_entries = (0 != (test_number & 1));
  xboolean_t have_test_entries = (0 != (test_number & 2));

  options = g_option_context_new (NULL);

  if (have_main_entries)
    g_option_context_add_main_entries (options, global_main_entries, NULL);
  if (have_test_entries)
    {
      group = xoption_group_new ("test", "Test Options",
                                  "Show all test options",
                                  NULL, NULL);
      g_option_context_add_group (options, group);
      xoption_group_add_entries (group, global_group_entries);
    }

  return options;
}

static void
print_help (xoption_context_t *options, xchar_t **argv)
{
  xint_t    argc = 3;
  xerror_t *error = NULL;

  g_option_context_parse (options, &argc, &argv, &error);
  g_option_context_free (options);
  exit(0);
}

static void
test_group_captions_help (xconstpointer test_number)
{
  xoption_context_t *options;
  xchar_t *argv[] = { __FILE__, "--help", NULL };

  options = make_options (GPOINTER_TO_INT (test_number));
  print_help (options, argv);
}

static void
test_group_captions_help_all (xconstpointer test_number)
{
  xoption_context_t *options;
  xchar_t *argv[] = { __FILE__, "--help-all", NULL };

  options = make_options (GPOINTER_TO_INT (test_number));
  print_help (options, argv);
}

static void
test_group_captions_help_test (xconstpointer test_number)
{
  xoption_context_t *options;
  xchar_t *argv[] = { __FILE__, "--help-test", NULL };

  options = make_options (GPOINTER_TO_INT (test_number));
  print_help (options, argv);
}

static void
test_group_captions (void)
{
  const xchar_t *test_name_base[] = { "help", "help-all", "help-test" };
  xchar_t *test_name;
  xuint_t i;
  xsize_t j;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=504142");

  for (i = 0; i < 4; ++i)
    {
      xboolean_t have_main_entries = (0 != (i & 1));
      xboolean_t have_test_entries = (0 != (i & 2));

      for (j = 0; j < G_N_ELEMENTS (test_name_base); ++j)
        {
          GTestSubprocessFlags trap_flags = 0;
          xboolean_t expect_main_description = FALSE;
          xboolean_t expect_main_switch      = FALSE;
          xboolean_t expect_test_description = FALSE;
          xboolean_t expect_test_switch      = FALSE;
          xboolean_t expect_test_group       = FALSE;

          if (g_test_verbose ())
            trap_flags |= G_TEST_SUBPROCESS_INHERIT_STDOUT | G_TEST_SUBPROCESS_INHERIT_STDERR;

          test_name = xstrdup_printf ("/option/group/captions/subprocess/%s-%u",
                                       test_name_base[j], i);
          g_test_trap_subprocess (test_name, 0, trap_flags);
          g_free (test_name);
          g_test_trap_assert_passed ();
          g_test_trap_assert_stderr ("");

          switch (j)
            {
            case 0:
              g_assert_cmpstr ("help", ==, test_name_base[j]);
              expect_main_switch = have_main_entries;
              expect_test_group  = have_test_entries;
              break;

            case 1:
              g_assert_cmpstr ("help-all", ==, test_name_base[j]);
              expect_main_switch = have_main_entries;
              expect_test_switch = have_test_entries;
              expect_test_group  = have_test_entries;
              break;

            case 2:
              g_assert_cmpstr ("help-test", ==, test_name_base[j]);
              expect_test_switch = have_test_entries;
              break;

            default:
              g_assert_not_reached ();
              break;
            }

          expect_main_description |= expect_main_switch;
          expect_test_description |= expect_test_switch;

          if (expect_main_description)
            g_test_trap_assert_stdout           ("*Application Options*");
          else
            g_test_trap_assert_stdout_unmatched ("*Application Options*");
          if (expect_main_switch)
            g_test_trap_assert_stdout           ("*--main-switch*");
          else
            g_test_trap_assert_stdout_unmatched ("*--main-switch*");

          if (expect_test_description)
            g_test_trap_assert_stdout           ("*Test Options*");
          else
            g_test_trap_assert_stdout_unmatched ("*Test Options*");
          if (expect_test_switch)
            g_test_trap_assert_stdout           ("*--test-switch*");
          else
            g_test_trap_assert_stdout_unmatched ("*--test-switch*");

          if (expect_test_group)
            g_test_trap_assert_stdout           ("*--help-test*");
          else
            g_test_trap_assert_stdout_unmatched ("*--help-test*");
        }
    }
}

int error_test1_int;
char *error_test2_string;
xboolean_t error_test3_boolean;

int arg_test1_int;
xchar_t *arg_test2_string;
xchar_t *arg_test3_filename;
xdouble_t arg_test4_double;
xdouble_t arg_test5_double;
gint64 arg_test6_int64;
gint64 arg_test6_int64_2;

xchar_t *callback_test1_string;
int callback_test2_int;

xchar_t *callback_test_optional_string;
xboolean_t callback_test_optional_boolean;

xchar_t **array_test1_array;

xboolean_t ignore_test1_boolean;
xboolean_t ignore_test2_boolean;
xchar_t *ignore_test3_string;

static xchar_t **
split_string (const char *str, int *argc)
{
  xchar_t **argv;
  int len;

  argv = xstrsplit (str, " ", 0);

  for (len = 0; argv[len] != NULL; len++);

  if (argc)
    *argc = len;

  return argv;
}

static xchar_t *
join_stringv (int argc, char **argv)
{
  int i;
  xstring_t *str;

  str = xstring_new (NULL);

  for (i = 0; i < argc; i++)
    {
      xstring_append (str, argv[i]);

      if (i < argc - 1)
	xstring_append_c (str, ' ');
    }

  return xstring_free (str, FALSE);
}

/* Performs a shallow copy */
static char **
copy_stringv (char **argv, int argc)
{
  return g_memdup2 (argv, sizeof (char *) * (argc + 1));
}

static void
check_identical_stringv (xchar_t **before, xchar_t **after)
{
  xuint_t i;

  /* Not only is it the same string... */
  for (i = 0; before[i] != NULL; i++)
    g_assert_cmpstr (before[i], ==, after[i]);

  /* ... it is actually the same pointer */
  for (i = 0; before[i] != NULL; i++)
    g_assert (before[i] == after[i]);

  g_assert (after[i] == NULL);
}


static xboolean_t
error_test1_pre_parse (xoption_context_t *context,
		       xoption_group_t   *group,
		       xpointer_t	       data,
		       xerror_t        **error)
{
  g_assert (error_test1_int == 0x12345678);

  return TRUE;
}

static xboolean_t
error_test1_post_parse (xoption_context_t *context,
			xoption_group_t   *group,
			xpointer_t	  data,
			xerror_t        **error)
{
  g_assert (error_test1_int == 20);

  /* Set an error in the post hook */
  g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, " ");

  return FALSE;
}

static void
error_test1 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  xoption_group_t *main_group;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_INT, &error_test1_int, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  error_test1_int = 0x12345678;

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Set pre and post parse hooks */
  main_group = g_option_context_get_main_group (context);
  xoption_group_set_parse_hooks (main_group,
				  error_test1_pre_parse, error_test1_post_parse);

  /* Now try parsing */
  argv = split_string ("program --test 20", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert (retval == FALSE);
  g_assert (error != NULL);
  /* An error occurred, so argv has not been changed */
  check_identical_stringv (argv_copy, argv);
  g_clear_error (&error);

  /* On failure, values should be reset */
  g_assert (error_test1_int == 0x12345678);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static xboolean_t
error_test2_pre_parse (xoption_context_t *context,
		       xoption_group_t   *group,
		       xpointer_t	  data,
		       xerror_t        **error)
{
  g_assert (strcmp (error_test2_string, "foo") == 0);

  return TRUE;
}

static xboolean_t
error_test2_post_parse (xoption_context_t *context,
			xoption_group_t   *group,
			xpointer_t	  data,
			xerror_t        **error)
{
  g_assert (strcmp (error_test2_string, "bar") == 0);

  /* Set an error in the post hook */
  g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, " ");

  return FALSE;
}

static void
error_test2 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  xoption_group_t *main_group;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_STRING, &error_test2_string, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  error_test2_string = "foo";

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Set pre and post parse hooks */
  main_group = g_option_context_get_main_group (context);
  xoption_group_set_parse_hooks (main_group,
				  error_test2_pre_parse, error_test2_post_parse);

  /* Now try parsing */
  argv = split_string ("program --test bar", &argc);
  argv_copy = copy_stringv (argv, argc);
  retval = g_option_context_parse (context, &argc, &argv, &error);

  g_assert (retval == FALSE);
  g_assert (error != NULL);
  check_identical_stringv (argv_copy, argv);
  g_clear_error (&error);

  g_assert (strcmp (error_test2_string, "foo") == 0);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static xboolean_t
error_test3_pre_parse (xoption_context_t *context,
		       xoption_group_t   *group,
		       xpointer_t	  data,
		       xerror_t        **error)
{
  g_assert (!error_test3_boolean);

  return TRUE;
}

static xboolean_t
error_test3_post_parse (xoption_context_t *context,
			xoption_group_t   *group,
			xpointer_t	  data,
			xerror_t        **error)
{
  g_assert (error_test3_boolean);

  /* Set an error in the post hook */
  g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, " ");

  return FALSE;
}

static void
error_test3 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  xoption_group_t *main_group;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_NONE, &error_test3_boolean, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  error_test3_boolean = FALSE;

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Set pre and post parse hooks */
  main_group = g_option_context_get_main_group (context);
  xoption_group_set_parse_hooks (main_group,
				  error_test3_pre_parse, error_test3_post_parse);

  /* Now try parsing */
  argv = split_string ("program --test", &argc);
  argv_copy = copy_stringv (argv, argc);
  retval = g_option_context_parse (context, &argc, &argv, &error);

  g_assert (retval == FALSE);
  g_assert (error != NULL);
  check_identical_stringv (argv_copy, argv);
  g_clear_error (&error);

  g_assert (!error_test3_boolean);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
arg_test1 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_INT, &arg_test1_int, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test 20 --test 30", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Last arg specified is the one that should be stored */
  g_assert (arg_test1_int == 30);

  /* We free all of the strings in a copy of argv, because now argv is a
   * subset - some have been removed in-place
   */
  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
arg_test2 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_STRING, &arg_test2_string, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test foo --test bar", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Last arg specified is the one that should be stored */
  g_assert (strcmp (arg_test2_string, "bar") == 0);

  g_free (arg_test2_string);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
arg_test3 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARXFILENAME, &arg_test3_filename, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test foo.txt", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Last arg specified is the one that should be stored */
  g_assert (strcmp (arg_test3_filename, "foo.txt") == 0);

  g_free (arg_test3_filename);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
arg_test4 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv_copy;
  xchar_t **argv;
  int argc;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_DOUBLE, &arg_test4_double, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test 20.0 --test 30.03", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Last arg specified is the one that should be stored */
  g_assert (arg_test4_double == 30.03);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
arg_test5 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  char *old_locale, *current_locale;
  const char *locale = "de_DE.UTF-8";
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_DOUBLE, &arg_test5_double, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test 20,0 --test 30,03", &argc);
  argv_copy = copy_stringv (argv, argc);

  /* set it to some locale that uses commas instead of decimal points */

  old_locale = xstrdup (setlocale (LC_NUMERIC, locale));
  current_locale = setlocale (LC_NUMERIC, NULL);
  if (strcmp (current_locale, locale) != 0)
    {
      fprintf (stderr, "Cannot set locale to %s, skipping\n", locale);
      goto cleanup;
    }

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Last arg specified is the one that should be stored */
  g_assert (arg_test5_double == 30.03);

 cleanup:
  setlocale (LC_NUMERIC, old_locale);
  g_free (old_locale);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
arg_test6 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_INT64, &arg_test6_int64, NULL, NULL },
      { "test2", 0, 0, G_OPTION_ARG_INT64, &arg_test6_int64_2, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test 4294967297 --test 4294967296 --test2 0xfffffffff", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Last arg specified is the one that should be stored */
  g_assert (arg_test6_int64 == G_GINT64_CONSTANT(4294967296));
  g_assert (arg_test6_int64_2 == G_GINT64_CONSTANT(0xfffffffff));

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static xboolean_t
callback_parse1 (const xchar_t *option_name, const xchar_t *value,
		 xpointer_t data, xerror_t **error)
{
	callback_test1_string = xstrdup (value);
	return TRUE;
}

static void
callback_test1 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_CALLBACK, callback_parse1, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test foo.txt", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  g_assert (strcmp (callback_test1_string, "foo.txt") == 0);

  g_free (callback_test1_string);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static xboolean_t
callback_parse2 (const xchar_t *option_name, const xchar_t *value,
		 xpointer_t data, xerror_t **error)
{
	callback_test2_int++;
	return TRUE;
}

static void
callback_test2 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "test", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, callback_parse2, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test --test", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  g_assert (callback_test2_int == 2);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static xboolean_t
callback_parse_optional (const xchar_t *option_name, const xchar_t *value,
		 xpointer_t data, xerror_t **error)
{
	callback_test_optional_boolean = TRUE;
	if (value)
		callback_test_optional_string = xstrdup (value);
	else
		callback_test_optional_string = NULL;
	return TRUE;
}

static void
callback_test_optional_1 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "test", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
	callback_parse_optional, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test foo.txt", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  g_assert (strcmp (callback_test_optional_string, "foo.txt") == 0);

  g_assert (callback_test_optional_boolean);

  g_free (callback_test_optional_string);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
callback_test_optional_2 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "test", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
	callback_parse_optional, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  g_assert (callback_test_optional_string == NULL);

  g_assert (callback_test_optional_boolean);

  g_free (callback_test_optional_string);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
callback_test_optional_3 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv_copy;
  xchar_t **argv;
  int argc;
  GOptionEntry entries [] =
    { { "test", 't', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
	callback_parse_optional, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program -t foo.txt", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  g_assert (strcmp (callback_test_optional_string, "foo.txt") == 0);

  g_assert (callback_test_optional_boolean);

  g_free (callback_test_optional_string);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}


static void
callback_test_optional_4 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "test", 't', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
	callback_parse_optional, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program -t", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  g_assert (callback_test_optional_string == NULL);

  g_assert (callback_test_optional_boolean);

  g_free (callback_test_optional_string);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
callback_test_optional_5 (void)
{
  xoption_context_t *context;
  xboolean_t dummy;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "dummy", 'd', 0, G_OPTION_ARG_NONE, &dummy, NULL, NULL },
      { "test", 't', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
	callback_parse_optional, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test --dummy", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  g_assert (callback_test_optional_string == NULL);

  g_assert (callback_test_optional_boolean);

  g_free (callback_test_optional_string);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
callback_test_optional_6 (void)
{
  xoption_context_t *context;
  xboolean_t dummy;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "dummy", 'd', 0, G_OPTION_ARG_NONE, &dummy, NULL, NULL },
      { "test", 't', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
	callback_parse_optional, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program -t -d", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  g_assert (callback_test_optional_string == NULL);

  g_assert (callback_test_optional_boolean);

  g_free (callback_test_optional_string);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
callback_test_optional_7 (void)
{
  xoption_context_t *context;
  xboolean_t dummy;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "dummy", 'd', 0, G_OPTION_ARG_NONE, &dummy, NULL, NULL },
      { "test", 't', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
	callback_parse_optional, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program -td", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  g_assert (callback_test_optional_string == NULL);

  g_assert (callback_test_optional_boolean);

  g_free (callback_test_optional_string);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
callback_test_optional_8 (void)
{
  xoption_context_t *context;
  xboolean_t dummy;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "dummy", 'd', 0, G_OPTION_ARG_NONE, &dummy, NULL, NULL },
      { "test", 't', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
	callback_parse_optional, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program -dt foo.txt", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  g_assert (callback_test_optional_string);

  g_assert (callback_test_optional_boolean);

  g_free (callback_test_optional_string);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static xptr_array_t *callback_remaining_args;
static xboolean_t
callback_remaining_test1_callback (const xchar_t *option_name, const xchar_t *value,
		         xpointer_t data, xerror_t **error)
{
	xptr_array_add (callback_remaining_args, xstrdup (value));
	return TRUE;
}

static void
callback_remaining_test1 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_CALLBACK, callback_remaining_test1_callback, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  callback_remaining_args = xptr_array_new ();
  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program foo.txt blah.txt", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  g_assert (callback_remaining_args->len == 2);
  g_assert (strcmp (callback_remaining_args->pdata[0], "foo.txt") == 0);
  g_assert (strcmp (callback_remaining_args->pdata[1], "blah.txt") == 0);

  xptr_array_foreach (callback_remaining_args, (GFunc) g_free, NULL);
  xptr_array_free (callback_remaining_args, TRUE);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static xboolean_t
callback_error (const xchar_t *option_name, const xchar_t *value,
                xpointer_t data, xerror_t **error)
{
  g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "42");
  return FALSE;
}

static void
callback_returns_false (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "error", 0, 0, G_OPTION_ARG_CALLBACK, callback_error, NULL, NULL },
      { "error-no-arg", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, callback_error, NULL, NULL },
      { "error-optional-arg", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, callback_error, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --error value", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
  g_assert (retval == FALSE);
  check_identical_stringv (argv_copy, argv);

  g_option_context_free (context);
  g_clear_error (&error);
  xstrfreev (argv_copy);
  g_free (argv);

  /* And again, this time with a no-arg variant */
  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  argv = split_string ("program --error-no-arg", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
  g_assert (retval == FALSE);
  check_identical_stringv (argv_copy, argv);

  g_option_context_free (context);
  g_clear_error (&error);
  xstrfreev (argv_copy);
  g_free (argv);

  /* And again, this time with an optional arg variant, with argument */
  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  argv = split_string ("program --error-optional-arg value", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
  g_assert (retval == FALSE);
  check_identical_stringv (argv_copy, argv);

  g_option_context_free (context);
  g_clear_error (&error);
  xstrfreev (argv_copy);
  g_free (argv);

  /* And again, this time with an optional arg variant, without argument */
  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  argv = split_string ("program --error-optional-arg", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
  g_assert (retval == FALSE);
  check_identical_stringv (argv_copy, argv);

  g_option_context_free (context);
  g_clear_error (&error);
  xstrfreev (argv_copy);
  g_free (argv);
}


static void
ignore_test1 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv, **argv_copy;
  int argc;
  xchar_t *arg;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_NONE, &ignore_test1_boolean, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (context, TRUE);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test --hello", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  arg = join_stringv (argc, argv);
  g_assert (strcmp (arg, "program --hello") == 0);

  g_free (arg);
  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
ignore_test2 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  xchar_t *arg;
  GOptionEntry entries [] =
    { { "test", 't', 0, G_OPTION_ARG_NONE, &ignore_test2_boolean, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (context, TRUE);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program -test", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  arg = join_stringv (argc, argv);
  g_assert (strcmp (arg, "program -es") == 0);

  g_free (arg);
  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
ignore_test3 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv, **argv_copy;
  int argc;
  xchar_t *arg;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_STRING, &ignore_test3_string, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (context, TRUE);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test foo --hello", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  arg = join_stringv (argc, argv);
  g_assert (strcmp (arg, "program --hello") == 0);

  g_assert (strcmp (ignore_test3_string, "foo") == 0);
  g_free (ignore_test3_string);

  g_free (arg);
  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
array_test1 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_STRING_ARRAY, &array_test1_array, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test foo --test bar", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  g_assert (strcmp (array_test1_array[0], "foo") == 0);
  g_assert (strcmp (array_test1_array[1], "bar") == 0);
  g_assert (array_test1_array[2] == NULL);

  xstrfreev (array_test1_array);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
add_test1 (void)
{
  xoption_context_t *context;

  GOptionEntry entries1 [] =
    { { "test1", 0, 0, G_OPTION_ARG_STRING_ARRAY, NULL, NULL, NULL },
      G_OPTION_ENTRY_NULL };
  GOptionEntry entries2 [] =
    { { "test2", 0, 0, G_OPTION_ARG_STRING_ARRAY, NULL, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries1, NULL);
  g_option_context_add_main_entries (context, entries2, NULL);

  g_option_context_free (context);
}

static void
empty_test2 (void)
{
  xoption_context_t *context;

  context = g_option_context_new (NULL);
  g_option_context_parse (context, NULL, NULL, NULL);

  g_option_context_free (context);
}

static void
empty_test3 (void)
{
  xoption_context_t *context;
  xint_t argc;
  xchar_t **argv;

  argc = 0;
  argv = NULL;

  context = g_option_context_new (NULL);
  g_option_context_parse (context, &argc, &argv, NULL);

  g_option_context_free (context);
}

/* check that non-option arguments are left in argv by default */
static void
rest_test1 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] = {
      { "test", 0, 0, G_OPTION_ARG_NONE, &ignore_test1_boolean, NULL, NULL },
      G_OPTION_ENTRY_NULL
  };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program foo --test bar", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  g_assert (ignore_test1_boolean);
  g_assert (strcmp (argv[0], "program") == 0);
  g_assert (strcmp (argv[1], "foo") == 0);
  g_assert (strcmp (argv[2], "bar") == 0);
  g_assert (argv[3] == NULL);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

/* check that -- works */
static void
rest_test2 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] = {
      { "test", 0, 0, G_OPTION_ARG_NONE, &ignore_test1_boolean, NULL, NULL },
      G_OPTION_ENTRY_NULL
  };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program foo --test -- -bar", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  g_assert (ignore_test1_boolean);
  g_assert (strcmp (argv[0], "program") == 0);
  g_assert (strcmp (argv[1], "foo") == 0);
  g_assert (strcmp (argv[2], "--") == 0);
  g_assert (strcmp (argv[3], "-bar") == 0);
  g_assert (argv[4] == NULL);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

/* check that -- stripping works */
static void
rest_test2a (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] = {
      { "test", 0, 0, G_OPTION_ARG_NONE, &ignore_test1_boolean, NULL, NULL },
      G_OPTION_ENTRY_NULL
  };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program foo --test -- bar", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  g_assert (ignore_test1_boolean);
  g_assert (strcmp (argv[0], "program") == 0);
  g_assert (strcmp (argv[1], "foo") == 0);
  g_assert (strcmp (argv[2], "bar") == 0);
  g_assert (argv[3] == NULL);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
rest_test2b (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] = {
      { "test", 0, 0, G_OPTION_ARG_NONE, &ignore_test1_boolean, NULL, NULL },
      G_OPTION_ENTRY_NULL
  };

  context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (context, TRUE);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program foo --test -bar --", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  g_assert (ignore_test1_boolean);
  g_assert (strcmp (argv[0], "program") == 0);
  g_assert (strcmp (argv[1], "foo") == 0);
  g_assert (strcmp (argv[2], "-bar") == 0);
  g_assert (argv[3] == NULL);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
rest_test2c (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] = {
      { "test", 0, 0, G_OPTION_ARG_NONE, &ignore_test1_boolean, NULL, NULL },
      G_OPTION_ENTRY_NULL
  };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test foo -- bar", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  g_assert (ignore_test1_boolean);
  g_assert (strcmp (argv[0], "program") == 0);
  g_assert (strcmp (argv[1], "foo") == 0);
  g_assert (strcmp (argv[2], "bar") == 0);
  g_assert (argv[3] == NULL);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
rest_test2d (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] = {
      { "test", 0, 0, G_OPTION_ARG_NONE, &ignore_test1_boolean, NULL, NULL },
      G_OPTION_ENTRY_NULL
  };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test -- -bar", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  g_assert (ignore_test1_boolean);
  g_assert (strcmp (argv[0], "program") == 0);
  g_assert (strcmp (argv[1], "--") == 0);
  g_assert (strcmp (argv[2], "-bar") == 0);
  g_assert (argv[3] == NULL);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}


/* check that G_OPTION_REMAINING collects non-option arguments */
static void
rest_test3 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] = {
      { "test", 0, 0, G_OPTION_ARG_NONE, &ignore_test1_boolean, NULL, NULL },
      { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &array_test1_array, NULL, NULL },
      G_OPTION_ENTRY_NULL
  };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program foo --test bar", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  g_assert (ignore_test1_boolean);
  g_assert (strcmp (array_test1_array[0], "foo") == 0);
  g_assert (strcmp (array_test1_array[1], "bar") == 0);
  g_assert (array_test1_array[2] == NULL);

  xstrfreev (array_test1_array);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}


/* check that G_OPTION_REMAINING and -- work together */
static void
rest_test4 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] = {
      { "test", 0, 0, G_OPTION_ARG_NONE, &ignore_test1_boolean, NULL, NULL },
      { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &array_test1_array, NULL, NULL },
      G_OPTION_ENTRY_NULL
  };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program foo --test -- -bar", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  g_assert (ignore_test1_boolean);
  g_assert (strcmp (array_test1_array[0], "foo") == 0);
  g_assert (strcmp (array_test1_array[1], "-bar") == 0);
  g_assert (array_test1_array[2] == NULL);

  xstrfreev (array_test1_array);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

/* test that G_OPTION_REMAINING works with G_OPTION_ARXFILENAME_ARRAY */
static void
rest_test5 (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] = {
      { "test", 0, 0, G_OPTION_ARG_NONE, &ignore_test1_boolean, NULL, NULL },
      { G_OPTION_REMAINING, 0, 0, G_OPTION_ARXFILENAME_ARRAY, &array_test1_array, NULL, NULL },
      G_OPTION_ENTRY_NULL
  };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program foo --test bar", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Check array */
  g_assert (ignore_test1_boolean);
  g_assert (strcmp (array_test1_array[0], "foo") == 0);
  g_assert (strcmp (array_test1_array[1], "bar") == 0);
  g_assert (array_test1_array[2] == NULL);

  xstrfreev (array_test1_array);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
unknown_short_test (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  GOptionEntry entries [] = { G_OPTION_ENTRY_NULL };

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=166609");

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program -0", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert (!retval);
  g_assert (error != NULL);
  g_clear_error (&error);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

/* test that lone dashes are treated as non-options */
static void
lonely_dash_test (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=168008");

  context = g_option_context_new (NULL);

  /* Now try parsing */
  argv = split_string ("program -", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  g_assert (argv[1] && strcmp (argv[1], "-") == 0);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

/* test that three dashes are treated as non-options */
static void
triple_dash_test (void)
{
  xoption_context_t *context;
  xoption_group_t *group;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  xint_t arg1, arg2;
  GOptionEntry entries [] =
    { { "foo", 0, 0, G_OPTION_ARG_INT, &arg1, NULL, NULL},
      G_OPTION_ENTRY_NULL
    };
  GOptionEntry group_entries [] =
    { { "test", 0, 0, G_OPTION_ARG_INT, &arg2, NULL, NULL},
      G_OPTION_ENTRY_NULL
    };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  group = xoption_group_new ("group", "Group description", "Group help", NULL, NULL);
  xoption_group_add_entries (group, group_entries);

  g_option_context_add_group (context, group);

  /* Now try parsing */
  argv = split_string ("program ---test 42", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_error (error, G_OPTION_ERROR, G_OPTION_ERROR_UNKNOWN_OPTION);
  g_assert (retval == FALSE);

  g_option_context_free (context);
  g_clear_error (&error);
  xstrfreev (argv_copy);
  g_free (argv);
}

static void
missing_arg_test (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  xchar_t *arg = NULL;
  GOptionEntry entries [] =
    { { "test", 't', 0, G_OPTION_ARG_STRING, &arg, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=305576");

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert (retval == FALSE);
  g_assert (error != NULL);
  /* An error occurred, so argv has not been changed */
  check_identical_stringv (argv_copy, argv);
  g_clear_error (&error);

  xstrfreev (argv_copy);
  g_free (argv);

  /* Try parsing again */
  argv = split_string ("program -t", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert (retval == FALSE);
  g_assert (error != NULL);
  /* An error occurred, so argv has not been changed */
  check_identical_stringv (argv_copy, argv);
  g_clear_error (&error);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);

  /* Checking g_option_context_parse_strv on NULL args */
  context = g_option_context_new (NULL);
  g_assert_true (g_option_context_parse_strv (context, NULL, NULL));
  g_option_context_free (context);
}

static xchar_t *test_arg;

static xboolean_t cb (const xchar_t  *option_name,
                    const xchar_t  *value,
                    xpointer_t      data,
                    xerror_t      **error)
{
  test_arg = xstrdup (value);
  return TRUE;
}

static void
dash_arg_test (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv;
  xchar_t **argv_copy;
  int argc;
  xboolean_t argb = FALSE;
  GOptionEntry entries [] =
    { { "test", 't', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, cb, NULL, NULL },
      { "three", '3', 0, G_OPTION_ARG_NONE, &argb, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=577638");

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test=-3", &argc);
  argv_copy = copy_stringv (argv, argc);

  test_arg = NULL;
  error = NULL;
  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert (retval);
  g_assert_no_error (error);
  g_assert_cmpstr (test_arg, ==, "-3");

  xstrfreev (argv_copy);
  g_free (argv);
  g_free (test_arg);
  test_arg = NULL;

  /* Try parsing again */
  argv = split_string ("program --test -3", &argc);
  argv_copy = copy_stringv (argv, argc);

  error = NULL;
  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);
  g_assert_cmpstr (test_arg, ==, NULL);

  g_option_context_free (context);
  xstrfreev (argv_copy);
  g_free (argv);
}

static void
test_basic (void)
{
  xoption_context_t *context;
  xchar_t *arg = NULL;
  GOptionEntry entries [] =
    { { "test", 't', 0, G_OPTION_ARG_STRING, &arg, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  g_assert (g_option_context_get_help_enabled (context));
  g_assert (!g_option_context_get_ignore_unknown_options (context));
  g_assert_cmpstr (g_option_context_get_summary (context), ==, NULL);
  g_assert_cmpstr (g_option_context_get_description (context), ==, NULL);

  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_ignore_unknown_options (context, TRUE);
  g_option_context_set_summary (context, "summary");
  g_option_context_set_description(context, "description");

  g_assert (!g_option_context_get_help_enabled (context));
  g_assert (g_option_context_get_ignore_unknown_options (context));
  g_assert_cmpstr (g_option_context_get_summary (context), ==, "summary");
  g_assert_cmpstr (g_option_context_get_description (context), ==, "description");

  g_option_context_free (context);
}

typedef struct {
  xboolean_t parameter_seen;
  xboolean_t summary_seen;
  xboolean_t description_seen;
  xboolean_t destroyed;
} TranslateData;

static const xchar_t *
translate_func (const xchar_t *str,
                xpointer_t     data)
{
  TranslateData *d = data;

  if (strcmp (str, "parameter") == 0)
    d->parameter_seen = TRUE;
  else if (strcmp (str, "summary") == 0)
    d->summary_seen = TRUE;
  else if (strcmp (str, "description") == 0)
    d->description_seen = TRUE;

  return str;
}

static void
destroy_notify (xpointer_t data)
{
  TranslateData *d = data;

  d->destroyed = TRUE;
}

static void
test_translate (void)
{
  xoption_context_t *context;
  xchar_t *arg = NULL;
  GOptionEntry entries [] =
    { { "test", 't', 0, G_OPTION_ARG_STRING, &arg, NULL, NULL },
      G_OPTION_ENTRY_NULL };
  TranslateData data = { 0, };
  xchar_t *str;

  context = g_option_context_new ("parameter");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_set_summary (context, "summary");
  g_option_context_set_description (context, "description");

  g_option_context_set_translate_func (context, translate_func, &data, destroy_notify);

  str = g_option_context_get_help (context, FALSE, NULL);
  g_free (str);
  g_option_context_free (context);

  g_assert (data.parameter_seen);
  g_assert (data.summary_seen);
  g_assert (data.description_seen);
  g_assert (data.destroyed);
}

static void
test_help (void)
{
  xoption_context_t *context;
  xoption_group_t *group;
  xchar_t *str;
  xchar_t *arg = NULL;
  xchar_t **sarr = NULL;
  GOptionEntry entries[] = {
    { "test", 't', 0, G_OPTION_ARG_STRING, &arg, "Test tests", "Argument to use in test" },
    { "test2", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, NULL, "Tests also", NULL },
    { "frob", 0, 0, G_OPTION_ARG_NONE, NULL, "Main frob", NULL },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &sarr, "Rest goes here", "REST" },
    G_OPTION_ENTRY_NULL
  };
  GOptionEntry group_entries[] = {
    { "test", 't', 0, G_OPTION_ARG_STRING, &arg, "Group test", "Group test arg" },
    { "frob", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE, NULL, "Group frob", NULL },
    G_OPTION_ENTRY_NULL
  };

  context = g_option_context_new ("blabla");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_set_summary (context, "Summary");
  g_option_context_set_description (context, "Description");

  group = xoption_group_new ("group1", "Group1-description", "Group1-help", NULL, NULL);
  xoption_group_add_entries (group, group_entries);

  g_option_context_add_group (context, group);

  str = g_option_context_get_help (context, FALSE, NULL);
  g_assert (strstr (str, "blabla") != NULL);
  g_assert (strstr (str, "Test tests") != NULL);
  g_assert (strstr (str, "Argument to use in test") != NULL);
  g_assert (strstr (str, "Tests also") == NULL);
  g_assert (strstr (str, "REST") != NULL);
  g_assert (strstr (str, "Summary") != NULL);
  g_assert (strstr (str, "Description") != NULL);
  g_assert (strstr (str, "--help") != NULL);
  g_assert (strstr (str, "--help-all") != NULL);
  g_assert (strstr (str, "--help-group1") != NULL);
  g_assert (strstr (str, "Group1-description") != NULL);
  g_assert (strstr (str, "Group1-help") != NULL);
  g_assert (strstr (str, "Group test arg") != NULL);
  g_assert (strstr (str, "Group frob") != NULL);
  g_assert (strstr (str, "Main frob") != NULL);
  g_assert (strstr (str, "--frob") != NULL);
  g_assert (strstr (str, "--group1-test") != NULL);
  g_assert (strstr (str, "--group1-frob") == NULL);
  g_free (str);

  g_option_context_free (context);
}

static void
test_help_no_options (void)
{
  xoption_context_t *context;
  xchar_t **sarr = NULL;
  GOptionEntry entries[] = {
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &sarr, "Rest goes here", "REST" },
    G_OPTION_ENTRY_NULL
  };
  xchar_t *str;

  context = g_option_context_new ("blabla");
  g_option_context_add_main_entries (context, entries, NULL);

  str = g_option_context_get_help (context, FALSE, NULL);
  g_assert (strstr (str, "blabla") != NULL);
  g_assert (strstr (str, "REST") != NULL);
  g_assert (strstr (str, "Help Options") != NULL);
  g_assert (strstr (str, "Application Options") == NULL);

  g_free (str);
  g_option_context_free (context);
}

static void
test_help_no_help_options (void)
{
  xoption_context_t *context;
  xoption_group_t *group;
  xchar_t *str;
  xchar_t *arg = NULL;
  xchar_t **sarr = NULL;
  GOptionEntry entries[] = {
    { "test", 't', 0, G_OPTION_ARG_STRING, &arg, "Test tests", "Argument to use in test" },
    { "test2", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, NULL, "Tests also", NULL },
    { "frob", 0, 0, G_OPTION_ARG_NONE, NULL, "Main frob", NULL },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &sarr, "Rest goes here", "REST" },
    G_OPTION_ENTRY_NULL
  };
  GOptionEntry group_entries[] = {
    { "test", 't', 0, G_OPTION_ARG_STRING, &arg, "Group test", "Group test arg" },
    { "frob", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE, NULL, "Group frob", NULL },
    G_OPTION_ENTRY_NULL
  };

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=697652");

  context = g_option_context_new ("blabla");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_set_summary (context, "Summary");
  g_option_context_set_description (context, "Description");
  g_option_context_set_help_enabled (context, FALSE);

  group = xoption_group_new ("group1", "Group1-description", "Group1-help", NULL, NULL);
  xoption_group_add_entries (group, group_entries);

  g_option_context_add_group (context, group);

  str = g_option_context_get_help (context, FALSE, NULL);
  g_assert (strstr (str, "blabla") != NULL);
  g_assert (strstr (str, "Test tests") != NULL);
  g_assert (strstr (str, "Argument to use in test") != NULL);
  g_assert (strstr (str, "Tests also") == NULL);
  g_assert (strstr (str, "REST") != NULL);
  g_assert (strstr (str, "Summary") != NULL);
  g_assert (strstr (str, "Description") != NULL);
  g_assert (strstr (str, "Help Options") == NULL);
  g_assert (strstr (str, "--help") == NULL);
  g_assert (strstr (str, "--help-all") == NULL);
  g_assert (strstr (str, "--help-group1") == NULL);
  g_assert (strstr (str, "Group1-description") != NULL);
  g_assert (strstr (str, "Group1-help") == NULL);
  g_assert (strstr (str, "Group test arg") != NULL);
  g_assert (strstr (str, "Group frob") != NULL);
  g_assert (strstr (str, "Main frob") != NULL);
  g_assert (strstr (str, "--frob") != NULL);
  g_assert (strstr (str, "--group1-test") != NULL);
  g_assert (strstr (str, "--group1-frob") == NULL);
  g_free (str);

  g_option_context_free (context);
}

static void
set_bool (xpointer_t data)
{
  xboolean_t *b = data;

  *b = TRUE;
}

static void
test_main_group (void)
{
  xoption_context_t *context;
  xoption_group_t *group;
  xboolean_t b = FALSE;

  context = g_option_context_new (NULL);
  g_assert (g_option_context_get_main_group (context) == NULL);
  group = xoption_group_new ("name", "description", "hlep", &b, set_bool);
  g_option_context_add_group (context, group);
  group = xoption_group_new ("name2", "description", "hlep", NULL, NULL);
  g_option_context_add_group (context, group);
  g_assert (g_option_context_get_main_group (context) == NULL);
  group = xoption_group_new ("name", "description", "hlep", NULL, NULL);
  g_option_context_set_main_group (context, group);
  g_assert (g_option_context_get_main_group (context) == group);

  g_option_context_free (context);

  g_assert (b);
}

static xboolean_t error_func_called = FALSE;

static void
error_func (xoption_context_t  *context,
            xoption_group_t    *group,
            xpointer_t         data,
            xerror_t         **error)
{
  g_assert_cmpint (GPOINTER_TO_INT(data), ==, 1234);
  error_func_called = TRUE;
}

static void
test_error_hook (void)
{
  xoption_context_t *context;
  xchar_t *arg = NULL;
  GOptionEntry entries [] =
    { { "test", 't', 0, G_OPTION_ARG_STRING, &arg, NULL, NULL },
      G_OPTION_ENTRY_NULL };
  xoption_group_t *group;
  xchar_t **argv;
  xchar_t **argv_copy;
  xint_t argc;
  xboolean_t retval;
  xerror_t *error = NULL;

  context = g_option_context_new (NULL);
  group = xoption_group_new ("name", "description", "hlep", GINT_TO_POINTER(1234), NULL);
  xoption_group_add_entries (group, entries);
  g_option_context_set_main_group (context, group);
  xoption_group_set_error_hook (g_option_context_get_main_group (context),
                                 error_func);

  argv = split_string ("program --test", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert (retval == FALSE);
  g_assert (error != NULL);
  /* An error occurred, so argv has not been changed */
  check_identical_stringv (argv_copy, argv);
  g_clear_error (&error);

  g_assert (error_func_called);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
test_group_parse (void)
{
  xoption_context_t *context;
  xoption_group_t *group;
  xchar_t *arg1 = NULL;
  xchar_t *arg2 = NULL;
  xchar_t *arg3 = NULL;
  xchar_t *arg4 = NULL;
  xchar_t *arg5 = NULL;
  GOptionEntry entries[] = {
    { "test", 't', 0, G_OPTION_ARG_STRING, &arg1, NULL, NULL },
    { "faz", 'f', 0, G_OPTION_ARG_STRING, &arg2, NULL, NULL },
    G_OPTION_ENTRY_NULL
  };
  GOptionEntry group_entries[] = {
    { "test", 0, 0, G_OPTION_ARG_STRING, &arg3, NULL, NULL },
    { "frob", 'f', 0, G_OPTION_ARG_STRING, &arg4, NULL, NULL },
    { "faz", 'z', 0, G_OPTION_ARG_STRING, &arg5, NULL, NULL },
    G_OPTION_ENTRY_NULL
  };
  xchar_t **argv, **orig_argv;
  xint_t argc;
  xerror_t *error = NULL;
  xboolean_t retval;

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);
  group = xoption_group_new ("group", "A group", "help for group", NULL, NULL);
  xoption_group_add_entries (group, group_entries);
  g_option_context_add_group (context, group);

  argv = split_string ("program --test arg1 -f arg2 --group-test arg3 --frob arg4 -z arg5", &argc);
  orig_argv = g_memdup2 (argv, (argc + 1) * sizeof (char *));

  retval = g_option_context_parse (context, &argc, &argv, &error);

  g_assert_no_error (error);
  g_assert (retval);
  g_assert_cmpstr (arg1, ==, "arg1");
  g_assert_cmpstr (arg2, ==, "arg2");
  g_assert_cmpstr (arg3, ==, "arg3");
  g_assert_cmpstr (arg4, ==, "arg4");
  g_assert_cmpstr (arg5, ==, "arg5");

  g_free (arg1);
  g_free (arg2);
  g_free (arg3);
  g_free (arg4);
  g_free (arg5);

  g_free (argv);
  xstrfreev (orig_argv);
  g_option_context_free (context);
}

static xint_t
option_context_parse_command_line (xoption_context_t *context,
                                   const xchar_t    *command_line)
{
  xchar_t **argv;
  xuint_t argv_len, argv_new_len;
  xboolean_t success;

  argv = split_string (command_line, NULL);
  argv_len = xstrv_length (argv);

  success = g_option_context_parse_strv (context, &argv, NULL);
  argv_new_len = xstrv_length (argv);

  xstrfreev (argv);
  return success ? (xint_t) (argv_len - argv_new_len) : -1;
}

static void
test_strict_posix (void)
{
  xoption_context_t *context;
  xboolean_t foo;
  xboolean_t bar;
  GOptionEntry entries[] = {
    { "foo", 'f', 0, G_OPTION_ARG_NONE, &foo, NULL, NULL },
    { "bar", 'b', 0, G_OPTION_ARG_NONE, &bar, NULL, NULL },
    G_OPTION_ENTRY_NULL
  };
  xint_t n_parsed;

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  foo = bar = FALSE;
  g_option_context_set_strict_posix (context, FALSE);
  n_parsed = option_context_parse_command_line (context, "program --foo command --bar");
  g_assert_cmpint (n_parsed, ==, 2);
  g_assert (foo == TRUE);
  g_assert (bar == TRUE);

  foo = bar = FALSE;
  g_option_context_set_strict_posix (context, TRUE);
  n_parsed = option_context_parse_command_line (context, "program --foo command --bar");
  g_assert_cmpint (n_parsed, ==, 1);
  g_assert (foo == TRUE);
  g_assert (bar == FALSE);

  foo = bar = FALSE;
  g_option_context_set_strict_posix (context, TRUE);
  n_parsed = option_context_parse_command_line (context, "program --foo --bar command");
  g_assert_cmpint (n_parsed, ==, 2);
  g_assert (foo == TRUE);
  g_assert (bar == TRUE);

  foo = bar = FALSE;
  g_option_context_set_strict_posix (context, TRUE);
  n_parsed = option_context_parse_command_line (context, "program command --foo --bar");
  g_assert_cmpint (n_parsed, ==, 0);
  g_assert (foo == FALSE);
  g_assert (bar == FALSE);

  g_option_context_free (context);
}

static void
flag_reverse_string (void)
{
  xoption_context_t *context;
  xchar_t *arg = NULL;
  GOptionEntry entries [] =
    { { "test", 't', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_STRING, &arg, NULL, NULL },
      G_OPTION_ENTRY_NULL };
  xchar_t **argv;
  xint_t argc;
  xboolean_t retval;
  xerror_t *error = NULL;

  if (!g_test_undefined ())
    return;

  context = g_option_context_new (NULL);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*ignoring reverse flag*");
  g_option_context_add_main_entries (context, entries, NULL);
  g_test_assert_expected_messages ();

  argv = split_string ("program --test bla", &argc);

  retval = g_option_context_parse_strv (context, &argv, &error);
  g_assert (retval == TRUE);
  g_assert_no_error (error);
  xstrfreev (argv);
  g_option_context_free (context);
  g_free (arg);
}

static void
flag_optional_int (void)
{
  xoption_context_t *context;
  xint_t arg = 0;
  GOptionEntry entries [] =
    { { "test", 't', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_INT, &arg, NULL, NULL },
      G_OPTION_ENTRY_NULL };
  xchar_t **argv;
  xint_t argc;
  xboolean_t retval;
  xerror_t *error = NULL;

  if (!g_test_undefined ())
    return;

  context = g_option_context_new (NULL);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*ignoring no-arg, optional-arg or filename flags*");
  g_option_context_add_main_entries (context, entries, NULL);
  g_test_assert_expected_messages ();

  argv = split_string ("program --test 5", &argc);

  retval = g_option_context_parse_strv (context, &argv, &error);
  g_assert (retval == TRUE);
  g_assert_no_error (error);
  xstrfreev (argv);
  g_option_context_free (context);
}

static void
short_remaining (void)
{
  xboolean_t ignore = FALSE;
  xboolean_t remaining = FALSE;
  xint_t number = 0;
  xchar_t* text = NULL;
  xchar_t** files = NULL;
  xerror_t* error = NULL;
  GOptionEntry entries[] =
  {
    { "ignore", 'i', 0, G_OPTION_ARG_NONE, &ignore, NULL, NULL },
    { "remaining", 'r', 0, G_OPTION_ARG_NONE, &remaining, NULL, NULL },
    { "number", 'n', 0, G_OPTION_ARG_INT, &number, NULL, NULL },
    { "text", 't', 0, G_OPTION_ARG_STRING, &text, NULL, NULL },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARXFILENAME_ARRAY, &files, NULL, NULL },
    G_OPTION_ENTRY_NULL
  };
  xoption_context_t* context;
  xchar_t **argv, **argv_copy;
  xint_t argc;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=729563");

  argv = split_string ("program -ri -n 4 -t hello file1 file2", &argc);
  argv_copy = copy_stringv (argv, argc);

  context = g_option_context_new (NULL);

  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_set_ignore_unknown_options (context, TRUE);

  g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);

  g_assert (ignore);
  g_assert (remaining);
  g_assert_cmpint (number, ==, 4);
  g_assert_cmpstr (text, ==, "hello");
  g_assert_cmpstr (files[0], ==, "file1");
  g_assert_cmpstr (files[1], ==, "file2");
  g_assert (files[2] == NULL);

  g_free (text);
  xstrfreev (files);
  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

static void
double_free (void)
{
  xchar_t* text = NULL;
  GOptionEntry entries[] =
  {
    { "known", 0, 0, G_OPTION_ARG_STRING, &text, NULL, NULL },
    G_OPTION_ENTRY_NULL
  };
  xoption_context_t* context;
  xchar_t **argv;
  xint_t argc;
  xerror_t *error = NULL;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=646926");

  argv = split_string ("program --known=foo --known=bar --unknown=baz", &argc);

  context = g_option_context_new (NULL);

  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_set_ignore_unknown_options (context, FALSE);
  g_option_context_parse (context, &argc, &argv, &error);

  g_assert_error (error, G_OPTION_ERROR, G_OPTION_ERROR_UNKNOWN_OPTION);
  g_assert_null (text);

  g_option_context_free (context);
  g_clear_error (&error);
  xstrfreev (argv);

}

static void
double_zero (void)
{
  xoption_context_t *context;
  xboolean_t retval;
  xerror_t *error = NULL;
  xchar_t **argv_copy;
  xchar_t **argv;
  int argc;
  double test_val = NAN;
  GOptionEntry entries [] =
    { { "test", 0, 0, G_OPTION_ARG_DOUBLE, &test_val, NULL, NULL },
      G_OPTION_ENTRY_NULL };

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);

  /* Now try parsing */
  argv = split_string ("program --test 0", &argc);
  argv_copy = copy_stringv (argv, argc);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_assert_no_error (error);
  g_assert (retval);

  /* Last arg specified is the one that should be stored */
  g_assert (test_val == 0);

  xstrfreev (argv_copy);
  g_free (argv);
  g_option_context_free (context);
}

int
main (int   argc,
      char *argv[])
{
  int i;
  xchar_t *test_name;

  g_setenv ("LC_ALL", "C", TRUE);
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/option/help/options", test_help);
  g_test_add_func ("/option/help/no-options", test_help_no_options);
  g_test_add_func ("/option/help/no-help-options", test_help_no_help_options);

  g_test_add_func ("/option/basic", test_basic);
  g_test_add_func ("/option/translate", test_translate);

  g_test_add_func ("/option/group/captions", test_group_captions);
  for (i = 0; i < 4; i++)
    {
      test_name = xstrdup_printf ("/option/group/captions/subprocess/help-%d", i);
      g_test_add_data_func (test_name, GINT_TO_POINTER (i),
                            test_group_captions_help);
      g_free (test_name);
      test_name = xstrdup_printf ("/option/group/captions/subprocess/help-all-%d", i);
      g_test_add_data_func (test_name, GINT_TO_POINTER (i),
                            test_group_captions_help_all);
      g_free (test_name);
      test_name = xstrdup_printf ("/option/group/captions/subprocess/help-test-%d", i);
      g_test_add_data_func (test_name, GINT_TO_POINTER (i),
                            test_group_captions_help_test);

      g_free (test_name);
    }

  g_test_add_func ("/option/group/main", test_main_group);
  g_test_add_func ("/option/group/error-hook", test_error_hook);
  g_test_add_func ("/option/group/parse", test_group_parse);
  g_test_add_func ("/option/strict-posix", test_strict_posix);

  /* Test that restoration on failure works */
  g_test_add_func ("/option/restoration/int", error_test1);
  g_test_add_func ("/option/restoration/string", error_test2);
  g_test_add_func ("/option/restoration/boolean", error_test3);

  /* Test that special argument parsing works */
  g_test_add_func ("/option/arg/repetition/int", arg_test1);
  g_test_add_func ("/option/arg/repetition/string", arg_test2);
  g_test_add_func ("/option/arg/repetition/filename", arg_test3);
  g_test_add_func ("/option/arg/repetition/double", arg_test4);
  g_test_add_func ("/option/arg/repetition/locale", arg_test5);
  g_test_add_func ("/option/arg/repetition/int64", arg_test6);

  /* Test string arrays */
  g_test_add_func ("/option/arg/array/string", array_test1);

  /* Test callback args */
  g_test_add_func ("/option/arg/callback/string", callback_test1);
  g_test_add_func ("/option/arg/callback/count", callback_test2);

  /* Test optional arg flag for callback */
  g_test_add_func ("/option/arg/callback/optional1", callback_test_optional_1);
  g_test_add_func ("/option/arg/callback/optional2", callback_test_optional_2);
  g_test_add_func ("/option/arg/callback/optional3", callback_test_optional_3);
  g_test_add_func ("/option/arg/callback/optional4", callback_test_optional_4);
  g_test_add_func ("/option/arg/callback/optional5", callback_test_optional_5);
  g_test_add_func ("/option/arg/callback/optional6", callback_test_optional_6);
  g_test_add_func ("/option/arg/callback/optional7", callback_test_optional_7);
  g_test_add_func ("/option/arg/callback/optional8", callback_test_optional_8);

  /* Test callback with G_OPTION_REMAINING */
  g_test_add_func ("/option/arg/remaining/callback", callback_remaining_test1);

  /* Test callbacks which return FALSE */
  g_test_add_func ("/option/arg/remaining/callback-false", callback_returns_false);

  /* Test ignoring options */
  g_test_add_func ("/option/arg/ignore/long", ignore_test1);
  g_test_add_func ("/option/arg/ignore/short", ignore_test2);
  g_test_add_func ("/option/arg/ignore/arg", ignore_test3);
  g_test_add_func ("/option/context/add", add_test1);

  /* Test parsing empty args */
  /* Note there used to be an empty1 here, but it effectively moved
   * to option-argv0.c.
   */
  g_test_add_func ("/option/context/empty2", empty_test2);
  g_test_add_func ("/option/context/empty3", empty_test3);

  /* Test handling of rest args */
  g_test_add_func ("/option/arg/rest/non-option", rest_test1);
  g_test_add_func ("/option/arg/rest/separator1", rest_test2);
  g_test_add_func ("/option/arg/rest/separator2", rest_test2a);
  g_test_add_func ("/option/arg/rest/separator3", rest_test2b);
  g_test_add_func ("/option/arg/rest/separator4", rest_test2c);
  g_test_add_func ("/option/arg/rest/separator5", rest_test2d);
  g_test_add_func ("/option/arg/remaining/non-option", rest_test3);
  g_test_add_func ("/option/arg/remaining/separator", rest_test4);
  g_test_add_func ("/option/arg/remaining/array", rest_test5);

  /* Test some invalid flag combinations */
  g_test_add_func ("/option/arg/reverse-string", flag_reverse_string);
  g_test_add_func ("/option/arg/optional-int", flag_optional_int);

  /* regression tests for individual bugs */
  g_test_add_func ("/option/bug/unknown-short", unknown_short_test);
  g_test_add_func ("/option/bug/lonely-dash", lonely_dash_test);
  g_test_add_func ("/option/bug/triple-dash", triple_dash_test);
  g_test_add_func ("/option/bug/missing-arg", missing_arg_test);
  g_test_add_func ("/option/bug/dash-arg", dash_arg_test);
  g_test_add_func ("/option/bug/short-remaining", short_remaining);
  g_test_add_func ("/option/bug/double-free", double_free);
  g_test_add_func ("/option/bug/double-zero", double_zero);

  return g_test_run();
}
