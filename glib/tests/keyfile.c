
#include <glib.h>
#include <glib/gstdio.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>

static xkey_file_t *
load_data (const xchar_t   *data,
           GKeyFileFlags  flags)
{
  xkey_file_t *keyfile;
  xerror_t *error = NULL;

  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, flags, &error);
  g_assert_no_error (error);
  return keyfile;
}

static void
check_error (xerror_t **error,
             xquark   domain,
             xint_t     code)
{
  g_assert_error (*error, domain, code);
  xerror_free (*error);
  *error = NULL;
}

static void
check_no_error (xerror_t **error)
{
  g_assert_no_error (*error);
}

static void
check_strinxvalue (xkey_file_t    *keyfile,
                    const xchar_t *group,
                    const xchar_t *key,
                    const xchar_t *expected)
{
  xerror_t *error = NULL;
  xchar_t *value;

  value = xkey_file_get_string (keyfile, group, key, &error);
  check_no_error (&error);
  g_assert_nonnull (value);
  g_assert_cmpstr (value, ==, expected);
  g_free (value);
}

static void
check_locale_strinxvalue (xkey_file_t    *keyfile,
                           const xchar_t *group,
                           const xchar_t *key,
                           const xchar_t *locale,
                           const xchar_t *expected)
{
  xerror_t *error = NULL;
  xchar_t *value;

  value = xkey_file_get_locale_string (keyfile, group, key, locale, &error);
  check_no_error (&error);
  g_assert_nonnull (value);
  g_assert_cmpstr (value, ==, expected);
  g_free (value);
}

static void
check_string_locale_value (xkey_file_t    *keyfile,
                           const xchar_t *group,
                           const xchar_t *key,
                           const xchar_t *locale,
                           const xchar_t *expected)
{
  xchar_t *value;

  value = xkey_file_get_locale_for_key (keyfile, group, key, locale);
  g_assert_cmpstr (value, ==, expected);
  g_free (value);
}

static void
check_strinxlist_value (xkey_file_t    *keyfile,
                         const xchar_t *group,
                         const xchar_t *key,
                         ...)
{
  xint_t i;
  xchar_t *v, **value;
  va_list args;
  xsize_t len;
  xerror_t *error = NULL;

  value = xkey_file_get_string_list (keyfile, group, key, &len, &error);
  check_no_error (&error);
  g_assert_nonnull (value);

  va_start (args, key);
  i = 0;
  v = va_arg (args, xchar_t*);
  while (v)
    {
      g_assert_nonnull (value[i]);
      g_assert_cmpstr (v, ==, value[i]);
      i++;
      v = va_arg (args, xchar_t*);
    }

  va_end (args);

  xstrfreev (value);
}

static void
check_locale_strinxlist_value (xkey_file_t    *keyfile,
                                const xchar_t *group,
                                const xchar_t *key,
                                const xchar_t *locale,
                                ...)
{
  xint_t i;
  xchar_t *v, **value;
  va_list args;
  xsize_t len;
  xerror_t *error = NULL;

  value = xkey_file_get_locale_string_list (keyfile, group, key, locale, &len, &error);
  check_no_error (&error);
  g_assert_nonnull (value);

  va_start (args, locale);
  i = 0;
  v = va_arg (args, xchar_t*);
  while (v)
    {
      g_assert_nonnull (value[i]);
      g_assert_cmpstr (v, ==, value[i]);
      i++;
      v = va_arg (args, xchar_t*);
    }

  va_end (args);

  xstrfreev (value);
}

static void
check_integer_list_value (xkey_file_t    *keyfile,
                          const xchar_t *group,
                          const xchar_t *key,
                          ...)
{
  xint_t i;
  xint_t v, *value;
  va_list args;
  xsize_t len;
  xerror_t *error = NULL;

  value = xkey_file_get_integer_list (keyfile, group, key, &len, &error);
  check_no_error (&error);
  g_assert_nonnull (value);

  va_start (args, key);
  i = 0;
  v = va_arg (args, xint_t);
  while (v != -100)
    {
      g_assert_cmpint (i, <, len);
      g_assert_cmpint (value[i], ==, v);
      i++;
      v = va_arg (args, xint_t);
    }

  va_end (args);

  g_free (value);
}

static void
check_double_list_value (xkey_file_t    *keyfile,
                          const xchar_t *group,
                          const xchar_t *key,
                          ...)
{
  xint_t i;
  xdouble_t v, *value;
  va_list args;
  xsize_t len;
  xerror_t *error = NULL;

  value = xkey_file_get_double_list (keyfile, group, key, &len, &error);
  check_no_error (&error);
  g_assert_nonnull (value);

  va_start (args, key);
  i = 0;
  v = va_arg (args, xdouble_t);
  while (v != -100)
    {
      g_assert_cmpint (i, <, len);
      g_assert_cmpfloat (value[i], ==, v);
      i++;
      v = va_arg (args, xdouble_t);
    }

  va_end (args);

  g_free (value);
}

static void
check_boolean_list_value (xkey_file_t    *keyfile,
                          const xchar_t *group,
                          const xchar_t *key,
                          ...)
{
  xint_t i;
  xboolean_t v, *value;
  va_list args;
  xsize_t len;
  xerror_t *error = NULL;

  value = xkey_file_get_boolean_list (keyfile, group, key, &len, &error);
  check_no_error (&error);
  g_assert_nonnull (value);

  va_start (args, key);
  i = 0;
  v = va_arg (args, xboolean_t);
  while (v != -100)
    {
      g_assert_cmpint (i, <, len);
      g_assert_cmpint (value[i], ==, v);
      i++;
      v = va_arg (args, xboolean_t);
    }

  va_end (args);

  g_free (value);
}

static void
check_boolean_value (xkey_file_t    *keyfile,
                     const xchar_t *group,
                     const xchar_t *key,
                     xboolean_t     expected)
{
  xerror_t *error = NULL;
  xboolean_t value;

  value = xkey_file_get_boolean (keyfile, group, key, &error);
  check_no_error (&error);
  g_assert_cmpint (value, ==, expected);
}

static void
check_integer_value (xkey_file_t    *keyfile,
                     const xchar_t *group,
                     const xchar_t *key,
                     xint_t         expected)
{
  xerror_t *error = NULL;
  xint_t value;

  value = xkey_file_get_integer (keyfile, group, key, &error);
  check_no_error (&error);
  g_assert_cmpint (value, ==, expected);
}

static void
check_double_value (xkey_file_t    *keyfile,
                     const xchar_t *group,
                     const xchar_t *key,
                     xdouble_t      expected)
{
  xerror_t *error = NULL;
  xdouble_t value;

  value = xkey_file_get_double (keyfile, group, key, &error);
  check_no_error (&error);
  g_assert_cmpfloat (value, ==, expected);
}

static void
check_name (const xchar_t *what,
            const xchar_t *value,
            const xchar_t *expected,
            xint_t         position)
{
  g_assert_cmpstr (value, ==, expected);
}

static void
check_length (const xchar_t *what,
              xint_t         n_items,
              xint_t         length,
              xint_t         expected)
{
  g_assert_cmpint (n_items, ==, length);
  g_assert_cmpint (n_items, ==, expected);
}


