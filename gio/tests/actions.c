#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

#include "gdbus-sessionbus.h"

typedef struct
{
  xvariant_t *params;
  xboolean_t did_run;
} Activation;

static void
activate (xaction_t  *action,
          xvariant_t *parameter,
          xpointer_t  user_data)
{
  Activation *activation = user_data;

  if (parameter)
    activation->params = xvariant_ref (parameter);
  else
    activation->params = NULL;
  activation->did_run = TRUE;
}

static void
test_basic (void)
{
  Activation a = { 0, };
  xsimple_action_t *action;
  xchar_t *name;
  xvariant_type_t *parameter_type;
  xboolean_t enabled;
  xvariant_type_t *state_type;
  xvariant_t *state;

  action = g_simple_action_new ("foo", NULL);
  g_assert_true (g_action_get_enabled (G_ACTION (action)));
  g_assert_null (g_action_get_parameter_type (G_ACTION (action)));
  g_assert_null (g_action_get_state_type (G_ACTION (action)));
  g_assert_null (g_action_get_state_hint (G_ACTION (action)));
  g_assert_null (g_action_get_state (G_ACTION (action)));
  xobject_get (action,
                "name", &name,
                "parameter-type", &parameter_type,
                "enabled", &enabled,
                "state-type", &state_type,
                "state", &state,
                 NULL);
  g_assert_cmpstr (name, ==, "foo");
  g_assert_null (parameter_type);
  g_assert_true (enabled);
  g_assert_null (state_type);
  g_assert_null (state);
  g_free (name);

  xsignal_connect (action, "activate", G_CALLBACK (activate), &a);
  g_assert_false (a.did_run);
  g_action_activate (G_ACTION (action), NULL);
  g_assert_true (a.did_run);
  a.did_run = FALSE;

  g_simple_action_set_enabled (action, FALSE);
  g_action_activate (G_ACTION (action), NULL);
  g_assert_false (a.did_run);

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*xvariant_is_of_type*failed*");
      g_action_activate (G_ACTION (action), xvariant_new_string ("xxx"));
      g_test_assert_expected_messages ();
    }

  xobject_unref (action);
  g_assert_false (a.did_run);

  action = g_simple_action_new ("foo", G_VARIANT_TYPE_STRING);
  g_assert_true (g_action_get_enabled (G_ACTION (action)));
  g_assert_true (xvariant_type_equal (g_action_get_parameter_type (G_ACTION (action)), G_VARIANT_TYPE_STRING));
  g_assert_null (g_action_get_state_type (G_ACTION (action)));
  g_assert_null (g_action_get_state_hint (G_ACTION (action)));
  g_assert_null (g_action_get_state (G_ACTION (action)));

  xsignal_connect (action, "activate", G_CALLBACK (activate), &a);
  g_assert_false (a.did_run);
  g_action_activate (G_ACTION (action), xvariant_new_string ("Hello world"));
  g_assert_true (a.did_run);
  g_assert_cmpstr (xvariant_get_string (a.params, NULL), ==, "Hello world");
  xvariant_unref (a.params);
  a.did_run = FALSE;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*failed*");
      g_action_activate (G_ACTION (action), NULL);
      g_test_assert_expected_messages ();
    }

  xobject_unref (action);
  g_assert_false (a.did_run);
}

static void
test_name (void)
{
  g_assert_false (g_action_name_is_valid (""));
  g_assert_false (g_action_name_is_valid ("("));
  g_assert_false (g_action_name_is_valid ("%abc"));
  g_assert_false (g_action_name_is_valid ("$x1"));
  g_assert_true (g_action_name_is_valid ("abc.def"));
  g_assert_true (g_action_name_is_valid ("ABC-DEF"));
}

static xboolean_t
strv_has_string (xchar_t       **haystack,
                 const xchar_t  *needle)
{
  xuint_t n;

  for (n = 0; haystack != NULL && haystack[n] != NULL; n++)
    {
      if (xstrcmp0 (haystack[n], needle) == 0)
        return TRUE;
    }
  return FALSE;
}

static xboolean_t
strv_strv_cmp (xchar_t **a, xchar_t **b)
{
  xuint_t n;

  for (n = 0; a[n] != NULL; n++)
    {
       if (!strv_has_string (b, a[n]))
         return FALSE;
    }

  for (n = 0; b[n] != NULL; n++)
    {
       if (!strv_has_string (a, b[n]))
         return FALSE;
    }

  return TRUE;
}

