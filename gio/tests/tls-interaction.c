/* GLib testing framework examples and tests
 *
 * Copyright (C) 2011 Collabora Ltd.
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
 * Author: Stef Walter <stefw@collobora.co.uk>
 */

#include "config.h"

#include <gio/gio.h>

#include "gtesttlsbackend.h"

static xptr_array_t *fixtures = NULL;

typedef struct {
  /* Class virtual interaction methods */
  xpointer_t ask_password_func;
  xpointer_t ask_password_async_func;
  xpointer_t ask_password_finish_func;
  xpointer_t request_certificate_func;
  xpointer_t request_certificate_async_func;
  xpointer_t request_certificate_finish_func;

  /* Expected results */
  GTlsInteractionResult result;
  xquark error_domain;
  xint_t error_code;
  const xchar_t *error_message;
} Fixture;

typedef struct {
  xtls_interaction_t *interaction;
  xtls_password_t *password;
  xtls_connection_t *connection;
  xmain_loop_t *loop;
  xthread_t *interaction_thread;
  xthread_t *test_thread;
  xthread_t *loop_thread;
  const Fixture *fixture;
} Test;

typedef struct {
  xtls_interaction_t parent;
  Test *test;
} TestInteraction;

typedef struct {
  GTlsInteractionClass parent;
} TestInteractionClass;

static xtype_t test_interaction_get_type (void);
G_DEFINE_TYPE (TestInteraction, test_interaction, XTYPE_TLS_INTERACTION)

#define TEST_TYPE_INTERACTION         (test_interaction_get_type ())
#define TEST_INTERACTION(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), TEST_TYPE_INTERACTION, TestInteraction))
#define TEST_IS_INTERACTION(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), TEST_TYPE_INTERACTION))

static void
test_interaction_init (TestInteraction *self)
{

}

static void
test_interaction_class_init (TestInteractionClass *klass)
{
  /* By default no virtual methods */
}

