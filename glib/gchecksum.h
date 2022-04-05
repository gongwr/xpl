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
 * The hashing algorithm to be used by #GChecksum when performing the
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
 * GChecksum:
 *
 * An opaque structure representing a checksumming operation.
 *
 * To create a new GChecksum, use g_checksum_new(). To free
 * a GChecksum, use g_checksum_free().
 *
 * Since: 2.16
 */
typedef struct _GChecksum       GChecksum;

XPL_AVAILABLE_IN_ALL
gssize                g_checksum_type_get_length    (GChecksumType    checksum_type);

XPL_AVAILABLE_IN_ALL
GChecksum *           g_checksum_new                (GChecksumType    checksum_type);
XPL_AVAILABLE_IN_ALL
void                  g_checksum_reset              (GChecksum       *checksum);
XPL_AVAILABLE_IN_ALL
GChecksum *           g_checksum_copy               (const GChecksum *checksum);
XPL_AVAILABLE_IN_ALL
void                  g_checksum_free               (GChecksum       *checksum);
XPL_AVAILABLE_IN_ALL
void                  g_checksum_update             (GChecksum       *checksum,
                                                     const guchar    *data,
                                                     gssize           length);
XPL_AVAILABLE_IN_ALL
const xchar_t *         g_checksum_get_string         (GChecksum       *checksum);
XPL_AVAILABLE_IN_ALL
void                  g_checksum_get_digest         (GChecksum       *checksum,
                                                     guint8          *buffer,
                                                     xsize_t           *digest_len);

XPL_AVAILABLE_IN_ALL
xchar_t                *g_compute_checksum_for_data   (GChecksumType    checksum_type,
                                                     const guchar    *data,
                                                     xsize_t            length);
XPL_AVAILABLE_IN_ALL
xchar_t                *g_compute_checksum_for_string (GChecksumType    checksum_type,
                                                     const xchar_t     *str,
                                                     gssize           length);

XPL_AVAILABLE_IN_2_34
xchar_t                *g_compute_checksum_for_bytes  (GChecksumType    checksum_type,
                                                     GBytes          *data);

G_END_DECLS

#endif /* __G_CHECKSUM_H__ */
