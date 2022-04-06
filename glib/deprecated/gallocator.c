/*
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

#include "config.h"

/* we know we are deprecated here, no need for warnings */
#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include "gallocator.h"

#include <glib/gmessages.h>
#include <glib/gslice.h>

struct _GMemChunk {
  xuint_t alloc_size;           /* the size of an atom */
};

GMemChunk*
g_mem_chunk_new (const xchar_t *name,
                 xint_t         atom_size,
                 xsize_t        area_size,
                 xint_t         type)
{
  GMemChunk *mem_chunk;

  g_return_val_if_fail (atom_size > 0, NULL);

  mem_chunk = g_slice_new (GMemChunk);
  mem_chunk->alloc_size = atom_size;

  return mem_chunk;
}

void
g_mem_chunk_destroy (GMemChunk *mem_chunk)
{
  g_return_if_fail (mem_chunk != NULL);

  g_slice_free (GMemChunk, mem_chunk);
}

xpointer_t
g_mem_chunk_alloc (GMemChunk *mem_chunk)
{
  g_return_val_if_fail (mem_chunk != NULL, NULL);

  return g_slice_alloc (mem_chunk->alloc_size);
}

xpointer_t
g_mem_chunk_alloc0 (GMemChunk *mem_chunk)
{
  g_return_val_if_fail (mem_chunk != NULL, NULL);

  return g_slice_alloc0 (mem_chunk->alloc_size);
}

void
g_mem_chunk_free (GMemChunk *mem_chunk,
                  xpointer_t   mem)
{
  g_return_if_fail (mem_chunk != NULL);

  g_slice_free1 (mem_chunk->alloc_size, mem);
}

GAllocator*
g_allocator_new (const xchar_t *name,
                 xuint_t        n_preallocs)
{
  /* some (broken) GAllocator uses depend on non-NULL allocators */
  return (void *) 1;
}

void g_allocator_free           (GAllocator *allocator) { }

void g_mem_chunk_clean          (GMemChunk *mem_chunk)  { }
void g_mem_chunk_reset          (GMemChunk *mem_chunk)  { }
void g_mem_chunk_print          (GMemChunk *mem_chunk)  { }
void g_mem_chunk_info           (void)                  { }
void g_blow_chunks              (void)                  { }

void xlist_push_allocator      (GAllocator *allocator) { }
void xlist_pop_allocator       (void)                  { }

void xslist_push_allocator     (GAllocator *allocator) { }
void xslist_pop_allocator      (void)                  { }

void g_node_push_allocator      (GAllocator *allocator) { }
void g_node_pop_allocator       (void)                  { }