/* check that both \n and \r\n are accepted as line ends,
 * and that stray \r are passed through
 */
static void
test_line_ends (void)
{
  xkey_file_t *keyfile;

  const xchar_t *data =
    "[group1]\n"
    "key1=value1\n"
    "key2=value2\r\n"
    "[group2]\r\n"
    "key3=value3\r\r\n"
    "key4=value4\n";

  keyfile = load_data (data, 0);

  check_strinxvalue (keyfile, "group1", "key1", "value1");
  check_strinxvalue (keyfile, "group1", "key2", "value2");
  check_strinxvalue (keyfile, "group2", "key3", "value3\r");
  check_strinxvalue (keyfile, "group2", "key4", "value4");

  xkey_file_free (keyfile);
}

/* check handling of whitespace
 */
static void
test_whitespace (void)
{
  xkey_file_t *keyfile;

  const xchar_t *data =
    "[group1]\n"
    "key1 = value1\n"
    "key2\t=\tvalue2\n"
    " [ group2 ] \n"
    "key3  =  value3  \n"
    "key4  =  value \t4\n"
    "  key5  =  value5\n";

  keyfile = load_data (data, 0);

  check_strinxvalue (keyfile, "group1", "key1", "value1");
  check_strinxvalue (keyfile, "group1", "key2", "value2");
  check_strinxvalue (keyfile, " group2 ", "key3", "value3  ");
  check_strinxvalue (keyfile, " group2 ", "key4", "value \t4");
  check_strinxvalue (keyfile, " group2 ", "key5", "value5");

  xkey_file_free (keyfile);
}

/* check handling of comments
 */
static void
test_comments (void)
{
  xkey_file_t *keyfile;
  xchar_t **names;
  xsize_t len;
  xerror_t *error = NULL;
  xchar_t *comment;

  const xchar_t *data =
    "# top comment\n"
    "# top comment, continued\n"
    "[group1]\n"
    "key1 = value1\n"
    "# key comment\n"
    "# key comment, continued\n"
    "key2 = value2\n"
    "# line end check\r\n"
    "key3 = value3\n"
    "# single line comment\n"
    "key4 = value4\n"
    "# group comment\n"
    "# group comment, continued\n"
    "[group2]\n";

  const xchar_t *top_comment = " top comment\n top comment, continued";
  const xchar_t *group_comment = " group comment\n group comment, continued";
  const xchar_t *key_comment = " key comment\n key comment, continued";
  const xchar_t *key4_comment = " single line comment";

  keyfile = load_data (data, 0);

  check_strinxvalue (keyfile, "group1", "key1", "value1");
  check_strinxvalue (keyfile, "group1", "key2", "value2");
  check_strinxvalue (keyfile, "group1", "key3", "value3");
  check_strinxvalue (keyfile, "group1", "key4", "value4");

  names = xkey_file_get_keys (keyfile, "group1", &len, &error);
  check_no_error (&error);

  check_length ("keys", xstrv_length (names), len, 4);
  check_name ("key", names[0], "key1", 0);
  check_name ("key", names[1], "key2", 1);
  check_name ("key", names[2], "key3", 2);
  check_name ("key", names[3], "key4", 3);

  xstrfreev (names);

  xkey_file_free (keyfile);

  keyfile = load_data (data, G_KEY_FILE_KEEP_COMMENTS);

  names = xkey_file_get_keys (keyfile, "group1", &len, &error);
  check_no_error (&error);

  check_length ("keys", xstrv_length (names), len, 4);
  check_name ("key", names[0], "key1", 0);
  check_name ("key", names[1], "key2", 1);
  check_name ("key", names[2], "key3", 2);
  check_name ("key", names[3], "key4", 3);

  xstrfreev (names);

  comment = xkey_file_get_comment (keyfile, NULL, NULL, &error);
  check_no_error (&error);
  check_name ("top comment", comment, top_comment, 0);
  g_free (comment);

  comment = xkey_file_get_comment (keyfile, "group1", "key2", &error);
  check_no_error (&error);
  check_name ("key comment", comment, key_comment, 0);
  g_free (comment);

  xkey_file_remove_comment (keyfile, "group1", "key2", &error);
  check_no_error (&error);
  comment = xkey_file_get_comment (keyfile, "group1", "key2", &error);
  check_no_error (&error);
  g_assert_null (comment);

  comment = xkey_file_get_comment (keyfile, "group1", "key4", &error);
  check_no_error (&error);
  check_name ("key comment", comment, key4_comment, 0);
  g_free (comment);

  comment = xkey_file_get_comment (keyfile, "group2", NULL, &error);
  check_no_error (&error);
  check_name ("group comment", comment, group_comment, 0);
  g_free (comment);

  comment = xkey_file_get_comment (keyfile, "group3", NULL, &error);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_GROUP_NOT_FOUND);
  g_assert_null (comment);

  xkey_file_free (keyfile);
}


/* check key and group listing */
static void
test_listing (void)
{
  xkey_file_t *keyfile;
  xchar_t **names;
  xsize_t len;
  xchar_t *start;
  xerror_t *error = NULL;

  const xchar_t *data =
    "[group1]\n"
    "key1=value1\n"
    "key2=value2\n"
    "[group2]\n"
    "key3=value3\n"
    "key4=value4\n";

  keyfile = load_data (data, 0);

  names = xkey_file_get_groups (keyfile, &len);
  g_assert_nonnull (names);

  check_length ("groups", xstrv_length (names), len, 2);
  check_name ("group name", names[0], "group1", 0);
  check_name ("group name", names[1], "group2", 1);

  xstrfreev (names);

  names = xkey_file_get_keys (keyfile, "group1", &len, &error);
  check_no_error (&error);

  check_length ("keys", xstrv_length (names), len, 2);
  check_name ("key", names[0], "key1", 0);
  check_name ("key", names[1], "key2", 1);

  xstrfreev (names);

  names = xkey_file_get_keys (keyfile, "no-such-group", &len, &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND);

  xstrfreev (names);

  g_assert_true (xkey_file_has_group (keyfile, "group1"));
  g_assert_true (xkey_file_has_group (keyfile, "group2"));
  g_assert_false (xkey_file_has_group (keyfile, "group10"));
  g_assert_false (xkey_file_has_group (keyfile, "group20"));

  start = xkey_file_get_start_group (keyfile);
  g_assert_cmpstr (start, ==, "group1");
  g_free (start);

  g_assert_true (xkey_file_has_key (keyfile, "group1", "key1", &error));
  check_no_error (&error);
  g_assert_true (xkey_file_has_key (keyfile, "group2", "key3", &error));
  check_no_error (&error);
  g_assert_false (xkey_file_has_key (keyfile, "group2", "no-such-key", NULL));

  xkey_file_has_key (keyfile, "no-such-group", "key", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND);

  xkey_file_free (keyfile);
}

