/* slice-threadinit.c - test GSlice across xthread_init
 * Copyright (C) 2007 Tim Janik
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

#define N_PAGES                 (101)                   /* number of pages to sample */
#define SAMPLE_SIZE             (7)
#define PAGE_SIZE               (128)                   /* must be <= minimum GSlice alignment block */
#define MAGAZINE_PROBES         { 97, 265, 347 }        /* block sizes hopefully unused by xthread_init */
#define MAX_PROBE_TRIALS        (1031)                  /* must be >= maximum magazine size */

#define ALIGN(size, base)       ((base) * (xsize_t) (((size) + (base) - 1) / (base)))

static struct {
  void *page;
  void *sample;
} pages[N_PAGES] = { { NULL, NULL }, };

static const xuint_t magazine_probes[] = MAGAZINE_PROBES;
#define N_MAGAZINE_PROBES       G_N_ELEMENTS (magazine_probes)

static void
release_trash_list (xslist_t **trash_list,
                    xsize_t    block_size)
{
  while (*trash_list)
    {
      g_slice_free1 (block_size, (*trash_list)->data);
      *trash_list = xslist_delete_link (*trash_list, *trash_list);
    }
}

static xslist_t *free_list = NULL;

static xboolean_t
allocate_from_known_page (void)
{
  xuint_t i, j, n_trials = N_PAGES * PAGE_SIZE / SAMPLE_SIZE; /* upper bound */
  for (i = 0; i < n_trials; i++)
    {
      void *b = g_slice_alloc (SAMPLE_SIZE);
      void *p = (void*) (PAGE_SIZE * ((xsize_t) b / PAGE_SIZE));
      free_list = xslist_prepend (free_list, b);
      /* find page */
      for (j = 0; j < N_PAGES; j++)
        if (pages[j].page == p)
          return TRUE;
    }
  return FALSE;
}

int
main (int   argc,
      char *argv[])
{
  xsize_t j, n_pages = 0;
  void *mps[N_MAGAZINE_PROBES];

  /* probe some magazine sizes */
  for (j = 0; j < N_MAGAZINE_PROBES; j++)
    mps[j] = g_slice_alloc (magazine_probes[j]);
  /* mps[*] now contains pointers to allocated slices */

  /* allocate blocks from N_PAGES different pages */
  while (n_pages < N_PAGES)
    {
      void *b = g_slice_alloc (SAMPLE_SIZE);
      void *p = (void*) (PAGE_SIZE * ((xsize_t) b / PAGE_SIZE));
      for (j = 0; j < N_PAGES; j++)
        if (pages[j].page == p)
          break;
      if (j < N_PAGES)  /* known page */
        free_list = xslist_prepend (free_list, b);
      else              /* new page */
        {
          j = n_pages++;
          pages[j].page = p;
          pages[j].sample = b;
        }
    }
  /* release intermediate allocations */
  release_trash_list (&free_list, SAMPLE_SIZE);

  /* ensure that we can allocate from known pages */
  if (!allocate_from_known_page())
    xerror ("failed to allocate from magazine/page cache (before xthread_init)");
  /* release intermediate allocations */
  release_trash_list (&free_list, SAMPLE_SIZE);

  /* release magazine probes to be retained */
  for (j = 0; j < N_MAGAZINE_PROBES; j++)
    g_slice_free1 (magazine_probes[j], mps[j]);
  /* mps[*] now contains pointers to releaed slices */

  /* ensure probes were retained */
  for (j = 0; j < N_MAGAZINE_PROBES; j++)
    {
      xslist_t *trash = NULL;
      xuint_t k;
      for (k = 0; k < MAX_PROBE_TRIALS; k++)
        {
          void *mem = g_slice_alloc (magazine_probes[j]);
          if (mem == mps[j])
            break;      /* reallocated previously freed slice */
          trash = xslist_prepend (trash, mem);
        }
      release_trash_list (&trash, magazine_probes[j]);
      if (k >= MAX_PROBE_TRIALS)        /* failed to reallocate slice */
        xerror ("failed to reallocate slice from magazine (before xthread_init): size=%d", magazine_probes[j]);
    }
  /* mps[*] now contains pointers to reallocated slices */

  /* release magazine probes to be retained across xthread_init */
  for (j = 0; j < N_MAGAZINE_PROBES; j++)
    g_slice_free1 (magazine_probes[j], mps[j]);
  /* mps[*] now contains pointers to released slices */

  /* initialize threading (should retain allocator state) */
  xthread_init (NULL);

  /* ensure probes were retained */
  for (j = 0; j < N_MAGAZINE_PROBES; j++)
    {
      xslist_t *trash = NULL;
      xuint_t k;
      for (k = 0; k < MAX_PROBE_TRIALS; k++)
        {
          void *mem = g_slice_alloc (magazine_probes[j]);
          if (mem == mps[j])
            break;      /* reallocated previously freed slice */
          trash = xslist_prepend (trash, mem);
        }
      release_trash_list (&trash, magazine_probes[j]);
      if (k >= MAX_PROBE_TRIALS)        /* failed to reallocate slice */
        xerror ("failed to reallocate slice from magazine (after xthread_init): size=%d", magazine_probes[j]);
    }
  /* mps[*] now contains pointers to reallocated slices */

  /* ensure that we can allocate from known pages */
  if (!allocate_from_known_page())
    xerror ("failed to allocate from magazine/page cache (after xthread_init)");

  /* some cleanups */
  for (j = 0; j < N_MAGAZINE_PROBES; j++)
    g_slice_free1 (magazine_probes[j], mps[j]);
  release_trash_list (&free_list, SAMPLE_SIZE);

  return 0;
}
