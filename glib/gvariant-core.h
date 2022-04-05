/*
 * Copyright © 2007, 2008 Ryan Lortie
 * Copyright © 2010 Codethink Limited
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

#ifndef __G_VARIANT_CORE_H__
#define __G_VARIANT_CORE_H__

#include <glib/gvarianttypeinfo.h>
#include <glib/gvariant.h>
#include <glib/gbytes.h>

/* gvariant-core.c */

xvariant_t *              g_variant_new_from_children                     (const xvariant_type_t  *type,
                                                                         xvariant_t           **children,
                                                                         xsize_t                n_children,
                                                                         xboolean_t             trusted);

xboolean_t                g_variant_is_trusted                            (xvariant_t            *value);

GVariantTypeInfo *      g_variant_get_type_info                         (xvariant_t            *value);

xsize_t                   g_variant_get_depth                             (xvariant_t            *value);

#endif /* __G_VARIANT_CORE_H__ */