/* check parsing of string values */
static void
test_string (void)
{
  xkey_file_t *keyfile;
  xerror_t *error = NULL;
  xchar_t *value;
  const xchar_t * const list[3] = {
    "one",
    "two;andahalf",
    "3",
  };
  const xchar_t *data =
    "[valid]\n"
    "key1=\\s\\n\\t\\r\\\\\n"
    "key2=\"quoted\"\n"
    "key3='quoted'\n"
    "key4=\xe2\x89\xa0\xe2\x89\xa0\n"
    "key5=  leading space\n"
    "key6=trailing space  \n"
    "[invalid]\n"
    "key1=\\a\\b\\0800xff\n"
    "key2=blabla\\\n";

  keyfile = load_data (data, 0);

  check_strinxvalue (keyfile, "valid", "key1", " \n\t\r\\");
  check_strinxvalue (keyfile, "valid", "key2", "\"quoted\"");
  check_strinxvalue (keyfile, "valid", "key3", "'quoted'");
  check_strinxvalue (keyfile, "valid", "key4", "\xe2\x89\xa0\xe2\x89\xa0");
  check_strinxvalue (keyfile, "valid", "key5", "leading space");
  check_strinxvalue (keyfile, "valid", "key6", "trailing space  ");

  value = xkey_file_get_string (keyfile, "invalid", "key1", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);
  g_free (value);

  value = xkey_file_get_string (keyfile, "invalid", "key2", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);
  g_free (value);

  xkey_file_set_string (keyfile, "inserted", "key1", "simple");
  xkey_file_set_string (keyfile, "inserted", "key2", " leading space");
  xkey_file_set_string (keyfile, "inserted", "key3", "\tleading tab");
  xkey_file_set_string (keyfile, "inserted", "key4", "new\nline");
  xkey_file_set_string (keyfile, "inserted", "key5", "carriage\rreturn");
  xkey_file_set_string (keyfile, "inserted", "key6", "slash\\yay!");
  xkey_file_set_string_list (keyfile, "inserted", "key7", list, 3);

  check_strinxvalue (keyfile, "inserted", "key1", "simple");
  check_strinxvalue (keyfile, "inserted", "key2", " leading space");
  check_strinxvalue (keyfile, "inserted", "key3", "\tleading tab");
  check_strinxvalue (keyfile, "inserted", "key4", "new\nline");
  check_strinxvalue (keyfile, "inserted", "key5", "carriage\rreturn");
  check_strinxvalue (keyfile, "inserted", "key6", "slash\\yay!");
  check_strinxlist_value (keyfile, "inserted", "key7", "one", "two;andahalf", "3", NULL);

  xkey_file_free (keyfile);
}

/* check parsing of boolean values */
static void
test_boolean (void)
{
  xkey_file_t *keyfile;
  xerror_t *error = NULL;

  const xchar_t *data =
    "[valid]\n"
    "key1=true\n"
    "key2=false\n"
    "key3=1\n"
    "key4=0\n"
    "key5= true\n"
    "key6=true \n"
    "[invalid]\n"
    "key1=t\n"
    "key2=f\n"
    "key3=yes\n"
    "key4=no\n";

  keyfile = load_data (data, 0);

  check_boolean_value (keyfile, "valid", "key1", TRUE);
  check_boolean_value (keyfile, "valid", "key2", FALSE);
  check_boolean_value (keyfile, "valid", "key3", TRUE);
  check_boolean_value (keyfile, "valid", "key4", FALSE);
  check_boolean_value (keyfile, "valid", "key5", TRUE);
  check_boolean_value (keyfile, "valid", "key6", TRUE);

  xkey_file_get_boolean (keyfile, "invalid", "key1", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);

  xkey_file_get_boolean (keyfile, "invalid", "key2", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);

  xkey_file_get_boolean (keyfile, "invalid", "key3", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);

  xkey_file_get_boolean (keyfile, "invalid", "key4", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);

  xkey_file_set_boolean (keyfile, "valid", "key1", FALSE);
  check_boolean_value (keyfile, "valid", "key1", FALSE);

  xkey_file_free (keyfile);
}

/* check parsing of integer and double values */
static void
test_number (void)
{
  xkey_file_t *keyfile;
  xerror_t *error = NULL;
  xdouble_t dval = 0.0;

  const xchar_t *data =
    "[valid]\n"
    "key1=0\n"
    "key2=1\n"
    "key3=-1\n"
    "key4=2324431\n"
    "key5=-2324431\n"
    "key6=000111\n"
    "key7= 1\n"
    "key8=1 \n"
    "dkey1=000111\n"
    "dkey2=145.45\n"
    "dkey3=-3453.7\n"
    "[invalid]\n"
    "key1=0xffff\n"
    "key2=0.5\n"
    "key3=1e37\n"
    "key4=ten\n"
    "key5=\n"
    "key6=1.0.0\n"
    "key7=2x2\n"
    "key8=abc\n";

  keyfile = load_data (data, 0);

  check_integer_value (keyfile, "valid", "key1", 0);
  check_integer_value (keyfile, "valid", "key2", 1);
  check_integer_value (keyfile, "valid", "key3", -1);
  check_integer_value (keyfile, "valid", "key4", 2324431);
  check_integer_value (keyfile, "valid", "key5", -2324431);
  check_integer_value (keyfile, "valid", "key6", 111);
  check_integer_value (keyfile, "valid", "key7", 1);
  check_integer_value (keyfile, "valid", "key8", 1);
  check_double_value (keyfile, "valid", "dkey1", 111.0);
  check_double_value (keyfile, "valid", "dkey2", 145.45);
  check_double_value (keyfile, "valid", "dkey3", -3453.7);

  xkey_file_get_integer (keyfile, "invalid", "key1", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);

  xkey_file_get_integer (keyfile, "invalid", "key2", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);

  xkey_file_get_integer (keyfile, "invalid", "key3", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);

  xkey_file_get_integer (keyfile, "invalid", "key4", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);

  dval = xkey_file_get_double (keyfile, "invalid", "key5", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);
  g_assert_cmpfloat (dval, ==, 0.0);

  dval = xkey_file_get_double (keyfile, "invalid", "key6", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);
  g_assert_cmpfloat (dval, ==, 0.0);

  dval = xkey_file_get_double (keyfile, "invalid", "key7", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);
  g_assert_cmpfloat (dval, ==, 0.0);

  dval = xkey_file_get_double (keyfile, "invalid", "key8", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);
  g_assert_cmpfloat (dval, ==, 0.0);

  xkey_file_free (keyfile);
}

