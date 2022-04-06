/*
 * Copyright © 2009, 2010 Codethink Limited
 * Copyright © 2011 Collabora Ltd.
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
 *         Stef Walter <stefw@collabora.co.uk>
 */

#ifndef __G_BYTES_H__
#define __G_BYTES_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>
#include <glib/garray.h>

G_BEGIN_DECLS

XPL_AVAILABLE_IN_ALL
xbytes_t *        xbytes_new                     (xconstpointer   data,
                                                 xsize_t           size);

XPL_AVAILABLE_IN_ALL
xbytes_t *        xbytes_new_take                (xpointer_t        data,
                                                 xsize_t           size);

XPL_AVAILABLE_IN_ALL
xbytes_t *        xbytes_new_static              (xconstpointer   data,
                                                 xsize_t           size);

XPL_AVAILABLE_IN_ALL
xbytes_t *        xbytes_new_with_free_func      (xconstpointer   data,
                                                 xsize_t           size,
                                                 xdestroy_notify_t  free_func,
                                                 xpointer_t        user_data);

XPL_AVAILABLE_IN_ALL
xbytes_t *        xbytes_new_from_bytes          (xbytes_t         *bytes,
                                                 xsize_t           offset,
                                                 xsize_t           length);

XPL_AVAILABLE_IN_ALL
xconstpointer   xbytes_get_data                (xbytes_t         *bytes,
                                                 xsize_t          *size);

XPL_AVAILABLE_IN_ALL
xsize_t           xbytes_get_size                (xbytes_t         *bytes);

XPL_AVAILABLE_IN_ALL
xbytes_t *        xbytes_ref                     (xbytes_t         *bytes);

XPL_AVAILABLE_IN_ALL
void            xbytes_unref                   (xbytes_t         *bytes);

XPL_AVAILABLE_IN_ALL
xpointer_t        xbytes_unref_to_data           (xbytes_t         *bytes,
                                                 xsize_t          *size);

XPL_AVAILABLE_IN_ALL
xbyte_array_t *    xbytes_unref_to_array          (xbytes_t         *bytes);

XPL_AVAILABLE_IN_ALL
xuint_t           xbytes_hash                    (xconstpointer   bytes);

XPL_AVAILABLE_IN_ALL
xboolean_t        xbytes_equal                   (xconstpointer   bytes1,
                                                 xconstpointer   bytes2);

XPL_AVAILABLE_IN_ALL
xint_t            xbytes_compare                 (xconstpointer   bytes1,
                                                 xconstpointer   bytes2);

XPL_AVAILABLE_IN_2_70
xconstpointer   xbytes_get_region              (xbytes_t         *bytes,
                                                 xsize_t           element_size,
                                                 xsize_t           offset,
                                                 xsize_t           n_elements);


G_END_DECLS

#endif /* __G_BYTES_H__ */
