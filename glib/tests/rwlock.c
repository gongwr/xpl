/* Unit tests for GRWLock
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

/* We are testing some deprecated APIs here */
#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include <glib.h>

static void
test_rwlock1 (void)
{
  GRWLock lock;

  g_rw_lock_init (&lock);
  g_rw_lock_writer_lock (&lock);
  g_rw_lock_writer_unlock (&lock);
  g_rw_lock_writer_lock (&lock);
  g_rw_lock_writer_unlock (&lock);
  g_rw_lock_clear (&lock);
}

static void
test_rwlock2 (void)
{
  static GRWLock lock;

  g_rw_lock_writer_lock (&lock);
  g_rw_lock_writer_unlock (&lock);
  g_rw_lock_writer_lock (&lock);
  g_rw_lock_writer_unlock (&lock);
}

static void
test_rwlock3 (void)
{
  static GRWLock lock;
  xboolean_t ret;

  ret = g_rw_lock_writer_trylock (&lock);
  g_assert (ret);
  ret = g_rw_lock_writer_trylock (&lock);
  g_assert (!ret);

  g_rw_lock_writer_unlock (&lock);
}

static void
test_rwlock4 (void)
{
  static GRWLock lock;

  g_rw_lock_reader_lock (&lock);
  g_rw_lock_reader_unlock (&lock);
  g_rw_lock_reader_lock (&lock);
  g_rw_lock_reader_unlock (&lock);
}

static void
test_rwlock5 (void)
{
  static GRWLock lock;
  xboolean_t ret;

  ret = g_rw_lock_reader_trylock (&lock);
  g_assert (ret);
  ret = g_rw_lock_reader_trylock (&lock);
  g_assert (ret);

  g_rw_lock_reader_unlock (&lock);
  g_rw_lock_reader_unlock (&lock);
}

static void
test_rwlock6 (void)
{
  static GRWLock lock;
  xboolean_t ret;

  g_rw_lock_writer_lock (&lock);
  ret = g_rw_lock_reader_trylock (&lock);
  g_assert (!ret);
  g_rw_lock_writer_unlock (&lock);

  g_rw_lock_reader_lock (&lock);
  ret = g_rw_lock_writer_trylock (&lock);
  g_assert (!ret);
  g_rw_lock_reader_unlock (&lock);
}


#define LOCKS      48
#define ITERATIONS 10000
#define THREADS    100


xthread_t *owners[LOCKS];
GRWLock  locks[LOCKS];

static void
acquire (xint_t nr)
{
  xthread_t *self;

  self = xthread_self ();

  if (!g_rw_lock_writer_trylock (&locks[nr]))
    {
      if (g_test_verbose ())
        g_printerr ("thread %p going to block on lock %d\n", self, nr);

      g_rw_lock_writer_lock (&locks[nr]);
    }

  g_assert (owners[nr] == NULL);   /* hopefully nobody else is here */
  owners[nr] = self;

  /* let some other threads try to ruin our day */
  xthread_yield ();
  xthread_yield ();
  xthread_yield ();

  g_assert (owners[nr] == self);   /* hopefully this is still us... */
  owners[nr] = NULL;               /* make way for the next guy */

  g_rw_lock_writer_unlock (&locks[nr]);
}

static xpointer_t
thread_func (xpointer_t data)
{
  xint_t i;
  xrand_t *rand;

  rand = g_rand_new ();

  for (i = 0; i < ITERATIONS; i++)
    acquire (g_rand_int_range (rand, 0, LOCKS));

  g_rand_free (rand);

  return NULL;
}

static void
test_rwlock7 (void)
{
  xint_t i;
  xthread_t *threads[THREADS];

  for (i = 0; i < LOCKS; i++)
    g_rw_lock_init (&locks[i]);

  for (i = 0; i < THREADS; i++)
    threads[i] = xthread_new ("test", thread_func, NULL);

  for (i = 0; i < THREADS; i++)
    xthread_join (threads[i]);

  for (i = 0; i < LOCKS; i++)
    g_rw_lock_clear (&locks[i]);

  for (i = 0; i < LOCKS; i++)
    g_assert (owners[i] == NULL);
}

static xint_t even;
static GRWLock even_lock;
xthread_t *writers[2];
xthread_t *readers[10];

static void
change_even (xpointer_t data)
{
  g_rw_lock_writer_lock (&even_lock);

  g_assert (even % 2 == 0);

  even += 1;

  if (GPOINTER_TO_INT (data) == 0)
    even += 1;
  else
    even -= 1;

  g_assert (even % 2 == 0);

  g_rw_lock_writer_unlock (&even_lock);
}

static void
verify_even (xpointer_t data)
{
  g_rw_lock_reader_lock (&even_lock);

  g_assert (even % 2 == 0);

  g_rw_lock_reader_unlock (&even_lock);
}

static xpointer_t
writer_func (xpointer_t data)
{
  xint_t i;

  for (i = 0; i < 100000; i++)
    change_even (data);

  return NULL;
}

static xpointer_t
reader_func (xpointer_t data)
{
  xint_t i;

  for (i = 0; i < 100000; i++)
    verify_even (data);

  return NULL;
}

/* This test has 2 writers and 10 readers.
 * The writers modify an integer multiple times,
 * but always leave it with an even value.
 * The readers verify that they can only observe
 * even values
 */
static void
test_rwlock8 (void)
{
  xint_t i;

  even = 0;
  g_rw_lock_init (&even_lock);

  for (i = 0; i < 2; i++)
    writers[i] = xthread_new ("a", writer_func, GINT_TO_POINTER (i));

  for (i = 0; i < 10; i++)
    readers[i] = xthread_new ("b", reader_func, NULL);

  for (i = 0; i < 2; i++)
    xthread_join (writers[i]);

  for (i = 0; i < 10; i++)
    xthread_join (readers[i]);

  g_assert (even % 2 == 0);

  g_rw_lock_clear (&even_lock);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/thread/rwlock1", test_rwlock1);
  g_test_add_func ("/thread/rwlock2", test_rwlock2);
  g_test_add_func ("/thread/rwlock3", test_rwlock3);
  g_test_add_func ("/thread/rwlock4", test_rwlock4);
  g_test_add_func ("/thread/rwlock5", test_rwlock5);
  g_test_add_func ("/thread/rwlock6", test_rwlock6);
  g_test_add_func ("/thread/rwlock7", test_rwlock7);
  g_test_add_func ("/thread/rwlock8", test_rwlock8);

  return g_test_run ();
}