/* check handling of translated strings */
static void
test_locale_string (void)
{
  xkey_file_t *keyfile;
  xchar_t *old_locale;

  const xchar_t *data =
    "[valid]\n"
    "key1=v1\n"
    "key1[de]=v1-de\n"
    "key1[de_DE]=v1-de_DE\n"
    "key1[de_DE.UTF8]=v1-de_DE.UTF8\n"
    "key1[fr]=v1-fr\n"
    "key1[en] =v1-en\n"
    "key1[sr@Latn]=v1-sr\n";

  keyfile = load_data (data, G_KEY_FILE_KEEP_TRANSLATIONS);

  check_locale_strinxvalue (keyfile, "valid", "key1", "it", "v1");
  check_locale_strinxvalue (keyfile, "valid", "key1", "de", "v1-de");
  check_locale_strinxvalue (keyfile, "valid", "key1", "de_DE", "v1-de_DE");
  check_locale_strinxvalue (keyfile, "valid", "key1", "de_DE.UTF8", "v1-de_DE.UTF8");
  check_locale_strinxvalue (keyfile, "valid", "key1", "fr", "v1-fr");
  check_locale_strinxvalue (keyfile, "valid", "key1", "fr_FR", "v1-fr");
  check_locale_strinxvalue (keyfile, "valid", "key1", "en", "v1-en");
  check_locale_strinxvalue (keyfile, "valid", "key1", "sr@Latn", "v1-sr");

  xkey_file_free (keyfile);

  /* now test that translations are thrown away */

  old_locale = xstrdup (setlocale (LC_ALL, NULL));
  g_setenv ("LANGUAGE", "de", TRUE);
  setlocale (LC_ALL, "");

  keyfile = load_data (data, 0);

  check_locale_strinxvalue (keyfile, "valid", "key1", "it", "v1");
  check_locale_strinxvalue (keyfile, "valid", "key1", "de", "v1-de");
  check_locale_strinxvalue (keyfile, "valid", "key1", "de_DE", "v1-de");
  check_locale_strinxvalue (keyfile, "valid", "key1", "de_DE.UTF8", "v1-de");
  check_locale_strinxvalue (keyfile, "valid", "key1", "fr", "v1");
  check_locale_strinxvalue (keyfile, "valid", "key1", "fr_FR", "v1");
  check_locale_strinxvalue (keyfile, "valid", "key1", "en", "v1");

  xkey_file_free (keyfile);

  setlocale (LC_ALL, old_locale);
  g_free (old_locale);
}

static void
test_locale_string_multiple_loads (void)
{
  xkey_file_t *keyfile = NULL;
  xerror_t *local_error = NULL;
  xchar_t *old_locale = NULL;
  xuint_t i;
  const xchar_t *data =
    "[valid]\n"
    "key1=v1\n"
    "key1[de]=v1-de\n"
    "key1[de_DE]=v1-de_DE\n"
    "key1[de_DE.UTF8]=v1-de_DE.UTF8\n"
    "key1[fr]=v1-fr\n"
    "key1[en] =v1-en\n"
    "key1[sr@Latn]=v1-sr\n";

  g_test_summary ("Check that loading with translations multiple times works");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2361");

  old_locale = xstrdup (setlocale (LC_ALL, NULL));
  g_setenv ("LANGUAGE", "de", TRUE);
  setlocale (LC_ALL, "");

  keyfile = xkey_file_new ();

  for (i = 0; i < 3; i++)
    {
      xkey_file_load_from_data (keyfile, data, -1, G_KEY_FILE_NONE, &local_error);
      g_assert_no_error (local_error);

      check_locale_strinxvalue (keyfile, "valid", "key1", "it", "v1");
      check_locale_strinxvalue (keyfile, "valid", "key1", "de", "v1-de");
      check_locale_strinxvalue (keyfile, "valid", "key1", "de_DE", "v1-de");
    }

  xkey_file_free (keyfile);

  setlocale (LC_ALL, old_locale);
  g_free (old_locale);
}

static void
test_lists (void)
{
  xkey_file_t *keyfile;

  const xchar_t *data =
    "[valid]\n"
    "key1=v1;v2\n"
    "key2=v1;v2;\n"
    "key3=v1,v2\n"
    "key4=v1\\;v2\n"
    "key5=true;false\n"
    "key6=1;0;-1\n"
    "key7= 1 ; 0 ; -1 \n"
    "key8=v1\\,v2\n"
    "key9=0;1.3456;-76532.456\n";

  keyfile = load_data (data, 0);

  check_strinxlist_value (keyfile, "valid", "key1", "v1", "v2", NULL);
  check_strinxlist_value (keyfile, "valid", "key2", "v1", "v2", NULL);
  check_strinxlist_value (keyfile, "valid", "key3", "v1,v2", NULL);
  check_strinxlist_value (keyfile, "valid", "key4", "v1;v2", NULL);
  check_boolean_list_value (keyfile, "valid", "key5", TRUE, FALSE, -100);
  check_integer_list_value (keyfile, "valid", "key6", 1, 0, -1, -100);
  check_double_list_value (keyfile, "valid", "key9", 0.0, 1.3456, -76532.456, -100.0);
  /* maybe these should be valid */
  /* check_integer_list_value (keyfile, "valid", "key7", 1, 0, -1, -100);*/
  /* check_strinxlist_value (keyfile, "valid", "key8", "v1\\,v2", NULL);*/

  xkey_file_free (keyfile);

  /* Now check an alternate separator */

  keyfile = load_data (data, 0);
  xkey_file_set_list_separator (keyfile, ',');

  check_strinxlist_value (keyfile, "valid", "key1", "v1;v2", NULL);
  check_strinxlist_value (keyfile, "valid", "key2", "v1;v2;", NULL);
  check_strinxlist_value (keyfile, "valid", "key3", "v1", "v2", NULL);

  xkey_file_free (keyfile);
}

static void
test_lists_set_get (void)
{
  xkey_file_t *keyfile;
  static const char * const strings[] = { "v1", "v2" };
  static const char * const locale_strings[] = { "v1-l", "v2-l" };
  static int integers[] = { 1, -1, 2 };
  static xdouble_t doubles[] = { 3.14, 2.71 };

  keyfile = xkey_file_new ();
  xkey_file_set_string_list (keyfile, "group0", "key1", strings, G_N_ELEMENTS (strings));
  xkey_file_set_locale_string_list (keyfile, "group0", "key1", "de", locale_strings, G_N_ELEMENTS (locale_strings));
  xkey_file_set_integer_list (keyfile, "group0", "key2", integers, G_N_ELEMENTS (integers));
  xkey_file_set_double_list (keyfile, "group0", "key3", doubles, G_N_ELEMENTS (doubles));

  check_strinxlist_value (keyfile, "group0", "key1", strings[0], strings[1], NULL);
  check_locale_strinxlist_value (keyfile, "group0", "key1", "de", locale_strings[0], locale_strings[1], NULL);
  check_integer_list_value (keyfile, "group0", "key2", integers[0], integers[1], -100);
  check_double_list_value (keyfile, "group0", "key3", doubles[0], doubles[1], -100.0);
  xkey_file_free (keyfile);

  /* and again with a different list separator */
  keyfile = xkey_file_new ();
  xkey_file_set_list_separator (keyfile, ',');
  xkey_file_set_string_list (keyfile, "group0", "key1", strings, G_N_ELEMENTS (strings));
  xkey_file_set_locale_string_list (keyfile, "group0", "key1", "de", locale_strings, G_N_ELEMENTS (locale_strings));
  xkey_file_set_integer_list (keyfile, "group0", "key2", integers, G_N_ELEMENTS (integers));
  xkey_file_set_double_list (keyfile, "group0", "key3", doubles, G_N_ELEMENTS (doubles));

  check_strinxlist_value (keyfile, "group0", "key1", strings[0], strings[1], NULL);
  check_locale_strinxlist_value (keyfile, "group0", "key1", "de", locale_strings[0], locale_strings[1], NULL);
  check_integer_list_value (keyfile, "group0", "key2", integers[0], integers[1], -100);
  check_double_list_value (keyfile, "group0", "key3", doubles[0], doubles[1], -100.0);
  xkey_file_free (keyfile);
}

