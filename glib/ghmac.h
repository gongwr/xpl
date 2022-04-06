/* ghmac.h - secure data hashing
 *
 * Copyright (C) 2011  Stef Walter  <stefw@collabora.co.uk>
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __G_HMAC_H__
#define __G_HMAC_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>
#include "gchecksum.h"

G_BEGIN_DECLS

/**
 * xhmac_t:
 *
 * An opaque structure representing a HMAC operation.
 * To create a new xhmac_t, use g_hmac_new(). To free
 * a xhmac_t, use g_hmac_unref().
 *
 * Since: 2.30
 */
typedef struct _GHmac       xhmac_t;

XPL_AVAILABLE_IN_2_30
xhmac_t *               g_hmac_new                    (GChecksumType  digest_type,
                                                     const guchar  *key,
                                                     xsize_t          key_len);
XPL_AVAILABLE_IN_2_30
xhmac_t *               g_hmac_copy                   (const xhmac_t   *hmac);
XPL_AVAILABLE_IN_2_30
xhmac_t *               g_hmac_ref                    (xhmac_t         *hmac);
XPL_AVAILABLE_IN_2_30
void                  g_hmac_unref                  (xhmac_t         *hmac);
XPL_AVAILABLE_IN_2_30
void                  g_hmac_update                 (xhmac_t         *hmac,
                                                     const guchar  *data,
                                                     xssize_t         length);
XPL_AVAILABLE_IN_2_30
const xchar_t *         g_hmac_get_string             (xhmac_t         *hmac);
XPL_AVAILABLE_IN_2_30
void                  g_hmac_get_digest             (xhmac_t         *hmac,
                                                     xuint8_t        *buffer,
                                                     xsize_t         *digest_len);

XPL_AVAILABLE_IN_2_30
xchar_t                *g_compute_hmac_for_data       (GChecksumType  digest_type,
                                                     const guchar  *key,
                                                     xsize_t          key_len,
                                                     const guchar  *data,
                                                     xsize_t          length);
XPL_AVAILABLE_IN_2_30
xchar_t                *g_compute_hmac_for_string     (GChecksumType  digest_type,
                                                     const guchar  *key,
                                                     xsize_t          key_len,
                                                     const xchar_t   *str,
                                                     xssize_t         length);
XPL_AVAILABLE_IN_2_50
xchar_t               *g_compute_hmac_for_bytes       (GChecksumType  digest_type,
                                                     xbytes_t        *key,
                                                     xbytes_t        *data);


G_END_DECLS

#endif /* __G_CHECKSUM_H__ */
