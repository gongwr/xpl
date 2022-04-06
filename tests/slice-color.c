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
#include <string.h>

#define ALIGN(size, base)       ((base) * (xsize_t) (((size) + (base) - 1) / (base)))

static xdouble_t parse_memsize (const xchar_t *cstring);
static void    usage         (void);

static void
fill_memory (xuint_t **mem,
             xuint_t   n,
             xuint_t   val)
{
  xuint_t j, o = 0;
  for (j = 0; j < n; j++)
    mem[j][o] = val;
}

static xuint64_t
access_memory3 (xuint_t  **mema,
                xuint_t  **memb,
                xuint_t  **memd,
                xuint_t    n,
                xuint64_t  repeats)
{
  xuint64_t accu = 0, i, j;
  const xuint_t o = 0;
  for (i = 0; i < repeats; i++)
    {
      for (j = 1; j < n; j += 2)
        memd[j][o] = mema[j][o] + memb[j][o];
    }
  for (i = 0; i < repeats; i++)
    for (j = 0; j < n; j++)
      accu += memd[j][o];
  return accu;
}

static void
touch_mem (xuint64_t block_size,
           xuint64_t n_blocks,
           xuint64_t repeats)
{
  xuint64_t j, accu, n = n_blocks;
  xtimer_t *timer;
  xuint_t **memc;
  xuint_t **memb;
  xuint_t **mema = g_new (xuint_t*, n);
  for (j = 0; j < n; j++)
    mema[j] = g_slice_alloc (block_size);
  memb = g_new (xuint_t*, n);
  for (j = 0; j < n; j++)
    memb[j] = g_slice_alloc (block_size);
  memc = g_new (xuint_t*, n);
  for (j = 0; j < n; j++)
    memc[j] = g_slice_alloc (block_size);

  timer = g_timer_new();
  fill_memory (mema, n, 2);
  fill_memory (memb, n, 3);
  fill_memory (memc, n, 4);
  access_memory3 (mema, memb, memc, n, 3);
  g_timer_start (timer);
  accu = access_memory3 (mema, memb, memc, n, repeats);
  g_timer_stop (timer);

  g_print ("Access-time = %fs\n", g_timer_elapsed (timer, NULL));
  g_assert (accu / repeats == (2 + 3) * n / 2 + 4 * n / 2);

  for (j = 0; j < n; j++)
    {
      g_slice_free1 (block_size, mema[j]);
      g_slice_free1 (block_size, memb[j]);
      g_slice_free1 (block_size, memc[j]);
    }
  g_timer_destroy (timer);
  g_free (mema);
  g_free (memb);
  g_free (memc);
}

static void
usage (void)
{
  g_print ("Usage: slice-color <block-size> [memory-size] [repeats] [colorization]\n");
}

int
main (int   argc,
      char *argv[])
{
  xuint64_t block_size = 512, area_size = 1024 * 1024, n_blocks, repeats = 1000000;

  if (argc > 1)
    block_size = parse_memsize (argv[1]);
  else
    {
      usage();
      block_size = 512;
    }
  if (argc > 2)
    area_size = parse_memsize (argv[2]);
  if (argc > 3)
    repeats = parse_memsize (argv[3]);
  if (argc > 4)
    g_slice_set_config (G_SLICE_CONFIG_COLOR_INCREMENT, parse_memsize (argv[4]));

  /* figure number of blocks from block and area size.
   * divide area by 3 because touch_mem() allocates 3 areas
   */
  n_blocks = area_size / 3 / ALIGN (block_size, sizeof (xsize_t) * 2);

  /* basic sanity checks */
  if (!block_size || !n_blocks || block_size >= area_size)
    {
      g_printerr ("Invalid arguments: block-size=%" G_GUINT64_FORMAT " memory-size=%" G_GUINT64_FORMAT "\n", block_size, area_size);
      usage();
      return 1;
    }

  g_printerr ("Will allocate and touch %" G_GUINT64_FORMAT " blocks of %" G_GUINT64_FORMAT " bytes (= %" G_GUINT64_FORMAT " bytes) %" G_GUINT64_FORMAT " times with color increment: 0x%08" G_GINT64_MODIFIER "x\n",
              n_blocks, block_size, n_blocks * block_size, repeats,
	      (xuint64_t)g_slice_get_config (G_SLICE_CONFIG_COLOR_INCREMENT));

  touch_mem (block_size, n_blocks, repeats);

  return 0;
}

static xdouble_t
parse_memsize (const xchar_t *cstring)
{
  xchar_t *mem = xstrdup (cstring);
  xchar_t *string = xstrstrip (mem);
  xuint_t l = strlen (string);
  xdouble_t f = 0;
  xchar_t *derr = NULL;
  xdouble_t msize;

  switch (l ? string[l - 1] : 0)
    {
    case 'k':   f = 1000;               break;
    case 'K':   f = 1024;               break;
    case 'm':   f = 1000000;            break;
    case 'M':   f = 1024 * 1024;        break;
    case 'g':   f = 1000000000;         break;
    case 'G':   f = 1024 * 1024 * 1024; break;
    }
  if (f)
    string[l - 1] = 0;
  msize = g_ascii_strtod (string, &derr);
  g_free (mem);
  if (derr && *derr)
    {
      g_printerr ("failed to parse number at: %s\n", derr);
      msize = 0;
    }
  if (f)
    msize *= f;
  return msize;
}