static void
test_group_remove (void)
{
  xkey_file_t *keyfile;
  xchar_t **names;
  xsize_t len;
  xerror_t *error = NULL;

  const xchar_t *data =
    "[group1]\n"
    "[group2]\n"
    "key1=bla\n"
    "key2=bla\n"
    "[group3]\n"
    "key1=bla\n"
    "key2=bla\n";

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=165887");

  keyfile = load_data (data, 0);

  names = xkey_file_get_groups (keyfile, &len);
  g_assert_nonnull (names);

  check_length ("groups", xstrv_length (names), len, 3);
  check_name ("group name", names[0], "group1", 0);
  check_name ("group name", names[1], "group2", 1);
  check_name ("group name", names[2], "group3", 2);

  xkey_file_remove_group (keyfile, "group1", &error);
  check_no_error (&error);

  xstrfreev (names);

  names = xkey_file_get_groups (keyfile, &len);
  g_assert_nonnull (names);

  check_length ("groups", xstrv_length (names), len, 2);
  check_name ("group name", names[0], "group2", 0);
  check_name ("group name", names[1], "group3", 1);

  xkey_file_remove_group (keyfile, "group2", &error);
  check_no_error (&error);

  xstrfreev (names);

  names = xkey_file_get_groups (keyfile, &len);
  g_assert_nonnull (names);

  check_length ("groups", xstrv_length (names), len, 1);
  check_name ("group name", names[0], "group3", 0);

  xkey_file_remove_group (keyfile, "no such group", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND);

  xstrfreev (names);

  xkey_file_free (keyfile);
}

static void
test_key_remove (void)
{
  xkey_file_t *keyfile;
  xchar_t *value;
  xerror_t *error = NULL;

  const xchar_t *data =
    "[group1]\n"
    "key1=bla\n"
    "key2=bla\n";

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=165980");

  keyfile = load_data (data, 0);

  check_strinxvalue (keyfile, "group1", "key1", "bla");

  xkey_file_remove_key (keyfile, "group1", "key1", &error);
  check_no_error (&error);

  value = xkey_file_get_string (keyfile, "group1", "key1", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND);
  g_free (value);

  xkey_file_remove_key (keyfile, "group1", "key1", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND);

  xkey_file_remove_key (keyfile, "no such group", "key1", &error);
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND);

  xkey_file_free (keyfile);
}


static void
test_groups (void)
{
  xkey_file_t *keyfile;

  const xchar_t *data =
    "[1]\n"
    "key1=123\n"
    "[2]\n"
    "key2=123\n";

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=316309");

  keyfile = load_data (data, 0);

  check_strinxvalue (keyfile, "1", "key1", "123");
  check_strinxvalue (keyfile, "2", "key2", "123");

  xkey_file_free (keyfile);
}

static void
test_group_names (void)
{
  xkey_file_t *keyfile;
  xerror_t *error = NULL;
  const xchar_t *data;
  xchar_t *value;

  /* [ in group name */
  data = "[a[b]\n"
         "key1=123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  xkey_file_free (keyfile);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_PARSE);

  /* ] in group name */
  data = "[a]b]\n"
         "key1=123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  xkey_file_free (keyfile);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_PARSE);

  /* control char in group name */
  data = "[a\tb]\n"
         "key1=123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  xkey_file_free (keyfile);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_PARSE);

  /* empty group name */
  data = "[]\n"
         "key1=123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  xkey_file_free (keyfile);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_PARSE);

  /* Unicode in group name */
  data = "[\xc2\xbd]\n"
         "key1=123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  xkey_file_free (keyfile);
  check_no_error (&error);

  keyfile = xkey_file_new ();
  /*xkey_file_set_string (keyfile, "a[b", "key1", "123");*/
  value = xkey_file_get_string (keyfile, "a[b", "key1", &error);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_GROUP_NOT_FOUND);
  g_assert_null (value);
  xkey_file_free (keyfile);

  keyfile = xkey_file_new ();
  /*xkey_file_set_string (keyfile, "a]b", "key1", "123");*/
  value = xkey_file_get_string (keyfile, "a]b", "key1", &error);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_GROUP_NOT_FOUND);
  g_assert_null (value);
  xkey_file_free (keyfile);

  keyfile = xkey_file_new ();
  /*xkey_file_set_string (keyfile, "a\tb", "key1", "123");*/
  value = xkey_file_get_string (keyfile, "a\tb", "key1", &error);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_GROUP_NOT_FOUND);
  g_assert_null (value);
  xkey_file_free (keyfile);

  keyfile = xkey_file_new ();
  xkey_file_set_string (keyfile, "\xc2\xbd", "key1", "123");
  check_strinxvalue (keyfile, "\xc2\xbd", "key1", "123");
  xkey_file_free (keyfile);
}

static void
test_key_names (void)
{
  xkey_file_t *keyfile;
  xerror_t *error = NULL;
  const xchar_t *data;
  xchar_t *value;

  /* [ in key name */
  data = "[a]\n"
         "key[=123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  xkey_file_free (keyfile);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_PARSE);

  /* empty key name */
  data = "[a]\n"
         " =123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  xkey_file_free (keyfile);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_PARSE);

  /* empty key name */
  data = "[a]\n"
         " [de] =123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  xkey_file_free (keyfile);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_PARSE);

  /* bad locale suffix */
  data = "[a]\n"
         "foo[@#!&%]=123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  xkey_file_free (keyfile);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_PARSE);

  /* initial space */
  data = "[a]\n"
         " foo=123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  check_no_error (&error);
  check_strinxvalue (keyfile, "a", "foo", "123");
  xkey_file_free (keyfile);

  /* final space */
  data = "[a]\n"
         "foo =123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  check_no_error (&error);
  check_strinxvalue (keyfile, "a", "foo", "123");
  xkey_file_free (keyfile);

  /* inner space */
  data = "[a]\n"
         "foo bar=123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  check_no_error (&error);
  check_strinxvalue (keyfile, "a", "foo bar", "123");
  xkey_file_free (keyfile);

  /* inner space */
  data = "[a]\n"
         "foo [de] =123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_PARSE);
  xkey_file_free (keyfile);

  /* control char in key name */
  data = "[a]\n"
         "key\tfoo=123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  xkey_file_free (keyfile);
  check_no_error (&error);

  /* Unicode in key name */
  data = "[a]\n"
         "\xc2\xbd=123\n";
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);
  xkey_file_free (keyfile);
  check_no_error (&error);

  keyfile = xkey_file_new ();
  xkey_file_set_string (keyfile, "a", "x", "123");
  /*xkey_file_set_string (keyfile, "a", "key=", "123");*/
  value = xkey_file_get_string (keyfile, "a", "key=", &error);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_KEY_NOT_FOUND);
  g_assert_null (value);
  xkey_file_free (keyfile);

  keyfile = xkey_file_new ();
  xkey_file_set_string (keyfile, "a", "x", "123");
  /*xkey_file_set_string (keyfile, "a", "key[", "123");*/
  value = xkey_file_get_string (keyfile, "a", "key[", &error);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_KEY_NOT_FOUND);
  g_assert_null (value);
  xkey_file_free (keyfile);

  keyfile = xkey_file_new ();
  xkey_file_set_string (keyfile, "a", "x", "123");
  xkey_file_set_string (keyfile, "a", "key\tfoo", "123");
  value = xkey_file_get_string (keyfile, "a", "key\tfoo", &error);
  check_no_error (&error);
  g_free (value);
  xkey_file_free (keyfile);

  keyfile = xkey_file_new ();
  xkey_file_set_string (keyfile, "a", "x", "123");
  /*xkey_file_set_string (keyfile, "a", " key", "123");*/
  value = xkey_file_get_string (keyfile, "a", " key", &error);
  check_error (&error,
               G_KEY_FILE_ERROR,
               G_KEY_FILE_ERROR_KEY_NOT_FOUND);
  g_assert_null (value);
  xkey_file_free (keyfile);

  keyfile = xkey_file_new ();
  xkey_file_set_string (keyfile, "a", "x", "123");

  /* Unicode key */
  xkey_file_set_string (keyfile, "a", "\xc2\xbd", "123");
  check_strinxvalue (keyfile, "a", "\xc2\xbd", "123");

  /* Keys with / + . (as used by the gnome-vfs mime cache) */
  xkey_file_set_string (keyfile, "a", "foo/bar", "/");
  check_strinxvalue (keyfile, "a", "foo/bar", "/");
  xkey_file_set_string (keyfile, "a", "foo+bar", "+");
  check_strinxvalue (keyfile, "a", "foo+bar", "+");
  xkey_file_set_string (keyfile, "a", "foo.bar", ".");
  check_strinxvalue (keyfile, "a", "foo.bar", ".");

  xkey_file_free (keyfile);
}