static xboolean_t
strv_set_equal (xchar_t **strv, ...)
{
  xuint_t count;
  va_list list;
  const xchar_t *str;
  xboolean_t res;

  res = TRUE;
  count = 0;
  va_start (list, strv);
  while (1)
    {
      str = va_arg (list, const xchar_t *);
      if (str == NULL)
        break;
      if (!strv_has_string (strv, str))
        {
          res = FALSE;
          break;
        }
      count++;
    }
  va_end (list);

  if (res)
    res = xstrv_length ((xchar_t**)strv) == count;

  return res;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
test_simple_group (void)
{
  xsimple_action_group_t *group;
  Activation a = { 0, };
  xsimple_action_t *simple;
  xaction_t *action;
  xchar_t **actions;
  xvariant_t *state;

  simple = g_simple_action_new ("foo", NULL);
  xsignal_connect (simple, "activate", G_CALLBACK (activate), &a);
  g_assert_false (a.did_run);
  g_action_activate (G_ACTION (simple), NULL);
  g_assert_true (a.did_run);
  a.did_run = FALSE;

  group = g_simple_action_group_new ();
  g_simple_action_group_insert (group, G_ACTION (simple));
  xobject_unref (simple);

  g_assert_false (a.did_run);
  xaction_group_activate_action (XACTION_GROUP (group), "foo", NULL);
  g_assert_true (a.did_run);

  simple = g_simple_action_new_stateful ("bar", G_VARIANT_TYPE_STRING, xvariant_new_string ("hihi"));
  g_simple_action_group_insert (group, G_ACTION (simple));
  xobject_unref (simple);

  g_assert_true (xaction_group_has_action (XACTION_GROUP (group), "foo"));
  g_assert_true (xaction_group_has_action (XACTION_GROUP (group), "bar"));
  g_assert_false (xaction_group_has_action (XACTION_GROUP (group), "baz"));
  actions = xaction_group_list_actions (XACTION_GROUP (group));
  g_assert_cmpint (xstrv_length (actions), ==, 2);
  g_assert_true (strv_set_equal (actions, "foo", "bar", NULL));
  xstrfreev (actions);
  g_assert_true (xaction_group_get_action_enabled (XACTION_GROUP (group), "foo"));
  g_assert_true (xaction_group_get_action_enabled (XACTION_GROUP (group), "bar"));
  g_assert_null (xaction_group_get_action_parameter_type (XACTION_GROUP (group), "foo"));
  g_assert_true (xvariant_type_equal (xaction_group_get_action_parameter_type (XACTION_GROUP (group), "bar"), G_VARIANT_TYPE_STRING));
  g_assert_null (xaction_group_get_action_state_type (XACTION_GROUP (group), "foo"));
  g_assert_true (xvariant_type_equal (xaction_group_get_action_state_type (XACTION_GROUP (group), "bar"), G_VARIANT_TYPE_STRING));
  g_assert_null (xaction_group_get_action_state_hint (XACTION_GROUP (group), "foo"));
  g_assert_null (xaction_group_get_action_state_hint (XACTION_GROUP (group), "bar"));
  g_assert_null (xaction_group_get_action_state (XACTION_GROUP (group), "foo"));
  state = xaction_group_get_action_state (XACTION_GROUP (group), "bar");
  g_assert_true (xvariant_type_equal (xvariant_get_type (state), G_VARIANT_TYPE_STRING));
  g_assert_cmpstr (xvariant_get_string (state, NULL), ==, "hihi");
  xvariant_unref (state);

  xaction_group_change_action_state (XACTION_GROUP (group), "bar", xvariant_new_string ("boo"));
  state = xaction_group_get_action_state (XACTION_GROUP (group), "bar");
  g_assert_cmpstr (xvariant_get_string (state, NULL), ==, "boo");
  xvariant_unref (state);

  action = g_simple_action_group_lookup (group, "bar");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
  g_assert_false (xaction_group_get_action_enabled (XACTION_GROUP (group), "bar"));

  g_simple_action_group_remove (group, "bar");
  action = g_simple_action_group_lookup (group, "foo");
  g_assert_cmpstr (g_action_get_name (action), ==, "foo");
  action = g_simple_action_group_lookup (group, "bar");
  g_assert_null (action);

  simple = g_simple_action_new ("foo", NULL);
  g_simple_action_group_insert (group, G_ACTION (simple));
  xobject_unref (simple);

  a.did_run = FALSE;
  xobject_unref (group);
  g_assert_false (a.did_run);
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_stateful (void)
{
  xsimple_action_t *action;
  xvariant_t *state;

  action = g_simple_action_new_stateful ("foo", NULL, xvariant_new_string ("hihi"));
  g_assert_true (g_action_get_enabled (G_ACTION (action)));
  g_assert_null (g_action_get_parameter_type (G_ACTION (action)));
  g_assert_null (g_action_get_state_hint (G_ACTION (action)));
  g_assert_true (xvariant_type_equal (g_action_get_state_type (G_ACTION (action)),
                                       G_VARIANT_TYPE_STRING));
  state = g_action_get_state (G_ACTION (action));
  g_assert_cmpstr (xvariant_get_string (state, NULL), ==, "hihi");
  xvariant_unref (state);

  if (g_test_undefined ())
    {
      xvariant_t *new_state = xvariant_ref_sink (xvariant_new_int32 (123));
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*xvariant_is_of_type*failed*");
      g_simple_action_set_state (action, new_state);
      g_test_assert_expected_messages ();
      xvariant_unref (new_state);
    }

  g_simple_action_set_state (action, xvariant_new_string ("hello"));
  state = g_action_get_state (G_ACTION (action));
  g_assert_cmpstr (xvariant_get_string (state, NULL), ==, "hello");
  xvariant_unref (state);

  xobject_unref (action);

  action = g_simple_action_new ("foo", NULL);

  if (g_test_undefined ())
    {
      xvariant_t *new_state = xvariant_ref_sink (xvariant_new_int32 (123));
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*failed*");
      g_simple_action_set_state (action, new_state);
      g_test_assert_expected_messages ();
      xvariant_unref (new_state);
    }

  xobject_unref (action);
}

static void
test_default_activate (void)
{
  xsimple_action_t *action;
  xvariant_t *state;

  /* test_t changing state via activation with parameter */
  action = g_simple_action_new_stateful ("foo", G_VARIANT_TYPE_STRING, xvariant_new_string ("hihi"));
  g_action_activate (G_ACTION (action), xvariant_new_string ("bye"));
  state = g_action_get_state (G_ACTION (action));
  g_assert_cmpstr (xvariant_get_string (state, NULL), ==, "bye");
  xvariant_unref (state);
  xobject_unref (action);

  /* test_t toggling a boolean action via activation with no parameter */
  action = g_simple_action_new_stateful ("foo", NULL, xvariant_new_boolean (FALSE));
  g_action_activate (G_ACTION (action), NULL);
  state = g_action_get_state (G_ACTION (action));
  g_assert_true (xvariant_get_boolean (state));
  xvariant_unref (state);
  /* and back again */
  g_action_activate (G_ACTION (action), NULL);
  state = g_action_get_state (G_ACTION (action));
  g_assert_false (xvariant_get_boolean (state));
  xvariant_unref (state);
  xobject_unref (action);
}

static xboolean_t foo_activated = FALSE;
static xboolean_t bar_activated = FALSE;

static void
activate_foo (xsimple_action_t *simple,
              xvariant_t      *parameter,
              xpointer_t       user_data)
{
  g_assert_true (user_data == GINT_TO_POINTER (123));
  g_assert_null (parameter);
  foo_activated = TRUE;
}

static void
activate_bar (xsimple_action_t *simple,
              xvariant_t      *parameter,
              xpointer_t       user_data)
{
  g_assert_true (user_data == GINT_TO_POINTER (123));
  g_assert_cmpstr (xvariant_get_string (parameter, NULL), ==, "param");
  bar_activated = TRUE;
}

static void
change_volume_state (xsimple_action_t *action,
                     xvariant_t      *value,
                     xpointer_t       user_data)
{
  xint_t requested;

  requested = xvariant_get_int32 (value);

  /* Volume only goes from 0 to 10 */
  if (0 <= requested && requested <= 10)
    g_simple_action_set_state (action, value);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
test_entries (void)
{
  const xaction_entry_t entries[] = {
    { "foo",    activate_foo, NULL, NULL,    NULL,                { 0 } },
    { "bar",    activate_bar, "s",  NULL,    NULL,                { 0 } },
    { "toggle", NULL,         NULL, "false", NULL,                { 0 } },
    { "volume", NULL,         NULL, "0",     change_volume_state, { 0 } },
  };
  xsimple_action_group_t *actions;
  xvariant_t *state;

  actions = g_simple_action_group_new ();
  g_simple_action_group_add_entries (actions, entries,
                                     G_N_ELEMENTS (entries),
                                     GINT_TO_POINTER (123));

  g_assert_false (foo_activated);
  xaction_group_activate_action (XACTION_GROUP (actions), "foo", NULL);
  g_assert_true (foo_activated);
  foo_activated = FALSE;

  g_assert_false (bar_activated);
  xaction_group_activate_action (XACTION_GROUP (actions), "bar",
                                  xvariant_new_string ("param"));
  g_assert_true (bar_activated);
  g_assert_false (foo_activated);

  if (g_test_undefined ())
    {
      const xaction_entry_t bad_type = {
        "bad-type", NULL, "ss", NULL, NULL, { 0 }
      };
      const xaction_entry_t bad_state = {
        "bad-state", NULL, NULL, "flse", NULL, { 0 }
      };

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*not a valid xvariant_t type string*");
      g_simple_action_group_add_entries (actions, &bad_type, 1, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*could not parse*");
      g_simple_action_group_add_entries (actions, &bad_state, 1, NULL);
      g_test_assert_expected_messages ();
    }

  state = xaction_group_get_action_state (XACTION_GROUP (actions), "volume");
  g_assert_cmpint (xvariant_get_int32 (state), ==, 0);
  xvariant_unref (state);

  /* should change */
  xaction_group_change_action_state (XACTION_GROUP (actions), "volume",
                                      xvariant_new_int32 (7));
  state = xaction_group_get_action_state (XACTION_GROUP (actions), "volume");
  g_assert_cmpint (xvariant_get_int32 (state), ==, 7);
  xvariant_unref (state);

  /* should not change */
  xaction_group_change_action_state (XACTION_GROUP (actions), "volume",
                                      xvariant_new_int32 (11));
  state = xaction_group_get_action_state (XACTION_GROUP (actions), "volume");
  g_assert_cmpint (xvariant_get_int32 (state), ==, 7);
  xvariant_unref (state);

  xobject_unref (actions);
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_parse_detailed (void)
{
  struct {
    const xchar_t *detailed;
    const xchar_t *expected_name;
    const xchar_t *expected_target;
    const xchar_t *expected_error;
    const xchar_t *detailed_roundtrip;
  } testcases[] = {
    { "abc",              "abc",    NULL,       NULL,             "abc" },
    { " abc",             NULL,     NULL,       "invalid format", NULL },
    { " abc",             NULL,     NULL,       "invalid format", NULL },
    { "abc:",             NULL,     NULL,       "invalid format", NULL },
    { ":abc",             NULL,     NULL,       "invalid format", NULL },
    { "abc(",             NULL,     NULL,       "invalid format", NULL },
    { "abc)",             NULL,     NULL,       "invalid format", NULL },
    { "(abc",             NULL,     NULL,       "invalid format", NULL },
    { ")abc",             NULL,     NULL,       "invalid format", NULL },
    { "abc::xyz",         "abc",    "'xyz'",    NULL,             "abc::xyz" },
    { "abc('xyz')",       "abc",    "'xyz'",    NULL,             "abc::xyz" },
    { "abc(42)",          "abc",    "42",       NULL,             "abc(42)" },
    { "abc(int32 42)",    "abc",    "42",       NULL,             "abc(42)" },
    { "abc(@i 42)",       "abc",    "42",       NULL,             "abc(42)" },
    { "abc (42)",         NULL,     NULL,       "invalid format", NULL },
    { "abc(42abc)",       NULL,     NULL,       "invalid character in number", NULL },
    { "abc(42, 4)",       "abc",    "(42, 4)",  "expected end of input", NULL },
    { "abc(42,)",         "abc",    "(42,)",    "expected end of input", NULL }
  };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (testcases); i++)
    {
      xerror_t *error = NULL;
      xvariant_t *target;
      xboolean_t success;
      xchar_t *name;

      success = g_action_parse_detailed_name (testcases[i].detailed, &name, &target, &error);
      g_assert_true (success == (error == NULL));
      if (success && testcases[i].expected_error)
        xerror ("Unexpected success on '%s'.  Expected error containing '%s'",
                 testcases[i].detailed, testcases[i].expected_error);

      if (!success && !testcases[i].expected_error)
        xerror ("Unexpected failure on '%s': %s", testcases[i].detailed, error->message);

      if (!success)
        {
          if (!strstr (error->message, testcases[i].expected_error))
            xerror ("Failure message '%s' for string '%s' did not contained expected substring '%s'",
                     error->message, testcases[i].detailed, testcases[i].expected_error);

          xerror_free (error);
          continue;
        }

      g_assert_cmpstr (name, ==, testcases[i].expected_name);
      g_assert_true ((target == NULL) == (testcases[i].expected_target == NULL));

      if (success)
        {
          xchar_t *detailed;

          detailed = g_action_print_detailed_name (name, target);
          g_assert_cmpstr (detailed, ==, testcases[i].detailed_roundtrip);
          g_free (detailed);
        }

      if (target)
        {
          xvariant_t *expected;

          expected = xvariant_parse (NULL, testcases[i].expected_target, NULL, NULL, NULL);
          g_assert_true (expected);

          g_assert_cmpvariant (expected, target);
          xvariant_unref (expected);
          xvariant_unref (target);
        }

      g_free (name);
    }
}

xhashtable_t *activation_counts;

static void
count_activation (const xchar_t *action)
{
  xint_t count;

  if (activation_counts == NULL)
    activation_counts = xhash_table_new (xstr_hash, xstr_equal);
  count = GPOINTER_TO_INT (xhash_table_lookup (activation_counts, action));
  count++;
  xhash_table_insert (activation_counts, (xpointer_t)action, GINT_TO_POINTER (count));

  xmain_context_wakeup (NULL);
}

static xint_t
activation_count (const xchar_t *action)
{
  if (activation_counts == NULL)
    return 0;

  return GPOINTER_TO_INT (xhash_table_lookup (activation_counts, action));
}

static void
activate_action (xsimple_action_t *action, xvariant_t *parameter, xpointer_t user_data)
{
  count_activation (g_action_get_name (G_ACTION (action)));
}

static void
activate_toggle (xsimple_action_t *action, xvariant_t *parameter, xpointer_t user_data)
{
  xvariant_t *old_state, *new_state;

  count_activation (g_action_get_name (G_ACTION (action)));

  old_state = g_action_get_state (G_ACTION (action));
  new_state = xvariant_new_boolean (!xvariant_get_boolean (old_state));
  g_simple_action_set_state (action, new_state);
  xvariant_unref (old_state);
}

static void
activate_radio (xsimple_action_t *action, xvariant_t *parameter, xpointer_t user_data)
{
  xvariant_t *new_state;

  count_activation (g_action_get_name (G_ACTION (action)));

  new_state = xvariant_new_string (xvariant_get_string (parameter, NULL));
  g_simple_action_set_state (action, new_state);
}

static xboolean_t
compare_action_groups (xaction_group_t *a, xaction_group_t *b)
{
  xchar_t **alist;
  xchar_t **blist;
  xint_t i;
  xboolean_t equal;
  xboolean_t ares, bres;
  xboolean_t aenabled, benabled;
  const xvariant_type_t *aparameter_type, *bparameter_type;
  const xvariant_type_t *astate_type, *bstate_type;
  xvariant_t *astate_hint, *bstate_hint;
  xvariant_t *astate, *bstate;

  alist = xaction_group_list_actions (a);
  blist = xaction_group_list_actions (b);
  equal = strv_strv_cmp (alist, blist);

  for (i = 0; equal && alist[i]; i++)
    {
      ares = xaction_group_query_action (a, alist[i], &aenabled, &aparameter_type, &astate_type, &astate_hint, &astate);
      bres = xaction_group_query_action (b, alist[i], &benabled, &bparameter_type, &bstate_type, &bstate_hint, &bstate);

      if (ares && bres)
        {
          equal = equal && (aenabled == benabled);
          equal = equal && ((!aparameter_type && !bparameter_type) || xvariant_type_equal (aparameter_type, bparameter_type));
          equal = equal && ((!astate_type && !bstate_type) || xvariant_type_equal (astate_type, bstate_type));
          equal = equal && ((!astate_hint && !bstate_hint) || xvariant_equal (astate_hint, bstate_hint));
          equal = equal && ((!astate && !bstate) || xvariant_equal (astate, bstate));

          if (astate_hint)
            xvariant_unref (astate_hint);
          if (bstate_hint)
            xvariant_unref (bstate_hint);
          if (astate)
            xvariant_unref (astate);
          if (bstate)
            xvariant_unref (bstate);
        }
      else
        equal = FALSE;
    }

  xstrfreev (alist);
  xstrfreev (blist);

  return equal;
}

static xboolean_t
stop_loop (xpointer_t data)
{
  xmain_loop_t *loop = data;

  xmain_loop_quit (loop);

  return G_SOURCE_REMOVE;
}

static xaction_entry_t exported_entries[] = {
  { "undo",  activate_action, NULL, NULL,      NULL, { 0 } },
  { "redo",  activate_action, NULL, NULL,      NULL, { 0 } },
  { "cut",   activate_action, NULL, NULL,      NULL, { 0 } },
  { "copy",  activate_action, NULL, NULL,      NULL, { 0 } },
  { "paste", activate_action, NULL, NULL,      NULL, { 0 } },
  { "bold",  activate_toggle, NULL, "true",    NULL, { 0 } },
  { "lang",  activate_radio,  "s",  "'latin'", NULL, { 0 } },
};

static void
list_cb (xobject_t      *source,
         xasync_result_t *res,
         xpointer_t      user_data)
{
  xdbus_connection_t *bus = G_DBUS_CONNECTION (source);
  xmain_loop_t *loop = user_data;
  xerror_t *error = NULL;
  xvariant_t *v;
  xchar_t **actions;

  v = xdbus_connection_call_finish (bus, res, &error);
  g_assert_nonnull (v);
  xvariant_get (v, "(^a&s)", &actions);
  g_assert_cmpint (xstrv_length (actions), ==, G_N_ELEMENTS (exported_entries));
  g_free (actions);
  xvariant_unref (v);
  xmain_loop_quit (loop);
}

static xboolean_t
call_list (xpointer_t user_data)
{
  xdbus_connection_t *bus;

  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  xdbus_connection_call (bus,
                          xdbus_connection_get_unique_name (bus),
                          "/",
                          "org.gtk.Actions",
                          "List",
                          NULL,
                          NULL,
                          0,
                          G_MAXINT,
                          NULL,
                          list_cb,
                          user_data);
  xobject_unref (bus);

  return G_SOURCE_REMOVE;
}

static void
describe_cb (xobject_t      *source,
             xasync_result_t *res,
             xpointer_t      user_data)
{
  xdbus_connection_t *bus = G_DBUS_CONNECTION (source);
  xmain_loop_t *loop = user_data;
  xerror_t *error = NULL;
  xvariant_t *v;
  xboolean_t enabled;
  xchar_t *param;
  xvariant_iter_t *iter;

  v = xdbus_connection_call_finish (bus, res, &error);
  g_assert_nonnull (v);
  /* FIXME: there's an extra level of tuplelization in here */
  xvariant_get (v, "((bgav))", &enabled, &param, &iter);
  g_assert_true (enabled);
  g_assert_cmpstr (param, ==, "");
  g_assert_cmpint (xvariant_iter_n_children (iter), ==, 0);
  g_free (param);
  xvariant_iter_free (iter);
  xvariant_unref (v);

  xmain_loop_quit (loop);
}

static xboolean_t
call_describe (xpointer_t user_data)
{
  xdbus_connection_t *bus;

  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  xdbus_connection_call (bus,
                          xdbus_connection_get_unique_name (bus),
                          "/",
                          "org.gtk.Actions",
                          "Describe",
                          xvariant_new ("(s)", "copy"),
                          NULL,
                          0,
                          G_MAXINT,
                          NULL,
                          describe_cb,
                          user_data);
  xobject_unref (bus);

  return G_SOURCE_REMOVE;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
action_added_removed_cb (xaction_group_t *action_group,
                         char         *action_name,
                         xpointer_t      user_data)
{
  xuint_t *counter = user_data;

  *counter = *counter + 1;
  xmain_context_wakeup (NULL);
}

static void
action_enabled_changed_cb (xaction_group_t *action_group,
                           char         *action_name,
                           xboolean_t      enabled,
                           xpointer_t      user_data)
{
  xuint_t *counter = user_data;

  *counter = *counter + 1;
  xmain_context_wakeup (NULL);
}

static void
action_state_changed_cb (xaction_group_t *action_group,
                         char         *action_name,
                         xvariant_t     *value,
                         xpointer_t      user_data)
{
  xuint_t *counter = user_data;

  *counter = *counter + 1;
  xmain_context_wakeup (NULL);
}

static void
test_dbus_export (void)
{
  xdbus_connection_t *bus;
  xsimple_action_group_t *group;
  xdbus_action_group_t *proxy;
  xsimple_action_t *action;
  xmain_loop_t *loop;
  xerror_t *error = NULL;
  xvariant_t *v;
  xuint_t id;
  xchar_t **actions;
  xuint_t n_actions_added = 0, n_actions_enabled_changed = 0, n_actions_removed = 0, n_actions_state_changed = 0;
  xulong_t added_signal_id, enabled_changed_signal_id, removed_signal_id, state_changed_signal_id;

  loop = xmain_loop_new (NULL, FALSE);

  session_bus_up ();
  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  group = g_simple_action_group_new ();
  g_simple_action_group_add_entries (group,
                                     exported_entries,
                                     G_N_ELEMENTS (exported_entries),
                                     NULL);

  id = xdbus_connection_export_action_group (bus, "/", XACTION_GROUP (group), &error);
  g_assert_no_error (error);

  proxy = xdbus_action_group_get (bus, xdbus_connection_get_unique_name (bus), "/");
  added_signal_id = xsignal_connect (proxy, "action-added", G_CALLBACK (action_added_removed_cb), &n_actions_added);
  enabled_changed_signal_id = xsignal_connect (proxy, "action-enabled-changed", G_CALLBACK (action_enabled_changed_cb), &n_actions_enabled_changed);
  removed_signal_id = xsignal_connect (proxy, "action-removed", G_CALLBACK (action_added_removed_cb), &n_actions_removed);
  state_changed_signal_id = xsignal_connect (proxy, "action-state-changed", G_CALLBACK (action_state_changed_cb), &n_actions_state_changed);

  actions = xaction_group_list_actions (XACTION_GROUP (proxy));
  g_assert_cmpint (xstrv_length (actions), ==, 0);
  xstrfreev (actions);

  /* Actions are queried from the bus asynchronously after the first
   * list_actions() call. Wait for the expected signals then check again. */
  while (n_actions_added < G_N_ELEMENTS (exported_entries))
    xmain_context_iteration (NULL, TRUE);

  actions = xaction_group_list_actions (XACTION_GROUP (proxy));
  g_assert_cmpint (xstrv_length (actions), ==, G_N_ELEMENTS (exported_entries));
  xstrfreev (actions);

  /* check that calling "List" works too */
  g_idle_add (call_list, loop);
  xmain_loop_run (loop);

  /* check that calling "Describe" works */
  g_idle_add (call_describe, loop);
  xmain_loop_run (loop);

  /* test that the initial transfer works */
  g_assert_true (X_IS_DBUS_ACTION_GROUP (proxy));
  g_assert_true (compare_action_groups (XACTION_GROUP (group), XACTION_GROUP (proxy)));

  /* test that various changes get propagated from group to proxy */
  n_actions_added = 0;
  action = g_simple_action_new_stateful ("italic", NULL, xvariant_new_boolean (FALSE));
  g_simple_action_group_insert (group, G_ACTION (action));
  xobject_unref (action);

  while (n_actions_added == 0)
    xmain_context_iteration (NULL, TRUE);

  g_assert_true (compare_action_groups (XACTION_GROUP (group), XACTION_GROUP (proxy)));

  action = G_SIMPLE_ACTION (g_simple_action_group_lookup (group, "cut"));
  g_simple_action_set_enabled (action, FALSE);

  while (n_actions_enabled_changed == 0)
    xmain_context_iteration (NULL, TRUE);

  g_assert_true (compare_action_groups (XACTION_GROUP (group), XACTION_GROUP (proxy)));

  action = G_SIMPLE_ACTION (g_simple_action_group_lookup (group, "bold"));
  g_simple_action_set_state (action, xvariant_new_boolean (FALSE));

  while (n_actions_state_changed == 0)
    xmain_context_iteration (NULL, TRUE);

  g_assert_true (compare_action_groups (XACTION_GROUP (group), XACTION_GROUP (proxy)));

  g_simple_action_group_remove (group, "italic");

  while (n_actions_removed == 0)
    xmain_context_iteration (NULL, TRUE);

  g_assert_true (compare_action_groups (XACTION_GROUP (group), XACTION_GROUP (proxy)));

  /* test that activations and state changes propagate the other way */
  n_actions_state_changed = 0;
  g_assert_cmpint (activation_count ("copy"), ==, 0);
  xaction_group_activate_action (XACTION_GROUP (proxy), "copy", NULL);

  while (activation_count ("copy") == 0)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpint (activation_count ("copy"), ==, 1);
  g_assert_true (compare_action_groups (XACTION_GROUP (group), XACTION_GROUP (proxy)));

  n_actions_state_changed = 0;
  g_assert_cmpint (activation_count ("bold"), ==, 0);
  xaction_group_activate_action (XACTION_GROUP (proxy), "bold", NULL);

  while (n_actions_state_changed == 0)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpint (activation_count ("bold"), ==, 1);
  g_assert_true (compare_action_groups (XACTION_GROUP (group), XACTION_GROUP (proxy)));
  v = xaction_group_get_action_state (XACTION_GROUP (group), "bold");
  g_assert_true (xvariant_get_boolean (v));
  xvariant_unref (v);

  n_actions_state_changed = 0;
  xaction_group_change_action_state (XACTION_GROUP (proxy), "bold", xvariant_new_boolean (FALSE));

  while (n_actions_state_changed == 0)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpint (activation_count ("bold"), ==, 1);
  g_assert_true (compare_action_groups (XACTION_GROUP (group), XACTION_GROUP (proxy)));
  v = xaction_group_get_action_state (XACTION_GROUP (group), "bold");
  g_assert_false (xvariant_get_boolean (v));
  xvariant_unref (v);

  xdbus_connection_unexport_action_group (bus, id);

  xsignal_handler_disconnect (proxy, added_signal_id);
  xsignal_handler_disconnect (proxy, enabled_changed_signal_id);
  xsignal_handler_disconnect (proxy, removed_signal_id);
  xsignal_handler_disconnect (proxy, state_changed_signal_id);
  xobject_unref (proxy);
  xobject_unref (group);
  xmain_loop_unref (loop);
  xobject_unref (bus);

  session_bus_down ();
}

static xpointer_t
do_export (xpointer_t data)
{
  xaction_group_t *group = data;
  xmain_context_t *ctx;
  xint_t i;
  xerror_t *error = NULL;
  xuint_t id;
  xdbus_connection_t *bus;
  xaction_t *action;
  xchar_t *path;

  ctx = xmain_context_new ();

  xmain_context_push_thread_default (ctx);

  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  path = xstrdup_printf("/%p", data);

  for (i = 0; i < 10000; i++)
    {
      id = xdbus_connection_export_action_group (bus, path, XACTION_GROUP (group), &error);
      g_assert_no_error (error);

      action = g_simple_action_group_lookup (G_SIMPLE_ACTION_GROUP (group), "a");
      g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                                   !g_action_get_enabled (action));

      xdbus_connection_unexport_action_group (bus, id);

      while (xmain_context_iteration (ctx, FALSE));
    }

  g_free (path);
  xobject_unref (bus);

  xmain_context_pop_thread_default (ctx);

  xmain_context_unref (ctx);

  return NULL;
}

static void
test_dbus_threaded (void)
{
  xsimple_action_group_t *group[10];
  xthread_t *export[10];
  static xaction_entry_t entries[] = {
    { "a",  activate_action, NULL, NULL, NULL, { 0 } },
    { "b",  activate_action, NULL, NULL, NULL, { 0 } },
  };
  xint_t i;

  session_bus_up ();

  for (i = 0; i < 10; i++)
    {
      group[i] = g_simple_action_group_new ();
      g_simple_action_group_add_entries (group[i], entries, G_N_ELEMENTS (entries), NULL);
      export[i] = xthread_new ("export", do_export, group[i]);
    }

  for (i = 0; i < 10; i++)
    xthread_join (export[i]);

  for (i = 0; i < 10; i++)
    xobject_unref (group[i]);

  session_bus_down ();
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_bug679509 (void)
{
  xdbus_connection_t *bus;
  xdbus_action_group_t *proxy;
  xmain_loop_t *loop;

  loop = xmain_loop_new (NULL, FALSE);

  session_bus_up ();
  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  proxy = xdbus_action_group_get (bus, xdbus_connection_get_unique_name (bus), "/");
  xstrfreev (xaction_group_list_actions (XACTION_GROUP (proxy)));
  xobject_unref (proxy);

  g_timeout_add (100, stop_loop, loop);
  xmain_loop_run (loop);

  xmain_loop_unref (loop);
  xobject_unref (bus);

  session_bus_down ();
}

static xchar_t *state_change_log;

static void
state_changed (xaction_group_t *group,
               const xchar_t  *action_name,
               xvariant_t     *value,
               xpointer_t      user_data)
{
  xstring_t *string;

  g_assert_false (state_change_log);

  string = xstring_new (action_name);
  xstring_append_c (string, ':');
  xvariant_print_string (value, string, TRUE);
  state_change_log = xstring_free (string, FALSE);
}

static void
verify_changed (const xchar_t *log_entry)
{
  g_assert_cmpstr (state_change_log, ==, log_entry);
  g_clear_pointer (&state_change_log, g_free);
}

static void
ensure_state (xsimple_action_group_t *group,
              const xchar_t        *action_name,
              const xchar_t        *expected)
{
  xvariant_t *value;
  xchar_t *printed;

  value = xaction_group_get_action_state (XACTION_GROUP (group), action_name);
  printed = xvariant_print (value, TRUE);
  xvariant_unref (value);

  g_assert_cmpstr (printed, ==, expected);
  g_free (printed);
}

static void
test_property_actions (void)
{
  xsimple_action_group_t *group;
  xproperty_action_t *action;
  xsocket_client_t *client;
  xapplication_t *app;
  xchar_t *name;
  xvariant_type_t *ptype, *stype;
  xboolean_t enabled;
  xvariant_t *state;

  group = g_simple_action_group_new ();
  xsignal_connect (group, "action-state-changed", G_CALLBACK (state_changed), NULL);

  client = xsocket_client_new ();
  app = xapplication_new ("org.gtk.test", 0);

  /* string... */
  action = g_property_action_new ("app-id", app, "application-id");
  xaction_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
  xobject_unref (action);

  /* uint... */
  action = g_property_action_new ("keepalive", app, "inactivity-timeout");
  xobject_get (action, "name", &name, "parameter-type", &ptype, "enabled", &enabled, "state-type", &stype, "state", &state, NULL);
  g_assert_cmpstr (name, ==, "keepalive");
  g_assert_true (enabled);
  g_free (name);
  xvariant_type_free (ptype);
  xvariant_type_free (stype);
  xvariant_unref (state);

  xaction_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
  xobject_unref (action);

  /* bool... */
  action = g_property_action_new ("tls", client, "tls");
  xaction_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
  xobject_unref (action);

  /* inverted */
  action = xobject_new (XTYPE_PROPERTY_ACTION,
                         "name", "disable-proxy",
                         "object", client,
                         "property-name", "enable-proxy",
                         "invert-boolean", TRUE,
                         NULL);
  xaction_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
  xobject_unref (action);

  /* enum... */
  action = g_property_action_new ("type", client, "type");
  xaction_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
  xobject_unref (action);

  /* the objects should be held alive by the actions... */
  xobject_unref (client);
  xobject_unref (app);

  ensure_state (group, "app-id", "'org.gtk.test'");
  ensure_state (group, "keepalive", "uint32 0");
  ensure_state (group, "tls", "false");
  ensure_state (group, "disable-proxy", "false");
  ensure_state (group, "type", "'stream'");

  verify_changed (NULL);

  /* some string tests... */
  xaction_group_change_action_state (XACTION_GROUP (group), "app-id", xvariant_new ("s", "org.gtk.test2"));
  verify_changed ("app-id:'org.gtk.test2'");
  g_assert_cmpstr (xapplication_get_application_id (app), ==, "org.gtk.test2");
  ensure_state (group, "app-id", "'org.gtk.test2'");

  xaction_group_activate_action (XACTION_GROUP (group), "app-id", xvariant_new ("s", "org.gtk.test3"));
  verify_changed ("app-id:'org.gtk.test3'");
  g_assert_cmpstr (xapplication_get_application_id (app), ==, "org.gtk.test3");
  ensure_state (group, "app-id", "'org.gtk.test3'");

  xapplication_set_application_id (app, "org.gtk.test");
  verify_changed ("app-id:'org.gtk.test'");
  ensure_state (group, "app-id", "'org.gtk.test'");

  /* uint tests */
  xaction_group_change_action_state (XACTION_GROUP (group), "keepalive", xvariant_new ("u", 1234));
  verify_changed ("keepalive:uint32 1234");
  g_assert_cmpuint (xapplication_get_inactivity_timeout (app), ==, 1234);
  ensure_state (group, "keepalive", "uint32 1234");

  xaction_group_activate_action (XACTION_GROUP (group), "keepalive", xvariant_new ("u", 5678));
  verify_changed ("keepalive:uint32 5678");
  g_assert_cmpuint (xapplication_get_inactivity_timeout (app), ==, 5678);
  ensure_state (group, "keepalive", "uint32 5678");

  xapplication_set_inactivity_timeout (app, 0);
  verify_changed ("keepalive:uint32 0");
  ensure_state (group, "keepalive", "uint32 0");

  /* bool tests */
  xaction_group_change_action_state (XACTION_GROUP (group), "tls", xvariant_new ("b", TRUE));
  verify_changed ("tls:true");
  g_assert_true (xsocket_client_get_tls (client));
  ensure_state (group, "tls", "true");

  xaction_group_change_action_state (XACTION_GROUP (group), "disable-proxy", xvariant_new ("b", TRUE));
  verify_changed ("disable-proxy:true");
  ensure_state (group, "disable-proxy", "true");
  g_assert_false (xsocket_client_get_enable_proxy (client));

  /* test toggle true->false */
  xaction_group_activate_action (XACTION_GROUP (group), "tls", NULL);
  verify_changed ("tls:false");
  g_assert_false (xsocket_client_get_tls (client));
  ensure_state (group, "tls", "false");

  /* and now back false->true */
  xaction_group_activate_action (XACTION_GROUP (group), "tls", NULL);
  verify_changed ("tls:true");
  g_assert_true (xsocket_client_get_tls (client));
  ensure_state (group, "tls", "true");

  xsocket_client_set_tls (client, FALSE);
  verify_changed ("tls:false");
  ensure_state (group, "tls", "false");

  /* now do the same for the inverted action */
  xaction_group_activate_action (XACTION_GROUP (group), "disable-proxy", NULL);
  verify_changed ("disable-proxy:false");
  g_assert_true (xsocket_client_get_enable_proxy (client));
  ensure_state (group, "disable-proxy", "false");

  xaction_group_activate_action (XACTION_GROUP (group), "disable-proxy", NULL);
  verify_changed ("disable-proxy:true");
  g_assert_false (xsocket_client_get_enable_proxy (client));
  ensure_state (group, "disable-proxy", "true");

  xsocket_client_set_enable_proxy (client, TRUE);
  verify_changed ("disable-proxy:false");
  ensure_state (group, "disable-proxy", "false");

  /* enum tests */
  xaction_group_change_action_state (XACTION_GROUP (group), "type", xvariant_new ("s", "datagram"));
  verify_changed ("type:'datagram'");
  g_assert_cmpint (xsocket_client_get_socket_type (client), ==, XSOCKET_TYPE_DATAGRAM);
  ensure_state (group, "type", "'datagram'");

  xaction_group_activate_action (XACTION_GROUP (group), "type", xvariant_new ("s", "stream"));
  verify_changed ("type:'stream'");
  g_assert_cmpint (xsocket_client_get_socket_type (client), ==, XSOCKET_TYPE_STREAM);
  ensure_state (group, "type", "'stream'");

  xsocket_client_set_socket_type (client, XSOCKET_TYPE_SEQPACKET);
  verify_changed ("type:'seqpacket'");
  ensure_state (group, "type", "'seqpacket'");

  /* Check some error cases... */
  g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_CRITICAL, "*non-existent*");
  action = g_property_action_new ("foo", app, "xyz");
  g_test_assert_expected_messages ();
  xobject_unref (action);

  g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_CRITICAL, "*writable*");
  action = g_property_action_new ("foo", app, "is-registered");
  g_test_assert_expected_messages ();
  xobject_unref (action);

  g_test_expect_message ("GLib-GIO", G_LOG_LEVEL_CRITICAL, "*type 'xsocket_address_t'*");
  action = g_property_action_new ("foo", client, "local-address");
  g_test_assert_expected_messages ();
  xobject_unref (action);

  xobject_unref (group);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/actions/basic", test_basic);
  g_test_add_func ("/actions/name", test_name);
  g_test_add_func ("/actions/simplegroup", test_simple_group);
  g_test_add_func ("/actions/stateful", test_stateful);
  g_test_add_func ("/actions/default-activate", test_default_activate);
  g_test_add_func ("/actions/entries", test_entries);
  g_test_add_func ("/actions/parse-detailed", test_parse_detailed);
  g_test_add_func ("/actions/dbus/export", test_dbus_export);
  g_test_add_func ("/actions/dbus/threaded", test_dbus_threaded);
  g_test_add_func ("/actions/dbus/bug679509", test_bug679509);
  g_test_add_func ("/actions/property", test_property_actions);

  return g_test_run ();
}
