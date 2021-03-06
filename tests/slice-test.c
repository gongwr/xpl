/* XPL sliced memory - fast threaded memory chunk allocator
 * Copyright (C) 2005 Tim Janik
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#include <glib.h>

#include <stdio.h>
#include <string.h>

#define quick_rand32()  (rand_accu = 1664525 * rand_accu + 1013904223, rand_accu)
static xuint_t    prime_size = 1021; /* 769; 509 */
static xboolean_t clean_memchunks = FALSE;
static xuint_t    number_of_blocks = 10000;          /* total number of blocks allocated */
static xuint_t    number_of_repetitions = 10000;     /* number of alloc+free repetitions */
static xboolean_t want_corruption = FALSE;

/* --- old memchunk prototypes (memchunks.c) --- */
GMemChunk*      old_mem_chunk_new       (const xchar_t  *name,
                                         xulong_t        atom_size,
                                         xulong_t        area_size,
                                         xint_t          type);
void            old_mem_chunk_destroy   (GMemChunk *mem_chunk);
xpointer_t        old_mem_chunk_alloc     (GMemChunk *mem_chunk);
xpointer_t        old_mem_chunk_alloc0    (GMemChunk *mem_chunk);
void            old_mem_chunk_free      (GMemChunk *mem_chunk,
                                         xpointer_t   mem);
void            old_mem_chunk_clean     (GMemChunk *mem_chunk);
void            old_mem_chunk_reset     (GMemChunk *mem_chunk);
void            old_mem_chunk_print     (GMemChunk *mem_chunk);
void            old_mem_chunk_info      (void);
#ifndef G_ALLOC_AND_FREE
#define G_ALLOC_AND_FREE  2
#endif

/* --- functions --- */
static inline int
corruption (void)
{
  if (G_UNLIKELY (want_corruption))
    {
      /* corruption per call likelyness is about 1:4000000 */
      xuint32_t r = g_random_int() % 8000009;
      return r == 277 ? +1 : r == 281 ? -1 : 0;
    }
  return 0;
}

static inline xpointer_t
memchunk_alloc (GMemChunk **memchunkp,
                xuint_t       size)
{
  size = MAX (size, 1);
  if (G_UNLIKELY (!*memchunkp))
    *memchunkp = old_mem_chunk_new ("", size, 4096, G_ALLOC_AND_FREE);
  return old_mem_chunk_alloc (*memchunkp);
}

static inline void
memchunk_free (GMemChunk *memchunk,
               xpointer_t   chunk)
{
  old_mem_chunk_free (memchunk, chunk);
  if (clean_memchunks)
    old_mem_chunk_clean (memchunk);
}

static xpointer_t
test_memchunk_thread (xpointer_t data)
{
  GMemChunk **memchunks;
  xuint_t i, j;
  xuint8_t **ps;
  xuint_t   *ss;
  xuint32_t rand_accu = 2147483563;
  /* initialize random numbers */
  if (data)
    rand_accu = *(xuint32_t*) data;
  else
    {
      GTimeVal rand_tv;
      g_get_current_time (&rand_tv);
      rand_accu = rand_tv.tv_usec + (rand_tv.tv_sec << 16);
    }

  /* prepare for memchunk creation */
  memchunks = g_newa0 (GMemChunk*, prime_size);

  ps = g_new (xuint8_t*, number_of_blocks);
  ss = g_new (xuint_t, number_of_blocks);
  /* create number_of_blocks random sizes */
  for (i = 0; i < number_of_blocks; i++)
    ss[i] = quick_rand32() % prime_size;
  /* allocate number_of_blocks blocks */
  for (i = 0; i < number_of_blocks; i++)
    ps[i] = memchunk_alloc (&memchunks[ss[i]], ss[i]);
  for (j = 0; j < number_of_repetitions; j++)
    {
      /* free number_of_blocks/2 blocks */
      for (i = 0; i < number_of_blocks; i += 2)
        memchunk_free (memchunks[ss[i]], ps[i]);
      /* allocate number_of_blocks/2 blocks with new sizes */
      for (i = 0; i < number_of_blocks; i += 2)
        {
          ss[i] = quick_rand32() % prime_size;
          ps[i] = memchunk_alloc (&memchunks[ss[i]], ss[i]);
        }
    }
  /* free number_of_blocks blocks */
  for (i = 0; i < number_of_blocks; i++)
    memchunk_free (memchunks[ss[i]], ps[i]);
  /* alloc and free many equally sized chunks in a row */
  for (i = 0; i < number_of_repetitions; i++)
    {
      xuint_t sz = quick_rand32() % prime_size;
      xuint_t k = number_of_blocks / 100;
      for (j = 0; j < k; j++)
        ps[j] = memchunk_alloc (&memchunks[sz], sz);
      for (j = 0; j < k; j++)
        memchunk_free (memchunks[sz], ps[j]);
    }
  /* cleanout memchunks */
  for (i = 0; i < prime_size; i++)
    if (memchunks[i])
      old_mem_chunk_destroy (memchunks[i]);
  g_free (ps);
  g_free (ss);

  return NULL;
}