static void
test_duplicate_keys (void)
{
  xkey_file_t *keyfile;
  const xchar_t *data =
    "[1]\n"
    "key1=123\n"
    "key1=345\n";

  keyfile = load_data (data, 0);
  check_strinxvalue (keyfile, "1", "key1", "345");

  xkey_file_free (keyfile);
}

static void
test_duplicate_groups (void)
{
  xkey_file_t *keyfile;
  const xchar_t *data =
    "[Desktop Entry]\n"
    "key1=123\n"
    "[Desktop Entry]\n"
    "key2=123\n";

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=157877");

  keyfile = load_data (data, 0);
  check_strinxvalue (keyfile, "Desktop Entry", "key1", "123");
  check_strinxvalue (keyfile, "Desktop Entry", "key2", "123");

  xkey_file_free (keyfile);
}

static void
test_duplicate_groups2 (void)
{
  xkey_file_t *keyfile;
  const xchar_t *data =
    "[A]\n"
    "foo=bar\n"
    "[B]\n"
    "foo=baz\n"
    "[A]\n"
    "foo=bang\n";

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=385910");

  keyfile = load_data (data, 0);
  check_strinxvalue (keyfile, "A", "foo", "bang");
  check_strinxvalue (keyfile, "B", "foo", "baz");

  xkey_file_free (keyfile);
}

static void
test_reload_idempotency (void)
{
  static const xchar_t *original_data=""
    "# Top comment\n"
    "\n"
    "# First comment\n"
    "[first]\n"
    "key=value\n"
    "# A random comment in the first group\n"
    "anotherkey=anothervalue\n"
    "# Second comment - one line\n"
    "[second]\n"
    "# Third comment - two lines\n"
    "# Third comment - two lines\n"
    "[third]\n"
    "blank_line=1\n"
    "\n"
    "blank_lines=2\n"
    "\n\n"
    "[fourth]\n"
    "[fifth]\n";
  xkey_file_t *keyfile;
  xerror_t *error = NULL;
  xchar_t *data1, *data2;
  xsize_t len1, len2;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=420686");

  /* check that we only insert a single new line between groups */
  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile,
                             original_data, strlen(original_data),
                             G_KEY_FILE_KEEP_COMMENTS,
                             &error);
  check_no_error (&error);

  data1 = xkey_file_to_data (keyfile, &len1, &error);
  g_assert_nonnull (data1);
  xkey_file_free (keyfile);

  keyfile = xkey_file_new ();
  xkey_file_load_from_data (keyfile,
                             data1, len1,
                             G_KEY_FILE_KEEP_COMMENTS,
                             &error);
  check_no_error (&error);

  data2 = xkey_file_to_data (keyfile, &len2, &error);
  g_assert_nonnull (data2);
  xkey_file_free (keyfile);

  g_assert_cmpstr (data1, ==, data2);

  g_free (data2);
  g_free (data1);
}

static const char int64_data[] =
"[bees]\n"
"a=1\n"
"b=2\n"
"c=123456789123456789\n"
"d=-123456789123456789\n";

static void
test_int64 (void)
{
  xkey_file_t *file;
  xboolean_t ok;
  xuint64_t c;
  gint64 d;
  xchar_t *value;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=614864");

  file = xkey_file_new ();

  ok = xkey_file_load_from_data (file, int64_data, strlen (int64_data),
      0, NULL);
  g_assert_true (ok);

  c = xkey_file_get_uint64 (file, "bees", "c", NULL);
  g_assert_cmpuint (c, ==, G_GUINT64_CONSTANT (123456789123456789));

  d = xkey_file_get_int64 (file, "bees", "d", NULL);
  g_assert_cmpint (d, ==, G_GINT64_CONSTANT (-123456789123456789));

  xkey_file_set_uint64 (file, "bees", "c",
      G_GUINT64_CONSTANT (987654321987654321));
  value = xkey_file_get_value (file, "bees", "c", NULL);
  g_assert_cmpstr (value, ==, "987654321987654321");
  g_free (value);

  xkey_file_set_int64 (file, "bees", "d",
      G_GINT64_CONSTANT (-987654321987654321));
  value = xkey_file_get_value (file, "bees", "d", NULL);
  g_assert_cmpstr (value, ==, "-987654321987654321");
  g_free (value);

  xkey_file_free (file);
}

static void
test_load (void)
{
  xkey_file_t *file;
  xerror_t *error;
  xboolean_t bools[2] = { TRUE, FALSE };
  xboolean_t loaded;

  file = xkey_file_new ();
  error = NULL;
#ifdef G_OS_UNIX
  /* Uses the value of $XDG_DATA_HOME we set in main() */
  loaded = xkey_file_load_from_data_dirs (file, "keyfiletest.ini", NULL, 0, &error);
#else
  loaded = xkey_file_load_from_file (file, g_test_get_filename (G_TEST_DIST, "keyfiletest.ini", NULL), 0, &error);
#endif
  g_assert_no_error (error);
  g_assert_true (loaded);

  xkey_file_set_locale_string (file, "test", "key4", "de", "Vierter Schlüssel");
  xkey_file_set_boolean_list (file, "test", "key5", bools, 2);
  xkey_file_set_integer (file, "test", "key6", 22);
  xkey_file_set_double (file, "test", "key7", 2.5);
  xkey_file_set_comment (file, "test", "key7", "some float", NULL);
  xkey_file_set_comment (file, "test", NULL, "the test group", NULL);
  xkey_file_set_comment (file, NULL, NULL, "top comment", NULL);

  xkey_file_free (file);

  file = xkey_file_new ();
  error = NULL;
  g_assert_false (xkey_file_load_from_data_dirs (file, "keyfile-test.ini", NULL, 0, &error));
  g_assert_error (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_NOT_FOUND);
  xerror_free (error);
  xkey_file_free (file);
}

