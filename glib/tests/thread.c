/* Unit tests for xthread_t
 * Copyright (C) 2011 Red Hat, Inc
 * Author: Matthias Clasen
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

#include <config.h>
#include <errno.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#include <glib.h>

#include "glib/glib-private.h"

#ifdef G_OS_UNIX
#include <unistd.h>
#include <sys/resource.h>
#endif

#ifdef THREADS_POSIX
#include <pthread.h>
#endif

static xpointer_t
thread1_func (xpointer_t data)
{
  xthread_exit (GINT_TO_POINTER (1));

  g_assert_not_reached ();

  return NULL;
}

/* test that xthread_exit() works */
static void
test_thread1 (void)
{
  xpointer_t result;
  xthread_t *thread;
  xerror_t *error = NULL;

  thread = xthread_try_new ("test", thread1_func, NULL, &error);
  g_assert_no_error (error);

  result = xthread_join (thread);

  g_assert_cmpint (GPOINTER_TO_INT (result), ==, 1);
}

static xpointer_t
thread2_func (xpointer_t data)
{
  return xthread_self ();
}

/* test that xthread_self() works */
static void
test_thread2 (void)
{
  xpointer_t result;
  xthread_t *thread;

  thread = xthread_new ("test", thread2_func, NULL);

  xassert (xthread_self () != thread);

  result = xthread_join (thread);

  xassert (result == thread);
}

static xpointer_t
thread3_func (xpointer_t data)
{
  xthread_t *peer = data;
  xint_t retval;

  retval = 3;

  if (peer)
    {
      xpointer_t result;

      result = xthread_join (peer);

      retval += GPOINTER_TO_INT (result);
    }

  return GINT_TO_POINTER (retval);
}

/* test that xthread_join() works across peers */
static void
test_thread3 (void)
{
  xpointer_t result;
  xthread_t *thread1, *thread2, *thread3;

  thread1 = xthread_new ("a", thread3_func, NULL);
  thread2 = xthread_new ("b", thread3_func, thread1);
  thread3 = xthread_new ("c", thread3_func, thread2);

  result = xthread_join (thread3);

  g_assert_cmpint (GPOINTER_TO_INT(result), ==, 9);
}

/* test that thread creation fails as expected,
 * by setting RLIMIT_NPROC ridiculously low
 */
static void
test_thread4 (void)
{
#ifdef _XPL_ADDRESS_SANITIZER
  g_test_incomplete ("FIXME: Leaks a GSystemThread's name, see glib#2308");
#elif defined(HAVE_PRLIMIT)
  struct rlimit ol, nl;
  xthread_t *thread;
  xerror_t *error;
  xint_t ret;

  getrlimit (RLIMIT_NPROC, &nl);
  nl.rlim_cur = 1;

  if ((ret = prlimit (getpid (), RLIMIT_NPROC, &nl, &ol)) != 0)
    xerror ("prlimit failed: %s", xstrerror (errno));

  error = NULL;
  thread = xthread_try_new ("a", thread1_func, NULL, &error);

  if (thread != NULL)
    {
      xpointer_t result;

      /* Privileged processes might be able to create new threads even
       * though the rlimit is too low. There isn't much we can do about
       * this; we just can't test this failure mode in this situation. */
      g_test_skip ("Unable to test xthread_try_new() failing with EAGAIN "
                   "while privileged (CAP_SYS_RESOURCE, CAP_SYS_ADMIN or "
                   "euid 0?)");
      result = xthread_join (thread);
      g_assert_cmpint (GPOINTER_TO_INT (result), ==, 1);
    }
  else
    {
      xassert (thread == NULL);
      g_assert_error (error, G_THREAD_ERROR, G_THREAD_ERROR_AGAIN);
      xerror_free (error);
    }

  if ((ret = prlimit (getpid (), RLIMIT_NPROC, &ol, NULL)) != 0)
    xerror ("resetting RLIMIT_NPROC failed: %s", xstrerror (errno));
#endif
}

static void
test_thread5 (void)
{
  xthread_t *thread;

  thread = xthread_new ("a", thread3_func, NULL);
  xthread_ref (thread);
  xthread_join (thread);
  xthread_unref (thread);
}

static xpointer_t
thread6_func (xpointer_t data)
{
#if defined (HAVE_PTHREAD_SETNAME_NP_WITH_TID) && defined (HAVE_PTHREAD_GETNAME_NP)
  char name[16];

  pthread_getname_np (pthread_self(), name, 16);

  g_assert_cmpstr (name, ==, data);
#endif

  return NULL;
}

static void
test_thread6 (void)
{
  xthread_t *thread;

  thread = xthread_new ("abc", thread6_func, "abc");
  xthread_join (thread);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/thread/thread1", test_thread1);
  g_test_add_func ("/thread/thread2", test_thread2);
  g_test_add_func ("/thread/thread3", test_thread3);
  g_test_add_func ("/thread/thread4", test_thread4);
  g_test_add_func ("/thread/thread5", test_thread5);
  g_test_add_func ("/thread/thread6", test_thread6);

  return g_test_run ();
}
