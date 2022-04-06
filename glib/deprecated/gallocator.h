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

#ifndef __G_ALLOCATOR_H__
#define __G_ALLOCATOR_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

typedef struct _GAllocator GAllocator;
typedef struct _GMemChunk  GMemChunk;

#define G_ALLOC_ONLY                    1
#define G_ALLOC_AND_FREE                2
#define G_ALLOCATOR_LIST                1
#define G_ALLOCATOR_SLIST               2
#define G_ALLOCATOR_NODE                3

#define g_chunk_new(type, chunk)        ((type *) g_mem_chunk_alloc (chunk))
#define g_chunk_new0(type, chunk)       ((type *) g_mem_chunk_alloc0 (chunk))
#define g_chunk_free(mem, mem_chunk)    (g_mem_chunk_free (mem_chunk, mem))
#define g_mem_chunk_create(type, x, y)  (g_mem_chunk_new (NULL, sizeof (type), 0, 0))


XPL_DEPRECATED
GMemChunk *     g_mem_chunk_new         (const xchar_t  *name,
                                         xint_t          atom_size,
                                         xsize_t         area_size,
                                         xint_t          type);
XPL_DEPRECATED
void            g_mem_chunk_destroy     (GMemChunk    *mem_chunk);
XPL_DEPRECATED
xpointer_t        g_mem_chunk_alloc       (GMemChunk    *mem_chunk);
XPL_DEPRECATED
xpointer_t        g_mem_chunk_alloc0      (GMemChunk    *mem_chunk);
XPL_DEPRECATED
void            g_mem_chunk_free        (GMemChunk    *mem_chunk,
                                         xpointer_t      mem);
XPL_DEPRECATED
void            g_mem_chunk_clean       (GMemChunk    *mem_chunk);
XPL_DEPRECATED
void            g_mem_chunk_reset       (GMemChunk    *mem_chunk);
XPL_DEPRECATED
void            g_mem_chunk_print       (GMemChunk    *mem_chunk);
XPL_DEPRECATED
void            g_mem_chunk_info        (void);
XPL_DEPRECATED
void            g_blow_chunks           (void);


XPL_DEPRECATED
GAllocator *    g_allocator_new         (const xchar_t  *name,
                                         xuint_t         n_preallocs);
XPL_DEPRECATED
void            g_allocator_free        (GAllocator   *allocator);
XPL_DEPRECATED
void            xlist_push_allocator   (GAllocator   *allocator);
XPL_DEPRECATED
void            xlist_pop_allocator    (void);
XPL_DEPRECATED
void            xslist_push_allocator  (GAllocator   *allocator);
XPL_DEPRECATED
void            xslist_pop_allocator   (void);
XPL_DEPRECATED
void            g_node_push_allocator   (GAllocator   *allocator);
XPL_DEPRECATED
void            g_node_pop_allocator    (void);

G_END_DECLS

#endif /* __G_ALLOCATOR_H__ */