static xpointer_t
test_sliced_mem_thread (xpointer_t data)
{
  xuint32_t rand_accu = 2147483563;
  xuint_t i, j;
  xuint8_t **ps;
  xuint_t   *ss;

  /* initialize random numbers */
  if (data)
    rand_accu = *(xuint32_t*) data;
  else
    {
      GTimeVal rand_tv;
      g_get_current_time (&rand_tv);
      rand_accu = rand_tv.tv_usec + (rand_tv.tv_sec << 16);
    }

  ps = g_new (xuint8_t*, number_of_blocks);
  ss = g_new (xuint_t, number_of_blocks);
  /* create number_of_blocks random sizes */
  for (i = 0; i < number_of_blocks; i++)
    ss[i] = quick_rand32() % prime_size;
  /* allocate number_of_blocks blocks */
  for (i = 0; i < number_of_blocks; i++)
    ps[i] = g_slice_alloc (ss[i] + corruption());
  for (j = 0; j < number_of_repetitions; j++)
    {
      /* free number_of_blocks/2 blocks */
      for (i = 0; i < number_of_blocks; i += 2)
        g_slice_free1 (ss[i] + corruption(), ps[i] + corruption());
      /* allocate number_of_blocks/2 blocks with new sizes */
      for (i = 0; i < number_of_blocks; i += 2)
        {
          ss[i] = quick_rand32() % prime_size;
          ps[i] = g_slice_alloc (ss[i] + corruption());
        }
    }
  /* free number_of_blocks blocks */
  for (i = 0; i < number_of_blocks; i++)
    g_slice_free1 (ss[i] + corruption(), ps[i] + corruption());
  /* alloc and free many equally sized chunks in a row */
  for (i = 0; i < number_of_repetitions; i++)
    {
      xuint_t sz = quick_rand32() % prime_size;
      xuint_t k = number_of_blocks / 100;
      for (j = 0; j < k; j++)
        ps[j] = g_slice_alloc (sz + corruption());
      for (j = 0; j < k; j++)
        g_slice_free1 (sz + corruption(), ps[j] + corruption());
    }
  g_free (ps);
  g_free (ss);

  return NULL;
}

static void
usage (void)
{
  g_print ("Usage: slice-test [n_threads] [G|S|M|O][f][c][~] [maxblocksize] [seed]\n");
}

int
main (int   argc,
      char *argv[])
{
  xuint_t seed32, *seedp = NULL;
  xboolean_t ccounters = FALSE, use_memchunks = FALSE;
  xuint_t n_threads = 1;
  const xchar_t *mode = "slab allocator + magazine cache", *emode = " ";
  if (argc > 1)
    n_threads = g_ascii_strtoull (argv[1], NULL, 10);
  if (argc > 2)
    {
      xuint_t i, l = strlen (argv[2]);
      for (i = 0; i < l; i++)
        switch (argv[2][i])
          {
          case 'G': /* GLib mode */
            g_slice_set_config (G_SLICE_CONFIG_ALWAYS_MALLOC, FALSE);
            g_slice_set_config (G_SLICE_CONFIG_BYPASS_MAGAZINES, FALSE);
            mode = "slab allocator + magazine cache";
            break;
          case 'S': /* slab mode */
            g_slice_set_config (G_SLICE_CONFIG_ALWAYS_MALLOC, FALSE);
            g_slice_set_config (G_SLICE_CONFIG_BYPASS_MAGAZINES, TRUE);
            mode = "slab allocator";
            break;
          case 'M': /* malloc mode */
            g_slice_set_config (G_SLICE_CONFIG_ALWAYS_MALLOC, TRUE);
            mode = "system malloc";
            break;
          case 'O': /* old memchunks */
            use_memchunks = TRUE;
            mode = "old memchunks";
            break;
          case 'f': /* eager freeing */
            g_slice_set_config (G_SLICE_CONFIG_WORKING_SET_MSECS, 0);
            clean_memchunks = TRUE;
            emode = " with eager freeing";
            break;
          case 'c': /* print contention counters */
            ccounters = TRUE;
            break;
          case '~':
            want_corruption = TRUE; /* force occasional corruption */
            break;
          default:
            usage();
            return 1;
          }
    }
  if (argc > 3)
    prime_size = g_ascii_strtoull (argv[3], NULL, 10);
  if (argc > 4)
    {
      seed32 = g_ascii_strtoull (argv[4], NULL, 10);
      seedp = &seed32;
    }

  if (argc <= 1)
    usage();

  {
    xchar_t strseed[64] = "<random>";
    xthread_t **threads;
    xuint_t i;

    if (seedp)
      g_snprintf (strseed, 64, "%u", *seedp);
    g_print ("Starting %d threads allocating random blocks <= %u bytes with seed=%s using %s%s\n", n_threads, prime_size, strseed, mode, emode);

    threads = g_alloca (sizeof(xthread_t*) * n_threads);
    if (!use_memchunks)
      for (i = 0; i < n_threads; i++)
        threads[i] = xthread_create (test_sliced_mem_thread, seedp, TRUE, NULL);
    else
      {
        for (i = 0; i < n_threads; i++)
          threads[i] = xthread_create (test_memchunk_thread, seedp, TRUE, NULL);
      }
    for (i = 0; i < n_threads; i++)
      xthread_join (threads[i]);

    if (ccounters)
      {
        xuint_t n, n_chunks = g_slice_get_config (G_SLICE_CONFIG_CHUNK_SIZES);
        g_print ("    ChunkSize | MagazineSize | Contention\n");
        for (i = 0; i < n_chunks; i++)
          {
            sint64_t *vals = g_slice_get_config_state (G_SLICE_CONFIG_CONTENTION_COUNTER, i, &n);
            g_print ("  %9" G_GINT64_FORMAT "   |  %9" G_GINT64_FORMAT "   |  %9" G_GINT64_FORMAT "\n", vals[0], vals[2], vals[1]);
            g_free (vals);
          }
      }
    else
      g_print ("Done.\n");
    return 0;
  }
}
