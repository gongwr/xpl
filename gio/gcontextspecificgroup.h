/*
 * Copyright © 2015 Canonical Limited
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
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __G_CONTEXT_SPECIFIC_GROUP_H__
#define __G_CONTEXT_SPECIFIC_GROUP_H__

#include <glib-object.h>

typedef struct
{
  xhashtable_t *table;
  xmutex_t      lock;
  xcond_t       cond;
  xboolean_t    requested_state;
  xcallback_t   requested_func;
  xboolean_t    effective_state;
} GContextSpecificGroup;

xpointer_t
g_context_specific_group_get (GContextSpecificGroup *group,
                              xtype_t                  type,
                              xoffset_t                context_offset,
                              xcallback_t              start_func);

void
g_context_specific_group_remove (GContextSpecificGroup *group,
                                 xmain_context_t          *context,
                                 xpointer_t               instance,
                                 xcallback_t              stop_func);

void
g_context_specific_group_emit (GContextSpecificGroup *group,
                               xuint_t                  signal_id);

#endif /* __G_CONTEXT_SPECIFIC_GROUP_H__ */
