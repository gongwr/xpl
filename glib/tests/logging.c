#include <stdlib.h>
#include <string.h>
#define G_LOG_USE_STRUCTURED 1
#include <glib.h>

/* Test g_warn macros */
static void
test_warnings (void)
{
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*test_warnings*should not be reached*");
  g_warn_if_reached ();
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*test_warnings*runtime check failed*");
  g_warn_if_fail (FALSE);
  g_test_assert_expected_messages ();
}

static xuint_t log_count = 0;

static void
log_handler (const xchar_t    *log_domain,
             GLogLevelFlags  log_level,
             const xchar_t    *message,
             xpointer_t        user_data)
{
  g_assert_cmpstr (log_domain, ==, "bu");
  g_assert_cmpint (log_level, ==, G_LOG_LEVEL_INFO);

  log_count++;
}

/* test that custom log handlers only get called for
 * their domain and level
 */
static void
test_set_handler (void)
{
  xuint_t id;

  id = g_log_set_handler ("bu", G_LOG_LEVEL_INFO, log_handler, NULL);

  g_log ("bu", G_LOG_LEVEL_DEBUG, "message");
  g_log ("ba", G_LOG_LEVEL_DEBUG, "message");
  g_log ("bu", G_LOG_LEVEL_INFO, "message");
  g_log ("ba", G_LOG_LEVEL_INFO, "message");

  g_assert_cmpint (log_count, ==, 1);

  g_log_remove_handler ("bu", id);
}

static void
test_default_handler_error (void)
{
  g_log_set_default_handler (g_log_default_handler, NULL);
  xerror ("message1");
  exit (0);
}

static void
test_default_handler_critical (void)
{
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_critical ("message2");
  exit (0);
}

static void
test_default_handler_warning (void)
{
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_warning ("message3");
  exit (0);
}

static void
test_default_handler_message (void)
{
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_message ("message4");
  exit (0);
}

static void
test_default_handler_info (void)
{
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "message5");
  exit (0);
}

static void
test_default_handler_bar_info (void)
{
  g_log_set_default_handler (g_log_default_handler, NULL);

  g_setenv ("G_MESSAGES_DEBUG", "foo bar baz", TRUE);

  g_log ("bar", G_LOG_LEVEL_INFO, "message5");
  exit (0);
}

static void
test_default_handler_baz_debug (void)
{
  g_log_set_default_handler (g_log_default_handler, NULL);

  g_setenv ("G_MESSAGES_DEBUG", "foo bar baz", TRUE);

  g_log ("baz", G_LOG_LEVEL_DEBUG, "message6");
  exit (0);
}

static void
test_default_handler_debug (void)
{
  g_log_set_default_handler (g_log_default_handler, NULL);

  g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);

  g_log ("foo", G_LOG_LEVEL_DEBUG, "6");
  g_log ("bar", G_LOG_LEVEL_DEBUG, "6");
  g_log ("baz", G_LOG_LEVEL_DEBUG, "6");

  exit (0);
}

static void
test_default_handler_debug_stderr (void)
{
  g_log_writer_default_set_use_stderr (TRUE);
  g_log_set_default_handler (g_log_default_handler, NULL);

  g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);

  g_log ("foo", G_LOG_LEVEL_DEBUG, "6");
  g_log ("bar", G_LOG_LEVEL_DEBUG, "6");
  g_log ("baz", G_LOG_LEVEL_DEBUG, "6");

  exit (0);
}

static void
test_default_handler_would_drop (void)
{
  g_unsetenv ("G_MESSAGES_DEBUG");

  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_ERROR, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_CRITICAL, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_WARNING, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_MESSAGE, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_INFO, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_false (g_log_writer_default_would_drop (1<<G_LOG_LEVEL_USER_SHIFT, "foo"));

  g_setenv ("G_MESSAGES_DEBUG", "bar baz", TRUE);

  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_ERROR, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_CRITICAL, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_WARNING, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_MESSAGE, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_INFO, "foo"));
  g_assert_true (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_false (g_log_writer_default_would_drop (1<<G_LOG_LEVEL_USER_SHIFT, "foo"));

  g_setenv ("G_MESSAGES_DEBUG", "foo bar", TRUE);

  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_ERROR, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_CRITICAL, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_WARNING, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_MESSAGE, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_INFO, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_false (g_log_writer_default_would_drop (1<<G_LOG_LEVEL_USER_SHIFT, "foo"));

  g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);

  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_ERROR, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_CRITICAL, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_WARNING, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_MESSAGE, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_INFO, "foo"));
  g_assert_false (g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, "foo"));
  g_assert_false (g_log_writer_default_would_drop (1<<G_LOG_LEVEL_USER_SHIFT, "foo"));

  exit (0);
}

