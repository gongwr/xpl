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
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __G_VARIANT_SERIALISER_H__
#define __G_VARIANT_SERIALISER_H__

#include "gvarianttypeinfo.h"

typedef struct
{
  GVariantTypeInfo *type_info;
  guchar           *data;
  xsize_t             size;
  xsize_t             depth;  /* same semantics as xvariant_t.depth */
} GVariantSerialised;

/* deserialization */
XPL_AVAILABLE_IN_ALL
xsize_t                           xvariant_serialised_n_children         (GVariantSerialised        container);
XPL_AVAILABLE_IN_ALL
GVariantSerialised              xvariant_serialised_get_child          (GVariantSerialised        container,
                                                                         xsize_t                     index);

/* serialization */
typedef void                  (*GVariantSerialisedFiller)               (GVariantSerialised       *serialised,
                                                                         xpointer_t                  data);

XPL_AVAILABLE_IN_ALL
xsize_t                           xvariant_serialiser_needed_size        (GVariantTypeInfo         *info,
                                                                         GVariantSerialisedFiller  gsv_filler,
                                                                         const xpointer_t           *children,
                                                                         xsize_t                     n_children);

XPL_AVAILABLE_IN_ALL
void                            xvariant_serialiser_serialise          (GVariantSerialised        container,
                                                                         GVariantSerialisedFiller  gsv_filler,
                                                                         const xpointer_t           *children,
                                                                         xsize_t                     n_children);

/* misc */
XPL_AVAILABLE_IN_2_60
xboolean_t                        xvariant_serialised_check              (GVariantSerialised        serialised);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_serialised_is_normal          (GVariantSerialised        value);
XPL_AVAILABLE_IN_ALL
void                            xvariant_serialised_byteswap           (GVariantSerialised        value);

/* validation of strings */
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_serialiser_is_string          (xconstpointer             data,
                                                                         xsize_t                     size);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_serialiser_is_object_path     (xconstpointer             data,
                                                                         xsize_t                     size);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_serialiser_is_signature       (xconstpointer             data,
                                                                         xsize_t                     size);

#endif /* __G_VARIANT_SERIALISER_H__ */