static void
test_save (void)
{
  xkey_file_t *kf;
  xkey_file_t *kf2;
  static const char data[] =
    "[bees]\n"
    "a=1\n"
    "b=2\n"
    "c=123456789123456789\n"
    "d=-123456789123456789\n";
  xboolean_t ok;
  xchar_t *file;
  xuint64_t c;
  xerror_t *error = NULL;
  int fd;

  kf = xkey_file_new ();
  ok = xkey_file_load_from_data (kf, data, strlen (data), 0, NULL);
  g_assert_true (ok);

  file = xstrdup ("key_file_XXXXXX");
  fd = g_mkstemp (file);
  g_assert_cmpint (fd, !=, -1);
  ok = g_close (fd, &error);
  g_assert_true (ok);
  g_assert_no_error (error);
  ok = xkey_file_save_to_file (kf, file, &error);
  g_assert_true (ok);
  g_assert_no_error (error);

  kf2 = xkey_file_new ();
  ok = xkey_file_load_from_file (kf2, file, 0, &error);
  g_assert_true (ok);
  g_assert_no_error (error);

  c = xkey_file_get_uint64 (kf2, "bees", "c", NULL);
  g_assert_cmpuint (c, ==, G_GUINT64_CONSTANT (123456789123456789));

  remove (file);
  g_free (file);
  xkey_file_free (kf);
  xkey_file_free (kf2);
}

static void
test_load_fail (void)
{
  xkey_file_t *file;
  xerror_t *error;

  file = xkey_file_new ();
  error = NULL;
  g_assert_false (xkey_file_load_from_file (file, g_test_get_filename (G_TEST_DIST, "keyfile.c", NULL), 0, &error));
  g_assert_error (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_PARSE);
  g_clear_error (&error);
  g_assert_false (xkey_file_load_from_file (file, "/nosuchfile", 0, &error));
  g_assert_error (error, XFILE_ERROR, XFILE_ERROR_NOENT);
  g_clear_error (&error);

  xkey_file_free (file);
}

static void
test_non_utf8 (void)
{
  xkey_file_t *file;
  static const char data[] =
"[group]\n"
"a=\230\230\230\n"
"b=a;b;\230\230\230;\n"
"c=a\\\n";
  xboolean_t ok;
  xerror_t *error;
  xchar_t *s;
  xchar_t **l;

  file = xkey_file_new ();

  ok = xkey_file_load_from_data (file, data, strlen (data), 0, NULL);
  g_assert_true (ok);

  error = NULL;
  s = xkey_file_get_string (file, "group", "a", &error);
  g_assert_error (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_UNKNOWN_ENCODING);
  g_assert_null (s);

  g_clear_error (&error);
  l = xkey_file_get_string_list (file, "group", "b", NULL, &error);
  g_assert_error (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_UNKNOWN_ENCODING);
  g_assert_null (l);

  g_clear_error (&error);
  l = xkey_file_get_string_list (file, "group", "c", NULL, &error);
  g_assert_error (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE);
  g_assert_null (l);

  g_clear_error (&error);

  xkey_file_free (file);
}

static void
test_page_boundary (void)
{
  xkey_file_t *file;
  xerror_t *error;
  xint_t i;

#define GROUP "main_section"
#define KEY_PREFIX "fill_abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvw_"
#define FIRST_KEY 10
#define LAST_KEY 99
#define VALUE 92

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=640695");

  file = xkey_file_new ();

  error = NULL;
  xkey_file_load_from_file (file, g_test_get_filename (G_TEST_DIST, "pages.ini", NULL), G_KEY_FILE_NONE, &error);
  g_assert_no_error (error);

  for (i = FIRST_KEY; i <= LAST_KEY; i++)
    {
      xchar_t *key;
      xint_t val;

      key = xstrdup_printf (KEY_PREFIX "%d", i);
      val = xkey_file_get_integer (file, GROUP, key, &error);
      g_free (key);
      g_assert_no_error (error);
      g_assert_cmpint (val, ==, VALUE);
    }

  xkey_file_free (file);
}

static void
test_ref (void)
{
  xkey_file_t *file;
  static const char data[] =
"[group]\n"
"a=1\n";
  xboolean_t ok;

  file = xkey_file_new ();

  ok = xkey_file_load_from_data (file, data, strlen (data), 0, NULL);
  g_assert_true (ok);
  g_assert_true (xkey_file_has_key (file, "group", "a", NULL));
  xkey_file_ref (file);
  xkey_file_free (file);
  xkey_file_unref (file);
}

/* https://bugzilla.gnome.org/show_bug.cgi?id=634232 */
static void
test_replace_value (void)
{
  xkey_file_t *keyfile;

  keyfile = xkey_file_new();
  xkey_file_set_value(keyfile, "grupo1", "chave1", "1234567890");
  xkey_file_set_value(keyfile, "grupo1", "chave1", "123123423423423432432423423");
  xkey_file_remove_group(keyfile, "grupo1", NULL);
  g_free (xkey_file_to_data (keyfile, NULL, NULL));
  xkey_file_unref (keyfile);
}

static void
test_list_separator (void)
{
  xkey_file_t *keyfile;
  xerror_t *error = NULL;

  const xchar_t *data =
    "[test]\n"
    "key1=v1,v2\n";

  keyfile = xkey_file_new ();
  xkey_file_set_list_separator (keyfile, ',');
  xkey_file_load_from_data (keyfile, data, -1, 0, &error);

  check_strinxlist_value (keyfile, "test", "key1", "v1", "v2", NULL);
  xkey_file_unref (keyfile);
}

static void
test_empty_string (void)
{
  xerror_t *error = NULL;
  xkey_file_t *kf;

  kf = xkey_file_new ();

  xkey_file_load_from_data (kf, "", 0, 0, &error);
  g_assert_no_error (error);

  xkey_file_load_from_data (kf, "", -1, 0, &error);
  g_assert_no_error (error);

  /* NULL is a fine pointer to use if length is zero */
  xkey_file_load_from_data (kf, NULL, 0, 0, &error);
  g_assert_no_error (error);

  /* should not attempt to access non-NULL pointer if length is zero */
  xkey_file_load_from_data (kf, GINT_TO_POINTER (1), 0, 0, &error);
  g_assert_no_error (error);

  xkey_file_unref (kf);
}

static void
test_limbo (void)
{
  xkey_file_t *file;
  static const char data[] =
"a=b\n"
"[group]\n"
"b=c\n";
  xboolean_t ok;
  xerror_t *error;

  file = xkey_file_new ();

  error = NULL;
  ok = xkey_file_load_from_data (file, data, strlen (data), 0, &error);
  g_assert_false (ok);
  g_assert_error (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND);
  g_clear_error (&error);
  xkey_file_free (file);
}

