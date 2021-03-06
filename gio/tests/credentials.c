/* GLib testing framework examples and tests
 *
 * Copyright © 2012 Collabora Ltd.
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

#include "config.h"

#include <gio/gio.h>
#include <gio/gcredentialsprivate.h>

#ifdef G_OS_WIN32

static void
test_basic (void)
{
  xcredentials_t *creds = xcredentials_new ();
  xchar_t *stringified;
  DWORD *pid;

  stringified = xcredentials_to_string (creds);
  g_test_message ("%s", stringified);
  g_free (stringified);

  pid = xcredentials_get_native (creds,
                                  G_CREDENTIALS_TYPE_WIN32_PID);
  g_assert_cmpuint (*pid, ==, GetCurrentProcessId ());

  xobject_unref (creds);
}

#else

static void
test_basic (void)
{
  xcredentials_t *creds = xcredentials_new ();
  xcredentials_t *other = xcredentials_new ();
  xpointer_t bad_native_creds;
#if G_CREDENTIALS_SUPPORTED
  xerror_t *error = NULL;
  xboolean_t set;
  pid_t not_me;
  xchar_t *stringified;
#endif

  /* You can always get a credentials object, but it might not work. */
  xassert (creds != NULL);
  xassert (other != NULL);

#if G_CREDENTIALS_SUPPORTED
  xassert (xcredentials_is_same_user (creds, other, &error));
  g_assert_no_error (error);

  if (geteuid () == 0)
    not_me = 65534; /* traditionally 'nobody' */
  else
    not_me = 0;

  g_assert_cmpuint (xcredentials_get_unix_user (creds, &error), ==,
      geteuid ());
  g_assert_no_error (error);

#if G_CREDENTIALS_HAS_PID
  g_assert_cmpint (xcredentials_get_unix_pid (creds, &error), ==,
      getpid ());
  g_assert_no_error (error);
#else
  g_assert_cmpint (xcredentials_get_unix_pid (creds, &error), ==, -1);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);
#endif

  set = xcredentials_set_unix_user (other, not_me, &error);
#if G_CREDENTIALS_SPOOFING_SUPPORTED
  g_assert_no_error (error);
  xassert (set);

  g_assert_cmpuint (xcredentials_get_unix_user (other, &error), ==, not_me);
  xassert (!xcredentials_is_same_user (creds, other, &error));
  g_assert_no_error (error);
#else
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED);
  xassert (!set);
  g_clear_error (&error);

  g_assert_cmpuint (xcredentials_get_unix_user (other, &error), ==, geteuid ());
  xassert (xcredentials_is_same_user (creds, other, &error));
  g_assert_no_error (error);
#endif

  stringified = xcredentials_to_string (creds);
  g_test_message ("%s", stringified);
  g_free (stringified);

  stringified = xcredentials_to_string (other);
  g_test_message ("%s", stringified);
  g_free (stringified);

#if G_CREDENTIALS_USE_LINUX_UCRED
        {
          struct ucred *native = xcredentials_get_native (creds,
              G_CREDENTIALS_TYPE_LINUX_UCRED);

          g_assert_cmpuint (native->uid, ==, geteuid ());
          g_assert_cmpuint (native->pid, ==, getpid ());
        }
#elif G_CREDENTIALS_USE_APPLE_XUCRED
        {
          struct xucred *native = xcredentials_get_native (creds,
              G_CREDENTIALS_TYPE_APPLE_XUCRED);

          g_assert_cmpuint (native->cr_version, ==, XUCRED_VERSION);
          g_assert_cmpuint (native->cr_uid, ==, geteuid ());
        }
#elif G_CREDENTIALS_USE_FREEBSD_CMSGCRED
        {
          struct cmsgcred *native = xcredentials_get_native (creds,
              G_CREDENTIALS_TYPE_FREEBSD_CMSGCRED);

          g_assert_cmpuint (native->cmcred_euid, ==, geteuid ());
          g_assert_cmpuint (native->cmcred_pid, ==, getpid ());
        }
#elif G_CREDENTIALS_USE_NETBSD_UNPCBID
        {
          struct unpcbid *native = xcredentials_get_native (creds,
              G_CREDENTIALS_TYPE_NETBSD_UNPCBID);

          g_assert_cmpuint (native->unp_euid, ==, geteuid ());
          g_assert_cmpuint (native->unp_pid, ==, getpid ());
        }
#elif G_CREDENTIALS_USE_OPENBSD_SOCKPEERCRED
        {
          struct sockpeercred *native = xcredentials_get_native (creds,
              G_CREDENTIALS_TYPE_OPENBSD_SOCKPEERCRED);

          g_assert_cmpuint (native->uid, ==, geteuid ());
          g_assert_cmpuint (native->pid, ==, getpid ());
        }
#elif G_CREDENTIALS_USE_SOLARIS_UCRED
        {
          ucred_t *native = xcredentials_get_native (creds,
              G_CREDENTIALS_TYPE_SOLARIS_UCRED);

          g_assert_cmpuint (ucred_geteuid (native), ==, geteuid ());
          g_assert_cmpuint (ucred_getpid (native), ==, getpid ());
        }
#else
#error "G_CREDENTIALS_SUPPORTED is set but there is no test for this platform"
#endif


#if G_CREDENTIALS_USE_LINUX_UCRED
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*xcredentials_get_native: Trying to get*"
                         "G_CREDENTIALS_TYPE_FREEBSD_CMSGCRED "
                         "but only G_CREDENTIALS_TYPE_LINUX_UCRED*"
                         "supported*");
  bad_native_creds = xcredentials_get_native (creds, G_CREDENTIALS_TYPE_FREEBSD_CMSGCRED);
  g_test_assert_expected_messages ();
  g_assert_null (bad_native_creds);
#else
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*xcredentials_get_native: Trying to get*"
                         "G_CREDENTIALS_TYPE_LINUX_UCRED "
                         "but only G_CREDENTIALS_TYPE_*supported*");
  bad_native_creds = xcredentials_get_native (creds, G_CREDENTIALS_TYPE_LINUX_UCRED);
  g_test_assert_expected_messages ();
  g_assert_null (bad_native_creds);
#endif

#else /* ! G_CREDENTIALS_SUPPORTED */

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*xcredentials_get_native: Trying to get "
                         "credentials *but*no support*");
  bad_native_creds = xcredentials_get_native (creds, G_CREDENTIALS_TYPE_LINUX_UCRED);
  g_test_assert_expected_messages ();
  g_assert_null (bad_native_creds);
#endif

  xobject_unref (creds);
  xobject_unref (other);
}

#endif /* !G_OS_WIN32 */

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/credentials/basic", test_basic);

  return g_test_run();
}