static void
test_interaction_ask_password_async_success (xtls_interaction_t    *interaction,
                                             xtls_password_t       *password,
                                             xcancellable_t       *cancellable,
                                             xasync_ready_callback_t callback,
                                             xpointer_t            user_data)
{
  xtask_t *task;
  TestInteraction *self;

  g_assert (TEST_IS_INTERACTION (interaction));
  self = TEST_INTERACTION (interaction);

  g_assert (xthread_self () == self->test->interaction_thread);

  g_assert (X_IS_TLS_PASSWORD (password));
  g_assert (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  task = xtask_new (self, cancellable, callback, user_data);

  /* Don't do this in real life. Include a null terminator for testing */
  xtls_password_set_value (password, (const guchar *)"the password", 13);
  xtask_return_int (task, G_TLS_INTERACTION_HANDLED);
  xobject_unref (task);
}


static GTlsInteractionResult
test_interaction_ask_password_finish_success (xtls_interaction_t    *interaction,
                                              xasync_result_t       *result,
                                              xerror_t            **error)
{
  TestInteraction *self;

  g_assert (TEST_IS_INTERACTION (interaction));
  self = TEST_INTERACTION (interaction);

  g_assert (xthread_self () == self->test->interaction_thread);

  g_assert (xtask_is_valid (result, interaction));
  g_assert (error != NULL);
  g_assert (*error == NULL);

  return xtask_propagate_int (XTASK (result), error);
}

static void
test_interaction_ask_password_async_failure (xtls_interaction_t    *interaction,
                                             xtls_password_t       *password,
                                             xcancellable_t       *cancellable,
                                             xasync_ready_callback_t callback,
                                             xpointer_t            user_data)
{
  xtask_t *task;
  TestInteraction *self;

  g_assert (TEST_IS_INTERACTION (interaction));
  self = TEST_INTERACTION (interaction);

  g_assert (xthread_self () == self->test->interaction_thread);

  g_assert (X_IS_TLS_PASSWORD (password));
  g_assert (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  task = xtask_new (self, cancellable, callback, user_data);

  xtask_return_new_error (task, XFILE_ERROR, XFILE_ERROR_ACCES, "The message");
  xobject_unref (task);
}

static GTlsInteractionResult
test_interaction_ask_password_finish_failure (xtls_interaction_t    *interaction,
                                              xasync_result_t       *result,
                                              xerror_t            **error)
{
  TestInteraction *self;

  g_assert (TEST_IS_INTERACTION (interaction));
  self = TEST_INTERACTION (interaction);

  g_assert (xthread_self () == self->test->interaction_thread);

  g_assert (xtask_is_valid (result, interaction));
  g_assert (error != NULL);
  g_assert (*error == NULL);

  if (xtask_propagate_int (XTASK (result), error) != -1)
    g_assert_not_reached ();

  return G_TLS_INTERACTION_FAILED;
}


/* Return a copy of @str that is allocated in a silly way, to exercise
 * custom free-functions. The returned pointer points to a copy of @str
 * in a buffer of the form "BEFORE \0 str \0 AFTER". */
static guchar *
special_dup (const char *str)
{
  xstring_t *buf = xstring_new ("BEFORE");
  guchar *ret;

  xstring_append_c (buf, '\0');
  xstring_append (buf, str);
  xstring_append_c (buf, '\0');
  xstring_append (buf, "AFTER");
  ret = (guchar *) xstring_free (buf, FALSE);
  return ret + strlen ("BEFORE") + 1;
}


/* Free a copy of @str that was made with special_dup(), after asserting
 * that it has not been corrupted. */
static void
special_free (xpointer_t p)
{
  xchar_t *s = p;
  xchar_t *buf = s - strlen ("BEFORE") - 1;

  g_assert_cmpstr (buf, ==, "BEFORE");
  g_assert_cmpstr (s + strlen (s) + 1, ==, "AFTER");
  g_free (buf);
}


static GTlsInteractionResult
test_interaction_ask_password_sync_success (xtls_interaction_t    *interaction,
                                            xtls_password_t       *password,
                                            xcancellable_t       *cancellable,
                                            xerror_t            **error)
{
  TestInteraction *self;
  const guchar *value;
  xsize_t len;

  g_assert (TEST_IS_INTERACTION (interaction));
  self = TEST_INTERACTION (interaction);

  g_assert (xthread_self () == self->test->interaction_thread);

  g_assert (X_IS_TLS_PASSWORD (password));
  g_assert (cancellable == NULL || X_IS_CANCELLABLE (cancellable));
  g_assert (error != NULL);
  g_assert (*error == NULL);

  /* Exercise different ways to set the value */
  xtls_password_set_value (password, (const guchar *) "foo", 4);
  len = 0;
  value = xtls_password_get_value (password, &len);
  g_assert_cmpmem (value, len, "foo", 4);

  xtls_password_set_value (password, (const guchar *) "bar", -1);
  len = 0;
  value = xtls_password_get_value (password, &len);
  g_assert_cmpmem (value, len, "bar", 3);

  xtls_password_set_value_full (password, special_dup ("baa"), 4, special_free);
  len = 0;
  value = xtls_password_get_value (password, &len);
  g_assert_cmpmem (value, len, "baa", 4);

  xtls_password_set_value_full (password, special_dup ("baz"), -1, special_free);
  len = 0;
  value = xtls_password_get_value (password, &len);
  g_assert_cmpmem (value, len, "baz", 3);

  /* Don't do this in real life. Include a null terminator for testing */
  xtls_password_set_value (password, (const guchar *)"the password", 13);
  return G_TLS_INTERACTION_HANDLED;
}

static GTlsInteractionResult
test_interaction_ask_password_sync_failure (xtls_interaction_t    *interaction,
                                            xtls_password_t       *password,
                                            xcancellable_t       *cancellable,
                                            xerror_t            **error)
{
  TestInteraction *self;

  g_assert (TEST_IS_INTERACTION (interaction));
  self = TEST_INTERACTION (interaction);

  g_assert (xthread_self () == self->test->interaction_thread);

  g_assert (X_IS_TLS_PASSWORD (password));
  g_assert (cancellable == NULL || X_IS_CANCELLABLE (cancellable));
  g_assert (error != NULL);
  g_assert (*error == NULL);

  g_set_error (error, XFILE_ERROR, XFILE_ERROR_ACCES, "The message");
  return G_TLS_INTERACTION_FAILED;
}

static void
test_interaction_request_certificate_async_success (xtls_interaction_t    *interaction,
                                                    xtls_connection_t     *connection,
                                                    xint_t                unused_flags,
                                                    xcancellable_t       *cancellable,
                                                    xasync_ready_callback_t callback,
                                                    xpointer_t            user_data)
{
  xtask_t *task;
  TestInteraction *self;

  g_assert (TEST_IS_INTERACTION (interaction));
  self = TEST_INTERACTION (interaction);

  g_assert (xthread_self () == self->test->interaction_thread);

  g_assert (X_IS_TLS_CONNECTION (connection));
  g_assert (cancellable == NULL || X_IS_CANCELLABLE (cancellable));
  g_assert (unused_flags == 0);

  task = xtask_new (self, cancellable, callback, user_data);

  /*
   * IRL would call xtls_connection_set_certificate(). But here just touch
   * the connection in a detectable way.
   */
  xobject_set_data (G_OBJECT (connection), "chosen-certificate", "my-certificate");
  xtask_return_int (task, G_TLS_INTERACTION_HANDLED);
  xobject_unref (task);
}

static GTlsInteractionResult
test_interaction_request_certificate_finish_success (xtls_interaction_t    *interaction,
                                                     xasync_result_t       *result,
                                                     xerror_t            **error)
{
  TestInteraction *self;

  g_assert (TEST_IS_INTERACTION (interaction));
  self = TEST_INTERACTION (interaction);

  g_assert (xthread_self () == self->test->interaction_thread);

  g_assert (xtask_is_valid (result, interaction));
  g_assert (error != NULL);
  g_assert (*error == NULL);

  return xtask_propagate_int (XTASK (result), error);
}

static void
test_interaction_request_certificate_async_failure (xtls_interaction_t    *interaction,
                                                    xtls_connection_t     *connection,
                                                    xint_t                unused_flags,
                                                    xcancellable_t       *cancellable,
                                                    xasync_ready_callback_t callback,
                                                    xpointer_t            user_data)
{
  xtask_t *task;
  TestInteraction *self;

  g_assert (TEST_IS_INTERACTION (interaction));
  self = TEST_INTERACTION (interaction);

  g_assert (xthread_self () == self->test->interaction_thread);

  g_assert (X_IS_TLS_CONNECTION (connection));
  g_assert (cancellable == NULL || X_IS_CANCELLABLE (cancellable));
  g_assert (unused_flags == 0);

  task = xtask_new (self, cancellable, callback, user_data);

  xtask_return_new_error (task, XFILE_ERROR, XFILE_ERROR_NOENT, "Another message");
  xobject_unref (task);
}

static GTlsInteractionResult
test_interaction_request_certificate_finish_failure (xtls_interaction_t    *interaction,
                                                     xasync_result_t       *result,
                                                     xerror_t            **error)
{
  TestInteraction *self;

  g_assert (TEST_IS_INTERACTION (interaction));
  self = TEST_INTERACTION (interaction);

  g_assert (xthread_self () == self->test->interaction_thread);

  g_assert (xtask_is_valid (result, interaction));
  g_assert (error != NULL);
  g_assert (*error == NULL);

  if (xtask_propagate_int (XTASK (result), error) != -1)
    g_assert_not_reached ();

  return G_TLS_INTERACTION_FAILED;
}

static GTlsInteractionResult
test_interaction_request_certificate_sync_success (xtls_interaction_t    *interaction,
                                                   xtls_connection_t      *connection,
                                                   xint_t                 unused_flags,
                                                   xcancellable_t        *cancellable,
                                                   xerror_t             **error)
{
  TestInteraction *self;

  g_assert (TEST_IS_INTERACTION (interaction));
  self = TEST_INTERACTION (interaction);

  g_assert (xthread_self () == self->test->interaction_thread);

  g_assert (X_IS_TLS_CONNECTION (connection));
  g_assert (cancellable == NULL || X_IS_CANCELLABLE (cancellable));
  g_assert (error != NULL);
  g_assert (*error == NULL);

  /*
   * IRL would call xtls_connection_set_certificate(). But here just touch
   * the connection in a detectable way.
   */
  xobject_set_data (G_OBJECT (connection), "chosen-certificate", "my-certificate");
  return G_TLS_INTERACTION_HANDLED;
}

static GTlsInteractionResult
test_interaction_request_certificate_sync_failure (xtls_interaction_t    *interaction,
                                                   xtls_connection_t     *connection,
                                                   xint_t                unused_flags,
                                                   xcancellable_t       *cancellable,
                                                   xerror_t            **error)
{
  TestInteraction *self;

  g_assert (TEST_IS_INTERACTION (interaction));
  self = TEST_INTERACTION (interaction);

  g_assert (xthread_self () == self->test->interaction_thread);

  g_assert (X_IS_TLS_CONNECTION (connection));
  g_assert (cancellable == NULL || X_IS_CANCELLABLE (cancellable));
  g_assert (unused_flags == 0);
  g_assert (error != NULL);
  g_assert (*error == NULL);

  g_set_error (error, XFILE_ERROR, XFILE_ERROR_NOENT, "Another message");
  return G_TLS_INTERACTION_FAILED;
}

/* ----------------------------------------------------------------------------
 * ACTUAL TESTS
 */

static void
on_ask_password_async_call (xobject_t      *source,
                            xasync_result_t *result,
                            xpointer_t      user_data)
{
  Test *test = user_data;
  GTlsInteractionResult res;
  xerror_t *error = NULL;

  g_assert (X_IS_TLS_INTERACTION (source));
  g_assert (G_TLS_INTERACTION (source) == test->interaction);

  /* Check that this callback is being run in the right place */
  g_assert (xthread_self () == test->interaction_thread);

  res = xtls_interaction_ask_password_finish (test->interaction, result,
                                               &error);

  /* Check that the results match the fixture */
  g_assert_cmpuint (test->fixture->result, ==, res);
  switch (test->fixture->result)
    {
      case G_TLS_INTERACTION_HANDLED:
        g_assert_no_error (error);
        g_assert_cmpstr ((const xchar_t *)xtls_password_get_value (test->password, NULL), ==, "the password");
        break;
      case G_TLS_INTERACTION_FAILED:
        g_assert_error (error, test->fixture->error_domain, test->fixture->error_code);
        g_assert_cmpstr (error->message, ==, test->fixture->error_message);
        g_clear_error (&error);
        break;
      case G_TLS_INTERACTION_UNHANDLED:
        g_assert_no_error (error);
        break;
      default:
        g_assert_not_reached ();
    }

  /* Signal the end of the test */
  xmain_loop_quit (test->loop);
}

static void
test_ask_password_async (Test            *test,
                         xconstpointer    unused)
{
  /* This test only works with a main loop */
  g_assert (test->loop);

  xtls_interaction_ask_password_async (test->interaction,
                                        test->password, NULL,
                                        on_ask_password_async_call,
                                        test);

  /* teardown waits until xmain_loop_quit(). called from callback */
}

static void
test_invoke_ask_password (Test         *test,
                          xconstpointer unused)
{
  GTlsInteractionResult res;
  xerror_t *error = NULL;

  res = xtls_interaction_invoke_ask_password (test->interaction, test->password,
                                               NULL, &error);

  /* Check that the results match the fixture */
  g_assert_cmpuint (test->fixture->result, ==, res);
  switch (test->fixture->result)
    {
      case G_TLS_INTERACTION_HANDLED:
        g_assert_no_error (error);
        g_assert_cmpstr ((const xchar_t *)xtls_password_get_value (test->password, NULL), ==, "the password");
        break;
      case G_TLS_INTERACTION_FAILED:
        g_assert_error (error, test->fixture->error_domain, test->fixture->error_code);
        g_assert_cmpstr (error->message, ==, test->fixture->error_message);
        g_clear_error (&error);
        break;
      case G_TLS_INTERACTION_UNHANDLED:
        g_assert_no_error (error);
        break;
      default:
        g_assert_not_reached ();
    }

  /* This allows teardown to stop if running with loop */
  if (test->loop)
    xmain_loop_quit (test->loop);
}

static void
test_ask_password (Test         *test,
                   xconstpointer unused)
{
  GTlsInteractionResult res;
  xerror_t *error = NULL;

  res = xtls_interaction_ask_password (test->interaction, test->password,
                                        NULL, &error);

  /* Check that the results match the fixture */
  g_assert_cmpuint (test->fixture->result, ==, res);
  switch (test->fixture->result)
    {
      case G_TLS_INTERACTION_HANDLED:
        g_assert_no_error (error);
        g_assert_cmpstr ((const xchar_t *)xtls_password_get_value (test->password, NULL), ==, "the password");
        break;
      case G_TLS_INTERACTION_FAILED:
        g_assert_error (error, test->fixture->error_domain, test->fixture->error_code);
        g_assert_cmpstr (error->message, ==, test->fixture->error_message);
        g_clear_error (&error);
        break;
      case G_TLS_INTERACTION_UNHANDLED:
        g_assert_no_error (error);
        break;
      default:
        g_assert_not_reached ();
    }

  /* This allows teardown to stop if running with loop */
  if (test->loop)
    xmain_loop_quit (test->loop);
}

static void
on_request_certificate_async_call (xobject_t      *source,
                                   xasync_result_t *result,
                                   xpointer_t      user_data)
{
  Test *test = user_data;
  GTlsInteractionResult res;
  xerror_t *error = NULL;

  g_assert (X_IS_TLS_INTERACTION (source));
  g_assert (G_TLS_INTERACTION (source) == test->interaction);

  /* Check that this callback is being run in the right place */
  g_assert (xthread_self () == test->interaction_thread);

  res = xtls_interaction_request_certificate_finish (test->interaction, result, &error);

  /* Check that the results match the fixture */
  g_assert_cmpuint (test->fixture->result, ==, res);
  switch (test->fixture->result)
    {
      case G_TLS_INTERACTION_HANDLED:
        g_assert_no_error (error);
        g_assert_cmpstr (xobject_get_data (G_OBJECT (test->connection), "chosen-certificate"), ==, "my-certificate");
        break;
      case G_TLS_INTERACTION_FAILED:
        g_assert_error (error, test->fixture->error_domain, test->fixture->error_code);
        g_assert_cmpstr (error->message, ==, test->fixture->error_message);
        g_clear_error (&error);
        break;
      case G_TLS_INTERACTION_UNHANDLED:
        g_assert_no_error (error);
        break;
      default:
        g_assert_not_reached ();
    }

  /* Signal the end of the test */
  xmain_loop_quit (test->loop);
}

static void
test_request_certificate_async (Test            *test,
                                xconstpointer    unused)
{
  /* This test only works with a main loop */
  g_assert (test->loop);

  xtls_interaction_request_certificate_async (test->interaction,
                                               test->connection, 0, NULL,
                                               on_request_certificate_async_call,
                                               test);

  /* teardown waits until xmain_loop_quit(). called from callback */
}

static void
test_invoke_request_certificate (Test         *test,
                                 xconstpointer unused)
{
  GTlsInteractionResult res;
  xerror_t *error = NULL;

  res = xtls_interaction_invoke_request_certificate (test->interaction,
                                                      test->connection,
                                                      0, NULL, &error);

  /* Check that the results match the fixture */
  g_assert_cmpuint (test->fixture->result, ==, res);
  switch (test->fixture->result)
    {
      case G_TLS_INTERACTION_HANDLED:
        g_assert_no_error (error);
        g_assert_cmpstr (xobject_get_data (G_OBJECT (test->connection), "chosen-certificate"), ==, "my-certificate");
        break;
      case G_TLS_INTERACTION_FAILED:
        g_assert_error (error, test->fixture->error_domain, test->fixture->error_code);
        g_assert_cmpstr (error->message, ==, test->fixture->error_message);
        g_clear_error (&error);
        break;
      case G_TLS_INTERACTION_UNHANDLED:
        g_assert_no_error (error);
        break;
      default:
        g_assert_not_reached ();
    }

  /* This allows teardown to stop if running with loop */
  if (test->loop)
    xmain_loop_quit (test->loop);
}

static void
test_request_certificate (Test         *test,
                          xconstpointer unused)
{
  GTlsInteractionResult res;
  xerror_t *error = NULL;

  res = xtls_interaction_request_certificate (test->interaction, test->connection,
                                               0, NULL, &error);

  /* Check that the results match the fixture */
  g_assert_cmpuint (test->fixture->result, ==, res);
  switch (test->fixture->result)
    {
      case G_TLS_INTERACTION_HANDLED:
        g_assert_no_error (error);
        g_assert_cmpstr (xobject_get_data (G_OBJECT (test->connection), "chosen-certificate"), ==, "my-certificate");
        break;
      case G_TLS_INTERACTION_FAILED:
        g_assert_error (error, test->fixture->error_domain, test->fixture->error_code);
        g_assert_cmpstr (error->message, ==, test->fixture->error_message);
        g_clear_error (&error);
        break;
      case G_TLS_INTERACTION_UNHANDLED:
        g_assert_no_error (error);
        break;
      default:
        g_assert_not_reached ();
    }

  /* This allows teardown to stop if running with loop */
  if (test->loop)
    xmain_loop_quit (test->loop);
}

/* ----------------------------------------------------------------------------
 * TEST SETUP
 */

static void
setup_without_loop (Test           *test,
                    xconstpointer   user_data)
{
  const Fixture *fixture = user_data;
  GTlsInteractionClass *klass;
  xtls_backend_t *backend;
  xerror_t *error = NULL;

  test->fixture = fixture;

  test->interaction = xobject_new (TEST_TYPE_INTERACTION, NULL);
  g_assert (TEST_IS_INTERACTION (test->interaction));

  TEST_INTERACTION (test->interaction)->test = test;

  klass =  G_TLS_INTERACTION_GET_CLASS (test->interaction);
  klass->ask_password = fixture->ask_password_func;
  klass->ask_password_async = fixture->ask_password_async_func;
  klass->ask_password_finish = fixture->ask_password_finish_func;
  klass->request_certificate = fixture->request_certificate_func;
  klass->request_certificate_async = fixture->request_certificate_async_func;
  klass->request_certificate_finish = fixture->request_certificate_finish_func;

  backend = xobject_new (XTYPE_TEST_TLS_BACKEND, NULL);
  test->connection = xobject_new (xtls_backend_get_server_connection_type (backend), NULL);
  g_assert_no_error (error);
  xobject_unref (backend);

  test->password = xtls_password_new (0, "Description");
  test->test_thread = xthread_self ();

  /*
   * If no loop is running then interaction should happen in the same
   * thread that the tests are running in.
   */
  test->interaction_thread = test->test_thread;
}

static void
teardown_without_loop (Test            *test,
                       xconstpointer    unused)
{
  xobject_unref (test->connection);
  xobject_unref (test->password);

  g_assert_finalize_object (test->interaction);
}

typedef struct {
  xmutex_t loop_mutex;
  xcond_t loop_started;
  xboolean_t started;
  Test *test;
} ThreadLoop;

static xpointer_t
thread_loop (xpointer_t user_data)
{
  xmain_context_t *context = xmain_context_default ();
  ThreadLoop *closure = user_data;
  Test *test = closure->test;

  g_mutex_lock (&closure->loop_mutex);

  g_assert (test->loop_thread == xthread_self ());
  g_assert (test->loop == NULL);
  test->loop = xmain_loop_new (context, TRUE);

  xmain_context_acquire (context);
  closure->started = TRUE;
  g_cond_signal (&closure->loop_started);
  g_mutex_unlock (&closure->loop_mutex);

  while (xmain_loop_is_running (test->loop))
    xmain_context_iteration (context, TRUE);

  xmain_context_release (context);
  return test;
}

static void
setup_with_thread_loop (Test            *test,
                        xconstpointer    user_data)
{
  ThreadLoop closure;

  setup_without_loop (test, user_data);

  g_mutex_init (&closure.loop_mutex);
  g_cond_init (&closure.loop_started);
  closure.started = FALSE;
  closure.test = test;

  g_mutex_lock (&closure.loop_mutex);
  test->loop_thread = xthread_new ("loop", thread_loop, &closure);
  while (!closure.started)
    g_cond_wait (&closure.loop_started, &closure.loop_mutex);
  g_mutex_unlock (&closure.loop_mutex);

  /*
   * When a loop is running then interaction should always occur in the main
   * context of that loop.
   */
  test->interaction_thread = test->loop_thread;

  g_mutex_clear (&closure.loop_mutex);
  g_cond_clear (&closure.loop_started);
}

static void
teardown_with_thread_loop (Test            *test,
                           xconstpointer    unused)
{
  xpointer_t check;

  g_assert (test->loop_thread);
  check = xthread_join (test->loop_thread);
  g_assert (check == test);
  test->loop_thread = NULL;

  xmain_loop_unref (test->loop);

  teardown_without_loop (test, unused);
}

static void
setup_with_normal_loop (Test            *test,
                        xconstpointer    user_data)
{
  xmain_context_t *context;

  setup_without_loop (test, user_data);

  context = xmain_context_default ();
  if (!xmain_context_acquire (context))
    g_assert_not_reached ();

  test->loop = xmain_loop_new (context, TRUE);
  g_assert (xmain_loop_is_running (test->loop));
}

static void
teardown_with_normal_loop (Test            *test,
                           xconstpointer    unused)
{
  xmain_context_t *context;

  context = xmain_context_default ();
  while (xmain_loop_is_running (test->loop))
    xmain_context_iteration (context, TRUE);

  xmain_context_release (context);

  /* Run test until complete */
  xmain_loop_unref (test->loop);
  test->loop = NULL;

  teardown_without_loop (test, unused);
}

typedef void (*TestFunc) (Test *test, xconstpointer data);

static void
test_with_async_ask_password (const xchar_t *name,
                              TestFunc     setup,
                              TestFunc     func,
                              TestFunc     teardown)
{
  xchar_t *test_name;
  Fixture *fixture;

  /* Async implementation that succeeds */
  fixture = g_new0 (Fixture, 1);
  fixture->ask_password_async_func = test_interaction_ask_password_async_success;
  fixture->ask_password_finish_func = test_interaction_ask_password_finish_success;
  fixture->ask_password_func = NULL;
  fixture->result = G_TLS_INTERACTION_HANDLED;
  test_name = xstrdup_printf ("%s/async-implementation-success", name);
  g_test_add (test_name, Test, fixture, setup, func, teardown);
  g_free (test_name);
  xptr_array_add (fixtures, fixture);

  /* Async implementation that fails */
  fixture = g_new0 (Fixture, 1);
  fixture->ask_password_async_func = test_interaction_ask_password_async_failure;
  fixture->ask_password_finish_func = test_interaction_ask_password_finish_failure;
  fixture->ask_password_func = NULL;
  fixture->result = G_TLS_INTERACTION_FAILED;
  fixture->error_domain = XFILE_ERROR;
  fixture->error_code = XFILE_ERROR_ACCES;
  fixture->error_message = "The message";
  test_name = xstrdup_printf ("%s/async-implementation-failure", name);
  g_test_add (test_name, Test, fixture, setup, func, teardown);
  g_free (test_name);
  xptr_array_add (fixtures, fixture);
}

static void
test_with_unhandled_ask_password (const xchar_t *name,
                                  TestFunc     setup,
                                  TestFunc     func,
                                  TestFunc     teardown)
{
  xchar_t *test_name;
  Fixture *fixture;

  /* Unhandled implementation */
  fixture = g_new0 (Fixture, 1);
  fixture->ask_password_async_func = NULL;
  fixture->ask_password_finish_func = NULL;
  fixture->ask_password_func = NULL;
  fixture->result = G_TLS_INTERACTION_UNHANDLED;
  test_name = xstrdup_printf ("%s/unhandled-implementation", name);
  g_test_add (test_name, Test, fixture, setup, func, teardown);
  g_free (test_name);
  xptr_array_add (fixtures, fixture);
}

static void
test_with_sync_ask_password (const xchar_t *name,
                                             TestFunc     setup,
                                             TestFunc     func,
                                             TestFunc     teardown)
{
  xchar_t *test_name;
  Fixture *fixture;

  /* Sync implementation that succeeds */
  fixture = g_new0 (Fixture, 1);
  fixture->ask_password_async_func = NULL;
  fixture->ask_password_finish_func = NULL;
  fixture->ask_password_func = test_interaction_ask_password_sync_success;
  fixture->result = G_TLS_INTERACTION_HANDLED;
  test_name = xstrdup_printf ("%s/sync-implementation-success", name);
  g_test_add (test_name, Test, fixture, setup, func, teardown);
  g_free (test_name);
  xptr_array_add (fixtures, fixture);

  /* Async implementation that fails */
  fixture = g_new0 (Fixture, 1);
  fixture->ask_password_async_func = NULL;
  fixture->ask_password_finish_func = NULL;
  fixture->ask_password_func = test_interaction_ask_password_sync_failure;
  fixture->result = G_TLS_INTERACTION_FAILED;
  fixture->error_domain = XFILE_ERROR;
  fixture->error_code = XFILE_ERROR_ACCES;
  fixture->error_message = "The message";
  test_name = xstrdup_printf ("%s/sync-implementation-failure", name);
  g_test_add (test_name, Test, fixture, setup, func, teardown);
  g_free (test_name);
  xptr_array_add (fixtures, fixture);
}

static void
test_with_all_ask_password (const xchar_t *name,
                            TestFunc setup,
                            TestFunc func,
                            TestFunc teardown)
{
  test_with_unhandled_ask_password (name, setup, func, teardown);
  test_with_async_ask_password (name, setup, func, teardown);
  test_with_sync_ask_password (name, setup, func, teardown);
}

static void
test_with_async_request_certificate (const xchar_t *name,
                                     TestFunc     setup,
                                     TestFunc     func,
                                     TestFunc     teardown)
{
  xchar_t *test_name;
  Fixture *fixture;

  /* Async implementation that succeeds */
  fixture = g_new0 (Fixture, 1);
  fixture->request_certificate_async_func = test_interaction_request_certificate_async_success;
  fixture->request_certificate_finish_func = test_interaction_request_certificate_finish_success;
  fixture->request_certificate_func = NULL;
  fixture->result = G_TLS_INTERACTION_HANDLED;
  test_name = xstrdup_printf ("%s/async-implementation-success", name);
  g_test_add (test_name, Test, fixture, setup, func, teardown);
  g_free (test_name);
  xptr_array_add (fixtures, fixture);

  /* Async implementation that fails */
  fixture = g_new0 (Fixture, 1);
  fixture->request_certificate_async_func = test_interaction_request_certificate_async_failure;
  fixture->request_certificate_finish_func = test_interaction_request_certificate_finish_failure;
  fixture->request_certificate_func = NULL;
  fixture->result = G_TLS_INTERACTION_FAILED;
  fixture->error_domain = XFILE_ERROR;
  fixture->error_code = XFILE_ERROR_NOENT;
  fixture->error_message = "Another message";
  test_name = xstrdup_printf ("%s/async-implementation-failure", name);
  g_test_add (test_name, Test, fixture, setup, func, teardown);
  g_free (test_name);
  xptr_array_add (fixtures, fixture);
}

static void
test_with_unhandled_request_certificate (const xchar_t *name,
                                         TestFunc     setup,
                                         TestFunc     func,
                                         TestFunc     teardown)
{
  xchar_t *test_name;
  Fixture *fixture;

  /* Unhandled implementation */
  fixture = g_new0 (Fixture, 1);
  fixture->request_certificate_async_func = NULL;
  fixture->request_certificate_finish_func = NULL;
  fixture->request_certificate_func = NULL;
  fixture->result = G_TLS_INTERACTION_UNHANDLED;
  test_name = xstrdup_printf ("%s/unhandled-implementation", name);
  g_test_add (test_name, Test, fixture, setup, func, teardown);
  g_free (test_name);
  xptr_array_add (fixtures, fixture);
}

static void
test_with_sync_request_certificate (const xchar_t *name,
                                    TestFunc     setup,
                                    TestFunc     func,
                                    TestFunc     teardown)
{
  xchar_t *test_name;
  Fixture *fixture;

  /* Sync implementation that succeeds */
  fixture = g_new0 (Fixture, 1);
  fixture->request_certificate_async_func = NULL;
  fixture->request_certificate_finish_func = NULL;
  fixture->request_certificate_func = test_interaction_request_certificate_sync_success;
  fixture->result = G_TLS_INTERACTION_HANDLED;
  test_name = xstrdup_printf ("%s/sync-implementation-success", name);
  g_test_add (test_name, Test, fixture, setup, func, teardown);
  g_free (test_name);
  xptr_array_add (fixtures, fixture);

  /* Async implementation that fails */
  fixture = g_new0 (Fixture, 1);
  fixture->request_certificate_async_func = NULL;
  fixture->request_certificate_finish_func = NULL;
  fixture->request_certificate_func = test_interaction_request_certificate_sync_failure;
  fixture->result = G_TLS_INTERACTION_FAILED;
  fixture->error_domain = XFILE_ERROR;
  fixture->error_code = XFILE_ERROR_NOENT;
  fixture->error_message = "Another message";
  test_name = xstrdup_printf ("%s/sync-implementation-failure", name);
  g_test_add (test_name, Test, fixture, setup, func, teardown);
  g_free (test_name);
  xptr_array_add (fixtures, fixture);
}

static void
test_with_all_request_certificate (const xchar_t *name,
                                   TestFunc setup,
                                   TestFunc func,
                                   TestFunc teardown)
{
  test_with_unhandled_request_certificate (name, setup, func, teardown);
  test_with_async_request_certificate (name, setup, func, teardown);
  test_with_sync_request_certificate (name, setup, func, teardown);
}
int
main (int   argc,
      char *argv[])
{
  xint_t ret;

  g_test_init (&argc, &argv, NULL);

  fixtures = xptr_array_new_with_free_func (g_free);

  /* Tests for xtls_interaction_invoke_ask_password */
  test_with_all_ask_password ("/tls-interaction/ask-password/invoke-with-loop",
                              setup_with_thread_loop, test_invoke_ask_password, teardown_with_thread_loop);
  test_with_all_ask_password ("/tls-interaction/ask-password/invoke-without-loop",
                              setup_without_loop, test_invoke_ask_password, teardown_without_loop);
  test_with_all_ask_password ("/tls-interaction/ask-password/invoke-in-loop",
                                              setup_with_normal_loop, test_invoke_ask_password, teardown_with_normal_loop);

  /* Tests for xtls_interaction_ask_password */
  test_with_unhandled_ask_password ("/tls-interaction/ask-password/sync",
                                    setup_without_loop, test_ask_password, teardown_without_loop);
  test_with_sync_ask_password ("/tls-interaction/ask-password/sync",
                               setup_without_loop, test_ask_password, teardown_without_loop);

  /* Tests for xtls_interaction_ask_password_async */
  test_with_unhandled_ask_password ("/tls-interaction/ask-password/async",
                                    setup_with_normal_loop, test_ask_password_async, teardown_with_normal_loop);
  test_with_async_ask_password ("/tls-interaction/ask-password/async",
                                setup_with_normal_loop, test_ask_password_async, teardown_with_normal_loop);

  /* Tests for xtls_interaction_invoke_request_certificate */
  test_with_all_request_certificate ("/tls-interaction/request-certificate/invoke-with-loop",
                                     setup_with_thread_loop, test_invoke_request_certificate, teardown_with_thread_loop);
  test_with_all_request_certificate ("/tls-interaction/request-certificate/invoke-without-loop",
                                     setup_without_loop, test_invoke_request_certificate, teardown_without_loop);
  test_with_all_request_certificate ("/tls-interaction/request-certificate/invoke-in-loop",
                              setup_with_normal_loop, test_invoke_request_certificate, teardown_with_normal_loop);

  /* Tests for xtls_interaction_ask_password */
  test_with_unhandled_request_certificate ("/tls-interaction/request-certificate/sync",
                                           setup_without_loop, test_request_certificate, teardown_without_loop);
  test_with_sync_request_certificate ("/tls-interaction/request-certificate/sync",
                                      setup_without_loop, test_request_certificate, teardown_without_loop);

  /* Tests for xtls_interaction_ask_password_async */
  test_with_unhandled_request_certificate ("/tls-interaction/request-certificate/async",
                                           setup_with_normal_loop, test_request_certificate_async, teardown_with_normal_loop);
  test_with_async_request_certificate ("/tls-interaction/request-certificate/async",
                                       setup_with_normal_loop, test_request_certificate_async, teardown_with_normal_loop);

  ret = g_test_run();
  xptr_array_free (fixtures, TRUE);
  return ret;
}
