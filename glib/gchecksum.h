/* gchecksum.h - data hashing functions
 *
 * Copyright (C) 2007  Emmanuele Bassi  <ebassi@gnome.org>
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

#ifndef __G_CHECKSUM_H__
#define __G_CHECKSUM_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>
#include <glib/gbytes.h>

G_BEGIN_DECLS

/**
 * GChecksumType:
 * @G_CHECKSUM_MD5: Use the MD5 hashing algorithm
 * @G_CHECKSUM_SHA1: Use the SHA-1 hashing algorithm
 * @G_CHECKSUM_SHA256: Use the SHA-256 hashing algorithm
 * @G_CHECKSUM_SHA384: Use the SHA-384 hashing algorithm (Since: 2.51)
 * @G_CHECKSUM_SHA512: Use the SHA-512 hashing algorithm (Since: 2.36)
 *
 * The hashing algorithm to be used by #xchecksum_t when performing the
 * digest of some data.
 *
 * Note that the #GChecksumType enumeration may be extended at a later
 * date to include new hashing algorithm types.
 *
 * Since: 2.16
 */
typedef enum {
  G_CHECKSUM_MD5,
  G_CHECKSUM_SHA1,
  G_CHECKSUM_SHA256,
  G_CHECKSUM_SHA512,
  G_CHECKSUM_SHA384
} GChecksumType;

/**
 * xchecksum_t:
 *
 * An opaque structure representing a checksumming operation.
 *
 * To create a new xchecksum_t, use xchecksum_new(). To free
 * a xchecksum_t, use xchecksum_free().
 *
 * Since: 2.16
 */
typedef struct _GChecksum       xchecksum_t;

XPL_AVAILABLE_IN_ALL
xssize_t                xchecksum_type_get_length    (GChecksumType    checksum_type);

XPL_AVAILABLE_IN_ALL
xchecksum_t *           xchecksum_new                (GChecksumType    checksum_type);
XPL_AVAILABLE_IN_ALL
void                  xchecksum_reset              (xchecksum_t       *checksum);
XPL_AVAILABLE_IN_ALL
xchecksum_t *           xchecksum_copy               (const xchecksum_t *checksum);
XPL_AVAILABLE_IN_ALL
void                  xchecksum_free               (xchecksum_t       *checksum);
XPL_AVAILABLE_IN_ALL
void                  xchecksum_update             (xchecksum_t       *checksum,
                                                     const guchar    *data,
                                                     xssize_t           length);
XPL_AVAILABLE_IN_ALL
const xchar_t *         xchecksum_get_string         (xchecksum_t       *checksum);
XPL_AVAILABLE_IN_ALL
void                  xchecksum_get_digest         (xchecksum_t       *checksum,
                                                     xuint8_t          *buffer,
                                                     xsize_t           *digest_len);

XPL_AVAILABLE_IN_ALL
xchar_t                *g_compute_checksum_for_data   (GChecksumType    checksum_type,
                                                     const guchar    *data,
                                                     xsize_t            length);
XPL_AVAILABLE_IN_ALL
xchar_t                *g_compute_checksum_for_string (GChecksumType    checksum_type,
                                                     const xchar_t     *str,
                                                     xssize_t           length);

XPL_AVAILABLE_IN_2_34
xchar_t                *g_compute_checksum_for_bytes  (GChecksumType    checksum_type,
                                                     xbytes_t          *data);

G_END_DECLS

#endif /* __G_CHECKSUM_H__ */