static void
test_default_handler_0x400 (void)
{
  g_log_set_default_handler (g_log_default_handler, NULL);
  g_log (G_LOG_DOMAIN, 1<<10, "message7");
  exit (0);
}

static void
test_default_handler (void)
{
  g_test_trap_subprocess ("/logging/default-handler/subprocess/error", 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*ERROR*message1*");

  g_test_trap_subprocess ("/logging/default-handler/subprocess/critical", 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*message2*");

  g_test_trap_subprocess ("/logging/default-handler/subprocess/warning", 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*WARNING*message3*");

  g_test_trap_subprocess ("/logging/default-handler/subprocess/message", 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("*Message*message4*");

  g_test_trap_subprocess ("/logging/default-handler/subprocess/info", 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout_unmatched ("*INFO*message5*");

  g_test_trap_subprocess ("/logging/default-handler/subprocess/bar-info", 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*INFO*message5*");

  g_test_trap_subprocess ("/logging/default-handler/subprocess/baz-debug", 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*DEBUG*message6*");

  g_test_trap_subprocess ("/logging/default-handler/subprocess/debug", 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*DEBUG*6*6*6*");

  g_test_trap_subprocess ("/logging/default-handler/subprocess/debug-stderr", 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout_unmatched ("DEBUG");
  g_test_trap_assert_stderr ("*DEBUG*6*6*6*");

  g_test_trap_subprocess ("/logging/default-handler/subprocess/0x400", 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*LOG-0x400*message7*");

  g_test_trap_subprocess ("/logging/default-handler/subprocess/would-drop", 0, 0);
  g_test_trap_assert_passed ();
}

static void
test_fatal_log_mask (void)
{
  if (g_test_subprocess ())
    {
      g_log_set_fatal_mask ("bu", G_LOG_LEVEL_INFO);
      g_log ("bu", G_LOG_LEVEL_INFO, "fatal");
      return;
    }
  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  /* G_LOG_LEVEL_INFO isn't printed by default */
  g_test_trap_assert_stdout_unmatched ("*fatal*");
}

static xint_t my_print_count = 0;
static void
my_print_handler (const xchar_t *text)
{
  my_print_count++;
}

static void
test_print_handler (void)
{
  GPrintFunc old_print_handler;

  old_print_handler = g_set_print_handler (my_print_handler);
  g_assert (old_print_handler == NULL);

  my_print_count = 0;
  g_print ("bu ba");
  g_assert_cmpint (my_print_count, ==, 1);

  g_set_print_handler (NULL);
}

static void
test_printerr_handler (void)
{
  GPrintFunc old_printerr_handler;

  old_printerr_handler = g_set_printerr_handler (my_print_handler);
  g_assert (old_printerr_handler == NULL);

  my_print_count = 0;
  g_printerr ("bu ba");
  g_assert_cmpint (my_print_count, ==, 1);

  g_set_printerr_handler (NULL);
}

static char *fail_str = "foo";
static char *loxstr = "bar";

static xboolean_t
good_failure_handler (const xchar_t    *log_domain,
                      GLogLevelFlags  log_level,
                      const xchar_t    *msg,
                      xpointer_t        user_data)
{
  g_test_message ("The Good Fail Message Handler\n");
  g_assert ((char *)user_data != loxstr);
  g_assert ((char *)user_data == fail_str);

  return FALSE;
}

static xboolean_t
bad_failure_handler (const xchar_t    *log_domain,
                     GLogLevelFlags  log_level,
                     const xchar_t    *msg,
                     xpointer_t        user_data)
{
  g_test_message ("The Bad Fail Message Handler\n");
  g_assert ((char *)user_data == loxstr);
  g_assert ((char *)user_data != fail_str);

  return FALSE;
}

static void
test_handler (const xchar_t    *log_domain,
              GLogLevelFlags  log_level,
              const xchar_t    *msg,
              xpointer_t        user_data)
{
  g_test_message ("The Log Message Handler\n");
  g_assert ((char *)user_data != fail_str);
  g_assert ((char *)user_data == loxstr);
}

static void
bug653052 (void)
{
  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=653052");

  g_test_log_set_fatal_handler (good_failure_handler, fail_str);
  g_log_set_default_handler (test_handler, loxstr);

  g_return_if_fail (0);

  g_test_log_set_fatal_handler (bad_failure_handler, fail_str);
  g_log_set_default_handler (test_handler, loxstr);

  g_return_if_fail (0);
}

static void
test_gibberish (void)
{
  if (g_test_subprocess ())
    {
      g_warning ("bla bla \236\237\190");
      return;
    }
  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*bla bla \\x9e\\x9f\\u000190*");
}

static GLogWriterOutput
null_log_writer (GLogLevelFlags   log_level,
                 const GLogField *fields,
                 xsize_t            n_fields,
                 xpointer_t         user_data)
{
  log_count++;
  return G_LOG_WRITER_HANDLED;
}

typedef struct {
  const GLogField *fields;
  xsize_t n_fields;
} ExpectedMessage;

static xboolean_t
compare_field (const GLogField *f1, const GLogField *f2)
{
  if (strcmp (f1->key, f2->key) != 0)
    return FALSE;
  if (f1->length != f2->length)
    return FALSE;

  if (f1->length == -1)
    return strcmp (f1->value, f2->value) == 0;
  else
    return memcmp (f1->value, f2->value, f1->length) == 0;
}

static xboolean_t
compare_fields (const GLogField *f1, xsize_t n1, const GLogField *f2, xsize_t n2)
{
  xsize_t i, j;

  for (i = 0; i < n1; i++)
    {
      for (j = 0; j < n2; j++)
        {
          if (compare_field (&f1[i], &f2[j]))
            break;
        }
      if (j == n2)
        return FALSE;
    }

  return TRUE;
}

static xslist_t *expected_messages = NULL;
static const guchar binary_field[] = {1, 2, 3, 4, 5};


static GLogWriterOutput
expect_log_writer (GLogLevelFlags   log_level,
                   const GLogField *fields,
                   xsize_t            n_fields,
                   xpointer_t         user_data)
{
  ExpectedMessage *expected = expected_messages->data;

  if (compare_fields (fields, n_fields, expected->fields, expected->n_fields))
    {
      expected_messages = xslist_delete_link (expected_messages, expected_messages);
    }
  else if ((log_level & G_LOG_LEVEL_DEBUG) != G_LOG_LEVEL_DEBUG)
    {
      char *str;

      str = g_log_writer_format_fields (log_level, fields, n_fields, FALSE);
      g_test_fail_printf ("Unexpected message: %s", str);
      g_free (str);
    }

  return G_LOG_WRITER_HANDLED;
}

static void
test_structured_logging_no_state (void)
{
  xpointer_t some_pointer = GUINT_TO_POINTER (0x100);
  xuint_t some_integer = 123;

  log_count = 0;
  g_log_set_writer_func (null_log_writer, NULL, NULL);

  g_loxstructured ("some-domain", G_LOG_LEVEL_MESSAGE,
                    "MESSAGE_ID", "06d4df59e6c24647bfe69d2c27ef0b4e",
                    "MY_APPLICATION_CUSTOM_FIELD", "some debug string",
                    "MESSAGE", "This is a debug message about pointer %p and integer %u.",
                    some_pointer, some_integer);

  g_assert_cmpint (log_count, ==, 1);
}

static void
test_structured_logging_some_state (void)
{
  xpointer_t state_object = NULL;  /* this must not be dereferenced */
  const GLogField fields[] = {
    { "MESSAGE", "This is a debug message.", -1 },
    { "MESSAGE_ID", "fcfb2e1e65c3494386b74878f1abf893", -1 },
    { "MY_APPLICATION_CUSTOM_FIELD", "some debug string", -1 },
    { "MY_APPLICATION_STATE", state_object, 0 },
  };

  log_count = 0;
  g_log_set_writer_func (null_log_writer, NULL, NULL);

  g_loxstructured_array (G_LOG_LEVEL_DEBUG, fields, G_N_ELEMENTS (fields));

  g_assert_cmpint (log_count, ==, 1);
}

static void
test_structured_logging_robustness (void)
{
  log_count = 0;
  g_log_set_writer_func (null_log_writer, NULL, NULL);

  /* NULL log_domain shouldn't crash */
  g_log (NULL, G_LOG_LEVEL_MESSAGE, "Test");
  g_loxstructured (NULL, G_LOG_LEVEL_MESSAGE, "MESSAGE", "Test");

  g_assert_cmpint (log_count, ==, 1);
}

static void
test_structured_logging_roundtrip1 (void)
{
  xpointer_t some_pointer = GUINT_TO_POINTER (0x100);
  xint_t some_integer = 123;
  xchar_t message[200];
  GLogField fields[] = {
    { "XPL_DOMAIN", "some-domain", -1 },
    { "PRIORITY", "5", -1 },
    { "MESSAGE", "String assigned using g_snprintf() below", -1 },
    { "MESSAGE_ID", "fcfb2e1e65c3494386b74878f1abf893", -1 },
    { "MY_APPLICATION_CUSTOM_FIELD", "some debug string", -1 }
  };
  ExpectedMessage expected = { fields, 5 };

  /* %p format is implementation defined and depends on the platform */
  g_snprintf (message, sizeof (message),
              "This is a debug message about pointer %p and integer %u.",
              some_pointer, some_integer);
  fields[2].value = message;

  expected_messages = xslist_append (NULL, &expected);
  g_log_set_writer_func (expect_log_writer, NULL, NULL);

  g_loxstructured ("some-domain", G_LOG_LEVEL_MESSAGE,
                    "MESSAGE_ID", "fcfb2e1e65c3494386b74878f1abf893",
                    "MY_APPLICATION_CUSTOM_FIELD", "some debug string",
                    "MESSAGE", "This is a debug message about pointer %p and integer %u.",
                    some_pointer, some_integer);

  if (expected_messages != NULL)
    {
      char *str;
      ExpectedMessage *msg = expected_messages->data;

      str = g_log_writer_format_fields (0, msg->fields, msg->n_fields, FALSE);
      g_test_fail_printf ("Unexpected message: %s", str);
      g_free (str);
    }
}

static void
test_structured_logging_roundtrip2 (void)
{
  const xchar_t *some_string = "abc";
  const GLogField fields[] = {
    { "XPL_DOMAIN", "some-domain", -1 },
    { "PRIORITY", "5", -1 },
    { "MESSAGE", "This is a debug message about string 'abc'.", -1 },
    { "MESSAGE_ID", "fcfb2e1e65c3494386b74878f1abf893", -1 },
    { "MY_APPLICATION_CUSTOM_FIELD", "some debug string", -1 }
  };
  ExpectedMessage expected = { fields, 5 };

  expected_messages = xslist_append (NULL, &expected);
  g_log_set_writer_func (expect_log_writer, NULL, NULL);

  g_loxstructured ("some-domain", G_LOG_LEVEL_MESSAGE,
                    "MESSAGE_ID", "fcfb2e1e65c3494386b74878f1abf893",
                    "MY_APPLICATION_CUSTOM_FIELD", "some debug string",
                    "MESSAGE", "This is a debug message about string '%s'.",
                    some_string);

  g_assert (expected_messages == NULL);
}

static void
test_structured_logging_roundtrip3 (void)
{
  const GLogField fields[] = {
    { "XPL_DOMAIN", "some-domain", -1 },
    { "PRIORITY", "4", -1 },
    { "MESSAGE", "Test test test.", -1 }
  };
  ExpectedMessage expected = { fields, 3 };

  expected_messages = xslist_append (NULL, &expected);
  g_log_set_writer_func (expect_log_writer, NULL, NULL);

  g_loxstructured ("some-domain", G_LOG_LEVEL_WARNING,
                    "MESSAGE", "Test test test.");

  g_assert (expected_messages == NULL);
}

static xvariant_t *
create_variant_fields (void)
{
  xvariant_t *binary;
  xvariant_builder_t builder;

  binary = xvariant_new_fixed_array (G_VARIANT_TYPE_BYTE, binary_field, G_N_ELEMENTS (binary_field), sizeof (binary_field[0]));

  xvariant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
  xvariant_builder_add (&builder, "{sv}", "MESSAGE_ID", xvariant_new_string ("06d4df59e6c24647bfe69d2c27ef0b4e"));
  xvariant_builder_add (&builder, "{sv}", "MESSAGE", xvariant_new_string ("This is a debug message"));
  xvariant_builder_add (&builder, "{sv}", "MY_APPLICATION_CUSTOM_FIELD", xvariant_new_string ("some debug string"));
  xvariant_builder_add (&builder, "{sv}", "MY_APPLICATION_CUSTOM_FIELD_BINARY", binary);

  return xvariant_builder_end (&builder);
}

static void
test_structured_logging_variant1 (void)
{
  xvariant_t *v = create_variant_fields ();

  log_count = 0;
  g_log_set_writer_func (null_log_writer, NULL, NULL);

  g_log_variant ("some-domain", G_LOG_LEVEL_MESSAGE, v);
  xvariant_unref (v);
  g_assert_cmpint (log_count, ==, 1);
}

static void
test_structured_logging_variant2 (void)
{
  const GLogField fields[] = {
    { "XPL_DOMAIN", "some-domain", -1 },
    { "PRIORITY", "5", -1 },
    { "MESSAGE", "This is a debug message", -1 },
    { "MESSAGE_ID", "06d4df59e6c24647bfe69d2c27ef0b4e", -1 },
    { "MY_APPLICATION_CUSTOM_FIELD", "some debug string", -1 },
    { "MY_APPLICATION_CUSTOM_FIELD_BINARY", binary_field, sizeof (binary_field) }
  };
  ExpectedMessage expected = { fields, 6 };
  xvariant_t *v = create_variant_fields ();

  expected_messages = xslist_append (NULL, &expected);
  g_log_set_writer_func (expect_log_writer, NULL, NULL);

  g_log_variant ("some-domain", G_LOG_LEVEL_MESSAGE, v);
  xvariant_unref (v);
  g_assert (expected_messages == NULL);
}

int
main (int argc, char *argv[])
{
  g_unsetenv ("G_MESSAGES_DEBUG");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/logging/default-handler", test_default_handler);
  g_test_add_func ("/logging/default-handler/subprocess/error", test_default_handler_error);
  g_test_add_func ("/logging/default-handler/subprocess/critical", test_default_handler_critical);
  g_test_add_func ("/logging/default-handler/subprocess/warning", test_default_handler_warning);
  g_test_add_func ("/logging/default-handler/subprocess/message", test_default_handler_message);
  g_test_add_func ("/logging/default-handler/subprocess/info", test_default_handler_info);
  g_test_add_func ("/logging/default-handler/subprocess/bar-info", test_default_handler_bar_info);
  g_test_add_func ("/logging/default-handler/subprocess/baz-debug", test_default_handler_baz_debug);
  g_test_add_func ("/logging/default-handler/subprocess/debug", test_default_handler_debug);
  g_test_add_func ("/logging/default-handler/subprocess/debug-stderr", test_default_handler_debug_stderr);
  g_test_add_func ("/logging/default-handler/subprocess/0x400", test_default_handler_0x400);
  g_test_add_func ("/logging/default-handler/subprocess/would-drop", test_default_handler_would_drop);
  g_test_add_func ("/logging/warnings", test_warnings);
  g_test_add_func ("/logging/fatal-log-mask", test_fatal_log_mask);
  g_test_add_func ("/logging/set-handler", test_set_handler);
  g_test_add_func ("/logging/print-handler", test_print_handler);
  g_test_add_func ("/logging/printerr-handler", test_printerr_handler);
  g_test_add_func ("/logging/653052", bug653052);
  g_test_add_func ("/logging/gibberish", test_gibberish);
  g_test_add_func ("/structured-logging/no-state", test_structured_logging_no_state);
  g_test_add_func ("/structured-logging/some-state", test_structured_logging_some_state);
  g_test_add_func ("/structured-logging/robustness", test_structured_logging_robustness);
  g_test_add_func ("/structured-logging/roundtrip1", test_structured_logging_roundtrip1);
  g_test_add_func ("/structured-logging/roundtrip2", test_structured_logging_roundtrip2);
  g_test_add_func ("/structured-logging/roundtrip3", test_structured_logging_roundtrip3);
  g_test_add_func ("/structured-logging/variant1", test_structured_logging_variant1);
  g_test_add_func ("/structured-logging/variant2", test_structured_logging_variant2);

  return g_test_run ();
}