static void
test_utf8 (void)
{
  const xchar_t *invalid_encoding_names[] =
    {
      "non-UTF-8",
      "UTF",
      "UTF-9",
    };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (invalid_encoding_names); i++)
    {
      xkey_file_t *file = NULL;
      xchar_t *data = NULL;
      xboolean_t ok;
      xerror_t *error = NULL;

      g_test_message ("Testing invalid encoding ‘%s’", invalid_encoding_names[i]);

      file = xkey_file_new ();
      data = xstrdup_printf ("[group]\n"
                              "Encoding=%s\n", invalid_encoding_names[i]);
      ok = xkey_file_load_from_data (file, data, strlen (data), 0, &error);
      g_assert_false (ok);
      g_assert_error (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_UNKNOWN_ENCODING);
      g_clear_error (&error);
      xkey_file_free (file);
      g_free (data);
    }
}

static void
test_roundtrip (void)
{
  xkey_file_t *kf;
  const xchar_t orig[] =
    "[Group1]\n"
    "key1=value1\n"
    "\n"
    "[Group2]\n"
    "key1=value1\n";
  xsize_t len;
  xchar_t *data;

  kf = load_data (orig, G_KEY_FILE_KEEP_COMMENTS);
  xkey_file_set_integer (kf, "Group1", "key2", 0);
  xkey_file_remove_key (kf, "Group1", "key2", NULL);

  data = xkey_file_to_data (kf, &len, NULL);
  g_assert_cmpstr (data, ==, orig);

  g_free (data);
  xkey_file_free (kf);
}

static void
test_bytes (void)
{
  const xchar_t data[] =
    "[Group1]\n"
    "key1=value1\n"
    "\n"
    "[Group2]\n"
    "key2=value2\n";

  xkey_file_t *kf = xkey_file_new ();
  xbytes_t *bytes = xbytes_new (data, strlen (data));
  xerror_t *error = NULL;

  xchar_t **names;
  xsize_t len;

  xkey_file_load_from_bytes (kf, bytes, 0, &error);

  g_assert_no_error (error);

  names = xkey_file_get_groups (kf, &len);
  g_assert_nonnull (names);

  check_length ("groups", xstrv_length (names), len, 2);
  check_name ("group name", names[0], "Group1", 0);
  check_name ("group name", names[1], "Group2", 1);

  check_strinxvalue (kf, "Group1", "key1", "value1");
  check_strinxvalue (kf, "Group2", "key2", "value2");

  xstrfreev (names);
  xbytes_unref (bytes);
  xkey_file_free (kf);
}

static void
test_get_locale (void)
{
  xkey_file_t *kf;

  kf = xkey_file_new ();
  xkey_file_load_from_data (kf,
                             "[Group]\n"
                             "x[fr_CA]=a\n"
                             "x[fr]=b\n"
                             "x=c\n",
                             -1, G_KEY_FILE_KEEP_TRANSLATIONS,
                             NULL);

  check_locale_strinxvalue (kf, "Group", "x", "fr_CA", "a");
  check_string_locale_value (kf, "Group", "x", "fr_CA", "fr_CA");

  check_locale_strinxvalue (kf, "Group", "x", "fr_CH", "b");
  check_string_locale_value (kf, "Group", "x", "fr_CH", "fr");

  check_locale_strinxvalue (kf, "Group", "x", "eo", "c");
  check_string_locale_value (kf, "Group", "x", "eo", NULL);

  xkey_file_free (kf);
}

static void
test_free_when_not_last_ref (void)
{
  xkey_file_t *kf;
  xerror_t *error = NULL;
  const xchar_t *data =
    "[Group]\n"
    "Key=Value\n";

  kf = load_data (data, G_KEY_FILE_NONE);
  /* Add a second ref */
  xkey_file_ref (kf);

  /* Quick coherence check */
  g_assert_true (xkey_file_has_group (kf, "Group"));
  g_assert_true (xkey_file_has_key (kf, "Group", "Key", &error));
  g_assert_no_error (error);

  /* Should clear all keys and groups, and remove one ref */
  xkey_file_free (kf);

  /* kf should still work */
  g_assert_false (xkey_file_has_group (kf, "Group"));
  g_assert_false (xkey_file_has_key (kf, "Group", "Key", &error));
  check_error (&error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND);
  g_clear_error (&error);

  xkey_file_load_from_data (kf, data, -1, G_KEY_FILE_NONE, &error);
  g_assert_no_error (error);

  g_assert_true (xkey_file_has_group (kf, "Group"));
  g_assert_true (xkey_file_has_key (kf, "Group", "Key", &error));

  xkey_file_unref (kf);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#ifdef G_OS_UNIX
  g_setenv ("XDG_DATA_HOME", g_test_get_dir (G_TEST_DIST), TRUE);
#endif

  g_test_add_func ("/keyfile/line-ends", test_line_ends);
  g_test_add_func ("/keyfile/whitespace", test_whitespace);
  g_test_add_func ("/keyfile/comments", test_comments);
  g_test_add_func ("/keyfile/listing", test_listing);
  g_test_add_func ("/keyfile/string", test_string);
  g_test_add_func ("/keyfile/boolean", test_boolean);
  g_test_add_func ("/keyfile/number", test_number);
  g_test_add_func ("/keyfile/locale-string", test_locale_string);
  g_test_add_func ("/keyfile/locale-string/multiple-loads", test_locale_string_multiple_loads);
  g_test_add_func ("/keyfile/lists", test_lists);
  g_test_add_func ("/keyfile/lists-set-get", test_lists_set_get);
  g_test_add_func ("/keyfile/group-remove", test_group_remove);
  g_test_add_func ("/keyfile/key-remove", test_key_remove);
  g_test_add_func ("/keyfile/groups", test_groups);
  g_test_add_func ("/keyfile/duplicate-keys", test_duplicate_keys);
  g_test_add_func ("/keyfile/duplicate-groups", test_duplicate_groups);
  g_test_add_func ("/keyfile/duplicate-groups2", test_duplicate_groups2);
  g_test_add_func ("/keyfile/group-names", test_group_names);
  g_test_add_func ("/keyfile/key-names", test_key_names);
  g_test_add_func ("/keyfile/reload", test_reload_idempotency);
  g_test_add_func ("/keyfile/int64", test_int64);
  g_test_add_func ("/keyfile/load", test_load);
  g_test_add_func ("/keyfile/save", test_save);
  g_test_add_func ("/keyfile/load-fail", test_load_fail);
  g_test_add_func ("/keyfile/non-utf8", test_non_utf8);
  g_test_add_func ("/keyfile/page-boundary", test_page_boundary);
  g_test_add_func ("/keyfile/ref", test_ref);
  g_test_add_func ("/keyfile/replace-value", test_replace_value);
  g_test_add_func ("/keyfile/list-separator", test_list_separator);
  g_test_add_func ("/keyfile/empty-string", test_empty_string);
  g_test_add_func ("/keyfile/limbo", test_limbo);
  g_test_add_func ("/keyfile/utf8", test_utf8);
  g_test_add_func ("/keyfile/roundtrip", test_roundtrip);
  g_test_add_func ("/keyfile/bytes", test_bytes);
  g_test_add_func ("/keyfile/get-locale", test_get_locale);
  g_test_add_func ("/keyfile/free-when-not-last-ref", test_free_when_not_last_ref);

  return g_test_run ();
}
